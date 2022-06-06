// I, the creator and copyright holder of this source code, release this work
// into the public domain. This applies worldwide.
// In some jurisdictions this not may be legally possible. In such case I grant
// anyone the right to use this work for any purpose, without any conditions,
// unless such conditions are required by law.
//
#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "kv/embedded/string.hpp"
#include <string>
#include <type_traits>

TEST_CASE( "Simple Success", "[string]" ) {
    kv::embedded::string<1> hello{"Hello"};
    CHECK( std::string("Hello") == std::string(hello.c_str()) );
    kv::embedded::string<1> good_by{"Good by!"};
    CHECK( std::string("Good b") == std::string(good_by.c_str()) );
}
