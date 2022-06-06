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

TEST_CASE( "Simple Success 1", "[string]" ) {
    kv::embedded::string<1> hello{"Hello"};
    CHECK( std::string("Hello") == std::string(hello.c_str()) );
    kv::embedded::string<1> good_by{"Good By!"};
    CHECK( std::string("Good B") == std::string(good_by.c_str()) );
}

TEST_CASE( "Simple Success 2", "[string]" ) {
    kv::embedded::string<2> hello{"Hello"};
    CHECK( std::string("Hello") == std::string(hello.c_str()) );
    kv::embedded::string<5> good_by{"Good By!"};
    CHECK( std::string("Good By!") == std::string(good_by.c_str()) );
    hello.append(" ");
    CHECK( std::string("Hello ") == std::string(hello.c_str()) );
    hello.append(good_by);
    CHECK( std::string("Hello Good By!") == std::string(hello.c_str()) );
    good_by.append(" and thanks").append(" for all the").append(" fish!");
    CHECK( std::string("Good By! and thanks for all the fish!") == std::string(good_by.c_str()) );
}
