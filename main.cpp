// This main program is a quick and simple unit test for
// the fluent hierarchical state machine library in this
// repo.

#include "kv/fhsm/StateMachine.h"

#include <cstdio>
#include <iostream>

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

#if 1
#define WHO() std::cout << "======= " << __FUNCTION__ << " =======================" << std::endl;
#else
#define WHO()
#endif

using namespace kv::fhsm;

namespace {

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

void test_01() {
   WHO();
   StatefulController uut;
   TEST_EQ(SOUTH, uut.CurrentState());
   TEST_EQ(1, uut.testpoint_enter[WHOLE]);
   TEST_EQ(1, uut.testpoint_enter[SOUTH]);
}
void test_02() {
   WHO();
   StatefulController uut;
   uut.Tick();
   TEST_EQ(1, uut.testpoint_tick[SOUTH]);
   uut.Tick();
   TEST_EQ(2, uut.testpoint_tick[SOUTH]);
}
void test_03() {
   WHO();
   StatefulController uut;
   uut.Signal(GO_NORTH);
   TEST_EQ(NORTH, uut.CurrentState());
   TEST_EQ(0, uut.testpoint_exit[WHOLE]);
   TEST_EQ(1, uut.testpoint_exit[SOUTH]);
   TEST_EQ(1, uut.testpoint_enter[NORTH]);
}
void test_04() {
   WHO();
   StatefulController uut;
   uut.Signal(GO_NORTH);
   uut.Signal(GO_WEST);
   TEST_EQ(NORTH_WEST, uut.CurrentState());
}
void test_05() {
   WHO();
   StatefulController uut;
   uut.Signal(GO_NORTH);
   uut.Signal(GO_WEST);
   uut.Signal(GO_NORTH);
   TEST_EQ(NNW, uut.CurrentState());
}
void test_06_least_common_ancestor() {
   WHO();
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
   WHO();
   StatefulControllerPlus uut;
   uut.Signal(GO_NORTH);
   TEST_EQ(true, uut.my_special_testpoint_executed);
}

void test_08_signal_action() {
   WHO();
   StatefulController uut;
   uut.Signal(GO_NORTH);
   TEST_EQ(0, uut.action_count);
   uut.Signal(DO_ACTION);
   TEST_EQ(1, uut.action_count);
}
void test_09_tick_handled_by_parent_if_null() {
   WHO();
   StatefulController uut;
   uut.Signal(GO_NORTH);
   uut.Signal(GO_WEST);
   uut.Signal(GO_NORTH); // Now in NNW which has a nullptr for OnTick
   uut.Tick();
   TEST_EQ(1, uut.testpoint_tick[NORTH_WEST]);
}
void test_10_signal_action_handled_by_parent() {
   WHO();
   StatefulController uut;
   uut.Signal(GO_NORTH);
   uut.Signal(GO_WEST);
   uut.Signal(GO_NORTH);
   TEST_EQ(0, uut.action_count);
   uut.Signal(DO_ACTION);
   TEST_EQ(1, uut.action_count);
}
void test_11_signal_transition_handled_by_parent() {
   WHO();
   StatefulController uut;
   uut.Signal(GO_NORTH);
   uut.Signal(GO_WEST);
   uut.Signal(GO_NORTH);
   uut.Signal(GO_WEST); // transition defined on NORTH
   TEST_EQ(NORTH_WEST, uut.CurrentState());
}
void test_12_signal_action_and_transition() {
   WHO();
   StatefulController uut;
   TEST_EQ(0, uut.action_count);
   uut.Signal(GO_EAST);
   TEST_EQ(1, uut.action_count);
   TEST_EQ(EAST, uut.CurrentState());
}
void test_13_signal_guard_prevents_transition() {
   WHO();
   StatefulController uut;
   uut.BlockEast();
   uut.Signal(GO_EAST);
   TEST_EQ(SOUTH, uut.CurrentState());
}

enum class ForestStates  { BIRCH_TRUNK, BIRCH_LEFT, BIRCH_RIGHT, PINE_TRUNK, PINE_LEFT, PINE_RIGHT };
enum class ForestSignals { GO_UP, GO_DOWN_LEFT, GO_DOWN_RIGHT, JUMP, SING_LOUD }; // SING is a Clang macro
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
         .ForSignal(ForestSignals::JUMP).GoTo(ForestStates::BIRCH_LEFT);

      m_hsm.DefineState(ForestStates::PINE_RIGHT)
         .SetParent(ForestStates::PINE_TRUNK)
         .ForSignal(ForestSignals::GO_UP).GoTo(ForestStates::PINE_TRUNK)
         .ForSignal(ForestSignals::JUMP).GoTo(ForestStates::BIRCH_RIGHT);

      m_hsm.DefineState(ForestStates::BIRCH_LEFT)
         .SetParent(ForestStates::BIRCH_TRUNK)
         .ForSignal(ForestSignals::GO_UP).GoTo(ForestStates::BIRCH_TRUNK)
         .ForSignal(ForestSignals::JUMP).GoTo(ForestStates::PINE_RIGHT)
         .ForSignal(ForestSignals::SING_LOUD).Do(&ForestTest::Sing);

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
   WHO();
   ForestTest uut;
   uut.Signal(ForestSignals::GO_DOWN_LEFT);
   TEST_EQ(int(ForestStates::PINE_LEFT), uut.CurrentState());
   uut.Signal(ForestSignals::JUMP);
   TEST_EQ(int(ForestStates::BIRCH_LEFT), uut.CurrentState());
   uut.Signal(ForestSignals::SING_LOUD);
   TEST_EQ(uut.sung, true);
}

void test_15_tick_propogates_to_parent() {
   WHO();
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

void test_16_round_and_round() {
   // don't allow circular parent graphs
   WHO();
   try {
      CircularTest uut;
   } catch(...) {
      return; // this is good
   }
   TEST_EQ(1, 2);
}

} // anonymous namespace

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
