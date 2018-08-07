// Copyright (c) 2012-2013 The Cryptonote developers
// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <algorithm>
#include <cstdio>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include "include_base_utils.h"
#include "common/db_lmdb_adapter.h"
#include "currency_basic_impl.h"
#include "blockchain_storage.h"
#include "currency_format_utils.h"
#include "currency_boost_serialization.h"
#include "blockchain_storage_boost_serialization.h"
#include "currency_config.h"
#include "miner.h"
#include "misc_language.h"
#include "profile_tools.h"
#include "file_io_utils.h"
#include "common/boost_serialization_helper.h"
#include "warnings.h"
#include "crypto/hash.h"
#include "miner_common.h"
#include "version.h"

using namespace std;
using namespace epee;
using namespace currency;

#define BLOCKCHAIN_CONTAINER_SPENT_KEYS       "spent_keys"
#define BLOCKCHAIN_CONTAINER_BLOCKS           "blocks"
#define BLOCKCHAIN_CONTAINER_OUTPUTS          "outputs"
#define BLOCKCHAIN_CONTAINER_MULTISIG_OUTS    "multisig_outs"
#define BLOCKCHAIN_CONTAINER_INVALID_BLOCKS   "invalid_blocks"
#define BLOCKCHAIN_CONTAINER_TRANSACTIONS     "transactions"
#define BLOCKCHAIN_CONTAINER_SOLO_OPTIONS     "solo"
#define BLOCKCHAIN_CONTAINER_ALIASES          "aliases"
#define BLOCKCHAIN_CONTAINER_ADDR_TO_ALIAS    "addr_to_alias"
#define BLOCKCHAIN_CONTAINER_SCRATCHPAD       "scratchpad"
#define BLOCKCHAIN_CONTAINER_BLOCKS_INDEX     "blocks_index"

#define BLOCKCHAIN_OPTIONS_ID_CURRENT_BLOCK_CUMUL_SZ_LIMIT          0
#define BLOCKCHAIN_OPTIONS_ID_CURRENT_PRUNED_RS_HEIGHT              1
#define BLOCKCHAIN_OPTIONS_ID_LAST_WORKED_VERSION                   2
#define BLOCKCHAIN_OPTIONS_ID_STORAGE_MAJOR_COMPABILITY_VERSION     3 //mismatch here means full resync

#define BLOCKCHAIN_STORAGE_MAJOR_COMPABILITY_VERSION                1


DISABLE_VS_WARNINGS(4267)



