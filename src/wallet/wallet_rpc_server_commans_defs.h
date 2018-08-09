// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "currency_protocol/currency_protocol_defs.h"
#include "currency_core/currency_basic.h"
#include "crypto/hash.h"
#include "wallet_rpc_server_error_codes.h"
namespace tools
{
namespace wallet_rpc
{
#define WALLET_RPC_STATUS_OK      "OK"
#define WALLET_RPC_STATUS_BUSY    "BUSY"


  struct wallet_transfer_info_details
  {
    std::list<uint64_t> rcv;
    std::list<uint64_t> spn;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(rcv)
      KV_SERIALIZE(spn)
    END_KV_SERIALIZE_MAP()

  };

  struct wallet_transfer_info
  {
    uint64_t      amount;
    uint64_t      timestamp;
    std::string   tx_hash;
    uint64_t      height;          //if height == 0 than tx is unconfirmed
    uint64_t      unlock_time;
    uint32_t      tx_blob_size;
    std::string   payment_id;
    std::string   destinations;      //optional
    std::string   destination_alias; //optional
    bool          is_income;
    uint64_t      fee;
    wallet_transfer_info_details td;
    
    //not included in serialization map
    currency::transaction tx;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(amount)
      KV_SERIALIZE(tx_hash)
      KV_SERIALIZE(height)
      KV_SERIALIZE(unlock_time)
      KV_SERIALIZE(tx_blob_size)
      KV_SERIALIZE(payment_id)
      KV_SERIALIZE(destinations)      
      KV_SERIALIZE(destination_alias)
      KV_SERIALIZE(is_income)
      KV_SERIALIZE(timestamp)
      KV_SERIALIZE(td)
      KV_SERIALIZE(fee)
    END_KV_SERIALIZE_MAP()
  };


  struct COMMAND_RPC_GET_BALANCE
  {
    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      uint64_t    balance;
      uint64_t    unlocked_balance;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(balance)
        KV_SERIALIZE(unlocked_balance)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_GET_ADDRESS
  {
    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string   address;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(address)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct trnsfer_destination
  {
    uint64_t amount;
    std::string address;
    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(amount)
      KV_SERIALIZE(address)
    END_KV_SERIALIZE_MAP()
  };

  struct COMMAND_RPC_TRANSFER
  {
    struct request
    {
      std::list<trnsfer_destination> destinations;
      uint64_t fee;
      uint64_t mixin;
      uint64_t unlock_time;
      std::string payment_id_hex;
      bool do_not_relay;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(destinations)
        KV_SERIALIZE(fee)
        KV_SERIALIZE(mixin)
        KV_SERIALIZE(unlock_time)
        KV_SERIALIZE(do_not_relay)
        KV_SERIALIZE(payment_id_hex)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string tx_hash;
      std::string tx_blob;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tx_hash)
        KV_SERIALIZE(tx_blob)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_SUBMIT
  {
    struct request
    {
      std::string tx_blob;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(destinations)
        KV_SERIALIZE(fee)
        KV_SERIALIZE(mixin)
        KV_SERIALIZE(unlock_time)
        KV_SERIALIZE(do_not_relay)
        KV_SERIALIZE(payment_id_hex)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string tx_hash;
      std::string tx_blob;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tx_hash)
        KV_SERIALIZE(tx_blob)
      END_KV_SERIALIZE_MAP()
    };
  };
  struct COMMAND_RPC_STORE
  {
    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
  };

  struct payment_details
  {
    std::string payment_id;
    std::string tx_hash;
    uint64_t amount;
    uint64_t block_height;
    uint64_t unlock_time;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(payment_id)      
      KV_SERIALIZE(tx_hash)
      KV_SERIALIZE(amount)
      KV_SERIALIZE(block_height)
      KV_SERIALIZE(unlock_time)
    END_KV_SERIALIZE_MAP()
  };

  
  
  struct COMMAND_RPC_GET_PAYMENTS
  {
    struct request
    {
      std::string payment_id;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(payment_id)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::list<payment_details> payments;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(payments)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_GET_BULK_PAYMENTS
  {
    struct request
    {
      std::vector<std::string> payment_ids;
      uint64_t min_block_height;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(payment_ids)
        KV_SERIALIZE(min_block_height)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::list<payment_details> payments;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(payments)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_GET_TRANSFERS
  {
    struct request
    {
      bool in;
      bool out;
      bool pending;
      bool failed;
      bool pool;
      bool filter_by_height;
      uint64_t min_height;
      uint64_t max_height;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(in)
        KV_SERIALIZE(out)
        KV_SERIALIZE(pending)
        KV_SERIALIZE(failed)
        KV_SERIALIZE(pool)
        KV_SERIALIZE(filter_by_height)
        KV_SERIALIZE(min_height)
        KV_SERIALIZE(max_height)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::list<wallet_transfer_info> in;
      std::list<wallet_transfer_info> out;
      std::list<wallet_transfer_info> pending;
      std::list<wallet_transfer_info> failed;
      std::list<wallet_transfer_info> pool;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(in)
        KV_SERIALIZE(out)
        KV_SERIALIZE(pending)
        KV_SERIALIZE(failed)
        KV_SERIALIZE(pool)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_CONVERT_ADDRESS
  {
    struct request
    {
      std::string address_str;
      std::string payment_id_hex;
      std::string integrated_address_str;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(address_str)
        KV_SERIALIZE(payment_id_hex)
        KV_SERIALIZE(integrated_address_str)
      END_KV_SERIALIZE_MAP()
    };

    struct response : public request
    {
    };
  };


  /*stay-alone instance*/
  struct telepod
  {
    std::string account_keys_hex;
    std::string basement_tx_id_hex;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(account_keys_hex)
      KV_SERIALIZE(basement_tx_id_hex)
    END_KV_SERIALIZE_MAP()
  };

  struct COMMAND_RPC_MAKETELEPOD
  {
    struct request
    {
      uint64_t amount;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(amount)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status; //"OK", "INSUFFICIENT_COINS", "INTERNAL_ERROR"
      telepod tpd;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(tpd)
      END_KV_SERIALIZE_MAP()
    };
  };


  struct COMMAND_RPC_TELEPODSTATUS
  {
    struct request
    {
      telepod tpd;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tpd)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;  //"OK", "UNCONFIRMED", "BAD", "SPENT", "INTERNAL_ERROR"

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
     END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_CLONETELEPOD
  {
    struct request
    {
      telepod tpd;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tpd)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;//"OK", "UNCONFIRMED", "BAD", "SPENT", "INTERNAL_ERROR:"
      telepod tpd;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(tpd)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_WITHDRAWTELEPOD
  {
    struct request
    {
      telepod tpd;
      std::string addr;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tpd)
        KV_SERIALIZE(addr)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;  //"OK", "UNCONFIRMED", "BAD", "SPENT", "INTERNAL_ERROR", "BAD_ADDRESS"
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };


}
}

