// I, the creator and copyright holder of this source code, release this work
// into the public domain. This applies worldwide.
// In some jurisdictions this not may be legally possible. In such case I grant
// anyone the right to use this work for any purpose, without any conditions,
// unless such conditions are required by law.
//
#pragma once

#include <cstdint>
#include <memory>

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
    friend class Status;
  protected:
    BaseStatus(const BaseStatus* p, const uint64_t k, const bool s) : parent(p), key(k), success(s) {}
  public:
    constexpr operator bool() const { return success;}
    constexpr bool as_bool() const { return success; }
    constexpr bool is_equal(const BaseStatus& other) const { return (other.key == key); }
    constexpr bool is_a(const BaseStatus& other) const {
      return (other.key == key) ? true : (parent && parent->is_a(other));
    }
  };

  class Status {
    const BaseStatus* status;
  public:
    Status(const BaseStatus* p) : status(p) {}
    constexpr operator bool() const { return status->as_bool(); }
    constexpr bool is_equal(const Status& rhs) const { return rhs.status && status->is_equal(*rhs.status); }
    constexpr bool is_a(const Status& rhs) const { return rhs.status && status->is_a(*rhs.status); }
  };


#define DEF_L0(name, parent, s) \
  class name##Singleton : public BaseStatus { \
    name##Singleton() : BaseStatus(parent, fvn_hash(#name), s) {} \
  public: \
    static const BaseStatus* get() { static name##Singleton me; return &me; } \
  }; \
  const Status name{name##Singleton::get()}

  #define DEF_L1(name, s) \
    DEF_L0(name, nullptr, s)

  #define DEF_L2(name, parent) \
    DEF_L0(name, parent##Singleton::get(), bool(*parent##Singleton::get()))


  DEF_L1(Success, true);
  DEF_L1(Uninitialized, false);
  DEF_L1(NonSuccess, false);

  DEF_L2(Already, Success);

  DEF_L2(Rejected, NonSuccess);
  DEF_L2(Error, NonSuccess);
  DEF_L2(Failure, NonSuccess);



//   class Status
//   {
//     uint64_t key;
//     bool temp;
//     friend constexpr bool operator==(const Status& lhs, const Status& rhs);
//     constexpr bool isEqual(const Status& rhs) const { return key == rhs.key; }
//   public:
//     constexpr Status() : key(0LLU), temp(false) {}
//     constexpr Status(const uint64_t k, const bool v) : key(k), temp(v) {}
//     constexpr operator bool() const { return temp;}
//     constexpr Status(const Status& other) : key(other.key), temp(other.temp) {}
//     constexpr Status& operator=(const Status& other) {key=other.key; temp=other.temp; return *this;}
//     constexpr bool is_a(const Status& other) { return (other.key == key); }
//   };
//   constexpr bool operator==(const Status& lhs, const Status& rhs)
//   {
//     return lhs.isEqual(rhs);
//   }
//
// #define MAKE1(name, good) \
//   constexpr Status name = Status(fvn_hash(#name), good)
//
//   constexpr Status Uninitialized = Status();
//
// //  constexpr Status Success = Status(1, true);
// //  constexpr Status Already = Status(2, true);
//   MAKE1(Success, true);
//   MAKE1(Already, true);
//
//   constexpr Status NonSuccess = Status(3, false);
//   constexpr Status Rejected = Status(4, false);
//   constexpr Status Error = Status(5, false);
//   constexpr Status Failure = Status(6, false);
}