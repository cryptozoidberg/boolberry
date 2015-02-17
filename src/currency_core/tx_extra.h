// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once 

#define TX_EXTRA_PADDING_MAX_COUNT                    40
#define TX_EXTRA_TAG_PUBKEY                           0x01
#define TX_EXTRA_TAG_USER_DATA                        0x02
#define TX_EXTRA_TAG_ALIAS                            0x03
#define TX_EXTRA_TAG_ALIAS_FLAGS_OP_UPDATE            0x01
#define TX_EXTRA_TAG_ALIAS_FLAGS_ADDR_WITH_TRACK      0x02

//types predefined in TX_EXTRA_TAG_USER_DATA, rules are not strict, just a recommendation
#define TX_USER_DATA_TAG_PAYMENT_ID                   0x00

#define TX_EXTRA_MAX_USER_DATA_SIZE                   250
#define TX_MAX_PAYMENT_ID_SIZE                        TX_EXTRA_MAX_USER_DATA_SIZE

