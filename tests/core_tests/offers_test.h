// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once 
#include "chaingen.h"

struct offers_tests : public test_chain_unit_base
{
  offers_tests();

  bool check_block_verification_context(const currency::block_verification_context& bvc, size_t event_idx, const currency::block& /*blk*/)
  {
    return true;
  }

  bool generate(std::vector<test_event_entry>& events) const;
  bool configure_core(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_offers_1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_offers_updated_1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_offers_updated_2(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

private:

  mutable crypto::hash tx_id_1;
  mutable crypto::hash tx_id_2;

};
