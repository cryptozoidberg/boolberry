// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once 
#include "misc_language.h"

namespace currency
{
  struct core_runtime_config
  {
    uint64_t min_coinage;
    uint64_t pos_minimum_heigh; //height
    uint64_t tx_pool_min_fee;
  };

  inline core_runtime_config get_default_core_runtime_config()
  {
    core_runtime_config pc = AUTO_VAL_INIT(pc);
    pc.min_coinage = POS_MIN_COINAGE;
    pc.pos_minimum_heigh = (60 * 60) / DIFFICULTY_TOTAL_TARGET;
    pc.tx_pool_min_fee = TX_POOL_MINIMUM_FEE;
    return pc;
  }
}
