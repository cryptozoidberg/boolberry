// Copyright (c) 2019 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once
#include "currency_basic.h"
#include "difficulty.h"

namespace currency
{

  struct transaction_chain_entry
  {
    transaction tx;
    uint64_t m_keeper_block_height;
    std::vector<uint64_t> m_global_output_indexes;
    std::vector<bool> m_spent_flags;
    uint32_t version;

    DEFINE_SERIALIZATION_VERSION(1)
      BEGIN_SERIALIZE_OBJECT()
      VERSION_ENTRY(version)
      FIELD(version)
      FIELDS(tx)
      FIELD(m_keeper_block_height)
      FIELD(m_global_output_indexes)
      FIELD(m_spent_flags)
      END_SERIALIZE()
  };

  struct block_extended_info
  {
    block   bl;
    uint64_t height;
    size_t block_cumulative_size;
    wide_difficulty_type cumulative_difficulty;
    uint64_t already_generated_coins;
    uint64_t already_donated_coins;
    uint64_t scratch_offset;

    uint32_t version;

    DEFINE_SERIALIZATION_VERSION(1)
      BEGIN_SERIALIZE_OBJECT()
      VERSION_ENTRY(version)
      FIELDS(bl)
      FIELD(height)
      FIELD(block_cumulative_size)
      FIELD(cumulative_difficulty)
      FIELD(already_generated_coins)
      FIELD(already_donated_coins)
      FIELD(scratch_offset)
      END_SERIALIZE()
  };

} // namespace currency
