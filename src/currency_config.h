// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#define CURRENCY_MAX_BLOCK_NUMBER                     500000000
#define CURRENCY_MAX_BLOCK_SIZE                       500000000  // block header blob limit, never used!
#define CURRENCY_MAX_TX_SIZE                          1000000000
#define CURRENCY_PUBLIC_ADDRESS_BASE58_PREFIX         1 // addresses start with "1"
#define CURRENCY_PUBLIC_INTEG_ADDRESS_BASE58_PREFIX   0xE94E
#define CURRENCY_MINED_MONEY_UNLOCK_WINDOW            10
#define CURRENT_TRANSACTION_VERSION                   1
#define CURRENT_BLOCK_MAJOR_VERSION                   1
#define CURRENT_BLOCK_MINOR_VERSION                   0
#define CURRENCY_BLOCK_FUTURE_TIME_LIMIT              60*60*2

#define BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW             60

// TOTAL_MONEY_SUPPLY - total number coins to be generated
#define TOTAL_MONEY_SUPPLY                            ((uint64_t)(-1))
#define DONATIONS_SUPPLY                              (TOTAL_MONEY_SUPPLY/100) 
#define EMISSION_SUPPLY                               (TOTAL_MONEY_SUPPLY - DONATIONS_SUPPLY) 

#define EMISSION_CURVE_CHARACTER                      20


#define CURRENCY_TO_KEY_OUT_RELAXED                   0
#define CURRENCY_TO_KEY_OUT_FORCED_NO_MIX             1

#define CURRENCY_REWARD_BLOCKS_WINDOW                 400
#define CURRENCY_BLOCK_GRANTED_FULL_REWARD_ZONE       30000 //size of block (bytes) after which reward for block calculated using block size
#define CURRENCY_COINBASE_BLOB_RESERVED_SIZE          600
#define CURRENCY_MAX_TRANSACTION_BLOB_SIZE            (CURRENCY_BLOCK_GRANTED_FULL_REWARD_ZONE - CURRENCY_COINBASE_BLOB_RESERVED_SIZE*2) 
#define CURRENCY_DISPLAY_DECIMAL_POINT                12

// COIN - number of smallest units in one coin
#define COIN                                            ((uint64_t)100000000) // pow(10, 8)
#define DEFAULT_DUST_THRESHOLD                          ((uint64_t)1000000) // pow(10, 6)

#define DEFAULT_FEE                                     ((uint64_t)1000000000) // pow(10, 9)
#define TX_POOL_MINIMUM_FEE                             ((uint64_t)10000000) // pow(10, 7)



#define ORPHANED_BLOCKS_MAX_COUNT                       100


#define DIFFICULTY_TARGET                               120 // seconds
#define DIFFICULTY_WINDOW                               720 // blocks
#define DIFFICULTY_LAG                                  15  // !!!
#define DIFFICULTY_CUT                                  60  // timestamps to cut after sorting
#define DIFFICULTY_BLOCKS_COUNT                         (DIFFICULTY_WINDOW + DIFFICULTY_LAG)

#define CURRENCY_BLOCK_PER_DAY                          ((60*60*24)/(DIFFICULTY_TARGET))

#define CURRENCY_LOCKED_TX_ALLOWED_DELTA_SECONDS        (DIFFICULTY_TARGET * CURRENCY_LOCKED_TX_ALLOWED_DELTA_BLOCKS)
#define CURRENCY_LOCKED_TX_ALLOWED_DELTA_BLOCKS         1

#define DIFFICULTY_BLOCKS_ESTIMATE_TIMESPAN             DIFFICULTY_TARGET //just alias


#define BLOCKS_IDS_SYNCHRONIZING_DEFAULT_COUNT          10000  //by default, blocks ids count in synchronizing
#define BLOCKS_SYNCHRONIZING_DEFAULT_COUNT              200    //by default, blocks count in blocks downloading
#define CURRENCY_PROTOCOL_HOP_RELAX_COUNT               3      //value of hop, after which we use only announce of new block


#define CURRENCY_ALT_BLOCK_LIVETIME_COUNT               (720*7)//one week
#define CURRENCY_MEMPOOL_TX_LIVETIME                    86400 //seconds, one day
#define CURRENCY_MEMPOOL_TX_FROM_ALT_BLOCK_LIVETIME     (CURRENCY_ALT_BLOCK_LIVETIME_COUNT*DIFFICULTY_TARGET) //seconds, one week


#ifndef TESTNET
#define P2P_DEFAULT_PORT                                10101
#define RPC_DEFAULT_PORT                                10102
#else 
#define P2P_DEFAULT_PORT                                50101
#define RPC_DEFAULT_PORT                                50102
#endif

