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

TEST_CASE( "Dispatch to subclass", "[fhsm]" ) {
   StatefulControllerPlus uut;
   uut.Signal(GO_NORTH);
   CHECK(uut.my_special_testpoint_executed);
}


