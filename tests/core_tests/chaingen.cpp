// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <vector>
#include <iostream>
#include <sstream>

#include "include_base_utils.h"

#include "console_handler.h"

#include "p2p/net_node.h"
#include "currency_core/currency_basic.h"
#include "currency_core/currency_basic_impl.h"
#include "currency_core/currency_format_utils.h"
#include "currency_core/miner.h"
#include "wallet/wallet2.h"
#include "chaingen.h"

using namespace std;

using namespace epee;
using namespace currency;

#define DIFF_UP_TIMESTAMP_DELTA 180

void test_generator::get_block_chain(std::vector<const block_info*>& blockchain, const crypto::hash& head, size_t n) const
{
  crypto::hash curr = head;
  while (null_hash != curr && blockchain.size() < n)
  {
    auto it = m_blocks_info.find(curr);
    if (m_blocks_info.end() == it)
    {
      throw std::runtime_error("block hash wasn't found");
    }

    blockchain.push_back(&it->second);
    curr = it->second.b.prev_id;
  }

  std::reverse(blockchain.begin(), blockchain.end());
}

void test_generator::get_last_n_block_sizes(std::vector<size_t>& block_sizes, const crypto::hash& head, size_t n) const
{
  std::vector<const block_info*> blockchain;
  get_block_chain(blockchain, head, n);
  BOOST_FOREACH(auto& bi, blockchain)
  {
    block_sizes.push_back(bi->block_size);
  }
}

uint64_t test_generator::get_already_generated_coins(const crypto::hash& blk_id) const
{
  auto it = m_blocks_info.find(blk_id);
  if (it == m_blocks_info.end())
    throw std::runtime_error("block hash wasn't found");

  return it->second.already_generated_coins;
}

currency::wide_difficulty_type test_generator::get_block_difficulty(const crypto::hash& blk_id) const
{
  auto it = m_blocks_info.find(blk_id);
  if (it == m_blocks_info.end())
    throw std::runtime_error("block hash wasn't found");
  
  auto it_prev = m_blocks_info.find(it->second.b.prev_id);
  if (it_prev == m_blocks_info.end())
    throw std::runtime_error("block hash wasn't found");

  return it->second.cumul_difficulty - it_prev->second.cumul_difficulty;
}

uint64_t test_generator::get_already_generated_coins(const currency::block& blk) const
{
  crypto::hash blk_hash;
  get_block_hash(blk, blk_hash);
  return get_already_generated_coins(blk_hash);
}

void test_generator::add_block(const currency::block& blk, size_t tsx_size, std::vector<size_t>& block_sizes, 
                               uint64_t already_generated_coins, 
                               wide_difficulty_type cum_diff, 
                               const std::list<currency::transaction>& tx_list)
{
  const size_t block_size = tsx_size + get_object_blobsize(blk.miner_tx);
  uint64_t block_reward;
  get_block_reward(misc_utils::median(block_sizes), block_size, already_generated_coins, block_reward);
  m_blocks_info[get_block_hash(blk)] = block_info(blk, already_generated_coins + block_reward, block_size, cum_diff, tx_list);
}

