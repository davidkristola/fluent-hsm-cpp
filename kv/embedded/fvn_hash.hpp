// I, the creator and copyright holder of this source code, release this work
// into the public domain. This applies worldwide.
// In some jurisdictions this not may be legally possible. In such case I grant
// anyone the right to use this work for any purpose, without any conditions,
// unless such conditions are required by law.
//
#pragma once

namespace kv::embedded
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
  // https://stackoverflow.com/questions/2111667/compile-time-string-hashing
  //unsigned constexpr const_hash(char const *input) {
  //  return *input ?
  //      static_cast<unsigned int>(*input) + 33 * const_hash(input + 1) :
  //      5381;
  //}
} // namespace kv::embedded
