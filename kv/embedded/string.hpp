// I, the creator and copyright holder of this source code, release this work
// into the public domain. This applies worldwide.
// In some jurisdictions this not may be legally possible. In such case I grant
// anyone the right to use this work for any purpose, without any conditions,
// unless such conditions are required by law.
//
#pragma once

#include "status.hpp"
#include <cstdint>

namespace kv::embedded {

template<size_t storage_units>
class string
{
    union {
        uint64_t m_storage[storage_units];
        char m_string[storage_units * 8];
    };
public:
    string(const char* s) noexcept
    {
        for (auto i=0; i<storage_units; i++) m_storage[i] = 0;
    }
    const char* c_str() const noexcept { return "Hello"; }
};

} // namespace kv::embedded
