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
#include "currency_core/swap_address.h"

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
  bool r = currency::set_payment_id_and_swap_addr_to_tx_extra(tx.extra, payment_id);
  ASSERT_TRUE(r);
  
  currency::payment_id_t payment_id_2;
  r = currency::get_payment_id_from_tx_extra(tx, payment_id_2);
  ASSERT_TRUE(r);
  ASSERT_EQ(payment_id, payment_id_2);
}

#if defined(TESTNET)
// this test requires SWAP_ADDRESS_ENCRYPTION_SEC_KEY to run
TEST(parse_and_validate_tx_extra, test_payment_ids_and_swap_address)
{
  currency::transaction tx = AUTO_VAL_INIT(tx);
  char h[100];
  generate_random_bytes(sizeof h, h);
  currency::payment_id_t payment_id(h, sizeof h);
  currency::account_public_address swap_addr = AUTO_VAL_INIT(swap_addr);
  generate_random_bytes(sizeof(swap_addr.m_view_public_key), &swap_addr.m_view_public_key);
  generate_random_bytes(sizeof(swap_addr.m_spend_public_key), &swap_addr.m_spend_public_key);
  swap_addr.is_swap_address = true;

  bool r = currency::set_payment_id_and_swap_addr_to_tx_extra(tx.extra, payment_id, swap_addr);
  ASSERT_TRUE(r);

  currency::keypair tx_keys = currency::keypair::generate();
  ASSERT_TRUE(currency::add_tx_pub_key_to_extra(tx, tx_keys.pub));
  ASSERT_TRUE(currency::encrypt_user_data_with_tx_secret_key(tx_keys.sec, tx.extra)); // encrypt using one-time tx secret key

  currency::payment_id_t payment_id_2;
  r = currency::get_payment_id_from_tx_extra(tx, payment_id_2);
  ASSERT_TRUE(r);
  ASSERT_EQ(payment_id, payment_id_2);

  crypto::secret_key swap_encrypt_sec_key = AUTO_VAL_INIT(swap_encrypt_sec_key);
  ASSERT_TRUE(epee::string_tools::hex_to_pod(SWAP_ADDRESS_ENCRYPTION_SEC_KEY, swap_encrypt_sec_key));

  currency::account_public_address swap_addr2 = AUTO_VAL_INIT(swap_addr2);
  r = currency::get_swap_info_from_tx_extra(tx, swap_encrypt_sec_key, swap_addr2); // decrypt using SWAP_ADDRESS_ENCRYPTION_SEC_KEY
  ASSERT_TRUE(r);
  ASSERT_EQ(swap_addr.m_spend_public_key, swap_addr2.m_spend_public_key);
  ASSERT_EQ(swap_addr.m_view_public_key, swap_addr2.m_view_public_key);
}
#endif // TESTNET


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
  ASSERT_TRUE(res);
  currency::tx_extra_info ei = AUTO_VAL_INIT(ei);
  res = parse_and_validate_tx_extra(miner_tx, ei);
  ASSERT_TRUE(res);
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


TEST(format_utils, print_money)
{

  auto naive_uint64_mixer = [](uint64_t v) -> uint64_t
  {
    v = v * 3935559000370003845 + 2691343689449507681;
    v ^= v >> 21;
    v ^= v << 37;
    v ^= v >> 4;
    v *= 4768777513237032717;
    v ^= v << 20;
    v ^= v >> 41;
    v ^= v << 5;
    v ^= v >> 33;
    v *= 0xff51afd7ed558ccd;
    v ^= v >> 33;
    v *= 0xc4ceb9fe1a85ec53;
    v ^= v >> 33;
    return v;
  };

  auto parse_amount = [](std::string str) -> uint64_t // intentional passing-by-value
  {
    // str is assumed to be space-trimmed
    uint64_t multiplier_pow = CURRENCY_DISPLAY_DECIMAL_POINT;
    size_t p = str.find('.');
    if (p != std::string::npos && p != str.size() - 1)
      multiplier_pow = CURRENCY_DISPLAY_DECIMAL_POINT + 1 + p - str.size();

    if (p != std::string::npos)
      str.erase(p, 1);

    int64_t value = 0;
    epee::string_tools::string_to_num_fast(str, value);

    while (multiplier_pow-- != 0)
      value *= 10;

    return value;
  };

  auto cycle = [&](const std::vector<uint64_t>& increments) -> void
  {
    size_t n = 0;
    for (uint64_t amount = 0; amount < 10000000000000; ++n)
    {
      std::string s = currency::print_money(amount, true);
      uint64_t parsed_amount = parse_amount(s);

      ASSERT_EQ(amount, parsed_amount);

      amount += increments[naive_uint64_mixer(amount) % increments.size()];
    }
    std::cout << "iterations: " << n << ENDL;
  };


  std::vector<uint64_t> increments_dec1{ 100000000 };
  std::vector<uint64_t> increments_dec2{ 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000 };
  std::vector<uint64_t> increments_fib{ 1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144, 233, 377, 610, 987, 1597, 2584, 4181, 6765, 10946, 17711, 28657, 46368, 75025, 121393, 196418, 317811, 514229, 832040, 1346269, 2178309, 3524578, 5702887, 9227465, 14930352, 24157817, 39088169, 63245986, 102334155, 165580141, 267914296 };// , 433494437, 701408733, 1134903170, 1836311903, 2971215073, 4807526976, 7778742049, 12586269025

  cycle(increments_dec1);
  cycle(increments_dec2);
  cycle(increments_fib);

}
