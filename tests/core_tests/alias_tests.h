// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once 
#include "chaingen.h"

struct gen_alias_tests : public test_chain_unit_base
{
  gen_alias_tests();

  bool check_block_verification_context(const currency::block_verification_context& bvc, size_t event_idx, const currency::block& /*blk*/)
  {
      return true;
  }

  bool generate(std::vector<test_event_entry>& events) const;
  bool check_first_alias_added(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_second_alias_added(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_aliases_removed(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_splitted_back(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_alias_changed(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_alias_not_changed(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

};
