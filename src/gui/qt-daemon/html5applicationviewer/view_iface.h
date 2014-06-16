// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <stdint.h>
#include <QObject>
#ifndef Q_MOC_RUN
#include "warnings.h"

PUSH_WARNINGS
DISABLE_VS_WARNINGS(4100)
DISABLE_VS_WARNINGS(4503)
#include "serialization/keyvalue_serialization.h"
#include "storages/portable_storage_template_helper.h"
POP_WARNINGS

#endif

namespace view
{

  /*slots structures*/
  struct transfer_destination
  {
    std::string address;
    std::string amount;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(address)
      KV_SERIALIZE(amount)
    END_KV_SERIALIZE_MAP()
  };

  struct transfer_params
  {
    std::list<transfer_destination> destinations;
    uint64_t mixin_count;
    std::string payment_id;
    std::string fee;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(destinations)
      KV_SERIALIZE(mixin_count)
      KV_SERIALIZE(payment_id)
      KV_SERIALIZE(fee)
    END_KV_SERIALIZE_MAP()
  };

  struct transfer_response
  {
    bool success;
    std::string tx_hash;
    uint64_t    tx_blob_size;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(success)
      KV_SERIALIZE(tx_hash)
      KV_SERIALIZE(tx_blob_size)
    END_KV_SERIALIZE_MAP()
  };


  /*signal structures*/
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

  struct wallet_transfer_info_details
  {
    std::list<std::string> rcv;
    std::list<std::string> spn;
  };

  struct wallet_transfer_info
  {
    std::string amount;
    uint64_t    timestamp;
    std::string tx_hash;
    uint64_t    height;
    bool        spent;
    bool        is_income;
    wallet_transfer_info_details td;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(amount)
      KV_SERIALIZE(tx_hash)
      KV_SERIALIZE(height)
      KV_SERIALIZE(spent)
      KV_SERIALIZE(is_income)
      KV_SERIALIZE(timestamp)      
      KV_SERIALIZE(td)
    END_KV_SERIALIZE_MAP()
  };

  struct wallet_info
  {
    std::string unlocked_balance;
    std::string balance;
    std::string address;
    std::string tracking_hey;
    std::string path;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(unlocked_balance)
      KV_SERIALIZE(balance)
      KV_SERIALIZE(address)
      KV_SERIALIZE(tracking_hey)
      KV_SERIALIZE(path)
    END_KV_SERIALIZE_MAP()
  };

  struct transfer_event_info
  {
    wallet_transfer_info ti;
    std::string unlocked_balance;
    std::string balance;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(ti)
      KV_SERIALIZE(unlocked_balance)
      KV_SERIALIZE(balance)
    END_KV_SERIALIZE_MAP()
  };

  struct i_view
  {
    virtual bool update_daemon_status(const daemon_status_info& info)=0;
    virtual bool on_backend_stopped()=0;
    virtual bool show_msg_box(const std::string& message)=0;
    virtual bool update_wallet_status(const wallet_status_info& wsi)=0;
    virtual bool update_wallet_info(const wallet_info& wsi)=0;
    virtual bool money_receive(const transfer_event_info& wsi)=0;
    virtual bool money_spent(const transfer_event_info& wsi)=0;
    virtual bool show_wallet()=0;
    virtual bool hide_wallet() = 0;
  };

  struct view_stub: public i_view
  {
    virtual bool update_daemon_status(const daemon_status_info& /*info*/){return true;}
    virtual bool on_backend_stopped(){return true;}
    virtual bool show_msg_box(const std::string& /*message*/){return true;}
    virtual bool update_wallet_status(const wallet_status_info& /*wsi*/){return true;}
    virtual bool update_wallet_info(const wallet_info& /*wsi*/){return true;}
    virtual bool money_receive(const transfer_event_info& /*wsi*/){return true;}
    virtual bool money_spent(const transfer_event_info& /*wsi*/){return true;}
    virtual bool show_wallet(){return true;}
    virtual bool hide_wallet(){ return true; }
  };
}
