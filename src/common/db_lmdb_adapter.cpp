// Copyright (c) 2012-2018 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "db_lmdb_adapter.h"
#include "misc_language.h"
#include "db/liblmdb/lmdb.h"
#include "common/util.h"

// TODO: estimate correct size
#define LMDB_MEMORY_MAP_SIZE (16ull * 1024 * 1024 * 1024)

#define CHECK_LOCAL(result, return_value, msg) \
  CHECK_AND_ASSERT_MES(result == MDB_SUCCESS,  \
    return_value, "LMDB error " << result << ", " << mdb_strerror(result) << ", " << msg)

namespace db
{
  struct lmdb_adapter_impl
  {
    lmdb_adapter_impl()
      : p_mdb_env(nullptr)
    {}

    MDB_env* p_mdb_env;
  };


  lmdb_adapter::lmdb_adapter()
  {
    m_p_impl = new lmdb_adapter_impl();
  }

  lmdb_adapter::~lmdb_adapter()
  {
    close();
    
    lmdb_adapter_impl* p = m_p_impl;
    m_p_impl = nullptr;
    delete p;
  }

  bool lmdb_adapter::open(const std::string& db_name)
  {
    int r = mdb_env_create(&m_p_impl->p_mdb_env);
    CHECK_LOCAL(r, false, "mdb_env_create failed");
      
    r = mdb_env_set_maxdbs(m_p_impl->p_mdb_env, 15);
    CHECK_LOCAL(r, false, "mdb_env_set_maxdbs failed");

    r = mdb_env_set_mapsize(m_p_impl->p_mdb_env, LMDB_MEMORY_MAP_SIZE);
    CHECK_LOCAL(r, false, "mdb_env_set_mapsize failed");
      
    m_db_folder = db_name;
    // TODO: any UTF-8 checks?
    r = tools::create_directories_if_necessary(m_db_folder);
    CHECK_AND_ASSERT_MES(r, false, "create_directories_if_necessary failed");

    r = mdb_env_open(m_p_impl->p_mdb_env, m_db_folder.c_str(), MDB_NORDAHEAD , 0644);
    CHECK_LOCAL(r, false, "mdb_env_open failed, m_db_folder = " << m_db_folder);

    return true;
  }
  
  bool lmdb_adapter::close()
  {
    if (m_p_impl->p_mdb_env)
    {
      mdb_env_close(m_p_impl->p_mdb_env);
      m_p_impl->p_mdb_env = nullptr;
    }
    return true;
  }

  bool lmdb_adapter::open_table(const std::string& table_name, const table_id h)
  {
    return true;
  }
  
  bool lmdb_adapter::clear_table(const table_id tid)
  {
    return true;
  }

  uint64_t lmdb_adapter::get_table_size(const table_id tid)
  {
    return 0;
  }
  
  bool lmdb_adapter::begin_transaction()
  {
    return true;
  }

  bool lmdb_adapter::commit_transaction()
  {
    return true;
  }

  void lmdb_adapter::abort_transaction()
  {
  }

} // namespace db