bool test_generator::construct_block(currency::block& blk, 
                                     uint64_t height, 
                                     const crypto::hash& prev_id,
                                     const currency::account_base& miner_acc, 
                                     uint64_t timestamp, 
                                     uint64_t already_generated_coins,
                                     std::vector<size_t>& block_sizes, 
                                     const std::list<currency::transaction>& tx_list, 
                                     const currency::alias_info& ai, 
                                     const std::list<currency::account_base>& coin_stake_sources)//in case of PoS block
{
  blk.major_version = CURRENT_BLOCK_MAJOR_VERSION;
  blk.minor_version = CURRENT_BLOCK_MINOR_VERSION;
  blk.timestamp = timestamp;
  blk.prev_id = prev_id;

  blk.tx_hashes.reserve(tx_list.size());
  BOOST_FOREACH(const transaction &tx, tx_list)
  {
    crypto::hash tx_hash;
    get_transaction_hash(tx, tx_hash);
    blk.tx_hashes.push_back(tx_hash);
  }

  uint64_t total_fee = 0;
  size_t txs_size = 0;
  BOOST_FOREACH(auto& tx, tx_list)
  {
    uint64_t fee = 0;
    bool r = get_tx_fee(tx, fee);
    CHECK_AND_ASSERT_MES(r, false, "wrong transaction passed to construct_block");
    total_fee += fee;
    txs_size += get_object_blobsize(tx);
  }

  // build block chain
  blockchain_vector blocks;
  outputs_index oi;
  tx_global_indexes txs_outs;
  get_block_chain(blocks, blk.prev_id, std::numeric_limits<size_t>::max());

  //pos
  wallets_vector wallets;

  size_t won_walled_index = 0;
  pos_entry pe = AUTO_VAL_INIT(pe);
  if (coin_stake_sources.size())
  {
    //build outputs index
    build_outputs_indext_for_chain(blocks, oi, txs_outs);

    //build wallets
    build_wallets(blocks, coin_stake_sources, txs_outs, wallets);

    bool r = find_kernel(coin_stake_sources, blocks, oi, wallets, pe, won_walled_index);
    CHECK_AND_ASSERT_THROW_MES(r, "failed to find_kernel ");
    blk.flags = CURRENCY_BLOCK_FLAG_POS_BLOCK;
  }

  blk.miner_tx = AUTO_VAL_INIT(blk.miner_tx);
  size_t target_block_size = txs_size + get_object_blobsize(blk.miner_tx);
  while (true)
  {
    if (!construct_miner_tx(height, misc_utils::median(block_sizes), 
                                    already_generated_coins,
                                    target_block_size, 
                                    total_fee, 
                                    miner_acc.get_keys().m_account_address, 
                                    blk.miner_tx, 
                                    blobdata(),
                                    11,                                    
                                    ai,
                                    static_cast<bool>(coin_stake_sources.size()), 
                                    pe))
      return false;

    size_t actual_block_size = txs_size + get_object_blobsize(blk.miner_tx);
    if (target_block_size < actual_block_size)
    {
      target_block_size = actual_block_size;
    }
    else if (actual_block_size < target_block_size)
    {
      size_t delta = target_block_size - actual_block_size;
      blk.miner_tx.extra.resize(blk.miner_tx.extra.size() + delta, 0);
      actual_block_size = txs_size + get_object_blobsize(blk.miner_tx);
      if (actual_block_size == target_block_size)
      {
        break;
      }
      else
      {
        CHECK_AND_ASSERT_MES(target_block_size < actual_block_size, false, "Unexpected block size");
        delta = actual_block_size - target_block_size;
        blk.miner_tx.extra.resize(blk.miner_tx.extra.size() - delta);
        actual_block_size = txs_size + get_object_blobsize(blk.miner_tx);
        if (actual_block_size == target_block_size)
        {
          break;
        }
        else
        {
          CHECK_AND_ASSERT_MES(actual_block_size < target_block_size, false, "Unexpected block size");
          blk.miner_tx.extra.resize(blk.miner_tx.extra.size() + delta, 0);
          target_block_size = txs_size + get_object_blobsize(blk.miner_tx);
        }
      }
    }
    else
    {
      break;
    }
  }

  //blk.tree_root_hash = get_tx_tree_hash(blk);

  wide_difficulty_type a_diffic = get_difficulty_for_next_block(blocks, !static_cast<bool>(coin_stake_sources.size()));
  CHECK_AND_ASSERT_MES(a_diffic, false, "get_difficulty_for_next_block for test blocks returned 0!");
  // Nonce search...
  blk.nonce = 0;
  if (!coin_stake_sources.size())
  {
    //pow block
    while (!find_nounce(blk, blocks, a_diffic, height))
      blk.timestamp++;
  }
  else
  {
    //need to build pos block
    bool r = sign_block(blk, pe, *wallets[won_walled_index], blocks, oi);
    CHECK_AND_ASSERT_MES(r, false, "Failed to find_kernel_and_sign()");
  }

  add_block(blk, 
            txs_size, 
            block_sizes,
            already_generated_coins, 
            blocks.size() ? blocks.back()->cumul_difficulty + a_diffic: a_diffic, 
            tx_list);

  return true;
}

bool test_generator::sign_block(currency::block& b, 
                                pos_entry& pe, 
                                tools::wallet2& w, 
                                const std::vector<const block_info*>& blocks, 
                                const outputs_index& oi)
{
  uint64_t h = 0;
  uint64_t out_i = 0;
  const transaction * pts = nullptr;
  crypto::public_key source_tx_pub_key = null_pkey;
  crypto::public_key out_key = null_pkey;

  bool r = get_output_details_by_global_index(blocks,
    oi,
    pe.amount,
    pe.index, 
    h,
    pts,
    out_i,
    source_tx_pub_key,
    out_key);
  CHECK_AND_ASSERT_THROW_MES(r, "Failed to get_output_details_by_global_index()");

  std::vector<const crypto::public_key*> keys_ptrs;
  keys_ptrs.push_back(&out_key);
  r = w.prepare_and_sign_pos_block(b, pe, source_tx_pub_key, out_i, keys_ptrs);
  CHECK_AND_ASSERT_THROW_MES(r,"Failed to prepare_and_sign_pos_block()");

  return true;
}

