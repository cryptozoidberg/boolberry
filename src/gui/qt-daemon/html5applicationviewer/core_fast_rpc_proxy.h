// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "rpc/core_rpc_server.h"
#include "wallet/core_rpc_proxy.h"
#include "currency_core/alias_helper.h"
namespace tools
{
  class core_fast_rpc_proxy: public i_core_proxy
  {
  public:
    core_fast_rpc_proxy(currency::core_rpc_server& rpc_srv) :m_rpc(rpc_srv)
    {}
    //------------------------------------------------------------------------------------------------------------------------------
    bool set_connection_addr(const std::string& url)
    {
      return true;
    }
    //------------------------------------------------------------------------------------------------------------------------------
    bool call_COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES(const currency::COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::request& req, currency::COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::response& res)
    {
      return m_rpc.on_get_indexes(req, res, m_cntxt_stub);
    }
    //------------------------------------------------------------------------------------------------------------------------------
    bool call_COMMAND_RPC_GET_BLOCKS_FAST(const currency::COMMAND_RPC_GET_BLOCKS_FAST::request& req, currency::COMMAND_RPC_GET_BLOCKS_FAST::response& res)
    {
      return m_rpc.on_get_blocks(req, res, m_cntxt_stub);
    }
    //------------------------------------------------------------------------------------------------------------------------------
    bool call_COMMAND_RPC_GET_INFO(const currency::COMMAND_RPC_GET_INFO::request& req, currency::COMMAND_RPC_GET_INFO::response& res)
    {
      return m_rpc.on_get_info(req, res, m_cntxt_stub);
    }
    //------------------------------------------------------------------------------------------------------------------------------
    bool call_COMMAND_RPC_GET_TX_POOL(const currency::COMMAND_RPC_GET_TX_POOL::request& req, currency::COMMAND_RPC_GET_TX_POOL::response& res)
    {
      return m_rpc.on_get_tx_pool(req, res, m_cntxt_stub);
    }
    //------------------------------------------------------------------------------------------------------------------------------
    bool call_COMMAND_RPC_GET_ALIASES_BY_ADDRESS(const currency::COMMAND_RPC_GET_ALIASES_BY_ADDRESS::request& req, currency::COMMAND_RPC_GET_ALIASES_BY_ADDRESS::response& res)
    {
      return m_rpc.on_alias_by_address(req, res, m_err_stub, m_cntxt_stub);
    }
    //------------------------------------------------------------------------------------------------------------------------------
    bool call_COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS(const currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::request& req, currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::response& res)
    {
      return m_rpc.on_get_random_outs(req, res, m_cntxt_stub);
    }
    //------------------------------------------------------------------------------------------------------------------------------
    bool call_COMMAND_RPC_SEND_RAW_TX(const currency::COMMAND_RPC_SEND_RAW_TX::request& req, currency::COMMAND_RPC_SEND_RAW_TX::response& res)
    {
      return m_rpc.on_send_raw_tx(req, res, m_cntxt_stub);
    }
    //------------------------------------------------------------------------------------------------------------------------------
    bool check_connection()
    {
      return true;
    }
    //------------------------------------------------------------------------------------------------------------------------------
    bool call_COMMAND_RPC_GET_ALL_ALIASES(currency::COMMAND_RPC_GET_ALL_ALIASES::response& res)
    {
      currency::COMMAND_RPC_GET_ALL_ALIASES::request req = AUTO_VAL_INIT(req);
      return m_rpc.on_get_all_aliases(req, res, m_err_stub, m_cntxt_stub);
    }
    //------------------------------------------------------------------------------------------------------------------------------
    bool call_COMMAND_RPC_GET_ALIAS_DETAILS(const currency::COMMAND_RPC_GET_ALIAS_DETAILS::request& req, currency::COMMAND_RPC_GET_ALIAS_DETAILS::response& res)
    {
      return m_rpc.on_get_alias_details(req, res, m_err_stub, m_cntxt_stub);
    }
    //------------------------------------------------------------------------------------------------------------------------------
    bool get_transfer_address(const std::string& adr_str, currency::account_public_address& addr)
    {
      return tools::get_transfer_address(adr_str, addr, this);
    }


/*    bool get_info(const currency::COMMAND_RPC_GET_INFO::response& info)
    {
      currency::core_rpc_server::connection_context stub_cntxt = AUTO_VAL_INIT(stub_cntxt);
      currency::COMMAND_RPC_GET_INFO::request res = AUTO_VAL_INIT(res);

      return m_rpc.on_get_info(res, info, stub_cntxt);
    }

    bool get_aliases(const currency::COMMAND_RPC_GET_ALL_ALIASES::response& aliases)
    {
      currency::core_rpc_server::connection_context stub_cntxt = AUTO_VAL_INIT(stub_cntxt);
      currency::COMMAND_RPC_GET_ALL_ALIASES::request res = AUTO_VAL_INIT(res);
      epee::json_rpc::error error_resp;

      return m_rpc.on_get_all_aliases(res, aliases, error_resp, stub_cntxt);
    }

    bool get_alias_info(const std::string& alias, currency::COMMAND_RPC_GET_ALIAS_DETAILS::response& alias_info)
    {
      currency::core_rpc_server::connection_context stub_cntxt = AUTO_VAL_INIT(stub_cntxt);
      currency::COMMAND_RPC_GET_ALIAS_DETAILS::request req = AUTO_VAL_INIT(req);
      req.alias = alias;
      epee::json_rpc::error error_resp;

      return m_rpc.on_get_alias_details(req, alias_info, error_resp, stub_cntxt);
    }
    */

  private:
    currency::core_rpc_server& m_rpc;
    currency::core_rpc_server::connection_context m_cntxt_stub;
    epee::json_rpc::error m_err_stub;
  };

}



