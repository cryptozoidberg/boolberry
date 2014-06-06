// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <stdint.h>
#include <QObject>


namespace view
{

  struct daemon_status_info: public QObject
  {
    Q_OBJECT
public:
    daemon_status_info(QObject* parent = 0):QObject(parent)
      {
         sync_status = out_connections_count = inc_connections_count = hashrate = 0;
      }

    std::string text_state;
    std::string sync_status;
    uint64_t out_connections_count;
    uint64_t inc_connections_count;
    std::string difficulty;
    uint64_t hashrate;
  private:
    Q_DISABLE_COPY(daemon_status_info)

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
