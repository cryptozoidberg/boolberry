// Copyright (c) 2012-2018 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "db_lmdb_adapter.h"
#include <thread>
#include <mutex>
#include "misc_language.h"
#include "db/liblmdb/lmdb.h"
#include "common/util.h"
#include "boost/thread/recursive_mutex.hpp"
#include "epee/include/misc_language.h"

// TODO: estimate correct size
#define LMDB_MEMORY_MAP_SIZE (128ull * 1024 * 1024 * 1024)

#define CHECK_DB_CALL_RESULT(result, return_value, msg) \
  CHECK_AND_ASSERT_MES(result == MDB_SUCCESS,  \
    return_value, "LMDB error " << result << ", " << mdb_strerror(result) << ", " << msg)

namespace db
{
  struct stack_entry_t
  {
    explicit stack_entry_t(MDB_txn* txn, bool ro_access) : txn(txn), ro_access(ro_access) {}
    MDB_txn* txn;         // lmdb transaction handle
    bool     ro_access;   // if true: this db transaction is declared by user as Read-Only
  };

  struct lmdb_adapter_impl
  {
    lmdb_adapter_impl()
      : p_mdb_env(nullptr)
    {}

    MDB_txn* get_current_transaction() const
    {
      std::lock_guard<boost::recursive_mutex> guard(m_transaction_stack_mutex);
      auto it = m_transaction_stack.find(std::this_thread::get_id());
      CHECK_AND_ASSERT_MES(it != m_transaction_stack.end(), nullptr, "m_transaction_stack is not found for thread id " << std::this_thread::get_id());
      CHECK_AND_ASSERT_MES(!it->second.empty(), nullptr, "m_transaction_stack is empty for thread id " << std::this_thread::get_id());
      return it->second.back().txn;
    }

    bool has_active_transaction() const
    {
      std::lock_guard<boost::recursive_mutex> guard(m_transaction_stack_mutex);
      auto it = m_transaction_stack.find(std::this_thread::get_id());
      return it != m_transaction_stack.end() && !it->second.empty();
    }

    MDB_env* p_mdb_env;
    std::map<std::thread::id, std::list<stack_entry_t>> m_transaction_stack; // thread_id -> (tx_entry, tx_entry, ...)
    mutable boost::recursive_mutex m_transaction_stack_mutex; // protects m_transaction_stack
    mutable boost::recursive_mutex m_begin_commit_abort_mutex; // protects db transaction sequence
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

    r = mdb_env_open(m_p_impl->p_mdb_env, m_db_folder.c_str(), MDB_NORDAHEAD, 0644);
    CHECK_DB_CALL_RESULT(r, false, "mdb_env_open failed, m_db_folder = " << m_db_folder);

    return true;
  }
  
