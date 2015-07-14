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
  void account_base::generate()
  {
    generate_keys(m_keys.m_account_address.m_spend_public_key, m_keys.m_spend_secret_key);
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
  std::string account_base::get_restore_data() const
  {
    return std::string((const char*)&m_keys.m_spend_secret_key, sizeof(m_keys.m_spend_secret_key));
  }
  //-----------------------------------------------------------------
  bool account_base::restore_keys(const std::string& restore_data)
  {
    CHECK_AND_ASSERT_MES(restore_data.size() == sizeof(m_keys.m_spend_secret_key), false, "wrong restore data size");
    memcpy(&m_keys.m_spend_secret_key, &restore_data[0], sizeof(m_keys.m_spend_secret_key));

    dependent_key(m_keys.m_spend_secret_key, m_keys.m_view_secret_key);

    bool r = crypto::secret_key_to_public_key(m_keys.m_spend_secret_key, m_keys.m_account_address.m_spend_public_key);
    CHECK_AND_ASSERT_MES(r, false, "failed to secret_key_to_public_key for secrete key");
    r = crypto::secret_key_to_public_key(m_keys.m_view_secret_key, m_keys.m_account_address.m_view_public_key);
    CHECK_AND_ASSERT_MES(r, false, "failed to secret_key_to_public_key for view key");
    return true;
  }
  //-----------------------------------------------------------------
  std::string account_base::get_public_address_str()
  {
    //TODO: change this code into base 58
    return get_account_address_as_str(m_keys.m_account_address);
  }
  //-----------------------------------------------------------------
}