#define COMMAND_RPC_GET_BLOCKS_FAST_MAX_COUNT           1000

#define P2P_LOCAL_WHITE_PEERLIST_LIMIT                  1000
#define P2P_LOCAL_GRAY_PEERLIST_LIMIT                   5000

#define P2P_DEFAULT_CONNECTIONS_COUNT                   8
#define P2P_DEFAULT_HANDSHAKE_INTERVAL                  60           //secondes
#define P2P_DEFAULT_PACKET_MAX_SIZE                     50000000     //50000000 bytes maximum packet size
#define P2P_DEFAULT_PEERS_IN_HANDSHAKE                  250
#define P2P_DEFAULT_CONNECTION_TIMEOUT                  5000       //5 seconds
#define P2P_DEFAULT_PING_CONNECTION_TIMEOUT             2000       //2 seconds
#define P2P_DEFAULT_INVOKE_TIMEOUT                      60*2*1000  //2 minutes
#define P2P_DEFAULT_HANDSHAKE_INVOKE_TIMEOUT            5000       //5 seconds
#define P2P_MAINTAINERS_PUB_KEY                         "d2f6bc35dc4e4a43235ae12620df4612df590c6e1df0a18a55c5e12d81502aa7"
#define P2P_MAINTAINERS_PUB_KEY2                        "b92345cfdeeef9dd837614e85d66b145cce38360de52102a5fbf4ef8709e88ca"
#define P2P_DEFAULT_WHITELIST_CONNECTIONS_PERCENT       70
#define P2P_FAILED_ADDR_FORGET_SECONDS                  (60*60)     //1 hour

#define P2P_IP_BLOCKTIME                                (60*60*24)  //24 hour
#define P2P_IP_FAILS_BEFOR_BLOCK                        10
#define P2P_IDLE_CONNECTION_KILL_INTERVAL               (5*60) //5 minutes


/* This money will go to growth of the project */
  #define CURRENCY_DONATIONS_ADDRESS                     "1Dmynv2xdH1WJKDh4ynWuDYrFbPRw15WDC6UDnA82SLi4zBFEVQ3fu4VT2Lc7T7WkxgnGipZAoR4LMrdyK2XiPC6JWyD2bZ"
  #define CURRENCY_DONATIONS_ADDRESS_TRACKING_KEY        "18316c72364e65abe895f5b1e7d5d7972a323f2cb7ce0c345a2eeb56e245310b"
/* This money will go to the founder of CryptoNote technology, 10% of donations */
  #define CURRENCY_ROYALTY_ADDRESS                       "1BDNvkzLabRjRMFmAxJqXK7b5JFHBNuk7Q6JHrdtxsy36oUMMH8VfBEUxEfcX6aLC9eAXX2pWxTncQRW87MyFbbY7WoQahe"
  #define CURRENCY_ROYALTY_ADDRESS_TRACKING_KEY          "53725c36758c365fa372643d68c83d80e1bf0f57189e5b52b1d1a0499e08c903"





#ifdef TESTNET
  #define CURRENCY_DONATIONS_INTERVAL                     10
#else
  #define CURRENCY_DONATIONS_INTERVAL                     720
#endif


#define ALLOW_DEBUG_COMMANDS

#define CURRENCY_NAME_BASE                              "Boolberry"
#define CURRENCY_NAME_SHORT_BASE                        "boolb"
#ifndef TESTNET
#define CURRENCY_NAME                                   CURRENCY_NAME_BASE
#define CURRENCY_NAME_SHORT                             CURRENCY_NAME_SHORT_BASE
#else
#define CURRENCY_NAME                                   CURRENCY_NAME_BASE"_testnet"
#define CURRENCY_NAME_SHORT                             CURRENCY_NAME_SHORT_BASE"_testnet"
#endif

#define CURRENCY_POOLDATA_FILENAME                      "poolstate.bin"
//#define CURRENCY_BLOCKCHAINDATA_FILENAME                "blockchain.bin"
//#define CURRENCY_BLOCKCHAINDATA_TEMP_FILENAME           "blockchain.bin.tmp"
#define CURRENCY_BLOCKCHAINDATA_FOLDERNAME              "blockchain"
#define CURRENCY_BLOCKCHAINDATA_SCRATCHPAD_CACHE        "scratchpad.cache"
#define P2P_NET_DATA_FILENAME                           "p2pstate.bin"
#define MINER_CONFIG_FILENAME                           "miner_conf.json"
#define GUI_CONFIG_FILENAME                             "gui_conf.json"
#define CURRENCY_CORE_INSTANCE_LOCK_FILE                "lock.lck"
