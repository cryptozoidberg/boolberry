// Copyright (c) 2012-2018 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once
#include "db_bridge.h"

namespace db
{
  struct lmdb_adapter_impl;

  class lmdb_adapter : public i_db_adapter
  {
  public:
    explicit lmdb_adapter();
    virtual ~lmdb_adapter();

    // interface i_db_adapter
    virtual bool open(const std::string& db_name) override;
    virtual bool open_table(const std::string& table_name, table_id &tid) override;
    virtual bool clear_table(const table_id tid) override;
    virtual size_t get_table_size(const table_id tid) override;
    virtual bool close() override;
    virtual bool begin_transaction(bool read_only_access = false) override;
    virtual bool commit_transaction() override;
    virtual void abort_transaction() override;
    virtual bool get(const table_id tid, const char* key_data, size_t key_size, std::string& out_buffer) override;
    virtual bool set(const table_id tid, const char* key_data, size_t key_size, const char* value_data, size_t value_size) override;
    virtual bool erase(const table_id tid, const char* key_data, size_t key_size) override;
    virtual bool visit_table(const table_id tid, i_db_visitor* visitor) override;

  private:
    lmdb_adapter_impl* m_p_impl;

    std::string m_db_folder;

  };

} // namespace db
