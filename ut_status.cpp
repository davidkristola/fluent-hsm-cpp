// I, the creator and copyright holder of this source code, release this work
// into the public domain. This applies worldwide.
// In some jurisdictions this not may be legally possible. In such case I grant
// anyone the right to use this work for any purpose, without any conditions,
// unless such conditions are required by law.
//
#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "kv/embedded/status.hpp"
#include <string>
#include <type_traits>

enum class TestControl {SUCCESS, ALREADY, UNINITIALIZED, REJECTED, ERROR, FAILURE};

kv::embedded::Status TestFunction(const TestControl control)
{
  switch (control)
  {
    case TestControl::SUCCESS: return kv::embedded::Success; break;
    case TestControl::ALREADY: return kv::embedded::Already; break;
    case TestControl::UNINITIALIZED: return kv::embedded::Status(); break;
    case TestControl::REJECTED: return kv::embedded::Rejected; break;
    case TestControl::ERROR: return kv::embedded::Error; break;
    case TestControl::FAILURE: return kv::embedded::Failure; break;
    default:
      return kv::embedded::NonSuccess;
  }
}

TEST_CASE( "Simple Success", "[status]" ) {
   CHECK(kv::embedded::Success);
}
TEST_CASE( "Simple NonSuccess", "[status]" ) {
   CHECK(!kv::embedded::NonSuccess);
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
   CHECK(kv::embedded::Success == TestFunction(TestControl::SUCCESS));
   CHECK(kv::embedded::Already == TestFunction(TestControl::ALREADY));
   CHECK(kv::embedded::Rejected == TestFunction(TestControl::REJECTED));
   CHECK(kv::embedded::Error == TestFunction(TestControl::ERROR));
   CHECK(kv::embedded::Failure == TestFunction(TestControl::FAILURE));
}
TEST_CASE( "Mutable variables", "[status]" ) {
   kv::embedded::Status status; //TODO(djk): should there be a default constructor?
   status = kv::embedded::Success;
   CHECK(kv::embedded::Success == status);
   status = kv::embedded::Error;
   CHECK(kv::embedded::Error == status);
}
TEST_CASE( "is_a identity", "[status]" ) {
   auto status = kv::embedded::Success;
   CHECK(status.is_a(kv::embedded::Success));
}
TEST_CASE( "is_a subclass", "[status]" ) {
   CHECK(kv::embedded::Already.is_a(kv::embedded::Success));
}
TEST_CASE( "is_a not a subclass", "[status]" ) {
   CHECK( ! kv::embedded::Already.is_a(kv::embedded::Error));
}
TEST_CASE( "name check", "[status]" ) {
   CHECK( std::string("Already") == std::string(kv::embedded::Already.c_str()) );
   CHECK( std::string("Success") == std::string(kv::embedded::Success.c_str()) );
   CHECK( std::string("Error") == std::string(kv::embedded::Error.c_str()) );
}
TEST_CASE( "trivially destructable", "[status]" ) {
  CHECK( std::is_trivially_destructible<decltype(kv::embedded::Already)>::value );
  CHECK( std::is_trivially_destructible<kv::embedded::hidden_details_look_away::AlreadySingleton>::value );
}

namespace kv::embedded {
DEFINE_STATUS(MyError, IS_A_CHILD_OF_STATUS(Error));
} // namespace kv::embedded

TEST_CASE( "MyError", "[status]" ) {
  CHECK( ! kv::embedded::MyError );
  CHECK( kv::embedded::MyError.is_a(kv::embedded::Error) );
  CHECK( kv::embedded::MyError.is_a(kv::embedded::NonSuccess) );
  CHECK( ! kv::embedded::MyError.is_a(kv::embedded::Success) );
  CHECK( std::string("MyError") == std::string(kv::embedded::MyError.c_str()) );
}

// This won't compile:
//namespace kv::embedded {
//DEFINE_STATUS(MyError, IS_A_CHILD_OF_STATUS(Error));
//} // namespace kv::embedded

// This won't compile:
//DEFINE_STATUS(AnotherError, IS_A_CHILD_OF_STATUS(Error));

TEST_CASE( "Don't crash if someone forces a bad Status", "[status]" ) {
  kv::embedded::Status intentionally_bad{nullptr};
  CHECK( ! intentionally_bad );
  CHECK( intentionally_bad != kv::embedded::Success );
  CHECK( ! intentionally_bad.is_a(kv::embedded::Success) );
  CHECK( ! intentionally_bad.is_a(kv::embedded::NonSuccess) );
  CHECK( std::string("Uninitialized") == std::string(intentionally_bad.c_str()) );
}
TEST_CASE( "Size of a pointer", "[status]" ) {
  int * ptr = nullptr;
  CHECK(sizeof(kv::embedded::Success) == sizeof(ptr) );
}
