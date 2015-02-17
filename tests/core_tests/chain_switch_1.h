// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once 
#include "chaingen.h"

/************************************************************************/
/*                                                                      */
/************************************************************************/
class gen_chain_switch_1 : public test_chain_unit_base
{
public: 
  gen_chain_switch_1();

  bool generate(std::vector<test_event_entry>& events) const;

  bool check_split_not_switched(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_split_switched(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

private:
  std::list<currency::block> m_chain_1;

  currency::account_base m_recipient_account_1;
  currency::account_base m_recipient_account_2;
  currency::account_base m_recipient_account_3;
  currency::account_base m_recipient_account_4;

  std::list<currency::transaction> m_tx_pool;
};
