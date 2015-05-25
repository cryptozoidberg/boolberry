// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#define CURRENCY_MAX_BLOCK_NUMBER                     500000000
#define CURRENCY_MAX_BLOCK_SIZE                       500000000  // block header blob limit, never used!
#define CURRENCY_MAX_TX_SIZE                          1000000000
#define CURRENCY_PUBLIC_ADDRESS_TEXTBLOB_VER          0
#define CURRENCY_PUBLIC_ADDRESS_BASE58_PREFIX         99 // addresses start with 
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
#define CURRENCY_BLOCK_GRANTED_FULL_REWARD_ZONE       500000 //size of block (bytes) after which reward for block calculated using block size
#define CURRENCY_COINBASE_BLOB_RESERVED_SIZE          600
#define CURRENCY_MAX_TRANSACTION_BLOB_SIZE            (CURRENCY_BLOCK_GRANTED_FULL_REWARD_ZONE - CURRENCY_COINBASE_BLOB_RESERVED_SIZE*2) 
#define CURRENCY_DISPLAY_DECIMAL_POINT                6

// COIN - number of smallest units in one coin
#define COIN                                            ((uint64_t)100000000) // pow(10, 8)
#define DEFAULT_DUST_THRESHOLD                          ((uint64_t)1000000) // pow(10, 6)

#define DEFAULT_FEE                                     ((uint64_t)1000000000) // pow(10, 9)
#define TX_POOL_MINIMUM_FEE                             ((uint64_t)10000000) // pow(10, 7)



#define ORPHANED_BLOCKS_MAX_COUNT                       100

#define DIFFICULTY_STARTER                              1
#define DIFFICULTY_POS_TARGET                           240 // seconds
#define DIFFICULTY_POW_TARGET                           240 // seconds
#define DIFFICULTY_TOTAL_TARGET                         ((DIFFICULTY_POW_TARGET + DIFFICULTY_POW_TARGET) / 4)
#define DIFFICULTY_WINDOW                               720 // blocks
#define DIFFICULTY_LAG                                  15  // !!!
#define DIFFICULTY_CUT                                  60  // timestamps to cut after sorting
#define DIFFICULTY_BLOCKS_COUNT                         (DIFFICULTY_WINDOW + DIFFICULTY_LAG)

#define CURRENCY_BLOCK_PER_DAY                          ((60*60*24)/(DIFFICULTY_TOTAL_TARGET))

#define CURRENCY_LOCKED_TX_ALLOWED_DELTA_SECONDS        (DIFFICULTY_TOTAL_TARGET * CURRENCY_LOCKED_TX_ALLOWED_DELTA_BLOCKS)
#define CURRENCY_LOCKED_TX_ALLOWED_DELTA_BLOCKS         1

#define DIFFICULTY_BLOCKS_ESTIMATE_TIMESPAN             DIFFICULTY_TOTAL_TARGET //just alias


#define BLOCKS_IDS_SYNCHRONIZING_DEFAULT_COUNT          10000  //by default, blocks ids count in synchronizing
#define BLOCKS_SYNCHRONIZING_DEFAULT_COUNT              200    //by default, blocks count in blocks downloading
#define CURRENCY_PROTOCOL_HOP_RELAX_COUNT               3      //value of hop, after which we use only announce of new block


#define CURRENCY_ALT_BLOCK_LIVETIME_COUNT               (720*7)//one week
#define CURRENCY_MEMPOOL_TX_LIVETIME                    86400 //seconds, one day
#define CURRENCY_MEMPOOL_TX_FROM_ALT_BLOCK_LIVETIME     (CURRENCY_ALT_BLOCK_LIVETIME_COUNT*DIFFICULTY_TOTAL_TARGET) //seconds, one week


#ifndef TESTNET
#define P2P_DEFAULT_PORT                                11111
#define RPC_DEFAULT_PORT                                11112
#else 
#define P2P_DEFAULT_PORT                                51111
#define RPC_DEFAULT_PORT                                51112
#endif