bool test_generator::build_wallets(const blockchain_vector& blocks, 
                                   const std::list<currency::account_base>& accs, 
                                   const tx_global_indexes& txs_outs,
                                   wallets_vector& wallets)
{
  struct stub_core_proxy: public tools::i_core_proxy
  {
    const tx_global_indexes& m_txs_outs;
    stub_core_proxy(const tx_global_indexes& txs_outs) : m_txs_outs(txs_outs)
    {}
    virtual bool call_COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES(const currency::COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::request& rqt, currency::COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::response& rsp)
    {
      auto it = m_txs_outs.find(rqt.txid);
      CHECK_AND_ASSERT_THROW_MES(it != m_txs_outs.end(), "tx not found");
      rsp.status = CORE_RPC_STATUS_OK;
      rsp.o_indexes = it->second;
      return true; 
    }
  };

  shared_ptr<tools::i_core_proxy> tmp_proxy(new stub_core_proxy(txs_outs));

  //build wallets
  wallets.clear();
  for (auto a : accs)
  {
    wallets.push_back(shared_ptr<tools::wallet2>(new tools::wallet2()));
    wallets.back()->assign_account(a);
    wallets.back()->get_account().set_createtime(0);
    wallets.back()->set_core_proxy(tmp_proxy);
  }
  uint64_t height = 0;
  for (auto& w : wallets)
  {
    for (auto& b : blocks)
    {
      uint64_t h = get_block_height(b->b);
      if (!h)
        continue;
      CHECK_AND_ASSERT_MES(height + 1 == h, false, "Failed to return");
      height = h;
      //skip genesis
      currency::block_complete_entry bce = AUTO_VAL_INIT(bce);
      bce.block = currency::block_to_blob(b->b);
      for (auto& tx : b->m_transactions)
      {
        bce.txs.push_back(currency::tx_to_blob(tx));
      }

      w->process_new_blockchain_entry(b->b, bce, currency::get_block_hash(b->b), height);
    }
  }
  return true;
}
bool test_generator::find_kernel(const std::list<currency::account_base>& accs,
  const blockchain_vector& blck_chain,
  const outputs_index& indexes,
  wallets_vector& wallets,
  currency::pos_entry& pe,
  size_t& found_wallet_index)
{
  uint64_t last_block_timestamp = 0;
  wide_difficulty_type basic_diff = 0;
  last_block_timestamp = blck_chain.back()->b.timestamp;
  basic_diff = get_difficulty_for_next_block(blck_chain, false);

  //lets try to find block
  size_t i = 0;
  for (auto& w : wallets)
  {
    currency::COMMAND_RPC_SCAN_POS::request scan_pos_entries;
    bool r = w->get_pos_entries(scan_pos_entries);
    CHECK_AND_ASSERT_THROW_MES(r, "Failed to get_pos_entries");

    for (size_t i = 0; i != scan_pos_entries.pos_entries.size(); i++)
    {
      for (uint64_t ts = last_block_timestamp - POS_SCAN_WINDOW; ts < last_block_timestamp + POS_SCAN_WINDOW; ts++)
      {
        stake_kernel sk = AUTO_VAL_INIT(sk);
        uint64_t coindays_weight = 0;
        build_kernel(scan_pos_entries.pos_entries[i].amount,
          scan_pos_entries.pos_entries[i].index,
          scan_pos_entries.pos_entries[i].keyimage,
          sk,
          coindays_weight,
          blck_chain,
          indexes);
        crypto::hash kernel_hash = crypto::cn_fast_hash(&sk, sizeof(sk));
        wide_difficulty_type this_coin_diff = basic_diff / coindays_weight;
        if (!check_hash(kernel_hash, this_coin_diff))
          continue;
        else
        {
          //found kernel
          LOG_PRINT_GREEN("Found kernel: amount=" << scan_pos_entries.pos_entries[i].amount
            << ", index=" << scan_pos_entries.pos_entries[i].index
            << ", key_image" << scan_pos_entries.pos_entries[i].keyimage, LOG_LEVEL_0);
          pe = scan_pos_entries.pos_entries[i];
          found_wallet_index = i;
          return true;
        }
      }
    }
    
  }
  return false;
}
//------------------------------------------------------------------
bool test_generator::build_outputs_indext_for_chain(const blockchain_vector& blocks, outputs_index& index, tx_global_indexes& txs_outs)
{
  for (size_t h = 0; h != blocks.size(); h++)
  {
    std::vector<uint64_t>& coinbase_outs = txs_outs[currency::get_transaction_hash(blocks[h]->b.miner_tx)];
    for (size_t out_i = 0; out_i != blocks[h]->b.miner_tx.vout.size(); out_i++)
    {
      coinbase_outs.push_back(index[blocks[h]->b.miner_tx.vout[out_i].amount].size());
      index[blocks[h]->b.miner_tx.vout[out_i].amount].push_back(std::tuple<size_t, size_t, size_t>(h, 0, out_i));
    }

    for (size_t tx_index = 0; tx_index != blocks[h]->m_transactions.size(); tx_index++)
    {
      std::vector<uint64_t>& tx_outs_indx = txs_outs[currency::get_transaction_hash(blocks[h]->b.miner_tx)];
      for (size_t out_i = 0; out_i != blocks[h]->m_transactions[tx_index].vout.size(); out_i++)
      {
        tx_outs_indx.push_back(index[blocks[h]->m_transactions[tx_index].vout[out_i].amount].size());
        index[blocks[h]->m_transactions[tx_index].vout[out_i].amount].push_back(std::tuple<size_t, size_t, size_t>(h, tx_index + 1, out_i));
      }
    }
  }
  return true;
}
//------------------------------------------------------------------
bool test_generator::get_output_details_by_global_index(const test_generator::blockchain_vector& blck_chain,
                                                        const test_generator::outputs_index& indexes,
                                                        uint64_t amount,
                                                        uint64_t global_index,
                                                        uint64_t& h,
                                                        const transaction* tx,
                                                        uint64_t& tx_out_index,
                                                        crypto::public_key& tx_pub_key,
                                                        crypto::public_key& output_key)
{
  auto it = indexes.find(amount);
  CHECK_AND_ASSERT_MES(it != indexes.end(), false, "Failed to find amount in coin stake " << amount);

  CHECK_AND_ASSERT_MES(it->second.size() > global_index, false, "wrong key offset " << global_index  << " with amount kernel_in.amount");

  h = std::get<0>(it->second[global_index]);
  size_t tx_index = std::get<1>(it->second[global_index]);
  tx_out_index = std::get<2>(it->second[global_index]);

  CHECK_AND_ASSERT_THROW_MES(h < blck_chain.size(), "std::get<0>(it->second[kernel.tx_out_global_index]) < blck_chain.size()");
  CHECK_AND_ASSERT_THROW_MES(tx_index  < blck_chain[h]->m_transactions.size() + 1, "tx_index < blck_chain[h].m_transactions.size()");
  tx = tx_index ? &blck_chain[h]->m_transactions[tx_index - 1] : &blck_chain[h]->b.miner_tx;

  CHECK_AND_ASSERT_THROW_MES(tx_out_index < tx->vout.size(), "tx_index < blck_chain[h].m_transactions.size()");

  tx_pub_key = get_tx_pub_key_from_extra(*tx);
  CHECK_AND_ASSERT_THROW_MES(tx->vout[tx_out_index].target.type() == typeid(currency::txout_to_key),
    "blck_chain[h]->m_transactions[tx_index].vout[tx_out_index].target.type() == typeid(currency::txout_to_key)");

  CHECK_AND_ASSERT_THROW_MES(tx->vout[tx_out_index].amount == amount,
    "blck_chain[h]->m_transactions[tx_index].vout[tx_out_index].amount == amount");

  output_key = boost::get<currency::txout_to_key>(tx->vout[tx_out_index].target).key;
  return true;
}
//------------------------------------------------------------------
bool test_generator::build_kernel(uint64_t amount, 
                                  uint64_t global_index, 
                                  const crypto::key_image& ki, 
                                  stake_kernel& kernel, 
                                  uint64_t& coindays_weight,
                                  const test_generator::blockchain_vector& blck_chain,
                                  const test_generator::outputs_index& indexes)
{
  coindays_weight = 0;
  kernel = stake_kernel();
  kernel.tx_out_global_index = global_index;
  kernel.kimage = ki;

  //get block related with coinstake source transaction
  uint64_t h = 0; 
  uint64_t out_i = 0;
  const transaction * pts = nullptr;
  crypto::public_key source_tx_pub_key = null_pkey;
  crypto::public_key out_key = null_pkey;

  bool r = get_output_details_by_global_index(blck_chain, 
                                     indexes, 
                                     amount, 
                                     global_index, 
                                     h, 
                                     pts, 
                                     out_i, 
                                     source_tx_pub_key, 
                                     out_key);
  CHECK_AND_ASSERT_THROW_MES(r,"Failed to get_output_details_by_global_index()");


  CHECK_AND_ASSERT_THROW_MES(blck_chain[h]->b.timestamp <= blck_chain.back()->b.timestamp, "wrong coin age");

  uint64_t coin_age = blck_chain.back()->b.timestamp - blck_chain[h]->b.timestamp;
  kernel.tx_block_timestamp = blck_chain[h]->b.timestamp;

  coindays_weight = get_coinday_weight(amount, coin_age);
  build_stake_modifier(kernel.stake_modifier, blck_chain);
  return true;
}
bool test_generator::build_stake_modifier(crypto::hash& sm, const test_generator::blockchain_vector& blck_chain)
{
  //temporary implementation (to close to beginning atm)
  uint64_t height_for_modifier = 0;
  sm = null_hash;
  if (blck_chain.size() < POS_MODFIFIER_INTERVAL)
  {
    bool r = string_tools::parse_tpod_from_hex_string(POS_STARTER_MODFIFIER, sm);
    CHECK_AND_ASSERT_THROW_MES(r, "Failed to parse POS_STARTER_MODFIFIER");
    return true;
  }

  height_for_modifier = blck_chain.size() - (blck_chain.size() % POS_MODFIFIER_INTERVAL);
  crypto::xor_pod(sm, get_block_hash(blck_chain[height_for_modifier - 4]->b));
  crypto::xor_pod(sm, get_block_hash(blck_chain[height_for_modifier - 6]->b));
  crypto::xor_pod(sm, get_block_hash(blck_chain[height_for_modifier - 8]->b));

  return true;
}

