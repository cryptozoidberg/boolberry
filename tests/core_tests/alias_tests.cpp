// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"
#include "chaingen_tests_list.h"

#include "alias_tests.h"

using namespace epee;
using namespace currency;


gen_alias_tests::gen_alias_tests()
{
  REGISTER_CALLBACK_METHOD(gen_alias_tests, check_first_alias_added);
  REGISTER_CALLBACK_METHOD(gen_alias_tests, check_second_alias_added);
  REGISTER_CALLBACK_METHOD(gen_alias_tests, check_aliases_removed);
  REGISTER_CALLBACK_METHOD(gen_alias_tests, check_splitted_back);
  REGISTER_CALLBACK_METHOD(gen_alias_tests, check_alias_changed);
  REGISTER_CALLBACK_METHOD(gen_alias_tests, check_alias_not_changed);
}
#define FIRST_ALIAS_NAME "first"
#define SECOND_ALIAS_NAME "second"

bool gen_alias_tests::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1338224400;

  GENERATE_ACCOUNT(miner_account);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);
  MAKE_ACCOUNT(events, first_acc);
  MAKE_ACCOUNT(events, second_acc);
  MAKE_ACCOUNT(events, third_acc);

  MAKE_NEXT_BLOCK(events, blk_1, blk_0, miner_account);
  currency::alias_info ai = AUTO_VAL_INIT(ai);
  ai.m_alias = FIRST_ALIAS_NAME;
  ai.m_text_comment = "first@first.com blablabla";
  ai.m_address = first_acc.get_keys().m_account_address;
  MAKE_NEXT_BLOCK_ALIAS(events, blk_2, blk_1, miner_account, ai);
  DO_CALLBACK(events, "check_first_alias_added");

  ai.m_alias = SECOND_ALIAS_NAME;
  ai.m_text_comment = "second@second.com blablabla";
  ai.m_address = second_acc.get_keys().m_account_address;
  MAKE_NEXT_BLOCK_ALIAS(events, blk_3, blk_2, miner_account, ai);
  DO_CALLBACK(events, "check_second_alias_added");
  
  //check split with remove alias
  MAKE_NEXT_BLOCK(events, blk_2_split, blk_1, miner_account);
  MAKE_NEXT_BLOCK(events, blk_3_split, blk_2_split, miner_account);
  MAKE_NEXT_BLOCK(events, blk_4_split, blk_3_split, miner_account);
  DO_CALLBACK(events, "check_aliases_removed");
    
  //make back split to original and try update alias
  MAKE_NEXT_BLOCK(events, blk_4, blk_3, miner_account);
  MAKE_NEXT_BLOCK(events, blk_5, blk_4, miner_account);
  DO_CALLBACK(events, "check_splitted_back");


  currency::alias_info ai_upd = AUTO_VAL_INIT(ai_upd);
  ai_upd.m_alias = FIRST_ALIAS_NAME;
  ai_upd.m_address =third_acc.get_keys().m_account_address;
  ai_upd.m_text_comment = "changed alias haha";
  bool r = sign_update_alias(ai_upd, first_acc.get_keys().m_account_address.m_spend_public_key, first_acc.get_keys().m_spend_secret_key);
  CHECK_AND_ASSERT_MES(r, false, "failed to sign update_alias");
  MAKE_NEXT_BLOCK_ALIAS(events, blk_6, blk_5, miner_account, ai_upd);
  DO_CALLBACK(events, "check_alias_changed");

  //try to make fake alias change
  currency::alias_info ai_upd_fake = AUTO_VAL_INIT(ai_upd_fake);
  ai_upd_fake.m_alias = SECOND_ALIAS_NAME;
  ai_upd_fake.m_address =third_acc.get_keys().m_account_address;
  ai_upd_fake.m_text_comment = "changed alias haha";
  r = sign_update_alias(ai_upd_fake, second_acc.get_keys().m_account_address.m_spend_public_key, second_acc.get_keys().m_spend_secret_key);
  CHECK_AND_ASSERT_MES(r, false, "failed to sign update_alias");
  ai_upd_fake.m_text_comment = "changed alias haha - fake"; // changed text, signature became wrong
  MAKE_NEXT_BLOCK_ALIAS(events, blk_7, blk_6, miner_account, ai_upd_fake);
  DO_CALLBACK(events, "check_alias_not_changed");


  return true;
}

bool gen_alias_tests::check_first_alias_added(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  const currency::account_base& first_acc = boost::get<const currency::account_base&>(events[1]);
  //currency::account_base& second_acc = boost::get<currency::account_base&>(events[2]);
  //currency::account_base& third_acc = boost::get<currency::account_base&>(events[3]);

  currency::alias_info_base ai = AUTO_VAL_INIT(ai);
  bool r = c.get_blockchain_storage().get_alias_info(FIRST_ALIAS_NAME, ai);
  CHECK_AND_ASSERT_MES(r, false, "first alias name check failed");
  CHECK_AND_ASSERT_MES(ai.m_address.m_spend_public_key == first_acc.get_keys().m_account_address.m_spend_public_key
                    && ai.m_address.m_view_public_key == first_acc.get_keys().m_account_address.m_view_public_key, false, "first alias name check failed");  
  return true;
}
bool gen_alias_tests::check_second_alias_added(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  const currency::account_base& second_acc = boost::get<const currency::account_base&>(events[2]);

  currency::alias_info_base ai = AUTO_VAL_INIT(ai);
  bool r = c.get_blockchain_storage().get_alias_info(SECOND_ALIAS_NAME, ai);
  CHECK_AND_ASSERT_MES(r, false, "first alias name check failed");
  CHECK_AND_ASSERT_MES(ai.m_address.m_spend_public_key == second_acc.get_keys().m_account_address.m_spend_public_key
                    && ai.m_address.m_view_public_key == second_acc.get_keys().m_account_address.m_view_public_key, false, "second alias name check failed");  

  return true;
}
bool gen_alias_tests::check_aliases_removed(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  currency::alias_info_base ai = AUTO_VAL_INIT(ai);
  bool r = c.get_blockchain_storage().get_alias_info(FIRST_ALIAS_NAME, ai);
  CHECK_AND_ASSERT_MES(!r, false, "first alias name check failed");
  r = c.get_blockchain_storage().get_alias_info(SECOND_ALIAS_NAME, ai);
  CHECK_AND_ASSERT_MES(!r, false, "second alias name check failed");
  return true;
}
bool gen_alias_tests::check_splitted_back(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  return check_first_alias_added(c, ev_index, events) && check_second_alias_added(c, ev_index, events);
}

bool gen_alias_tests::check_alias_changed(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  const currency::account_base& third_acc = boost::get<const currency::account_base&>(events[3]);

  currency::alias_info_base ai = AUTO_VAL_INIT(ai);
  bool r = c.get_blockchain_storage().get_alias_info(FIRST_ALIAS_NAME, ai);
  CHECK_AND_ASSERT_MES(r, false, "first alias name check failed");
  CHECK_AND_ASSERT_MES(ai.m_address.m_spend_public_key == third_acc.get_keys().m_account_address.m_spend_public_key
                    && ai.m_address.m_view_public_key == third_acc.get_keys().m_account_address.m_view_public_key, false, "first alias update name check failed");  

  return true;
}

bool gen_alias_tests::check_alias_not_changed(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  return check_second_alias_added(c, ev_index, events);
}
