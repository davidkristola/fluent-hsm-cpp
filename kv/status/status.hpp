// I, the creator and copyright holder of this source code, release this work
// into the public domain. This applies worldwide.
// In some jurisdictions this not may be legally possible. In such case I grant
// anyone the right to use this work for any purpose, without any conditions,
// unless such conditions are required by law.
//
#pragma once

#include "status_implementation_details.hpp"

namespace kv::status
{

  // This is a "pass by value" type.
  class Status {
    //TODO(djk): see if there is a way to make this a "not nullptr" type
    const BaseStatus* status;
  public:
    constexpr explicit Status(const BaseStatus* p) noexcept : status(p) {}
    constexpr operator bool() const noexcept { return status && status->success; }
    constexpr bool is_a(const Status& rhs) const noexcept { return status && rhs.status && status->is_a(*rhs.status); }
    constexpr const char * c_str() const noexcept { return status ? status->image : ""; }
  };

  DEFINE_PARENT_LEVEL_GOOD_STATUS(Success);
  DEFINE_PARENT_LEVEL_BAD_STATUS(Uninitialized);
  DEFINE_PARENT_LEVEL_BAD_STATUS(NonSuccess);

  DEFINE_STATUS(Already, IS_A_CHILD_OF_STATUS(Success));

  DEFINE_STATUS(Rejected, IS_A_CHILD_OF_STATUS(NonSuccess));
  DEFINE_STATUS(Error, IS_A_CHILD_OF_STATUS(NonSuccess));
  DEFINE_STATUS(Failure, IS_A_CHILD_OF_STATUS(NonSuccess));

} // namespace kv::status
