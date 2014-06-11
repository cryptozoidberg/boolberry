// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <stdint.h>
#include <QObject>
#ifndef Q_MOC_RUN
#include "serialization/keyvalue_serialization.h"
#include "storages/portable_storage_template_helper.h"
#endif

namespace view
{
  struct daemon_status_info
  {
public:

    std::string text_state;
    uint64_t daemon_network_state;
    uint64_t synchronization_start_height;
    uint64_t max_net_seen_height;
    uint64_t height;
    uint64_t out_connections_count;
    uint64_t inc_connections_count;
    std::string difficulty;
    uint64_t hashrate;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(text_state)
      KV_SERIALIZE(daemon_network_state)
      KV_SERIALIZE(synchronization_start_height)
      KV_SERIALIZE(max_net_seen_height)
      KV_SERIALIZE(height)
      KV_SERIALIZE(out_connections_count)
      KV_SERIALIZE(inc_connections_count)
      KV_SERIALIZE(difficulty)
      KV_SERIALIZE(hashrate)
    END_KV_SERIALIZE_MAP()
  };




  struct wallet_status_info
  {
    enum state
    {
      wallet_state_synchronizing = 1,
      wallet_state_ready = 2
    };


    uint64_t wallet_state;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(wallet_state)
    END_KV_SERIALIZE_MAP()
  };

  struct wallet_transfer_info
  {
    uint64_t    m_amount;
    std::string tx_hash;
    uint64_t    m_height;
    bool        m_spent;
    bool        m_is_income;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(m_amount)
      KV_SERIALIZE(tx_hash)
      KV_SERIALIZE(m_height)
      KV_SERIALIZE(m_spent)
      KV_SERIALIZE(m_is_income)
    END_KV_SERIALIZE_MAP()
  };

  struct wallet_info
  {
    uint64_t unlocked_balance;
    uint64_t balance;
    std::list<wallet_transfer_info> m_transfers;
    std::string m_address;
    std::string m_tracking_hey;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(unlocked_balance)
      KV_SERIALIZE(balance)
      KV_SERIALIZE(m_transfers)
      KV_SERIALIZE(m_address)
      KV_SERIALIZE(m_tracking_hey)
    END_KV_SERIALIZE_MAP()
  };


  struct i_view
  {
    virtual bool update_daemon_status(const daemon_status_info& info)=0;
    virtual bool on_backend_stopped()=0;
    virtual bool show_msg_box(const std::string& message)=0;
    virtual bool update_wallet_status(const wallet_status_info& wsi)=0;
    virtual bool update_wallet_info(const wallet_info& wsi)=0;
  };

  struct view_stub: public i_view
  {
    virtual bool update_daemon_status(const daemon_status_info& /*info*/){return true;}
    virtual bool on_backend_stopped(){return true;}
    virtual bool update_wallet_status(const wallet_status_info& wsi){return true;}
    virtual bool update_wallet_info(const wallet_info& wsi){return true;}
  };
}
