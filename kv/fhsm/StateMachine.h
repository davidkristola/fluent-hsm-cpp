
#ifndef kv_fhsm_StateMachine_h
#define kv_fhsm_StateMachine_h

#include "State.h"

#include <array>
#include <exception>

#define NOT !

namespace kv {
namespace fhsm {

//! Fluent style hierarchical state machine.
//! Actor is the class using the state machine.
//! StateSpace is the type (convertable to size_t) that defines the states.
//! SignalSpace is the type (convertable to int) that defines state transition events.
//
template<class Actor, typename StateSpace, const StateSpace first, const StateSpace last, typename SignalSpace>
class StateMachine {
public:
   using BoundState = State<Actor, StateSpace, StateMachine, SignalSpace>;
   using IndexType = size_t;
   static const IndexType FIRST{static_cast<IndexType>(first)};
   static const IndexType LAST{static_cast<IndexType>(last)};
   static const IndexType COUNT{LAST - FIRST + 1};
   static const IndexType UNKNOWN{COUNT + 1};
   using MethodPointer = void(Actor::*)();
   using StateChangeCallback = void(Actor::*)(const StateSpace s);

   // Hierarchical states must form trees or forrests: no cycles!
   //
   class CyclicGraphException : public std::exception {};

private:
   class ParentSetter {
      StateMachine& m_sm;
      IndexType m_index;
      friend StateMachine;
      ParentSetter(StateMachine& sm, IndexType i) : m_sm(sm), m_index(i) {}
   public:
      BoundState& SetParent(StateSpace p) {
         auto parent = m_sm.StateToIndex(p);
         if (m_sm.IsAncestorOf(m_index, parent)) {
            throw CyclicGraphException();
         }
         return m_sm.StateRef(m_index).SetParent(parent);
      }
      BoundState& SetNoParent() {
         return m_sm.StateRef(m_index).SetParent(COUNT);
      }
   };

public:
   //! Create a state machine object.
   StateMachine(Actor& actor) : m_actor(actor), m_current(StateToIndex(first)) {
      for (IndexType i=0; i<COUNT; i++) {
         m_states[i].Initialize(&m_actor, IndexToState(i), this);
      }
   }

   //! Start the process of defining a state (to be called for each state).
   //! This returns a helper class that requires you to set a parent state
   //! or affirm that there is no parent state.
   ParentSetter DefineState(StateSpace state) {
      return ParentSetter(*this, StateToIndex(state));
   }

   //! Once all of the states have been defined, this completes the process.
   //! Provide an optional method to receive state change notifications.
   void ConcludeSetupAndSetInitialState(StateSpace initial, StateChangeCallback noteState=nullptr) {
      m_noteState = noteState;
      m_current = StateToIndex(initial);
      InformActorOfCurrentState();
      EnterParentOf(m_current);
   }

   //! Tick (or step if you like) the current active state.
   void Tick() {
      StateRef(m_current).OnTick();
   }

   //! Send a state transition event/signal to the current active state.
   void Signal(const SignalSpace s) {
      StateRef(m_current).OnSignal(s);
   }

private:
   friend BoundState;
    
   StateSpace IndexToState(const IndexType i) const { return static_cast<StateSpace>(i + FIRST); }
   IndexType StateToIndex(const StateSpace s) const { return static_cast<IndexType>(s) - FIRST; }
   BoundState& StateRef(StateSpace state) {
      return m_states[StateToIndex(state)];
   }
   BoundState& StateRef(IndexType i) {
      return m_states[i];
   }

   void InformActorOfCurrentState() {
      if (m_noteState) {
         (m_actor.*m_noteState)(IndexToState(m_current));
      }
   }
   void EnterParentOf(IndexType i) {
      auto current = m_states[i];
      if (current.HasParent()) {
         EnterParentOf(current.GetParent());
      }
      current.OnEnter();
   }
   IndexType ExecuteTransition(IndexType destination, IndexType leastCommonAncestor=UNKNOWN) {
      IndexType lca = leastCommonAncestor;
      if (UNKNOWN == lca) {
         lca = LeastCommonAncestor(m_current, destination);
      }
      ExitHereToLCA(m_current, lca);
      EnterLCAToHere(lca, destination);
      m_current = destination;
      InformActorOfCurrentState();
      return lca;
   }
   IndexType LeastCommonAncestor(IndexType source, IndexType destination) {
      if (source == destination) return source; // Don't care about a parent in this case.
      if ( NOT m_states[source].IsParentSet()) return UNKNOWN;
      if ( NOT m_states[destination].IsParentSet()) return UNKNOWN;
      if (IsAncestorOf(source, destination)) return source;
      if (m_states[source].GetParent() == COUNT) return COUNT; // no common ancestor
      return LeastCommonAncestor(m_states[source].GetParent(), destination);
   }
   bool IsAncestorOf(IndexType candidate, IndexType child) {
      if (candidate == child) return true;
      if (m_states[child].GetParent() == COUNT) return false;
      return IsAncestorOf(candidate, m_states[child].GetParent());
   }
   void ExitHereToLCA(IndexType here, IndexType lca) {
      if (here == COUNT) return;
      if (here == lca) return;
      m_states[here].OnExit();
      if (m_states[here].GetParent() != lca) {
         ExitHereToLCA(m_states[here].GetParent(), lca);
      }
   }
   void EnterLCAToHere(IndexType lca, IndexType here) {
      if (here == COUNT) return;
      if (here == lca) return;
      if (m_states[here].GetParent() != lca) {
         EnterLCAToHere(lca, m_states[here].GetParent());
      }
      m_states[here].OnEnter();
   }

   Actor& m_actor;
   std::array<BoundState, COUNT> m_states;
   IndexType m_current;
   StateChangeCallback m_noteState{nullptr};
};

} // namespace fhsm
} // namespace kv

#undef NOT

#endif
