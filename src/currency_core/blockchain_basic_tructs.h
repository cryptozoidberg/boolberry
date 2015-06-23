// Copyright (c) 2012-2015 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once 
#include "currency_basic.h"
#include "blockchain_basic_tructs.h"
#include "difficulty.h"
namespace currency
{
  struct block_extended_info
  {
    block   bl;
    uint64_t height;
    size_t block_cumulative_size;
    wide_difficulty_type cumulative_difficulty;
    uint64_t already_generated_coins;
    uint64_t already_donated_coins;
    uint64_t scratch_offset;
  };
}