bool test_generator::call_COMMAND_RPC_SCAN_POS(const currency::COMMAND_RPC_SCAN_POS::request& req, currency::COMMAND_RPC_SCAN_POS::response& rsp)
{
  return false;
}

currency::wide_difficulty_type test_generator::get_difficulty_for_next_block(const crypto::hash& head_id, bool pow)
{
  std::vector<const block_info*> blocks;
  get_block_chain(blocks, head_id, std::numeric_limits<size_t>::max());

  return get_difficulty_for_next_block(blocks, pow);
}

currency::wide_difficulty_type test_generator::get_difficulty_for_next_block(const std::vector<const block_info*>& blocks, bool pow)
{
  std::vector<uint64_t> timestamps;
  std::vector<wide_difficulty_type> commulative_difficulties;
  if (!blocks.size())
    return DIFFICULTY_STARTER;

  for (size_t i = blocks.size() - 1; i != 0; --i)
  {
    if ((pow && is_pos_block(blocks[i]->b)) || (!pow && !is_pos_block(blocks[i]->b)))
      continue;
    timestamps.push_back(blocks[i]->b.timestamp);
    commulative_difficulties.push_back(blocks[i]->cumul_difficulty);
  }
  return next_difficulty(timestamps, commulative_difficulties, DIFFICULTY_POW_TARGET);
}


