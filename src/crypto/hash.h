// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <stddef.h>

#include "common/pod-class.h"
#include "generic-ops.h"

namespace crypto {

  extern "C" {
#include "hash-ops.h"
  }

#pragma pack(push, 1)
  POD_CLASS hash {
    char data[HASH_SIZE];
  };
#pragma pack(pop)

  static_assert(sizeof(hash) == HASH_SIZE, "Invalid structure size");

  /*
    Cryptonight hash functions
  */

  inline void cn_fast_hash(const void *data, std::size_t length, hash &hash) {
    cn_fast_hash(data, length, reinterpret_cast<char *>(&hash));
  }

  inline hash cn_fast_hash(const void *data, std::size_t length) {
    hash h;
    cn_fast_hash(data, length, reinterpret_cast<char *>(&h));
    return h;
  }

  inline void tree_hash(const hash *hashes, std::size_t count, hash &root_hash) {
    tree_hash(reinterpret_cast<const char (*)[HASH_SIZE]>(hashes), count, reinterpret_cast<char *>(&root_hash));
  }

  inline uint64_t cn_fast_hash_64(const void *data, std::size_t length)
  {
    static_assert(sizeof(hash) == 4 * sizeof(uint64_t), "types sanity check failed");
    if (length == 0)
      return 0;
    hash h = cn_fast_hash(data, length);
    uint8_t idx = reinterpret_cast<uint8_t*>(&h)[5] % 4;
    return reinterpret_cast<uint64_t*>(&h)[idx];
  }

}

POD_MAKE_HASHABLE(crypto, hash)
