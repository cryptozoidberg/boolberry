// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "core_default_rpc_proxy.h"
using namespace epee;
#include "storages/http_abstract_invoke.h"
#include "currency_core/currency_format_utils.h"
#include "currency_core/alias_helper.h"

namespace tools
{
  bool default_http_core_proxy::set_connection_addr(const std::string& url)
  {
    m_daemon_address = url;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool default_http_core_proxy::call_COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES(const currency::COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::request& req, currency::COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::response& res)
  {
    return net_utils::invoke_http_bin_remote_command2(m_daemon_address + "/get_o_indexes.bin", req, res, m_http_client, WALLET_RCP_CONNECTION_TIMEOUT);
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool default_http_core_proxy::call_COMMAND_RPC_GET_BLOCKS_FAST(const currency::COMMAND_RPC_GET_BLOCKS_FAST::request& req, currency::COMMAND_RPC_GET_BLOCKS_FAST::response& res)
  {
    return net_utils::invoke_http_bin_remote_command2(m_daemon_address + "/getblocks.bin", req, res, m_http_client, WALLET_RCP_CONNECTION_TIMEOUT);
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool default_http_core_proxy::call_COMMAND_RPC_GET_INFO(const currency::COMMAND_RPC_GET_INFO::request& req, currency::COMMAND_RPC_GET_INFO::response& res)
  {
    return net_utils::invoke_http_json_remote_command2(m_daemon_address + "/getinfo", req, res, m_http_client, WALLET_RCP_CONNECTION_TIMEOUT);
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool default_http_core_proxy::call_COMMAND_RPC_GET_TX_POOL(const currency::COMMAND_RPC_GET_TX_POOL::request& req, currency::COMMAND_RPC_GET_TX_POOL::response& res)
  {
    return net_utils::invoke_http_bin_remote_command2(m_daemon_address + "/get_tx_pool.bin", req, res, m_http_client, WALLET_RCP_CONNECTION_TIMEOUT);
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool default_http_core_proxy::call_COMMAND_RPC_GET_ALIASES_BY_ADDRESS(const currency::COMMAND_RPC_GET_ALIASES_BY_ADDRESS::request& req, currency::COMMAND_RPC_GET_ALIASES_BY_ADDRESS::response& res)
  {
    return epee::net_utils::invoke_http_json_rpc("/json_rpc", "get_alias_by_address", req, res, m_http_client); 
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool default_http_core_proxy::call_COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS(const currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::request& req, currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::response& res)
  {
    return epee::net_utils::invoke_http_bin_remote_command2(m_daemon_address + "/getrandom_outs.bin", req, res, m_http_client, WALLET_RCP_CONNECTION_TIMEOUT);
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool default_http_core_proxy::call_COMMAND_RPC_SEND_RAW_TX(const currency::COMMAND_RPC_SEND_RAW_TX::request& req, currency::COMMAND_RPC_SEND_RAW_TX::response& res)
  {
    return epee::net_utils::invoke_http_json_remote_command2(m_daemon_address + "/sendrawtransaction", req, res, m_http_client, WALLET_RCP_CONNECTION_TIMEOUT);
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool default_http_core_proxy::call_COMMAND_RPC_GET_TRANSACTIONS(const currency::COMMAND_RPC_GET_TRANSACTIONS::request& req, currency::COMMAND_RPC_GET_TRANSACTIONS::response& rsp)
  {
    return epee::net_utils::invoke_http_json_remote_command2(m_daemon_address + "/gettransactions", req, rsp, m_http_client, WALLET_RCP_CONNECTION_TIMEOUT);
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool default_http_core_proxy::call_COMMAND_RPC_COMMAND_RPC_CHECK_KEYIMAGES(const currency::COMMAND_RPC_CHECK_KEYIMAGES::request& req, currency::COMMAND_RPC_CHECK_KEYIMAGES::response& rsp)
  {
    return epee::net_utils::invoke_http_bin_remote_command2(m_daemon_address + "/check_keyimages.bin", req, rsp, m_http_client, WALLET_RCP_CONNECTION_TIMEOUT);
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool default_http_core_proxy::call_COMMAND_RPC_RELAY_TXS(const currency::COMMAND_RPC_RELAY_TXS::request& req, currency::COMMAND_RPC_RELAY_TXS::response& rsp)
  {
    return epee::net_utils::invoke_http_json_rpc("/json_rpc", "relay_txs", req, rsp, m_http_client);
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool default_http_core_proxy::check_connection()
  {
    if (m_http_client.is_connected())
      return true;

    epee::net_utils::http::url_content u;
    epee::net_utils::parse_url(m_daemon_address, u);
    if (!u.port)
      u.port = 8081;
    return m_http_client.connect(u.host, std::to_string(u.port), WALLET_RCP_CONNECTION_TIMEOUT);
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool default_http_core_proxy::call_COMMAND_RPC_GET_ALL_ALIASES(currency::COMMAND_RPC_GET_ALL_ALIASES::response& res)
  {
    currency::COMMAND_RPC_GET_ALL_ALIASES::request req = AUTO_VAL_INIT(req);
    return epee::net_utils::invoke_http_json_rpc("/json_rpc", "get_all_alias_details", req, res, m_http_client);
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool default_http_core_proxy::call_COMMAND_RPC_VALIDATE_SIGNED_TEXT(const currency::COMMAND_RPC_VALIDATE_SIGNED_TEXT::request& req, currency::COMMAND_RPC_VALIDATE_SIGNED_TEXT::response& rsp)
  {
    return epee::net_utils::invoke_http_json_rpc("/json_rpc", "validate_signed_text", req, rsp, m_http_client);
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool default_http_core_proxy::call_COMMAND_RPC_GET_ALIAS_DETAILS(const currency::COMMAND_RPC_GET_ALIAS_DETAILS::request& req, currency::COMMAND_RPC_GET_ALIAS_DETAILS::response& res)
  {
    return epee::net_utils::invoke_http_json_rpc("/json_rpc", "get_alias_details", req, res, m_http_client);
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool default_http_core_proxy::get_transfer_address(const std::string& adr_str, currency::account_public_address& addr, currency::payment_id_t& payment_id)
  {
    return tools::get_transfer_address(adr_str, addr, payment_id, this);
  }

}

