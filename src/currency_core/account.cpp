// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <fstream>

#include "include_base_utils.h"
#include "account.h"
#include "warnings.h"
#include "crypto/crypto.h"
extern "C" {
#include "crypto/keccak.h"

}
#include "currency_core/currency_basic_impl.h"
#include "currency_core/currency_format_utils.h"
using namespace std;

DISABLE_VS_WARNINGS(4244 4345)

  namespace currency
{
  //-----------------------------------------------------------------
  account_base::account_base()
  {
    set_null();
  }
  //-----------------------------------------------------------------
  account_base::account_base(const account_base& ab)
  {
    m_keys = ab.m_keys;
    m_creation_timestamp = ab.m_creation_timestamp;
  }
  //-----------------------------------------------------------------
  void account_base::set_null()
  {
    m_keys = account_keys();
  }
  //-----------------------------------------------------------------
  vector<unsigned char> account_base::generate()
  {
    vector<unsigned char> seed = generate_keys(m_keys.m_account_address.m_spend_public_key, m_keys.m_spend_secret_key);
    dependent_key(m_keys.m_spend_secret_key, m_keys.m_view_secret_key);
    if (!crypto::secret_key_to_public_key(m_keys.m_view_secret_key, m_keys.m_account_address.m_view_public_key))
      throw std::runtime_error("Failed to create public view key");
    m_creation_timestamp = time(NULL);
    return seed;
  }
  //-----------------------------------------------------------------
  void account_base::restore(const vector<unsigned char>& restore_seed)
  {
    assert(restore_seed.size() == 32);
    restore_keys(m_keys.m_account_address.m_spend_public_key, m_keys.m_spend_secret_key, restore_seed);
    dependent_key(m_keys.m_spend_secret_key, m_keys.m_view_secret_key);
    if (!crypto::secret_key_to_public_key(m_keys.m_view_secret_key, m_keys.m_account_address.m_view_public_key))
      throw std::runtime_error("Failed to create public view key");
    m_creation_timestamp = time(NULL);
  }
   //-----------------------------------------------------------------
  const account_keys& account_base::get_keys() const
  {
    return m_keys;
  }
  //-----------------------------------------------------------------
  void account_base::make_account_view_only()
  {
    m_keys.m_spend_secret_key = currency::null_skey;
  }
  //-----------------------------------------------------------------
  std::string account_base::get_public_address_str()
  {
    return get_account_address_as_str(m_keys.m_account_address);
  }
  //-----------------------------------------------------------------
}
