// I, the creator and copyright holder of this source code, release this work
// into the public domain. This applies worldwide.
// In some jurisdictions this not may be legally possible. In such case I grant
// anyone the right to use this work for any purpose, without any conditions,
// unless such conditions are required by law.
//
#pragma once

#include "fvn_hash.hpp"
#include <cstdint>

namespace kv::status
{
  class BaseStatus
  {
    const BaseStatus* parent;
    const uint64_t key;
    const bool success;
    const char * image;
    friend class Status;
  protected:
    constexpr BaseStatus(const BaseStatus* p, const uint64_t k, const bool s, const char * i) noexcept : parent(p), key(k), success(s), image(i) {}
  public:
    constexpr operator bool() const noexcept { return success;}
    constexpr bool is_equal(const BaseStatus& other) const noexcept { return (other.key == key); }
    constexpr bool is_a(const BaseStatus& other) const noexcept {
      return (other.key == key) ? true : (parent && parent->is_a(other));
    }
  };

  // Yes, this creates a singleton, but it is immutable and trivially destructable

  #define INTERNAL_USE_ONLY_DEF_UNIQUE_SINGLETON(name, parent, s) \
  namespace hidden_details_look_away { \
  class name##Singleton : public BaseStatus { \
    explicit name##Singleton(const char * i) : BaseStatus(parent, fvn_hash(#name), s, i) {} \
  public: \
    static const BaseStatus* get() { \
      constexpr const char * img = #name; \
      static const name##Singleton me{img}; \
      return &me; } \
  }; }

  // This is a default singleton used when Status is given a nullptr.
  INTERNAL_USE_ONLY_DEF_UNIQUE_SINGLETON(Uninitialized, nullptr, false) // no semicolon

  #define INTERNAL_USE_ONLY_DEF_L0(name, parent, s) \
  INTERNAL_USE_ONLY_DEF_UNIQUE_SINGLETON(name, parent, s) \
  const Status name{hidden_details_look_away::name##Singleton::get()}

  #define DEFINE_PARENT_LEVEL_GOOD_STATUS(name) \
    INTERNAL_USE_ONLY_DEF_L0(name, nullptr, true)

  #define DEFINE_PARENT_LEVEL_BAD_STATUS(name) \
    INTERNAL_USE_ONLY_DEF_L0(name, nullptr, false)

  #define DEFINE_STATUS(name, parent) \
    INTERNAL_USE_ONLY_DEF_L0(name, parent, bool(*parent))

  #define IS_A_CHILD_OF_STATUS(parent) \
     parent##Singleton::get()
} // namespace kv::status

// https://www.fluentcpp.com/2020/06/26/implementing-a-universal-reference-wrapper/
