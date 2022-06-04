#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "kv/fhsm/StateMachine.h"

using namespace kv::fhsm;

enum MySignals{ GO_NORTH, GO_SOUTH, GO_EAST, GO_WEST, GO_HOME, DO_ACTION };
enum MyStates { INVALID=-1, WHOLE, NORTH, SOUTH, EAST, NORTH_WEST, NNW, STATE_SENTINAL };
class StatefulController {
   MyStates m_state = INVALID;
   StateMachine<StatefulController, MyStates, WHOLE, NNW, MySignals> m_hsm;

public:
   StatefulController() : m_hsm(*this) {
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
         .ForSignal(GO_EAST).Do(&StatefulController::DoAction); // Action plus transition on GO_EAST from SOUTH

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


TEST_CASE( "Enter states to starting sub-state", "[fhsm]" ) {
   StatefulController uut;
   CHECK(SOUTH == uut.CurrentState());
   CHECK(1 == uut.testpoint_enter[WHOLE]);
   CHECK(1 == uut.testpoint_enter[SOUTH]);
}

TEST_CASE( "Tick the current (default) state", "[fhsm]" ) {
   StatefulController uut;
   uut.Tick();
   CHECK(1 == uut.testpoint_tick[SOUTH]);
   uut.Tick();
   CHECK(2 == uut.testpoint_tick[SOUTH]);
}

TEST_CASE( "State transition with exit and enter events", "[fhsm]" ) {
   StatefulController uut;
   uut.Signal(GO_NORTH);
   CHECK(NORTH == uut.CurrentState());
   CHECK(0 == uut.testpoint_exit[WHOLE]);
   CHECK(1 == uut.testpoint_exit[SOUTH]);
   CHECK(1 == uut.testpoint_enter[NORTH]);
}

TEST_CASE( "State transition into deeper sub-state", "[fhsm]" ) {
   StatefulController uut;
   uut.Signal(GO_NORTH);
   uut.Signal(GO_WEST);
   CHECK(NORTH_WEST == uut.CurrentState());
}

TEST_CASE( "State transition into third level sub-state", "[fhsm]" ) {
   StatefulController uut;
   uut.Signal(GO_NORTH);
   uut.Signal(GO_WEST);
   uut.Signal(GO_NORTH);
   CHECK(NNW == uut.CurrentState());
}

TEST_CASE( "Least common ancestor", "[fhsm]" ) {
   StatefulController uut;
   uut.Signal(GO_NORTH);
   uut.Signal(GO_WEST);
   CHECK(0 == uut.testpoint_exit[NORTH]); // no exit yet yet
   uut.Signal(GO_NORTH);
   CHECK(0 == uut.testpoint_exit[NORTH_WEST]); // no exit yet yet
   uut.Signal(GO_SOUTH);
   CHECK(SOUTH == uut.CurrentState());
   CHECK(0 == uut.testpoint_exit[WHOLE]);
   CHECK(1 == uut.testpoint_exit[NORTH]);
   CHECK(1 == uut.testpoint_exit[NORTH_WEST]);
   CHECK(1 == uut.testpoint_exit[NNW]);
   CHECK(2 == uut.testpoint_enter[SOUTH]);
}

class StatefulControllerPlus : public StatefulController {
public:
   bool my_special_testpoint_executed = false;
   void NorthOnEnter() override {
      my_special_testpoint_executed = true;
   }
};

//TEST_CASE( "Dispatch to subclass", "[fhsm]" ) {
//   StatefulControllerPlus uut;
//   uut.Signal(GO_NORTH);
//   CHECK(uut.my_special_testpoint_executed);
//}

SCENARIO("Derived state machine class", "[fhsm]") {
   GIVEN("A state machine derived from another"){
      StatefulControllerPlus uut;
      WHEN("A signal is sent to cause a state transition") {
         uut.Signal(GO_NORTH);
         THEN("The derived class's methods are called") {
            CHECK(uut.my_special_testpoint_executed);
         }
      }
   }
}

SCENARIO("A normal state machine", "[fhsm]") {
   StatefulController uut;
   GIVEN("We are in an action-processing state") {
      uut.Signal(GO_NORTH);
      REQUIRE(NORTH == uut.CurrentState());
      REQUIRE(0 == uut.action_count);
      WHEN("A signal is sent to cause an action") {
         uut.Signal(DO_ACTION);
         THEN("The acion method was called") {
            CHECK(1 == uut.action_count);
         }
      }
   }

   GIVEN("We are in a sub-state that handles nothing locally") {
      uut.Signal(GO_NORTH);
      uut.Signal(GO_WEST);
      uut.Signal(GO_NORTH); // Now in NNW which has no tick, no action, and only one transition (south)
      REQUIRE(NNW == uut.CurrentState());
      WHEN("A signal is sent to cause a transition") {
         uut.Signal(GO_WEST); // transition defined on NORTH
         THEN("The signal is handled by a higher state") {
            CHECK(NORTH_WEST == uut.CurrentState());
         }
      }
      WHEN("A signal is sent to cause an action") {
         uut.Signal(DO_ACTION);
         THEN("The action is handled by a higher state") {
            CHECK(1 == uut.action_count);
         }
      }
      WHEN("A tick is sent") {
         uut.Tick();
         THEN("The tick is handled by a higher state") {
            CHECK(1 == uut.testpoint_tick[NORTH_WEST]);
         }
      }
   }

   GIVEN("A signal causes both an action and a transition") {
      REQUIRE(0 == uut.action_count);
      REQUIRE(SOUTH == uut.CurrentState());
      WHEN("The signal is given") {
         uut.Signal(GO_EAST);
         THEN("Both the action and the transition are executed") {
            CHECK(1 == uut.action_count);
            CHECK(EAST == uut.CurrentState());
         }
      }
      WHEN("The transition is blocked and the signal is given") {
         uut.BlockEast();
         uut.Signal(GO_EAST);
         THEN("Only the action is executed") {
            CHECK(1 == uut.action_count);
            CHECK(SOUTH == uut.CurrentState());
         }
      }
   }
}

enum class ForestStates  { BIRCH_TRUNK, BIRCH_LEFT, BIRCH_RIGHT, PINE_TRUNK, PINE_LEFT, PINE_RIGHT };
enum class ForestSignals { GO_UP, GO_DOWN_LEFT, GO_DOWN_RIGHT, GO_JUMP, DO_SING };
class ForestTest {
   ForestStates m_state = ForestStates::BIRCH_TRUNK;
   StateMachine<ForestTest, ForestStates, ForestStates::BIRCH_TRUNK, ForestStates::PINE_RIGHT, ForestSignals> m_hsm;
public:
   ForestTest() : m_hsm(*this) {
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
         .ForSignal(ForestSignals::GO_JUMP).GoTo(ForestStates::BIRCH_LEFT);

      m_hsm.DefineState(ForestStates::PINE_RIGHT)
         .SetParent(ForestStates::PINE_TRUNK)
         .ForSignal(ForestSignals::GO_UP).GoTo(ForestStates::PINE_TRUNK)
         .ForSignal(ForestSignals::GO_JUMP).GoTo(ForestStates::BIRCH_RIGHT);

      m_hsm.DefineState(ForestStates::BIRCH_LEFT)
         .SetParent(ForestStates::BIRCH_TRUNK)
         .ForSignal(ForestSignals::GO_UP).GoTo(ForestStates::BIRCH_TRUNK)
         .ForSignal(ForestSignals::GO_JUMP).GoTo(ForestStates::PINE_RIGHT)
         .ForSignal(ForestSignals::DO_SING).Do(&ForestTest::Sing);

      m_hsm.DefineState(ForestStates::BIRCH_RIGHT)
         .SetParent(ForestStates::BIRCH_TRUNK)
         .ForSignal(ForestSignals::GO_UP).GoTo(ForestStates::BIRCH_TRUNK)
         .ForSignal(ForestSignals::GO_JUMP).GoTo(ForestStates::PINE_LEFT);

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

SCENARIO("Disconnected states", "[fhsm]") {
   ForestTest uut;
   GIVEN("A forest state hierarchy") {
      REQUIRE(int(ForestStates::PINE_TRUNK) == uut.CurrentState());
      REQUIRE_FALSE(uut.sung);
      REQUIRE(0 == uut.tickCount);
      WHEN("Move deeper in one tree") {
         uut.Signal(ForestSignals::GO_DOWN_LEFT);
         THEN("Transition down to second level within branch") {
            CHECK(int(ForestStates::PINE_LEFT) == uut.CurrentState());
         }
         AND_WHEN("Jump to other trunk") {
            uut.Signal(ForestSignals::GO_JUMP);
            THEN("Transition down to third level") {
               CHECK(int(ForestStates::BIRCH_LEFT) == uut.CurrentState());
            }
            AND_WHEN("Signal action") {
               uut.Signal(ForestSignals::DO_SING);
               THEN("The action executed") {
                  CHECK(uut.sung);
               }
            }
         }
      }

      WHEN("A tick is signaled") {
         uut.Tick();
         THEN("The tick is executed") {
            CHECK(1 == uut.tickCount);
         }
      }
   }
}

enum class CircularStates  { CHICKEN, EGG };
enum class CircularSignals { MOVE };
class CircularTest {
   CircularStates m_state = CircularStates::CHICKEN;
   StateMachine<CircularTest, CircularStates, CircularStates::CHICKEN, CircularStates::EGG, CircularSignals> m_hsm;
public:
   CircularTest() : m_hsm(*this) {
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

SCENARIO("Usage errors", "[fhsm]") {
   GIVEN("An ill-formed state hierarchy"){
      WHEN("You try to create an object") {
         THEN("It blows up") {
            REQUIRE_THROWS( new CircularTest() );
         }
      }
   }
}