bool test_generator::find_nounce(currency::block& blk, std::vector<const block_info*>& blocks, wide_difficulty_type dif, uint64_t height)
{
  if(height != blocks.size())
    throw std::runtime_error("wrong height in find_nounce");
  std::vector<crypto::hash> scratchpad_local;
  size_t count = 1;
  for(auto& i: blocks)
  {
    push_block_scratchpad_data(i->b, scratchpad_local);
#ifdef ENABLE_HASHING_DEBUG
    LOG_PRINT2("block_generation.log", "SCRATCHPAD_SHOT FOR H=" << count << ENDL << dump_scratchpad(scratchpad_local), LOG_LEVEL_3);
#endif
    ++count;
  }
  
  bool r = miner::find_nonce_for_given_block(blk, dif, height, [&](uint64_t index) -> crypto::hash&
  {
    return scratchpad_local[index%scratchpad_local.size()];
  });
#ifdef ENABLE_HASHING_DEBUG
  size_t call_no = 0;
  std::stringstream ss;
  crypto::hash pow = get_block_longhash(blk, height, [&](uint64_t index) -> crypto::hash&
  {
    ss << "[" << call_no << "][" << index << "%" << scratchpad_local.size() <<"(" << index%scratchpad_local.size() << ")]" << scratchpad_local[index%scratchpad_local.size()] << ENDL;
    ++call_no;
    return scratchpad_local[index%scratchpad_local.size()];
  });
  LOG_PRINT2("block_generation.log", "ID: " << get_block_hash(blk) << "[" << height <<  "]" << ENDL << "POW:" << pow << ENDL << ss.str(), LOG_LEVEL_3);
#endif
  return r;
}

bool test_generator::construct_block(currency::block& blk, const currency::account_base& miner_acc, uint64_t timestamp, const currency::alias_info& ai)
{
  std::vector<size_t> block_sizes;
  std::list<currency::transaction> tx_list;
  return construct_block(blk, 0, null_hash, miner_acc, timestamp, 0, block_sizes, tx_list, ai);
}

bool test_generator::construct_block(currency::block& blk, 
                                     const currency::block& blk_prev,
                                     const currency::account_base& miner_acc,
                                     const std::list<currency::transaction>& tx_list, 
                                     const currency::alias_info& ai,
                                     const std::list<currency::account_base>& coin_stake_sources)
{
  uint64_t height = boost::get<txin_gen>(blk_prev.miner_tx.vin.front()).height + 1;
  crypto::hash prev_id = get_block_hash(blk_prev);
  // Keep push difficulty little up to be sure about PoW hash success
  uint64_t timestamp = height > 10 ? blk_prev.timestamp + DIFFICULTY_POW_TARGET : blk_prev.timestamp + DIFFICULTY_POW_TARGET - DIFF_UP_TIMESTAMP_DELTA;
  uint64_t already_generated_coins = get_already_generated_coins(prev_id);
  std::vector<size_t> block_sizes;
  get_last_n_block_sizes(block_sizes, prev_id, CURRENCY_REWARD_BLOCKS_WINDOW);

  return construct_block(blk, height, prev_id, miner_acc, timestamp, already_generated_coins, block_sizes, tx_list, ai, coin_stake_sources);
}

 bool test_generator::construct_block_manually(currency::block& blk, const block& prev_block, const account_base& miner_acc,
                                               int actual_params/* = bf_none*/, uint8_t major_ver/* = 0*/,
                                               uint8_t minor_ver/* = 0*/, uint64_t timestamp/* = 0*/,
                                              const crypto::hash& prev_id/* = crypto::hash()*/, const wide_difficulty_type& diffic/* = 1*/,
                                              const transaction& miner_tx/* = transaction()*/,
                                              const std::vector<crypto::hash>& tx_hashes/* = std::vector<crypto::hash>()*/,
                                              size_t txs_sizes/* = 0*/)
{
  size_t height = get_block_height(prev_block) + 1;
  blk.major_version = actual_params & bf_major_ver ? major_ver : CURRENT_BLOCK_MAJOR_VERSION;
  blk.minor_version = actual_params & bf_minor_ver ? minor_ver : CURRENT_BLOCK_MINOR_VERSION;
  blk.timestamp     = actual_params & bf_timestamp ? timestamp : (height > 10 ? prev_block.timestamp + DIFFICULTY_BLOCKS_ESTIMATE_TIMESPAN: prev_block.timestamp + DIFFICULTY_BLOCKS_ESTIMATE_TIMESPAN-DIFF_UP_TIMESTAMP_DELTA); // Keep difficulty unchanged
  blk.prev_id       = actual_params & bf_prev_id   ? prev_id   : get_block_hash(prev_block);
  blk.tx_hashes     = actual_params & bf_tx_hashes ? tx_hashes : std::vector<crypto::hash>();

  
  uint64_t already_generated_coins = get_already_generated_coins(prev_block);
  std::vector<size_t> block_sizes;
  get_last_n_block_sizes(block_sizes, get_block_hash(prev_block), CURRENCY_REWARD_BLOCKS_WINDOW);
  if (actual_params & bf_miner_tx)
  {
    blk.miner_tx = miner_tx;
  }
  else
  {
    size_t current_block_size = txs_sizes + get_object_blobsize(blk.miner_tx);
    // TODO: This will work, until size of constructed block is less then CURRENCY_BLOCK_GRANTED_FULL_REWARD_ZONE
    if (!construct_miner_tx(height, misc_utils::median(block_sizes), already_generated_coins, current_block_size, 0, miner_acc.get_keys().m_account_address, blk.miner_tx, blobdata(), 1))
      return false;
  }

  //blk.tree_root_hash = get_tx_tree_hash(blk);
  std::vector<const block_info*> blocks;
  get_block_chain(blocks, blk.prev_id, std::numeric_limits<size_t>::max());

  wide_difficulty_type a_diffic = actual_params & bf_diffic ? diffic : get_difficulty_for_next_block(blocks);
  find_nounce(blk, blocks, a_diffic, height);

  std::list<transaction> txs; // fake list here
  add_block(blk, txs_sizes, block_sizes, already_generated_coins, blocks.size() ? blocks.back()->cumul_difficulty + a_diffic : a_diffic, txs);

  return true;
}

