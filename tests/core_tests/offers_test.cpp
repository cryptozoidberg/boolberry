// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"
#include "chaingen_tests_list.h"

#include "offers_test.h"

using namespace epee;
using namespace currency;


bool fill_default_offer(offer_details& od)
{
  od.offer_type = OFFER_TYPE_LUI_TO_ETC;
  od.amount_lui = 1000000000;
  od.amount_etc = 22222222;
  od.target = "USD";
  od.location_country = "US";
  od.location_city = "New York City";
  od.contacts = "skype: zina; icq: 12313212; email: zina@zina.com; mobile: +621234567834";
  od.comment = "The best ever rate in NYC!!!";
  od.payment_types = "BTC;BANK;CASH";
  od.expiration_time = 10;
  return true;
}

offers_tests::offers_tests()
{
  REGISTER_CALLBACK_METHOD(offers_tests, configure_core);
  REGISTER_CALLBACK_METHOD(offers_tests, check_offers_1);
  REGISTER_CALLBACK_METHOD(offers_tests, check_offers_updated_1);
  REGISTER_CALLBACK_METHOD(offers_tests, check_offers_updated_2);
}
#define FIRST_UPDATED_LOCATION "Washington"
#define SECOND_UPDATED_LOCATION "LA"

bool offers_tests::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1338224400;

  GENERATE_ACCOUNT(miner_account);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);
  MAKE_ACCOUNT(events, third_acc);
  DO_CALLBACK(events, "configure_core");
  MAKE_NEXT_BLOCK(events, blk_1, blk_0, miner_account);
  REWIND_BLOCKS_N(events, blk_11, blk_1, miner_account, 10);


  //push first offers
  std::vector<currency::attachment_v> attachments;
  attachments.push_back(currency::offer_details());
  offer_details& od = boost::get<currency::offer_details&>(attachments.back());
  fill_default_offer(od);
  MAKE_TX_LIST_START_WITH_ATTACHS(events, txs_blk, miner_account, miner_account, MK_TEST_COINS(1), blk_11, attachments);
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_12, blk_11, miner_account, txs_blk);

  //push second offer
  std::vector<currency::attachment_v> attachments2;
  attachments2.push_back(currency::offer_details());
  offer_details& od2 = boost::get<currency::offer_details&>(attachments2.back());
  fill_default_offer(od2);
  MAKE_TX_LIST_START_WITH_ATTACHS(events, txs_blk2, miner_account, miner_account, MK_TEST_COINS(1), blk_12, attachments2);
  crypto::secret_key offer_secrete_key = last_tx_generated_secrete_key;
  currency::transaction tx = txs_blk2.back();
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_13, blk_12, miner_account, txs_blk2);
  DO_CALLBACK(events, "check_offers_1");

  //now update offer
  std::vector<currency::attachment_v> attachments3;
  attachments3.push_back(currency::update_offer());
  update_offer& uo = boost::get<currency::update_offer&>(attachments3.back());
  fill_default_offer(uo.of);
  uo.of.location_city = "Washington";
  uo.offer_index = 0;
  uo.tx_id = currency::get_transaction_hash(tx);
  blobdata bl_for_sig = currency::make_offer_sig_blob(uo);
  crypto::generate_signature(crypto::cn_fast_hash(bl_for_sig.data(), bl_for_sig.size()),
    currency::get_tx_pub_key_from_extra(tx),
    offer_secrete_key, uo.sig);
  MAKE_TX_LIST_START_WITH_ATTACHS(events, txs_blk3, miner_account, miner_account, MK_TEST_COINS(1), blk_13, attachments3);
  offer_secrete_key = last_tx_generated_secrete_key;
  tx = txs_blk3.back();
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_14, blk_13, miner_account, txs_blk3);
  DO_CALLBACK(events, "check_offers_updated_1");

  //and update this offer once again
  std::vector<currency::attachment_v> attachments4;
  attachments4.push_back(currency::update_offer());
  update_offer& uo2 = boost::get<currency::update_offer&>(attachments4.back());
  fill_default_offer(uo2.of);
  uo2.of.location_city = "LA";
  uo2.offer_index = 0;
  uo2.tx_id = currency::get_transaction_hash(tx);
  bl_for_sig = currency::make_offer_sig_blob(uo2);
  crypto::generate_signature(crypto::cn_fast_hash(bl_for_sig.data(), bl_for_sig.size()),
    currency::get_tx_pub_key_from_extra(tx),
    offer_secrete_key, 
    uo2.sig);
  MAKE_TX_LIST_START_WITH_ATTACHS(events, txs_blk4, miner_account, miner_account, MK_TEST_COINS(1), blk_14, attachments4);
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_15, blk_14, miner_account, txs_blk4);
  DO_CALLBACK(events, "check_offers_updated_2");


  MAKE_NEXT_BLOCK(events, blk_16, blk_15, miner_account);
  

  return true;
}

bool offers_tests::configure_core(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  currency::core_runtime_config pc = c.get_blockchain_storage().get_core_runtime_config();
  pc.min_coinage = TESTS_POS_CONFIG_MIN_COINAGE; //four blocks
  pc.pos_minimum_heigh = TESTS_POS_CONFIG_POS_MINIMUM_HEIGH; //four blocks

  c.get_blockchain_storage().set_core_runtime_config(pc);
  return true;
}
bool offers_tests::check_offers_1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  std::list<offer_details_ex> offers;
  c.get_blockchain_storage().get_all_offers(offers);
  CHECK_EQ(2, offers.size());
  return true;
}
bool offers_tests::check_offers_updated_1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  std::list<offer_details_ex> offers;
  c.get_blockchain_storage().get_all_offers(offers);
  CHECK_EQ(2, offers.size());

  CHECK_EQ(offers.back().location_city, FIRST_UPDATED_LOCATION);

  return true;
}
bool offers_tests::check_offers_updated_2(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  std::list<offer_details_ex> offers;
  c.get_blockchain_storage().get_all_offers(offers);
  CHECK_EQ(2, offers.size());

  CHECK_EQ(offers.back().location_city, SECOND_UPDATED_LOCATION);

  return true;
}

