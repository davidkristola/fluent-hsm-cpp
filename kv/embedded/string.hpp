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

struct unused_base_class{};
constexpr bool small_enough(size_t size) { return size < 33; }

template<size_t storage_units>
class string : private std::enable_if<small_enough(storage_units),unused_base_class>::type
{
    union {
        uint64_t m_storage[storage_units];
        char m_string[storage_units * 8];
    };

    constexpr void inc() noexcept { m_string[0]++; }
public:
    constexpr size_t storage() const noexcept { return (storage_units * 8) - 2; }
    constexpr uint8_t length() const noexcept { return static_cast<uint8_t>(m_string[0]); }
    constexpr void clear() noexcept
    {
        for (auto i=0; i<storage_units; i++)
        {
            m_storage[i] = 0;
        }
    }

    constexpr string() noexcept
    {
        clear();
    }

    constexpr string(const char* s) noexcept
    {
        clear();
        for (auto i=0; (s[i] != 0) && (length() < storage()); i++)
        {
            m_string[1 + length()] = s[i];
            inc();
        }
    }

    constexpr const char* c_str() const noexcept { return &m_string[1]; }

    constexpr string<storage_units>& append(const char *s) noexcept
    {
        for (auto i=0; (s[i] != 0) && (length() < storage()); i++)
        {
            m_string[1 + length()] = s[i];
            inc();
        }
        return *this;
    }

    template<size_t other_storage>
    constexpr string<storage_units>& append(const string<other_storage>& other)
    {
        append(other.c_str());
        return *this;
    }
};

} // namespace kv::embedded
