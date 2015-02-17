// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "checkpoints.h"
#include "misc_log_ex.h"

#define ADD_CHECKPOINT(h, hash)  CHECK_AND_ASSERT(checkpoints.add_checkpoint(h,  hash), false);

namespace currency {
  inline bool create_checkpoints(currency::checkpoints& checkpoints)
  {
#ifndef TESTNET
//    ADD_CHECKPOINT(1,  "0d6f94ae7e565c6d90ee0087d2feaad4617cd3038c4f8853f26756a6b6d535f3");
#endif
    return true;
  }
}
