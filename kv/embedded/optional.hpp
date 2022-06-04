// I, the creator and copyright holder of this source code, release this work
// into the public domain. This applies worldwide.
// In some jurisdictions this not may be legally possible. In such case I grant
// anyone the right to use this work for any purpose, without any conditions,
// unless such conditions are required by law.
//
#pragma once

#include "status.hpp"

namespace kv::embedded {

    template<typename T>
    struct optional {
        Status status;
        T value;
        optional() : status(), value() {}
        /* implicit */ optional(const T& v) : status(Success), value(v) {}
        constexpr operator bool() const { return bool(status); }
    };

} // namespace kv::embedded