bool test_generator::construct_block_manually_tx(currency::block& blk, const currency::block& prev_block,
                                                 const currency::account_base& miner_acc,
                                                 const std::vector<crypto::hash>& tx_hashes, size_t txs_size)
{
  return construct_block_manually(blk, prev_block, miner_acc, bf_tx_hashes, 0, 0, 0, crypto::hash(), 0, transaction(), tx_hashes, txs_size);
}


struct output_index {
    const currency::txout_target_v out;
    uint64_t amount;
    size_t blk_height; // block height
    size_t tx_no; // index of transaction in block
    size_t out_no; // index of out in transaction
    size_t idx;
    bool spent;
    const currency::block *p_blk;
    const currency::transaction *p_tx;

    output_index(const currency::txout_target_v &_out, uint64_t _a, size_t _h, size_t tno, size_t ono, const currency::block *_pb, const currency::transaction *_pt)
        : out(_out), amount(_a), blk_height(_h), tx_no(tno), out_no(ono), idx(0), spent(false), p_blk(_pb), p_tx(_pt) { }

    output_index(const output_index &other)
        : out(other.out), amount(other.amount), blk_height(other.blk_height), tx_no(other.tx_no), out_no(other.out_no), idx(other.idx), spent(other.spent), p_blk(other.p_blk), p_tx(other.p_tx) {  }

    const std::string toString() const {
        std::stringstream ss;

        ss << "output_index{blk_height=" << blk_height
           << " tx_no=" << tx_no
           << " out_no=" << out_no
           << " amount=" << amount
           << " idx=" << idx
           << " spent=" << spent
           << "}";

        return ss.str();
    }

    output_index& operator=(const output_index& other)
    {
      new(this) output_index(other);
      return *this;
    }
};

typedef std::map<uint64_t, std::vector<size_t> > map_output_t;
typedef std::map<uint64_t, std::vector<output_index> > map_output_idx_t;
typedef pair<uint64_t, size_t>  outloc_t;

namespace
{
  uint64_t get_inputs_amount(const vector<tx_source_entry> &s)
  {
    uint64_t r = 0;
    BOOST_FOREACH(const tx_source_entry &e, s)
    {
      r += e.amount;
    }

    return r;
  }
}

bool init_output_indices(map_output_idx_t& outs, std::map<uint64_t, std::vector<size_t> >& outs_mine, const std::vector<currency::block>& blockchain, const map_hash2tx_t& mtx, const currency::account_base& from) {

    BOOST_FOREACH (const block& blk, blockchain) {
        vector<const transaction*> vtx;
        vtx.push_back(&blk.miner_tx);

        BOOST_FOREACH(const crypto::hash &h, blk.tx_hashes) {
            const map_hash2tx_t::const_iterator cit = mtx.find(h);
            if (mtx.end() == cit)
                throw std::runtime_error("block contains an unknown tx hash");

            vtx.push_back(cit->second);
        }

        //vtx.insert(vtx.end(), blk.);
        // TODO: add all other txes
        for (size_t i = 0; i < vtx.size(); i++) {
            const transaction &tx = *vtx[i];

            for (size_t j = 0; j < tx.vout.size(); ++j) {
                const tx_out &out = tx.vout[j];

                output_index oi(out.target, out.amount, boost::get<txin_gen>(*blk.miner_tx.vin.begin()).height, i, j, &blk, vtx[i]);

                if (2 == out.target.which()) { // out_to_key
                    outs[out.amount].push_back(oi);
                    size_t tx_global_idx = outs[out.amount].size() - 1;
                    outs[out.amount][tx_global_idx].idx = tx_global_idx;
                    // Is out to me?
                    if (is_out_to_acc(from.get_keys(), boost::get<txout_to_key>(out.target), get_tx_pub_key_from_extra(tx), j)) {
                        outs_mine[out.amount].push_back(tx_global_idx);
                    }
                }
            }
        }
    }

    return true;
}

bool init_spent_output_indices(map_output_idx_t& outs, map_output_t& outs_mine, const std::vector<currency::block>& blockchain, const map_hash2tx_t& mtx, const currency::account_base& from) {

    BOOST_FOREACH (const map_output_t::value_type &o, outs_mine) {
        for (size_t i = 0; i < o.second.size(); ++i) {
            output_index &oi = outs[o.first][o.second[i]];

            // construct key image for this output
            crypto::key_image img;
            keypair in_ephemeral;
            generate_key_image_helper(from.get_keys(), get_tx_pub_key_from_extra(*oi.p_tx), oi.out_no, in_ephemeral, img);

            // lookup for this key image in the events vector
            BOOST_FOREACH(auto& tx_pair, mtx) {
                const transaction& tx = *tx_pair.second;
                BOOST_FOREACH(const txin_v &in, tx.vin) {
                    if (typeid(txin_to_key) == in.type()) {
                        const txin_to_key &itk = boost::get<txin_to_key>(in);
                        if (itk.k_image == img) {
                            oi.spent = true;
                        }
                    }
                }
            }
        }
    }

    return true;
}