#define COMMAND_RPC_GET_BLOCKS_FAST_MAX_COUNT           4000

#define P2P_LOCAL_WHITE_PEERLIST_LIMIT                  1000
#define P2P_LOCAL_GRAY_PEERLIST_LIMIT                   5000

#define P2P_DEFAULT_CONNECTIONS_COUNT                   8
#define P2P_DEFAULT_HANDSHAKE_INTERVAL                  60           //secondes
#define P2P_DEFAULT_PACKET_MAX_SIZE                     50000000     //50000000 bytes maximum packet size
#define P2P_DEFAULT_PEERS_IN_HANDSHAKE                  250
#define P2P_DEFAULT_CONNECTION_TIMEOUT                  5000       //5 seconds
#define P2P_DEFAULT_PING_CONNECTION_TIMEOUT             2000       //2 seconds
#define P2P_DEFAULT_INVOKE_TIMEOUT                      60*2*1000  //2 minutes
#define P2P_DEFAULT_HANDSHAKE_INVOKE_TIMEOUT            10000       //5 seconds
#define P2P_MAINTAINERS_PUB_KEY                         "d2f6bc35dc4e4a43235ae12620df4612df590c6e1df0a18a55c5e12d81502aa7"
#define P2P_DEFAULT_WHITELIST_CONNECTIONS_PERCENT       70
#define P2P_FAILED_ADDR_FORGET_SECONDS                  (60*60)     //1 hour

#define P2P_IP_BLOCKTIME                                (60*60*24)  //24 hour
#define P2P_IP_FAILS_BEFOR_BLOCK                        10
#define P2P_IDLE_CONNECTION_KILL_INTERVAL               (5*60) //5 minutes

//PoS definitions
#define POS_SCAN_WINDOW                                 60*20 //seconds//(60*20) // 20 minutes
#define POS_SCAN_STEP                                   15    //seconds
#define POS_MIN_COINAGE                                 (60*60) // 1 hour
#define POS_MAX_COINAGE                                 (60*60*24*90) // 90 days
#define POS_STARTER_KERNEL_HASH                         "bd82e18d42a7ad239588b24fd356d63cc82717e1fae8f6a492cd25d62fda263f"
#define POS_MODFIFIER_INTERVAL                          10
#define POS_WALLET_MINING_SCAN_INTERVAL                 POS_SCAN_STEP  //seconds

#define WALLET_FILE_SIGNATURE                           0x1111011101101011LL  //Bender's nightmare
#define WALLET_FILE_MAX_BODY_SIZE                       0x88888888L //2GB
#define WALLET_FILE_MAX_KEYS_SIZE                       10000 //2GB



#define GUI_BLOCKS_DISPLAY_COUNT                        40
#define GUI_DISPATCH_QUE_MAXSIZE                        100

#define ALLOW_DEBUG_COMMANDS

#define CURRENCY_NAME_BASE                              "Lui"
#define CURRENCY_NAME_SHORT_BASE                        "lui"
#ifndef TESTNET
#define CURRENCY_NAME                                   CURRENCY_NAME_BASE
#define CURRENCY_NAME_SHORT                             CURRENCY_NAME_SHORT_BASE
#else
#define CURRENCY_NAME                                   CURRENCY_NAME_BASE"_testnet"
#define CURRENCY_NAME_SHORT                             CURRENCY_NAME_SHORT_BASE"_testnet"
#endif

#define CURRENCY_POOLDATA_FILENAME                      "poolstate.bin"
#define CURRENCY_BLOCKCHAINDATA_FILENAME                "blockchain.bin"
#define CURRENCY_BLOCKCHAINDATA_TEMP_FILENAME           "blockchain.bin.tmp"
#define P2P_NET_DATA_FILENAME                           "p2pstate.bin"
#define MINER_CONFIG_FILENAME                           "miner_conf.json"
#define GUI_SECURE_CONFIG_FILENAME                      "gui_secure_conf.bin"
#define GUI_CONFIG_FILENAME                             "gui_settings.json"
