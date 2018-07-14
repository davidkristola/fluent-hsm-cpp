
#include <cstdio>
#include <string>
#include <iostream>
#include <array>
#include <map>
#include <exception>

#define TEST_EQ(expected, actual) \
  { \
    auto e = (expected); \
    std::string e_image{ #expected }; \
    auto a = (actual); \
    std::string a_image{ #actual }; \
    if (!(a == e)) { \
      std::cout << "TEST_EQ failed " << __FUNCTION__ \
      << " on line " << __LINE__ << "! expected:" << \
      e_image << " (" << e << ") != actual:" << a_image \
      << " (" << a << ")" << std::endl; \
    } \
  }

namespace {

  template<class Actor, typename StateSpace, class Container, typename SignalSpace>
  class State {
  public:
    using MethodPointer = void(Actor::*)();
    using AllowPointer = bool(Actor::*)()const;
    using BoundState = State<Actor, StateSpace, Container, SignalSpace>;
    using IndexType = typename Container::IndexType;

  private:
    Container* m_c;
    Actor* m_actor;
    StateSpace m_value;
    IndexType m_parent{Container::COUNT};
    bool m_hasParent = false;
    MethodPointer m_onEnter = nullptr;
    MethodPointer m_onTick = nullptr;
    MethodPointer m_onExit = nullptr;

    struct Trans {
      StateSpace m_destination;
      IndexType leastCommonAncestor;
      AllowPointer allow = nullptr;

      Trans() : m_destination(static_cast<StateSpace>(0)), leastCommonAncestor(Container::COUNT), allow(nullptr) {}
      Trans(StateSpace d) : m_destination(d), leastCommonAncestor(Container::COUNT), allow(nullptr) {}
      Trans(StateSpace d, AllowPointer allow) : m_destination(d), leastCommonAncestor(Container::COUNT), allow(allow) {}
      StateSpace GetDestination() const { return m_destination; }
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
         return m_s.AddTransition(m_signal, dest);
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
    void Initialize(Actor* actor, const StateSpace value, Container* c) {
      m_actor = actor;
      m_value = value;
      m_c = c;
    }

    // Initialization methods return self reference so they can be chained.
    BoundState& SetUp(MethodPointer onEnter, MethodPointer onTick, MethodPointer onExit) {
      m_onEnter = onEnter;
      m_onTick = onTick;
      m_onExit = onExit;
      return *this;
    }
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

    BoundState& AddTransition(SignalSpace signal, StateSpace destination) {
      Trans t{destination};
      m_transitions.insert(std::pair<int, Trans>(SignalToInt(signal), t));
      return *this;
    }
    BoundState& AddTransition(SignalSpace signal, StateSpace destination, AllowPointer allow) {
      Trans t{destination, allow};
      m_transitions.insert(std::pair<int, Trans>(SignalToInt(signal), t));
      return *this;
    }
    BoundState& AddAction(SignalSpace signal, MethodPointer onSignal) {
      m_actions.insert(std::pair<int, MethodPointer>(SignalToInt(signal), onSignal));
      return *this;
    }

    BoundState& SetParent(StateSpace p) {
      m_parent = m_c->StateToIndex(p);
      m_hasParent = true;
      return *this;
    }

    bool HasParent() const { return m_hasParent; }
    IndexType GetParent() const { return m_parent; }

    void OnEnter() {
      //std::cout << "OnEnter for " << m_value << std::endl;
      if (m_onEnter) {
        (m_actor->*m_onEnter)();
      }
    }
    void OnExit() {
      //std::cout << "OnExit for " << m_value << std::endl;
      if (m_onExit) {
        (m_actor->*m_onExit)();
      }
    }
    void OnTick() {
      //std::cout << "OnTick for " << m_value << std::endl;
      if (m_onTick) {
        (m_actor->*m_onTick)();
      } else if (HasParent()) {
         m_c->StateRef(GetParent()).OnTick();
      }
    }
    void OnSignal(const SignalSpace sig) {
      auto s = SignalToInt(sig);
      //std::cout << "OnSignal for " << m_value << " with signal " << s << std::endl;
      auto consumed = false;
      OnSignalDoActionIf(s, consumed);
      OnSignalDoTransitionIf(s, consumed);
      ElevateIfNotConsumed(sig, consumed);
    }
    void OnSignalDoActionIf(int s, bool& consumed) {
      if (m_actions.count(s)) {
         auto action = m_actions[s];
         //std::cout << "Performing action for signal " << s << std::endl;
         if (action) {
            (m_actor->*(action))();
         }
         consumed = true;
      }
    }
    void OnSignalDoTransitionIf(int s, bool& consumed) {
      if (m_transitions.count(s)) {
        auto t = m_transitions[s];
        //std::cout << "Going to " << t.destination << std::endl;
        auto isAllowed = true;
        if (t.allow) { // If a guard was set, check it
           isAllowed = (m_actor->*(t.allow))();
        }
        if (isAllowed) {
           m_c->ExecuteTransition(m_c->StateToIndex(t.GetDestination()));
        }
        consumed = true;
      }
    }
    void ElevateIfNotConsumed(SignalSpace sig, bool consumed) {
      if (!consumed && HasParent()) {
         // Try parent
         m_c->StateRef(GetParent()).OnSignal(sig);
      }
    }
  };

  template<class Actor, typename StateSpace, const StateSpace first, const StateSpace last, typename SignalSpace>
  class StateMachine {
  public:
    using BoundState = State<Actor, StateSpace, StateMachine, SignalSpace>;
    using IndexType = size_t;
    static const IndexType FIRST{static_cast<IndexType>(first)};
    static const IndexType LAST{static_cast<IndexType>(last)};
    static const IndexType COUNT{LAST - FIRST + 1};
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
         return m_sm.StateRef(m_index).SetParent(p);
      }
      BoundState& SetNoParent() {
         return m_sm.StateRef(m_index);
      }
    };
         
  public:

    StateMachine(Actor& actor) : m_actor(actor), m_current(StateToIndex(first))
    {
      for (IndexType i=0; i<COUNT; i++) {
        m_states[i].Initialize(&m_actor, IndexToState(i), this);
      }
    }

    ParentSetter DefineState(StateSpace state) {
      StateRef(state).SetUp(nullptr, nullptr, nullptr);
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
    void ExecuteTransition(IndexType destination) {
      //std::cout << "executing transition from " << m_current << " to " << destination << std::endl;
      IndexType lca = LeastCommonAncestor(m_current, destination);
      //std::cout << "via LCA " << lca << std::endl;
      ExitHereToLCA(m_current, lca);
      EnterLCAToHere(lca, destination);
      m_current = destination;
      InformActorOfCurrentState();
    }
    IndexType LeastCommonAncestor(IndexType source, IndexType destination) {
      if (source == destination) return source;
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

  enum MySignals{ GO_NORTH, GO_SOUTH, GO_EAST, GO_WEST, GO_HOME, DO_ACTION };
  enum MyStates { INVALID=-1, WHOLE, NORTH, SOUTH, EAST, NORTH_EAST, NORTH_WEST, NNE, ENE, NNW, WNW, STATE_SENTINAL };
  class StatefulController {
    MyStates m_state = INVALID;
    StateMachine<StatefulController, MyStates, WHOLE, WNW, MySignals> m_hsm;

  public:
    StatefulController()
      : m_hsm(*this)
    {
      // clear unit test counters *before* doing any state stuff!
      for (auto& i : testpoint_enter) i = 0;
      for (auto& i : testpoint_tick) i = 0;
      for (auto& i : testpoint_exit) i = 0;

      m_hsm.DefineState(WHOLE)
         .SetNoParent()
         .SetOnEnter(&StatefulController::WholeOnEnter)
         .SetOnTick(&StatefulController::WholeOnTick)
         .SetOnExit(&StatefulController::WholeOnExit);

      m_hsm.DefineState(NORTH)
         .SetParent(WHOLE)
         .SetOnEnter(&StatefulController::NorthOnEnter)
         .SetOnTick(&StatefulController::NorthOnTick)
         .SetOnExit(&StatefulController::NorthOnExit)
         .ForSignal(GO_WEST).GoTo(NORTH_WEST)
         .ForSignal(DO_ACTION).Do(&StatefulController::DoAction);

      m_hsm.DefineState(SOUTH)
         .SetParent(WHOLE)
         .SetOnEnter(&StatefulController::SouthOnEnter)
         .SetOnTick(&StatefulController::SouthOnTick)
         .SetOnExit(&StatefulController::SouthOnExit)
         .ForSignal(GO_NORTH).GoTo(NORTH)
         .ForSignal(GO_EAST).GoToIf(EAST, &StatefulController::IsEastOpen)
         .ForSignal(GO_EAST).Do(&StatefulController::DoAction); // Action plus transition on SOUTH

      m_hsm.DefineState(NORTH_WEST)
         .SetParent(NORTH)
         .SetOnEnter(&StatefulController::NWOnEnter)
         .SetOnTick(&StatefulController::NWOnTick)
         .SetOnExit(&StatefulController::NWOnExit)
         .ForSignal(GO_NORTH).GoTo(NNW);

      m_hsm.DefineState(NNW)
         .SetParent(NORTH_WEST)
         .SetOnEnter(&StatefulController::NNWOnEnter)
         .SetOnExit(&StatefulController::NNWOnExit)
         .ForSignal(GO_SOUTH).GoTo(SOUTH); // multiple exits

      m_hsm.ConcludeSetupAndSetInitialState(SOUTH, &StatefulController::NewState);
    }

    int CurrentState() const { return static_cast<int>(m_state); }
    void SetState(int s) { m_state = static_cast<MyStates>(s); }
    void NewState(const MyStates s) { m_state = s; }
    void Tick() { m_hsm.Tick(); }
    void Signal(const MySignals s) { m_hsm.Signal(s); }

    void WholeOnEnter() {
      testpoint_enter[WHOLE] += 1;
    }
    void WholeOnTick() {
      testpoint_tick[WHOLE] += 1;
    }
    void WholeOnExit() {
      testpoint_exit[WHOLE] += 1;
    }

    virtual void NorthOnEnter() {
      testpoint_enter[NORTH] += 1;
    }
    void NorthOnTick() {
      testpoint_tick[NORTH] += 1;
    }
    void NorthOnExit() {
      testpoint_exit[NORTH] += 1;
    }

    void SouthOnEnter() {
      testpoint_enter[SOUTH] += 1;
    }
    void SouthOnTick() {
      testpoint_tick[SOUTH] += 1;
    }
    void SouthOnExit() {
      testpoint_exit[SOUTH] += 1;
    }

    void NWOnEnter() {
      testpoint_enter[NORTH_WEST] += 1;
    }
    void NWOnTick() {
      testpoint_tick[NORTH_WEST] += 1;
    }
    void NWOnExit() {
      testpoint_exit[NORTH_WEST] += 1;
    }

    void NNWOnEnter() {
      testpoint_enter[NNW] += 1;
    }
    void NNWOnTick() {
      testpoint_tick[NNW] += 1;
    }
    void NNWOnExit() {
      testpoint_exit[NNW] += 1;
    }
    
    void DoAction() {
      ++action_count;
      //std::cout << "DoAction()" << std::endl;
    }

    bool m_eastIsBlocked = false;
    void BlockEast() { m_eastIsBlocked = true; }
    bool IsEastOpen() const { return !m_eastIsBlocked; }

    int testpoint_enter[STATE_SENTINAL];
    int testpoint_tick[STATE_SENTINAL];
    int testpoint_exit[STATE_SENTINAL];
    int action_count = 0;
  };

  void test_01() {
    //std::cout << "======= " << __FUNCTION__ << " =======================" << std::endl;
    StatefulController uut;
    TEST_EQ(SOUTH, uut.CurrentState());
    TEST_EQ(1, uut.testpoint_enter[WHOLE]);
    TEST_EQ(1, uut.testpoint_enter[SOUTH]);
  }
  void test_02() {
    //std::cout << "======= " << __FUNCTION__ << " =======================" << std::endl;
    StatefulController uut;
    uut.Tick();
    TEST_EQ(1, uut.testpoint_tick[SOUTH]);
    uut.Tick();
    TEST_EQ(2, uut.testpoint_tick[SOUTH]);
  }
  void test_03() {
    //std::cout << "======= " << __FUNCTION__ << " =======================" << std::endl;
    StatefulController uut;
    uut.Signal(GO_NORTH);
    TEST_EQ(NORTH, uut.CurrentState());
    TEST_EQ(0, uut.testpoint_exit[WHOLE]);
    TEST_EQ(1, uut.testpoint_exit[SOUTH]);
    TEST_EQ(1, uut.testpoint_enter[NORTH]);
  }
  void test_04() {
    //std::cout << "======= " << __FUNCTION__ << " =======================" << std::endl;
    StatefulController uut;
    uut.Signal(GO_NORTH);
    uut.Signal(GO_WEST);
    TEST_EQ(NORTH_WEST, uut.CurrentState());
  }
  void test_05() {
    //std::cout << "======= " << __FUNCTION__ << " =======================" << std::endl;
    StatefulController uut;
    uut.Signal(GO_NORTH);
    uut.Signal(GO_WEST);
    uut.Signal(GO_NORTH);
    TEST_EQ(NNW, uut.CurrentState());
  }
  void test_06_least_common_ancestor() {
    //std::cout << "======= " << __FUNCTION__ << " =======================" << std::endl;
    StatefulController uut;
    uut.Signal(GO_NORTH);
    uut.Signal(GO_WEST);
    TEST_EQ(0, uut.testpoint_exit[NORTH]); // no exit yet yet
    uut.Signal(GO_NORTH);
    TEST_EQ(0, uut.testpoint_exit[NORTH_WEST]); // no exit yet yet
    uut.Signal(GO_SOUTH);
    TEST_EQ(SOUTH, uut.CurrentState());
    TEST_EQ(0, uut.testpoint_exit[WHOLE]);
    TEST_EQ(1, uut.testpoint_exit[NORTH]);
    TEST_EQ(1, uut.testpoint_exit[NORTH_WEST]);
    TEST_EQ(1, uut.testpoint_exit[NNW]);
    TEST_EQ(2, uut.testpoint_enter[SOUTH]);
  }
  class StatefulControllerPlus : public StatefulController {
  public:
    bool my_special_testpoint_executed = false;
    void NorthOnEnter() override {
      my_special_testpoint_executed = true;
    }
  };
  void test_07_dispatch_to_subclass() {
    //std::cout << "======= " << __FUNCTION__ << " =======================" << std::endl;
    StatefulControllerPlus uut;
    uut.Signal(GO_NORTH);
    TEST_EQ(true, uut.my_special_testpoint_executed);
  }

  void test_08_signal_action() {
    //std::cout << "======= " << __FUNCTION__ << " =======================" << std::endl;
    StatefulController uut;
    uut.Signal(GO_NORTH);
    TEST_EQ(0, uut.action_count);
    uut.Signal(DO_ACTION);
    TEST_EQ(1, uut.action_count);
  }
  void test_09_tick_handled_by_parent_if_null() {
    //std::cout << "======= " << __FUNCTION__ << " =======================" << std::endl;
    StatefulController uut;
    uut.Signal(GO_NORTH);
    uut.Signal(GO_WEST);
    uut.Signal(GO_NORTH); // Now in NNW which has a nullptr for OnTick
    uut.Tick();
    TEST_EQ(1, uut.testpoint_tick[NORTH_WEST]);
  }
  void test_10_signal_action_handled_by_parent() {
    //std::cout << "======= " << __FUNCTION__ << " =======================" << std::endl;
    StatefulController uut;
    uut.Signal(GO_NORTH);
    uut.Signal(GO_WEST);
    uut.Signal(GO_NORTH);
    TEST_EQ(0, uut.action_count);
    uut.Signal(DO_ACTION);
    TEST_EQ(1, uut.action_count);
  }
  void test_11_signal_transition_handled_by_parent() {
    //std::cout << "======= " << __FUNCTION__ << " =======================" << std::endl;
    StatefulController uut;
    uut.Signal(GO_NORTH);
    uut.Signal(GO_WEST);
    uut.Signal(GO_NORTH);
    uut.Signal(GO_WEST); // transition defined on NORTH
    TEST_EQ(NORTH_WEST, uut.CurrentState());
  }
  void test_12_signal_action_and_transition() {
    //std::cout << "======= " << __FUNCTION__ << " =======================" << std::endl;
    StatefulController uut;
    TEST_EQ(0, uut.action_count);
    uut.Signal(GO_EAST);
    TEST_EQ(1, uut.action_count);
    TEST_EQ(EAST, uut.CurrentState());
  }
  void test_13_signal_guard_prevents_transition() {
    //std::cout << "======= " << __FUNCTION__ << " =======================" << std::endl;
    StatefulController uut;
    uut.BlockEast();
    uut.Signal(GO_EAST);
    TEST_EQ(SOUTH, uut.CurrentState());
  }

   enum class ForestStates  { BIRCH_TRUNK, BIRCH_LEFT, BIRCH_RIGHT, PINE_TRUNK, PINE_LEFT, PINE_RIGHT };
   enum class ForestSignals { GO_UP, GO_DOWN_LEFT, GO_DOWN_RIGHT, JUMP, SING };
   class ForestTest {
      ForestStates m_state = ForestStates::BIRCH_TRUNK;
      StateMachine<ForestTest, ForestStates, ForestStates::BIRCH_TRUNK, ForestStates::PINE_RIGHT, ForestSignals> m_hsm;
   public:
      ForestTest()
         : m_hsm(*this)
      {
         m_hsm.DefineState(ForestStates::BIRCH_TRUNK)
            .SetNoParent()
            .ForSignal(ForestSignals::GO_DOWN_LEFT).GoTo(ForestStates::BIRCH_LEFT)
            .ForSignal(ForestSignals::GO_DOWN_RIGHT).GoTo(ForestStates::BIRCH_RIGHT);

         m_hsm.DefineState(ForestStates::PINE_TRUNK)
            .SetNoParent()
            .SetOnTick(&ForestTest::DoIt)
            .ForSignal(ForestSignals::GO_DOWN_LEFT).GoTo(ForestStates::PINE_LEFT)
            .ForSignal(ForestSignals::GO_DOWN_RIGHT).GoTo(ForestStates::PINE_RIGHT);

         m_hsm.DefineState(ForestStates::PINE_LEFT)
            .SetParent(ForestStates::PINE_TRUNK)
            .ForSignal(ForestSignals::GO_UP).GoTo(ForestStates::PINE_TRUNK)
            .ForSignal(ForestSignals::JUMP).GoTo(ForestStates::BIRCH_LEFT);

         m_hsm.DefineState(ForestStates::PINE_RIGHT)
            .SetParent(ForestStates::PINE_TRUNK)
            .ForSignal(ForestSignals::GO_UP).GoTo(ForestStates::PINE_TRUNK)
            .ForSignal(ForestSignals::JUMP).GoTo(ForestStates::BIRCH_RIGHT);

         m_hsm.DefineState(ForestStates::BIRCH_LEFT)
            .SetParent(ForestStates::BIRCH_TRUNK)
            .ForSignal(ForestSignals::GO_UP).GoTo(ForestStates::BIRCH_TRUNK)
            .ForSignal(ForestSignals::JUMP).GoTo(ForestStates::PINE_RIGHT)
            .ForSignal(ForestSignals::SING).Do(&ForestTest::Sing);

         m_hsm.DefineState(ForestStates::BIRCH_RIGHT)
            .SetParent(ForestStates::BIRCH_TRUNK)
            .ForSignal(ForestSignals::GO_UP).GoTo(ForestStates::BIRCH_TRUNK)
            .ForSignal(ForestSignals::JUMP).GoTo(ForestStates::PINE_LEFT);

         m_hsm.ConcludeSetupAndSetInitialState(ForestStates::PINE_TRUNK, &ForestTest::NewState);
      }
      void SetState(const int s) { m_state = static_cast<ForestStates>(s); }
      void NewState(const ForestStates s) { m_state = s; }
      int CurrentState() const { return static_cast<int>(m_state); }
      void Signal(const ForestSignals s) { m_hsm.Signal(s); }
      void Tick() { m_hsm.Tick(); }
      bool sung{false};
      void Sing() {
         sung = true;
         //std::cout << "Laaaaaaaa aaaaaa!" << std::endl;
      }
      int tickCount{0};
      void DoIt() { ++tickCount; }
   };

  void test_14_forest_transition() {
    // make sure forest transitions work correctly
    //std::cout << "======= " << __FUNCTION__ << " =======================" << std::endl;
    ForestTest uut;
    uut.Signal(ForestSignals::GO_DOWN_LEFT);
    TEST_EQ(int(ForestStates::PINE_LEFT), uut.CurrentState());
    uut.Signal(ForestSignals::JUMP);
    TEST_EQ(int(ForestStates::BIRCH_LEFT), uut.CurrentState());
    uut.Signal(ForestSignals::SING);
    TEST_EQ(uut.sung, true);
  }

  void test_15_tick_propogates_to_parent() {
    //std::cout << "======= " << __FUNCTION__ << " =======================" << std::endl;
    ForestTest uut;
    TEST_EQ(0, uut.tickCount);
    uut.Tick();
    TEST_EQ(1, uut.tickCount);
    uut.Signal(ForestSignals::GO_DOWN_LEFT);
    // Tick and make sure the tick handler in pine trunk ticks
    uut.Tick();
    TEST_EQ(3, uut.tickCount);
  }

   enum class CircularStates  { CHICKEN, EGG };
   enum class CircularSignals { MOVE };
   class CircularTest {
      CircularStates m_state = CircularStates::CHICKEN;
      StateMachine<CircularTest, CircularStates, CircularStates::CHICKEN, CircularStates::EGG, CircularSignals> m_hsm;
   public:
      CircularTest()
         : m_hsm(*this)
      {
         m_hsm.DefineState(CircularStates::CHICKEN)
            .SetParent(CircularStates::EGG)
            .ForSignal(CircularSignals::MOVE).GoTo(CircularStates::EGG);

         m_hsm.DefineState(CircularStates::EGG)
            .SetParent(CircularStates::CHICKEN)
            .ForSignal(CircularSignals::MOVE).GoTo(CircularStates::CHICKEN);

         m_hsm.ConcludeSetupAndSetInitialState(CircularStates::EGG, &CircularTest::NewState);
      }
      void NewState(const CircularStates s) { m_state = s; }
      void Signal(const CircularSignals s) { m_hsm.Signal(s); }
      void Tick() { m_hsm.Tick(); }
   };

  void test_16_round_and_round() {
    // don't allow circular parent graphs
    //std::cout << "======= " << __FUNCTION__ << " =======================" << std::endl;
    try {
      CircularTest uut;
    } catch(...) {
      return; // this is good
    }
    TEST_EQ(1, 2);
  }

  //TODO compute LCA once when transition is added
}

int main(int argc, char** argv) {
   std::cout << "Running..." << std::endl;
   test_01();
   test_02();
   test_03();
   test_04();
   test_05();
   test_06_least_common_ancestor();
   test_07_dispatch_to_subclass();
   test_08_signal_action();
   test_09_tick_handled_by_parent_if_null();
   test_10_signal_action_handled_by_parent();
   test_11_signal_transition_handled_by_parent();
   test_12_signal_action_and_transition();
   test_13_signal_guard_prevents_transition();
   test_14_forest_transition();
   test_16_round_and_round();
   printf("Done.\n");
   return 0;
}