bool fill_output_entries(std::vector<output_index>& out_indices,
                         size_t sender_out, size_t nmix, uint64_t& real_entry_idx,
                         std::vector<tx_source_entry::output_entry>& output_entries)
{
  if (out_indices.size() <= nmix)
    return false;

  bool sender_out_found = false;
  size_t rest = nmix;
  for (size_t i = 0; i < out_indices.size() && (0 < rest || !sender_out_found); ++i)
  {
    const output_index& oi = out_indices[i];
    if (oi.spent)
      continue;

    bool append = false;
    if (i == sender_out)
    {
      append = true;
      sender_out_found = true;
      real_entry_idx = output_entries.size();
    }
    else if (0 < rest)
    {
      if(boost::get<txout_to_key>(oi.out).mix_attr == CURRENCY_TO_KEY_OUT_FORCED_NO_MIX || boost::get<txout_to_key>(oi.out).mix_attr > nmix+1)
        continue;

      --rest;
      append = true;
    }

    if (append)
    {
      const txout_to_key& otk = boost::get<txout_to_key>(oi.out);
      output_entries.push_back(tx_source_entry::output_entry(oi.idx, otk.key));
    }
  }

  return 0 == rest && sender_out_found;
}

bool fill_tx_sources(std::vector<tx_source_entry>& sources, const std::vector<test_event_entry>& events,
                     const block& blk_head, const currency::account_base& from, uint64_t amount, size_t nmix, bool check_for_spends = true)
{
    map_output_idx_t outs;
    map_output_t outs_mine;

    std::vector<currency::block> blockchain;
    map_hash2tx_t mtx;
    if (!find_block_chain(events, blockchain, mtx, get_block_hash(blk_head)))
        return false;

    if (!init_output_indices(outs, outs_mine, blockchain, mtx, from))
        return false;

    if(check_for_spends)
    {
      if (!init_spent_output_indices(outs, outs_mine, blockchain, mtx, from))
        return false;
    }

    // Iterate in reverse is more efficiency
    uint64_t sources_amount = 0;
    bool sources_found = false;
    BOOST_REVERSE_FOREACH(const map_output_t::value_type o, outs_mine)
    {
        for (size_t i = 0; i < o.second.size() && !sources_found; ++i)
        {
            size_t sender_out = o.second[i];
            const output_index& oi = outs[o.first][sender_out];
            if (oi.spent)
                continue;

            if (oi.p_tx->unlock_time > blockchain.size())
              continue;


            currency::tx_source_entry ts;
            ts.amount = oi.amount;
            ts.real_output_in_tx_index = oi.out_no;
            ts.real_out_tx_key = get_tx_pub_key_from_extra(*oi.p_tx); // incoming tx public key
            if (!fill_output_entries(outs[o.first], sender_out, nmix, ts.real_output, ts.outputs))
              continue;

            sources.push_back(ts);

            sources_amount += ts.amount;
            sources_found = amount <= sources_amount;
        }

        if (sources_found)
            break;
    }

    return sources_found;
}

bool fill_tx_destination(tx_destination_entry &de, const currency::account_base &to, uint64_t amount) {
    de.addr = to.get_keys().m_account_address;
    de.amount = amount;
    return true;
}

void fill_tx_sources_and_destinations(const std::vector<test_event_entry>& events, const block& blk_head,
                                      const currency::account_base& from, const currency::account_base& to,
                                      uint64_t amount, uint64_t fee, size_t nmix, std::vector<tx_source_entry>& sources,
                                      std::vector<tx_destination_entry>& destinations,
                                      bool check_for_spends)
{
  sources.clear();
  destinations.clear();

  if (!fill_tx_sources(sources, events, blk_head, from, amount + fee, nmix, check_for_spends))
    throw std::runtime_error("couldn't fill transaction sources");

  tx_destination_entry de;
  if (!fill_tx_destination(de, to, amount))
    throw std::runtime_error("couldn't fill transaction destination");
  destinations.push_back(de);

  tx_destination_entry de_change;
  uint64_t cache_back = get_inputs_amount(sources) - (amount + fee);
  if (0 < cache_back)
  {
    if (!fill_tx_destination(de_change, from, cache_back))
      throw std::runtime_error("couldn't fill transaction cache back destination");
    destinations.push_back(de_change);
  }
}

/*
void fill_nonce(currency::block& blk, const wide_difficulty_type& diffic, uint64_t height)
{
  blk.nonce = 0;
  while (!miner::find_nonce_for_given_block(blk, diffic, height))
    blk.timestamp++;
}*/

