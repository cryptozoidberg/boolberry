// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"
#include "cumulative_diificulty_adjustments_tests.h"

#include "pos_basic_tests.h"

using namespace epee;
using namespace currency;


cumulative_difficulty_adjustment_test::cumulative_difficulty_adjustment_test()
{
  REGISTER_CALLBACK_METHOD(cumulative_difficulty_adjustment_test, configure_core);
  REGISTER_CALLBACK_METHOD(cumulative_difficulty_adjustment_test, configure_check_height1);
  REGISTER_CALLBACK_METHOD(cumulative_difficulty_adjustment_test, memorize_main_chain);
  REGISTER_CALLBACK_METHOD(cumulative_difficulty_adjustment_test, check_main_chain);
}
#define FIRST_ALIAS_NAME "first"
#define SECOND_ALIAS_NAME "second"

bool cumulative_difficulty_adjustment_test::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1338224400;
  std::list<currency::account_base> coin_stake_sources;

  GENERATE_ACCOUNT(miner_account);
  GENERATE_ACCOUNT(some_account_1);

  coin_stake_sources.push_back(miner_account);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);
  MAKE_ACCOUNT(events, first_acc);
  DO_CALLBACK(events, "configure_core");


  MAKE_NEXT_BLOCK(events, blk_1, blk_0, miner_account);
  REWIND_BLOCKS_N(events, blk_11, blk_1, miner_account, 10);
  MAKE_NEXT_POS_BLOCK(events, blk_12, blk_11, miner_account, coin_stake_sources);
  //DO_CALLBACK(events, "configure_check_height1");

  MAKE_NEXT_POS_BLOCK(events, blk_27, blk_12, miner_account, coin_stake_sources);
  MAKE_NEXT_POS_BLOCK(events, blk_28, blk_27, miner_account, coin_stake_sources);
  MAKE_NEXT_POS_BLOCK(events, blk_29, blk_28, miner_account, coin_stake_sources);
  MAKE_NEXT_POS_BLOCK(events, blk_30, blk_29, miner_account, coin_stake_sources);
  MAKE_NEXT_POS_BLOCK(events, blk_31, blk_30, miner_account, coin_stake_sources);
  MAKE_NEXT_POS_BLOCK(events, blk_32, blk_31, miner_account, coin_stake_sources);
   MAKE_NEXT_POS_BLOCK(events, blk_33, blk_32, miner_account, coin_stake_sources);
   MAKE_NEXT_POS_BLOCK(events, blk_34, blk_33, miner_account, coin_stake_sources);
  MAKE_NEXT_POS_BLOCK(events, blk_35, blk_34, miner_account, coin_stake_sources);
  MAKE_NEXT_POS_BLOCK(events, blk_36, blk_35, miner_account, coin_stake_sources);
  MAKE_NEXT_POS_BLOCK(events, blk_37, blk_36, miner_account, coin_stake_sources);
  MAKE_NEXT_POS_BLOCK(events, blk_38, blk_37, miner_account, coin_stake_sources);
  MAKE_NEXT_POS_BLOCK(events, blk_39, blk_38, miner_account, coin_stake_sources);
//   MAKE_NEXT_POS_BLOCK(events, blk_40, blk_39, miner_account, coin_stake_sources);
//   MAKE_NEXT_POS_BLOCK(events, blk_41, blk_40, miner_account, coin_stake_sources);
//   MAKE_NEXT_POS_BLOCK(events, blk_42, blk_41, miner_account, coin_stake_sources);
//   MAKE_NEXT_POS_BLOCK(events, blk_43, blk_42, miner_account, coin_stake_sources);
//   MAKE_NEXT_POS_BLOCK(events, blk_44, blk_43, miner_account, coin_stake_sources);
//   MAKE_NEXT_POS_BLOCK(events, blk_45, blk_44, miner_account, coin_stake_sources);
//   MAKE_NEXT_POS_BLOCK(events, blk_46, blk_45, miner_account, coin_stake_sources);
//   MAKE_NEXT_POS_BLOCK(events, blk_47, blk_46, miner_account, coin_stake_sources);
//   MAKE_NEXT_POS_BLOCK(events, blk_48, blk_47, miner_account, coin_stake_sources);
//   MAKE_NEXT_POS_BLOCK(events, blk_49, blk_48, miner_account, coin_stake_sources);
//   MAKE_NEXT_POS_BLOCK(events, blk_50, blk_49, miner_account, coin_stake_sources);
//   MAKE_NEXT_POS_BLOCK(events, blk_51, blk_50, miner_account, coin_stake_sources);
//   MAKE_NEXT_POS_BLOCK(events, blk_52, blk_51, miner_account, coin_stake_sources);
//   MAKE_NEXT_POS_BLOCK(events, blk_53, blk_52, miner_account, coin_stake_sources);
//   MAKE_NEXT_POS_BLOCK(events, blk_54, blk_53, miner_account, coin_stake_sources);
//   MAKE_NEXT_POS_BLOCK(events, blk_55, blk_54, miner_account, coin_stake_sources);
  DO_CALLBACK(events, "memorize_main_chain");
  MAKE_NEXT_BLOCK(events, blk_39_pow, blk_38, miner_account);
  DO_CALLBACK(events, "check_main_chain");
  return true;
}

bool cumulative_difficulty_adjustment_test::configure_core(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  currency::pos_config pc = get_default_pos_config();
  pc.min_coinage = DIFFICULTY_POW_TARGET * 10; //four blocks
  pc.pos_minimum_heigh = 4; //four blocks

  c.get_blockchain_storage().set_pos_config(pc);
  return true;
}
bool cumulative_difficulty_adjustment_test::configure_check_height1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  uint64_t h = c.get_current_blockchain_height();  
  CHECK_EQ(h, 27);
  return true;
}
bool cumulative_difficulty_adjustment_test::memorize_main_chain(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  currency::block b_from_main = boost::get<currency::block>(events[ev_index - 1]);

  bool r = c.get_blockchain_storage().get_block_extended_info_by_hash(get_block_hash(b_from_main), bei);
  CHECK_EQ(r, true);

  return true;
}
bool cumulative_difficulty_adjustment_test::check_main_chain(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{

  currency::blockchain_storage::block_extended_info bei_2;
  bool r = c.get_blockchain_storage().get_block_extended_info_by_hash(get_block_hash(bei.bl), bei_2);

  //r = is_pos_block(b);
  CHECK_EQ(bei.cumulative_diff_adjusted, bei_2.cumulative_diff_adjusted);
  return true;
}
