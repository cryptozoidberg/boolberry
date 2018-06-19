// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "gtest/gtest.h"

extern "C"
{
  #include "crypto/random.h"
}

#include "common/util.h"
#include "currency_core/currency_format_utils.h"


TEST(parse_and_validate_tx_extra, is_correct_parse_and_validate_tx_extra)
{
  currency::transaction tx = AUTO_VAL_INIT(tx);
  currency::account_base acc;
  acc.generate();
  currency::blobdata b = "dsdsdfsdfsf";
  bool r = currency::construct_miner_tx(0, 0, 10000000000000, 1000, DEFAULT_FEE, acc.get_keys().m_account_address, tx, b, 1);
  ASSERT_TRUE(r);
  crypto::public_key tx_pub_key;
  r = currency::parse_and_validate_tx_extra(tx, tx_pub_key);
  ASSERT_TRUE(r);
}
TEST(parse_and_validate_tx_extra, is_correct_extranonce_too_big)
{
  currency::transaction tx = AUTO_VAL_INIT(tx);
  currency::account_base acc;
  acc.generate();
  currency::blobdata b(260, 0);
  bool r = currency::construct_miner_tx(0, 0, 10000000000000, 1000, DEFAULT_FEE, acc.get_keys().m_account_address, tx, b, 1);
  ASSERT_FALSE(r);
}
TEST(parse_and_validate_tx_extra, is_correct_wrong_extra_couner_too_big)
{
  currency::transaction tx = AUTO_VAL_INIT(tx);
  tx.extra.resize(20, 0);
  tx.extra[0] = TX_EXTRA_TAG_USER_DATA;
  tx.extra[1] = 255;
  crypto::public_key tx_pub_key;
  bool r = parse_and_validate_tx_extra(tx, tx_pub_key);
  ASSERT_FALSE(r);
}
TEST(parse_and_validate_tx_extra, is_correct_wrong_extra_nonce_double_entry)
{
  currency::transaction tx = AUTO_VAL_INIT(tx);
  tx.extra.resize(20, 0);
  currency::blobdata v = "asasdasd";
  currency::add_tx_extra_nonce(tx, v);
  currency::add_tx_extra_nonce(tx, v);
  crypto::public_key tx_pub_key;
  bool r = parse_and_validate_tx_extra(tx, tx_pub_key);
  ASSERT_FALSE(r);
}

TEST(parse_and_validate_tx_extra, user_data_test)
{
  currency::transaction tx = AUTO_VAL_INIT(tx);
  tx.extra.resize(20, 0);
  tx.extra[0] = TX_EXTRA_TAG_USER_DATA;
  tx.extra[1] = 18;
  currency::tx_extra_info ei = AUTO_VAL_INIT(ei);
  bool r = parse_and_validate_tx_extra(tx, ei);
  ASSERT_TRUE(r);
}


TEST(parse_and_validate_tx_extra, test_payment_ids)
{
  currency::transaction tx = AUTO_VAL_INIT(tx);
  char h[100];
  generate_random_bytes(sizeof h, h);
  currency::payment_id_t payment_id(h, sizeof h);
  bool r = currency::set_payment_id_to_tx_extra(tx.extra, payment_id);
  ASSERT_TRUE(r);
  
  currency::payment_id_t payment_id_2;
  r = currency::get_payment_id_from_tx_extra(tx, payment_id_2);
  ASSERT_TRUE(r);
  ASSERT_EQ(payment_id, payment_id_2);
}

template<int sz>
struct t_dummy
{
  char a[sz];
};
template <typename forced_to_pod_t>
void force_random(forced_to_pod_t& o)
{

  t_dummy<sizeof(forced_to_pod_t)> d = crypto::rand<t_dummy<sizeof(forced_to_pod_t)>>();
  o = *reinterpret_cast<forced_to_pod_t*>(&d);
}

TEST(parse_and_validate_tx_extra, put_and_load_alias)
{

  currency::transaction miner_tx = AUTO_VAL_INIT(miner_tx);
  currency::account_public_address acc = AUTO_VAL_INIT(acc);
  currency::alias_info alias = AUTO_VAL_INIT(alias);
  force_random(alias.m_address);
  force_random(alias.m_sign);
  force_random(alias.m_view_key);
  alias.m_alias = "sdsdsd";
  alias.m_text_comment = "werwrwerw";

  bool res = currency::construct_miner_tx(0, 0, 0, 0, 0, 1000, acc, acc, acc, miner_tx, currency::blobdata(), 10, 50, alias);
  currency::tx_extra_info ei = AUTO_VAL_INIT(ei);
  bool r = parse_and_validate_tx_extra(miner_tx, ei);
  ASSERT_TRUE(r);
  if(ei.m_alias.m_address.m_spend_public_key == alias.m_address.m_spend_public_key &&
    ei.m_alias.m_address.m_view_public_key == alias.m_address.m_view_public_key &&
    ei.m_alias.m_alias == alias.m_alias &&
    ei.m_alias.m_sign == alias.m_sign &&
    ei.m_alias.m_text_comment == alias.m_text_comment &&
    ei.m_alias.m_view_key == alias.m_view_key)
  {
    return;
  }else 
    ASSERT_TRUE(false);
}


TEST(validate_parse_amount_case, validate_parse_amount)
{
  uint64_t res = 0;
  bool r = currency::parse_amount(res, "0.0001");
  ASSERT_TRUE(r);
  ASSERT_EQ(res, 100000000);

  r = currency::parse_amount(res, "100.0001");
  ASSERT_TRUE(r);
  ASSERT_EQ(res, 100000100000000);

  r = currency::parse_amount(res, "000.0000");
  ASSERT_TRUE(r);
  ASSERT_EQ(res, 0);

  r = currency::parse_amount(res, "0");
  ASSERT_TRUE(r);
  ASSERT_EQ(res, 0);


  r = currency::parse_amount(res, "   100.0001    ");
  ASSERT_TRUE(r);
  ASSERT_EQ(res, 100000100000000);

  r = currency::parse_amount(res, "   100.0000    ");
  ASSERT_TRUE(r);
  ASSERT_EQ(res, 100000000000000);

  r = currency::parse_amount(res, "   100. 0000    ");
  ASSERT_FALSE(r);

  r = currency::parse_amount(res, "100. 0000");
  ASSERT_FALSE(r);

  r = currency::parse_amount(res, "100 . 0000");
  ASSERT_FALSE(r);

  r = currency::parse_amount(res, "100.00 00");
  ASSERT_FALSE(r);

  r = currency::parse_amount(res, "1 00.00 00");
  ASSERT_FALSE(r);
}
