// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "include_base_utils.h"
using namespace epee;

#include "currency_basic_impl.h"
#include "string_tools.h"
#include "serialization/binary_utils.h"
#include "serialization/vector.h"
#include "currency_format_utils.h"
#include "currency_config.h"
#include "misc_language.h"
#include "common/base58.h"
#include "common/int-util.h"
#include "common/util.h"
#include "crypto/hash.h"

namespace currency {

  /************************************************************************/
  /* Cryptonote helper functions                                          */
  /************************************************************************/
  //-----------------------------------------------------------------------------------------------
  size_t get_max_block_size()
  {
    return CURRENCY_MAX_BLOCK_SIZE;
  }
  //-----------------------------------------------------------------------------------------------
  size_t get_max_tx_size()
  {
    return CURRENCY_MAX_TX_SIZE;
  }
  //-----------------------------------------------------------------------------------------------
  uint64_t get_donations_anount_for_day(uint64_t already_donated_coins, const std::vector<bool>& votes)
  {
    CHECK_AND_ASSERT_THROW_MES(votes.size() == CURRENCY_DONATIONS_INTERVAL, "wrong flags vector size");

    size_t votes_count = 0;
    for(auto v: votes)
      if(v) ++votes_count;

    int64_t amount = 0;
    for(size_t i = 0; i != CURRENCY_DONATIONS_INTERVAL; i++)
    {
      amount += (DONATIONS_SUPPLY - already_donated_coins) >> EMISSION_CURVE_CHARACTER;
      already_donated_coins -= amount;
    }
    amount = (amount*votes_count)/CURRENCY_DONATIONS_INTERVAL;

    amount = amount - amount%DEFAULT_DUST_THRESHOLD;
    return amount;
  }
  //-----------------------------------------------------------------------------------------------
  void get_donation_parts(uint64_t total_donations, uint64_t& royalty, uint64_t& donation)
  {
    royalty = total_donations/10; 
    donation = total_donations - royalty;
  }
  //-----------------------------------------------------------------------------------------------
  bool get_block_reward(size_t median_size, size_t current_block_size, uint64_t already_generated_coins, uint64_t already_donated_coins, uint64_t &reward, uint64_t& /*max_donation*/) 
  {    
    uint64_t base_reward = (EMISSION_SUPPLY - already_generated_coins) >> EMISSION_CURVE_CHARACTER;
    //max_donation = (DONATIONS_SUPPLY - already_donated_coins) >> EMISSION_CURVE_CHARACTER;
    //crop dust
    base_reward = base_reward - base_reward%DEFAULT_DUST_THRESHOLD;
    //max_donation = max_donation - max_donation%DEFAULT_DUST_THRESHOLD;


    //make it soft
    if (median_size < CURRENCY_BLOCK_GRANTED_FULL_REWARD_ZONE) 
    {
      median_size = CURRENCY_BLOCK_GRANTED_FULL_REWARD_ZONE;
    }

    if (current_block_size <= median_size) 
    {
      reward = base_reward;
      return true;
    }

    if(current_block_size > 2 * median_size) 
    {
      LOG_PRINT_L4("Block cumulative size is too big: " << current_block_size << ", expected less than " << 2 * median_size);
      return false;
    }

    CHECK_AND_ASSERT_MES(median_size < std::numeric_limits<uint32_t>::max(), false, "median_size < std::numeric_limits<uint32_t>::max() asert failed");
    CHECK_AND_ASSERT_MES(current_block_size < std::numeric_limits<uint32_t>::max(), false, "current_block_size < std::numeric_limits<uint32_t>::max()");

    uint64_t product_hi;
    uint64_t product_lo = mul128(base_reward, current_block_size * (2 * median_size - current_block_size), &product_hi);

    uint64_t reward_hi;
    uint64_t reward_lo;
    div128_32(product_hi, product_lo, static_cast<uint32_t>(median_size), &reward_hi, &reward_lo);
    div128_32(reward_hi, reward_lo, static_cast<uint32_t>(median_size), &reward_hi, &reward_lo);
    CHECK_AND_ASSERT_MES(0 == reward_hi, false, "0 == reward_hi");
    CHECK_AND_ASSERT_MES(reward_lo < base_reward, false, "reward_lo < base_reward");

    reward = reward_lo;
    return true;
  }
  //------------------------------------------------------------------------------------
  uint8_t get_account_address_checksum(const public_address_outer_blob& bl)
  {
    const unsigned char* pbuf = reinterpret_cast<const unsigned char*>(&bl);
    uint8_t summ = 0;
    for(size_t i = 0; i!= sizeof(public_address_outer_blob)-1; i++)
      summ += pbuf[i];

    return summ;
  }
  //-----------------------------------------------------------------------
  std::string get_account_address_as_str(const account_public_address& adr)
  {
    return tools::base58::encode_addr(CURRENCY_PUBLIC_ADDRESS_BASE58_PREFIX, t_serializable_object_to_blob(adr));
  }
  //-----------------------------------------------------------------------
  bool is_coinbase(const transaction& tx)
  {
    if(tx.vin.size() != 1)
      return false;

    if(tx.vin[0].type() != typeid(txin_gen))
      return false;

    return true;
  }
  //-----------------------------------------------------------------------
  bool get_account_address_from_str(account_public_address& adr, const std::string& str)
  {
    if (2 * sizeof(public_address_outer_blob) != str.size())
    {
      blobdata data;
      uint64_t prefix;
      if (!tools::base58::decode_addr(str, prefix, data))
      {
        LOG_PRINT_L1("Invalid address format");
        return false;
      }

      if (CURRENCY_PUBLIC_ADDRESS_BASE58_PREFIX != prefix)
      {
        LOG_PRINT_L1("Wrong address prefix: " << prefix << ", expected " << CURRENCY_PUBLIC_ADDRESS_BASE58_PREFIX);
        return false;
      }

      if (!::serialization::parse_binary(data, adr))
      {
        LOG_PRINT_L1("Account public address keys can't be parsed");
        return false;
      }

      if (!crypto::check_key(adr.m_spend_public_key) || !crypto::check_key(adr.m_view_public_key))
      {
        LOG_PRINT_L1("Failed to validate address keys");
        return false;
      }
    }
    else
    {
      // Old address format
      std::string buff;
      if(!string_tools::parse_hexstr_to_binbuff(str, buff))
        return false;

      if(buff.size()!=sizeof(public_address_outer_blob))
      {
        LOG_PRINT_L1("Wrong public address size: " << buff.size() << ", expected size: " << sizeof(public_address_outer_blob));
        return false;
      }

      public_address_outer_blob blob = *reinterpret_cast<const public_address_outer_blob*>(buff.data());


      if(blob.m_ver > CURRENCY_PUBLIC_ADDRESS_TEXTBLOB_VER)
      {
        LOG_PRINT_L1("Unknown version of public address: " << blob.m_ver << ", expected " << CURRENCY_PUBLIC_ADDRESS_TEXTBLOB_VER);
        return false;
      }

      if(blob.check_sum != get_account_address_checksum(blob))
      {
        LOG_PRINT_L1("Wrong public address checksum");
        return false;
      }

      //we success
      adr = blob.m_address;
    }

    return true;
  }

  bool operator ==(const currency::transaction& a, const currency::transaction& b) {
    return currency::get_transaction_hash(a) == currency::get_transaction_hash(b);
  }

  bool operator ==(const currency::block& a, const currency::block& b) {
    return currency::get_block_hash(a) == currency::get_block_hash(b);
  }
}

//--------------------------------------------------------------------------------
bool parse_hash256(const std::string str_hash, crypto::hash& hash)
{
  std::string buf;
  bool res = epee::string_tools::parse_hexstr_to_binbuff(str_hash, buf);
  if (!res || buf.size() != sizeof(crypto::hash))
  {
    std::cout << "invalid hash format: <" << str_hash << '>' << std::endl;
    return false;
  }
  else
  {
    buf.copy(reinterpret_cast<char *>(&hash), sizeof(crypto::hash));
    return true;
  }
}
