// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "common/int-util.h"
#include "crypto/hash.h"
#include "currency_config.h"
#include "difficulty.h"

namespace currency {

  using std::size_t;
  using std::uint64_t;
  using std::vector;

#if defined(_MSC_VER)
#include <windows.h>
#include <winnt.h>

  static inline void mul(uint64_t a, uint64_t b, uint64_t &low, uint64_t &high) {
    low = UnsignedMultiply128(a, b, &high);
  }

#else

  static inline void mul(uint64_t a, uint64_t b, uint64_t &low, uint64_t &high) {
    typedef unsigned __int128 uint128_t;
    uint128_t res = (uint128_t) a * (uint128_t) b;
    low = (uint64_t) res;
    high = (uint64_t) (res >> 64);
  }

#endif

  static inline bool cadd(uint64_t a, uint64_t b) {
    return a + b < a;
  }

  static inline bool cadc(uint64_t a, uint64_t b, bool c) {
    return a + b < a || (c && a + b == (uint64_t) -1);
  }

  bool check_hash_old(const crypto::hash &hash, difficulty_type difficulty) {
    uint64_t low, high, top, cur;
    // First check the highest word, this will most likely fail for a random hash.
    mul(swap64le(((const uint64_t *) &hash)[3]), difficulty, top, high);
    if (high != 0) {
      return false;
    }
    mul(swap64le(((const uint64_t *) &hash)[0]), difficulty, low, cur);
    mul(swap64le(((const uint64_t *) &hash)[1]), difficulty, low, high);
    bool carry = cadd(cur, low);
    cur = high;
    mul(swap64le(((const uint64_t *) &hash)[2]), difficulty, low, high);
    carry = cadc(cur, low, carry);
    carry = cadc(high, top, carry);
    return !carry;
  }

#if defined(_MSC_VER)
#ifdef max
#undef max
#endif
#endif

  const wide_difficulty_type max64bit(std::numeric_limits<std::uint64_t>::max());
  const boost::multiprecision::uint256_t max128bit(std::numeric_limits<boost::multiprecision::uint128_t>::max());
  const boost::multiprecision::uint512_t max256bit(std::numeric_limits<boost::multiprecision::uint256_t>::max());
  
  bool check_hash(const crypto::hash &hash, wide_difficulty_type difficulty) {
    if (difficulty < max64bit){ // if can convert to small difficulty - do it
      std::uint64_t dl = difficulty.convert_to<std::uint64_t>();
      uint64_t low, high, top, cur;
      // First check the highest word, this will most likely fail for a random hash.
      mul(swap64le(((const uint64_t *) &hash)[3]), dl, top, high);
      if (high != 0) 
        return false;
      mul(swap64le(((const uint64_t *) &hash)[0]), dl, low, cur);
      mul(swap64le(((const uint64_t *) &hash)[1]), dl, low, high);
      bool carry = cadd(cur, low);
      cur = high;
      mul(swap64le(((const uint64_t *) &hash)[2]), dl, low, high);
      carry = cadc(cur, low, carry);
      carry = cadc(high, top, carry);
      return !carry;
    }
    // fast check
    if (((const uint64_t *) &hash)[3] > 0)
      return false;
    // usual slow check
    boost::multiprecision::uint512_t hashVal = 0;
    for(int i = 1; i < 4; i++) { // highest word is zero
      hashVal |= swap64le(((const uint64_t *) &hash)[3 - i]);
      hashVal << 64;
    }
    return (hashVal * difficulty > max256bit);
  }

