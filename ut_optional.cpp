// I, the creator and copyright holder of this source code, release this work
// into the public domain. This applies worldwide.
// In some jurisdictions this not may be legally possible. In such case I grant
// anyone the right to use this work for any purpose, without any conditions,
// unless such conditions are required by law.
//
#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "kv/embedded/optional.hpp"
#include <string>
#include <type_traits>

TEST_CASE( "Simple Failure", "[optional]" ) {
    kv::embedded::optional<int> uninitialized;
    CHECK( ! uninitialized );
}
TEST_CASE( "Simple Success", "[optional]" ) {
    kv::embedded::optional<int> initialized{0};
    CHECK( initialized );
}

kv::embedded::optional<int> test_function(bool good)
{
    if (good) return 37;
    return kv::embedded::Error;
}

TEST_CASE( "Indirect Success", "[optional]" ) {
    const auto value = test_function(true);
    CHECK( value );
    CHECK( value.value() == 37 );
}
