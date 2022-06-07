// I, the creator and copyright holder of this source code, release this work
// into the public domain. This applies worldwide.
// In some jurisdictions this not may be legally possible. In such case I grant
// anyone the right to use this work for any purpose, without any conditions,
// unless such conditions are required by law.
//
#pragma once

#include "status.hpp"

namespace kv::embedded {

// http://www.club.cc.cmu.edu/~ajo/disseminate/2017-02-15-Optional-From-Scratch.pdf

template<typename T>
struct optional {
    Status m_status;
    T m_value;
    optional() : m_status(), m_value() {}
    /* implicit */ optional(const T& v) : m_status(Success), m_value(v) {}
    /* implicit */ optional(Status s) : m_status(s), m_value() {}
    constexpr operator bool() const { return bool(m_status); }
    constexpr T value() const { return m_value; }
};

} // namespace kv::embedded
