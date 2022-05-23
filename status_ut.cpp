// I, the creator and copyright holder of this source code, release this work
// into the public domain. This applies worldwide.
// In some jurisdictions this not may be legally possible. In such case I grant
// anyone the right to use this work for any purpose, without any conditions,
// unless such conditions are required by law.
//
#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "kv/status/status.hpp"

//using namespace kv::status;

enum class TestControl {SUCCESS, ALREADY, UNINITIALIZED, REJECTED, ERROR, FAILURE};

kv::status::Status TestFunction(const TestControl control)
{
  switch (control)
  {
    case TestControl::SUCCESS: return kv::status::Success; break;
    case TestControl::ALREADY: return kv::status::Already; break;
    case TestControl::UNINITIALIZED: return kv::status::Uninitialized; break;
    case TestControl::REJECTED: return kv::status::Rejected; break;
    case TestControl::ERROR: return kv::status::Error; break;
    case TestControl::FAILURE: return kv::status::Failure; break;
    default:
      return kv::status::NonSuccess;
  }
}

TEST_CASE( "Simple Success", "[status]" ) {
   CHECK(kv::status::Success);
}
TEST_CASE( "Simple NonSuccess", "[status]" ) {
   CHECK(!kv::status::NonSuccess);
}
TEST_CASE( "Return values as bool", "[status]" ) {
   CHECK(   TestFunction(TestControl::SUCCESS));
   CHECK(   TestFunction(TestControl::ALREADY));
   CHECK( ! TestFunction(TestControl::UNINITIALIZED));
   CHECK( ! TestFunction(TestControl::REJECTED));
   CHECK( ! TestFunction(TestControl::ERROR));
   CHECK( ! TestFunction(TestControl::FAILURE));
}
TEST_CASE( "Return values as Status", "[status]" ) {
   CHECK(kv::status::Success == TestFunction(TestControl::SUCCESS));
   CHECK(kv::status::Already == TestFunction(TestControl::ALREADY));
   CHECK(kv::status::Uninitialized == TestFunction(TestControl::UNINITIALIZED));
   CHECK(kv::status::Rejected == TestFunction(TestControl::REJECTED));
   CHECK(kv::status::Error == TestFunction(TestControl::ERROR));
   CHECK(kv::status::Failure == TestFunction(TestControl::FAILURE));
}
TEST_CASE( "Mutable variables", "[status]" ) {
   auto status = kv::status::Success;
   CHECK(kv::status::Success == status);
   status = kv::status::Error;
   CHECK(kv::status::Error == status);
}
TEST_CASE( "is_a identity", "[status]" ) {
   auto status = kv::status::Success;
   CHECK(status.is_a(kv::status::Success));
}
TEST_CASE( "is_a subclass", "[status]" ) {
   auto status = kv::status::Already;
   CHECK(status.is_a(kv::status::Success));
}
