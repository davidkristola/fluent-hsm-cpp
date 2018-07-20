
#ifndef kv_fhsm_StateMachine_h
#define kv_fhsm_StateMachine_h

#include "State.h"

#include <cstdio>
#include <string>
#include <iostream>
#include <array>
#include <map>
#include <exception>

namespace kv {
namespace fhsm {

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
   StateMachine(Actor& actor) : m_actor(actor), m_current(StateToIndex(first)) {
      for (IndexType i=0; i<COUNT; i++) {
         m_states[i].Initialize(&m_actor, IndexToState(i), this);
      }
   }

   ParentSetter DefineState(StateSpace state) {
      return ParentSetter(*this, StateToIndex(state));
   }

   void ConcludeSetupAndSetInitialState(StateSpace initial, StateChangeCallback noteState=nullptr) {
      m_noteState = noteState;
      m_current = StateToIndex(initial);
      InformActorOfCurrentState();
      EnterParentOf(m_current);
   }

   void Tick() {
      StateRef(m_current).OnTick();
   }
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
      if (!m_states[source].IsParentSet()) return UNKNOWN;
      if (!m_states[destination].IsParentSet()) return UNKNOWN;
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

#endif
