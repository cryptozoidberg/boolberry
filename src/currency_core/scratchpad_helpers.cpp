// Copyright (c) 2012-2018 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "boost/filesystem.hpp"
#include "scratchpad_helpers.h"
#include "file_io_utils.h"
#include "profile_tools.h"

//#define SELF_VALIDATE_SCRATCHPAD

namespace currency
{
  scratchpad_wrapper::scratchpad_wrapper(scratchpad_container& m_db_scratchpad) :m_rdb_scratchpad(m_db_scratchpad)
  {}


  bool scratchpad_wrapper::init(const std::string& config_folder)
  {
    m_config_folder = config_folder;
    LOG_PRINT_MAGENTA("Loading scratchpad cache...", LOG_LEVEL_0);
    bool success_from_cache = false;
    if (epee::file_io_utils::load_file_to_vector(config_folder + "/" + CURRENCY_BLOCKCHAINDATA_SCRATCHPAD_CACHE, m_scratchpad_cache) && m_scratchpad_cache.size())
    {
      LOG_PRINT_MAGENTA("from " << config_folder << "/" << CURRENCY_BLOCKCHAINDATA_SCRATCHPAD_CACHE << " have just been loaded loaded " << m_scratchpad_cache.size() << " elements", LOG_LEVEL_1);
      size_t sz = m_rdb_scratchpad.size();
      if (sz == m_scratchpad_cache.size() && m_scratchpad_cache[m_scratchpad_cache.size() - 1] == m_rdb_scratchpad[m_rdb_scratchpad.size() - 1])
      {
        success_from_cache = true;
        LOG_PRINT_MAGENTA("Scratchpad loaded from cache file OK (" << m_scratchpad_cache.size() << " elements, " << (m_scratchpad_cache.size() * 32) / 1024 << " KB)", LOG_LEVEL_0);
      }
    }
    boost::system::error_code ec;
    boost::filesystem::remove(config_folder + "/" + CURRENCY_BLOCKCHAINDATA_SCRATCHPAD_CACHE, ec); 
    LOG_PRINT_MAGENTA(config_folder << "/" << CURRENCY_BLOCKCHAINDATA_SCRATCHPAD_CACHE << " was removed, status: " << ec, LOG_LEVEL_1);

    if (!success_from_cache)
    {
      LOG_PRINT_MAGENTA("Loading scratchpad from db...", LOG_LEVEL_0);
      //load scratchpad from db to cache
      PROF_L1_START(cache_load_timer);
      bool res = m_rdb_scratchpad.load_all_itmes_to_container(m_scratchpad_cache);
      CHECK_AND_ASSERT_MES(res, false, "scratchpad loading failed");
      PROF_L1_FINISH(cache_load_timer);
      LOG_PRINT_MAGENTA("Scratchpad loaded from db OK (" << m_scratchpad_cache.size() << "elements, " << (m_scratchpad_cache.size() * 32) / 1024 << " KB)" << PROF_L1_STR_MS_STR(" in ", cache_load_timer, " ms"), LOG_LEVEL_0);
    }

    return true;
  }

  bool scratchpad_wrapper::deinit()
  {
#ifdef SELF_VALIDATE_SCRATCHPAD
    std::vector<crypto::hash> scratchpad_cache;
    load_scratchpad_from_db(m_rdb_scratchpad, scratchpad_cache);
    if (scratchpad_cache != m_scratchpad_cache)
    {
      LOG_PRINT_L0("scratchpads mismatch, memory version: "
        << ENDL << dump_scratchpad(m_scratchpad_cache)
        << ENDL << "db version:" << ENDL << dump_scratchpad(scratchpad_cache)
        );
    }
#endif
    epee::file_io_utils::save_buff_to_file(m_config_folder + "/" + CURRENCY_BLOCKCHAINDATA_SCRATCHPAD_CACHE, &m_scratchpad_cache[0], m_scratchpad_cache.size()*sizeof(m_scratchpad_cache[0]));
    LOG_PRINT_MAGENTA(m_scratchpad_cache.size() << " elements (" << m_scratchpad_cache.size() * sizeof(m_scratchpad_cache[0]) << " bytes)" << " has just been saved to " << m_config_folder << " / " << CURRENCY_BLOCKCHAINDATA_SCRATCHPAD_CACHE, LOG_LEVEL_1);
    return true;
  }

  void scratchpad_wrapper::clear()
  {
    m_scratchpad_cache.clear();
    m_rdb_scratchpad.clear();
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
#ifdef SELF_VALIDATE_SCRATCHPAD
    std::vector<crypto::hash> scratchpad_cache;
    load_scratchpad_from_db(m_rdb_scratchpad, scratchpad_cache);
    if (scratchpad_cache != m_scratchpad_cache)
    {
      LOG_PRINT_L0("scratchpads mismatch, memory version: "
        << ENDL << dump_scratchpad(m_scratchpad_cache)
        << ENDL << "db version:" << ENDL << dump_scratchpad(scratchpad_cache)
        );
    }
#endif
    return res;
  }

  bool scratchpad_wrapper::pop_block_scratchpad_data(const block& b)
  {
    bool res = currency::pop_block_scratchpad_data(b, m_scratchpad_cache);
    res &= currency::pop_block_scratchpad_data(b, m_rdb_scratchpad);
    return res;
  }

}
