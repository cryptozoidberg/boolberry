// Copyright (c) 2012-2018 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "db_lmdb_adapter.h"
#include "misc_language.h"
#include "db/liblmdb/lmdb.h"
#include "common/util.h"

// TODO: estimate correct size
#define LMDB_MEMORY_MAP_SIZE (16ull * 1024 * 1024 * 1024)

#define CHECK_DB_CALL_RESULT(result, return_value, msg) \
  CHECK_AND_ASSERT_MES(result == MDB_SUCCESS,  \
    return_value, "LMDB error " << result << ", " << mdb_strerror(result) << ", " << msg)

namespace db
{
  struct lmdb_adapter_impl
  {
    lmdb_adapter_impl()
      : p_mdb_env(nullptr)
    {}

    MDB_txn* get_current_transaction() const
    {
      CHECK_AND_ASSERT_MES(!m_transaction_stack.empty(), nullptr, "m_transaction_stack is empty");
      return m_transaction_stack.back();
    }

    bool has_active_transaction() const
    {
      return !m_transaction_stack.empty();
    }

    MDB_env* p_mdb_env;
    std::list<MDB_txn*> m_transaction_stack;
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
    CHECK_DB_CALL_RESULT(r, false, "mdb_env_create failed");
      
    r = mdb_env_set_maxdbs(m_p_impl->p_mdb_env, 15);
    CHECK_DB_CALL_RESULT(r, false, "mdb_env_set_maxdbs failed");

    r = mdb_env_set_mapsize(m_p_impl->p_mdb_env, LMDB_MEMORY_MAP_SIZE);
    CHECK_DB_CALL_RESULT(r, false, "mdb_env_set_mapsize failed");
      
    m_db_folder = db_name;
    // TODO: any UTF-8 checks?
    bool br = tools::create_directories_if_necessary(m_db_folder);
    CHECK_AND_ASSERT_MES(br, false, "create_directories_if_necessary failed");

    r = mdb_env_open(m_p_impl->p_mdb_env, m_db_folder.c_str(), MDB_NORDAHEAD , 0644);
    CHECK_DB_CALL_RESULT(r, false, "mdb_env_open failed, m_db_folder = " << m_db_folder);

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

  bool lmdb_adapter::open_table(const std::string& table_name, table_id &tid)
  {
    MDB_dbi dbi = AUTO_VAL_INIT(dbi);

    begin_transaction();
    int r = mdb_dbi_open(m_p_impl->get_current_transaction(), table_name.c_str(), MDB_CREATE, &dbi);
    CHECK_DB_CALL_RESULT(r, false, "mdb_dbi_open failed to open table " << table_name);
    commit_transaction();

    tid = static_cast<table_id>(dbi);
    return true;
  }
  
  bool lmdb_adapter::clear_table(const table_id tid)
  {
    int r = mdb_drop(m_p_impl->get_current_transaction(), static_cast<MDB_dbi>(tid), 0);
    CHECK_DB_CALL_RESULT(r, false, "mdb_drop failed");
    return true;
  }

  uint64_t lmdb_adapter::get_table_size(const table_id tid)
  {
    return 0;
  }
  
  bool lmdb_adapter::begin_transaction()
  {
    MDB_txn* p_parent_tx = nullptr;
    MDB_txn* p_new_tx = nullptr;
    if (m_p_impl->has_active_transaction())
      p_parent_tx = m_p_impl->get_current_transaction();

    unsigned int flags = 0;
    int r = mdb_txn_begin(m_p_impl->p_mdb_env, p_parent_tx, flags, &p_new_tx);
    CHECK_DB_CALL_RESULT(r, false, "mdb_txn_begin");

    m_p_impl->m_transaction_stack.push_back(p_new_tx);

    return true;
  }

  bool lmdb_adapter::commit_transaction()
  {
    CHECK_AND_ASSERT_MES(m_p_impl->has_active_transaction(), false, "no active transaction");

    MDB_txn* txn = m_p_impl->m_transaction_stack.back();
    m_p_impl->m_transaction_stack.pop_back();
          
    int r = 0;
    r = mdb_txn_commit(txn);
    CHECK_DB_CALL_RESULT(r, false, "mdb_txn_commit failed");

    return true;
  }

  void lmdb_adapter::abort_transaction()
  {
    CHECK_AND_ASSERT_MES_NO_RET(m_p_impl->has_active_transaction(), "no active transaction");

    MDB_txn* txn = m_p_impl->m_transaction_stack.back();
    m_p_impl->m_transaction_stack.pop_back();
          
    mdb_txn_abort(txn);
  }
  
  bool lmdb_adapter::get(const table_id tid, const char* key_data, size_t key_size, std::string& out_buffer)
  {
    int r = 0;
    MDB_val key = AUTO_VAL_INIT(key);
    MDB_val data = AUTO_VAL_INIT(data);
    key.mv_data = const_cast<char*>(key_data);
    key.mv_size = key_size;
    
    r = mdb_get(m_p_impl->get_current_transaction(), static_cast<MDB_dbi>(tid), &key, &data);
    
    if (r == MDB_NOTFOUND)
      return false;

    CHECK_DB_CALL_RESULT(r, false, "mdb_get failed");

    out_buffer.assign(reinterpret_cast<const char*>(data.mv_data), data.mv_size);
    return true;
  }

  bool lmdb_adapter::set(const table_id tid, const char* key_data, size_t key_size, const char* value_data, size_t value_size)
  {
    int r = 0;
    MDB_val key = AUTO_VAL_INIT(key);
    MDB_val data = AUTO_VAL_INIT(data);
    key.mv_data = const_cast<char*>(key_data);
    key.mv_size = key_size;
    data.mv_data = const_cast<char*>(value_data);
    data.mv_size = value_size;

    r = mdb_put(m_p_impl->get_current_transaction(), static_cast<MDB_dbi>(tid), &key, &data, 0);
    CHECK_DB_CALL_RESULT(r, false, "mdb_put failed");
    return true;
  }


} // namespace db
