// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once 
#include "chaingen.h"

struct gen_pos_basic_tests : public test_chain_unit_base
{
  gen_pos_basic_tests();

  bool check_block_verification_context(const currency::block_verification_context& bvc, size_t event_idx, const currency::block& /*blk*/)
  {
      return true;
  }

  bool generate(std::vector<test_event_entry>& events) const;
  bool configure_core(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool configure_check_height1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool configure_check_height2(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

};
