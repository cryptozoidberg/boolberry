// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once 
#include "chaingen.h"

struct mix_attr_tests : public test_chain_unit_base
{
  mix_attr_tests ();
  
  
  bool check_tx_verification_context(const currency::tx_verification_context& tvc, bool tx_added, size_t event_idx, const currency::transaction& /*blk*/)
  {
    return true;
  }
  bool check_block_verification_context(const currency::block_verification_context& bvc, size_t event_idx, const currency::block& /*blk*/)
  {
    return true;
  }
  bool generate(std::vector<test_event_entry>& events) const;

  bool check_balances_1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool remember_last_block(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_last_not_changed(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_last2_and_balance(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_last_changed(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
private:
  crypto::hash top_id_befor_split;
};
