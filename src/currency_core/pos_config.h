// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once 
#include "misc_language.h"

namespace currency
{
  struct pos_config
  {
    uint64_t min_coinage;
    uint64_t max_coinage;
    uint64_t pos_minimum_heigh; //height
  };

  inline pos_config get_default_pos_config()
  {
    pos_config pc = AUTO_VAL_INIT(pc);
    pc.min_coinage = POS_MIN_COINAGE;
    pc.max_coinage = POS_MAX_COINAGE;
    pc.pos_minimum_heigh = (60 * 60) / DIFFICULTY_TOTAL_TARGET;
    return pc;
  }
}
