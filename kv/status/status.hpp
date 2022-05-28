// I, the creator and copyright holder of this source code, release this work
// into the public domain. This applies worldwide.
// In some jurisdictions this not may be legally possible. In such case I grant
// anyone the right to use this work for any purpose, without any conditions,
// unless such conditions are required by law.
//
#pragma once

#include <cstdint>

// https://www.fluentcpp.com/2020/06/26/implementing-a-universal-reference-wrapper/

namespace kv::status
{
  // http://isthe.com/chongo/tech/comp/fnv
  static constexpr uint64_t fvn_prime = 1099511628211ULL;
  static constexpr uint64_t fvn_offset_basis = 14695981039346656037ULL;

  constexpr uint64_t fvn_hash(const char * str)
  {
    uint64_t hash = fvn_offset_basis;
    while (*str)
    {
      hash ^= static_cast<uint64_t>(*str++);
      hash *= fvn_prime;
    }
    return hash;
  }

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

  class Status {
    const BaseStatus* status;
  public:
    constexpr Status(const BaseStatus* p) noexcept : status(p) {}
    constexpr operator bool() const noexcept { return status->success; }
    constexpr bool is_equal(const Status& rhs) const noexcept { return rhs.status && status->is_equal(*rhs.status); }
    constexpr bool is_a(const Status& rhs) const noexcept { return rhs.status && status->is_a(*rhs.status); }
    constexpr const char * c_str() const noexcept { return status->image; }
  };

  // Yes, this creates a singleton, but it is immutable and trivially destructable

  #define INTERNAL_USE_ONLY_DEF_L0(name, parent, s) \
  namespace hidden_details_look_away { \
  class name##Singleton : public BaseStatus { \
    explicit name##Singleton(const char * i) : BaseStatus(parent, fvn_hash(#name), s, i) {} \
  public: \
    static const BaseStatus* get() { \
      constexpr const char * img = #name; \
      static const name##Singleton me{img}; \
      return &me; } \
  }; } \
  const Status name{hidden_details_look_away::name##Singleton::get()}

  #define DEFINE_PARENT_LEVEL_GOOD_STATUS(name) \
    INTERNAL_USE_ONLY_DEF_L0(name, nullptr, true)

  #define DEFINE_PARENT_LEVEL_BAD_STATUS(name) \
    INTERNAL_USE_ONLY_DEF_L0(name, nullptr, false)

  #define DEFINE_STATUS(name, parent) \
    INTERNAL_USE_ONLY_DEF_L0(name, parent##Singleton::get(), bool(*parent##Singleton::get()))


  DEFINE_PARENT_LEVEL_GOOD_STATUS(Success);
  DEFINE_PARENT_LEVEL_BAD_STATUS(Uninitialized);
  DEFINE_PARENT_LEVEL_BAD_STATUS(NonSuccess);

  DEFINE_STATUS(Already, Success);

  DEFINE_STATUS(Rejected, NonSuccess);
  DEFINE_STATUS(Error, NonSuccess);
  DEFINE_STATUS(Failure, NonSuccess);

} // namespace kv::status