bool construct_miner_tx_manually(size_t height, uint64_t already_generated_coins,
                                 const account_public_address& miner_address, transaction& tx, uint64_t fee,
                                 keypair* p_txkey/* = 0*/)
{
  keypair txkey;
  txkey = keypair::generate();
  add_tx_pub_key_to_extra(tx, txkey.pub);

  if (0 != p_txkey)
    *p_txkey = txkey;

  txin_gen in;
  in.height = height;
  tx.vin.push_back(in);

  // This will work, until size of constructed block is less then CURRENCY_BLOCK_GRANTED_FULL_REWARD_ZONE
  uint64_t block_reward;
  if (!get_block_reward(0, 0, already_generated_coins, block_reward))
  {
    LOG_PRINT_L0("Block is too big");
    return false;
  }
  block_reward += fee;

  crypto::key_derivation derivation;
  crypto::public_key out_eph_public_key;
  crypto::generate_key_derivation(miner_address.m_view_public_key, txkey.sec, derivation);
  crypto::derive_public_key(derivation, 0, miner_address.m_spend_public_key, out_eph_public_key);

  tx_out out;
  out.amount = block_reward;
  out.target = txout_to_key(out_eph_public_key);
  tx.vout.push_back(out);

  tx.version = CURRENT_TRANSACTION_VERSION;
  tx.unlock_time = height + CURRENCY_MINED_MONEY_UNLOCK_WINDOW;

  return true;
}

bool construct_tx_to_key(const std::vector<test_event_entry>& events, currency::transaction& tx, const block& blk_head,
                         const currency::account_base& from, const currency::account_base& to, uint64_t amount,
                         uint64_t fee, size_t nmix, uint8_t mix_attr, const std::list<currency::offer_details>& off, bool check_for_spends)
{
  vector<tx_source_entry> sources;
  vector<tx_destination_entry> destinations;
  fill_tx_sources_and_destinations(events, blk_head, from, to, amount, fee, nmix, sources, destinations, check_for_spends);

  return construct_tx(from.get_keys(), sources, destinations, tx, 0, mix_attr, off);
}

transaction construct_tx_with_fee(std::vector<test_event_entry>& events, const block& blk_head,
                                  const account_base& acc_from, const account_base& acc_to, uint64_t amount, uint64_t fee)
{
  transaction tx;
  construct_tx_to_key(events, tx, blk_head, acc_from, acc_to, amount, fee, 0);
  events.push_back(tx);
  return tx;
}

uint64_t get_balance(const currency::account_base& addr, const std::vector<currency::block>& blockchain, const map_hash2tx_t& mtx) {
    uint64_t res = 0;
    std::map<uint64_t, std::vector<output_index> > outs;
    std::map<uint64_t, std::vector<size_t> > outs_mine;

    map_hash2tx_t confirmed_txs;
    get_confirmed_txs(blockchain, mtx, confirmed_txs);

    if (!init_output_indices(outs, outs_mine, blockchain, confirmed_txs, addr))
        return false;

    if (!init_spent_output_indices(outs, outs_mine, blockchain, confirmed_txs, addr))
        return false;

    BOOST_FOREACH (const map_output_t::value_type &o, outs_mine) {
        for (size_t i = 0; i < o.second.size(); ++i) {
            if (outs[o.first][o.second[i]].spent)
                continue;

            res += outs[o.first][o.second[i]].amount;
        }
    }

    return res;
}

void get_confirmed_txs(const std::vector<currency::block>& blockchain, const map_hash2tx_t& mtx, map_hash2tx_t& confirmed_txs)
{
  std::unordered_set<crypto::hash> confirmed_hashes;
  BOOST_FOREACH(const block& blk, blockchain)
  {
    BOOST_FOREACH(const crypto::hash& tx_hash, blk.tx_hashes)
    {
      confirmed_hashes.insert(tx_hash);
    }
  }

  BOOST_FOREACH(const auto& tx_pair, mtx)
  {
    if (0 != confirmed_hashes.count(tx_pair.first))
    {
      confirmed_txs.insert(tx_pair);
    }
  }
}

bool find_block_chain(const std::vector<test_event_entry>& events, std::vector<currency::block>& blockchain, map_hash2tx_t& mtx, const crypto::hash& head) {
    std::unordered_map<crypto::hash, const block*> block_index;
    BOOST_FOREACH(const test_event_entry& ev, events)
    {
        if (typeid(currency::block) == ev.type())
        {
            const block* blk = &boost::get<block>(ev);
            block_index[get_block_hash(*blk)] = blk;
        }
        else if (typeid(transaction) == ev.type())
        {
            const transaction& tx = boost::get<transaction>(ev);
            mtx[get_transaction_hash(tx)] = &tx;
        }
    }

    bool b_success = false;
    crypto::hash id = head;
    for (auto it = block_index.find(id); block_index.end() != it; it = block_index.find(id))
    {
        blockchain.push_back(*it->second);
        id = it->second->prev_id;
        if (null_hash == id)
        {
            b_success = true;
            break;
        }
    }
    reverse(blockchain.begin(), blockchain.end());

    return b_success;
}


void test_chain_unit_base::register_callback(const std::string& cb_name, verify_callback cb)
{
  m_callbacks[cb_name] = cb;
}
bool test_chain_unit_base::verify(const std::string& cb_name, currency::core& c, size_t ev_index, const std::vector<test_event_entry> &events)
{
  auto cb_it = m_callbacks.find(cb_name);
  if(cb_it == m_callbacks.end())
  {
    LOG_ERROR("Failed to find callback " << cb_name);
    return false;
  }
  return cb_it->second(c, ev_index, events);
}
