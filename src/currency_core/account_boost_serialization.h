// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "account.h"
#include "currency_core/currency_boost_serialization.h"

//namespace currency {
namespace boost
{
  namespace serialization
  {
    template <class Archive>
    inline void serialize(Archive &a, currency::account_keys &x, const boost::serialization::version_type ver)
    {
      a & x.m_account_address;
      a & x.m_spend_secret_key;
      a & x.m_view_secret_key;
    }

    template <class Archive>
    inline void serialize(Archive &a, currency::account_public_address &x, const boost::serialization::version_type ver)
    {
      a & x.m_spend_public_key;
      a & x.m_view_public_key;
      if (Archive::is_loading::value)
      {
        x.is_swap_address = false;
      }
    }
  }
}
