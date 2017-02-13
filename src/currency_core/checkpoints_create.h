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
    ADD_CHECKPOINT(1,       "0d6f94ae7e565c6d90ee0087d2feaad4617cd3038c4f8853f26756a6b6d535f3");
    ADD_CHECKPOINT(100,     "4a3783222d3f241bcfb8cec6803b5c5dbea73ddcd35c8f6f8b3320c90ad56c05");
    ADD_CHECKPOINT(1000,    "60200d3f6405a2a338c47aae868128395856d6982dff114b98b04f3187e37fa0");
    ADD_CHECKPOINT(2000,    "66e8cd560748e41a3aeb4d0dc9a071c5d5145929aa6dc0df98baac5b43e584a6");
    ADD_CHECKPOINT(3000,    "0a24811b48634fdd205a7fc9cc594b8fc485ec886be84d546bbbe0a2ff6662e3");
    ADD_CHECKPOINT(5000,    "01a1a9fc533196000143b918ee5a47a5c49d2a19cad812d04e3a16fdcee97fbb");
    ADD_CHECKPOINT(70000,   "e7e9fd61e132d7d4d3aefdbf1759bdde2e728b9c84d245deefb2c574b3cb1b53");
    ADD_CHECKPOINT(100000,  "f235fcdaec6ea73286fc5f45b3557d47b61bf85b58b917b63dc3dae6e4bc2f3f");
    ADD_CHECKPOINT(150000,  "3c61e912b4eee300476890e69a13d57d7432a0cfe639c15663a67113a7cecc7b");
    ADD_CHECKPOINT(200000,  "f397e4a0067a40f702795d4ab1bcb50e47f36ca3ea3a20801aa44825f028999a");
    ADD_CHECKPOINT(250000,  "7f0517fcefd88f40922114941730d0ae1a4a5d4cf49b1c4e6dedfac70966c5c1");
    ADD_CHECKPOINT(300000,  "4ae780aa4ba3e751dfc16c000d0514268983b342f3633159b4200750ece10ce6");
    ADD_CHECKPOINT(350000,  "388100e954eec21f3fd54576f03c90f19e4c89e06b1b572022db106c9d421c28");
    ADD_CHECKPOINT(400000,  "32dde08e598dfa5dbd3712e5ea9b069947f7b30e0e209271fbf9087eb24d9939");
    ADD_CHECKPOINT(450000,  "d8429fd77423d0e0f3804bcf20d305b174dfc26ab0547b2152772c241f436428");
    ADD_CHECKPOINT(500000,  "774f103be1dbe3e4531dabdeb611b3ba2810bef14f9de7392eaf182320ff6153");
    ADD_CHECKPOINT(550000,  "2582cc179b45a480ed229bd21ca0e8337b944d44401f2177971bf23e22b416b4");
#endif
    return true;
  }
}
