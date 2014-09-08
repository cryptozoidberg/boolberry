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
    ADD_CHECKPOINT(1,  "0d6f94ae7e565c6d90ee0087d2feaad4617cd3038c4f8853f26756a6b6d535f3");
    ADD_CHECKPOINT(100,  "4a3783222d3f241bcfb8cec6803b5c5dbea73ddcd35c8f6f8b3320c90ad56c05");
    ADD_CHECKPOINT(1000,  "60200d3f6405a2a338c47aae868128395856d6982dff114b98b04f3187e37fa0");
    ADD_CHECKPOINT(2000,  "66e8cd560748e41a3aeb4d0dc9a071c5d5145929aa6dc0df98baac5b43e584a6");
    ADD_CHECKPOINT(3000,  "0a24811b48634fdd205a7fc9cc594b8fc485ec886be84d546bbbe0a2ff6662e3");
    ADD_CHECKPOINT(5000,  "01a1a9fc533196000143b918ee5a47a5c49d2a19cad812d04e3a16fdcee97fbb");
    ADD_CHECKPOINT(70000,  "e7e9fd61e132d7d4d3aefdbf1759bdde2e728b9c84d245deefb2c574b3cb1b53");
#endif
    return true;
  }
}
