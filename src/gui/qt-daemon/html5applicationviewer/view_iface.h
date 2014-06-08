// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <stdint.h>
#include <QObject>
#include "serialization/keyvalue_serialization.h"

namespace view
{
  struct daemon_status_info
  {
public:

    std::string text_state;
    std::string sync_status;
    uint64_t out_connections_count;
    uint64_t inc_connections_count;
    std::string difficulty;
    uint64_t hashrate;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(text_state)
      KV_SERIALIZE(sync_status)
      KV_SERIALIZE(out_connections_count)
      KV_SERIALIZE(inc_connections_count)
      KV_SERIALIZE(difficulty)
      KV_SERIALIZE(hashrate)
    END_KV_SERIALIZE_MAP()
  };



  struct i_view
  {
     virtual bool update_daemon_status(const daemon_status_info& info)=0;
  };

  struct view_stub: public i_view
  {
    virtual bool update_daemon_status(const daemon_status_info& /*info*/){return true;}
  };
}
