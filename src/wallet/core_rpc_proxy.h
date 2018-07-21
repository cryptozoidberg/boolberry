// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#pragma once
#include "rpc/core_rpc_server_commands_defs.h"
#include "currency_core/account.h"

namespace tools
{
  /*
  wrapper to core api (rpc/direct call)
  */
  struct i_core_proxy
  {
    virtual bool set_connection_addr(const std::string& url) = 0;
    virtual bool call_COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES(const currency::COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::request& rqt, currency::COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::response& rsp) = 0;
    virtual bool call_COMMAND_RPC_GET_BLOCKS_FAST(const currency::COMMAND_RPC_GET_BLOCKS_FAST::request& rqt, currency::COMMAND_RPC_GET_BLOCKS_FAST::response& rsp) = 0;
    virtual bool call_COMMAND_RPC_GET_INFO(const currency::COMMAND_RPC_GET_INFO::request& rqt, currency::COMMAND_RPC_GET_INFO::response& rsp) = 0;
    virtual bool call_COMMAND_RPC_GET_TX_POOL(const currency::COMMAND_RPC_GET_TX_POOL::request& rqt, currency::COMMAND_RPC_GET_TX_POOL::response& rsp) = 0;
    virtual bool call_COMMAND_RPC_GET_ALIASES_BY_ADDRESS(const currency::COMMAND_RPC_GET_ALIASES_BY_ADDRESS::request& rqt, currency::COMMAND_RPC_GET_ALIASES_BY_ADDRESS::response& rsp) = 0;
    virtual bool call_COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS(const currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::request& rqt, currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::response& rsp) = 0;
    virtual bool call_COMMAND_RPC_SEND_RAW_TX(const currency::COMMAND_RPC_SEND_RAW_TX::request& rqt, currency::COMMAND_RPC_SEND_RAW_TX::response& rsp) = 0;
    virtual bool call_COMMAND_RPC_GET_ALL_ALIASES(currency::COMMAND_RPC_GET_ALL_ALIASES::response& rsp) = 0;
    virtual bool call_COMMAND_RPC_GET_ALIAS_DETAILS(const currency::COMMAND_RPC_GET_ALIAS_DETAILS::request& req, currency::COMMAND_RPC_GET_ALIAS_DETAILS::response& rsp) = 0;
    virtual bool call_COMMAND_RPC_GET_TRANSACTIONS(const currency::COMMAND_RPC_GET_TRANSACTIONS::request& req, currency::COMMAND_RPC_GET_TRANSACTIONS::response& rsp) = 0;
    virtual bool call_COMMAND_RPC_COMMAND_RPC_CHECK_KEYIMAGES(const currency::COMMAND_RPC_CHECK_KEYIMAGES::request& req, currency::COMMAND_RPC_CHECK_KEYIMAGES::response& rsp) = 0;
    virtual bool call_COMMAND_RPC_VALIDATE_SIGNED_TEXT(const currency::COMMAND_RPC_VALIDATE_SIGNED_TEXT::request& req, currency::COMMAND_RPC_VALIDATE_SIGNED_TEXT::response& rsp) = 0;
    virtual bool call_COMMAND_RPC_RELAY_TXS(const currency::COMMAND_RPC_RELAY_TXS::request& req, currency::COMMAND_RPC_RELAY_TXS::response& rsp) = 0;
    
    

    virtual bool check_connection() = 0;
    virtual bool get_transfer_address(const std::string& adr_str, currency::account_public_address& addr, currency::payment_id_t& payment_id) = 0;
  };
}

