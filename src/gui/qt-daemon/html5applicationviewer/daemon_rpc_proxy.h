// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "rpc/core_rpc_server.h"

namespace tools
{
  class daemon_rpc_proxy_fast
  {
  public:
    daemon_rpc_proxy_fast(currency::core_rpc_server& rpc_srv):m_rpc(rpc_srv)
    {}

    bool get_info(currency::COMMAND_RPC_GET_INFO::response& info)
    {
      currency::core_rpc_server::connection_context stub_cntxt = AUTO_VAL_INIT(stub_cntxt);
      currency::COMMAND_RPC_GET_INFO::request res = AUTO_VAL_INIT(res);

      return m_rpc.on_get_info(res, info, stub_cntxt);
    }

    bool get_aliases(currency::COMMAND_RPC_GET_ALL_ALIASES::response& aliases)
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


  private:
    currency::core_rpc_server & m_rpc;
  };

}



