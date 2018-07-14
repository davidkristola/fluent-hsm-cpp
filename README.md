# fluent-hsm-cpp
A (hopefully) easy-to-use hierarchical state machine for C++14 using a fluent style of programming.

States and signals (events) can be defined using `enum` or `enum class` types. A hierarchical state
machine can be added to a class (the _actor_) by adding a templated member (`m_hsm` in the example).

```C++
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
```
