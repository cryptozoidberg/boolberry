// Copyright (c) 2012-2014 The HoneyPenny developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "gtest/gtest.h"

#include <cstdint>
#include <vector>

#include "currency_core/currency_format_utils.h"
#include "common/boost_serialization_helper.h"
#include "currency_core/currency_boost_serialization.h"

TEST(block_pack_unpack, basic_struct_packing)
{
  currency::block b = AUTO_VAL_INIT(b);
  currency::generate_genesis_block(b);
  currency::blobdata blob = currency::t_serializable_object_to_blob(b);
  currency::block b_loaded = AUTO_VAL_INIT(b_loaded);
  currency::parse_and_validate_block_from_blob(blob, b_loaded);
  crypto::hash original_id = get_block_hash(b);
  crypto::hash loaded_id = get_block_hash(b_loaded);
  ASSERT_EQ(original_id, loaded_id);

  std::stringstream ss;
  boost::archive::binary_oarchive a(ss);
  a << b;
  currency::block b_loaded_from_boost = AUTO_VAL_INIT(b_loaded_from_boost);
  boost::archive::binary_iarchive a2(ss);
  a2 >> b_loaded_from_boost;
  crypto::hash loaded_boost_id = get_block_hash(b_loaded_from_boost);
  ASSERT_EQ(original_id, loaded_boost_id);
}
