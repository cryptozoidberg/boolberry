// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"
#include "chaingen_tests_list.h"

#include "get_random_outs.h"

using namespace epee;
using namespace currency;

#define MIX_ATTR_TEST_AMOUNT 11111111111


get_random_outs_test::get_random_outs_test()
{
  REGISTER_CALLBACK_METHOD(get_random_outs_test, check_get_rand_outs);
}

bool get_random_outs_test::generate(std::vector<test_event_entry>& events) const
{
#define TEST_COIN_TRANSFER_UNIQUE_SIZE    111010210111

  uint64_t ts_start = 1338224400;
  GENERATE_ACCOUNT(miner_account);

  //TODO                                                                              events
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);                                           //  0
  MAKE_ACCOUNT(events, bob_account);                                                                    //  1
  MAKE_NEXT_BLOCK(events, blk_1, blk_0, miner_account);                                                 //  3
  MAKE_NEXT_BLOCK(events, blk_2, blk_1, miner_account);                                                 //  4
  MAKE_NEXT_BLOCK(events, blk_3, blk_2, miner_account);                                                 //  5
  REWIND_BLOCKS(events, blk_3r, blk_3, miner_account);                                                  // <N blocks>
  MAKE_TX_LIST_START(events, txs_blk_4, miner_account, miner_account, TEST_COIN_TRANSFER_UNIQUE_SIZE, blk_3);

  MAKE_TX_LIST(events, txs_blk_4, miner_account, miner_account, TEST_COIN_TRANSFER_UNIQUE_SIZE, blk_3);                    //  7 + N
  MAKE_TX_LIST(events, txs_blk_4, miner_account, miner_account, TEST_COIN_TRANSFER_UNIQUE_SIZE, blk_3);                    //  8 + N
  MAKE_TX_LIST(events, txs_blk_4, miner_account, bob_account,   TEST_COIN_TRANSFER_UNIQUE_SIZE, blk_3);                    //  9 + N

  MAKE_NEXT_BLOCK_TX_LIST(events, blk_4, blk_3r, miner_account, txs_blk_4);                             // 10 + N

  MAKE_TX_LIST_START(events, txs_blk_5, bob_account, miner_account, TEST_COIN_TRANSFER_UNIQUE_SIZE - TESTS_DEFAULT_FEE, blk_4);
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_5, blk_4, miner_account, txs_blk_5);                             // 10 + N
  REWIND_BLOCKS(events, blk_5r, blk_5, miner_account);                                                  // <N blocks>
  DO_CALLBACK(events, "check_get_rand_outs");                                                                
  return true;
}

bool get_random_outs_test::check_get_rand_outs(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{

  currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::request req = AUTO_VAL_INIT(req);
  req.amounts.push_back(TEST_COIN_TRANSFER_UNIQUE_SIZE);
  req.outs_count = 4;
  req.use_forced_mix_outs = false;
  currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::response res = AUTO_VAL_INIT(res);
  c.get_blockchain_storage().get_random_outs_for_amounts(req, res);
  CHECK_EQ(res.outs[0].outs.size(), 3);
  return true;
}

