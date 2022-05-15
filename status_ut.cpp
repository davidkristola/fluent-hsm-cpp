#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "kv/status/status.hpp"

using namespace kv::status;

TEST_CASE( "Simple Success", "[status]" ) {
   CHECK(kv::status::Success);
}
TEST_CASE( "Simple NonSuccess", "[status]" ) {
   CHECK(!kv::status::NonSuccess);
}
