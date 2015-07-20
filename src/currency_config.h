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
#define CURRENCY_POS_BLOCK_FUTURE_TIME_LIMIT          60*20

#define BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW             60

// TOTAL_MONEY_SUPPLY - total number coins to be generated
#define TOTAL_MONEY_SUPPLY                            ((uint64_t)(-1))

#define TEST_FAST_EMISSION_CURVE

#ifndef TEST_FAST_EMISSION_CURVE 
#define POS_START_HEIGHT                              60000
#define SIGNIFICANT_EMISSION_RANGE                    920000
#define SIGNIFICANT_EMISSION_SWICH_HEIGHT             910000
#define SIGNIFICANT_EMISSION_REWARD_MULTIPLIER        5 / 16
#define PERCENTS_PERIOD                               262800
#else
#define POS_START_HEIGHT                              600
#define SIGNIFICANT_EMISSION_RANGE                    9200
#define SIGNIFICANT_EMISSION_SWICH_HEIGHT             9100
#define SIGNIFICANT_EMISSION_REWARD_MULTIPLIER        300000
#define PERCENTS_PERIOD                               2628
#endif


#define CURRENCY_TO_KEY_OUT_RELAXED                   0
#define CURRENCY_TO_KEY_OUT_FORCED_NO_MIX             1

#define CURRENCY_REWARD_BLOCKS_WINDOW                 400
#define CURRENCY_BLOCK_GRANTED_FULL_REWARD_ZONE       500000 //size of block (bytes) after which reward for block calculated using block size
#define CURRENCY_COINBASE_BLOB_RESERVED_SIZE          600
#define CURRENCY_MAX_TRANSACTION_BLOB_SIZE            (CURRENCY_BLOCK_GRANTED_FULL_REWARD_ZONE - CURRENCY_COINBASE_BLOB_RESERVED_SIZE*2) 
#define CURRENCY_DISPLAY_DECIMAL_POINT                8

// COIN - number of smallest units in one coin
#define COIN                                            ((uint64_t)100000000) // pow(10, 8)
#define DEFAULT_DUST_THRESHOLD                          ((uint64_t)1000000) // pow(10, 6)

#define TX_POOL_MINIMUM_FEE                             ((uint64_t)1000000) // pow(10, 7)



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

#define MAX_ALIAS_PER_BLOCK                             4
#define ALIAS_COAST_PERIOD                              CURRENCY_BLOCK_PER_DAY*7 //week

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
#define POS_SCAN_WINDOW                                 60*10 //seconds//(60*20) // 10 minutes
#define POS_SCAN_STEP                                   15    //seconds
//#define POS_MAX_COINAGE                                 (60*60*24*90) // 90 days
#define POS_STARTER_KERNEL_HASH                         "bd82e18d42a7ad239588b24fd356d63cc82717e1fae8f6a492cd25d62fda263f"
#define POS_MODFIFIER_INTERVAL                          10
#define POS_WALLET_MINING_SCAN_INTERVAL                 POS_SCAN_STEP  //seconds

#define WALLET_FILE_SIGNATURE                           0x1111011101101011LL  //Bender's nightmare
#define WALLET_FILE_MAX_BODY_SIZE                       0x88888888L //2GB
#define WALLET_FILE_MAX_KEYS_SIZE                       10000 //2GB

#define OFFER_MAXIMUM_LIFE_TIME                         (60*60*24*14)  // 2 weeks       

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

