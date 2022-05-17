#pragma once

#include <cstdint>
#include <memory>

// https://www.fluentcpp.com/2020/06/26/implementing-a-universal-reference-wrapper/

namespace kv::status
{
  class BaseStatus
  {
    const BaseStatus* parent;
    const uint32_t key;
    const bool success;
    friend class StatusPointer;
  protected:
    BaseStatus(const BaseStatus* p, const uint32_t k, const bool s) : parent(p), key(k), success(s) {}
  public:
    constexpr operator bool() const { return success;}
    constexpr bool isA(const BaseStatus& other) { return (other.key == key); }
  };

#define DEF(name, parent, k, s) \
  class name : public BaseStatus { \
    name() : BaseStatus(nullptr, k, s) {} \
  public: \
    const BaseStatus* get() const { static name me; return &me; } \
  }

  DEF(SuccessImpl, nullptr, 1, true);

  class StatusPointer {
    const BaseStatus * status;
  public:
    StatusPointer(const BaseStatus * p) : status(p) {}
    constexpr bool isEqual(const StatusPointer& rhs) const { return status->key == rhs.status->key; }
  };

  class Status
  {
    uint32_t key;
    bool temp;
    friend constexpr bool operator==(const Status& lhs, const Status& rhs);
    constexpr bool isEqual(const Status& rhs) const { return key == rhs.key; }
  public:
    constexpr Status() : key(0U), temp(false) {}
    constexpr Status(const uint32_t k, const bool v) : key(k), temp(v) {}
    constexpr operator bool() const { return temp;}
    constexpr Status(const Status& other) : key(other.key), temp(other.temp) {}
    constexpr Status& operator=(const Status& other) {key=other.key; temp=other.temp; return *this;}
    constexpr bool isA(const Status& other) { return (other.key == key); }
  };
  constexpr bool operator==(const Status& lhs, const Status& rhs)
  {
    return lhs.isEqual(rhs);
  }


  constexpr Status Uninitialized = Status();
  constexpr Status Success = Status(1, true);
  constexpr Status Already = Status(2, true);

  constexpr Status NonSuccess = Status(3, false);
  constexpr Status Rejected = Status(4, false);
  constexpr Status Error = Status(5, false);
  constexpr Status Failure = Status(6, false);
}
