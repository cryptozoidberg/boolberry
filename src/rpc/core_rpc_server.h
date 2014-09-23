// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma  once 

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>

#include "net/http_server_impl_base.h"
#include "core_rpc_server_commands_defs.h"
#include "currency_core/currency_core.h"
#include "p2p/net_node.h"
#include "currency_protocol/currency_protocol_handler.h"
#include "mining_protocol_defs.h"

namespace currency
{
  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  class core_rpc_server: public epee::http_server_impl_base<core_rpc_server>
  {
  public:
    typedef epee::net_utils::connection_context_base connection_context;

    core_rpc_server(core& cr, nodetool::node_server<currency::t_currency_protocol_handler<currency::core> >& p2p);

    static void init_options(boost::program_options::options_description& desc);
    bool init(const boost::program_options::variables_map& vm);

    bool on_get_height(const COMMAND_RPC_GET_HEIGHT::request& req, COMMAND_RPC_GET_HEIGHT::response& res, connection_context& cntx);
    bool on_get_blocks(const COMMAND_RPC_GET_BLOCKS_FAST::request& req, COMMAND_RPC_GET_BLOCKS_FAST::response& res, connection_context& cntx);
    bool on_get_transactions(const COMMAND_RPC_GET_TRANSACTIONS::request& req, COMMAND_RPC_GET_TRANSACTIONS::response& res, connection_context& cntx);
    bool on_get_indexes(const COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::request& req, COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::response& res, connection_context& cntx);
    bool on_send_raw_tx(const COMMAND_RPC_SEND_RAW_TX::request& req, COMMAND_RPC_SEND_RAW_TX::response& res, connection_context& cntx);
    bool on_start_mining(const COMMAND_RPC_START_MINING::request& req, COMMAND_RPC_START_MINING::response& res, connection_context& cntx);
    bool on_stop_mining(const COMMAND_RPC_STOP_MINING::request& req, COMMAND_RPC_STOP_MINING::response& res, connection_context& cntx);
    bool on_get_random_outs(const COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::request& req, COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::response& res, connection_context& cntx);
    bool on_get_info(const COMMAND_RPC_GET_INFO::request& req, COMMAND_RPC_GET_INFO::response& res, connection_context& cntx);
    bool on_set_maintainers_info(const COMMAND_RPC_SET_MAINTAINERS_INFO::request& req, COMMAND_RPC_SET_MAINTAINERS_INFO::response& res, connection_context& cntx);
    bool on_get_tx_pool(const COMMAND_RPC_GET_TX_POOL::request& req, COMMAND_RPC_GET_TX_POOL::response& res, connection_context& cntx);

    //json_rpc
    bool on_getblockcount(const COMMAND_RPC_GETBLOCKCOUNT::request& req, COMMAND_RPC_GETBLOCKCOUNT::response& res, connection_context& cntx);
    bool on_getblockhash(const COMMAND_RPC_GETBLOCKHASH::request& req, COMMAND_RPC_GETBLOCKHASH::response& res, epee::json_rpc::error& error_resp, connection_context& cntx);
    bool on_getblocktemplate(const COMMAND_RPC_GETBLOCKTEMPLATE::request& req, COMMAND_RPC_GETBLOCKTEMPLATE::response& res, epee::json_rpc::error& error_resp, connection_context& cntx);
    bool on_submitblock(const COMMAND_RPC_SUBMITBLOCK::request& req, COMMAND_RPC_SUBMITBLOCK::response& res, epee::json_rpc::error& error_resp, connection_context& cntx);
    bool on_get_last_block_header(const COMMAND_RPC_GET_LAST_BLOCK_HEADER::request& req, COMMAND_RPC_GET_LAST_BLOCK_HEADER::response& res, epee::json_rpc::error& error_resp, connection_context& cntx);
    bool on_get_block_header_by_hash(const COMMAND_RPC_GET_BLOCK_HEADER_BY_HASH::request& req, COMMAND_RPC_GET_BLOCK_HEADER_BY_HASH::response& res, epee::json_rpc::error& error_resp, connection_context& cntx);
    bool on_get_block_header_by_height(const COMMAND_RPC_GET_BLOCK_HEADER_BY_HEIGHT::request& req, COMMAND_RPC_GET_BLOCK_HEADER_BY_HEIGHT::response& res, epee::json_rpc::error& error_resp, connection_context& cntx);
    bool on_get_alias_details(const COMMAND_RPC_GET_ALIAS_DETAILS::request& req, COMMAND_RPC_GET_ALIAS_DETAILS::response& res, epee::json_rpc::error& error_resp, connection_context& cntx);
    bool on_get_all_aliases(const COMMAND_RPC_GET_ALL_ALIASES::request& req, COMMAND_RPC_GET_ALL_ALIASES::response& res, epee::json_rpc::error& error_resp, connection_context& cntx);
    bool on_alias_by_address(const COMMAND_RPC_GET_ALIASES_BY_ADDRESS::request& req, COMMAND_RPC_GET_ALIASES_BY_ADDRESS::response& res, epee::json_rpc::error& error_resp, connection_context& cntx);
    bool on_get_addendums(const COMMAND_RPC_GET_ADDENDUMS::request& req, COMMAND_RPC_GET_ADDENDUMS::response& res, epee::json_rpc::error& error_resp, connection_context& cntx);
    
    
    //mining rpc
    bool on_login(const mining::COMMAND_RPC_LOGIN::request& req, mining::COMMAND_RPC_LOGIN::response& res, connection_context& cntx);
    bool on_getjob(const mining::COMMAND_RPC_GETJOB::request& req, mining::COMMAND_RPC_GETJOB::response& res, connection_context& cntx);
    bool on_getscratchpad(const mining::COMMAND_RPC_GET_FULLSCRATCHPAD::request& req, mining::COMMAND_RPC_GET_FULLSCRATCHPAD::response& res, connection_context& cntx);
    bool on_submit(const mining::COMMAND_RPC_SUBMITSHARE::request& req, mining::COMMAND_RPC_SUBMITSHARE::response& res, connection_context& cntx);
    bool on_store_scratchpad(const mining::COMMAND_RPC_STORE_SCRATCHPAD::request& req, mining::COMMAND_RPC_STORE_SCRATCHPAD::response& res, connection_context& cntx);
    bool on_getfullscratchpad2(const epee::net_utils::http::http_request_info& query_info, epee::net_utils::http::http_response_info& response_info, connection_context& cntx);

    

  private:

    CHAIN_HTTP_TO_MAP2(connection_context); //forward http requests to uri map

    BEGIN_URI_MAP2()
      MAP_URI_AUTO_JON2("/getheight", on_get_height, COMMAND_RPC_GET_HEIGHT)
      MAP_URI_AUTO_BIN2("/getblocks.bin", on_get_blocks, COMMAND_RPC_GET_BLOCKS_FAST)
      MAP_URI_AUTO_BIN2("/get_o_indexes.bin", on_get_indexes, COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES)      
      MAP_URI_AUTO_BIN2("/getrandom_outs.bin", on_get_random_outs, COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS)
      MAP_URI_AUTO_BIN2("/set_maintainers_info.bin", on_set_maintainers_info, COMMAND_RPC_SET_MAINTAINERS_INFO)
      MAP_URI_AUTO_BIN2("/get_tx_pool.bin", on_get_tx_pool, COMMAND_RPC_GET_TX_POOL)
      MAP_URI_AUTO_JON2("/gettransactions", on_get_transactions, COMMAND_RPC_GET_TRANSACTIONS)
      MAP_URI_AUTO_JON2("/sendrawtransaction", on_send_raw_tx, COMMAND_RPC_SEND_RAW_TX)
      MAP_URI_AUTO_JON2("/start_mining", on_start_mining, COMMAND_RPC_START_MINING)
      MAP_URI_AUTO_JON2("/stop_mining", on_stop_mining, COMMAND_RPC_STOP_MINING)
      MAP_URI_AUTO_JON2("/getinfo", on_get_info, COMMAND_RPC_GET_INFO)
      MAP_URI2("/getfullscratchpad2", on_getfullscratchpad2)
      BEGIN_JSON_RPC_MAP("/json_rpc")
        MAP_JON_RPC("getblockcount",             on_getblockcount,              COMMAND_RPC_GETBLOCKCOUNT)
        MAP_JON_RPC_WE("on_getblockhash",        on_getblockhash,               COMMAND_RPC_GETBLOCKHASH)
        MAP_JON_RPC_WE("getblocktemplate",       on_getblocktemplate,           COMMAND_RPC_GETBLOCKTEMPLATE)
        MAP_JON_RPC_WE("submitblock",            on_submitblock,                COMMAND_RPC_SUBMITBLOCK)
        MAP_JON_RPC_WE("getlastblockheader",     on_get_last_block_header,      COMMAND_RPC_GET_LAST_BLOCK_HEADER)
        MAP_JON_RPC_WE("getblockheaderbyhash",   on_get_block_header_by_hash,   COMMAND_RPC_GET_BLOCK_HEADER_BY_HASH)
        MAP_JON_RPC_WE("getblockheaderbyheight", on_get_block_header_by_height, COMMAND_RPC_GET_BLOCK_HEADER_BY_HEIGHT)
        MAP_JON_RPC_WE("get_alias_details",      on_get_alias_details,          COMMAND_RPC_GET_ALIAS_DETAILS)
        MAP_JON_RPC_WE("get_all_alias_details",  on_get_all_aliases,            COMMAND_RPC_GET_ALL_ALIASES)
        MAP_JON_RPC_WE("get_alias_by_address",   on_alias_by_address,           COMMAND_RPC_GET_ALIASES_BY_ADDRESS)
        MAP_JON_RPC_WE("get_addendums",          on_get_addendums,              COMMAND_RPC_GET_ADDENDUMS)
        //remote miner rpc
        MAP_JON_RPC_N(on_login,            mining::COMMAND_RPC_LOGIN)
        MAP_JON_RPC_N(on_getjob,           mining::COMMAND_RPC_GETJOB)
        MAP_JON_RPC_N(on_getscratchpad,    mining::COMMAND_RPC_GET_FULLSCRATCHPAD)
        MAP_JON_RPC_N(on_submit,           mining::COMMAND_RPC_SUBMITSHARE)        
        MAP_JON_RPC_N(on_store_scratchpad, mining::COMMAND_RPC_STORE_SCRATCHPAD)        
      END_JSON_RPC_MAP()
    END_URI_MAP2()
  


    //-----------------------
    bool handle_command_line(const boost::program_options::variables_map& vm);
    bool check_core_ready();
    bool get_addendum_for_hi(const mining::height_info& hi, std::list<mining::addendum>& res);
    bool get_job(const std::string& job_id, mining::job_details& job, epee::json_rpc::error& err, connection_context& cntx);
    bool get_current_hi(mining::height_info& hi);

    //utils
    uint64_t get_block_reward(const block& blk);
    bool fill_block_header_responce(const block& blk, bool orphan_status, block_header_responce& responce);
    void set_session_blob(const std::string& session_id, const currency::block& blob);
    bool get_session_blob(const std::string& session_id, currency::block& blob);
    
    core& m_core;
    nodetool::node_server<currency::t_currency_protocol_handler<currency::core> >& m_p2p;
    std::string m_port;
    std::string m_bind_ip;
    //mining stuff
    epee::critical_section m_session_jobs_lock;
    std::map<std::string, currency::block> m_session_jobs; //session id -> blob
    std::atomic<size_t> m_session_counter;
  };
}