  difficulty_type next_difficulty_old(vector<uint64_t> timestamps, vector<difficulty_type> cumulative_difficulties, size_t target_seconds) {
    //cutoff DIFFICULTY_LAG
    if(timestamps.size() > DIFFICULTY_WINDOW)
    {
      timestamps.resize(DIFFICULTY_WINDOW);
      cumulative_difficulties.resize(DIFFICULTY_WINDOW);
    }


    size_t length = timestamps.size();
    assert(length == cumulative_difficulties.size());
    if (length <= 1) {
      return 1;
    }
    static_assert(DIFFICULTY_WINDOW >= 2, "Window is too small");
    assert(length <= DIFFICULTY_WINDOW);
    sort(timestamps.begin(), timestamps.end());
    size_t cut_begin, cut_end;
    static_assert(2 * DIFFICULTY_CUT <= DIFFICULTY_WINDOW - 2, "Cut length is too large");
    if (length <= DIFFICULTY_WINDOW - 2 * DIFFICULTY_CUT) {
      cut_begin = 0;
      cut_end = length;
    } else {
      cut_begin = (length - (DIFFICULTY_WINDOW - 2 * DIFFICULTY_CUT) + 1) / 2;
      cut_end = cut_begin + (DIFFICULTY_WINDOW - 2 * DIFFICULTY_CUT);
    }
    assert(/*cut_begin >= 0 &&*/ cut_begin + 2 <= cut_end && cut_end <= length);
    uint64_t time_span = timestamps[cut_end - 1] - timestamps[cut_begin];
    if (time_span == 0) {
      time_span = 1;
    }
    difficulty_type total_work = cumulative_difficulties[cut_end - 1] - cumulative_difficulties[cut_begin];
    assert(total_work > 0);
    uint64_t low, high;
    mul(total_work, target_seconds, low, high);
    if (high != 0 || low + time_span - 1 < low) {
      return 0;
    }
    return (low + time_span - 1) / time_span;
  }

  wide_difficulty_type next_difficulty(vector<uint64_t> timestamps, vector<wide_difficulty_type> cumulative_difficulties, size_t target_seconds) {
    //cutoff DIFFICULTY_LAG
    if(timestamps.size() > DIFFICULTY_WINDOW)
    {
      timestamps.resize(DIFFICULTY_WINDOW);
      cumulative_difficulties.resize(DIFFICULTY_WINDOW);
    }


    size_t length = timestamps.size();
    assert(length == cumulative_difficulties.size());
    if (length <= 1) {
      return 1;
    }
    static_assert(DIFFICULTY_WINDOW >= 2, "Window is too small");
    assert(length <= DIFFICULTY_WINDOW);
    sort(timestamps.begin(), timestamps.end());
    size_t cut_begin, cut_end;
    static_assert(2 * DIFFICULTY_CUT <= DIFFICULTY_WINDOW - 2, "Cut length is too large");
    if (length <= DIFFICULTY_WINDOW - 2 * DIFFICULTY_CUT) {
      cut_begin = 0;
      cut_end = length;
    } else {
      cut_begin = (length - (DIFFICULTY_WINDOW - 2 * DIFFICULTY_CUT) + 1) / 2;
      cut_end = cut_begin + (DIFFICULTY_WINDOW - 2 * DIFFICULTY_CUT);
    }
    assert(/*cut_begin >= 0 &&*/ cut_begin + 2 <= cut_end && cut_end <= length);
    uint64_t time_span = timestamps[cut_end - 1] - timestamps[cut_begin];
    if (time_span == 0) {
      time_span = 1;
    }
    wide_difficulty_type total_work = cumulative_difficulties[cut_end - 1] - cumulative_difficulties[cut_begin];
    assert(total_work > 0);
    boost::multiprecision::uint256_t res =  (boost::multiprecision::uint256_t(total_work) * target_seconds + time_span - 1) / time_span;
    if(res > max128bit)
      return 0; // to behave like previuos implementation, may be better return max128bit?
    return res.convert_to<wide_difficulty_type>();
  }

  difficulty_type next_difficulty_old(vector<uint64_t> timestamps, vector<difficulty_type> cumulative_difficulties)
  {
    return next_difficulty_old(std::move(timestamps), std::move(cumulative_difficulties), DIFFICULTY_TARGET);
  }

  wide_difficulty_type next_difficulty(vector<uint64_t> timestamps, vector<wide_difficulty_type> cumulative_difficulties)
  {
    return next_difficulty(std::move(timestamps), std::move(cumulative_difficulties), DIFFICULTY_TARGET);
  }
}
