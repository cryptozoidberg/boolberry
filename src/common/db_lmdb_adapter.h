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
    virtual bool open(const std::string& db_name);
    virtual bool open_table(const std::string& table_name, const table_id h);
    virtual bool clear_table(const table_id tid);
    virtual uint64_t get_table_size(const table_id tid);
    virtual bool close();
    virtual bool begin_transaction();
    virtual bool commit_transaction();
    virtual void abort_transaction();

  private:
    lmdb_adapter_impl* m_p_impl;

    std::string m_db_folder;

  };

} // namespace db