//------------------------------------------------------------------
blockchain_storage::blockchain_storage(tx_memory_pool& tx_pool) : m_lmdb_adapter(new db::lmdb_adapter()),
                                                                 m_db(m_lmdb_adapter),
                                                                 m_db_blocks(m_db),
                                                                 m_db_blocks_index(m_db),
                                                                 m_db_transactions(m_db),
                                                                 m_db_spent_keys(m_db),
                                                                 m_db_outputs(m_db),
                                                                 m_db_solo_options(m_db),
                                                                 m_db_aliases(m_db),
                                                                 m_db_addr_to_alias(m_db), 
                                                                 m_db_scratchpad_internal(m_db),
                                                                 m_scratchpad_wr(m_db_scratchpad_internal),
                                                                 m_db_current_block_cumul_sz_limit(BLOCKCHAIN_OPTIONS_ID_CURRENT_BLOCK_CUMUL_SZ_LIMIT, m_db_solo_options),
                                                                 m_db_current_pruned_rs_height(BLOCKCHAIN_OPTIONS_ID_CURRENT_PRUNED_RS_HEIGHT, m_db_solo_options),
                                                                 m_db_last_worked_version(BLOCKCHAIN_OPTIONS_ID_LAST_WORKED_VERSION, m_db_solo_options),
                                                                 m_db_storage_major_compability_version(BLOCKCHAIN_OPTIONS_ID_STORAGE_MAJOR_COMPABILITY_VERSION, m_db_solo_options),                                                               
                                                                 m_tx_pool(tx_pool),
                                                                 m_is_in_checkpoint_zone(false), 
                                                                 m_donations_account(AUTO_VAL_INIT(m_donations_account)), 
                                                                 m_royalty_account(AUTO_VAL_INIT(m_royalty_account)),
                                                                 m_is_blockchain_storing(false), 
                                                                 m_locker_file(0)
{
  bool r = get_donation_accounts(m_donations_account, m_royalty_account);
  CHECK_AND_ASSERT_THROW_MES(r, "failed to load donation accounts");
}
//------------------------------------------------------------------
bool blockchain_storage::have_tx(const crypto::hash &id)
{
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  return m_db_transactions.find(id) != m_db_transactions.end();
}
//------------------------------------------------------------------
bool blockchain_storage::have_tx_keyimg_as_spent(const crypto::key_image &key_im)
{
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  return  m_db_spent_keys.find(key_im) != m_db_spent_keys.end();
}
//------------------------------------------------------------------
std::shared_ptr<transaction> blockchain_storage::get_tx(const crypto::hash &id)
{
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  auto it = m_db_transactions.find(id);
  if (it == m_db_transactions.end())
    return std::shared_ptr<transaction>(nullptr);

  return std::make_shared<transaction>(it->tx);
}
//------------------------------------------------------------------
uint64_t blockchain_storage::get_current_blockchain_height()
{
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  return m_db_blocks.size();
}
// ------------------------------------------------------------------
// void blockchain_storage::fill_addr_to_alias_dict()
// {
//   size_t count = m_db_aliases.size();
//   for (size_t i = 0; i != count; i++)
//   {
//     auto a = m_db_aliases[i];
//     if (a.second.size())
//     {
//       m_db_addr_to_alias[a.second.back()->m_address] = a.first;
//     }
//   }
// }
//------------------------------------------------------------------
void blockchain_storage::init_options(boost::program_options::options_description& desc)
{
  db::lmdb_adapter::init_options(desc);
}
//------------------------------------------------------------------
bool blockchain_storage::init(const boost::program_options::variables_map& vm, const std::string& config_folder)
{
  CRITICAL_REGION_LOCAL(m_blockchain_lock);

  bool res = m_lmdb_adapter->init(vm);
  CHECK_AND_ASSERT_MES(res, false, "Unable to init lmdb adapter");

  m_config_folder = config_folder;
  LOG_PRINT_L0("Loading blockchain...");
  const std::string folder_name = m_config_folder + "/" CURRENCY_BLOCKCHAINDATA_FOLDERNAME;
  tools::create_directories_if_necessary(folder_name);
  res = m_db.open(folder_name);
  CHECK_AND_ASSERT_MES(res, false, "Failed to initialize database in folder: " << folder_name);

  res = m_db_blocks.init(BLOCKCHAIN_CONTAINER_BLOCKS);
  CHECK_AND_ASSERT_MES(res, false, "Unable to init db container");
  res = m_db_blocks_index.init(BLOCKCHAIN_CONTAINER_BLOCKS_INDEX);
  CHECK_AND_ASSERT_MES(res, false, "Unable to init db container");
  res = m_db_transactions.init(BLOCKCHAIN_CONTAINER_TRANSACTIONS);
  CHECK_AND_ASSERT_MES(res, false, "Unable to init db container");
  res = m_db_spent_keys.init(BLOCKCHAIN_CONTAINER_SPENT_KEYS);
  CHECK_AND_ASSERT_MES(res, false, "Unable to init db container");
  res = m_db_outputs.init(BLOCKCHAIN_CONTAINER_OUTPUTS);
  CHECK_AND_ASSERT_MES(res, false, "Unable to init db container");
  res = m_db_solo_options.init(BLOCKCHAIN_CONTAINER_SOLO_OPTIONS);
  CHECK_AND_ASSERT_MES(res, false, "Unable to init db container");
  res = m_db_aliases.init(BLOCKCHAIN_CONTAINER_ALIASES);
  CHECK_AND_ASSERT_MES(res, false, "Unable to init db container");
  res = m_db_addr_to_alias.init(BLOCKCHAIN_CONTAINER_ADDR_TO_ALIAS);
  CHECK_AND_ASSERT_MES(res, false, "Unable to init db container");
  res = m_db_scratchpad_internal.init(BLOCKCHAIN_CONTAINER_SCRATCHPAD);
  CHECK_AND_ASSERT_MES(res, false, "Unable to init db container");

  res = m_scratchpad_wr.init(config_folder);
  CHECK_AND_ASSERT_MES(res, false, "Unable to init scratchpad wrapper");

  bool need_reinit = false;
  if (!m_db_blocks.size())
    need_reinit = true;
  else if (m_db_storage_major_compability_version != BLOCKCHAIN_STORAGE_MAJOR_COMPABILITY_VERSION)
    need_reinit = true;

  if (need_reinit)
  {
    clear();
    block bl = boost::value_initialized<block>();
    block_verification_context bvc = boost::value_initialized<block_verification_context>();
    generate_genesis_block(bl);
    add_new_block(bl, bvc);
    CHECK_AND_ASSERT_MES(!bvc.m_verifivation_failed, false, "Failed to add genesis block to blockchain");
    LOG_PRINT_MAGENTA("Storage initialized with genesis", LOG_LEVEL_0);
  }
  initialize_db_solo_options_values();

  //print information message
  uint64_t timestamp_diff = time(nullptr) - m_db_blocks.back()->bl.timestamp;
  if (!m_db_blocks.back()->bl.timestamp)
    timestamp_diff = time(nullptr) - 1341378000;

  LOG_PRINT_GREEN("Blockchain initialized. last block: " << m_db_blocks.size() - 1
    << ", " << misc_utils::get_time_interval_string(timestamp_diff) << " time ago", LOG_LEVEL_0);

  return true;
}
//------------------------------------------------------------------
void blockchain_storage::initialize_db_solo_options_values()
{
  m_db.begin_transaction();
  m_db_storage_major_compability_version = BLOCKCHAIN_STORAGE_MAJOR_COMPABILITY_VERSION;
  m_db_last_worked_version = std::string(PROJECT_VERSION_LONG);
  m_db.commit_transaction();
}
//------------------------------------------------------------------
bool blockchain_storage::deinit()
{
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  m_scratchpad_wr.deinit();
  m_db.close();
  epee::file_io_utils::unlock_and_close_file(m_locker_file);
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::pop_block_from_blockchain()
{
  CRITICAL_REGION_LOCAL(m_blockchain_lock);

  CHECK_AND_ASSERT_MES(m_db_blocks.size() > 1, false, "pop_block_from_blockchain: can't pop from blockchain with size = " << m_db_blocks.size());
  size_t h = m_db_blocks.size() - 1;
  auto vptr = m_db_blocks[h];
  CHECK_AND_ASSERT_MES(vptr.get(), false, "pop_block_from_blockchain: can't pop from blockchain");
  block_extended_info bei = *vptr;
  
  bool r = m_scratchpad_wr.pop_block_scratchpad_data(bei.bl);
  CHECK_AND_ASSERT_MES(r, false, "Failed to pop_block_scratchpad_data for block " << get_block_hash(bei.bl) << " on height " << h);

  r = purge_block_data_from_blockchain(bei.bl, bei.bl.tx_hashes.size());
  CHECK_AND_ASSERT_MES(r, false, "Failed to purge_block_data_from_blockchain for block " << get_block_hash(bei.bl) << " on height " << h);

  //remove from index
  r = m_db_blocks_index.erase_validate(get_block_hash(bei.bl));
  CHECK_AND_ASSERT_MES(r, false, "pop_block_from_blockchain: block id not found in m_blocks_index while trying to delete it");

  //pop block from core
  m_db_blocks.pop_back();
  m_tx_pool.on_blockchain_dec(m_db_blocks.size() - 1, get_top_block_id());
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::set_checkpoints(checkpoints&& chk_pts) 
{
  m_checkpoints = chk_pts;
  try
  {
    m_db.begin_transaction();
    prune_ring_signatures_if_need();
    m_db.commit_transaction();
    if (!m_checkpoints.is_in_checkpoint_zone(get_current_blockchain_height()))
    {
      m_is_in_checkpoint_zone = false;
    }
    else
    {
      m_is_in_checkpoint_zone = true;
    }
    return true;
  }
  catch (const std::exception& ex)
  {
    m_db.abort_transaction();
    LOG_ERROR("UNKNOWN EXCEPTION WHILE ADDINIG NEW BLOCK: " << ex.what());
    return false;
  }
  catch (...)
  {
    m_db.abort_transaction();
    LOG_ERROR("UNKNOWN EXCEPTION WHILE ADDINIG NEW BLOCK.");
    return false;
  }
}
//------------------------------------------------------------------
bool blockchain_storage::prune_ring_signatures(uint64_t height, uint64_t& transactions_pruned, uint64_t& signatures_pruned)
{
  CRITICAL_REGION_LOCAL(m_blockchain_lock);

  CHECK_AND_ASSERT_MES(height < m_db_blocks.size(), false, "prune_ring_signatures called with wrong parameter: " << height << ", m_blocks.size() " << m_db_blocks.size());
  auto vptr = m_db_blocks[height];
  CHECK_AND_ASSERT_MES(vptr.get(), false, "Failed to get block on height");

  for (const auto& h : vptr->bl.tx_hashes)
  {
    auto it = m_db_transactions.find(h);
    CHECK_AND_ASSERT_MES(it != m_db_transactions.end(), false, "failed to find transaction " << h << " in blockchain index, in block on height = " << height);


    CHECK_AND_ASSERT_MES(it->m_keeper_block_height == height, false,
      "failed to validate extra check, it->second.m_keeper_block_height = " << it->m_keeper_block_height <<
      "is mot equal to height = " << height << " in blockchain index, for block on height = " << height);

    transaction_chain_entry lolcal_chain_entry = *it;
    signatures_pruned += lolcal_chain_entry.tx.signatures.size();
    lolcal_chain_entry.tx.signatures.clear();
    //reassign to db
    m_db_transactions.set(h, lolcal_chain_entry);
    ++transactions_pruned;
  }
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::check_instance(const std::string& path)
{
  std::string locker_name = path + "/" + std::string(CURRENCY_CORE_INSTANCE_LOCK_FILE);
  bool r = epee::file_io_utils::open_and_lock_file(locker_name, m_locker_file);

  if (r)
    return true;
  else
  {
    LOG_ERROR("Failed to initialize db: some other instance is already running");
    return false;
  }
}

//------------------------------------------------------------------
bool blockchain_storage::prune_ring_signatures_if_need()
{
  CRITICAL_REGION_LOCAL(m_blockchain_lock);

  if (m_db_blocks.size() && m_checkpoints.get_top_checkpoint_height() && m_checkpoints.get_top_checkpoint_height() > m_db_current_pruned_rs_height)
  {
    LOG_PRINT_CYAN("Starting pruning ring signatues and attachments...", LOG_LEVEL_0);
    uint64_t tx_count = 0, sig_count = 0, attach_count = 0;
    for (uint64_t height = m_db_current_pruned_rs_height; height < m_db_blocks.size() && height <= m_checkpoints.get_top_checkpoint_height(); height++)
    {
      bool res = prune_ring_signatures(height, tx_count, sig_count);
      CHECK_AND_ASSERT_MES(res, false, "failed to prune_ring_signatures_and_attachments for height = " << height);
    }
    m_db_current_pruned_rs_height = m_checkpoints.get_top_checkpoint_height();
    LOG_PRINT_CYAN("Transaction pruning finished: " << sig_count << " signatures and " << attach_count << " attachments released in " << tx_count << " transactions.", LOG_LEVEL_0);
  }
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::clear()
{
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  m_db.begin_transaction();

  m_db_blocks.clear();
  m_db_blocks_index.clear();
  m_db_transactions.clear();
  m_db_spent_keys.clear();
  m_db_solo_options.clear();
  initialize_db_solo_options_values();
  m_db_outputs.clear();
  m_invalid_blocks.clear(); 
  m_db_aliases.clear();
  m_db_addr_to_alias.clear();
  m_scratchpad_wr.clear();
  m_db.commit_transaction();
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::reset_and_set_genesis_block(const block& b)
{
  clear();
  block_verification_context bvc = boost::value_initialized<block_verification_context>();
  add_new_block(b, bvc);
  if (!bvc.m_added_to_main_chain || bvc.m_verifivation_failed)
  {
    LOG_ERROR("Blockchain reset failed.")
      return false;
  }
  LOG_PRINT_GREEN("Blockchain reset. Genesis block: " << get_block_hash(b) << ", " << misc_utils::get_time_interval_string(time(nullptr) - b.timestamp) << " ago", LOG_LEVEL_0);
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::copy_scratchpad(std::vector<crypto::hash>& scr)
{
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  scr = m_scratchpad_wr.get_scratchpad();
  return true;
}
//------------------------------------------------------------------
// bool blockchain_storage::set_scratchpad(const std::vector<crypto::hash>& scr)
// {
//   CRITICAL_REGION_LOCAL(m_blockchain_lock);
//   m_db_scratchpad.clear();
//   for (size_t i = 0; i != scr.size(); i++)
//   {
//     m_db_scratchpad.push_back(scr[i]);
//   }
// 
//   return true;
// }
bool blockchain_storage::copy_scratchpad_as_blob(std::string& dst)
{

  //TODO here:
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  std::vector<crypto::hash> scr;
  copy_scratchpad(scr);

  if (scr.size())
  {
    dst.append(reinterpret_cast<const char*>(&scr[0]), scr.size() * 32);
  }
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::purge_transaction_keyimages_from_blockchain(const transaction& tx, bool strict_check)
{
  CRITICAL_REGION_LOCAL(m_blockchain_lock);

  struct purge_transaction_visitor : public boost::static_visitor<bool>
  {
    blockchain_storage& m_bcs;
    key_images_container& m_spent_keys;
    bool m_strict_check;
    purge_transaction_visitor(blockchain_storage& bcs, key_images_container& spent_keys, bool strict_check) :
      m_bcs(bcs),
      m_spent_keys(spent_keys),
      m_strict_check(strict_check){}

    bool operator()(const txin_to_key& inp) const
    {
      bool r = m_spent_keys.erase_validate(inp.k_image);
      CHECK_AND_ASSERT_MES(!(!r && m_strict_check), false, "purge_block_data_from_blockchain: key image in transaction not found");

      if (inp.key_offsets.size() == 1)
      {
        //direct spend detected
        if (!m_bcs.update_spent_tx_flags_for_input(inp.amount, inp.key_offsets[0], false))
        {
          //internal error
          LOG_PRINT_L0("Failed to  update_spent_tx_flags_for_input");
          return false;
        }
      }

      return true;
    }
    bool operator()(const txin_to_script& inp) const { return true; }
    bool operator()(const txin_to_scripthash& inp) const { return true; }
    bool operator()(const txin_gen& inp) const { return true; }
  };

  BOOST_FOREACH(const txin_v& in, tx.vin)
  {
    bool r = boost::apply_visitor(purge_transaction_visitor(*this, m_db_spent_keys, strict_check), in);
    CHECK_AND_ASSERT_MES(!strict_check || r, false, "failed to process purge_transaction_visitor");
  }
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::purge_transaction_from_blockchain(const crypto::hash& tx_id)
{
  CRITICAL_REGION_LOCAL(m_blockchain_lock);

  auto tx_res_ptr = m_db_transactions.find(tx_id);
  CHECK_AND_ASSERT_MES(tx_res_ptr != m_db_transactions.end(), false, "purge_block_data_from_blockchain: transaction not found in blockchain index!!");
  const transaction& tx = tx_res_ptr->tx;

  purge_transaction_keyimages_from_blockchain(tx, true);

  bool r = unprocess_blockchain_tx_extra(tx);
  CHECK_AND_ASSERT_MES(r, false, "failed to unprocess_blockchain_tx_extra");

  if (!is_coinbase(tx))
  {
    currency::tx_verification_context tvc = AUTO_VAL_INIT(tvc);
    bool r = m_tx_pool.add_tx(tx, tvc, true);
    CHECK_AND_ASSERT_MES(r, false, "purge_block_data_from_blockchain: failed to add transaction to transaction pool");
  }

  bool res = pop_transaction_from_global_index(tx, tx_id);
  CHECK_AND_ASSERT_MES(res, false, "Failed to pop_transaction_from_global_index");
  bool res_erase = m_db_transactions.erase_validate(tx_id);
  CHECK_AND_ASSERT_MES(res_erase, false, "Failed to m_transactions.erase with id = " << tx_id);

  LOG_PRINT_L1("Removed transaction from blockchain history:" << tx_id << ENDL);
  return res;
}
//------------------------------------------------------------------
bool blockchain_storage::purge_block_data_from_blockchain(const block& bl, size_t processed_tx_count)
{
  CRITICAL_REGION_LOCAL(m_blockchain_lock);

  bool res = true;
  CHECK_AND_ASSERT_MES(processed_tx_count <= bl.tx_hashes.size(), false, "wrong processed_tx_count in purge_block_data_from_blockchain");
  for (size_t count = 0; count != processed_tx_count; count++)
  {
    res = purge_transaction_from_blockchain(bl.tx_hashes[(processed_tx_count - 1) - count]) && res;
  }

  res = purge_transaction_from_blockchain(get_transaction_hash(bl.miner_tx)) && res;
  return res;
}
//------------------------------------------------------------------
crypto::hash blockchain_storage::get_top_block_id(uint64_t& height)
{
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  height = get_current_blockchain_height()-1;
  return get_top_block_id();
}
//------------------------------------------------------------------
crypto::hash blockchain_storage::get_top_block_id()
{
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  crypto::hash id = null_hash;
  if(m_db_blocks.size())
  {
    get_block_hash(m_db_blocks.back()->bl, id);
  }
  return id;
}
//------------------------------------------------------------------
bool blockchain_storage::get_top_block(block& b)
{
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  CHECK_AND_ASSERT_MES(m_db_blocks.size(), false, "Wrong blockchain state, m_blocks.size()=0!");
  auto val_ptr = m_db_blocks.back();
  CHECK_AND_ASSERT_MES(val_ptr.get(), false, "m_blocks.back() returned null");
  b = val_ptr->bl;
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::get_short_chain_history(std::list<crypto::hash>& ids)
{
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  size_t i = 0;
  size_t current_multiplier = 1;
  size_t sz = m_db_blocks.size();
  if (!sz)
    return true;
  size_t current_back_offset = 1;
  bool genesis_included = false;
  while (current_back_offset < sz)
  {
    ids.push_back(get_block_hash(m_db_blocks[sz - current_back_offset]->bl));
    if (sz - current_back_offset == 0)
      genesis_included = true;
    if (i < 10)
    {
      ++current_back_offset;
    }
    else
    {
      current_back_offset += current_multiplier *= 2;
    }
    ++i;
  }
  if (!genesis_included)
    ids.push_back(get_block_hash(m_db_blocks[0]->bl));

  return true;
}
//------------------------------------------------------------------
crypto::hash blockchain_storage::get_block_id_by_height(uint64_t height)
{
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  if (height >= m_db_blocks.size())
    return null_hash;

  return get_block_hash(m_db_blocks[height]->bl);
}
//------------------------------------------------------------------
bool blockchain_storage::get_block_by_hash(const crypto::hash &h, block &blk) {
  CRITICAL_REGION_LOCAL(m_blockchain_lock);

  // try to find block in main chain
  auto it = m_db_blocks_index.find(h);
  if (m_db_blocks_index.end() != it)
  {
    blk = m_db_blocks[*it]->bl;
    return true;
  }

  // try to find block in alternative chain
  blocks_ext_by_hash::const_iterator it_alt = m_alternative_chains.find(h);
  if (m_alternative_chains.end() != it_alt)
  {
    blk = it_alt->second.bl;
    return true;
  }

  return false;
}
//------------------------------------------------------------------
bool blockchain_storage::get_block_extended_info_by_hash(const crypto::hash &h, block_extended_info &blk) const
{
  CRITICAL_REGION_LOCAL(m_blockchain_lock);

  // try to find block in main chain
  auto vptr = m_db_blocks_index.find(h);
  if (vptr)
  {
    return get_block_extended_info_by_height(*vptr, blk);
  }

  // try to find block in alternative chain
  blocks_ext_by_hash::const_iterator it_alt = m_alternative_chains.find(h);
  if (m_alternative_chains.end() != it_alt)
  {
    blk = it_alt->second;
    return true;
  }

  return false;
}
//------------------------------------------------------------------
bool blockchain_storage::get_block_extended_info_by_height(uint64_t h, block_extended_info &blk) const
{
  CRITICAL_REGION_LOCAL(m_blockchain_lock);

  if (h >= m_db_blocks.size())
    return false;

  blk = *m_db_blocks[h];
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::get_block_by_height(uint64_t h, block &blk)
{
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  if (h >= m_db_blocks.size())
    return false;
  blk = m_db_blocks[h]->bl;
  return true;
}
//------------------------------------------------------------------
// void blockchain_storage::get_all_known_block_ids(std::list<crypto::hash> &main, std::list<crypto::hash> &alt, std::list<crypto::hash> &invalid) {
//   CRITICAL_REGION_LOCAL(m_blockchain_lock);
// 
//   BOOST_FOREACH(blocks_by_id_index::value_type &v, m_db_blocks_index)
//     main.push_back(v.first);
// 
//   BOOST_FOREACH(blocks_ext_by_hash::value_type &v, m_alternative_chains)
//     alt.push_back(v.first);
// 
//   BOOST_FOREACH(blocks_ext_by_hash::value_type &v, m_db_invalid_blocks)
//     invalid.push_back(v.first);
// }

//------------------------------------------------------------------
bool blockchain_storage::rollback_blockchain_switching(std::list<block>& original_chain, size_t rollback_height)
{
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  //remove failed subchain
  for (size_t i = m_db_blocks.size() - 1; i >= rollback_height; i--)
  {
    bool r = pop_block_from_blockchain();
    CHECK_AND_ASSERT_MES(r, false, "PANIC!!! failed to remove block while chain switching during the rollback!");
  }
  //return back original chain
  BOOST_FOREACH(auto& bl, original_chain)
  {
    block_verification_context bvc = boost::value_initialized<block_verification_context>();
    bool r = handle_block_to_main_chain(bl, bvc);
    CHECK_AND_ASSERT_MES(r && bvc.m_added_to_main_chain, false, "PANIC!!! failed to add (again) block while chain switching during the rollback!");
  }

  LOG_PRINT_L0("Rollback success.");
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::switch_to_alternative_blockchain(std::list<blocks_ext_by_hash::iterator>& alt_chain)
{
  CRITICAL_REGION_LOCAL(m_blockchain_lock);

  CHECK_AND_ASSERT_MES(alt_chain.size(), false, "switch_to_alternative_blockchain: empty chain passed");

  size_t split_height = alt_chain.front()->second.height;
  CHECK_AND_ASSERT_MES(m_db_blocks.size() > split_height, false, "switch_to_alternative_blockchain: blockchain size is lower than split height");

  //disconnecting old chain
  std::list<block> disconnected_chain;
  for (size_t i = m_db_blocks.size() - 1; i >= split_height; i--)
  {
    block b = m_db_blocks[i]->bl;
    bool r = pop_block_from_blockchain();
    CHECK_AND_ASSERT_MES(r, false, "failed to remove block on chain switching");
    disconnected_chain.push_front(b);
  }

  //connecting new alternative chain
  for (auto alt_ch_iter = alt_chain.begin(); alt_ch_iter != alt_chain.end(); alt_ch_iter++)
  {
    auto ch_ent = *alt_ch_iter;
    block_verification_context bvc = boost::value_initialized<block_verification_context>();
    bool r = handle_block_to_main_chain(ch_ent->second.bl, bvc);
    if (!r || !bvc.m_added_to_main_chain)
    {
      LOG_PRINT_L0("Failed to switch to alternative blockchain");
      rollback_blockchain_switching(disconnected_chain, split_height);
      add_block_as_invalid(ch_ent->second, get_block_hash(ch_ent->second.bl));
      LOG_PRINT_L0("The block was inserted as invalid while connecting new alternative chain,  block_id: " << get_block_hash(ch_ent->second.bl));
      m_alternative_chains.erase(ch_ent);

      for (auto alt_ch_to_orph_iter = ++alt_ch_iter; alt_ch_to_orph_iter != alt_chain.end(); alt_ch_to_orph_iter++)
      {
        //block_verification_context bvc = boost::value_initialized<block_verification_context>();
        add_block_as_invalid((*alt_ch_iter)->second, (*alt_ch_iter)->first);
        m_alternative_chains.erase(*alt_ch_to_orph_iter);
      }
      return false;
    }
  }

  //pushing old chain as alternative chain
  BOOST_FOREACH(auto& old_ch_ent, disconnected_chain)
  {
    block_verification_context bvc = boost::value_initialized<block_verification_context>();
    bool r = handle_alternative_block(old_ch_ent, get_block_hash(old_ch_ent), bvc);
    if (!r)
    {
      LOG_ERROR("Failed to push ex-main chain blocks to alternative chain ");
      rollback_blockchain_switching(disconnected_chain, split_height);
      return false;
    }
  }

  //removing all_chain entries from alternative chain
  BOOST_FOREACH(auto ch_ent, alt_chain)
  {
    m_alternative_chains.erase(ch_ent);
  }

  LOG_PRINT_GREEN("REORGANIZE SUCCESS! on height: " << split_height << ", new blockchain size: " << m_db_blocks.size(), LOG_LEVEL_0);
  return true;
}
//------------------------------------------------------------------
//------------------------------------------------------------------
wide_difficulty_type blockchain_storage::get_difficulty_for_next_block()
{
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  std::vector<uint64_t> timestamps;
  std::vector<wide_difficulty_type> commulative_difficulties;
  size_t offset = m_db_blocks.size() - std::min(m_db_blocks.size(), static_cast<size_t>(DIFFICULTY_BLOCKS_COUNT));
  if (!offset)
    ++offset;//skip genesis block
  for (; offset < m_db_blocks.size(); offset++)
  {
    timestamps.push_back(m_db_blocks[offset]->bl.timestamp);
    commulative_difficulties.push_back(m_db_blocks[offset]->cumulative_difficulty);
  }
  return next_difficulty(timestamps, commulative_difficulties);
}
//------------------------------------------------------------------


wide_difficulty_type blockchain_storage::get_next_difficulty_for_alternative_chain(const std::list<blocks_ext_by_hash::iterator>& alt_chain, block_extended_info& bei)
{
  std::vector<uint64_t> timestamps;
  std::vector<wide_difficulty_type> commulative_difficulties;
  if (alt_chain.size()< DIFFICULTY_BLOCKS_COUNT)
  {
    CRITICAL_REGION_LOCAL(m_blockchain_lock);
    size_t main_chain_stop_offset = alt_chain.size() ? alt_chain.front()->second.height : bei.height;
    size_t main_chain_count = DIFFICULTY_BLOCKS_COUNT - std::min(static_cast<size_t>(DIFFICULTY_BLOCKS_COUNT), alt_chain.size());
    main_chain_count = std::min(main_chain_count, main_chain_stop_offset);
    size_t main_chain_start_offset = main_chain_stop_offset - main_chain_count;

    if (!main_chain_start_offset)
      ++main_chain_start_offset; //skip genesis block
    for (; main_chain_start_offset < main_chain_stop_offset; ++main_chain_start_offset)
    {
      timestamps.push_back(m_db_blocks[main_chain_start_offset]->bl.timestamp);
      commulative_difficulties.push_back(m_db_blocks[main_chain_start_offset]->cumulative_difficulty);
    }

    CHECK_AND_ASSERT_MES((alt_chain.size() + timestamps.size()) <= DIFFICULTY_BLOCKS_COUNT, false, "Internal error, alt_chain.size()[" << alt_chain.size()
      << "] + vtimestampsec.size()[" << timestamps.size() << "] NOT <= DIFFICULTY_WINDOW[]" << DIFFICULTY_BLOCKS_COUNT);
    BOOST_FOREACH(auto it, alt_chain)
    {
      timestamps.push_back(it->second.bl.timestamp);
      commulative_difficulties.push_back(it->second.cumulative_difficulty);
    }
  }
  else
  {
    timestamps.resize(std::min(alt_chain.size(), static_cast<size_t>(DIFFICULTY_BLOCKS_COUNT)));
    commulative_difficulties.resize(std::min(alt_chain.size(), static_cast<size_t>(DIFFICULTY_BLOCKS_COUNT)));
    size_t count = 0;
    size_t max_i = timestamps.size() - 1;
    BOOST_REVERSE_FOREACH(auto it, alt_chain)
    {
      timestamps[max_i - count] = it->second.bl.timestamp;
      commulative_difficulties[max_i - count] = it->second.cumulative_difficulty;
      count++;
      if (count >= DIFFICULTY_BLOCKS_COUNT)
        break;
    }
  }
  return next_difficulty(timestamps, commulative_difficulties);
}
//------------------------------------------------------------------
bool blockchain_storage::prevalidate_miner_transaction(const block& b, uint64_t height)
{
  CHECK_AND_ASSERT_MES(b.miner_tx.vin.size() == 1, false, "coinbase transaction in the block has no inputs");
  CHECK_AND_ASSERT_MES(b.miner_tx.vin[0].type() == typeid(txin_gen), false, "coinbase transaction in the block has the wrong type");
  if (boost::get<txin_gen>(b.miner_tx.vin[0]).height != height)
  {
    LOG_PRINT_RED_L0("The miner transaction in block has invalid height: " << boost::get<txin_gen>(b.miner_tx.vin[0]).height << ", expected: " << height);
    return false;
  }
  CHECK_AND_ASSERT_MES(b.miner_tx.unlock_time == height + CURRENCY_MINED_MONEY_UNLOCK_WINDOW,
    false,
    "coinbase transaction transaction have wrong unlock time=" << b.miner_tx.unlock_time << ", expected " << height + CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  //check outs overflow
  if (!check_outs_overflow(b.miner_tx))
  {
    LOG_PRINT_RED_L0("miner transaction have money overflow in block " << get_block_hash(b));
    return false;
  }

  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::lookfor_donation(const transaction& tx, uint64_t& donation, uint64_t& royalty)
{
  crypto::public_key coin_base_tx_pub_key = null_pkey;
  bool r = parse_and_validate_tx_extra(tx, coin_base_tx_pub_key);
  CHECK_AND_ASSERT_MES(r, false, "Failed to validate coinbase extra");


  if (tx.vout.size() >= 2)
  {
    //check donations value
    size_t i = tx.vout.size() - 2;
    CHECK_AND_ASSERT_MES(tx.vout[i].target.type() == typeid(txout_to_key), false, "wrong type id in transaction out");
    if (is_out_to_acc(m_donations_account, boost::get<txout_to_key>(tx.vout[i].target), coin_base_tx_pub_key, i))
    {
      donation = tx.vout[i].amount;
    }
  }
  if (tx.vout.size() >= 1)
  {
    size_t i = tx.vout.size() - 1;
    CHECK_AND_ASSERT_MES(tx.vout[i].target.type() == typeid(txout_to_key), false, "wrong type id in transaction out");
    if (donation)
    {
      //donations already found, check royalty
      if (is_out_to_acc(m_royalty_account, boost::get<txout_to_key>(tx.vout[i].target), coin_base_tx_pub_key, i))
      {
        royalty = tx.vout[i].amount;
      }
    }
    else
    {
      //only donations, without royalty 
      if (is_out_to_acc(m_donations_account, boost::get<txout_to_key>(tx.vout[i].target), coin_base_tx_pub_key, i))
      {
        donation = tx.vout[i].amount;
      }
    }
  }
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::get_required_donations_value_for_next_block(uint64_t& don_am)
{
  TRY_ENTRY();
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  uint64_t sz = get_current_blockchain_height();
  if (sz < CURRENCY_DONATIONS_INTERVAL || sz%CURRENCY_DONATIONS_INTERVAL)
  {
    LOG_ERROR("internal error: validate_donations_value at wrong height: " << get_current_blockchain_height());
    return false;
  }
  std::vector<bool> donation_votes;
  for (size_t i = m_db_blocks.size() - CURRENCY_DONATIONS_INTERVAL; i != m_db_blocks.size(); i++)
    donation_votes.push_back(!(m_db_blocks[i]->bl.flags&BLOCK_FLAGS_SUPPRESS_DONATION));

  don_am = get_donations_anount_for_day(m_db_blocks.back()->already_donated_coins, donation_votes);
  return true;

  CATCH_ENTRY_L0("blockchain_storage::validate_donations_value", false);
}
//------------------------------------------------------------------
bool blockchain_storage::validate_donations_value(uint64_t donation, uint64_t royalty)
{
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  uint64_t expected_don_total = 0;
  if (!get_required_donations_value_for_next_block(expected_don_total))
    return false;

  CHECK_AND_ASSERT_MES(donation + royalty == expected_don_total, false, "Wrong donations amount: " << donation + royalty << ", expected " << expected_don_total);
  uint64_t expected_donation = 0;
  uint64_t expected_royalty = 0;
  get_donation_parts(expected_don_total, expected_royalty, expected_donation);
  CHECK_AND_ASSERT_MES(expected_royalty == royalty && expected_donation == donation,
    false,
    "wrong donation parts: " << donation << "/" << royalty << ", expected: " << expected_donation << "/" << expected_royalty);
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::validate_miner_transaction(const block& b, size_t cumulative_block_size, uint64_t fee, uint64_t& base_reward, uint64_t already_generated_coins, uint64_t already_donated_coins, uint64_t& donation_total)
{
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  //validate reward
  uint64_t money_in_use = 0;
  uint64_t royalty = 0;
  uint64_t donation = 0;

  //donations should be last one 
  //size_t outs_total = b.miner_tx.vout.size();
  BOOST_FOREACH(auto& o, b.miner_tx.vout)
  {
    money_in_use += o.amount;
  }
  uint64_t h = get_block_height(b);

  //once a day, without
  if (h && !(h%CURRENCY_DONATIONS_INTERVAL) /*&& h > 21600*/)
  {
    bool r = lookfor_donation(b.miner_tx, donation, royalty);
    CHECK_AND_ASSERT_MES(r, false, "Failed to lookfor_donation");

    r = validate_donations_value(donation, royalty);
    CHECK_AND_ASSERT_MES(r, false, "Failed to validate donations value");

    money_in_use -= donation + royalty;
  }


  std::vector<size_t> last_blocks_sizes;
  get_last_n_blocks_sizes(last_blocks_sizes, CURRENCY_REWARD_BLOCKS_WINDOW);
  uint64_t max_donation = 0;
  if (!get_block_reward(misc_utils::median(last_blocks_sizes), cumulative_block_size, already_generated_coins, already_donated_coins, base_reward, max_donation))
  {
    LOG_PRINT_L0("block size " << cumulative_block_size << " is bigger than allowed for this blockchain");
    return false;
  }
  if (base_reward + fee < money_in_use)
  {
    LOG_ERROR("coinbase transaction spend too much money (" << print_money(money_in_use) << "). Block reward is " << print_money(base_reward + fee) << "(" << print_money(base_reward) << "+" << print_money(fee) << ")");
    return false;
  }
  if (base_reward + fee != money_in_use)
  {
    LOG_ERROR("coinbase transaction doesn't use full amount of block reward:  spent: "
      << print_money(money_in_use) << ",  block reward " << print_money(base_reward + fee) << "(" << print_money(base_reward) << "+" << print_money(fee) << ")");
    return false;
  }

  donation_total = royalty + donation;
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::get_backward_blocks_sizes(size_t from_height, std::vector<size_t>& sz, size_t count)
{
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  CHECK_AND_ASSERT_MES(from_height < m_db_blocks.size(), false, "Internal error: get_backward_blocks_sizes called with from_height=" << from_height << ", blockchain height = " << m_db_blocks.size());

  size_t start_offset = (from_height + 1) - std::min((from_height + 1), count);
  for (size_t i = start_offset; i != from_height + 1; i++)
    sz.push_back(m_db_blocks[i]->block_cumulative_size);

  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::get_last_n_blocks_sizes(std::vector<size_t>& sz, size_t count)
{
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  if (!m_db_blocks.size())
    return true;
  return get_backward_blocks_sizes(m_db_blocks.size() - 1, sz, count);
}
//------------------------------------------------------------------
uint64_t blockchain_storage::get_current_comulative_blocksize_limit()
{
  return m_db_current_block_cumul_sz_limit;
}
//------------------------------------------------------------------
uint64_t blockchain_storage::get_already_generated_coins(crypto::hash &hash, uint64_t &count)
{
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  auto it = m_db_blocks_index.find(hash);
  if (m_db_blocks_index.end() != it) {
    count = m_db_blocks[*it]->already_generated_coins;
    return true;
  }
  return false;
}
//------------------------------------------------------------------
uint64_t blockchain_storage::get_already_donated_coins(crypto::hash &hash, uint64_t &count)
{
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  auto it = m_db_blocks_index.find(hash);
  if (m_db_blocks_index.end() != it) {
    count = m_db_blocks[*it]->already_donated_coins;
    return true;
  }
  return false;
}
//------------------------------------------------------------------
bool blockchain_storage::get_block_containing_tx(const crypto::hash &txId, crypto::hash &blockId, uint64_t &blockHeight)
{
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  auto it = m_db_transactions.find(txId);
  if (!it) {
    return false;
  } else {
    blockHeight = m_db_blocks[it->m_keeper_block_height]->height;
    blockId = get_block_id_by_height(blockHeight);
    return true;
  }
}

//------------------------------------------------------------------
bool blockchain_storage::create_block_template(block& b, const account_public_address& miner_address, wide_difficulty_type& diffic, uint64_t& height, const blobdata& ex_nonce, bool vote_for_donation, const alias_info& ai)
{
  size_t median_size;
  uint64_t already_generated_coins;
  uint64_t already_donated_coins;
  uint64_t donation_amount_for_this_block = 0;

  CRITICAL_REGION_BEGIN(m_blockchain_lock);
  b.major_version = CURRENT_BLOCK_MAJOR_VERSION;
  b.minor_version = CURRENT_BLOCK_MINOR_VERSION;
  b.prev_id = get_top_block_id();
  b.timestamp = time(NULL);
  b.flags = 0;
  if (!vote_for_donation)
    b.flags = BLOCK_FLAGS_SUPPRESS_DONATION;
  height = m_db_blocks.size();
  diffic = get_difficulty_for_next_block();
  if (!(height%CURRENCY_DONATIONS_INTERVAL))
    get_required_donations_value_for_next_block(donation_amount_for_this_block);
  CHECK_AND_ASSERT_MES(diffic, false, "difficulty owverhead.");

  median_size = m_db_current_block_cumul_sz_limit / 2;
  already_generated_coins = m_db_blocks.back()->already_generated_coins;
  already_donated_coins = m_db_blocks.back()->already_donated_coins;

  CRITICAL_REGION_END();

  size_t txs_size;
  uint64_t fee;
  if (!m_tx_pool.fill_block_template(b, median_size, already_generated_coins, already_donated_coins, txs_size, fee)) {
    return false;
  }
  /*
  two-phase miner transaction generation: we don't know exact block size until we prepare block, but we don't know reward until we know
  block size, so first miner transaction generated with fake amount of money, and with phase we know think we know expected block size
  */
  //make blocks coin-base tx looks close to real coinbase tx to get truthful blob size
  bool r = construct_miner_tx(height, median_size, already_generated_coins, already_donated_coins,
    txs_size,
    fee,
    miner_address,
    m_donations_account.m_account_address,
    m_royalty_account.m_account_address,
    b.miner_tx, ex_nonce,
    11, donation_amount_for_this_block, ai);
  CHECK_AND_ASSERT_MES(r, false, "Failed to construc miner tx, first chance");
#ifdef _DEBUG
  std::list<size_t> try_val;
  try_val.push_back(get_object_blobsize(b.miner_tx));
#endif

  size_t cumulative_size = txs_size + get_object_blobsize(b.miner_tx);
  for (size_t try_count = 0; try_count != 10; ++try_count) {
    r = construct_miner_tx(height, median_size, already_generated_coins, already_donated_coins,
      cumulative_size,
      fee,
      miner_address,
      m_donations_account.m_account_address,
      m_royalty_account.m_account_address,
      b.miner_tx, ex_nonce,
      11,
      donation_amount_for_this_block, ai);
#ifdef _DEBUG
    try_val.push_back(get_object_blobsize(b.miner_tx));
#endif

    CHECK_AND_ASSERT_MES(r, false, "Failed to construc miner tx, second chance");
    size_t coinbase_blob_size = get_object_blobsize(b.miner_tx);
    if (coinbase_blob_size > cumulative_size - txs_size) {
      cumulative_size = txs_size + coinbase_blob_size;
      continue;
    }

    if (coinbase_blob_size < cumulative_size - txs_size) {
      size_t delta = cumulative_size - txs_size - coinbase_blob_size;
      b.miner_tx.extra.insert(b.miner_tx.extra.end(), delta, 0);
      //here  could be 1 byte difference, because of extra field counter is varint, and it can become from 1-byte len to 2-bytes len.
      if (cumulative_size != txs_size + get_object_blobsize(b.miner_tx)) {
        CHECK_AND_ASSERT_MES(cumulative_size + 1 == txs_size + get_object_blobsize(b.miner_tx), false, "unexpected case: cumulative_size=" << cumulative_size << " + 1 is not equal txs_cumulative_size=" << txs_size << " + get_object_blobsize(b.miner_tx)=" << get_object_blobsize(b.miner_tx));
        b.miner_tx.extra.resize(b.miner_tx.extra.size() - 1);
        if (cumulative_size != txs_size + get_object_blobsize(b.miner_tx)) {
          //fuck, not lucky, -1 makes varint-counter size smaller, in that case we continue to grow with cumulative_size
          LOG_PRINT_RED("Miner tx creation have no luck with delta_extra size = " << delta << " and " << delta - 1, LOG_LEVEL_2);
          cumulative_size += delta - 1;
          continue;
        }
        LOG_PRINT_GREEN("Setting extra for block: " << b.miner_tx.extra.size() << ", try_count=" << try_count, LOG_LEVEL_1);
      }
    }
    CHECK_AND_ASSERT_MES(cumulative_size == txs_size + get_object_blobsize(b.miner_tx), false, "unexpected case: cumulative_size=" << cumulative_size << " is not equal txs_cumulative_size=" << txs_size << " + get_object_blobsize(b.miner_tx)=" << get_object_blobsize(b.miner_tx));
    return true;
  }
  LOG_ERROR("Failed to create_block_template with " << 10 << " tries");
  return false;
}
// ------------------------------------------------------------------
// bool blockchain_storage::print_transactions_statistics()
// {
//   CRITICAL_REGION_LOCAL(m_blockchain_lock);
//   LOG_PRINT_L0("Started to collect transaction statistics, pleas wait...");
//   size_t total_count = 0;
//   size_t coinbase_count = 0;
//   size_t total_full_blob = 0;
//   size_t total_cropped_blob = 0;
//   for(auto tx_entry: m_db_transactions)
//   {
//     ++total_count;
//     if(is_coinbase(tx_entry.second.tx))
//       ++coinbase_count;
//     else
//     {
//       total_full_blob += get_object_blobsize<transaction>(tx_entry.second.tx);
//       transaction tx = tx_entry.second.tx;
//       tx.signatures.clear();
//       total_cropped_blob += get_object_blobsize<transaction>(tx);
//     }    
//   }
//   LOG_PRINT_L0("Done" << ENDL
//       << "total transactions: " << total_count << ENDL 
//       << "coinbase transactions: " << coinbase_count << ENDL 
//       << "avarage size of transaction: " << total_full_blob/(total_count-coinbase_count) << ENDL
//       << "avarage size of transaction without ring signatures: " << total_cropped_blob/(total_count-coinbase_count) << ENDL
//       );
//   return true;
// }
//------------------------------------------------------------------
bool blockchain_storage::complete_timestamps_vector(uint64_t start_top_height, std::vector<uint64_t>& timestamps)
{

  if (timestamps.size() >= BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW)
    return true;

  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  size_t need_elements = BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW - timestamps.size();
  CHECK_AND_ASSERT_MES(start_top_height < m_db_blocks.size(), false, "internal error: passed start_height = " << start_top_height << " not less then m_blocks.size()=" << m_db_blocks.size());
  size_t stop_offset = start_top_height > need_elements ? start_top_height - need_elements : 0;
  do
  {
    timestamps.push_back(m_db_blocks[start_top_height]->bl.timestamp);
    if (start_top_height == 0)
      break;
    --start_top_height;
  } while (start_top_height != stop_offset);
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::handle_alternative_block(const block& b, const crypto::hash& id, block_verification_context& bvc)
{
  if (m_checkpoints.is_height_passed_zone(get_block_height(b), get_current_blockchain_height() - 1))
  {
    LOG_PRINT_RED_L0("Block with id: " << id << "[" << get_block_height(b) << "]" << ENDL << " for alternative chain, is under checkpoint zone, declined");
    bvc.m_verifivation_failed = true;
    return false;
  }

  TRY_ENTRY();
  CRITICAL_REGION_LOCAL(m_blockchain_lock);

  //block is not related with head of main chain
  //first of all - look in alternative chains container
  auto it_main_prev = m_db_blocks_index.find(b.prev_id);
  auto it_prev = m_alternative_chains.find(b.prev_id);
  if (it_prev != m_alternative_chains.end() || it_main_prev != m_db_blocks_index.end())
  {
    //we have new block in alternative chain
    //build alternative subchain, front -> mainchain, back -> alternative head
    blocks_ext_by_hash::iterator alt_it = it_prev; //m_alternative_chains.find()
    std::list<blocks_ext_by_hash::iterator> alt_chain;
    std::vector<crypto::hash> alt_scratchppad;
    std::map<uint64_t, crypto::hash> alt_scratchppad_patch;
    std::vector<uint64_t> timestamps;
    while (alt_it != m_alternative_chains.end())
    {
      alt_chain.push_front(alt_it);
      timestamps.push_back(alt_it->second.bl.timestamp);
      alt_it = m_alternative_chains.find(alt_it->second.bl.prev_id);
    }

    if (alt_chain.size())
    {
      //make sure that it has right connection to main chain
      CHECK_AND_ASSERT_MES(m_db_blocks.size() > alt_chain.front()->second.height, false, "main blockchain wrong height");
      crypto::hash h = null_hash;
      get_block_hash(m_db_blocks[alt_chain.front()->second.height - 1]->bl, h);
      CHECK_AND_ASSERT_MES(h == alt_chain.front()->second.bl.prev_id, false, "alternative chain have wrong connection to main chain");
      complete_timestamps_vector(alt_chain.front()->second.height - 1, timestamps);
      //build alternative scratchpad
      for (auto& ach : alt_chain)
      {
        if (!push_block_scratchpad_data(ach->second.scratch_offset, ach->second.bl, alt_scratchppad, alt_scratchppad_patch))
        {
          LOG_PRINT_RED_L0("Block with id: " << id
            << ENDL << " for alternative chain, have invalid data");
          bvc.m_verifivation_failed = true;
          return false;
        }
      }
    }
    else
    {
      CHECK_AND_ASSERT_MES(it_main_prev != m_db_blocks_index.end(), false, "internal error: broken imperative condition it_main_prev != m_blocks_index.end()");
      complete_timestamps_vector(*it_main_prev, timestamps);
    }
    //check timestamp correct
    if (!check_block_timestamp(timestamps, b))
    {
      LOG_PRINT_RED_L0("Block with id: " << id
        << ENDL << " for alternative chain, have invalid timestamp: " << b.timestamp);
      //add_block_as_invalid(b, id);//do not add blocks to invalid storage before proof of work check was passed
      bvc.m_verifivation_failed = true;
      return false;
    }

    block_extended_info bei = boost::value_initialized<block_extended_info>();
    bei.bl = b;
    bei.height = alt_chain.size() ? it_prev->second.height + 1 : *it_main_prev + 1;
    uint64_t connection_height = alt_chain.size() ? alt_chain.front()->second.height : bei.height;
    CHECK_AND_ASSERT_MES(connection_height, false, "INTERNAL ERROR: Wrong connection_height==0 in handle_alternative_block");
    bei.scratch_offset = m_db_blocks[connection_height]->scratch_offset + alt_scratchppad.size();
    CHECK_AND_ASSERT_MES(bei.scratch_offset, false, "INTERNAL ERROR: Wrong bei.scratch_offset==0 in handle_alternative_block");

    //lets collect patchs from main line
    std::map<uint64_t, crypto::hash> main_line_patches;
    for (uint64_t i = connection_height; i != m_db_blocks.size(); i++)
    {
      std::vector<crypto::hash> block_addendum;
      bool res = get_block_scratchpad_addendum(m_db_blocks[i]->bl, block_addendum);
      CHECK_AND_ASSERT_MES(res, false, "Failed to get_block_scratchpad_addendum for alt block");
      get_scratchpad_patch(m_db_blocks[i]->scratch_offset, 
        0, 
        block_addendum.size(),
        block_addendum,
        main_line_patches);
    }
    //apply only that patches that lay under alternative scratchpad offset
    for (auto ml : main_line_patches)
    {
      if (ml.first < m_db_blocks[connection_height]->scratch_offset)
        alt_scratchppad_patch[ml.first] = crypto::xor_pod(alt_scratchppad_patch[ml.first], ml.second);
    }

    wide_difficulty_type current_diff = get_next_difficulty_for_alternative_chain(alt_chain, bei);
    CHECK_AND_ASSERT_MES(current_diff, false, "!!!!!!! DIFFICULTY OVERHEAD !!!!!!!");
    crypto::hash proof_of_work = null_hash;
    // POW
#ifdef ENABLE_HASHING_DEBUG
    size_t call_no = 0;
    std::stringstream ss;      
#endif
    get_block_longhash(bei.bl, proof_of_work, bei.height, [&](uint64_t index) -> crypto::hash
    {
      uint64_t offset = index%bei.scratch_offset;
      crypto::hash res;
      if(offset >= m_db_blocks[connection_height]->scratch_offset)
      {
        res = alt_scratchppad[offset - m_db_blocks[connection_height]->scratch_offset];
      }
      else
      {
        res = m_scratchpad_wr.get_scratchpad()[offset];
      }
      auto it = alt_scratchppad_patch.find(offset);
      if (it != alt_scratchppad_patch.end())
      {//apply patch
        res = crypto::xor_pod(res, it->second);
      }
#ifdef ENABLE_HASHING_DEBUG
      ss << "[" << call_no << "][" << index << "%" << bei.scratch_offset << "(" << index%bei.scratch_offset << ")]" << res << ENDL;
      ++call_no;
#endif
      return res;
    });
#ifdef ENABLE_HASHING_DEBUG
    LOG_PRINT_L3("ID: " << get_block_hash(bei.bl) << "[" << bei.height << "]" << ENDL << "POW:" << proof_of_work << ENDL << ss.str());
#endif
    if (!check_hash(proof_of_work, current_diff))
    {
      LOG_PRINT_RED_L0("Block with id: " << id
        << ENDL << " for alternative chain, have not enough proof of work: " << proof_of_work
        << ENDL << " expected difficulty: " << current_diff);
      bvc.m_verifivation_failed = true;
      return false;
    }
    //

    if (!m_checkpoints.is_in_checkpoint_zone(bei.height))
    {
      m_is_in_checkpoint_zone = false;
    }
    else
    {
      m_is_in_checkpoint_zone = true;
      if (!m_checkpoints.check_block(bei.height, id))
      {
        LOG_ERROR("CHECKPOINT VALIDATION FAILED");
        bvc.m_verifivation_failed = true;
        return false;
      }
    }

    if (!prevalidate_miner_transaction(b, bei.height))
    {
      LOG_PRINT_RED_L0("Block with id: " << string_tools::pod_to_hex(id)
        << " (as alternative) have wrong miner transaction.");
      bvc.m_verifivation_failed = true;
      return false;

    }

    bei.cumulative_difficulty = alt_chain.size() ? it_prev->second.cumulative_difficulty : m_db_blocks[*it_main_prev]->cumulative_difficulty;
    bei.cumulative_difficulty += current_diff;

#ifdef _DEBUG
    auto i_dres = m_alternative_chains.find(id);
    CHECK_AND_ASSERT_MES(i_dres == m_alternative_chains.end(), false, "insertion of new alternative block returned as it already exist");
#endif
    auto i_res = m_alternative_chains.insert(blocks_ext_by_hash::value_type(id, bei));
    CHECK_AND_ASSERT_MES(i_res.second, false, "insertion of new alternative block returned as it already exist");
    alt_chain.push_back(i_res.first);
    //check if difficulty bigger then in main chain
    if (m_db_blocks.back()->cumulative_difficulty < bei.cumulative_difficulty)
    {
      //do reorganize!
      LOG_PRINT_GREEN("###### REORGANIZE on height: " << alt_chain.front()->second.height << " of " << m_db_blocks.size() - 1 << " with cum_difficulty " << m_db_blocks.back()->cumulative_difficulty
        << ENDL << " alternative blockchain size: " << alt_chain.size() << " with cum_difficulty " << bei.cumulative_difficulty, LOG_LEVEL_0);
      bool r = switch_to_alternative_blockchain(alt_chain);
      if (r) bvc.m_added_to_main_chain = true;
      else bvc.m_verifivation_failed = true;
      return r;
    }
    LOG_PRINT_BLUE("----- BLOCK ADDED AS ALTERNATIVE ON HEIGHT " << bei.height
      << ENDL << "id:\t" << id
      << ENDL << "PoW:\t" << proof_of_work
      << ENDL << "difficulty:\t" << current_diff, LOG_LEVEL_0);
    return true;
  }
  else
  {
    //block orphaned
    bvc.m_marked_as_orphaned = true;
    LOG_PRINT_RED_L0("Block recognized as orphaned and rejected, id = " << id);
  }

  return true;
  CATCH_ENTRY_L0("blockchain_storage::handle_alternative_block", false);
}
//------------------------------------------------------------------
bool blockchain_storage::get_blocks(uint64_t start_offset, size_t count, std::list<block>& blocks, std::list<transaction>& txs)
{
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  if (start_offset >= m_db_blocks.size())
    return false;
  for (size_t i = start_offset; i < start_offset + count && i < m_db_blocks.size(); i++)
  {
    blocks.push_back(m_db_blocks[i]->bl);
    std::list<crypto::hash> missed_ids;
    get_transactions(m_db_blocks[i]->bl.tx_hashes, txs, missed_ids);
    CHECK_AND_ASSERT_MES(!missed_ids.size(), false, "have missed transactions in own block in main blockchain");
  }

  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::get_blocks(uint64_t start_offset, size_t count, std::list<block>& blocks)
{
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  if (start_offset >= m_db_blocks.size())
    return false;

  for (size_t i = start_offset; i < start_offset + count && i < m_db_blocks.size(); i++)
    blocks.push_back(m_db_blocks[i]->bl);
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::handle_get_objects(NOTIFY_REQUEST_GET_OBJECTS::request& arg, NOTIFY_RESPONSE_GET_OBJECTS::request& rsp)
{
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  rsp.current_blockchain_height = get_current_blockchain_height();
  std::list<block> blocks;
  get_blocks(arg.blocks, blocks, rsp.missed_ids);

  BOOST_FOREACH(const auto& bl, blocks)
  {
    std::list<crypto::hash> missed_tx_id;
    std::list<transaction> txs;
    get_transactions(bl.tx_hashes, txs, rsp.missed_ids);
    CHECK_AND_ASSERT_MES(!missed_tx_id.size(), false, "Internal error: have missed missed_tx_id.size()=" << missed_tx_id.size()
      << ENDL << "for block id = " << get_block_hash(bl));
    rsp.blocks.push_back(block_complete_entry());
    block_complete_entry& e = rsp.blocks.back();
    //pack block
    e.block = t_serializable_object_to_blob(bl);
    //pack transactions
    BOOST_FOREACH(transaction& tx, txs)
      e.txs.push_back(t_serializable_object_to_blob(tx));

  }
  //get another transactions, if need
  std::list<transaction> txs;
  get_transactions(arg.txs, txs, rsp.missed_ids);
  //pack aside transactions
  BOOST_FOREACH(const auto& tx, txs)
    rsp.txs.push_back(t_serializable_object_to_blob(tx));

  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::get_transactions_daily_stat(uint64_t& daily_cnt, uint64_t& daily_volume)
{
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  daily_cnt = daily_volume = 0;
  for (size_t i = (m_db_blocks.size() > CURRENCY_BLOCK_PER_DAY ? m_db_blocks.size() - CURRENCY_BLOCK_PER_DAY : 0); i != m_db_blocks.size(); i++)
  {
    auto ptr = m_db_blocks[i];
    for (auto& h : ptr->bl.tx_hashes)
    {
      ++daily_cnt;
      auto tx_ptr = m_db_transactions.find(h);
      CHECK_AND_ASSERT_MES(tx_ptr, false, "Wrong transaction hash " << h << " in block on height " << i);
      uint64_t am = 0;
      bool r = get_inputs_money_amount(tx_ptr->tx, am);
      CHECK_AND_ASSERT_MES(r, false, "failed to get_inputs_money_amount");
      daily_volume += am;
    }
  }
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::check_keyimages(const std::list<crypto::key_image>& images, std::list<bool>& images_stat)
{
  //true - unspent, false - spent
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  for (auto& ki : images)
  {
    images_stat.push_back(m_db_spent_keys.count(ki) ? false : true);
  }
  return true;
}
//------------------------------------------------------------------
uint64_t blockchain_storage::get_current_hashrate(size_t aprox_count)
{
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  if (m_db_blocks.size() <= aprox_count)
    return 0;

  wide_difficulty_type w_hr = (m_db_blocks.back()->cumulative_difficulty - m_db_blocks[m_db_blocks.size() - aprox_count]->cumulative_difficulty) /
    (m_db_blocks.back()->bl.timestamp - m_db_blocks[m_db_blocks.size() - aprox_count]->bl.timestamp);
  return w_hr.convert_to<uint64_t>();
}
//------------------------------------------------------------------
bool blockchain_storage::extport_scratchpad_to_file(const std::string& path)
{
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  export_scratchpad_file_header fh;
  memset(&fh, 0, sizeof(fh));
  const std::vector<crypto::hash>& scr_vector = m_scratchpad_wr.get_scratchpad();

  fh.current_hi.prevhash = currency::get_block_hash(m_db_blocks.back()->bl);
  fh.current_hi.height = m_db_blocks.size() - 1;
  fh.scratchpad_size = scr_vector.size() * 4;

  try
  {

    std::string tmp_path = path + ".tmp";
    std::ofstream fstream;
    fstream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fstream.open(tmp_path, std::ios_base::binary | std::ios_base::out | std::ios_base::trunc);
    fstream.write((const char*)&fh, sizeof(fh));
    fstream.write((const char*)&scr_vector[0], scr_vector.size() * 32);
    fstream.close();

    boost::filesystem::remove(path);
    boost::filesystem::rename(tmp_path, path);

    LOG_PRINT_L0("Scratchpad exported to " << path << ", " << (scr_vector.size() * 32) / 1024 << "kbytes");
    return true;
  }
  catch (const std::exception& e)
  {
    LOG_PRINT_L0("Failed to store scratchpad, error: " << e.what());
    return false;
  }

  catch (...)
  {
    LOG_PRINT_L0("Failed to store scratchpad, unknown error");
    return false;
  }

  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::get_alternative_blocks(std::list<block>& blocks)
{
  CRITICAL_REGION_LOCAL(m_blockchain_lock);

  BOOST_FOREACH(const auto& alt_bl, m_alternative_chains)
  {
    blocks.push_back(alt_bl.second.bl);
  }
  return true;
}
//------------------------------------------------------------------
size_t blockchain_storage::get_alternative_blocks_count()
{
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  return m_alternative_chains.size();
}
//------------------------------------------------------------------
bool blockchain_storage::add_out_to_get_random_outs(COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount& result_outs, uint64_t amount, size_t i, uint64_t mix_count, bool use_only_forced_to_mix)
{
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  auto out_ptr = m_db_outputs.get_subitem(amount, i);
  auto tx_ptr = m_db_transactions.find(out_ptr->first);
  CHECK_AND_ASSERT_MES(tx_ptr, false, "internal error: transaction with id " << out_ptr->first << ENDL <<
    ", used in mounts global index for amount=" << amount << ": i=" << i << "not found in transactions index");
  CHECK_AND_ASSERT_MES(tx_ptr->tx.vout.size() > out_ptr->second, false, "internal error: in global outs index, transaction out index="
    << out_ptr->second << " more than transaction outputs = " << tx_ptr->tx.vout.size() << ", for tx id = " << out_ptr->first);

  const transaction& tx = tx_ptr->tx;
  CHECK_AND_ASSERT_MES(tx.vout[out_ptr->second].target.type() == typeid(txout_to_key), false, "unknown tx out type");

  CHECK_AND_ASSERT_MES(tx_ptr->m_spent_flags.size() == tx.vout.size(), false, "internal error");

  //do not use outputs that obviously spent for mixins
  if (tx_ptr->m_spent_flags[out_ptr->second])
    return false;

  //check if transaction is unlocked
  if (!is_tx_spendtime_unlocked(tx.unlock_time))
    return false;

  //use appropriate mix_attr out 
  uint8_t mix_attr = boost::get<txout_to_key>(tx.vout[out_ptr->second].target).mix_attr;

  if (mix_attr == CURRENCY_TO_KEY_OUT_FORCED_NO_MIX)
    return false; //COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS call means that ring signature will have more than one entry.
  else if (use_only_forced_to_mix && mix_attr == CURRENCY_TO_KEY_OUT_RELAXED)
    return false; //relaxed not allowed
  else if (mix_attr != CURRENCY_TO_KEY_OUT_RELAXED && mix_attr > mix_count)
    return false;//mix_attr set to specific minimum, and mix_count is less then desired count


  COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::out_entry& oen = *result_outs.outs.insert(result_outs.outs.end(), COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::out_entry());
  oen.global_amount_index = i;
  oen.out_key = boost::get<txout_to_key>(tx.vout[out_ptr->second].target).key;
  return true;
}
//------------------------------------------------------------------
size_t blockchain_storage::find_end_of_allowed_index(uint64_t amount)
{
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  uint64_t sz = m_db_outputs.get_item_size(amount);

  if (!sz)
    return 0;
  uint64_t i = sz;
  do
  {
    --i;
    auto out_ptr = m_db_outputs.get_subitem(amount, i);
    auto tx_ptr = m_db_transactions.find(out_ptr->first);
    CHECK_AND_ASSERT_MES(tx_ptr, 0, "internal error: failed to find transaction from outputs index with tx_id=" << out_ptr->first);
    if (tx_ptr->m_keeper_block_height + CURRENCY_MINED_MONEY_UNLOCK_WINDOW <= get_current_blockchain_height())
      return i + 1;
  } while (i != 0);
  return 0;
}
//------------------------------------------------------------------
bool blockchain_storage::get_random_outs_for_amounts(const COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::request& req, COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::response& res)
{
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  BOOST_FOREACH(uint64_t amount, req.amounts)
  {
    COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount& result_outs = *res.outs.insert(res.outs.end(), COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount());
    result_outs.amount = amount;
    uint64_t outs_container_size = m_db_outputs.get_item_size(amount);
    if (!outs_container_size)
    {
      LOG_ERROR("COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS: not outs for amount " << amount << ", wallet should use some real outs when it lookup for some mix, so, at least one out for this amount should exist");
      continue;//actually this is strange situation, wallet should use some real outs when it lookup for some mix, so, at least one out for this amount should exist
    }
    //it is not good idea to use top fresh outs, because it increases possibility of transaction canceling on split
    //lets find upper bound of not fresh outs
    size_t up_index_limit = find_end_of_allowed_index(amount);
    CHECK_AND_ASSERT_MES(up_index_limit <= outs_container_size, false, "internal error: find_end_of_allowed_index returned wrong index=" << up_index_limit << ", with amount_outs.size = " << outs_container_size);
    if (up_index_limit >= req.outs_count)
    {
      std::set<size_t> used;
      size_t try_count = 0;
      for (uint64_t j = 0; j != req.outs_count && try_count < up_index_limit;)
      {
        size_t i = crypto::rand<size_t>() % up_index_limit;
        if (used.count(i))
          continue;
        bool added = add_out_to_get_random_outs(result_outs, amount, i, req.outs_count, req.use_forced_mix_outs);
        used.insert(i);
        if (added)
          ++j;
        ++try_count;
      }
      if (result_outs.outs.size() < req.outs_count)
      {
        LOG_PRINT_RED_L0("Not enough inputs for amount " << amount << ", needed " << req.outs_count << ", added " << result_outs.outs.size() << " good outs from " << up_index_limit << " unlocked of " << outs_container_size << " total");
      }
    }
    else
    {
      size_t added = 0;
      for (size_t i = 0; i != up_index_limit; i++)
        added += add_out_to_get_random_outs(result_outs, amount, i, req.outs_count, req.use_forced_mix_outs) ? 1 : 0;
      LOG_PRINT_RED_L0("Not enough inputs for amount " << amount << ", needed " << req.outs_count << ", added " << added << " good outs from " << up_index_limit << " unlocked of " << outs_container_size << " total - respond with all good outs");
    }
  }
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::update_spent_tx_flags_for_input(uint64_t amount, uint64_t global_index, bool spent)
{
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  uint64_t outs_count = m_db_outputs.get_item_size(amount);
  CHECK_AND_ASSERT_MES(outs_count, false, "Amount " << amount << " have not found during update_spent_tx_flags_for_input()");
  CHECK_AND_ASSERT_MES(global_index < outs_count, false, "Global index" << global_index << " for amount " << amount << " bigger value than amount's vector size()=" << outs_count);
  auto out_ptr = m_db_outputs.get_subitem(amount, global_index);
  return update_spent_tx_flags_for_input(out_ptr->first, out_ptr->second, spent);
}
//------------------------------------------------------------------
bool blockchain_storage::update_spent_tx_flags_for_input(const crypto::hash& tx_id, size_t n, bool spent)
{
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  auto tx_ptr = m_db_transactions.find(tx_id);
  CHECK_AND_ASSERT_MES(tx_ptr, false, "Can't find transaction id: " << tx_id);
  transaction_chain_entry tce_local = *tx_ptr;

  CHECK_AND_ASSERT_MES(n < tce_local.m_spent_flags.size(), false, "Wrong input offset: " << n << " in transaction id: " << tx_id);

  tce_local.m_spent_flags[n] = spent;
  m_db_transactions.set(tx_id, tce_local);

  return true;
}
// ------------------------------------------------------------------
// bool blockchain_storage::resync_spent_tx_flags()
// {
//   LOG_PRINT_L0("Started re-building spent tx outputs data...");
//   CRITICAL_REGION_LOCAL(m_blockchain_lock);
//   for(auto& tx: m_db_transactions)
//   {
//     if(is_coinbase(tx.second.tx))
//       continue;
// 
//     for(auto& in: tx.second.tx.vin)
//     {      
//       CHECKED_GET_SPECIFIC_VARIANT(in, txin_to_key, in_to_key, false);
//       if(in_to_key.key_offsets.size() != 1)
//         continue;
// 
//       //direct spending
//       if(!update_spent_tx_flags_for_input(in_to_key.amount, in_to_key.key_offsets[0], true))
//         return false;
// 
//     }
//   }
//   LOG_PRINT_L0("Finished re-building spent tx outputs data");
//   return true;
// }
//------------------------------------------------------------------
bool blockchain_storage::find_blockchain_supplement(const std::list<crypto::hash>& qblock_ids, uint64_t& starter_offset)
{
  CRITICAL_REGION_LOCAL(m_blockchain_lock);

  if (!qblock_ids.size() /*|| !req.m_total_height*/)
  {
    LOG_ERROR("Client sent wrong NOTIFY_REQUEST_CHAIN: m_block_ids.size()=" << qblock_ids.size() << /*", m_height=" << req.m_total_height <<*/ ", dropping connection");
    return false;
  }
  //check genesis match
  if (qblock_ids.back() != get_block_hash(m_db_blocks[0]->bl))
  {
    LOG_ERROR("Client sent wrong NOTIFY_REQUEST_CHAIN: genesis block missmatch: " << ENDL << "id: "
      << qblock_ids.back() << ", " << ENDL << "expected: " << get_block_hash(m_db_blocks[0]->bl)
      << "," << ENDL << " dropping connection");
    return false;
  }

  /* Figure out what blocks we should request to get state_normal */
  size_t i = 0;
  auto bl_it = qblock_ids.begin();
  auto block_index_ptr = m_db_blocks_index.find(*bl_it);
  for (; bl_it != qblock_ids.end(); bl_it++, i++)
  {
    block_index_ptr = m_db_blocks_index.find(*bl_it);
    if (block_index_ptr)
      break;
  }

  if (bl_it == qblock_ids.end())
  {
    LOG_ERROR("Internal error handling connection, can't find split point");
    return false;
  }

  if (!block_index_ptr)
  {
    //this should NEVER happen, but, dose of paranoia in such cases is not too bad
    LOG_ERROR("Internal error handling connection, can't find split point");
    return false;
  }

  //we start to put block ids INCLUDING last known id, just to make other side be sure
  starter_offset = *block_index_ptr;
  return true;
}
//------------------------------------------------------------------
wide_difficulty_type blockchain_storage::block_difficulty(size_t i)
{
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  CHECK_AND_ASSERT_MES(i < m_db_blocks.size(), false, "wrong block index i = " << i << " at blockchain_storage::block_difficulty()");
  if (i == 0)
    return m_db_blocks[i]->cumulative_difficulty;

  return m_db_blocks[i]->cumulative_difficulty - m_db_blocks[i - 1]->cumulative_difficulty;
}
//------------------------------------------------------------------
void blockchain_storage::print_blockchain(uint64_t start_index, uint64_t end_index)
{
  std::stringstream ss;
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  if (start_index >= m_db_blocks.size())
  {
    LOG_PRINT_L0("Wrong starter index set: " << start_index << ", expected max index " << m_db_blocks.size() - 1);
    return;
  }

  for (size_t i = start_index; i != m_db_blocks.size() && i != end_index; i++)
  {
    ss << "height " << i << ", timestamp " << m_db_blocks[i]->bl.timestamp << ", cumul_dif " << m_db_blocks[i]->cumulative_difficulty << ", cumul_size " << m_db_blocks[i]->block_cumulative_size
      << "\nid\t\t" << get_block_hash(m_db_blocks[i]->bl)
      << "\ndifficulty\t\t" << block_difficulty(i) << ", nonce " << m_db_blocks[i]->bl.nonce << ", tx_count " << m_db_blocks[i]->bl.tx_hashes.size() << ENDL;
  }
  LOG_PRINT_L1("Current blockchain:" << ENDL << ss.str());
  LOG_PRINT_L0("Blockchain printed with log level 1");
}
//------------------------------------------------------------------
void blockchain_storage::print_blockchain_index()
{
  LOG_ERROR("NOT IMPLEMENTED YET");
  //   std::stringstream ss;
  //   CRITICAL_REGION_LOCAL(m_blockchain_lock);
  //   BOOST_FOREACH(const blocks_by_id_index::value_type& v, m_db_blocks_index)
  //     ss << "id\t\t" <<  v.first << " height" <<  v.second << ENDL << "";
  // 
  //   LOG_PRINT_L0("Current blockchain index:" << ENDL << ss.str());
}
//------------------------------------------------------------------
void blockchain_storage::print_blockchain_outs(const std::string& file)
{
  LOG_ERROR("NOT IMPLEMENTED YET");
  //   std::stringstream ss;
  //   CRITICAL_REGION_LOCAL(m_blockchain_lock);
  //   BOOST_FOREACH(const outputs_container::value_type& v, m_db_outputs)
  //   {
  //     const std::vector<std::pair<crypto::hash, size_t> >& vals = v.second;
  //     if(vals.size())
  //     {
  //       ss << "amount: " <<  v.first << ENDL;
  //       for(size_t i = 0; i != vals.size(); i++)
  //         ss << "\t" << vals[i].first << ": " << vals[i].second << ENDL;
  //     }
  //   }
  //   if(file_io_utils::save_string_to_file(file, ss.str()))
  //   {
  //     LOG_PRINT_L0("Current outputs index writen to file: " << file);
  //   }else
  //   {
  //     LOG_PRINT_L0("Failed to write current outputs index to file: " << file);
  //   }
}
//------------------------------------------------------------------
bool blockchain_storage::find_blockchain_supplement(const std::list<crypto::hash>& qblock_ids, NOTIFY_RESPONSE_CHAIN_ENTRY::request& resp)
{
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  if (!find_blockchain_supplement(qblock_ids, resp.start_height))
    return false;

  resp.total_height = get_current_blockchain_height();
  size_t count = 0;
  for (size_t i = resp.start_height; i != m_db_blocks.size() && count < BLOCKS_IDS_SYNCHRONIZING_DEFAULT_COUNT; i++, count++)
    resp.m_block_ids.push_back(get_block_hash(m_db_blocks[i]->bl));
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::find_blockchain_supplement(const std::list<crypto::hash>& qblock_ids, std::list<std::pair<block, std::list<transaction> > >& blocks, uint64_t& total_height, uint64_t& start_height, size_t max_count)
{
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  PROF_L2_START(find_blockchain_supplement_time);
  if (!find_blockchain_supplement(qblock_ids, start_height))
    return false;
  PROF_L2_FINISH(find_blockchain_supplement_time);

  PROF_L2_START(get_transactions_time);
  total_height = get_current_blockchain_height();
  size_t count = 0;
  size_t txs_count = 0;
  for (size_t i = start_height; i != m_db_blocks.size() && count < max_count; i++, count++)
  {
    blocks.resize(blocks.size() + 1);
    blocks.back().first = m_db_blocks[i]->bl;
    std::list<crypto::hash> mis;
    get_transactions(m_db_blocks[i]->bl.tx_hashes, blocks.back().second, mis);
    CHECK_AND_ASSERT_MES(!mis.size(), false, "internal error, transaction from block not found");
    txs_count += blocks.back().second.size();
  }
  PROF_L2_FINISH(get_transactions_time);
  PROF_L2_LOG_PRINT("find_blockchain_supplement(5): " << blocks.size() << " blocks, " << txs_count << " txs, timings: " << print_mcsec_as_ms(find_blockchain_supplement_time) << " / " << print_mcsec_as_ms(get_transactions_time), LOG_LEVEL_1);
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::add_block_as_invalid(const block& bl, const crypto::hash& h)
{
  block_extended_info bei = AUTO_VAL_INIT(bei);
  bei.bl = bl;
  return add_block_as_invalid(bei, h);
}
//------------------------------------------------------------------
bool blockchain_storage::add_block_as_invalid(const block_extended_info& bei, const crypto::hash& h)
{
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  auto i_res = m_invalid_blocks.insert(std::map<crypto::hash, block_extended_info>::value_type(h, bei));
  CHECK_AND_ASSERT_MES(i_res.second, false, "at insertion invalid by tx returned status existed");
  LOG_PRINT_L0("BLOCK ADDED AS INVALID: " << h << ENDL << ", prev_id=" << bei.bl.prev_id << ", m_invalid_blocks count=" << m_invalid_blocks.size());
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::have_block(const crypto::hash& id)
{
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  if (m_db_blocks_index.find(id))
    return true;
  if (m_alternative_chains.count(id))
    return true;
  /*if(m_orphaned_blocks.get<by_id>().count(id))
  return true;*/

  /*if(m_orphaned_by_tx.count(id))
  return true;*/
  if (m_invalid_blocks.count(id))
    return true;

  return false;
}
//------------------------------------------------------------------
bool blockchain_storage::handle_block_to_main_chain(const block& bl, block_verification_context& bvc)
{
  crypto::hash id = get_block_hash(bl);
  return handle_block_to_main_chain(bl, id, bvc);
}
//------------------------------------------------------------------
bool blockchain_storage::push_transaction_to_global_outs_index(const transaction& tx, const crypto::hash& tx_id, std::vector<uint64_t>& global_indexes)
{
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  size_t i = 0;
  BOOST_FOREACH(const auto& ot, tx.vout)
  {
    if (ot.target.type() == typeid(txout_to_key))
    {
      m_db_outputs.push_back_item(ot.amount, std::pair<crypto::hash, size_t>(tx_id, i));
      global_indexes.push_back(m_db_outputs.get_item_size(ot.amount) - 1);
    }
    ++i;
  }
  return true;
}
//------------------------------------------------------------------
size_t blockchain_storage::get_total_transactions()
{
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  return m_db_transactions.size();
}
//------------------------------------------------------------------
bool blockchain_storage::get_outs(uint64_t amount, std::list<crypto::public_key>& pkeys)
{
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  uint64_t sz = m_db_outputs.get_item_size(amount);

  if (!sz)
    return true;

  for (uint64_t i = 0; i != sz; i++)
  {
    auto out_entry_ptr = m_db_outputs.get_subitem(amount, i);

    auto tx_ptr = m_db_transactions.find(out_entry_ptr->first);
    CHECK_AND_ASSERT_MES(tx_ptr, false, "transactions outs global index consistency broken: wrong tx id in index");
    CHECK_AND_ASSERT_MES(tx_ptr->tx.vout.size() > out_entry_ptr->second, false, "transactions outs global index consistency broken: index in tx_outx more then size");
    CHECK_AND_ASSERT_MES(tx_ptr->tx.vout[out_entry_ptr->second].target.type() == typeid(txout_to_key), false, "transactions outs global index consistency broken: index in tx_outx more then size");
    pkeys.push_back(boost::get<txout_to_key>(tx_ptr->tx.vout[out_entry_ptr->second].target).key);
  }

  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::pop_transaction_from_global_index(const transaction& tx, const crypto::hash& tx_id)
{
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  size_t i = tx.vout.size() - 1;
  BOOST_REVERSE_FOREACH(const auto& ot, tx.vout)
  {
    if (ot.target.type() == typeid(txout_to_key))
    {
      uint64_t sz = m_db_outputs.get_item_size(ot.amount);
      CHECK_AND_ASSERT_MES(sz, false, "transactions outs global index: empty index for amount: " << ot.amount);
      auto back_item = m_db_outputs.get_subitem(ot.amount, sz - 1);
      CHECK_AND_ASSERT_MES(back_item->first == tx_id, false, "transactions outs global index consistency broken: tx id missmatch");
      CHECK_AND_ASSERT_MES(back_item->second == i, false, "transactions outs global index consistency broken: in transaction index missmatch");
      m_db_outputs.pop_back_item(ot.amount);
      //do not let to exist empty m_outputs entries - this will broke scratchpad selector
      //if (!it->second.size())
      //  m_db_outputs.erase(it);
    }
    --i;
  }
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::unprocess_blockchain_tx_extra(const transaction& tx)
{
  if (is_coinbase(tx))
  {
    tx_extra_info ei = AUTO_VAL_INIT(ei);
    bool r = parse_and_validate_tx_extra(tx, ei);
    CHECK_AND_ASSERT_MES(r, false, "failed to validate transaction extra on unprocess_blockchain_tx_extra");
    if (ei.m_alias.m_alias.size())
    {
      r = pop_alias_info(ei.m_alias);
      CHECK_AND_ASSERT_MES(r, false, "failed to pop_alias_info");
    }
  }
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::get_alias_info(const std::string& alias, alias_info_base& info)
{
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  auto al_ptr = m_db_aliases.find(alias);
  if (al_ptr)
  {
    if (al_ptr->size())
    {
      info = al_ptr->back();
      return true;
    }
  }
  return false;
}
//------------------------------------------------------------------
uint64_t blockchain_storage::get_aliases_count()
{
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  return m_db_aliases.size();
}
//------------------------------------------------------------------
uint64_t blockchain_storage::get_scratchpad_size()
{
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  return m_scratchpad_wr.get_scratchpad().size() * 32;
}
//------------------------------------------------------------------
bool blockchain_storage::get_all_aliases(std::list<alias_info>& aliases)
{
  CRITICAL_REGION_LOCAL(m_blockchain_lock);

  m_db_aliases.enumerate_items([&](uint64_t i, const std::string& alias, const std::list<alias_info_base>& elias_entries)
  {
    if (elias_entries.size())
    {
      aliases.push_back(alias_info());
      aliases.back().m_alias = alias;
      static_cast<alias_info_base&>(aliases.back()) = elias_entries.back();
    }
    return true;
  });

  return true;
}
//------------------------------------------------------------------
std::string blockchain_storage::get_alias_by_address(const account_public_address& addr)
{
  auto alias_ptr = m_db_addr_to_alias.find(addr);
  if (alias_ptr && alias_ptr->size())
  {
    return *(alias_ptr->begin());
  }

  return "";
}
//------------------------------------------------------------------
bool blockchain_storage::pop_alias_info(const alias_info& ai)
{
  CRITICAL_REGION_LOCAL(m_blockchain_lock);

  CHECK_AND_ASSERT_MES(ai.m_alias.size(), false, "empty name in pop_alias_info");
  auto alias_history_ptr = m_db_aliases.find(ai.m_alias);
  CHECK_AND_ASSERT_MES(alias_history_ptr && alias_history_ptr->size(), false, "empty name list in pop_alias_info");

  auto addr_to_alias_ptr = m_db_addr_to_alias.find(alias_history_ptr->back().m_address);
  if (addr_to_alias_ptr)
  {
    //update db
    address_to_aliases_container::t_value_type local_v = *addr_to_alias_ptr;

    auto it_in_set = local_v.find(ai.m_alias);
    CHECK_AND_ASSERT_MES(it_in_set != local_v.end(), false, "it_in_set != it->second.end() validation failed");

    local_v.erase(it_in_set);
    if (!local_v.size())
    {
      //delete the whole record from db
      m_db_addr_to_alias.erase(alias_history_ptr->back().m_address);
    }
    else
    {
      //update db
      m_db_addr_to_alias.set(alias_history_ptr->back().m_address, local_v);
    }
  }
  else
  {
    LOG_ERROR("In m_addr_to_alias not found " << get_account_address_as_str(alias_history_ptr->back().m_address));
  }

  aliases_container::t_value_type local_alias_hist = *alias_history_ptr;
  local_alias_hist.pop_back();
  m_db_aliases.set(ai.m_alias, local_alias_hist);


  if (local_alias_hist.size())
  {
    address_to_aliases_container::t_value_type local_copy = AUTO_VAL_INIT(local_copy);
    auto set_ptr = m_db_addr_to_alias.get(local_alias_hist.back().m_address);
    if (set_ptr)
      local_copy = *set_ptr;

    local_copy.insert(ai.m_alias);
    m_db_addr_to_alias.set(local_alias_hist.back().m_address, local_copy);
  }

  LOG_PRINT_MAGENTA("[ALIAS_UNREGISTERED]: " << ai.m_alias << ": " << get_account_address_as_str(ai.m_address) << " -> " << (!alias_history_ptr->empty() ? get_account_address_as_str(alias_history_ptr->back().m_address) : "(available)"), LOG_LEVEL_0);
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::put_alias_info(const alias_info& ai)
{
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  CHECK_AND_ASSERT_MES(ai.m_alias.size(), false, "empty name in put_alias_info");

  aliases_container::t_value_type local_alias_history = AUTO_VAL_INIT(local_alias_history);
  auto alias_history_ptr_ = m_db_aliases.get(ai.m_alias);
  if (alias_history_ptr_)
    local_alias_history = *alias_history_ptr_;

  if (ai.m_sign == null_sig)
  {//adding new alias, check sat name is free
    CHECK_AND_ASSERT_MES(!local_alias_history.size(), false, "alias " << ai.m_alias << " already in use");
    //update alias entry in db
    local_alias_history.push_back(ai);
    m_db_aliases.set(ai.m_alias, local_alias_history);

    //update addr-to-alias db entry
    address_to_aliases_container::t_value_type addr_to_alias_local = AUTO_VAL_INIT(addr_to_alias_local);
    auto addr_to_alias_ptr_ = m_db_addr_to_alias.get(local_alias_history.back().m_address);
    if (addr_to_alias_ptr_)
      addr_to_alias_local = *addr_to_alias_ptr_;

    addr_to_alias_local.insert(ai.m_alias);
    m_db_addr_to_alias.set(local_alias_history.back().m_address, addr_to_alias_local);

    LOG_PRINT_MAGENTA("[ALIAS_REGISTERED]: " << ai.m_alias << ": " << get_account_address_as_str(ai.m_address), LOG_LEVEL_0);
  }
  else
  {
    //update procedure
    CHECK_AND_ASSERT_MES(local_alias_history.size(), false, "alias " << ai.m_alias << " can't be update becouse it doesn't exists");
    std::string old_address = currency::get_account_address_as_str(local_alias_history.back().m_address);
    std::string signed_buff;
    make_tx_extra_alias_entry(signed_buff, ai, true);
    bool r = crypto::check_signature(get_blob_hash(signed_buff), local_alias_history.back().m_address.m_spend_public_key, ai.m_sign);
    CHECK_AND_ASSERT_MES(r, false, "Failed to check signature, alias update failed");
    //update granted

    //update adr-to-alias db
    auto addr_to_alias_ptr_ = m_db_addr_to_alias.find(local_alias_history.back().m_address);
    if (addr_to_alias_ptr_)
    {
      address_to_aliases_container::t_value_type addr_to_alias_local = *addr_to_alias_ptr_;
      auto it_in_set = addr_to_alias_local.find(ai.m_alias);
      if (it_in_set == addr_to_alias_local.end())
      {
        LOG_ERROR("it_in_set == it->second.end()");
      }
      else
      {
        addr_to_alias_local.erase(it_in_set);
      }

      if (!addr_to_alias_local.size())
        m_db_addr_to_alias.erase(local_alias_history.back().m_address);
      else
        m_db_addr_to_alias.set(local_alias_history.back().m_address, addr_to_alias_local);
    }
    else
    {
      LOG_ERROR("Wrong m_addr_to_alias state: address not found " << get_account_address_as_str(local_alias_history.back().m_address));
    }
    //update alias db
    local_alias_history.push_back(ai);
    m_db_aliases.set(ai.m_alias, local_alias_history);

    //update addr_to_alias db
    address_to_aliases_container::t_value_type addr_to_alias_local2 = AUTO_VAL_INIT(addr_to_alias_local2);
    auto addr_to_alias_ptr_2 = m_db_addr_to_alias.get(local_alias_history.back().m_address);
    if (addr_to_alias_ptr_2)
      addr_to_alias_local2 = *addr_to_alias_ptr_2;

    addr_to_alias_local2.insert(ai.m_alias);
    m_db_addr_to_alias.set(local_alias_history.back().m_address, addr_to_alias_local2);

    LOG_PRINT_MAGENTA("[ALIAS_UPDATED]: " << ai.m_alias << ": from: " << old_address << " to " << get_account_address_as_str(ai.m_address), LOG_LEVEL_0);
  }
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::process_blockchain_tx_extra(const transaction& tx)
{
  //check transaction extra
  tx_extra_info ei = AUTO_VAL_INIT(ei);
  bool r = parse_and_validate_tx_extra(tx, ei);
  CHECK_AND_ASSERT_MES(r, false, "failed to validate transaction extra");
  if (is_coinbase(tx) && ei.m_alias.m_alias.size())
  {
    r = put_alias_info(ei.m_alias);
    CHECK_AND_ASSERT_MES(r, false, "failed to put_alias_info");
  }
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::add_transaction_from_block(const transaction& tx, const crypto::hash& tx_id, const crypto::hash& bl_id, uint64_t bl_height)
{
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  PROF_L2_START(process_tx_extra_time);
  bool r = process_blockchain_tx_extra(tx);
  CHECK_AND_ASSERT_MES(r, false, "failed to process_blockchain_tx_extra");
  PROF_L2_FINISH(process_tx_extra_time);
  struct add_transaction_input_visitor : public boost::static_visitor<bool>
  {
    blockchain_storage& m_bcs;
    key_images_container& m_db_spent_keys;
    const crypto::hash& m_tx_id;
    const crypto::hash& m_bl_id;
    add_transaction_input_visitor(blockchain_storage& bcs, key_images_container& spent_keys, const crypto::hash& tx_id, const crypto::hash& bl_id) :
      m_bcs(bcs),
      m_db_spent_keys(spent_keys),
      m_tx_id(tx_id),
      m_bl_id(bl_id)
    {}
    bool operator()(const txin_to_key& in) const
    {
      const crypto::key_image& ki = in.k_image;

      auto ki_ptr = m_db_spent_keys.get(ki);
      if (ki_ptr)
      {
        //double spend detected
        LOG_PRINT_RED_L0("tx with id: " << m_tx_id << " in block id: " << m_bl_id << " have input marked as spent with key image: " << ki << ", block declined");
        return false;
      }
      m_db_spent_keys.set(ki, true);

      if (in.key_offsets.size() == 1)
      {
        //direct spend detected
        if (!m_bcs.update_spent_tx_flags_for_input(in.amount, in.key_offsets[0], true))
        {
          //internal error
          LOG_PRINT_RED_L0("Failed to  update_spent_tx_flags_for_input");
          return false;
        }
      }

      return true;
    }

    bool operator()(const txin_gen& tx) const{ return true; }
    bool operator()(const txin_to_script& tx) const{ return false; }
    bool operator()(const txin_to_scripthash& tx) const{ return false; }
  };

  PROF_L2_START(process_tx_inputs_time);
  BOOST_FOREACH(const txin_v& in, tx.vin)
  {
    if (!boost::apply_visitor(add_transaction_input_visitor(*this, m_db_spent_keys, tx_id, bl_id), in))
    {
      LOG_ERROR("critical internal error: add_transaction_input_visitor failed. but key_images should be already checked");
      purge_transaction_keyimages_from_blockchain(tx, false);
      bool r = unprocess_blockchain_tx_extra(tx);
      CHECK_AND_ASSERT_MES(r, false, "failed to unprocess_blockchain_tx_extra");
      return false;
    }
  }
  PROF_L2_FINISH(process_tx_inputs_time);
  PROF_L2_START(push_tx_to_global_index_time_1);
  transaction_chain_entry ch_e;
  ch_e.m_keeper_block_height = bl_height;
  ch_e.m_spent_flags.resize(tx.vout.size(), false);
  ch_e.tx = tx;

  //check if there is already transaction with this hash
  auto tx_entry_ptr = m_db_transactions.get(tx_id);
  PROF_L2_FINISH(push_tx_to_global_index_time_1);
  PROF_L2_START(push_tx_to_global_index_time_2);
  if (tx_entry_ptr)
  {
    LOG_ERROR("critical internal error: tx with id: " << tx_id << " in block id: " << bl_id << " already in blockchain");
    purge_transaction_keyimages_from_blockchain(tx, true);
    bool r = unprocess_blockchain_tx_extra(tx);
    CHECK_AND_ASSERT_MES(r, false, "failed to unprocess_blockchain_tx_extra");
    return false;
  }

  r = push_transaction_to_global_outs_index(tx, tx_id, ch_e.m_global_output_indexes);
  CHECK_AND_ASSERT_MES(r, false, "failed to return push_transaction_to_global_outs_index tx id " << tx_id);
  PROF_L2_FINISH(push_tx_to_global_index_time_2);

  //store everything to db
  PROF_L2_START(store_to_db_time);
  m_db_transactions.set(tx_id, ch_e);
  PROF_L2_FINISH(store_to_db_time);
  LOG_PRINT_L2("Added transaction to blockchain history:" << ENDL
    << "tx_id: " << tx_id << ENDL
    << "inputs: " << tx.vin.size() << ", outs: " << tx.vout.size() << ", spend money: " << print_money(get_outs_money_amount(tx)) << "(fee: " << (is_coinbase(tx) ? "0[coinbase]" : print_money(get_tx_fee(tx))) << ")"
    << PROF_L2_STR("  Profiling results (ms):")
    << PROF_L2_STR_MS(ENDL << "  process_tx_extra_time:          ", process_tx_extra_time)
    << PROF_L2_STR_MS(ENDL << "  process_tx_inputs_time:         ", process_tx_inputs_time)
    << PROF_L2_STR_MS(ENDL << "  push_tx_to_global_index_time_1: ", push_tx_to_global_index_time_1)
    << PROF_L2_STR_MS(ENDL << "  push_tx_to_global_index_time_2: ", push_tx_to_global_index_time_2)
    << PROF_L2_STR_MS(ENDL << "  store_to_db_time:               ", store_to_db_time)
    << PROF_L2_STR(ENDL)
  );

  return true;
}
//------------------------------------------------------------------
//------------------------------------------------------------------
bool blockchain_storage::get_tx_outputs_gindexs(const crypto::hash& tx_id, std::vector<uint64_t>& indexs)
{
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  auto tx_ptr = m_db_transactions.find(tx_id);
  if (!tx_ptr)
  {
    LOG_PRINT_RED_L0("warning: get_tx_outputs_gindexs failed to find transaction with id = " << tx_id);
    return false;
  }

  CHECK_AND_ASSERT_MES(tx_ptr->m_global_output_indexes.size(), false, "internal error: global indexes for transaction " << tx_id << " is empty");
  indexs = tx_ptr->m_global_output_indexes;
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::check_tx_inputs(const transaction& tx, uint64_t& max_used_block_height, crypto::hash& max_used_block_id)
{
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  bool res = check_tx_inputs(tx, &max_used_block_height);
  if (!res) return false;
  CHECK_AND_ASSERT_MES(max_used_block_height < m_db_blocks.size(), false, "internal error: max used block index=" << max_used_block_height << " is not less then blockchain size = " << m_db_blocks.size());
  get_block_hash(m_db_blocks[max_used_block_height]->bl, max_used_block_id);
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::have_tx_keyimges_as_spent(const transaction &tx)
{
  BOOST_FOREACH(const txin_v& in, tx.vin)
  {
    CHECKED_GET_SPECIFIC_VARIANT(in, const txin_to_key, in_to_key, true);
    if (have_tx_keyimg_as_spent(in_to_key.k_image))
      return true;
  }
  return false;
}
//------------------------------------------------------------------
bool blockchain_storage::check_tx_inputs(const transaction& tx, uint64_t* pmax_used_block_height)
{
  crypto::hash tx_prefix_hash = get_transaction_prefix_hash(tx);
  return check_tx_inputs(tx, tx_prefix_hash, pmax_used_block_height);
}
//------------------------------------------------------------------
bool blockchain_storage::check_tx_inputs(const transaction& tx, const crypto::hash& tx_prefix_hash, uint64_t* pmax_used_block_height)
{
  size_t sig_index = 0;
  if (pmax_used_block_height)
    *pmax_used_block_height = 0;

  BOOST_FOREACH(const auto& txin, tx.vin)
  {
    CHECK_AND_ASSERT_MES(txin.type() == typeid(txin_to_key), false, "wrong type id in tx input at blockchain_storage::check_tx_inputs");
    const txin_to_key& in_to_key = boost::get<txin_to_key>(txin);

    CHECK_AND_ASSERT_MES(in_to_key.key_offsets.size(), false, "empty in_to_key.key_offsets in transaction with id " << get_transaction_hash(tx));

    if (have_tx_keyimg_as_spent(in_to_key.k_image))
    {
      LOG_PRINT_L1("Key image already spent in blockchain: " << string_tools::pod_to_hex(in_to_key.k_image));
      return false;
    }

    std::vector<crypto::signature> sig_stub;
    const std::vector<crypto::signature>* psig = &sig_stub;
    if (!m_is_in_checkpoint_zone)
    {
      CHECK_AND_ASSERT_MES(sig_index < tx.signatures.size(), false, "wrong transaction: not signature entry for input with index= " << sig_index);
      psig = &tx.signatures[sig_index];
    }
    if (!check_tx_input(in_to_key, tx_prefix_hash, *psig, pmax_used_block_height))
    {
      LOG_PRINT_L0("Failed to check input #" << sig_index << " for tx " << get_transaction_hash(tx));
      return false;
    }

    sig_index++;
  }
  if (!m_is_in_checkpoint_zone)
  {
    CHECK_AND_ASSERT_MES(tx.signatures.size() == sig_index, false, "tx signatures count differs from inputs");
  }

  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::is_tx_spendtime_unlocked(uint64_t unlock_time)
{
  if (unlock_time < CURRENCY_MAX_BLOCK_NUMBER)
  {
    //interpret as block index
    if (get_current_blockchain_height() - 1 + CURRENCY_LOCKED_TX_ALLOWED_DELTA_BLOCKS >= unlock_time)
      return true;
    else
      return false;
  }
  else
  {
    //interpret as time
    uint64_t current_time = static_cast<uint64_t>(time(NULL));
    if (current_time + CURRENCY_LOCKED_TX_ALLOWED_DELTA_SECONDS >= unlock_time)
      return true;
    else
      return false;
  }
  return false;
}
//------------------------------------------------------------------
bool blockchain_storage::check_tx_input(const txin_to_key& txin, const crypto::hash& tx_prefix_hash, const std::vector<crypto::signature>& sig, uint64_t* pmax_related_block_height)
{
  CRITICAL_REGION_LOCAL(m_blockchain_lock);

  struct outputs_visitor
  {
    std::vector<crypto::public_key>& m_results_collector;
    blockchain_storage& m_bch;
    outputs_visitor(std::vector<crypto::public_key>& results_collector, blockchain_storage& bch) :m_results_collector(results_collector), m_bch(bch)
    {}
    bool handle_output(const transaction& tx, const tx_out& out)
    {
      //check tx unlock time
      if (!m_bch.is_tx_spendtime_unlocked(tx.unlock_time))
      {
        LOG_PRINT_L0("One of outputs for one of inputs have wrong tx.unlock_time = " << tx.unlock_time);
        return false;
      }

      if (out.target.type() != typeid(txout_to_key))
      {
        LOG_PRINT_L0("Output have wrong type id, which=" << out.target.which());
        return false;
      }

      m_results_collector.push_back(boost::get<txout_to_key>(out.target).key);
      return true;
    }
  };

  //check ring signature
  std::vector<crypto::public_key> output_keys;
  outputs_visitor vi(output_keys, *this);
  if (!scan_outputkeys_for_indexes(txin, vi, pmax_related_block_height))
  {
    LOG_PRINT_L0("Failed to get output keys for tx with amount = " << print_money(txin.amount) << " and count indexes " << txin.key_offsets.size());
    return false;
  }

  if (txin.key_offsets.size() != output_keys.size())
  {
    LOG_PRINT_L0("Output keys for tx with amount = " << txin.amount << " and count indexes " << txin.key_offsets.size() << " returned wrong keys count " << output_keys.size());
    return false;
  }
  if (m_is_in_checkpoint_zone)
    return true;

  CHECK_AND_ASSERT_MES(sig.size() == output_keys.size(), false, "internal error: tx signatures count=" << sig.size() << " mismatch with outputs keys count for inputs=" << output_keys.size());
  return crypto::check_ring_signature(tx_prefix_hash, txin.k_image, output_keys, sig.data());
}
//------------------------------------------------------------------
uint64_t blockchain_storage::get_adjusted_time()
{
  //TODO: add collecting median time
  return time(NULL);
}
//------------------------------------------------------------------
bool blockchain_storage::check_block_timestamp_main(const block& b)
{
  if (b.timestamp > get_adjusted_time() + CURRENCY_BLOCK_FUTURE_TIME_LIMIT)
  {
    LOG_PRINT_L0("Timestamp of block with id: " << get_block_hash(b) << ", " << b.timestamp << ", bigger than adjusted time + 2 hours");
    return false;
  }

  std::vector<uint64_t> timestamps;
  size_t offset = m_db_blocks.size() <= BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW ? 0 : m_db_blocks.size() - BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW;
  for (; offset != m_db_blocks.size(); ++offset)
    timestamps.push_back(m_db_blocks[offset]->bl.timestamp);

  return check_block_timestamp(std::move(timestamps), b);
}
//------------------------------------------------------------------
bool blockchain_storage::check_block_timestamp(std::vector<uint64_t> timestamps, const block& b)
{
  if (timestamps.size() < BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW)
    return true;

  uint64_t median_ts = epee::misc_utils::median(timestamps);

  if (b.timestamp < median_ts)
  {
    LOG_PRINT_L0("Timestamp of block with id: " << get_block_hash(b) << ", " << b.timestamp << ", less than median of last " << BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW << " blocks, " << median_ts);
    return false;
  }

  return true;
}

//------------------------------------------------------------------
bool blockchain_storage::get_block_for_scratchpad_alt(uint64_t connection_height, uint64_t block_index, std::list<blockchain_storage::blocks_ext_by_hash::iterator>& alt_chain, block & b)
{
  CRITICAL_REGION_LOCAL(m_blockchain_lock);
  if (block_index >= connection_height)
  {
    //take it from alt chain
    for (auto it : alt_chain)
    {
      if (it->second.height == block_index)
      {
        b = it->second.bl;
        return true;
      }
    }
    CHECK_AND_ASSERT_MES(false, false, "Failed to find alternative block with height=" << block_index);
  }
  else
  {
    b = m_db_blocks[block_index]->bl;
  }
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::prune_aged_alt_blocks()
{
  CRITICAL_REGION_LOCAL(m_blockchain_lock);

  uint64_t current_height = get_current_blockchain_height();

  for (auto it = m_alternative_chains.begin(); it != m_alternative_chains.end();)
  {
    if (current_height > it->second.height && current_height - it->second.height > CURRENCY_ALT_BLOCK_LIVETIME_COUNT)
      m_alternative_chains.erase(it++);
    else
      ++it;
  }

  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::handle_block_to_main_chain(const block& bl, const crypto::hash& id, block_verification_context& bvc)
{
  PROF_L1_START(block_processing_time);
  CRITICAL_REGION_LOCAL(m_blockchain_lock);

  if (bl.prev_id != get_top_block_id())
  {
    LOG_PRINT_L0("Block with id: " << id << ENDL
      << "have wrong prev_id: " << bl.prev_id << ENDL
      << "expected: " << get_top_block_id());
    return false;
  }

  PROF_L2_START(timestamp_check_time);
  if (!check_block_timestamp_main(bl))
  {
    LOG_PRINT_L0("Block with id: " << id << ENDL
      << "have invalid timestamp: " << bl.timestamp);
    //add_block_as_invalid(bl, id);//do not add blocks to invalid storage befor proof of work check was passed
    bvc.m_verifivation_failed = true;
    return false;
  }
  PROF_L2_FINISH(timestamp_check_time);

  //check proof of work
  PROF_L1_START(target_calculating_time);
  wide_difficulty_type current_diffic = get_difficulty_for_next_block();
  CHECK_AND_ASSERT_MES(current_diffic, false, "!!!!!!!!! difficulty overhead !!!!!!!!!");
  PROF_L1_FINISH(target_calculating_time);
  PROF_L1_START(longhash_calculating_time);
  crypto::hash proof_of_work = null_hash;

  proof_of_work = get_block_longhash(bl, m_db_blocks.size(), [&](uint64_t index) -> crypto::hash
  {
    return m_scratchpad_wr.get_scratchpad()[index%m_scratchpad_wr.get_scratchpad().size()];
  });

  if (!check_hash(proof_of_work, current_diffic))
  {
    LOG_PRINT_L0("Block with id: " << id << ENDL
      << "have not enough proof of work: " << proof_of_work << ENDL
      << "nexpected difficulty: " << current_diffic);
    bvc.m_verifivation_failed = true;
    return false;
  }
  if (m_checkpoints.is_in_checkpoint_zone(get_current_blockchain_height()))
  {
    m_is_in_checkpoint_zone = true;
    if (!m_checkpoints.check_block(get_current_blockchain_height(), id))
    {
      LOG_ERROR("CHECKPOINT VALIDATION FAILED");
      bvc.m_verifivation_failed = true;
      return false;
    }
  }
  else
    m_is_in_checkpoint_zone = false;

  PROF_L1_FINISH(longhash_calculating_time);

  PROF_L2_START(prevalidate_miner_tx_time);
  if (!prevalidate_miner_transaction(bl, m_db_blocks.size()))
  {
    LOG_PRINT_L0("Block with id: " << id
      << " failed to pass prevalidation");
    bvc.m_verifivation_failed = true;
    return false;
  }
  PROF_L2_FINISH(prevalidate_miner_tx_time);

  PROF_L2_START(add_miner_tx_time);
  size_t coinbase_blob_size = get_object_blobsize(bl.miner_tx);
  size_t cumulative_block_size = coinbase_blob_size;
  //process transactions
  if (!add_transaction_from_block(bl.miner_tx, get_transaction_hash(bl.miner_tx), id, get_current_blockchain_height()))
  {
    LOG_PRINT_L0("Block with id: " << id << " failed to add transaction to blockchain storage");
    bvc.m_verifivation_failed = true;
    return false;
  }
  PROF_L2_FINISH(add_miner_tx_time);


  PROF_L2_START(process_transactions_time);
  size_t tx_processed_count = 0;
  uint64_t fee_summary = 0;
  BOOST_FOREACH(const crypto::hash& tx_id, bl.tx_hashes)
  {
    transaction tx;
    size_t blob_size = 0;
    uint64_t fee = 0;
    if (!m_tx_pool.take_tx(tx_id, tx, blob_size, fee))
    {
      LOG_PRINT_L0("Block with id: " << id << "have at least one unknown transaction with id: " << tx_id);
      purge_block_data_from_blockchain(bl, tx_processed_count);
      //add_block_as_invalid(bl, id);
      bvc.m_verifivation_failed = true;
      return false;
    }

    //If we under checkpoints, ring signatures should be pruned    
    if (m_is_in_checkpoint_zone)
    {
      tx.signatures.clear();
    }

    if (!check_tx_inputs(tx))
    {
      LOG_PRINT_L0("Block with id: " << id << "have at least one transaction (id: " << tx_id << ") with wrong inputs.");
      currency::tx_verification_context tvc = AUTO_VAL_INIT(tvc);
      bool add_res = m_tx_pool.add_tx(tx, tvc, true);
      CHECK_AND_ASSERT_MES2(add_res, "handle_block_to_main_chain: failed to add transaction back to transaction pool");
      purge_block_data_from_blockchain(bl, tx_processed_count);
      add_block_as_invalid(bl, id);
      LOG_PRINT_L0("Block with id " << id << " added as invalid becouse of wrong inputs in transactions");
      bvc.m_verifivation_failed = true;
      return false;
    }

    if (!add_transaction_from_block(tx, tx_id, id, get_current_blockchain_height()))
    {
      LOG_PRINT_L0("Block with id: " << id << " failed to add transaction to blockchain storage");
      currency::tx_verification_context tvc = AUTO_VAL_INIT(tvc);
      bool add_res = m_tx_pool.add_tx(tx, tvc, true);
      CHECK_AND_ASSERT_MES2(add_res, "handle_block_to_main_chain: failed to add transaction back to transaction pool");
      purge_block_data_from_blockchain(bl, tx_processed_count);
      bvc.m_verifivation_failed = true;
      return false;
    }
    fee_summary += fee;
    cumulative_block_size += blob_size;
    ++tx_processed_count;
  }
  PROF_L2_FINISH(process_transactions_time);


  PROF_L2_START(validate_miner_tx_time);
  uint64_t base_reward = 0;
  uint64_t already_generated_coins = m_db_blocks.size() ? m_db_blocks.back()->already_generated_coins : 0;
  uint64_t already_donated_coins = m_db_blocks.size() ? m_db_blocks.back()->already_donated_coins : 0;
  uint64_t donation_total = 0;
  if (!validate_miner_transaction(bl, cumulative_block_size, fee_summary, base_reward, already_generated_coins, already_donated_coins, donation_total))
  {
    LOG_PRINT_L0("Block with id: " << id
      << " have wrong miner transaction");
    purge_block_data_from_blockchain(bl, tx_processed_count);
    bvc.m_verifivation_failed = true;
    return false;
  }
  PROF_L2_FINISH(validate_miner_tx_time);


  PROF_L2_START(update_blocks_table_time1);
  block_extended_info bei = boost::value_initialized<block_extended_info>();
  bei.bl = bl;
  bei.scratch_offset = m_scratchpad_wr.get_scratchpad().size();
  bei.block_cumulative_size = cumulative_block_size;
  bei.cumulative_difficulty = current_diffic;
  bei.already_generated_coins = already_generated_coins + base_reward;
  bei.already_donated_coins = already_donated_coins + donation_total;
  if (m_db_blocks.size())
    bei.cumulative_difficulty += m_db_blocks.back()->cumulative_difficulty;

  bei.height = m_db_blocks.size();

  auto blocks_index_ptr = m_db_blocks_index.get(id);
  if (blocks_index_ptr)
  {
    LOG_ERROR("block with id: " << id << " already in block indexes");
    purge_block_data_from_blockchain(bl, tx_processed_count);
    bvc.m_verifivation_failed = true;
    return false;
  }
  m_db_blocks_index.set(id, bei.height);
  PROF_L2_FINISH(update_blocks_table_time1);

  PROF_L2_START(update_scratchpad_time);
  if (!m_scratchpad_wr.push_block_scratchpad_data(bl))
  {
    LOG_ERROR("Internal error for block id: " << id << ": failed to put_block_scratchpad_data");
    purge_block_data_from_blockchain(bl, tx_processed_count);
    bvc.m_verifivation_failed = true;
    return false;
  }

#ifdef ENABLE_HASHING_DEBUG  
  LOG_PRINT_L3("SCRATCHPAD_SHOT FOR H=" << bei.height + 1 << ENDL << dump_scratchpad(m_scratchpad_wr.get_scratchpad()));
#endif
  PROF_L2_FINISH(update_scratchpad_time);

  PROF_L2_START(update_blocks_table_time2);
  m_db_blocks.push_back(bei);
  update_next_comulative_size_limit();
  PROF_L2_FINISH(update_blocks_table_time2);

  PROF_L1_FINISH(block_processing_time);
  LOG_PRINT_L1("+++++ BLOCK SUCCESSFULLY ADDED" << ENDL << "id:\t" << id
    << ENDL << "PoW:\t" << proof_of_work
    << ENDL << "HEIGHT " << bei.height << ", difficulty:\t" << current_diffic
    << ENDL << "block reward: " << print_money(fee_summary + base_reward) << "(" << print_money(base_reward) << " + " << print_money(fee_summary)
    << ")" << ((bei.height%CURRENCY_DONATIONS_INTERVAL) ? std::string("") : std::string("donation: ") + print_money(donation_total)) << ", coinbase_blob_size: " << coinbase_blob_size << ", cumulative size: " << cumulative_block_size
    << PROF_L1_STR(", " << print_mcsec_as_ms(block_processing_time))
    << PROF_L1_STR("(" << print_mcsec_as_ms(target_calculating_time))
    << PROF_L1_STR("/" << print_mcsec_as_ms(longhash_calculating_time))
    << PROF_L1_STR(")ms")
    << PROF_L2_STR("  Profiling results (ms): ")
    << PROF_L2_STR_MS(ENDL << "  timestamp_check_time:       ", timestamp_check_time)
    << PROF_L2_STR_MS(ENDL << "  target_calculating_time:    ", target_calculating_time)
    << PROF_L2_STR_MS(ENDL << "  longhash_calculating_time:  ", longhash_calculating_time)
    << PROF_L2_STR_MS(ENDL << "  prevalidate_miner_tx_time:  ", prevalidate_miner_tx_time)
    << PROF_L2_STR_MS(ENDL << "  add_miner_tx_time:          ", add_miner_tx_time)
    << PROF_L2_STR_MS(ENDL << "  process_transactions_time:  ", process_transactions_time)
    << PROF_L2_STR_MS(ENDL << "  validate_miner_tx_time:     ", validate_miner_tx_time)
    << PROF_L2_STR_MS(ENDL << "  update_blocks_table_time1:  ", update_blocks_table_time1)
    << PROF_L2_STR_MS(ENDL << "  update_scratchpad_time:     ", update_scratchpad_time)
    << PROF_L2_STR_MS(ENDL << "  update_blocks_table_time2:  ", update_blocks_table_time2)
  );

  bvc.m_added_to_main_chain = true;


  m_tx_pool.on_blockchain_inc(bei.height, id);
  //LOG_PRINT_L0("BLOCK: " << ENDL << "" << dump_obj_as_json(bei.bl));
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::update_next_comulative_size_limit()
{
  std::vector<size_t> sz;
  get_last_n_blocks_sizes(sz, CURRENCY_REWARD_BLOCKS_WINDOW);

  uint64_t median = misc_utils::median(sz);
  if (median <= CURRENCY_BLOCK_GRANTED_FULL_REWARD_ZONE)
    median = CURRENCY_BLOCK_GRANTED_FULL_REWARD_ZONE;

  m_db_current_block_cumul_sz_limit = median * 2;
  return true;
}
//------------------------------------------------------------------
bool blockchain_storage::add_new_block(const block& bl_, block_verification_context& bvc)
{
  try
  {
    block bl = bl_;
    crypto::hash id = get_block_hash(bl);
    CRITICAL_REGION_LOCAL(m_tx_pool);//to avoid deadlock lets lock tx_pool for whole add/reorganize process
    CRITICAL_REGION_LOCAL1(m_blockchain_lock);
    PROF_L2_START(time_have_block_check);
    if (have_block(id))
    {
      LOG_PRINT_L3("block with id = " << id << " already exists");
      bvc.m_already_exists = true;
      return false;
    }
    PROF_L2_FINISH(time_have_block_check);

    //check that block refers to chain tail

    PROF_L2_START(time_handle_alt);
    if (!(bl.prev_id == get_top_block_id()))
    {
      //chain switching or wrong block
      bvc.m_added_to_main_chain = false;
      m_db.begin_transaction();
      bool r = handle_alternative_block(bl, id, bvc);
      m_db.commit_transaction();
      return r;
      //never relay alternative blocks
    }
    PROF_L2_FINISH(time_handle_alt);

    PROF_L2_START(time_handle_main);
    PROF_L2_START(time_handle_main_1);
    m_db.begin_transaction();
    PROF_L2_FINISH(time_handle_main_1);
    PROF_L2_START(time_handle_main_2);
    bool res = handle_block_to_main_chain(bl, id, bvc);
    PROF_L2_FINISH(time_handle_main_2);
    PROF_L2_START(time_handle_main_3);
    m_db.commit_transaction();
    PROF_L2_FINISH(time_handle_main_3);
    PROF_L2_FINISH(time_handle_main);

#if PROFILING_LEVEL >= 2
    LOG_PRINT_L2("bcs::add_new_block timings (ms) have block: " << print_mcsec_as_ms(time_have_block_check) << ", handle alt: " << print_mcsec_as_ms(time_handle_alt) <<
      ", handle main: " << print_mcsec_as_ms(time_handle_main) << "(" << print_mcsec_as_ms(time_handle_main_1) << "+" << print_mcsec_as_ms(time_handle_main_2) << "+" << print_mcsec_as_ms(time_handle_main_3) << ")");
#endif

    return res;
  }
  catch (const std::exception& ex)
  {
    bvc.m_verifivation_failed = true;
    bvc.m_added_to_main_chain = false;
    m_db.abort_transaction();
    LOG_ERROR("UNKNOWN EXCEPTION WHILE ADDINIG NEW BLOCK: " << ex.what());
    return false;
  }
  catch (...)
  {
    bvc.m_verifivation_failed = true;
    bvc.m_added_to_main_chain = false;
    m_db.abort_transaction();
    LOG_ERROR("UNKNOWN EXCEPTION WHILE ADDINIG NEW BLOCK.");
    return false;
  }
}
