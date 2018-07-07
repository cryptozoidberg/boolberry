// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include <boost/foreach.hpp>
#include "include_base_utils.h"
#include <boost/serialization/variant.hpp>
using namespace epee;

#include "core_rpc_server.h"
#include "common/command_line.h"
#include "currency_core/currency_format_utils.h"
#include "currency_core/account.h"
#include "currency_core/currency_basic_impl.h"
#include "misc_language.h"
#include "crypto/hash.h"
#include "core_rpc_server_error_codes.h"
#include "currency_core/alias_helper.h"

namespace currency
{
  namespace
  {
    const command_line::arg_descriptor<std::string> arg_rpc_bind_ip   = {"rpc-bind-ip", "IP for RPC Server", "127.0.0.1"};
    const command_line::arg_descriptor<std::string> arg_rpc_bind_port = {"rpc-bind-port", "Port for RPC Server", std::to_string(RPC_DEFAULT_PORT)};
	const command_line::arg_descriptor<bool> arg_rpc_restricted_rpc = { "restricted-rpc", "Restrict RPC to view only commands", false};
  }
  //-----------------------------------------------------------------------------------
  void core_rpc_server::init_options(boost::program_options::options_description& desc)
  {
    command_line::add_arg(desc, arg_rpc_bind_ip);
    command_line::add_arg(desc, arg_rpc_bind_port);
	command_line::add_arg(desc, arg_rpc_restricted_rpc);
  }
  //------------------------------------------------------------------------------------------------------------------------------
  core_rpc_server::core_rpc_server(core& cr, nodetool::node_server<currency::t_currency_protocol_handler<currency::core> >& p2p):m_core(cr), m_p2p(p2p), m_session_counter(0)
  {}
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::handle_command_line(const boost::program_options::variables_map& vm)
  {
    m_bind_ip = command_line::get_arg(vm, arg_rpc_bind_ip);
    m_port = command_line::get_arg(vm, arg_rpc_bind_port);
	m_restricted = command_line::get_arg(vm, arg_rpc_restricted_rpc);
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::init(const boost::program_options::variables_map& vm)
  {
    m_net_server.set_threads_prefix("RPC");
    bool r = handle_command_line(vm);
    CHECK_AND_ASSERT_MES(r, false, "Failed to process command line in core_rpc_server");
    return epee::http_server_impl_base<core_rpc_server, connection_context>::init(m_port, m_bind_ip);
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::check_core_ready()
  {
#ifndef TESTNET
    if(!m_p2p.get_payload_object().is_synchronized())
    {
      return false;
    }
#endif
    if(m_p2p.get_payload_object().get_core().get_blockchain_storage().is_storing_blockchain())
    {
      return false;
    }
    return true;
  }
#define CHECK_CORE_READY() if(!check_core_ready()){res.status =  CORE_RPC_STATUS_BUSY;return true;}

  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_height(const COMMAND_RPC_GET_HEIGHT::request& req, COMMAND_RPC_GET_HEIGHT::response& res, connection_context& cntx)
  {
    CHECK_CORE_READY();
    res.height = m_core.get_current_blockchain_height();
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_info(const COMMAND_RPC_GET_INFO::request& req, COMMAND_RPC_GET_INFO::response& res, connection_context& cntx)
  {
    if (m_p2p.get_payload_object().get_core().get_blockchain_storage().is_storing_blockchain())
    {
      res.status = CORE_RPC_STATUS_BUSY; 
      return true; 
    }

    res.height = m_core.get_current_blockchain_height();
    res.difficulty = m_core.get_blockchain_storage().get_difficulty_for_next_block().convert_to<uint64_t>();
    res.tx_count = m_core.get_blockchain_storage().get_total_transactions() - res.height; //without coinbase
    res.tx_pool_size = m_core.get_pool_transactions_count();
    res.alt_blocks_count = m_core.get_blockchain_storage().get_alternative_blocks_count();
    uint64_t total_conn = m_p2p.get_connections_count();
    res.outgoing_connections_count = m_p2p.get_outgoing_connections_count();
    res.incoming_connections_count = total_conn - res.outgoing_connections_count;
    res.white_peerlist_size = m_p2p.get_peerlist_manager().get_white_peers_count();
    res.grey_peerlist_size = m_p2p.get_peerlist_manager().get_gray_peers_count();
    res.current_blocks_median = m_core.get_blockchain_storage().get_current_comulative_blocksize_limit()/2;
    res.current_network_hashrate_50 = m_core.get_blockchain_storage().get_current_hashrate(50);
    res.current_network_hashrate_350 = m_core.get_blockchain_storage().get_current_hashrate(350);
    res.scratchpad_size = m_core.get_blockchain_storage().get_scratchpad_size();
    res.alias_count = m_core.get_blockchain_storage().get_aliases_count();
    m_core.get_blockchain_storage().get_transactions_daily_stat(res.transactions_cnt_per_day, res.transactions_volume_per_day);

    if (!res.outgoing_connections_count)
      res.daemon_network_state = COMMAND_RPC_GET_INFO::daemon_network_state_connecting;
    else if (m_p2p.get_payload_object().is_synchronized())
      res.daemon_network_state = COMMAND_RPC_GET_INFO::daemon_network_state_online;
    else
      res.daemon_network_state = COMMAND_RPC_GET_INFO::daemon_network_state_synchronizing;
    
    res.synchronization_start_height = m_p2p.get_payload_object().get_core_inital_height();
    res.max_net_seen_height = m_p2p.get_payload_object().get_max_seen_height();
    m_p2p.get_maintainers_info(res.mi);
    
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_stop_daemon(const COMMAND_RPC_STOP_DAEMON::request& req, COMMAND_RPC_STOP_DAEMON::response& res, connection_context& cntx)
  {
	  m_p2p.send_stop_signal();
	  res.status = CORE_RPC_STATUS_OK;
	  return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_blocks(const COMMAND_RPC_GET_BLOCKS_FAST::request& req, COMMAND_RPC_GET_BLOCKS_FAST::response& res, connection_context& cntx)
  {
    CHECK_CORE_READY();
    std::list<std::pair<block, std::list<transaction> > > bs;
    if(!m_core.find_blockchain_supplement(req.block_ids, bs, res.current_height, res.start_height, COMMAND_RPC_GET_BLOCKS_FAST_MAX_COUNT))
    {
      res.status = "Failed";
      return false;
    }

    BOOST_FOREACH(auto& b, bs)
    {
      res.blocks.resize(res.blocks.size()+1);
      res.blocks.back().block = block_to_blob(b.first);
      BOOST_FOREACH(auto& t, b.second)
      {
        res.blocks.back().txs.push_back(tx_to_blob(t));
      }
    }

    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_random_outs(const COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::request& req, COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::response& res, connection_context& cntx)
  {
    CHECK_CORE_READY();
    res.status = "Failed";
    if(!m_core.get_random_outs_for_amounts(req, res))
    {
      return true;
    }

    res.status = CORE_RPC_STATUS_OK;
    std::stringstream ss;
    typedef COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount outs_for_amount;
    typedef COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::out_entry out_entry;
    std::for_each(res.outs.begin(), res.outs.end(), [&](outs_for_amount& ofa)
    {
      ss << "[" << ofa.amount << "]:";
      CHECK_AND_ASSERT_MES(ofa.outs.size(), ;, "internal error: ofa.outs.size() is empty for amount " << ofa.amount);
      std::for_each(ofa.outs.begin(), ofa.outs.end(), [&](out_entry& oe)
          {
            ss << oe.global_amount_index << " ";
          });
      ss << ENDL;
    });
    std::string s = ss.str();
    LOG_PRINT_L2("COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS: " << ENDL << s);
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_indexes(const COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::request& req, COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::response& res, connection_context& cntx)
  {
    CHECK_CORE_READY();
    bool r = m_core.get_tx_outputs_gindexs(req.txid, res.o_indexes);
    if(!r)
    {
      res.status = "Failed";
      return true;
    }
    res.status = CORE_RPC_STATUS_OK;
    LOG_PRINT_L2("COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES: [" << res.o_indexes.size() << "]");
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_set_maintainers_info(const COMMAND_RPC_SET_MAINTAINERS_INFO::request& req, COMMAND_RPC_SET_MAINTAINERS_INFO::response& res, connection_context& cntx)
  {
    if(!m_p2p.handle_maintainers_entry(req))
    {
      res.status = "Failed to get call handle_maintainers_entry()";
      return true;
    }

    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_tx_pool(const COMMAND_RPC_GET_TX_POOL::request& req, COMMAND_RPC_GET_TX_POOL::response& res, connection_context& cntx)
  {
    CHECK_CORE_READY();
    std::list<transaction> txs;
    if (!m_core.get_pool_transactions(txs))
    {
      res.status = "Failed to call get_pool_transactions()";
      return true;
    }

    for(auto& tx: txs)
    {
      res.txs.push_back(t_serializable_object_to_blob(tx));
    }
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_check_keyimages(const COMMAND_RPC_CHECK_KEYIMAGES::request& req, COMMAND_RPC_CHECK_KEYIMAGES::response& res, connection_context& cntx)
  {
    m_core.get_blockchain_storage().check_keyimages(req.images, res.images_stat);
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_transactions(const COMMAND_RPC_GET_TRANSACTIONS::request& req, COMMAND_RPC_GET_TRANSACTIONS::response& res, connection_context& cntx)
  {
    CHECK_CORE_READY();
    std::vector<crypto::hash> vh;
    BOOST_FOREACH(const auto& tx_hex_str, req.txs_hashes)
    {
      blobdata b;
      if(!string_tools::parse_hexstr_to_binbuff(tx_hex_str, b))
      {
        res.status = "Failed to parse hex representation of transaction hash";
        return true;
      }
      if(b.size() != sizeof(crypto::hash))
      {
        res.status = "Failed, size of data mismatch";
      }
      vh.push_back(*reinterpret_cast<const crypto::hash*>(b.data()));
    }
    std::list<crypto::hash> missed_txs;
    std::list<transaction> txs;
    bool r = m_core.get_transactions(vh, txs, missed_txs);
    if(!r)
    {
      res.status = "Failed";
      return true;
    }

    BOOST_FOREACH(auto& tx, txs)
    {
      blobdata blob = t_serializable_object_to_blob(tx);
      res.txs_as_hex.push_back(string_tools::buff_to_hex_nodelimer(blob));
    }

    BOOST_FOREACH(const auto& miss_tx, missed_txs)
    {
      res.missed_tx.push_back(string_tools::pod_to_hex(miss_tx));
    }

    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_send_raw_tx(const COMMAND_RPC_SEND_RAW_TX::request& req, COMMAND_RPC_SEND_RAW_TX::response& res, connection_context& cntx)
  {
    CHECK_CORE_READY();

    std::string tx_blob;
    if(!string_tools::parse_hexstr_to_binbuff(req.tx_as_hex, tx_blob))
    {
      LOG_PRINT_L0("[on_send_raw_tx]: Failed to parse tx from hexbuff: " << req.tx_as_hex);
      res.status = "Failed";
      return true;
    }

    currency_connection_context fake_context = AUTO_VAL_INIT(fake_context);
    tx_verification_context tvc = AUTO_VAL_INIT(tvc);
    if(!m_core.handle_incoming_tx(tx_blob, tvc, false))
    {
      LOG_PRINT_L0("[on_send_raw_tx]: Failed to process tx");
      res.status = "Failed";
      return true;
    }

    if(tvc.m_verifivation_failed)
    {
      LOG_PRINT_L0("[on_send_raw_tx]: tx verification failed");
      res.status = "Failed";
      return true;
    }


    NOTIFY_NEW_TRANSACTIONS::request r;
    r.txs.push_back(tx_blob);
    m_core.get_protocol()->relay_transactions(r, fake_context);
    //TODO: make sure that tx has reached other nodes here, probably wait to receive reflections from other nodes
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_start_mining(const COMMAND_RPC_START_MINING::request& req, COMMAND_RPC_START_MINING::response& res, connection_context& cntx)
  {
    CHECK_CORE_READY();
    account_public_address adr;
    if(!get_account_address_from_str(adr, req.miner_address))
    {
      res.status = "Failed, wrong address";
      return true;
    }

    if(!m_core.get_miner().start(adr, static_cast<size_t>(req.threads_count)))
    {
      res.status = "Failed, mining not started";
      return true;
    }
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_stop_mining(const COMMAND_RPC_STOP_MINING::request& req, COMMAND_RPC_STOP_MINING::response& res, connection_context& cntx)
  {
    CHECK_CORE_READY();
    if(!m_core.get_miner().stop())
    {
      res.status = "Failed, mining not stopped";
      return true;
    }
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_getblockcount(const COMMAND_RPC_GETBLOCKCOUNT::request& req, COMMAND_RPC_GETBLOCKCOUNT::response& res, connection_context& cntx)
  {
    CHECK_CORE_READY();
    res.count = m_core.get_current_blockchain_height();
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_getblockhash(const COMMAND_RPC_GETBLOCKHASH::request& req, COMMAND_RPC_GETBLOCKHASH::response& res, epee::json_rpc::error& error_resp, connection_context& cntx)
  {
    if(!check_core_ready())
    {
      error_resp.code = CORE_RPC_ERROR_CODE_CORE_BUSY;
      error_resp.message = "Core is busy";
      return false;
    }
    if(req.size() != 1)
    {
      error_resp.code = CORE_RPC_ERROR_CODE_WRONG_PARAM;
      error_resp.message = "Wrong parameters, expected height";
      return false;
    }
    uint64_t h = req[0];
    if(m_core.get_current_blockchain_height() <= h)
    {
      error_resp.code = CORE_RPC_ERROR_CODE_TOO_BIG_HEIGHT;
      error_resp.message = std::string("To big height: ") + std::to_string(h) + ", current blockchain height = " +  std::to_string(m_core.get_current_blockchain_height());
    }
    res = string_tools::pod_to_hex(m_core.get_block_id_by_height(h));
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  uint64_t slow_memmem(void* start_buff, size_t buflen,void* pat,size_t patlen)
  {
    void* buf = start_buff;
    void* end=(char*)buf+buflen-patlen;
    while((buf=memchr(buf,((char*)pat)[0],buflen)))
    {
      if(buf>end)
        return 0;
      if(memcmp(buf,pat,patlen)==0)
        return (char*)buf - (char*)start_buff;
      buf=(char*)buf+1;
    }
    return 0;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_getblocktemplate(const COMMAND_RPC_GETBLOCKTEMPLATE::request& req, COMMAND_RPC_GETBLOCKTEMPLATE::response& res, epee::json_rpc::error& error_resp, connection_context& cntx)
  {
    if(!check_core_ready())
    {
      error_resp.code = CORE_RPC_ERROR_CODE_CORE_BUSY;
      error_resp.message = "Core is busy";
      return false;
    }

    if(req.reserve_size > 255)
    {
      error_resp.code = CORE_RPC_ERROR_CODE_TOO_BIG_RESERVE_SIZE;
      error_resp.message = "To big reserved size, maximum 255";
      return false;
    }

    currency::account_public_address acc = AUTO_VAL_INIT(acc);

    if(!req.wallet_address.size() || !currency::get_account_address_from_str(acc, req.wallet_address))
    {
      error_resp.code = CORE_RPC_ERROR_CODE_WRONG_WALLET_ADDRESS;
      error_resp.message = "Failed to parse wallet address";
      return false;
    }
    alias_info ai = AUTO_VAL_INIT(ai);
    if(req.alias_details.alias.size())
    {
      //have alias requested to register
      if(!validate_alias_name(req.alias_details.alias))
      {
        error_resp.code = CORE_RPC_ERROR_CODE_INVALID_ALIAS_NAME;
        error_resp.message = "Invalid alias name";
        return false;
      }
      if(!currency::get_account_address_from_str(ai.m_address, req.alias_details.details.address))
      {
        LOG_ERROR("Invalid alias address: " << req.alias_details.details.address);
        error_resp.code = CORE_RPC_ERROR_CODE_INVALID_ALIAS_ADDRESS;
        error_resp.message = "Invalid alias address";
        return false;
      }
      if(req.alias_details.details.comment.size() > std::numeric_limits<uint8_t>::max())
      {
        error_resp.code = CORE_RPC_ERROR_CODE_ALIAS_COMMENT_TO_LONG;
        error_resp.message = "Alias comment is too long";
        return false;
      }
      if(req.alias_details.details.tracking_key.size())
      {//load tracking key
        if(!string_tools::parse_tpod_from_hex_string(req.alias_details.details.tracking_key, ai.m_view_key))
        {
          error_resp.code = CORE_RPC_ERROR_CODE_INVALID_ALIAS_ADDRESS;
          error_resp.message = "Invalid alias address";
          return false;
        }
      }
      ai.m_alias = req.alias_details.alias;
      ai.m_text_comment = req.alias_details.details.comment;
      LOG_PRINT_L1("COMMAND_RPC_GETBLOCKTEMPLATE: Alias requested for " << ai.m_alias << " -->>" << req.alias_details.details.address);
    }

    bool donations_vote = true;
    if (req.dev_bounties_vote.type() == typeid(bool))
    {
      donations_vote = boost::get<bool>(req.dev_bounties_vote);
    }

    block b = AUTO_VAL_INIT(b);
    currency::blobdata blob_reserve = PROJECT_VERSION_LONG;
    blob_reserve.resize(blob_reserve.size() + 1 + req.reserve_size, 0);
    wide_difficulty_type dt = 0;
    if (!m_core.get_block_template(b, acc, dt, res.height, blob_reserve, donations_vote, ai))
    {
      error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
      error_resp.message = "Internal error: failed to create block template";
      LOG_ERROR("Failed to create block template");
      return false;
    }


    res.difficulty = dt.convert_to<uint64_t>();
    blobdata block_blob = t_serializable_object_to_blob(b);
    std::string::size_type pos = block_blob.find(PROJECT_VERSION_LONG);
    if(pos == std::string::npos)
    {
      error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
      error_resp.message = "Internal error: failed to create block template";
      LOG_ERROR("Failed to find tx pub key in blockblob");
      return false;
    }
    res.reserved_offset  = pos + strlen(PROJECT_VERSION_LONG)+1;
    if(res.reserved_offset + req.reserve_size > block_blob.size())
    {
      error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
      error_resp.message = "Internal error: failed to create block template";
      LOG_ERROR("Failed to calculate offset for ");
      return false;
    }
    res.blocktemplate_blob = string_tools::buff_to_hex_nodelimer(block_blob);
    res.status = CORE_RPC_STATUS_OK;


    //self validating code for alias registration problem(reported by Clintar)
    if (ai.m_alias.size())
    {
      tx_extra_info tei = AUTO_VAL_INIT(tei);
      bool r = parse_and_validate_tx_extra(b.miner_tx, tei);
      if (!r)
      {
        error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
        error_resp.message = "Internal error: failed to create block template, self extra_info validation failed";
        LOG_ERROR("Failed to validate extra");
        return false;
      }
      if (!tei.m_alias.m_alias.size())
      {
        error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
        error_resp.message = "Internal error: failed to create block template, self extra_info validation failed: alias not found in extra";
        LOG_ERROR("Failed to validate extra: alias not found");
        return false;
      }
      LOG_PRINT_L0("BLOCK_WITH_ALIAS( " << tei.m_alias.m_alias << "): " << ENDL
        << currency::obj_to_json_str(b) << ENDL
        << res.blocktemplate_blob
        );
    }


    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_submitblock(const COMMAND_RPC_SUBMITBLOCK::request& req, COMMAND_RPC_SUBMITBLOCK::response& res, epee::json_rpc::error& error_resp, connection_context& cntx)
  {
    CHECK_CORE_READY();
    if(req.size()!=1)
    {
      error_resp.code = CORE_RPC_ERROR_CODE_WRONG_PARAM;
      error_resp.message = "Wrong param";
      return false;
    }
    blobdata blockblob;
    if(!string_tools::parse_hexstr_to_binbuff(req[0], blockblob))
    {
      error_resp.code = CORE_RPC_ERROR_CODE_WRONG_BLOCKBLOB;
      error_resp.message = "Wrong block blob";
      return false;
    }
    
    block b = AUTO_VAL_INIT(b);
    if(!parse_and_validate_block_from_blob(blockblob, b))
    {
      error_resp.code = CORE_RPC_ERROR_CODE_WRONG_BLOCKBLOB;
      error_resp.message = "Wrong block blob";
      return false;
    }

    if(!m_core.handle_block_found(b))
    {
      error_resp.code = CORE_RPC_ERROR_CODE_BLOCK_NOT_ACCEPTED;
      error_resp.message = "Block not accepted";
      return false;
    }
    res.status = "OK";
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  uint64_t core_rpc_server::get_block_reward(const block& blk)
  {
    uint64_t reward = 0;
    uint64_t royalty = 0;
    uint64_t donation = 0;
    uint64_t h = get_block_height(blk);
    BOOST_FOREACH(const tx_out& out, blk.miner_tx.vout)
    {
      reward += out.amount;
    }
  
  if(h && !(h%CURRENCY_DONATIONS_INTERVAL) /*&& h > 21600*/)
  {
    bool r = m_core.get_blockchain_storage().lookfor_donation(blk.miner_tx, donation, royalty);
    CHECK_AND_ASSERT_MES(r, false, "Failed to lookfor_donation");
    reward -= donation + royalty;
  }

    return reward;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::fill_block_header_responce(const block& blk, bool orphan_status, block_header_responce& responce)
  {
    responce.major_version = blk.major_version;
    responce.minor_version = blk.minor_version;
    responce.timestamp = blk.timestamp;
    responce.prev_hash = string_tools::pod_to_hex(blk.prev_id);
    responce.nonce = blk.nonce;
    responce.orphan_status = orphan_status;
    responce.height = get_block_height(blk);
    responce.depth = m_core.get_current_blockchain_height() - responce.height - 1;
    responce.hash = string_tools::pod_to_hex(get_block_hash(blk));
    responce.difficulty = m_core.get_blockchain_storage().block_difficulty(responce.height).convert_to<uint64_t>();
    responce.reward = get_block_reward(blk);
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_last_block_header(const COMMAND_RPC_GET_LAST_BLOCK_HEADER::request& req, COMMAND_RPC_GET_LAST_BLOCK_HEADER::response& res, epee::json_rpc::error& error_resp, connection_context& cntx)
  {
    if(!check_core_ready())
    {
      error_resp.code = CORE_RPC_ERROR_CODE_CORE_BUSY;
      error_resp.message = "Core is busy.";
      return false;
    }
    block last_block = AUTO_VAL_INIT(last_block);
    bool have_last_block_hash = m_core.get_blockchain_storage().get_top_block(last_block);
    if (!have_last_block_hash)
    {
      error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
      error_resp.message = "Internal error: can't get last block hash.";
      return false;
    }
    bool responce_filled = fill_block_header_responce(last_block, false, res.block_header);
    if (!responce_filled)
    {
      error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
      error_resp.message = "Internal error: can't produce valid response.";
      return false;
    }
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_block_header_by_hash(const COMMAND_RPC_GET_BLOCK_HEADER_BY_HASH::request& req, COMMAND_RPC_GET_BLOCK_HEADER_BY_HASH::response& res, epee::json_rpc::error& error_resp, connection_context& cntx){
    if(!check_core_ready())
    {
      error_resp.code = CORE_RPC_ERROR_CODE_CORE_BUSY;
      error_resp.message = "Core is busy.";
      return false;
    }
    crypto::hash block_hash;
    bool hash_parsed = parse_hash256(req.hash, block_hash);
    if(!hash_parsed)
    {
      error_resp.code = CORE_RPC_ERROR_CODE_WRONG_PARAM;
      error_resp.message = "Failed to parse hex representation of block hash. Hex = " + req.hash + '.';
      return false;
    }
    block blk;
    bool have_block = m_core.get_block_by_hash(block_hash, blk);
    if (!have_block)
    {
      error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
      error_resp.message = "Internal error: can't get block by hash. Hash = " + req.hash + '.';
      return false;
    }
    if (blk.miner_tx.vin.front().type() != typeid(txin_gen))
    {
      error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
      error_resp.message = "Internal error: coinbase transaction in the block has the wrong type";
      return false;
    }

    bool responce_filled = fill_block_header_responce(blk, false, res.block_header);
    if (!responce_filled)
    {
      error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
      error_resp.message = "Internal error: can't produce valid response.";
      return false;
    }
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_block_header_by_height(const COMMAND_RPC_GET_BLOCK_HEADER_BY_HEIGHT::request& req, COMMAND_RPC_GET_BLOCK_HEADER_BY_HEIGHT::response& res, epee::json_rpc::error& error_resp, connection_context& cntx){
    if(!check_core_ready())
    {
      error_resp.code = CORE_RPC_ERROR_CODE_CORE_BUSY;
      error_resp.message = "Core is busy.";
      return false;
    }
    if(m_core.get_current_blockchain_height() <= req.height)
    {
      error_resp.code = CORE_RPC_ERROR_CODE_TOO_BIG_HEIGHT;
      error_resp.message = std::string("To big height: ") + std::to_string(req.height) + ", current blockchain height = " +  std::to_string(m_core.get_current_blockchain_height());
      return false;
    }
    block blk = AUTO_VAL_INIT(blk);
    m_core.get_blockchain_storage().get_block_by_height(req.height, blk);
    
    bool responce_filled = fill_block_header_responce(blk, false, res.block_header);
    if (!responce_filled)
    {
      error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
      error_resp.message = "Internal error: can't produce valid response.";
      return false;
    }
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  
bool core_rpc_server::f_on_blocks_list_json(const F_COMMAND_RPC_GET_BLOCKS_LIST::request& req, F_COMMAND_RPC_GET_BLOCKS_LIST::response& res, epee::json_rpc::error& error_resp, connection_context& cntx) {
    if(!check_core_ready())
    {
      error_resp.code = CORE_RPC_ERROR_CODE_CORE_BUSY;
      error_resp.message = "Core is busy.";
      return false;
    }
  if (m_core.get_current_blockchain_height() <= req.height) {
      error_resp.code = CORE_RPC_ERROR_CODE_TOO_BIG_HEIGHT;
      error_resp.message = std::string("To big height: ") + std::to_string(req.height) + ", current blockchain height = " + std::to_string(m_core.get_current_blockchain_height());
      return false;
  }

  uint64_t print_blocks_count = 30;
  uint64_t last_height = req.height - print_blocks_count;
  if (req.height <= print_blocks_count)  {
    last_height = 0;
  } 

  for (uint64_t i = req.height; i >= last_height; i--) 
  {
    crypto::hash block_hash = m_core.get_block_id_by_height(static_cast<uint32_t>(i));
    block blk;
    if (!m_core.get_block_by_hash(block_hash, blk)) {
      error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
      error_resp.message = "Internal error: can't get block by height. Height = " + std::to_string(i) + '.';
      return false;
    }
    std::vector<size_t> blocks_sizes;
    m_core.get_backward_blocks_sizes(i, blocks_sizes, 1);
    
    size_t tx_cumulative_block_size = misc_utils::median(blocks_sizes);
    //m_core.getBlockSize(block_hash, tx_cumulative_block_size);
    
    size_t blokBlobSize = get_object_blobsize(blk);
    size_t minerTxBlobSize = get_object_blobsize(blk.miner_tx);

    f_block_short_response block_short;
    block_short.cumul_size = blokBlobSize + tx_cumulative_block_size - minerTxBlobSize;
    block_short.timestamp = blk.timestamp;
    block_short.height = i;
    block_short.hash = string_tools::pod_to_hex(block_hash);
    block_short.cumul_size = blokBlobSize + tx_cumulative_block_size - minerTxBlobSize;
    block_short.tx_count = blk.tx_hashes.size() + 1;
    block_short.difficulty = m_core.get_blockchain_storage().block_difficulty(i).convert_to<uint64_t>();
    res.blocks.push_back(block_short);

    if (i == 0)
      break;
  }

  res.status = CORE_RPC_STATUS_OK;
  return true;
}

bool core_rpc_server::f_on_block_json(const F_COMMAND_RPC_GET_BLOCK_DETAILS::request& req, F_COMMAND_RPC_GET_BLOCK_DETAILS::response& res, epee::json_rpc::error& error_resp, connection_context& cntx) {
  crypto::hash hash;

  if (!parse_hash256(req.hash, hash)) {
    error_resp.code = CORE_RPC_ERROR_CODE_WRONG_PARAM;
      error_resp.message = "Failed to parse hex representation of block hash. Hex = " + req.hash + '.';
      return false;
      
  }

  block blk;
  if (!m_core.get_block_by_hash(hash, blk)) {
    error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
      error_resp.message = "Internal error: can't get block by hash. Hash = " + req.hash + '.';
      return false;
  }
  if (blk.miner_tx.vin.front().type() != typeid(txin_gen)) {
      error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
      error_resp.message = "Internal error: coinbase transaction in the block has the wrong type";
      return false;
  }

  block_header_responce block_header;
  res.block.height = boost::get<txin_gen>(blk.miner_tx.vin.front()).height;
  fill_block_header_responce(blk, false, block_header);

  res.block.major_version = block_header.major_version;
  res.block.minor_version = block_header.minor_version;
  res.block.timestamp = block_header.timestamp;
  res.block.prev_hash = block_header.prev_hash;
  res.block.nonce = block_header.nonce;
  res.block.hash = string_tools::pod_to_hex(hash);
  res.block.depth = m_core.get_current_blockchain_height() - res.block.height - 1;
  res.block.difficulty = block_header.difficulty;

  res.block.reward = block_header.reward;

  std::vector<size_t> blocksSizes;
  if (!m_core.get_backward_blocks_sizes(res.block.height, blocksSizes, CURRENCY_REWARD_BLOCKS_WINDOW)) {
    return false;
  }
  res.block.sizeMedian = misc_utils::median(blocksSizes);

  size_t blockSize = 0;
  std::vector<size_t> blocks_sizes;
    m_core.get_backward_blocks_sizes(res.block.height, blocks_sizes, 1);
  if (!misc_utils::median(blocks_sizes)) {
    error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
    error_resp.message = "Internal error: no block sizes";
    return false;
  }
  res.block.transactionsCumulativeSize = misc_utils::median(blocks_sizes);

  size_t blokBlobSize = get_object_blobsize(blk);
  size_t minerTxBlobSize = get_object_blobsize(blk.miner_tx);
  res.block.blockSize = blokBlobSize + res.block.transactionsCumulativeSize - minerTxBlobSize;

  if (!m_core.get_blockchain_storage().get_already_generated_coins(hash, res.block.alreadyGeneratedCoins)) {
    error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
    error_resp.message = "Internal error: could not get generated coins for block with hash: " + string_tools::pod_to_hex(hash);
    return false;
  }

//  if (!m_core.getGeneratedTransactionsNumber(res.block.height, res.block.alreadyGeneratedTransactions)) {
//    return false;
//  }
  res.block.alreadyGeneratedTransactions = 0;

  uint64_t prevBlockGeneratedCoins = 0;
  if (res.block.height > 0) {
    if (!m_core.get_blockchain_storage().get_already_generated_coins(blk.prev_id, prevBlockGeneratedCoins)) {
      error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
      error_resp.message = "Internal error: could not get generated coins for block with hash: " + string_tools::pod_to_hex(blk.prev_id);
      return false;
    }
  }
  uint64_t prevBlockDonatedCoins = 0;
  if (res.block.height > 0) {
    if (!m_core.get_blockchain_storage().get_already_donated_coins(blk.prev_id, prevBlockDonatedCoins)) {
      error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
      error_resp.message = "Internal error: could not get donated coins for block with hash: " + string_tools::pod_to_hex(blk.prev_id);
      return false;
    }
  }
  uint64_t maxReward = 0;
  uint64_t currentReward = 0;
  uint64_t emissionChange = 0;
  if (!currency::get_block_reward(res.block.sizeMedian, res.block.transactionsCumulativeSize, prevBlockGeneratedCoins, prevBlockDonatedCoins, maxReward, emissionChange)) {
    error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
    error_resp.message = "Internal error: could not get block reward";
    return false;
  }

  res.block.baseReward = maxReward;
  if (maxReward == 0 && currentReward == 0) {
    res.block.penalty = static_cast<double>(0);
  } else {
    if (maxReward < currentReward) {
      error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
      error_resp.message = "Internal error: currentReward above maxReward";
      return false;
      return false;
    }
    res.block.penalty = 0;
  }

  // Base transaction adding
  f_transaction_short_response transaction_short;
  transaction_short.hash =  string_tools::pod_to_hex(get_transaction_hash(blk.miner_tx));
  transaction_short.fee = 0;
  transaction_short.amount_out = get_outs_money_amount(blk.miner_tx);
  transaction_short.size = get_object_blobsize(blk.miner_tx);
  res.block.transactions.push_back(transaction_short);


  std::list<crypto::hash> missed_txs;
  std::list<transaction> txs;
  m_core.get_transactions(blk.tx_hashes, txs, missed_txs);

  res.block.totalFeeAmount = 0;

  for (const transaction& tx : txs) {
    f_transaction_short_response transaction_short;
    uint64_t amount_in = 0;
    get_inputs_money_amount(tx, amount_in);
    uint64_t amount_out = get_outs_money_amount(tx);

    transaction_short.hash =  string_tools::pod_to_hex(get_transaction_hash(tx));
    transaction_short.fee = amount_in - amount_out;
    transaction_short.amount_out = amount_out;
    transaction_short.size = get_object_blobsize(tx);
    res.block.transactions.push_back(transaction_short);

    res.block.totalFeeAmount += transaction_short.fee;
  }

  res.status = CORE_RPC_STATUS_OK;
  return true;
}
bool core_rpc_server::f_on_transaction_json(const F_COMMAND_RPC_GET_TRANSACTION_DETAILS::request& req, F_COMMAND_RPC_GET_TRANSACTION_DETAILS::response& res, epee::json_rpc::error& error_resp, connection_context& cntx) {
  crypto::hash hash;

  if (!parse_hash256(req.hash, hash)) {
    error_resp.code = CORE_RPC_ERROR_CODE_WRONG_PARAM;
    error_resp.message = "Failed to parse hex representation of transaction hash. Hex = " + req.hash + '.';
    return false;
  }

  std::vector<crypto::hash> tx_ids;
  tx_ids.push_back(hash);

  std::list<crypto::hash> missed_txs;
  std::list<transaction> txs;
  m_core.get_transactions(tx_ids, txs, missed_txs);
  transaction restx;
  
  if (1 == txs.size()) {
    restx = txs.front();
    BOOST_FOREACH(const auto& txin,  restx.vin)
    {
      f_txin_short txinput;
      if(txin.type() == typeid(txin_to_key))
      {
        
        const txin_to_key& in_to_key = boost::get<txin_to_key>(txin);
        txinput.k_image = string_tools::pod_to_hex(in_to_key.k_image);
        CHECKED_GET_SPECIFIC_VARIANT(txin, const txin_to_key, tokey_in, false);
        txinput.amount = tokey_in.amount;
        
      }
      res.txin.push_back(txinput);
    }
    BOOST_FOREACH(const auto& txout,  restx.vout)
    {
      f_txout_short txoutput;
      if(txout.target.type() == typeid(txout_to_key))
      {
        
        const txout_to_key& out_to_key = boost::get<txout_to_key>(txout.target);
        txoutput.key = string_tools::pod_to_hex(out_to_key.key);
//        CHECKED_GET_SPECIFIC_VARIANT(txin, const txout_to_key, tokey_out, false);
        txoutput.amount = txout.amount;
        
      }
      res.txout.push_back(txoutput);
//      res.txout.push_back(txinput);
    }
  } else {
    error_resp.code = CORE_RPC_ERROR_CODE_WRONG_PARAM;
    error_resp.message = "transaction wasn't found. Hash = " + req.hash + '.';
    return false;
  }

  crypto::hash blockHash;
  uint64_t blockHeight;
  if (m_core.get_blockchain_storage().get_block_containing_tx(hash, blockHash, blockHeight)) {
    block blk;
    if (m_core.get_block_by_hash(blockHash, blk)) {
      std::vector<size_t> blocks_sizes;
      m_core.get_backward_blocks_sizes(blockHeight, blocks_sizes, 1);
      size_t tx_cumulative_block_size = misc_utils::median(blocks_sizes);
      size_t blokBlobSize = get_object_blobsize(blk);
      size_t minerTxBlobSize = get_object_blobsize(blk.miner_tx);
      f_block_short_response block_short;

      block_short.cumul_size = blokBlobSize + tx_cumulative_block_size - minerTxBlobSize;
      block_short.timestamp = blk.timestamp;
      block_short.height = blockHeight;
      block_short.hash = string_tools::pod_to_hex(blockHash);
      block_short.tx_count = blk.tx_hashes.size() + 1;
      res.ablock = block_short;
    }
  }

  uint64_t amount_in = 0;
  get_inputs_money_amount(restx, amount_in);
  uint64_t amount_out = get_outs_money_amount(restx);

  res.txDetails.hash = string_tools::pod_to_hex(get_transaction_hash(restx));
  res.txDetails.fee = amount_in - amount_out;
  if (amount_in == 0)
    res.txDetails.fee = 0;
  res.txDetails.amount_out = amount_out;
  res.txDetails.size = get_object_blobsize(restx);

  uint64_t mixin;
  if (!f_getMixin(restx, mixin)) {
    return false;
  }
  res.txDetails.mixin = mixin;

  payment_id_t paymentId;
  if (currency::get_payment_id_from_tx_extra(restx, paymentId)) {
    res.txDetails.paymentId = string_tools::buff_to_hex_nodelimer(paymentId);
  } else {
    res.txDetails.paymentId = "";
  }

  res.status = CORE_RPC_STATUS_OK;
  return true;
}
bool core_rpc_server::f_on_pool_json(const F_COMMAND_RPC_GET_POOL::request& req, F_COMMAND_RPC_GET_POOL::response& res, epee::json_rpc::error& error_resp, connection_context& cntx) {
  
  res.transactions = m_core.print_pool(true);
  res.status = CORE_RPC_STATUS_OK;
  return true;
}

bool core_rpc_server::f_getMixin(const transaction& transaction, uint64_t& mixin) {
  mixin = 0;
  for (const txin_v& txin : transaction.vin) {
    if (txin.type() != typeid(txin_to_key)) {
      continue;
    }
    uint64_t currentMixin = boost::get<txin_to_key>(txin).key_offsets.size();
    if (currentMixin > mixin) {
      mixin = currentMixin;
    }
  }
  return true;
}

  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_alias_details(const COMMAND_RPC_GET_ALIAS_DETAILS::request& req, COMMAND_RPC_GET_ALIAS_DETAILS::response& res, epee::json_rpc::error& error_resp, connection_context& cntx)
  {
    if(!check_core_ready())
    {
      error_resp.code = CORE_RPC_ERROR_CODE_CORE_BUSY;
      error_resp.message = "Core is busy.";
      return false;
    }
    alias_info_base aib = AUTO_VAL_INIT(aib);
    if(!validate_alias_name(req.alias))
    {      
      res.status = "Alias have wrong name";
      return false;
    }
    if(!m_core.get_blockchain_storage().get_alias_info(req.alias, aib))
    {
      res.status = "Alias not found.";
      return true;
    }
    res.alias_details.address = currency::get_account_address_as_str(aib.m_address);
    res.alias_details.comment = aib.m_text_comment;
    if(aib.m_view_key != currency::null_skey)
      res.alias_details.tracking_key = string_tools::pod_to_hex(aib.m_view_key);

    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_all_aliases(const COMMAND_RPC_GET_ALL_ALIASES::request& req, COMMAND_RPC_GET_ALL_ALIASES::response& res, epee::json_rpc::error& error_resp, connection_context& cntx)
  {
    if(!check_core_ready())
    {
      error_resp.code = CORE_RPC_ERROR_CODE_CORE_BUSY;
      error_resp.message = "Core is busy.";
      return false;
    }

    std::list<currency::alias_info> aliases;
    m_core.get_blockchain_storage().get_all_aliases(aliases);
    for(auto a: aliases)
    {
      res.aliases.push_back(alias_rpc_details());
      res.aliases.back().alias = a.m_alias;
      res.aliases.back().details.address =  currency::get_account_address_as_str(a.m_address);
      res.aliases.back().details.comment = a.m_text_comment;
      if(a.m_view_key != currency::null_skey)
        res.aliases.back().details.tracking_key = string_tools::pod_to_hex(a.m_view_key);
    }
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_getblock(const COMMAND_RPC_GETBLOCK::request& req, COMMAND_RPC_GETBLOCK::response& res, epee::json_rpc::error& error_resp, connection_context& cntx)
  {
      if(!check_core_ready())
      {
          error_resp.code = CORE_RPC_ERROR_CODE_CORE_BUSY;
          error_resp.message = "Core is busy";
          return false;
      }

      currency::block b;
      if (!m_core.get_blockchain_storage().get_block_by_height(req.height, b)) {
          error_resp.code = CORE_RPC_ERROR_CODE_TOO_BIG_HEIGHT;
          error_resp.message = "Invalid block height";
          return false;
      }
      res.block_hash = string_tools::pod_to_hex(get_block_hash(b));
      res.block_time = b.timestamp;
          
      std::vector<crypto::hash> txs_ids;
      txs_ids.reserve(b.tx_hashes.size());
      for(const auto& tx: b.tx_hashes) {
          txs_ids.push_back(tx);
      }

      std::list<crypto::hash> missed_ids;
      std::list<transaction> txs;
      m_core.get_transactions(txs_ids, txs, missed_ids);
      if (missed_ids.size()) {
          //can't find transaction from block. Major boo-boo
          error_resp.code = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
          error_resp.message = "Can't find transaction in block";
          return false;
      }

      for(const auto& tx: txs) {
          payment_id_t payment_id;
          get_payment_id_from_tx_extra(tx, payment_id);
          crypto::hash hash;
          get_transaction_hash(tx, hash);

          transfer_rpc_details transfer;
          transfer.tx_hash = string_tools::pod_to_hex(hash);
          transfer.payment_id = string_tools::buff_to_hex_nodelimer(payment_id);
          
          for (const auto& vout: tx.vout) {
              const txout_to_key* key_out = boost::get<txout_to_key>(&vout.target);
              if (!key_out) {
                  LOG_PRINT_L0("Found tx_out with target != txout_to_key... skipping");
                  continue;
              }
              transfer.amount = vout.amount;
              transfer.recipient = string_tools::pod_to_hex(key_out->key);
              res.transfers.push_back(transfer);
          }
      }
      return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::get_addendum_for_hi(const mining::height_info& hi, std::list<mining::addendum>& res)
  {
    if(!hi.height || hi.height + 1 == m_core.get_current_blockchain_height())
      return true;//do not make addendum for whole blockchain

    CHECK_AND_ASSERT_MES(hi.height < m_core.get_current_blockchain_height(), false, "wrong height parameter passed: " << hi.height);

    crypto::hash h = null_hash;
    bool r = string_tools::hex_to_pod(hi.block_id, h);
    CHECK_AND_ASSERT_MES(r, false, "wrong block_id parameter passed: " << hi.block_id);
        

    crypto::hash block_chain_id = m_core.get_blockchain_storage().get_block_id_by_height(hi.height);
    CHECK_AND_ASSERT_MES(block_chain_id != null_hash, false, "internal error: can't get block id by height: " << hi.height);
    uint64_t height = hi.height;
    if(block_chain_id != h)
    {
      //probably split
      CHECK_AND_ASSERT_MES(hi.height > 0, false, "wrong height passed");
      --height;
    }

    std::list<block> blocks;
    r = m_core.get_blockchain_storage().get_blocks(height + 1, m_core.get_current_blockchain_height() - (height+1), blocks);
    CHECK_AND_ASSERT_MES(r, false, "failed to get blocks");
    for(auto it = blocks.begin(); it!= blocks.end(); it++)
    {
      res.push_back(mining::addendum());
      res.back().hi.height = get_block_height(*it);
      res.back().hi.block_id = string_tools::pod_to_hex(get_block_hash(*it));
      res.back().prev_id = string_tools::pod_to_hex(it->prev_id);
      std::vector<crypto::hash> ad;
      r = get_block_scratchpad_addendum(*it, ad);
      CHECK_AND_ASSERT_MES(r, false, "Failed to add block addendum");
      addendum_to_hexstr(ad, res.back().addm);      
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::get_current_hi(mining::height_info& hi)
  {
    block prev_block = AUTO_VAL_INIT(prev_block);
    m_core.get_blockchain_storage().get_top_block(prev_block);
    hi.block_id  = string_tools::pod_to_hex(currency::get_block_hash(prev_block));
    hi.height = get_block_height(prev_block);
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  void core_rpc_server::set_session_blob(const std::string& session_id, const currency::block& blob)
  {
    CRITICAL_REGION_LOCAL(m_session_jobs_lock);
    m_session_jobs[session_id] = blob;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::get_session_blob(const std::string& session_id, currency::block& blob)
  {
    CRITICAL_REGION_LOCAL(m_session_jobs_lock);
    auto it = m_session_jobs.find(session_id);
    if(it == m_session_jobs.end())
      return false;

    blob = it->second;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::get_job(const std::string& job_id, mining::job_details& job, epee::json_rpc::error& err, connection_context& cntx)
  {
    COMMAND_RPC_GETBLOCKTEMPLATE::request bt_req = AUTO_VAL_INIT(bt_req);
    COMMAND_RPC_GETBLOCKTEMPLATE::response bt_res = AUTO_VAL_INIT(bt_res);

    //bt_req.alias_details  - set alias here
    bt_req.dev_bounties_vote = epee::serialization::storage_entry(true); //set vote 
    bt_req.reserve_size = 0; //if you want to put some data into extra
    // !!!!!!!! SET YOUR WALLET ADDRESS HERE  !!!!!!!!
    bt_req.wallet_address = "1HNJjUsofq5LYLoXem119dd491yFAb5g4bCHkecV4sPqigmuxw57Ci9am71fEN4CRmA9jgnvo5PDNfaq8QnprWmS5uLqnbq";
    
    if(!on_getblocktemplate(bt_req, bt_res, err, cntx))
      return false;

    //patch block blob if you need(bt_res.blocktemplate_blob), and than load block from blob template
    //important: you can't change block size, since it could touch reward and block became invalid

    block b = AUTO_VAL_INIT(b);
    std::string bin_buff;
    bool r = string_tools::parse_hexstr_to_binbuff(bt_res.blocktemplate_blob, bin_buff);
    CHECK_AND_ASSERT_MES(r, false, "internal error, failed to parse hex block");
    r = currency::parse_and_validate_block_from_blob(bin_buff, b);
    CHECK_AND_ASSERT_MES(r, false, "internal error, failed to parse block");

    set_session_blob(job_id, b);
    job.blob = string_tools::buff_to_hex_nodelimer(currency::get_block_hashing_blob(b));
    //TODO: set up share difficulty here!
    job.difficulty = std::to_string(bt_res.difficulty); //difficulty leaved as string field since it will be refactored into 128 bit format
    job.job_id = "SOME_JOB_ID";
    get_current_hi(job.prev_hi);
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_login(const mining::COMMAND_RPC_LOGIN::request& req, mining::COMMAND_RPC_LOGIN::response& res, connection_context& cntx)
  {
    if(!check_core_ready())
    {
      res.status = CORE_RPC_STATUS_BUSY;
      return true;
    }
    
    //TODO: add login information here

    if(!get_addendum_for_hi(req.hi, res.job.addms))
    {
      res.status = "Fail at get_addendum_for_hi, check daemon logs for details";
      return true;
    }

    res.id =  std::to_string(m_session_counter++); //session id

    if(req.hi.height)
    {
      epee::json_rpc::error err = AUTO_VAL_INIT(err);
      if(!get_job(res.id, res.job, err, cntx))
      {
        res.status = err.message;
        return true;
      }
    }

    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_getjob(const mining::COMMAND_RPC_GETJOB::request& req, mining::COMMAND_RPC_GETJOB::response& res, connection_context& cntx)
  {
    if(!check_core_ready())
    {
      res.status = CORE_RPC_STATUS_BUSY;
      return true;
    }
    
    if(!get_addendum_for_hi(req.hi, res.jd.addms))
    {
      res.status = "Fail at get_addendum_for_hi, check daemon logs for details";
      return true;
    }

    /*epee::json_rpc::error err = AUTO_VAL_INIT(err);
    if(!get_job(req.id, res.jd, err, cntx))
    {
      res.status = err.message;
      return true;
    }*/

    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_getscratchpad(const mining::COMMAND_RPC_GET_FULLSCRATCHPAD::request& req, mining::COMMAND_RPC_GET_FULLSCRATCHPAD::response& res, connection_context& cntx)
  {
    if(!check_core_ready())
    {
      res.status = CORE_RPC_STATUS_BUSY;
      return true;
    }
    std::vector<crypto::hash> scratchpad_local;
    m_core.get_blockchain_storage().copy_scratchpad(scratchpad_local);
    addendum_to_hexstr(scratchpad_local, res.scratchpad_hex); 
    get_current_hi(res.hi);
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_alias_by_address(const COMMAND_RPC_GET_ALIASES_BY_ADDRESS::request& req, COMMAND_RPC_GET_ALIASES_BY_ADDRESS::response& res, epee::json_rpc::error& error_resp, connection_context& cntx)
  {
    account_public_address addr = AUTO_VAL_INIT(addr);
    if (!get_account_address_from_str(addr, req))
    {
      res.status = CORE_RPC_STATUS_FAILED;
    }
    res.alias = m_core.get_blockchain_storage().get_alias_by_address(addr);
    if (res.alias.size())
      res.status = CORE_RPC_STATUS_OK;
    else
      res.status = CORE_RPC_STATUS_NOT_FOUND;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_submit(const mining::COMMAND_RPC_SUBMITSHARE::request& req, mining::COMMAND_RPC_SUBMITSHARE::response& res, connection_context& cntx)
  {
    if(!check_core_ready())
    {
      res.status = CORE_RPC_STATUS_BUSY;
      return true;
    }
    block b = AUTO_VAL_INIT(b);
    if(!get_session_blob(req.id, b))
    {
      res.status = "Wrong session id";
      return true;
    }

    b.nonce = req.nonce;

    if(!m_core.handle_block_found(b))
    {
      res.status = "Block not accepted";
      LOG_ERROR("Submited block not accepted");
      return true;
    }
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_store_scratchpad(const mining::COMMAND_RPC_STORE_SCRATCHPAD::request& req, mining::COMMAND_RPC_STORE_SCRATCHPAD::response& res, connection_context& cntx)
  {
    if(!check_core_ready())
    {
      res.status = CORE_RPC_STATUS_BUSY;
      return true;
    }

    if(!m_core.get_blockchain_storage().extport_scratchpad_to_file(req.local_file_path))    
      res.status = CORE_RPC_STATUS_BUSY;
    else
      res.status = CORE_RPC_STATUS_OK;

    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_getfullscratchpad2(const epee::net_utils::http::http_request_info& query_info, epee::net_utils::http::http_response_info& response_info, connection_context& cntx)
  {
    if (!check_core_ready())
    {
      return true;
    }
    mining::height_info hi = AUTO_VAL_INIT(hi);
    get_current_hi(hi);
    std::string json_hi;
    epee::serialization::store_t_to_json(hi, json_hi);
    uint32_t str_len = static_cast<uint32_t>(json_hi.size());
    response_info.m_body.append(reinterpret_cast<const char*>(&str_len), sizeof(str_len));
    response_info.m_body.append(json_hi.data(), json_hi.size());
    m_core.get_blockchain_storage().copy_scratchpad_as_blob(response_info.m_body);    

    //TODO: remove this code
    LOG_PRINT_L0("[getfullscratchpad2]: json prefix len: " << str_len << ", JSON: " << json_hi);
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_get_addendums(const COMMAND_RPC_GET_ADDENDUMS::request& req, COMMAND_RPC_GET_ADDENDUMS::response& res, epee::json_rpc::error& error_resp, connection_context& cntx)
  {
    if (!check_core_ready())
    {
      res.status = CORE_RPC_STATUS_BUSY;
      return true;
    }

    if (!get_addendum_for_hi(req, res.addms))
    {
      res.status = "Fail at get_addendum_for_hi, check daemon logs for details";
      return true;
    }
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_reset_transaction_pool(const COMMAND_RPC_RESET_TX_POOL::request& req, COMMAND_RPC_RESET_TX_POOL::response& res, connection_context& cntx)
  {
    m_core.get_tx_pool().purge_transactions();
    res.status = CORE_RPC_STATUS_OK;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::on_validate_signed_text(const COMMAND_RPC_VALIDATE_SIGNED_TEXT::request& req, COMMAND_RPC_VALIDATE_SIGNED_TEXT::response& res, connection_context& cntx)
  {

    crypto::signature sig = AUTO_VAL_INIT(sig);
    if (!epee::string_tools::parse_tpod_from_hex_string(req.signature, sig))
    {
      LOG_PRINT_RED_L0("Failed to parse signature: " << req.signature);
      res.status = CORE_RPC_STATUS_INVALID_ARGUMENT;
      return true;
    }
     
    currency::account_public_address ac_adr = AUTO_VAL_INIT(ac_adr);
    currency::payment_id_t payment_id_stub;
    if (!tools::get_transfer_address_t(req.address, ac_adr, payment_id_stub, *this))
    {
      LOG_PRINT_RED_L0("Failed to parse address: " << req.address);
      res.status = CORE_RPC_STATUS_INVALID_ARGUMENT;
      return true;
    }

    crypto::hash h = currency::null_hash;
    crypto::cn_fast_hash(req.text.data(), req.text.size(), h);
    bool r = crypto::check_signature(h, ac_adr.m_spend_public_key, sig);
    if (!r)
    {
      res.status = CORE_RPC_STATUS_FAILED;
      LOG_PRINT_MAGENTA("Failed to validate signature" 
        << ENDL << "text: \"" << req.text << "\"(h=" << h << ")" 
        << ENDL << "address: " << ac_adr.m_spend_public_key
        << ENDL << "signature: " << epee::string_tools::pod_to_hex(sig), LOG_LEVEL_0);
    }      
    else
      res.status = CORE_RPC_STATUS_OK;

    return true;
  }
}