  bool lmdb_adapter::close()
  {
    if (m_p_impl->p_mdb_env)
    {
      {
        std::lock_guard<boost::recursive_mutex> guard(m_p_impl->m_transaction_stack_mutex);
        bool unlock_begin_commit_abort_mutex = false;
        for (auto& pair_id_tx_stack : m_p_impl->m_transaction_stack)
        {
          for(auto& tx_stack : pair_id_tx_stack.second)
          {
            int result = mdb_txn_commit(tx_stack.txn);
            if (result != MDB_SUCCESS)
            {
              LOG_ERROR("mdb_txn_commit: mdb_txn_commit() failed : " << mdb_strerror(result));
            }
            if (!tx_stack.ro_access)
              unlock_begin_commit_abort_mutex = true;
          }
        }
        if (unlock_begin_commit_abort_mutex)
          m_p_impl->m_begin_commit_abort_mutex.lock();
      } // lock_guard : m_p_impl->m_transaction_stack_mutex

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

  size_t lmdb_adapter::get_table_size(const table_id tid)
  {
    MDB_stat table_stat = AUTO_VAL_INIT(table_stat);
    bool local_transaction = false;
    if (!m_p_impl->has_active_transaction())
    {
      local_transaction = true;
      begin_transaction();
    }
    int r = mdb_stat(m_p_impl->get_current_transaction(), static_cast<MDB_dbi>(tid), &table_stat);
    if (local_transaction)
      commit_transaction();
    CHECK_DB_CALL_RESULT(r, false, "Unable to mdb_stat");
    return table_stat.ms_entries;
  }
  
  bool lmdb_adapter::begin_transaction(bool read_only_access)
  {
    if (!read_only_access)
      m_p_impl->m_begin_commit_abort_mutex.lock(); // lock db tx sequence guard only for write-enabled transactions

    std::lock_guard<boost::recursive_mutex> guard(m_p_impl->m_transaction_stack_mutex);
    std::list<stack_entry_t>& tx_stack = m_p_impl->m_transaction_stack[std::this_thread::get_id()]; // get or create empty list

    MDB_txn* p_parent_tx = nullptr;
    MDB_txn* p_new_tx = nullptr;
    if (!tx_stack.empty())
      p_parent_tx = tx_stack.back().txn;

    tx_stack.push_back(stack_entry_t(p_new_tx, read_only_access)); // new stack entry should be added in ANY case, don't return before this line
    auto& new_stack_entry = tx_stack.back();

    unsigned int flags = read_only_access ? MDB_RDONLY : 0;
    // TODO: review the following check thorughly
    CHECK_AND_ASSERT_MES(m_p_impl != nullptr && m_p_impl->p_mdb_env != nullptr, false, "db env is null");
    int r = mdb_txn_begin(m_p_impl->p_mdb_env, p_parent_tx, flags, &p_new_tx);
    CHECK_DB_CALL_RESULT(r, false, "mdb_txn_begin");

    new_stack_entry.txn = p_new_tx; // update stack entry with correct txn

    return true;
  }

  bool lmdb_adapter::commit_transaction()
  {
    // unlock m_begin_commit_abort_mutex at the end of the function in ANY case (for write-enabled transactions)
    bool read_only_access = false; // will be set later
    auto unlocker = epee::misc_utils::create_scope_leave_handler([this, &read_only_access](){
      if (!read_only_access)
        m_p_impl->m_begin_commit_abort_mutex.unlock();
    });

    std::lock_guard<boost::recursive_mutex> guard(m_p_impl->m_transaction_stack_mutex);
    auto it = m_p_impl->m_transaction_stack.find(std::this_thread::get_id());
    // TODO: consider changing the following two checks to CHECK_AND_ASSERT_THROW_MES
    CHECK_AND_ASSERT_MES(it != m_p_impl->m_transaction_stack.end(), false, "m_transaction_stack is not found for thread id " << std::this_thread::get_id());
    CHECK_AND_ASSERT_MES(!it->second.empty(), false, "m_transaction_stack is empty for thread id " << std::this_thread::get_id());
    std::list<stack_entry_t>& tx_stack = it->second;

    MDB_txn* txn = tx_stack.back().txn;
    read_only_access = tx_stack.back().ro_access; // set actual value for unlocker

    tx_stack.pop_back();
    if (tx_stack.empty())
      m_p_impl->m_transaction_stack.erase(it);
    // tx_stack could be invalid after this point 
          
    int r = 0;
    r = mdb_txn_commit(txn);
    CHECK_DB_CALL_RESULT(r, false, "mdb_txn_commit failed");

    return true;
  }

  void lmdb_adapter::abort_transaction()
  {
    // unlock m_begin_commit_abort_mutex at the end of the function in ANY case (for write-enabled transactions)
    bool read_only_access = false; // will be set later
    auto unlocker = epee::misc_utils::create_scope_leave_handler([this, &read_only_access](){
      if (!read_only_access)
        m_p_impl->m_begin_commit_abort_mutex.unlock();
    });

    std::lock_guard<boost::recursive_mutex> guard(m_p_impl->m_transaction_stack_mutex);
    auto it = m_p_impl->m_transaction_stack.find(std::this_thread::get_id());
    // TODO: consider changing the following two checks to CHECK_AND_ASSERT_THROW_MES
    CHECK_AND_ASSERT_MES_NO_RET(it != m_p_impl->m_transaction_stack.end(), "m_transaction_stack is not found for thread id " << std::this_thread::get_id());
    CHECK_AND_ASSERT_MES_NO_RET(!it->second.empty(), "m_transaction_stack is empty for thread id " << std::this_thread::get_id());
    std::list<stack_entry_t>& tx_stack = it->second;

    MDB_txn* txn = tx_stack.back().txn;
    read_only_access = tx_stack.back().ro_access; // set actual value for unlocker

    tx_stack.pop_back();
    if (tx_stack.empty())
      m_p_impl->m_transaction_stack.erase(it);
    // tx_stack could be invalid after this point 
          
    mdb_txn_abort(txn);
  }
  
  bool lmdb_adapter::get(const table_id tid, const char* key_data, size_t key_size, std::string& out_buffer)
  {
    int r = 0;
    MDB_val key = AUTO_VAL_INIT(key);
    MDB_val data = AUTO_VAL_INIT(data);
    key.mv_data = const_cast<char*>(key_data);
    key.mv_size = key_size;

    bool local_transaction = !m_p_impl->has_active_transaction();
    if (local_transaction)
      begin_transaction(true);
    
    r = mdb_get(m_p_impl->get_current_transaction(), static_cast<MDB_dbi>(tid), &key, &data);

    if (local_transaction)
      commit_transaction();
    
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

  bool lmdb_adapter::erase(const table_id tid, const char* key_data, size_t key_size)
  {
    int r = 0;
    MDB_val key = AUTO_VAL_INIT(key);
    key.mv_data = const_cast<char*>(key_data);
    key.mv_size = key_size;

    r = mdb_del(m_p_impl->get_current_transaction(), static_cast<MDB_dbi>(tid), &key, nullptr);
    if (r == MDB_NOTFOUND)
      return false;

    CHECK_DB_CALL_RESULT(r, false, "mdb_del failed");
    return true;
  }

  bool lmdb_adapter::visit_table(const table_id tid, i_db_visitor* visitor)
  {
    CHECK_AND_ASSERT_MES(visitor != nullptr, false, "visitor is null");
    MDB_val key = AUTO_VAL_INIT(key);
    MDB_val data = AUTO_VAL_INIT(data);

    bool local_transaction = false;
    if (!m_p_impl->has_active_transaction())
    {
      local_transaction = true;
      begin_transaction();
    }
    MDB_cursor* p_cursor = nullptr;
    int r = mdb_cursor_open(m_p_impl->get_current_transaction(), static_cast<MDB_dbi>(tid), &p_cursor);
    CHECK_DB_CALL_RESULT(r, false, "mdb_cursor_open failed");
    CHECK_AND_ASSERT_MES(p_cursor != nullptr, false, "p_cursor == nullptr");

    size_t count = 0;
    for(;;)
    {
      int res = mdb_cursor_get(p_cursor, &key, &data, MDB_NEXT);
      if (res == MDB_NOTFOUND)
        break;
      if (!visitor->on_visit_db_item(count, key.mv_data, key.mv_size, data.mv_data, data.mv_size))
        break;
      ++count;
    }

    mdb_cursor_close(p_cursor);
    if (local_transaction)
      commit_transaction();
    return true;
  }



} // namespace db