//premine
#define PREMINE_AMOUNT                                  (100000000*COIN)
#define PREMINE_WALLET_ADDRESS_0                        "HgBGCZTVFVA3uaeQx1Tyi7Vz1StYcWofGF3seFfiduzwadHcj4ha7PGgLwgHzVbzmTV1vpEbDnpuaUF6CAcvwkM8GstFX5R"
#define PREMINE_WALLET_ADDRESS_1                        "HgBGCZTVFVA3uaeQx1Tyi7Vz1StYcWofGF3seFfiduzwadHcj4ha7PGgLwgHzVbzmTV1vpEbDnpuaUF6CAcvwkM8GstFX5R"
#define PREMINE_WALLET_ADDRESS_2                        "HgBGCZTVFVA3uaeQx1Tyi7Vz1StYcWofGF3seFfiduzwadHcj4ha7PGgLwgHzVbzmTV1vpEbDnpuaUF6CAcvwkM8GstFX5R"
#define PREMINE_WALLET_ADDRESS_3                        "HgBGCZTVFVA3uaeQx1Tyi7Vz1StYcWofGF3seFfiduzwadHcj4ha7PGgLwgHzVbzmTV1vpEbDnpuaUF6CAcvwkM8GstFX5R"
#define PREMINE_WALLET_ADDRESS_4                        "HgBGCZTVFVA3uaeQx1Tyi7Vz1StYcWofGF3seFfiduzwadHcj4ha7PGgLwgHzVbzmTV1vpEbDnpuaUF6CAcvwkM8GstFX5R"
#define PREMINE_WALLET_ADDRESS_5                        "HgBGCZTVFVA3uaeQx1Tyi7Vz1StYcWofGF3seFfiduzwadHcj4ha7PGgLwgHzVbzmTV1vpEbDnpuaUF6CAcvwkM8GstFX5R"
#define PREMINE_WALLET_ADDRESS_6                        "HgBGCZTVFVA3uaeQx1Tyi7Vz1StYcWofGF3seFfiduzwadHcj4ha7PGgLwgHzVbzmTV1vpEbDnpuaUF6CAcvwkM8GstFX5R"
#define PREMINE_WALLET_ADDRESS_7                        "HgBGCZTVFVA3uaeQx1Tyi7Vz1StYcWofGF3seFfiduzwadHcj4ha7PGgLwgHzVbzmTV1vpEbDnpuaUF6CAcvwkM8GstFX5R"
#define PREMINE_WALLET_ADDRESS_8                        "HgBGCZTVFVA3uaeQx1Tyi7Vz1StYcWofGF3seFfiduzwadHcj4ha7PGgLwgHzVbzmTV1vpEbDnpuaUF6CAcvwkM8GstFX5R"
#define PREMINE_WALLET_ADDRESS_9                        "HgBGCZTVFVA3uaeQx1Tyi7Vz1StYcWofGF3seFfiduzwadHcj4ha7PGgLwgHzVbzmTV1vpEbDnpuaUF6CAcvwkM8GstFX5R"

//alias registration wallet
#define ALIAS_REWARDS_ACCOUNT_SPEND_PUB_KEY             "c6022907d695103e53b0efd0853d31fe2ee53726dc3fb233169244d54c170269" //SHA-256 from text "ALIAS_REWARDS_ACCOUNT_SPEND_PUB_KEY"
#define ALIAS_REWARDS_ACCOUNT_VIEW_PUB_KEY              "360514ab3e6856659d3d7b1c6100a6e05662899729f33ca518bb3e13e076fac1"
#define ALIAS_REWARDS_ACCOUNT_VIEW_SEC_KEY              "a6948a493fca24e13fce41bf18514e88d865ea236d1c30b4472edfb55220b60e"



#define CURRENCY_POOLDATA_FILENAME                      "poolstate.bin"
#define CURRENCY_BLOCKCHAINDATA_FILENAME                "blockchain.bin"
#define CURRENCY_BLOCKCHAINDATA_TEMP_FILENAME           "blockchain.bin.tmp"
#define P2P_NET_DATA_FILENAME                           "p2pstate.bin"
#define MINER_CONFIG_FILENAME                           "miner_conf.json"
#define GUI_SECURE_CONFIG_FILENAME                      "gui_secure_conf.bin"
#define GUI_CONFIG_FILENAME                             "gui_settings.json"
