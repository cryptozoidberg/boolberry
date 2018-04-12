// Copyright (c) 2012-2018 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once

#include "misc_language.h"
#include "misc_log_ex.h"
#include "currency_core/currency_format_utils.h"

namespace db
{
  typedef uint64_t table_id;
  constexpr bool tx_read_write = false;
  constexpr bool tx_read_only = true;

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

    virtual bool begin_transaction(bool read_only_access = false) = 0;
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
    static_assert(std::is_pod<tkey_pod_t>::value, "pod type expected");
    len_out = sizeof tkey;
    return reinterpret_cast<const char*>(&tkey);
  }

  template<class tkey_pod_t>
  void tkey_from_pointer(tkey_pod_t& tkey_out, const void* pointer, const uint64_t len)
  {
    static_assert(std::is_pod<tkey_pod_t>::value, "pod type expected");
    CHECK_AND_ASSERT_THROW_MES(sizeof(tkey_pod_t) == len, "wrong size");
    CHECK_AND_ASSERT_THROW_MES(pointer != nullptr, "pointer is null");
    tkey_out = *static_cast<tkey_pod_t*>(pointer);
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
      close();
    }

    bool begin_db_transaction(bool read_only_access = false)
    {
      // TODO     !!!
      bool r = m_db_adapter_ptr->begin_transaction(read_only_access);
      return r;
    }

    void commit_db_transaction()
    {
      // TODO    !!!
      bool r = m_db_adapter_ptr->commit_transaction();
      CHECK_AND_ASSERT_THROW_MES(r, "commit_transaction failed");
    }

    void abort_db_transaction()
    {
      // TODO     !!!
      m_db_adapter_ptr->abort_transaction();
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
      size_t key_size = 0;
      const char* key_data = tkey_to_pointer(tkey, key_size);
      return m_db_adapter_ptr->erase(tid, key_data, key_size);
    }

    template<class tkey_pod_t, class t_object>
    bool get_serializable_object(const table_id tid, const tkey_pod_t& tkey, t_object& obj) const
    {
      std::string buffer;
      size_t key_size = 0;
      const char* key_data = tkey_to_pointer(tkey, key_size);

      if (!m_db_adapter_ptr->get(tid, key_data, key_size, buffer))
        return false;

      return currency::t_unserializable_object_from_blob(obj, buffer);
    }

    template<class tkey_pod_t, class t_object>
    bool set_serializable_object(const table_id tid, const tkey_pod_t& tkey, const t_object& obj)
    {
      std::string buffer;
      currency::t_serializable_object_to_blob(obj, buffer);

      size_t key_size = 0;
      const char* key_data = tkey_to_pointer(tkey, key_size);
      return m_db_adapter_ptr->set(tid, key_data, key_size, buffer.data(), buffer.size());
    }

    template<class tkey_pod_t, class t_object_pod_t>
    bool get_pod_object(const table_id tid, const tkey_pod_t& tkey, t_object_pod_t& obj) const
    {
      static_assert(std::is_pod<t_object_pod_t>::value, "POD type expected");

      std::string buffer;
      size_t key_size = 0;
      const char* key_data = tkey_to_pointer(tkey, key_size);

      if (!m_db_adapter_ptr->get(tid, key_data, key_size, buffer))
        return false;

      CHECK_AND_ASSERT_MES(sizeof(t_object_pod_t) == buffer.size(), false,
        "DB returned object with size " << buffer.size() << " bytes, while " << sizeof(t_object_pod_t) << " bytes object is expected");

      obj = *reinterpret_cast<const t_object_pod_t*>(buffer.data());
      return true;
    }

    template<class tkey_pod_t, class t_object_pod_t>
    bool set_pod_object(const table_id tid, const tkey_pod_t& tkey, const t_object_pod_t& obj)
    {
      static_assert(std::is_pod<t_object_pod_t>::value, "POD type expected");

      size_t key_size = 0;
      const char* key_data = tkey_to_pointer(tkey, key_size);

      if (!m_db_adapter_ptr->set(tid, key_data, key_size, reinterpret_cast<const char*>(&obj), sizeof obj))
        return false;

      return true;
    }


    protected:
      std::shared_ptr<i_db_adapter> m_db_adapter_ptr;
      bool m_db_opened;
  };
    

} // namespace db
