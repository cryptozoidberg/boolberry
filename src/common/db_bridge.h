// Copyright (c) 2012-2018 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once

#include "misc_language.h"
#include "misc_log_ex.h"

namespace db
{
  typedef uint64_t table_id;

  class i_db_visitor
  {
  public:
    // visitor callback, would be called for each item in a table
    // return value: false to stop the process, true - to continue
    virtual bool on_visit_db_item(size_t i, const void* key_data, size_t key_size, const void* value_data, size_t value_size) = 0;
  };

  // interface for database implementation
  class i_db_adapter
  {
  public:
    virtual bool open(const std::string& db_name) = 0;
    virtual bool open_table(const std::string& table_name, table_id &tid) = 0;
    virtual bool clear_table(const table_id tid) = 0;
    virtual size_t get_table_size(const table_id tid) = 0;
    virtual bool close() = 0;

    virtual bool begin_transaction() = 0;
    virtual bool commit_transaction() = 0;
    virtual void abort_transaction() = 0;

    virtual bool get(const table_id tid, const char* key_data, size_t key_size, std::string& out_buffer) = 0;
    virtual bool set(const table_id tid, const char* key_data, size_t key_size, const char* value_data, size_t value_size) = 0;
    virtual bool erase(const table_id tid, const char* key_data, size_t key_size) = 0;

    virtual bool visit_table(const table_id tid, i_db_visitor* visitor) = 0;
    
    virtual ~i_db_adapter()
    {};
  };

  // POD table keys accessors
  template<class tkey_pod_t>
  const char* tkey_to_pointer(const tkey_pod_t& tkey, size_t& len_out) 
  {
    static_assert(std::is_pod<t_pod_key>::value, "pod type expected");
    len_out = sizeof(tkey);
    return reinterpret_cast<const char*>(&tkey);
  }

  template<class tkey_pod_t>
  void tkey_from_pointer(tkey_pod_t& tkey_out, const void* pointer, const uint64_t len)
  {
    static_assert(std::is_pod<t_pod_key>::value, "pod type expected");
    CHECK_AND_ASSERT_THROW_MES(sizeof(t_pod_key) == len, "wrong size");
    CHECK_AND_ASSERT_THROW_MES(pointer != nullptr, "pointer is null");
    tkey_out = *static_cast<t_pod_key*>(pointer);
  }

  
  class db_bridge_base
  {
  public:
    explicit db_bridge_base(std::shared_ptr<i_db_adapter> adapter_ptr)
      : m_db_adapter_ptr(adapter_ptr)
      , m_db_opened(false)
    {}

    ~db_bridge_base()
    {
    }

    void begin_db_transaction()
    {
      // TODO
    }

    void commit_db_transaction()
    {
      // TODO
    }

    void abort_db_transaction()
    {
      // TODO
    }

    bool is_open() const
    {
      return m_db_opened;
    }

    std::shared_ptr<i_db_adapter> get_adapter() const
    {
      return m_db_adapter_ptr;
    }

    bool open(const std::string& db_name)
    {
      m_db_opened = m_db_adapter_ptr->open(db_name);
      return m_db_opened;
    }

    bool close()
    {
      m_db_opened = false;
      return m_db_adapter_ptr->close();
    }

    bool clear(const table_id tid)
    {
      return m_db_adapter_ptr->clear_table(tid);
    }

    uint64_t size(const table_id tid) const
    {
      return m_db_adapter_ptr->get_table_size(tid);
    }

    template<class tkey_pod_t>
    bool erase(table_id tid, const tkey_pod_t& tkey)
    {
      // todo
      return true;
    }

    protected:
      std::shared_ptr<i_db_adapter> m_db_adapter_ptr;
      bool m_db_opened;
  };
    

} // namespace db
