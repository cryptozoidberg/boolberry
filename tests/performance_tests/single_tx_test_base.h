// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "currency_core/account.h"
#include "currency_core/currency_basic.h"
#include "currency_core/currency_format_utils.h"

class single_tx_test_base
{
public:
  bool init()
  {
    using namespace currency;

    m_bob.generate();
    m_donation_acc.generate();
    m_royalty_acc.generate();

    if (!construct_miner_tx(0, 0, 0, 0, 2, 0, m_bob.get_keys().m_account_address, m_donation_acc.get_keys().m_account_address, m_royalty_acc.get_keys().m_account_address, m_tx, blobdata(), 11))
      return false;

    m_tx_pub_key = get_tx_pub_key_from_extra(m_tx);
    return true;
  }

protected:
  currency::account_base m_bob;
  currency::account_base m_donation_acc;
  currency::account_base m_royalty_acc;
  currency::transaction m_tx;
  crypto::public_key m_tx_pub_key;
};
