// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once



namespace view
{

  struct daemon_status_info
  {
    std::string text_state;
    uint64_t sync_progress;
    uint64_t out_connections_count;
    uint64_t inc_connections_count;
    std::string difficulty;
    uint64_t hashrate;
  };

  struct i_view
  {
     virtual bool update_daemon_status(const daemon_status_info& info)=0;
  };


  struct view_stub: public i_view
  {
    virtual bool update_daemon_status(const daemon_status_info& info){return true;}
  };
}
