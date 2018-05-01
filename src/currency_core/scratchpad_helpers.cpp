// Copyright (c) 2012-2018 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "scratchpad_helpers.h"


namespace currency
{

  scratchpad_wrapper::scratchpad_wrapper(scratchpad_container& m_db_scratchpad) :m_rdb_scratchpad(m_db_scratchpad)
  {}
  bool scratchpad_wrapper::init()
  {
    LOG_PRINT_MAGENTA("Loading scratchpad cache...", LOG_LEVEL_0);
    //load scratchpad from db to cache
    size_t count = m_rdb_scratchpad.size();
    m_scratchpad_cache.resize(count);
    size_t i = 0;
    for (; i != count; i++)
    {
      m_scratchpad_cache[i] = m_rdb_scratchpad[i];
    }
    LOG_PRINT_MAGENTA("Scratchpad loaded OK(" << (i * 32) / 1024 << "kB)", LOG_LEVEL_0);
    return true;
  }

  const std::vector<crypto::hash>& scratchpad_wrapper::get_scratchpad()
  {
    return m_scratchpad_cache;
  }
  void scratchpad_wrapper::set_scratchpad(const std::vector<crypto::hash>& sc)
  {
    m_rdb_scratchpad.clear();
    for (size_t i = 0; i != sc.size(); i++)
    {
      m_rdb_scratchpad.push_back(sc[i]);
    }
  }
  bool scratchpad_wrapper::push_block_scratchpad_data(const block& b)
  {
    bool res = currency::push_block_scratchpad_data(b, m_scratchpad_cache);
    res &= currency::push_block_scratchpad_data(b, m_rdb_scratchpad);
    return res;
  }

  bool scratchpad_wrapper::pop_block_scratchpad_data(const block& b)
  {
    bool res = currency::pop_block_scratchpad_data(b, m_scratchpad_cache);
    res &= currency::pop_block_scratchpad_data(b, m_rdb_scratchpad);
    return res;
  }

}
