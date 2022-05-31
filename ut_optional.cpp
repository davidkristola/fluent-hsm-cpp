// I, the creator and copyright holder of this source code, release this work
// into the public domain. This applies worldwide.
// In some jurisdictions this not may be legally possible. In such case I grant
// anyone the right to use this work for any purpose, without any conditions,
// unless such conditions are required by law.
//
#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "kv/status/optional.hpp"
#include <string>
#include <type_traits>

TEST_CASE( "Simple Failure", "[optional]" ) {
    kv::status::optional<int> uninitialized;
    CHECK( ! uninitialized );
}
TEST_CASE( "Simple Success", "[optional]" ) {
    kv::status::optional<int> initialized{0};
    CHECK( initialized );
}
