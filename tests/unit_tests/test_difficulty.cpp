// Copyright (c) 2019 Hyle Team (https://hyle.io/)
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "gtest/gtest.h"

#include "currency_core/difficulty.h"

#include "common/util.h"
#include "currency_core/currency_format_utils.h"



TEST(test_difficulty, basic_difficulty_test)
{

#define HASH_FROM_STR(str, name) crypto::hash name = currency::null_hash; \
  epee::string_tools::parse_tpod_from_hex_string(str, name);

  HASH_FROM_STR("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", full);
  HASH_FROM_STR("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff00", dif_below_256);
  HASH_FROM_STR("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff0000", dif_below_64k);
  HASH_FROM_STR("ffffffffffffffffffffffffffffffffffffffffffffffffffffffff00000000", dif_below_32_bit);
  HASH_FROM_STR("ffffffffffffffffffffffffffffffffffffffffffffffffffff000000000000", dif_below_48_bit); 
  HASH_FROM_STR("ffffffffffffffffffffffffffffffffffffffffffffffff0000000000000000", dif_below_64_bit); // this one crossing over 64 bit
  HASH_FROM_STR("ffffffffffffffffffffffffffffffffffffffff000000000000000000000000", dif_below_96_bit); 
  

  currency::wide_difficulty_type diff = 1;
  bool r = currency::check_hash(full, diff);
  ASSERT_TRUE(r);
  diff = 2;
  r = currency::check_hash(full, diff);
  ASSERT_FALSE(r);
  diff = 256;
  r = currency::check_hash(dif_below_256, diff);
  ASSERT_TRUE(r);
  diff = 257;
  r = currency::check_hash(dif_below_256, diff);
  ASSERT_FALSE(r);
  
  //check 2^16
  diff = 1;
  diff = diff << 16;
  r = currency::check_hash(dif_below_64k, diff);
  ASSERT_TRUE(r);
  diff += 1;
  r = currency::check_hash(dif_below_64k, diff);
  ASSERT_FALSE(r);
  
  //check 2^32
  diff = 1;
  diff = diff << 32;
  r = currency::check_hash(dif_below_32_bit, diff);
  ASSERT_TRUE(r);
  diff += 1;
  r = currency::check_hash(dif_below_32_bit, diff);
  ASSERT_FALSE(r);

  //check 2^48
  diff = 1;
  diff = diff << 48;
  r = currency::check_hash(dif_below_48_bit, diff);
  ASSERT_TRUE(r);
  diff += 1;
  r = currency::check_hash(dif_below_48_bit, diff);
  ASSERT_FALSE(r);

  //check 2^64
  diff = 1;
  diff = diff << 64;
  r = currency::check_hash(dif_below_64_bit, diff);
  ASSERT_TRUE(r);
  diff += 1;
  r = currency::check_hash(dif_below_64_bit, diff);
  ASSERT_FALSE(r);

  //check 2^96
  diff = 1;
  diff = diff << 96;
  r = currency::check_hash(dif_below_96_bit, diff);
  ASSERT_TRUE(r);
  diff += 1;
  r = currency::check_hash(dif_below_96_bit, diff);
  ASSERT_FALSE(r);

}
