// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include <boost/foreach.hpp>
#include "include_base_utils.h"
using namespace epee;

#include "core_rpc_server.h"
#include "common/command_line.h"
#include "currency_core/currency_format_utils.h"
#include "currency_core/account.h"
#include "currency_core/currency_basic_impl.h"
#include "misc_language.h"
#include "crypto/hash.h"
#include "core_rpc_server_error_codes.h"

namespace currency
{
  namespace
  {
    const command_line::arg_descriptor<std::string> arg_rpc_bind_ip   = {"rpc-bind-ip", "", "127.0.0.1"};
    const command_line::arg_descriptor<std::string> arg_rpc_bind_port = {"rpc-bind-port", "", std::to_string(RPC_DEFAULT_PORT)};
  }

  //-----------------------------------------------------------------------------------
  void core_rpc_server::init_options(boost::program_options::options_description& desc)
  {
    command_line::add_arg(desc, arg_rpc_bind_ip);
    command_line::add_arg(desc, arg_rpc_bind_port);
  }
  //------------------------------------------------------------------------------------------------------------------------------
  core_rpc_server::core_rpc_server(core& cr, nodetool::node_server<currency::t_currency_protocol_handler<currency::core> >& p2p):m_core(cr), m_p2p(p2p), m_session_counter(0)
  {}
  //------------------------------------------------------------------------------------------------------------------------------
  bool core_rpc_server::handle_command_line(const boost::program_options::variables_map& vm)
  {
    m_bind_ip = command_line::get_arg(vm, arg_rpc_bind_ip);
    m_port = command_line::get_arg(vm, arg_rpc_bind_port);
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

    if(!tvc.m_should_be_relayed)
    {
      LOG_PRINT_L0("[on_send_raw_tx]: tx accepted, but not relayed");
      res.status = "Not relayed";
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

    block b = AUTO_VAL_INIT(b);
    currency::blobdata blob_reserve = PROJECT_VERSION_LONG;
    blob_reserve.resize(blob_reserve.size() + 1 + req.reserve_size, 0);
    wide_difficulty_type dt = 0;
    if(!m_core.get_block_template(b, acc, dt, res.height, blob_reserve, req.dev_bounties_vote, ai))
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
    BOOST_FOREACH(const tx_out& out, blk.miner_tx.vout)
    {
      reward += out.amount;
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
    uint64_t block_height = boost::get<txin_gen>(blk.miner_tx.vin.front()).height;
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
    bool r = m_core.get_blockchain_storage().get_block_by_height(req.height, blk);
    
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
    bt_req.dev_bounties_vote = true; //set vote 
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
    //@#@
    //LOG_PRINT_L0("block_hash: " << get_block_hash(b));
    //LOG_PRINT_L0("block_hashing_blob:" << string_tools::buff_to_hex_nodelimer(currency::get_block_hashing_blob(b)));
    
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

}
