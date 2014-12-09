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
  void account_base::set_null()
  {
    m_keys = account_keys();
  }
  //-----------------------------------------------------------------
  vector<unsigned char> account_base::generate()
  {
    vector<unsigned char> seed = generate_keys(m_keys.m_account_address.m_spend_public_key, m_keys.m_spend_secret_key);
	vector<unsigned char> seed2(32, 0);
	keccak(&seed[0], seed.size(), &seed2[0], seed2.size());
    restore_keys(m_keys.m_account_address.m_view_public_key, m_keys.m_view_secret_key, seed2);
    m_creation_timestamp = time(NULL);
    return seed;
  }
   //-----------------------------------------------------------------
  void account_base::restore(const vector<unsigned char>& restore_seed)
  {
    assert(restore_seed.size() == 32);
	vector<unsigned char> seed2(32, 0);
	keccak(&restore_seed[0], restore_seed.size(), &seed2[0], seed2.size());
	restore_keys(m_keys.m_account_address.m_spend_public_key, m_keys.m_spend_secret_key, restore_seed);
    restore_keys(m_keys.m_account_address.m_view_public_key, m_keys.m_view_secret_key, seed2);
    m_creation_timestamp = time(NULL);
  }
   //-----------------------------------------------------------------
  const account_keys& account_base::get_keys() const
  {
    return m_keys;
  }
  //-----------------------------------------------------------------
  std::string account_base::get_public_address_str()
  {
    //TODO: change this code into base 58
    return get_account_address_as_str(m_keys.m_account_address);
  }
  //-----------------------------------------------------------------
}
