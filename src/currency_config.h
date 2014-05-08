// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#define CURRENCY_MAX_BLOCK_NUMBER                     500000000
#define CURRENCY_MAX_BLOCK_SIZE                       500000000  // block header blob limit, never used!
#define CURRENCY_MAX_TX_SIZE                          1000000000
#define CURRENCY_PUBLIC_ADDRESS_TEXTBLOB_VER          0
#define CURRENCY_PUBLIC_ADDRESS_BASE58_PREFIX         1 // addresses start with "1"
#define CURRENCY_MINED_MONEY_UNLOCK_WINDOW            10
#define CURRENT_TRANSACTION_VERSION                   1
#define CURRENT_BLOCK_MAJOR_VERSION                   1
#define CURRENT_BLOCK_MINOR_VERSION                   0
#define CURRENCY_BLOCK_FUTURE_TIME_LIMIT              60*60*2

#define BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW             60

// TOTAL_MONEY_SUPPLY - total number coins to be generated
#define TOTAL_MONEY_SUPPLY                            ((uint64_t)(-1))
#define DONATIONS_SUPPLY                              (TOTAL_MONEY_SUPPLY/10) 
#define EMISSION_SUPPLY                               (TOTAL_MONEY_SUPPLY - DONATIONS_SUPPLY) 

#define EMISSION_CURVE_CHARACTER                      20


#define CURRENCY_TO_KEY_OUT_RELAXED                   0
#define CURRENCY_TO_KEY_OUT_FORCED_NO_MIX             1

#define CURRENCY_REWARD_BLOCKS_WINDOW                 400
#define CURRENCY_BLOCK_GRANTED_FULL_REWARD_ZONE       20000 //size of block (bytes) after which reward for block calculated using block size
#define CURRENCY_COINBASE_BLOB_RESERVED_SIZE          600
#define CURRENCY_DISPLAY_DECIMAL_POINT                8

// COIN - number of smallest units in one coin
#define COIN                                            ((uint64_t)100000000) // pow(10, 8)
#define DEFAULT_DUST_THRESHOLD                          ((uint64_t)1000000) // pow(10, 6)

#define DEFAULT_FEE                                     DEFAULT_DUST_THRESHOLD // pow(10, 6)


#define ORPHANED_BLOCKS_MAX_COUNT                       100


#define DIFFICULTY_TARGET                               120 // seconds
#define DIFFICULTY_WINDOW                               720 // blocks
#define DIFFICULTY_LAG                                  15  // !!!
#define DIFFICULTY_CUT                                  60  // timestamps to cut after sorting
#define DIFFICULTY_BLOCKS_COUNT                         (DIFFICULTY_WINDOW + DIFFICULTY_LAG)


#define CURRENCY_LOCKED_TX_ALLOWED_DELTA_SECONDS        (DIFFICULTY_TARGET * CURRENCY_LOCKED_TX_ALLOWED_DELTA_BLOCKS)
#define CURRENCY_LOCKED_TX_ALLOWED_DELTA_BLOCKS         1

#define CURRENCY_GENESIS_TIME_PROOF                     "0000000000000000000000000000000000000000000000000000000000000000"

#define DIFFICULTY_BLOCKS_ESTIMATE_TIMESPAN             DIFFICULTY_TARGET //just alias


#define BLOCKS_IDS_SYNCHRONIZING_DEFAULT_COUNT          10000  //by default, blocks ids count in synchronizing
#define BLOCKS_SYNCHRONIZING_DEFAULT_COUNT              200    //by default, blocks count in blocks downloading
#define CURRENCY_PROTOCOL_HOP_RELAX_COUNT               3      //value of hop, after which we use only announce of new block


#define P2P_DEFAULT_PORT                                10101
#define RPC_DEFAULT_PORT                                10102
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
#define P2P_DEFAULT_WHITELIST_CONNECTIONS_PERCENT       70
#define P2P_FAILED_ADDR_FORGET_SECONDS                  60*60     //1 hour


/* This money will go to growth of the project */
#define P2P_DONATIONS_ADDRESS                           "1CXauKc2Kq11W5zRXzcQSWTZGyZ81dVhiBmdt5CxbB5MdFY15xUWCDWR3iM2gQokwzCzu31f8BXHvQtZR7yKHLkeCtFaus6"
#define P2P_DONATIONS_ADDRESS_TRACKING_KEY              "ae67e521b6c944198e07daf134b81607e7e7020c00463831c4ed7326e510be0b"
/* This money will go to the founder of Cryptonote technology, 10% of donations */
#define P2P_ROYALTY_ADDRESS                             "1GZftqCMdB2dTmRn5bNuxRWcZAFD6LrGp1sXqSCAY3QTj8mJxojTr8y4ARbfivt2szSkis8XUybf1WDBsW3R22stQqcniF1"
#define P2P_ROYALTY_ADDRESS_TRACKING_KEY                "d8844de0e50915206e86d2b95b7bd34d0e518f381003c9d271c592c350ac5d03"


#define ALLOW_DEBUG_COMMANDS

#define CURRENCY_NAME                                   "HoneyPenny"
#define CURRENCY_POOLDATA_FILENAME                      "poolstate.bin"
#define CURRENCY_BLOCKCHAINDATA_FILENAME                "blockchain.bin"
#define CURRENCY_BLOCKCHAINDATA_TEMP_FILENAME           "blockchain.bin.tmp"
#define P2P_NET_DATA_FILENAME                           "p2pstate.bin"
#define MINER_CONFIG_FILE_NAME                          "miner_conf.json"

