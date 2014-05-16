// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"
#include "chaingen_tests_list.h"

#include "mixin_attr.h"

using namespace epee;
using namespace currency;

#define MIX_ATTR_TEST_AMOUNT 11111111111


mix_attr_tests::mix_attr_tests()
{
  REGISTER_CALLBACK_METHOD(mix_attr_tests, check_balances_1);
  REGISTER_CALLBACK_METHOD(mix_attr_tests, remember_last_block);
  REGISTER_CALLBACK_METHOD(mix_attr_tests, check_last_not_changed);
  REGISTER_CALLBACK_METHOD(mix_attr_tests, check_last2_and_balance);
  REGISTER_CALLBACK_METHOD(mix_attr_tests, check_last_changed);  
}

bool mix_attr_tests::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1338224400;
  GENERATE_ACCOUNT(miner_account);

  //TODO                                                                              events
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);                                           //  0
  MAKE_ACCOUNT(events, bob_account);                                                                    //  1
  MAKE_ACCOUNT(events, alice_account);                                                                  //  2
  MAKE_NEXT_BLOCK(events, blk_1, blk_0, miner_account);                                                 //  3
  MAKE_NEXT_BLOCK(events, blk_2, blk_1, miner_account);                                                 //  4
  MAKE_NEXT_BLOCK(events, blk_3, blk_2, miner_account);                                                 //  5
  REWIND_BLOCKS(events, blk_3r, blk_3, miner_account);                                                  // <N blocks>
  MAKE_TX_LIST_START_MIX_ATTR(events, txs_blk_4, miner_account, 
                                                 miner_account, 
                                                 MK_COINS(5), 
                                                 blk_3, 
                                                 CURRENCY_TO_KEY_OUT_RELAXED);                    //  6 + N

  MAKE_TX_LIST_MIX_ATTR(events, txs_blk_4, miner_account, miner_account, MK_COINS(5), blk_3, 
                                                 CURRENCY_TO_KEY_OUT_RELAXED);                    //  7 + N
  MAKE_TX_LIST_MIX_ATTR(events, txs_blk_4, miner_account, miner_account, MK_COINS(5), blk_3,
                                                 CURRENCY_TO_KEY_OUT_RELAXED);                    //  8 + N
  MAKE_TX_LIST_MIX_ATTR(events, txs_blk_4, miner_account, bob_account, MK_COINS(5), blk_3,
                                                 CURRENCY_TO_KEY_OUT_FORCED_NO_MIX);                    //  9 + N

  MAKE_NEXT_BLOCK_TX_LIST(events, blk_4, blk_3r, miner_account, txs_blk_4);                             // 10 + N
  DO_CALLBACK(events, "check_balances_1");                                                              // 11 + N
  REWIND_BLOCKS(events, blk_4r, blk_4, miner_account);                                                  // <N blocks>

  //let's try to spend it with mixin
  DO_CALLBACK(events, "remember_last_block");                                                           
  MAKE_TX_MIX(events, tx_0, bob_account, alice_account, MK_COINS(5) - TESTS_DEFAULT_FEE, 3, blk_4);    
  MAKE_NEXT_BLOCK_TX1(events, blk_5_f, blk_4r, miner_account, tx_0);                                      
  DO_CALLBACK(events, "check_last_not_changed");                                                                    


  MAKE_TX_MIX_ATTR(events, tx_1, bob_account, alice_account, MK_COINS(5) - TESTS_DEFAULT_FEE, 0, blk_4, CURRENCY_TO_KEY_OUT_RELAXED, false);
  MAKE_NEXT_BLOCK_TX1(events, blk_5, blk_4r, miner_account, tx_1);                                      
  DO_CALLBACK(events, "check_last2_and_balance");                                                                


  //check mixin
  MAKE_ACCOUNT(events, bob_account2);
  MAKE_ACCOUNT(events, alice_account2);


  MAKE_TX_LIST_START_MIX_ATTR(events, txs_blk_6, miner_account, 
                                                 miner_account, 
                                                 MK_COINS(5),
                                                 blk_5, 
                                                 CURRENCY_TO_KEY_OUT_RELAXED);
  //MAKE_TX_LIST_MIX_ATTR(events, txs_blk_6, miner_account, miner_account, MK_COINS(5), blk_5, 4);
  MAKE_TX_LIST_MIX_ATTR(events, txs_blk_6, miner_account, bob_account2, MK_COINS(5), blk_5, 4);
  MAKE_TX_LIST_MIX_ATTR(events, txs_blk_6, miner_account, bob_account2, MK_COINS(5), blk_5, 4);
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_6, blk_5, miner_account, txs_blk_6);
  
  DO_CALLBACK(events, "remember_last_block");                                                           
  MAKE_TX_MIX(events, tx_3, bob_account2, alice_account2, MK_COINS(5) - TESTS_DEFAULT_FEE, 2, blk_6);
  MAKE_NEXT_BLOCK_TX1(events, blk_7_f, blk_6, miner_account, tx_3);                                      
  DO_CALLBACK(events, "check_last_not_changed");                                                                    

  MAKE_TX_MIX(events, tx_4, bob_account2, alice_account2, MK_COINS(5) - TESTS_DEFAULT_FEE, 3, blk_6);
  MAKE_NEXT_BLOCK_TX1(events, blk_7, blk_6, miner_account, tx_4);
  DO_CALLBACK(events, "check_last_changed");                                                                    
  return true;
}

bool mix_attr_tests::check_balances_1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  currency::account_base m_bob_account = boost::get<account_base>(events[1]);
  currency::account_base m_alice_account = boost::get<account_base>(events[2]);

  std::list<block> blocks;
  bool r = c.get_blocks(0, 100 + 2 * CURRENCY_MINED_MONEY_UNLOCK_WINDOW, blocks);
  CHECK_TEST_CONDITION(r);

  std::vector<currency::block> chain;
  map_hash2tx_t mtx;
  r = find_block_chain(events, chain, mtx, get_block_hash(blocks.back()));
  CHECK_TEST_CONDITION(r);
  CHECK_EQ(MK_COINS(5), get_balance(m_bob_account, chain, mtx));
  CHECK_EQ(0, get_balance(m_alice_account, chain, mtx));

  return true;
}

bool mix_attr_tests::remember_last_block(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{  
  top_id_befor_split = c.get_tail_id();
  return true;
}

bool mix_attr_tests::check_last_not_changed(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{  
  CHECK_EQ(top_id_befor_split, c.get_tail_id());
  return true;
}
bool mix_attr_tests::check_last2_and_balance(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{  
  CHECK_NOT_EQ(top_id_befor_split, c.get_tail_id());

  currency::account_base m_bob_account = boost::get<account_base>(events[1]);
  currency::account_base m_alice_account = boost::get<account_base>(events[2]);

  std::list<block> blocks;
  bool r = c.get_blocks(0, 100 + 2 * CURRENCY_MINED_MONEY_UNLOCK_WINDOW, blocks);
  CHECK_TEST_CONDITION(r);

  std::vector<currency::block> chain;
  map_hash2tx_t mtx;
  r = find_block_chain(events, chain, mtx, get_block_hash(blocks.back()));
  CHECK_TEST_CONDITION(r);
  CHECK_EQ(0, get_balance(m_bob_account, chain, mtx));
  CHECK_EQ(MK_COINS(5)-TESTS_DEFAULT_FEE, get_balance(m_alice_account, chain, mtx));

  return true;
}
bool mix_attr_tests::check_last_changed(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{  
  CHECK_NOT_EQ(top_id_befor_split, c.get_tail_id());
  return true;
}