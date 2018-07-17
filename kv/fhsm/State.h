
#ifndef kv_fhsm_State_h
#define kv_fhsm_State_h

#include <cstdio>
#include <string>
#include <iostream>
#include <array>
#include <map>
#include <exception>

namespace kv {
namespace fhsm {
   
     template<class Actor, typename StateSpace, class StateMachine, typename SignalSpace>
  class State {
  public:
    using MethodPointer = void(Actor::*)();
    using AllowPointer = bool(Actor::*)()const;
    using BoundState = State<Actor, StateSpace, StateMachine, SignalSpace>;
    using IndexType = typename StateMachine::IndexType;

  private:
    StateMachine* m_sm;
    Actor* m_actor;
    StateSpace m_value;
    IndexType m_parent{StateMachine::COUNT}; //TODO(djk): figure out why this can't be UNKNOWN
    bool m_hasParent = false;
    bool m_parentIsSet = false;
    MethodPointer m_onEnter = nullptr;
    MethodPointer m_onTick = nullptr;
    MethodPointer m_onExit = nullptr;

    struct Trans {
      IndexType m_destination;
      IndexType leastCommonAncestor;
      AllowPointer allow = nullptr;

      Trans() : m_destination(StateMachine::COUNT), leastCommonAncestor(StateMachine::UNKNOWN), allow(nullptr) {}
      Trans(IndexType d) : m_destination(d), leastCommonAncestor(StateMachine::UNKNOWN), allow(nullptr) {}
      Trans(IndexType d, AllowPointer allow) : m_destination(d), leastCommonAncestor(StateMachine::UNKNOWN), allow(allow) {}
      IndexType GetDestination() const { return m_destination; }
      IndexType GetLCA() const { return leastCommonAncestor; }
      void SetLCA(IndexType lca) { leastCommonAncestor = lca; }
    };

    std::map<int, Trans> m_transitions;
    std::map<int, MethodPointer> m_actions;
    
    SignalSpace IntToSignal(int s) const { return static_cast<SignalSpace>(s); }
    int SignalToInt(SignalSpace s) const { return static_cast<int>(s); }

    class SignalSetter {
      BoundState& m_s;
      SignalSpace m_signal;
      friend BoundState;
      SignalSetter(BoundState& state, SignalSpace signal) : m_s(state), m_signal(signal) {}
    public:
      BoundState& GoTo(StateSpace dest) {
         return m_s.AddTransition(m_signal, dest, nullptr);
      }
      BoundState& GoToIf(StateSpace dest, AllowPointer allow) {
         return m_s.AddTransition(m_signal, dest, allow);
      }
      BoundState& Do(MethodPointer action) {
         return m_s.AddAction(m_signal, action);
      }
    };


  public:
    State() {}
    virtual ~State() = default;
    void Initialize(Actor* actor, const StateSpace value, StateMachine* hsm) {
      m_actor = actor;
      m_value = value;
      m_sm = hsm;
    }

    // Initialization methods return self reference so they can be chained.
    BoundState& SetOnEnter(MethodPointer onEnter) {
      m_onEnter = onEnter;
      return *this;
    }
    BoundState& SetOnTick(MethodPointer onTick) {
      m_onTick = onTick;
      return *this;
    }
    BoundState& SetOnExit(MethodPointer onExit) {
      m_onExit = onExit;
      return *this;
    }

    SignalSetter ForSignal(SignalSpace signal) {
      return SignalSetter(*this, signal);
    }

  private:
    friend SignalSetter;
    friend StateMachine; //TODO(djk): see if there is a clean way to eliminate this.

    BoundState& AddTransition(SignalSpace signal, StateSpace destination, AllowPointer allow) {
      auto there = m_sm->StateToIndex(destination);
      auto here = m_sm->StateToIndex(m_value);
      Trans t{there, allow};
      t.SetLCA(m_sm->LeastCommonAncestor(here, there));
      auto s = SignalToInt(signal);
      m_transitions[s] = t;
      return *this;
    }
    BoundState& AddAction(SignalSpace signal, MethodPointer onSignal) {
      m_actions.insert(std::pair<int, MethodPointer>(SignalToInt(signal), onSignal));
      return *this;
    }

    BoundState& SetParent(IndexType p) {
      m_parent = p;
      m_hasParent = (p != StateMachine::COUNT);
      m_parentIsSet = true;
      return *this;
    }

    bool HasParent() const { return m_hasParent; }
    bool IsParentSet() const { return m_parentIsSet; }
    IndexType GetParent() const { return m_parent; }

    void OnEnter() {
      if (m_onEnter) {
        (m_actor->*m_onEnter)();
      }
    }
    void OnExit() {
      if (m_onExit) {
        (m_actor->*m_onExit)();
      }
    }
    void OnTick() {
      if (m_onTick) {
        (m_actor->*m_onTick)();
      } else if (HasParent()) {
         m_sm->StateRef(GetParent()).OnTick();
      }
    }
    void OnSignal(const SignalSpace sig) {
      auto s = SignalToInt(sig);
      auto consumed = false;
      OnSignalDoActionIf(s, consumed);
      OnSignalDoTransitionIf(s, consumed);
      ElevateIfNotConsumed(sig, consumed);
    }
    void OnSignalDoActionIf(int s, bool& consumed) {
      if (m_actions.count(s)) {
         auto action = m_actions[s];
         if (action) {
            (m_actor->*(action))();
         }
         consumed = true;
      }
    }
    void OnSignalDoTransitionIf(int s, bool& consumed) {
      if (m_transitions.count(s)) {
        auto t = m_transitions[s];
        auto isAllowed = true;
        if (t.allow) { // If a guard was set, check it
           isAllowed = (m_actor->*(t.allow))();
        }
        if (isAllowed) {
           t.SetLCA(m_sm->ExecuteTransition(t.GetDestination(), t.GetLCA()));
        }
        consumed = true;
      }
    }
    void ElevateIfNotConsumed(SignalSpace sig, bool consumed) {
      if (!consumed && HasParent()) {
         // Try parent
         m_sm->StateRef(GetParent()).OnSignal(sig);
      }
    }
  };

   
} // namespace fhsm
} // namespace kv

#endif
