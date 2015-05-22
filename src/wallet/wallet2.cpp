// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include <boost/utility/value_init.hpp>
#include "include_base_utils.h"
using namespace epee;

#include "wallet2.h"
#include "currency_core/currency_format_utils.h"
#include "rpc/core_rpc_server_commands_defs.h"
#include "misc_language.h"
#include "currency_core/currency_basic_impl.h"
#include "common/boost_serialization_helper.h"
#include "crypto/crypto.h"
#include "serialization/binary_utils.h"
using namespace currency;

namespace tools
{
//----------------------------------------------------------------------------------------------------
void fill_transfer_details(const currency::transaction& tx, const tools::money_transfer2_details& td, tools::wallet_rpc::wallet_transfer_info_details& res_td)
{
  for (auto si : td.spent_indices)
  {
    CHECK_AND_ASSERT_MES(si < tx.vin.size(), void(), "Internal error: wrong tx transfer details: spend index=" << si << " is greater than transaction inputs vector " << tx.vin.size());
    CHECK_AND_ASSERT_MES(tx.vin[si].type() == typeid(currency::txin_to_key), void(), "Internal error: Wrong type of tx.vin[si]: " << tx.vin[si].type().name() << ", expected: currency::txin_to_key");
    res_td.spn.push_back(boost::get<currency::txin_to_key>(tx.vin[si]).amount);
  }

  for (auto ri : td.receive_indices)
  {
    CHECK_AND_ASSERT_MES(ri < tx.vout.size(), void(), "Internal error: wrong tx transfer details: reciev index=" << ri << " is greater than transaction outputs vector " << tx.vout.size());
    res_td.rcv.push_back(tx.vout[ri].amount);
  }
}
//----------------------------------------------------------------------------------------------------
void wallet2::init(const std::string& daemon_address)
{
  m_core_proxy->set_connection_addr(daemon_address);
}
//----------------------------------------------------------------------------------------------------
bool wallet2::set_core_proxy(const std::shared_ptr<i_core_proxy>& proxy)
{
  m_core_proxy = proxy;
  return true;
}
//----------------------------------------------------------------------------------------------------
std::shared_ptr<i_core_proxy> wallet2::get_core_proxy()
{
  return m_core_proxy;
}
//----------------------------------------------------------------------------------------------------
void wallet2::process_new_transaction(const currency::transaction& tx, uint64_t height, const currency::block& b)
{
  std::string recipient, recipient_alias, comment;
  process_unconfirmed(tx, recipient, recipient_alias, comment);
  std::vector<size_t> outs;
  uint64_t tx_money_got_in_outs = 0;
  crypto::public_key tx_pub_key = null_pkey;
  bool r = parse_and_validate_tx_extra(tx, tx_pub_key);
  CHECK_AND_THROW_WALLET_EX(!r, error::tx_extra_parse_error, tx);

  r = lookup_acc_outs(m_account.get_keys(), tx, tx_pub_key, outs, tx_money_got_in_outs);
  CHECK_AND_THROW_WALLET_EX(!r, error::acc_outs_lookup_error, tx, tx_pub_key, m_account.get_keys());

  money_transfer2_details mtd;
  if(!outs.empty() && tx_money_got_in_outs)
  {

    //good news - got money! take care about it
    //usually we have only one transfer for user in transaction
    currency::COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::request req = AUTO_VAL_INIT(req);
    currency::COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::response res = AUTO_VAL_INIT(res);
    req.txid = get_transaction_hash(tx);
    bool r = m_core_proxy->call_COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES(req, res);
    CHECK_AND_THROW_WALLET_EX(!r, error::no_connection_to_daemon, "get_o_indexes.bin");
    CHECK_AND_THROW_WALLET_EX(res.status == CORE_RPC_STATUS_BUSY, error::daemon_busy, "get_o_indexes.bin");
    CHECK_AND_THROW_WALLET_EX(res.status != CORE_RPC_STATUS_OK, error::get_out_indices_error, res.status);
    CHECK_AND_THROW_WALLET_EX(res.o_indexes.size() != tx.vout.size(), error::wallet_internal_error,
      "transactions outputs size=" + std::to_string(tx.vout.size()) +
      " not match with COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES response size=" + std::to_string(res.o_indexes.size()));

    BOOST_FOREACH(size_t o, outs)
    {
      CHECK_AND_THROW_WALLET_EX(tx.vout.size() <= o, error::wallet_internal_error, "wrong out in transaction: internal index=" +
        std::to_string(o) + ", total_outs=" + std::to_string(tx.vout.size()));

      mtd.receive_indices.push_back(o);

      m_transfers.push_back(boost::value_initialized<transfer_details>());
      transfer_details& td = m_transfers.back();
      td.m_block_height = height;
      td.m_block_timestamp = b.timestamp;
      td.m_internal_output_index = o;
      td.m_global_output_index = res.o_indexes[o];
      td.m_tx = tx;
      td.m_spent = false;
      
      currency::keypair in_ephemeral;
      currency::generate_key_image_helper(m_account.get_keys(), tx_pub_key, o, in_ephemeral, td.m_key_image);
      CHECK_AND_THROW_WALLET_EX(in_ephemeral.pub != boost::get<currency::txout_to_key>(tx.vout[o].target).key,
        error::wallet_internal_error, "key_image generated ephemeral public key not matched with output_key");

      m_key_images[td.m_key_image] = m_transfers.size()-1;
      LOG_PRINT_L0("Received money: " << print_money(td.amount()) << ", with tx: " << get_transaction_hash(tx));
      m_wcallback->on_money_received(height, td.m_tx, td.m_internal_output_index);
    }
  }
  
  uint64_t tx_money_spent_in_ins = 0;
  // check all outputs for spending (compare key images)
  size_t i = 0;
  BOOST_FOREACH(auto& in, tx.vin)
  {
    if (in.type() != typeid(currency::txin_to_key))
    {
      i++;
      continue;
    }
    auto it = m_key_images.find(boost::get<currency::txin_to_key>(in).k_image);
    if(it != m_key_images.end())
    {
      LOG_PRINT_L0("Spent money: " << print_money(boost::get<currency::txin_to_key>(in).amount) << ", with tx: " << get_transaction_hash(tx));
      tx_money_spent_in_ins += boost::get<currency::txin_to_key>(in).amount;
      transfer_details& td = m_transfers[it->second];
      td.m_spent = true;
      
      mtd.spent_indices.push_back(i);

      m_wcallback->on_money_spent(height, td.m_tx, td.m_internal_output_index, tx);
    }
    i++;
  }

    crypto::hash payment_id;
    if (tx_money_got_in_outs && get_payment_id_from_tx_extra(tx, payment_id))
    {
      uint64_t received = (tx_money_spent_in_ins < tx_money_got_in_outs) ? tx_money_got_in_outs - tx_money_spent_in_ins : 0;
      if (0 < received && null_hash != payment_id)
      {
        payment_details payment;
        payment.m_tx_hash      = currency::get_transaction_hash(tx);
        payment.m_amount       = received;
        payment.m_block_height = height;
        payment.m_unlock_time  = tx.unlock_time;
        m_payments.emplace(payment_id, payment);
        LOG_PRINT_L2("Payment found: " << payment_id << " / " << payment.m_tx_hash << " / " << payment.m_amount);
      }
    }

    if (tx_money_spent_in_ins)
    {//this actually is transfer transaction, notify about spend
      if (tx_money_spent_in_ins > tx_money_got_in_outs)
      {//usual transfer 
        handle_money_spent2(b, tx, tx_money_spent_in_ins - tx_money_got_in_outs, mtd, recipient, recipient_alias, comment);
      }
      else
      {//strange transfer, seems that in one transaction have transfers from different wallets.
        //LOG_PRINT_RED_L0("Unusual transaction " << currency::get_transaction_hash(tx) << ", tx_money_spent_in_ins: " << tx_money_spent_in_ins << ", tx_money_got_in_outs: " << tx_money_got_in_outs);
        //handle_money_spent2(b, tx, tx_money_spent_in_ins, mtd, recipient, recipient_alias);
        handle_money_received2(b, tx, tx_money_got_in_outs - tx_money_spent_in_ins, mtd);
      }
    }
    else
    {
      if(tx_money_got_in_outs)
        handle_money_received2(b, tx, tx_money_got_in_outs, mtd);
    }
}
//----------------------------------------------------------------------------------------------------
void wallet2::prepare_wti_decrypted_attachments(wallet_rpc::wallet_transfer_info& wti, const std::vector<currency::attachment_v>& decrypted_att)
{
  if (wti.is_income)
  {
    tx_payer tp = AUTO_VAL_INIT(tp);
    if (get_attachment(decrypted_att, tp))
      wti.remote_address = currency::get_account_address_as_str(tp.acc_addr);
  }
  currency::tx_comment cm;
  if (currency::get_attachment(decrypted_att, cm))
    wti.comment = cm.comment;
}
//------------
void wallet2::prepare_wti(wallet_rpc::wallet_transfer_info& wti, uint64_t height, uint64_t timestamp, const currency::transaction& tx, uint64_t amount, const money_transfer2_details& td)
{
  wti.tx = tx;
  wti.amount = amount;
  wti.height = height;
  crypto::hash pid;
  if(currency::get_payment_id_from_tx_extra(tx, pid))
    wti.payment_id = string_tools::pod_to_hex(pid);
  fill_transfer_details(tx, td, wti.td);
  wti.timestamp = timestamp;
  wti.fee = currency::is_coinbase(tx) ? 0:currency::get_tx_fee(tx);
  wti.unlock_time = tx.unlock_time;
  wti.tx_blob_size = static_cast<uint32_t>(currency::get_object_blobsize(wti.tx));
  wti.tx_hash = string_tools::pod_to_hex(currency::get_transaction_hash(tx));
  wti.is_service = currency::is_service_tx(tx);
}
//----------------------------------------------------------------------------------------------------
void wallet2::handle_money_received2(const currency::block& b, const currency::transaction& tx, uint64_t amount, const money_transfer2_details& td)
{
  //decrypt attachments
  std::vector<currency::attachment_v> decrypted_att;
  currency::decrypt_attachments(tx, m_account.get_keys(), decrypted_att);

  m_transfer_history.push_back(wallet_rpc::wallet_transfer_info());
  wallet_rpc::wallet_transfer_info& wti = m_transfer_history.back();
  wti.is_income = true;
  prepare_wti(wti, get_block_height(b), b.timestamp, tx, amount, td);
  prepare_wti_decrypted_attachments(wti, decrypted_att);

  m_wcallback->on_transfer2(wti);
}
//----------------------------------------------------------------------------------------------------
void wallet2::handle_money_spent2(const currency::block& b, 
                                  const currency::transaction& in_tx, 
                                  uint64_t amount, 
                                  const money_transfer2_details& td, 
                                  const std::string& recipient, 
                                  const std::string& recipient_alias, 
                                  const std::string& comment)
{
  m_transfer_history.push_back(wallet_rpc::wallet_transfer_info());
  wallet_rpc::wallet_transfer_info& wti = m_transfer_history.back();
  wti.is_income = false;
  wti.remote_address = recipient;
  wti.recipient_alias = recipient_alias;
  wti.comment = comment;
  prepare_wti(wti, get_block_height(b), b.timestamp, in_tx, amount, td);

  m_wcallback->on_transfer2(wti);
}
//----------------------------------------------------------------------------------------------------
void wallet2::process_unconfirmed(const currency::transaction& tx, std::string& recipient, std::string& recipient_alias, std::string& comment)
{
  auto unconf_it = m_unconfirmed_txs.find(get_transaction_hash(tx));
  if (unconf_it != m_unconfirmed_txs.end())
  {
    recipient = unconf_it->second.remote_address;
    recipient_alias = unconf_it->second.recipient_alias;
    comment = unconf_it->second.comment;
    m_unconfirmed_txs.erase(unconf_it);
  }
}
//----------------------------------------------------------------------------------------------------
void wallet2::process_new_blockchain_entry(const currency::block& b, currency::block_complete_entry& bche, const crypto::hash& bl_id, uint64_t height)
{
  //handle transactions from new block
  CHECK_AND_THROW_WALLET_EX(height != m_blockchain.size(), error::wallet_internal_error,
    "current_index=" + std::to_string(height) + ", m_blockchain.size()=" + std::to_string(m_blockchain.size()));

  //optimization: seeking only for blocks that are not older then the wallet creation time plus 1 day. 1 day is for possible user incorrect time setup
  if(b.timestamp + 60*60*24 > m_account.get_createtime())
  {
    TIME_MEASURE_START(miner_tx_handle_time);
    process_new_transaction(b.miner_tx, height, b);
    TIME_MEASURE_FINISH(miner_tx_handle_time);

    TIME_MEASURE_START(txs_handle_time);
    BOOST_FOREACH(auto& txblob, bche.txs)
    {
      currency::transaction tx;
      bool r = parse_and_validate_tx_from_blob(txblob, tx);
      CHECK_AND_THROW_WALLET_EX(!r, error::tx_parse_error, txblob);
      process_new_transaction(tx, height, b);
    }
    TIME_MEASURE_FINISH(txs_handle_time);
    LOG_PRINT_L2("Processed block: " << bl_id << ", height " << height << ", " <<  miner_tx_handle_time + txs_handle_time << "(" << miner_tx_handle_time << "/" << txs_handle_time <<")ms");
  }else
  {
    LOG_PRINT_L2( "Skipped block by timestamp, height: " << height << ", block time " << b.timestamp << ", account time " << m_account.get_createtime());
  }
  m_blockchain.push_back(bl_id);
  ++m_local_bc_height;
  m_last_bc_timestamp = b.timestamp;

  m_wcallback->on_new_block(height, b);
}
//----------------------------------------------------------------------------------------------------
void wallet2::get_short_chain_history(std::list<crypto::hash>& ids)
{
  size_t i = 0;
  size_t current_multiplier = 1;
  size_t sz = m_blockchain.size();
  if(!sz)
    return;
  size_t current_back_offset = 1;
  bool genesis_included = false;
  while(current_back_offset < sz)
  {
    ids.push_back(m_blockchain[sz-current_back_offset]);
    if(sz-current_back_offset == 0)
      genesis_included = true;
    if(i < 10)
    {
      ++current_back_offset;
    }else
    {
      current_back_offset += current_multiplier *= 2;
    }
    ++i;
  }
  if(!genesis_included)
    ids.push_back(m_blockchain[0]);
}
//----------------------------------------------------------------------------------------------------
void wallet2::pull_blocks(size_t& blocks_added, std::atomic<bool>& stop)
{
  blocks_added = 0;
  currency::COMMAND_RPC_GET_BLOCKS_FAST::request req = AUTO_VAL_INIT(req);
  currency::COMMAND_RPC_GET_BLOCKS_FAST::response res = AUTO_VAL_INIT(res);
  get_short_chain_history(req.block_ids);
  bool r = m_core_proxy->call_COMMAND_RPC_GET_BLOCKS_FAST(req, res);
  CHECK_AND_THROW_WALLET_EX(!r, error::no_connection_to_daemon, "getblocks.bin");
  CHECK_AND_THROW_WALLET_EX(res.status == CORE_RPC_STATUS_BUSY, error::daemon_busy, "getblocks.bin");
  CHECK_AND_THROW_WALLET_EX(res.status != CORE_RPC_STATUS_OK, error::get_blocks_error, res.status);
  CHECK_AND_THROW_WALLET_EX(m_blockchain.size() <= res.start_height, error::wallet_internal_error,
    "wrong daemon response: m_start_height=" + std::to_string(res.start_height) +
    " not less than local blockchain size=" + std::to_string(m_blockchain.size()));

  size_t current_index = res.start_height;
  BOOST_FOREACH(auto& bl_entry, res.blocks)
  {
    if (stop)
      break;

    currency::block bl;
    r = currency::parse_and_validate_block_from_blob(bl_entry.block, bl);
    CHECK_AND_THROW_WALLET_EX(!r, error::block_parse_error, bl_entry.block);

    crypto::hash bl_id = get_block_hash(bl);
    if(current_index >= m_blockchain.size())
    {
      process_new_blockchain_entry(bl, bl_entry, bl_id, current_index);
      ++blocks_added;
    }
    else if(bl_id != m_blockchain[current_index])
    {
      //split detected here !!!
      CHECK_AND_THROW_WALLET_EX(current_index == res.start_height, error::wallet_internal_error,
        "wrong daemon response: split starts from the first block in response " + string_tools::pod_to_hex(bl_id) + 
        " (height " + std::to_string(res.start_height) + "), local block id at this height: " +
        string_tools::pod_to_hex(m_blockchain[current_index]));

      detach_blockchain(current_index);
      process_new_blockchain_entry(bl, bl_entry, bl_id, current_index);
    }
    else
    {
      LOG_PRINT_L2("Block is already in blockchain: " << string_tools::pod_to_hex(bl_id));
    }

    ++current_index;
    if (res.current_height > height_of_start_sync)
    {
      uint64_t next_percent = (100 * (current_index - height_of_start_sync)) / (res.current_height - height_of_start_sync);
      if (next_percent != last_sync_percent)
      {
        m_wcallback->on_sync_progress(next_percent);
        last_sync_percent = next_percent;
      }
    }
  }
}
//----------------------------------------------------------------------------------------------------
void wallet2::refresh()
{
  size_t blocks_fetched = 0;
  refresh(blocks_fetched);
}
//----------------------------------------------------------------------------------------------------
void wallet2::refresh(size_t & blocks_fetched)
{
  bool received_money = false;
  refresh(blocks_fetched, received_money, m_stop);
}
//----------------------------------------------------------------------------------------------------
void wallet2::refresh(std::atomic<bool>& stop)
{
  bool f;
  size_t n;
  refresh(n, f, stop);
}
//----------------------------------------------------------------------------------------------------
void wallet2::update_current_tx_limit()
{
  currency::COMMAND_RPC_GET_INFO::request req = AUTO_VAL_INIT(req);
  currency::COMMAND_RPC_GET_INFO::response res = AUTO_VAL_INIT(res);
  bool r = m_core_proxy->call_COMMAND_RPC_GET_INFO(req, res);
  CHECK_AND_THROW_WALLET_EX(!r, error::no_connection_to_daemon, "getinfo");
  CHECK_AND_THROW_WALLET_EX(res.status == CORE_RPC_STATUS_BUSY, error::daemon_busy, "getinfo");
  CHECK_AND_THROW_WALLET_EX(res.status != CORE_RPC_STATUS_OK, error::get_blocks_error, res.status);
  CHECK_AND_THROW_WALLET_EX(res.current_blocks_median < CURRENCY_BLOCK_GRANTED_FULL_REWARD_ZONE, error::get_blocks_error, "bad median size");
  m_upper_transaction_size_limit = res.current_blocks_median - CURRENCY_COINBASE_BLOB_RESERVED_SIZE;
}
//----------------------------------------------------------------------------------------------------
void wallet2::scan_tx_pool()
{
  //get transaction pool content 
  currency::COMMAND_RPC_GET_TX_POOL::request req = AUTO_VAL_INIT(req);
  currency::COMMAND_RPC_GET_TX_POOL::response res = AUTO_VAL_INIT(res);
  bool r = m_core_proxy->call_COMMAND_RPC_GET_TX_POOL(req, res);
  if (res.status == CORE_RPC_STATUS_BUSY)
    throw error::daemon_busy("", "get_tx_pool");
  CHECK_AND_THROW_WALLET_EX(!r, error::no_connection_to_daemon, "get_tx_pool");
  CHECK_AND_THROW_WALLET_EX(res.status == CORE_RPC_STATUS_BUSY, error::daemon_busy, "get_tx_pool");
  CHECK_AND_THROW_WALLET_EX(res.status != CORE_RPC_STATUS_OK, error::get_blocks_error, res.status);
  
  std::unordered_map<crypto::hash, currency::transaction> unconfirmed_in_transfers_local(std::move(m_unconfirmed_in_transfers));

  for (const auto &tx_blob : res.txs)
  {
    currency::transaction tx;
    bool r = parse_and_validate_tx_from_blob(tx_blob, tx);
    CHECK_AND_THROW_WALLET_EX(!r, error::tx_parse_error, tx_blob);
    crypto::hash tx_hash = currency::get_transaction_hash(tx);
    auto it = unconfirmed_in_transfers_local.find(tx_hash);
    if (it != unconfirmed_in_transfers_local.end())
    {
      m_unconfirmed_in_transfers.insert(*it);
      continue;
    }

    // read extra
    std::vector<size_t> outs;
    uint64_t tx_money_got_in_outs = 0;
    crypto::public_key tx_pub_key = null_pkey;
    r = parse_and_validate_tx_extra(tx, tx_pub_key);
    CHECK_AND_THROW_WALLET_EX(!r, error::tx_extra_parse_error, tx);
    //check if we have money
    r = lookup_acc_outs(m_account.get_keys(), tx, tx_pub_key, outs, tx_money_got_in_outs);
    CHECK_AND_THROW_WALLET_EX(!r, error::acc_outs_lookup_error, tx, tx_pub_key, m_account.get_keys());
    //check if we have spendings
    uint64_t tx_money_spent_in_ins = 0;
    // check all outputs for spending (compare key images)
    for(auto& in: tx.vin)
    {
      if (in.type() != typeid(currency::txin_to_key))
        continue;
      if(m_key_images.count(boost::get<currency::txin_to_key>(in).k_image))
        tx_money_spent_in_ins += boost::get<currency::txin_to_key>(in).amount;
    }

    if (!tx_money_spent_in_ins && tx_money_got_in_outs)
    {
      
      std::vector<currency::attachment_v> decrypted_att;
      decrypt_attachments(tx, m_account.get_keys(), decrypted_att);

      //prepare notification about pending transaction
      wallet_rpc::wallet_transfer_info unconfirmed_wti = m_unconfirmed_txs[currency::get_transaction_hash(tx)];

      unconfirmed_wti.timestamp = 
      unconfirmed_wti.is_income = true;
      m_unconfirmed_in_transfers[tx_hash] = tx;
      prepare_wti(unconfirmed_wti, 0, time(NULL), tx, tx_money_got_in_outs, money_transfer2_details());
      prepare_wti_decrypted_attachments(unconfirmed_wti, decrypted_att);

      m_wcallback->on_transfer2(unconfirmed_wti);
    }
  }
}
//----------------------------------------------------------------------------------------------------
void wallet2::refresh(size_t & blocks_fetched, bool& received_money, std::atomic<bool>& stop)
{
  received_money = false;
  blocks_fetched = 0;
  size_t added_blocks = 0;
  size_t try_count = 0;
  crypto::hash last_tx_hash_id = m_transfers.size() ? get_transaction_hash(m_transfers.back().m_tx) : null_hash;
  height_of_start_sync = m_blockchain.size();
  last_sync_percent = 0;
  while (!stop.load(std::memory_order_relaxed))
  {
    try
    {
      pull_blocks(added_blocks, stop);
      blocks_fetched += added_blocks;
      if(!added_blocks)
        break;
    }
    catch (const std::exception& e)
    {
      blocks_fetched += added_blocks;
      if(try_count < 3)
      {
        LOG_PRINT_L1("Another try pull_blocks (try_count=" << try_count << "), exception: " << e.what());
        ++try_count;
      }
      else
      {
        LOG_ERROR("pull_blocks failed, try_count=" << try_count << ", exception: " << e.what());
        throw;
      }
    }
  }
  if(last_tx_hash_id != (m_transfers.size() ? get_transaction_hash(m_transfers.back().m_tx) : null_hash))
    received_money = true;

  LOG_PRINT_L1("Refresh done, blocks received: " << blocks_fetched << ", balance: " << print_money(balance()) << ", unlocked: " << print_money(unlocked_balance()));
}
//----------------------------------------------------------------------------------------------------
bool wallet2::refresh(size_t & blocks_fetched, bool& received_money, bool& ok, std::atomic<bool>& stop)
{
  try
  {
    refresh(blocks_fetched, received_money, stop);
    ok = true;
  }
  catch (...)
  {
    ok = false;
  }
  return ok;
}
//----------------------------------------------------------------------------------------------------
void wallet2::detach_blockchain(uint64_t height)
{
  LOG_PRINT_L0("Detaching blockchain on height " << height);
  size_t transfers_detached = 0;

  auto it = std::find_if(m_transfers.begin(), m_transfers.end(), [&](const transfer_details& td){return td.m_block_height >= height;});
  size_t i_start = it - m_transfers.begin();

  for(size_t i = i_start; i!= m_transfers.size();i++)
  {
    auto it_ki = m_key_images.find(m_transfers[i].m_key_image);
    CHECK_AND_THROW_WALLET_EX(it_ki == m_key_images.end(), error::wallet_internal_error, "key image not found");
    m_key_images.erase(it_ki);
    ++transfers_detached;
  }
  m_transfers.erase(it, m_transfers.end());

  size_t blocks_detached = m_blockchain.end() - (m_blockchain.begin()+height);
  m_blockchain.erase(m_blockchain.begin()+height, m_blockchain.end());
  m_local_bc_height -= blocks_detached;

  for (auto it = m_payments.begin(); it != m_payments.end(); )
  {
    if(height <= it->second.m_block_height)
      it = m_payments.erase(it);
    else
      ++it;
  }

  LOG_PRINT_L0("Detached blockchain on height " << height << ", transfers detached " << transfers_detached << ", blocks detached " << blocks_detached);
}
//----------------------------------------------------------------------------------------------------
bool wallet2::deinit()
{
  return true;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::clear()
{
  m_blockchain.clear();
  m_transfers.clear();
  m_key_images.clear();
  m_unconfirmed_txs.clear();
  m_payments.clear();
  m_transfer_history.clear();

  currency::block b;
  currency::generate_genesis_block(b);
  m_blockchain.push_back(get_block_hash(b));
  m_local_bc_height = 1;
  return true;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::store_keys(std::string& buff, const std::string& password)
{
  std::string account_data;
  bool r = epee::serialization::store_t_to_binary(m_account, account_data);
  CHECK_AND_ASSERT_MES(r, false, "failed to serialize wallet keys");
  wallet2::keys_file_data keys_file_data = boost::value_initialized<wallet2::keys_file_data>();

  crypto::chacha8_key key;
  crypto::generate_chacha8_key(password, key);
  std::string cipher;
  cipher.resize(account_data.size());
  keys_file_data.iv = crypto::rand<crypto::chacha8_iv>();
  crypto::chacha8(account_data.data(), account_data.size(), key, keys_file_data.iv, &cipher[0]);
  keys_file_data.account_data = cipher;

  r = ::serialization::dump_binary(keys_file_data, buff);

  return true;
}
//----------------------------------------------------------------------------------------------------
namespace
{
  bool verify_keys(const crypto::secret_key& sec, const crypto::public_key& expected_pub)
  {
    crypto::public_key pub;
    bool r = crypto::secret_key_to_public_key(sec, pub);
    return r && expected_pub == pub;
  }
}
//----------------------------------------------------------------------------------------------------
void wallet2::load_keys(const std::string& buff, const std::string& password)
{
  wallet2::keys_file_data keys_file_data;
//  std::string buf;
//  bool r = epee::file_io_utils::load_file_to_string(keys_file_name, buf);
//  CHECK_AND_THROW_WALLET_EX(!r, error::file_read_error, keys_file_name);
  bool r = ::serialization::parse_binary(buff, keys_file_data);
  CHECK_AND_THROW_WALLET_EX(!r, error::wallet_internal_error, "internal error: failed to deserialize");

  crypto::chacha8_key key;
  crypto::generate_chacha8_key(password, key);
  std::string account_data;
  account_data.resize(keys_file_data.account_data.size());
  crypto::chacha8(keys_file_data.account_data.data(), keys_file_data.account_data.size(), key, keys_file_data.iv, &account_data[0]);

  const currency::account_keys& keys = m_account.get_keys();
  r = epee::serialization::load_t_from_binary(m_account, account_data);
  r = r && verify_keys(keys.m_view_secret_key,  keys.m_account_address.m_view_public_key);
  r = r && verify_keys(keys.m_spend_secret_key, keys.m_account_address.m_spend_public_key);
  CHECK_AND_THROW_WALLET_EX(!r, error::invalid_password);
}
//----------------------------------------------------------------------------------------------------
void wallet2::assign_account(const currency::account_base& acc)
{
  clear();
  m_account = acc;
  m_account_public_address = m_account.get_keys().m_account_address;
}
//----------------------------------------------------------------------------------------------------
void wallet2::generate(const std::string& wallet_, const std::string& password)
{
  clear();
  m_wallet_file = wallet_;
  m_password = password;

  boost::system::error_code ignored_ec;
  CHECK_AND_THROW_WALLET_EX(boost::filesystem::exists(m_wallet_file, ignored_ec), error::file_exists, m_wallet_file);

  m_account.generate();
  m_account_public_address = m_account.get_keys().m_account_address;

  bool r = file_io_utils::save_string_to_file(m_wallet_file + ".address.txt", m_account.get_public_address_str() );
  if(!r) LOG_PRINT_RED_L0("String with address text not saved");

  store();
}
//----------------------------------------------------------------------------------------------------
bool wallet2::check_connection()
{
  return m_core_proxy->check_connection();
}
//----------------------------------------------------------------------------------------------------
void wallet2::load(const std::string& wallet_, const std::string& password)
{
  clear();
  m_wallet_file = wallet_;
  m_password = password;

  std::string keys_buff;
  std::string body_buff;


  boost::system::error_code e;
  bool exists = boost::filesystem::exists(m_wallet_file, e);
  CHECK_AND_THROW_WALLET_EX(e || !exists, error::file_not_found, m_wallet_file);

  std::ifstream data_file;
  data_file.open(m_wallet_file, std::ios_base::binary | std::ios_base::in);
  CHECK_AND_THROW_WALLET_EX(data_file.fail(), error::file_not_found, m_wallet_file);

  wallet_file_binary_header wbh = AUTO_VAL_INIT(wbh);
  //wbh//WALLET_FILE_SIGNATURE;


  data_file.read((char*)&wbh, sizeof(wbh));
  CHECK_AND_THROW_WALLET_EX(data_file.fail(), error::file_not_found, m_wallet_file);

  CHECK_AND_THROW_WALLET_EX(wbh.m_signature != WALLET_FILE_SIGNATURE, error::file_not_found, m_wallet_file);
  CHECK_AND_THROW_WALLET_EX(wbh.m_cb_body > WALLET_FILE_MAX_BODY_SIZE || 
    wbh.m_cb_keys > WALLET_FILE_MAX_KEYS_SIZE, error::file_not_found, m_wallet_file);


  keys_buff.resize(wbh.m_cb_keys);
  data_file.read((char*)keys_buff.data(), wbh.m_cb_keys);

  load_keys(keys_buff, password);
  LOG_PRINT_L0("Loaded wallet keys file, with public address: " << m_account.get_public_address_str());

  bool r = tools::portable_unserialize_obj_from_stream(*this, data_file);
//CHECK_AND_THROW_WALLET_EX(!r, error::file_read_error, m_wallet_file);
//   CHECK_AND_THROW_WALLET_EX(
//     m_account_public_address.m_spend_public_key != m_account.get_keys().m_account_address.m_spend_public_key ||
//     m_account_public_address.m_view_public_key  != m_account.get_keys().m_account_address.m_view_public_key,
//     error::wallet_files_doesnt_correspond, m_wallet_file);

  if (!r)
  {
    reset_history();
  }
  m_local_bc_height = m_blockchain.size();
  CHECK_AND_THROW_WALLET_EX(!r, error::wallet_load_notice_wallet_restored, m_wallet_file);
}
//----------------------------------------------------------------------------------------------------
void wallet2::store()
{
  //prepare data
  std::string keys_buff;
  bool r = store_keys(keys_buff, m_password);
  CHECK_AND_THROW_WALLET_EX(!r, error::file_save_error, m_wallet_file);

  wallet_file_binary_header wbh = AUTO_VAL_INIT(wbh);

  std::stringstream ss;
  r = tools::portble_serialize_obj_to_stream(*this, ss);
  CHECK_AND_THROW_WALLET_EX(!r, error::file_save_error, m_wallet_file);
  std::string body_buff = ss.str();

  //store data

  wbh.m_signature = WALLET_FILE_SIGNATURE;
  wbh.m_cb_keys = keys_buff.size();
  wbh.m_cb_body = body_buff.size();

  std::string header_buff((const char*)&wbh, sizeof(wbh));


  std::ofstream data_file;
  data_file.open(m_wallet_file, std::ios_base::binary | std::ios_base::out | std::ios::trunc);
  CHECK_AND_THROW_WALLET_EX(data_file.fail(), error::file_save_error, m_wallet_file);
  data_file << header_buff << keys_buff << body_buff;
  data_file.flush();
  data_file.close();
}
//----------------------------------------------------------------------------------------------------
uint64_t wallet2::unlocked_balance()
{
  uint64_t amount = 0;
  BOOST_FOREACH(transfer_details& td, m_transfers)
    if(!td.m_spent && is_transfer_unlocked(td))
      amount += td.amount();

  return amount;
}
//----------------------------------------------------------------------------------------------------
uint64_t wallet2::balance()
{
  uint64_t amount = 0;
  BOOST_FOREACH(auto& td, m_transfers)
    if(!td.m_spent)
      amount += td.amount();


  BOOST_FOREACH(auto& utx, m_unconfirmed_txs)
  if (utx.second.is_income)
      amount+= utx.second.amount;

  return amount;
}
//----------------------------------------------------------------------------------------------------
void wallet2::get_transfers(wallet2::transfer_container& incoming_transfers) const
{
  incoming_transfers = m_transfers;
}
//----------------------------------------------------------------------------------------------------
void wallet2::get_payments(const crypto::hash& payment_id, std::list<wallet2::payment_details>& payments) const
{
  auto range = m_payments.equal_range(payment_id);
  std::for_each(range.first, range.second, [&payments](const payment_container::value_type& x) {
    payments.push_back(x.second);
  });
}
//----------------------------------------------------------------------------------------------------
void wallet2::get_recent_transfers_history(std::vector<wallet_rpc::wallet_transfer_info>& trs, size_t offset, size_t count)
{
  if (offset >= m_transfer_history.size())
    return;

  auto start = m_transfer_history.rbegin() + offset;
  auto stop = m_transfer_history.size() - offset >= count ? start + count : m_transfer_history.rend();

  trs.insert(trs.end(), start, stop);
}
//----------------------------------------------------------------------------------------------------
bool wallet2::get_transfer_address(const std::string& adr_str, currency::account_public_address& addr)
{
  return m_core_proxy->get_transfer_address(adr_str, addr);
}
//----------------------------------------------------------------------------------------------------
bool wallet2::is_coin_age_okay(const transfer_details& tr)
{
  //blockchain conditions
  if (!is_transfer_unlocked(tr))
    return false;

  //TODO: add extra coinage check here
  //@#@
  return true;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::get_pos_entries(currency::COMMAND_RPC_SCAN_POS::request& req)
{
  for (size_t i = 0; i != m_transfers.size(); i++)
  {
    auto& tr = m_transfers[i];
    if (tr.m_spent)
      continue;
    if (!is_coin_age_okay(tr))
      continue;
    currency::pos_entry pe = AUTO_VAL_INIT(pe);
    pe.amount = tr.amount();
    pe.index = tr.m_global_output_index;
    pe.keyimage = tr.m_key_image;
    pe.wallet_index = i;
    pe.block_timestamp = tr.m_block_timestamp;
    req.pos_entries.push_back(pe);
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::prepare_and_sign_pos_block(currency::block& b, 
                                         const currency::pos_entry& pos_info, 
                                         const crypto::public_key& source_tx_pub_key, 
                                         uint64_t in_tx_output_index, 
                                         const std::vector<const crypto::public_key*>& keys_ptrs)
{
  //generate coinbase transaction
  CHECK_AND_ASSERT_MES(b.miner_tx.vin[0].type() == typeid(currency::txin_gen), false, "Wrong output input in transaction");
  CHECK_AND_ASSERT_MES(b.miner_tx.vin[1].type() == typeid(currency::txin_to_key), false, "Wrong output input in transaction");
  auto& txin = boost::get<currency::txin_to_key>(b.miner_tx.vin[1]);
  txin.k_image = pos_info.keyimage;
  CHECK_AND_ASSERT_MES(b.miner_tx.signatures.size() == 1 && b.miner_tx.signatures[0].size() == txin.key_offsets.size(),
    false, "Wrong signatures amount in coinbase transacton");



  //derive secrete key
  crypto::key_derivation pos_coin_derivation = AUTO_VAL_INIT(pos_coin_derivation);
  bool r = crypto::generate_key_derivation(source_tx_pub_key,
    m_account.get_keys().m_view_secret_key,
    pos_coin_derivation);

  CHECK_AND_ASSERT_MES(r, false, "internal error: pos coin base generator: failed to generate_key_derivation("
    <<  source_tx_pub_key
    << ", view secret key: " << m_account.get_keys().m_view_secret_key << ")");

  crypto::secret_key derived_secrete_ephemeral_key = AUTO_VAL_INIT(derived_secrete_ephemeral_key);
  crypto::derive_secret_key(pos_coin_derivation,
    in_tx_output_index,
    m_account.get_keys().m_spend_secret_key,
    derived_secrete_ephemeral_key);

  // sign block actually in coinbase transaction
  crypto::hash block_hash = currency::get_block_hash(b);

  crypto::generate_ring_signature(block_hash,
    txin.k_image,
    keys_ptrs,
    derived_secrete_ephemeral_key,
    0,
    &b.miner_tx.signatures[0][0]);

  LOG_PRINT_L4("GENERATED RING SIGNATURE: block_id " << block_hash
    << "txin.k_image" << txin.k_image
    << "key_ptr:" << *keys_ptrs[0]
    << "signature:" << b.miner_tx.signatures[0][0]);

  return true;
}
//------------------------------------------------------------------
bool wallet2::build_kernel(const pos_entry& pe, const stake_modifier_type& stake_modifier, stake_kernel& kernel, uint64_t& coindays_weight, uint64_t timestamp)
{
  PROFILE_FUNC("build_kernel");
  coindays_weight = 0;
  kernel = stake_kernel();
  kernel.kimage = pe.keyimage;
  kernel.stake_modifier = stake_modifier;
  kernel.block_timestamp = timestamp;

  coindays_weight = get_coinday_weight(pe.amount);
  return true;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::fill_mining_context(mining_context& ctx)
{
  bool r = get_pos_entries(ctx.sp);
  CHECK_AND_ASSERT_MES(r, false, "Failed to get_pos_entries()");

  currency::COMMAND_RPC_GET_POS_MINING_DETAILS::request pos_details_req = AUTO_VAL_INIT(pos_details_req);
  currency::COMMAND_RPC_GET_POS_MINING_DETAILS::response pos_details_resp = AUTO_VAL_INIT(pos_details_resp);
  ctx.rsp.status = CORE_RPC_STATUS_NOT_FOUND;
  m_core_proxy->call_COMMAND_RPC_GET_POS_MINING_DETAILS(pos_details_req, pos_details_resp);
  if (pos_details_resp.status != CORE_RPC_STATUS_OK)
    return false;
  ctx.basic_diff.assign(pos_details_resp.pos_basic_difficulty);
  ctx.sm = pos_details_resp.sm;
  ctx.rsp.height = pos_details_resp.height;
  ctx.rsp.status = CORE_RPC_STATUS_OK;
  return true;
}
//------------------------------------------------------------------
bool wallet2::try_mint_pos()
{
  mining_context ctx = AUTO_VAL_INIT(ctx);
  LOG_PRINT_L0("Starting PoS mint iteration");
  fill_mining_context(ctx);
  LOG_PRINT_L0("POS_ENTRIES: " << ctx.sp.pos_entries.size());

  std::atomic<bool> stop(false);
  scan_pos(ctx, stop, [this](){
    size_t blocks_fetched;
    refresh(blocks_fetched);
    if (blocks_fetched)
    {
      LOG_PRINT_L0("Detected new block, minting interrupted");
      return false;
    }
    return true;
  });
  if (ctx.rsp.status == CORE_RPC_STATUS_OK)
  {
    build_minted_block(ctx.sp, ctx.rsp);
  }
  LOG_PRINT_L0("PoS mint iteration finished(" << ctx.rsp.status << ")");

  return true;
}
//-------------------------------
bool wallet2::reset_history()
{
  std::string pass = m_password;
  std::string file_path = m_wallet_file;
  account_base acc_tmp = m_account;
  clear();
  m_account = acc_tmp;
  m_password = pass;
  m_wallet_file = file_path;
  return true;
}
//-------------------------------
bool wallet2::build_minted_block(const currency::COMMAND_RPC_SCAN_POS::request& req, 
                                 const currency::COMMAND_RPC_SCAN_POS::response& rsp)
{
    //found a block, construct it, sign and push to daemon
    LOG_PRINT_GREEN("Found kernel, constructing block", LOG_LEVEL_1);

    CHECK_AND_NO_ASSERT_MES(rsp.index < req.pos_entries.size(), false, "call_COMMAND_RPC_SCAN_POS returned wrong index: " << rsp.index << ", expected less then " << req.pos_entries.size());

    currency::COMMAND_RPC_GETBLOCKTEMPLATE::request tmpl_req = AUTO_VAL_INIT(tmpl_req);
    currency::COMMAND_RPC_GETBLOCKTEMPLATE::response tmpl_rsp = AUTO_VAL_INIT(tmpl_rsp);
    tmpl_req.wallet_address = m_account.get_public_address_str();
    tmpl_req.pos_block = true;
    tmpl_req.pos_amount = req.pos_entries[rsp.index].amount;
    tmpl_req.pos_index = req.pos_entries[rsp.index].index;
    m_core_proxy->call_COMMAND_RPC_GETBLOCKTEMPLATE(tmpl_req, tmpl_rsp);
    CHECK_AND_ASSERT_MES(tmpl_rsp.status == CORE_RPC_STATUS_OK, false, "Failed to create block template after kernel hash found!");

    currency::block b = AUTO_VAL_INIT(b);
    currency::blobdata block_blob;
    bool res = epee::string_tools::parse_hexstr_to_binbuff(tmpl_rsp.blocktemplate_blob, block_blob);
    CHECK_AND_ASSERT_MES(res, false, "Failed to create block template after kernel hash found!");
    res = parse_and_validate_block_from_blob(block_blob, b);
    CHECK_AND_ASSERT_MES(res, false, "Failed to create block template after kernel hash found!");

    if (rsp.height != currency::get_block_height(b))
    {
      LOG_PRINT_YELLOW("Found kernel but block is behindhand", LOG_LEVEL_0);
      return false;
    }

    std::vector<const crypto::public_key*> keys_ptrs;
    CHECK_AND_ASSERT_MES(req.pos_entries[rsp.index].wallet_index < m_transfers.size(),
        false, "Wrong wallet_index at generating coinbase transacton");

    const auto& target = m_transfers[req.pos_entries[rsp.index].wallet_index].m_tx.vout[m_transfers[req.pos_entries[rsp.index].wallet_index].m_internal_output_index].target;
    CHECK_AND_ASSERT_MES(target.type() == typeid(currency::txout_to_key), false, "wrong type_id in source transaction in coinbase tx");

    const currency::txout_to_key& txtokey = boost::get<currency::txout_to_key>(target);
    keys_ptrs.push_back(&txtokey.key);

    b.timestamp = rsp.block_timestamp;

    res = prepare_and_sign_pos_block(b,
      req.pos_entries[rsp.index],
      get_tx_pub_key_from_extra(m_transfers[req.pos_entries[rsp.index].wallet_index].m_tx),
      m_transfers[req.pos_entries[rsp.index].wallet_index].m_internal_output_index,
      keys_ptrs);
    CHECK_AND_ASSERT_MES(res, false, "Failed to prepare_and_sign_pos_block");
    
    LOG_PRINT_GREEN("Block constructed, sending to core...", LOG_LEVEL_1);

    currency::COMMAND_RPC_SUBMITBLOCK::request subm_req = AUTO_VAL_INIT(subm_req);
    currency::COMMAND_RPC_SUBMITBLOCK::response subm_rsp = AUTO_VAL_INIT(subm_rsp);
    subm_req.push_back(epee::string_tools::buff_to_hex_nodelimer(currency::t_serializable_object_to_blob(b)));
    m_core_proxy->call_COMMAND_RPC_SUBMITBLOCK(subm_req, subm_rsp);
    if (subm_rsp.status != CORE_RPC_STATUS_OK)
    {
      LOG_ERROR("Constructed block is not accepted by core, status: " << subm_rsp.status);
      return false;
    }    
    LOG_PRINT_GREEN("POS block generated and accepted, congrats!", LOG_LEVEL_0);
    m_wcallback->on_pos_block_found(b);
    return true;
}

//----------------------------------------------------------------------------------------------------
void wallet2::get_unconfirmed_transfers(std::vector<wallet_rpc::wallet_transfer_info>& trs)
{
  for (auto& u : m_unconfirmed_txs)
    trs.push_back(u.second);
}
//----------------------------------------------------------------------------------------------------
bool wallet2::is_transfer_unlocked(const transfer_details& td) const
{
  if(!is_tx_spendtime_unlocked(td.m_tx.unlock_time))
    return false;

  if(td.m_block_height + WALLET_DEFAULT_TX_SPENDABLE_AGE > m_blockchain.size())
    return false;

  return true;
}
//----------------------------------------------------------------------------------------------------
bool wallet2::is_tx_spendtime_unlocked(uint64_t unlock_time) const
{
  if(unlock_time < CURRENCY_MAX_BLOCK_NUMBER)
  {
    //interpret as block index
    if(m_blockchain.size()-1 + CURRENCY_LOCKED_TX_ALLOWED_DELTA_BLOCKS >= unlock_time)
      return true;
    else
      return false;
  }else
  {
    //interpret as time
    uint64_t current_time = static_cast<uint64_t>(time(NULL));
    if(current_time + CURRENCY_LOCKED_TX_ALLOWED_DELTA_SECONDS >= unlock_time)
      return true;
    else
      return false;
  }
  return false;
}
//----------------------------------------------------------------------------------------------------
namespace
{
  template<typename T>
  T pop_random_value(std::vector<T>& vec)
  {
    CHECK_AND_ASSERT_MES(!vec.empty(), T(), "Vector must be non-empty");

    size_t idx = crypto::rand<size_t>() % vec.size();
    T res = vec[idx];
    if (idx + 1 != vec.size())
    {
      vec[idx] = vec.back();
    }
    vec.resize(vec.size() - 1);

    return res;
  }
}
//----------------------------------------------------------------------------------------------------
void wallet2::push_offer(const currency::offer_details& od, currency::transaction& res_tx)
{
  currency::tx_destination_entry tx_dest;
  tx_dest.addr = m_account.get_keys().m_account_address;
  tx_dest.amount = DEFAULT_FEE;
  std::vector<currency::tx_destination_entry> destinations;
  std::vector<currency::extra_v> extra;
  std::vector<currency::attachment_v> attachments;
  attachments.push_back(od);
  destinations.push_back(tx_dest);
  transfer(destinations, 0, 0, DEFAULT_FEE, extra, attachments, res_tx);
}
//----------------------------------------------------------------------------------------------------
void wallet2::request_alias_registration(const currency::alias_info& ai, currency::transaction& res_tx)
{
  currency::tx_destination_entry tx_dest;
  tx_dest.addr = m_account.get_keys().m_account_address;
  tx_dest.amount = DEFAULT_FEE;
  std::vector<currency::tx_destination_entry> destinations;
  std::vector<currency::extra_v> extra;
  std::vector<currency::attachment_v> attachments;
  currency::extra_alias_entry eae;
  currency::make_tx_extra_alias_entry(eae.buff, ai, false);
  extra.push_back(eae);

  destinations.push_back(tx_dest);
  transfer(destinations, 0, 0, DEFAULT_FEE, extra, attachments, res_tx);
}
//----------------------------------------------------------------------------------------------------
uint64_t wallet2::select_indices_for_transfer(std::list<size_t>& selected_indexes, std::map<uint64_t, std::list<size_t> >& found_free_amounts, uint64_t needed_money)
{
  uint64_t found_money = 0;
  while(found_money < needed_money && found_free_amounts.size())
  {
    auto it = found_free_amounts.lower_bound(needed_money - found_money);
    if(it != found_free_amounts.end() && it->second.size() )
    {      
      found_money += it->first;
      selected_indexes.push_back(it->second.back());
      break;
    }else
    {
      it = --found_free_amounts.end();
      CHECK_AND_ASSERT_MES(it->second.size(), 0, "internal error: empty found_free_amounts map");
      found_money += it->first;
      selected_indexes.push_back(it->second.back());

      it->second.pop_back();
      if(!it->second.size())
        found_free_amounts.erase(it);
    }
  }
  return found_money;
}
//----------------------------------------------------------------------------------------------------
uint64_t wallet2::select_transfers(uint64_t needed_money, size_t fake_outputs_count, uint64_t dust, std::list<transfer_container::iterator>& selected_transfers)
{
  std::map<uint64_t, std::list<size_t> > found_free_amounts;

  for (size_t i = 0; i < m_transfers.size(); ++i)
  {
    const transfer_details& td = m_transfers[i];
    if (!td.m_spent && is_transfer_unlocked(td) && 
      currency::is_mixattr_applicable_for_fake_outs_counter(boost::get<currency::txout_to_key>(td.m_tx.vout[td.m_internal_output_index].target).mix_attr, fake_outputs_count))
    {
      found_free_amounts[td.m_tx.vout[td.m_internal_output_index].amount].push_back(i);
    }
  }

  std::list<size_t> selected_indexes;
  uint64_t found_money = select_indices_for_transfer(selected_indexes, found_free_amounts, needed_money);
  for(auto i: selected_indexes)
    selected_transfers.push_back(m_transfers.begin() + i);
  
  return found_money;

  /*
  std::vector<size_t> unused_transfers_indices;
  std::vector<size_t> unused_dust_indices;
  for (size_t i = 0; i < m_transfers.size(); ++i)
  {
    const transfer_details& td = m_transfers[i];
    if (!td.m_spent && is_transfer_unlocked(td) )
    {
      if (dust < td.amount())
        unused_transfers_indices.push_back(i);
      else
        unused_dust_indices.push_back(i);
    }
  }

  bool select_one_dust = add_dust && !unused_dust_indices.empty();
  uint64_t found_money = 0;
  while (found_money < needed_money && (!unused_transfers_indices.empty() || !unused_dust_indices.empty()))
  {
    size_t idx;
    if (select_one_dust)
    {
      idx = pop_random_value(unused_dust_indices);
      select_one_dust = false;
    }
    else
    {
      idx = !unused_transfers_indices.empty() ? pop_random_value(unused_transfers_indices) : pop_random_value(unused_dust_indices);
    }

    transfer_container::iterator it = m_transfers.begin() + idx;
    selected_transfers.push_back(it);
    found_money += it->amount();
  }
  
  return found_money;
  */
}
//----------------------------------------------------------------------------------------------------
void wallet2::add_sent_unconfirmed_tx(const currency::transaction& tx, 
                                      uint64_t change_amount, 
                                      const std::string& recipient, 
                                      const std::string& comment)
{
  wallet_rpc::wallet_transfer_info& unconfirmed_wti = m_unconfirmed_txs[currency::get_transaction_hash(tx)];

  //unconfirmed_wti.tx = tx;
  unconfirmed_wti.remote_address = recipient;
  unconfirmed_wti.recipient_alias = get_alias_for_address(recipient);
  unconfirmed_wti.is_income = false;
  
  uint64_t inputs_amount = 0;
  bool r = get_inputs_money_amount(tx, inputs_amount);
  CHECK_AND_ASSERT_THROW_MES(r, "internal error in getting amounts from tx: " << get_transaction_hash(tx));
  prepare_wti(unconfirmed_wti, 0, time(NULL), tx, inputs_amount - change_amount, money_transfer2_details());

  m_wcallback->on_transfer2(unconfirmed_wti);
}
//----------------------------------------------------------------------------------------------------
std::string wallet2::get_alias_for_address(const std::string& addr)
{
  currency::COMMAND_RPC_GET_ALIASES_BY_ADDRESS::request req = addr;
  currency::COMMAND_RPC_GET_ALIASES_BY_ADDRESS::response res = AUTO_VAL_INIT(res);
  if (!m_core_proxy->call_COMMAND_RPC_GET_ALIASES_BY_ADDRESS(req, res))
  {
    LOG_PRINT_L0("Failed to COMMAND_RPC_GET_ALIASES_BY_ADDRESS");
    return "";
  }
  return res.alias;
}
//----------------------------------------------------------------------------------------------------
void wallet2::transfer(const std::vector<currency::tx_destination_entry>& dsts, size_t fake_outputs_count,
  uint64_t unlock_time, uint64_t fee, const std::vector<currency::extra_v>& extra, 
  const std::vector<currency::attachment_v> attachments, 
  currency::transaction& tx)
{
  transfer(dsts, fake_outputs_count, unlock_time, fee, extra, attachments, detail::digit_split_strategy, tx_dust_policy(fee), tx);
}
//----------------------------------------------------------------------------------------------------
void wallet2::transfer(const std::vector<currency::tx_destination_entry>& dsts, size_t fake_outputs_count,
  uint64_t unlock_time, uint64_t fee, const std::vector<currency::extra_v>& extra, 
  const std::vector<currency::attachment_v> attachments)
{
  currency::transaction tx;
  transfer(dsts, fake_outputs_count, unlock_time, fee, extra, attachments, tx);
}
//----------------------------------------------------------------------------------------------------
bool wallet2::is_connected_to_net()
{
  currency::COMMAND_RPC_GET_INFO::request req = AUTO_VAL_INIT(req);
  currency::COMMAND_RPC_GET_INFO::response res = AUTO_VAL_INIT(res);
  if (!m_core_proxy->call_COMMAND_RPC_GET_INFO(req, res))
  {
    LOG_PRINT_L0("Failed to COMMAND_RPC_GET_INFO");
    return "";
  }
  return (res.synchronized_connections_count) ? true : false;
}


}
