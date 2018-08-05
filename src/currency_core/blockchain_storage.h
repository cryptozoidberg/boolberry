// Copyright (c) 2012-2013 The Cryptonote developers
// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/list.hpp>

#include <boost/foreach.hpp>
#include <atomic>


#include "serialization/serialization.h"
#include "serialization/string.h"
#include "serialization/multiprecision.h"


#include "tx_pool.h"
#include "currency_basic.h"
#include "common/util.h"
#include "common/db_bridge.h"
#include "currency_protocol/currency_protocol_defs.h"
#include "rpc/core_rpc_server_commands_defs.h"
#include "difficulty.h"
#include "common/difficulty_boost_serialization.h"
#include "currency_core/currency_format_utils.h"
#include "verification_context.h"
#include "crypto/hash.h"
#include "checkpoints.h"
#include "scratchpad_helpers.h"
#include "file_io_utils.h"
#include "common/db_lmdb_adapter.h"

MAKE_POD_C11(crypto::key_image);
typedef std::pair<crypto::hash, uint64_t> macro_alias_1;
MAKE_POD_C11(macro_alias_1);

POD_MAKE_HASHABLE(currency, account_public_address);

namespace currency
{

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  class blockchain_storage
  {
  public:
    struct transaction_chain_entry
    {
      transaction tx;
      uint64_t m_keeper_block_height;
      std::vector<uint64_t> m_global_output_indexes;
      std::vector<bool> m_spent_flags;
      uint32_t version;

      DEFINE_SERIALIZATION_VERSION(1)

      BEGIN_SERIALIZE_OBJECT()
        VERSION_ENTRY(version)
        FIELD(version)
        FIELDS(tx)
        FIELD(m_keeper_block_height)
        FIELD(m_global_output_indexes)
        FIELD(m_spent_flags)
      END_SERIALIZE()
    };

    struct block_extended_info
    {
      block   bl;
      uint64_t height;
      size_t block_cumulative_size;
      wide_difficulty_type cumulative_difficulty;
      uint64_t already_generated_coins;
      uint64_t already_donated_coins;
      uint64_t scratch_offset;

      uint32_t version;

      DEFINE_SERIALIZATION_VERSION(1)
      BEGIN_SERIALIZE_OBJECT()
        VERSION_ENTRY(version)
        FIELDS(bl)
        FIELD(height)
        FIELD(block_cumulative_size)
        FIELD(cumulative_difficulty)
        FIELD(already_generated_coins)
        FIELD(already_donated_coins)
        FIELD(scratch_offset)
      END_SERIALIZE()
    };

    typedef db::key_to_array_accessor_base<uint64_t, std::pair<crypto::hash, uint64_t>, false>  outputs_container;

    blockchain_storage(tx_memory_pool& tx_pool);

    static void init_options(boost::program_options::options_description& desc);

    bool init(const boost::program_options::variables_map& vm, const std::string& config_folder);
    bool deinit();

    bool set_checkpoints(checkpoints&& chk_pts);
    checkpoints& get_checkpoints() { return m_checkpoints; }

    //bool push_new_block();
    bool get_blocks(uint64_t start_offset, size_t count, std::list<block>& blocks, std::list<transaction>& txs);
    bool get_blocks(uint64_t start_offset, size_t count, std::list<block>& blocks);
    bool get_alternative_blocks(std::list<block>& blocks);
    size_t get_alternative_blocks_count();
    crypto::hash get_block_id_by_height(uint64_t height);
    bool get_block_by_hash(const crypto::hash &h, block &blk);
    bool get_block_by_height(uint64_t h, block &blk);
    //void get_all_known_block_ids(std::list<crypto::hash> &main, std::list<crypto::hash> &alt, std::list<crypto::hash> &invalid);

    template<class archive_t>
    void serialize(archive_t & ar, const unsigned int version);

    bool have_tx(const crypto::hash &id);
    bool have_tx_keyimges_as_spent(const transaction &tx);
    bool have_tx_keyimg_as_spent(const crypto::key_image &key_im);
    std::shared_ptr<transaction> get_tx(const crypto::hash &id);

    template<class visitor_t>
    bool scan_outputkeys_for_indexes(const txin_to_key& tx_in_to_key, visitor_t& vis, uint64_t* pmax_related_block_height = NULL);

    uint64_t get_current_blockchain_height();
    crypto::hash get_top_block_id();
    crypto::hash get_top_block_id(uint64_t& height);
    bool get_top_block(block& b);
    wide_difficulty_type get_difficulty_for_next_block();
    bool add_new_block(const block& bl_, block_verification_context& bvc);
    bool reset_and_set_genesis_block(const block& b);
    bool create_block_template(block& b, const account_public_address& miner_address, wide_difficulty_type& di, uint64_t& height, const blobdata& ex_nonce, bool vote_for_donation, const alias_info& ai);
    bool have_block(const crypto::hash& id);
    size_t get_total_transactions();
    bool get_outs(uint64_t amount, std::list<crypto::public_key>& pkeys);
    bool get_short_chain_history(std::list<crypto::hash>& ids);
    bool find_blockchain_supplement(const std::list<crypto::hash>& qblock_ids, NOTIFY_RESPONSE_CHAIN_ENTRY::request& resp);
    bool find_blockchain_supplement(const std::list<crypto::hash>& qblock_ids, uint64_t& starter_offset);
    bool find_blockchain_supplement(const std::list<crypto::hash>& qblock_ids, std::list<std::pair<block, std::list<transaction> > >& blocks, uint64_t& total_height, uint64_t& start_height, size_t max_count);
    bool handle_get_objects(NOTIFY_REQUEST_GET_OBJECTS::request& arg, NOTIFY_RESPONSE_GET_OBJECTS::request& rsp);
    bool handle_get_objects(const COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::request& req, COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::response& res);
    bool get_random_outs_for_amounts(const COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::request& req, COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::response& res);
    bool get_backward_blocks_sizes(size_t from_height, std::vector<size_t>& sz, size_t count);
    bool get_tx_outputs_gindexs(const crypto::hash& tx_id, std::vector<uint64_t>& indexs);
    bool get_alias_info(const std::string& alias, alias_info_base& info);
    std::string get_alias_by_address(const account_public_address& addr);
    bool get_all_aliases(std::list<alias_info>& aliases);
    uint64_t get_aliases_count();
    uint64_t get_scratchpad_size();
    //bool store_blockchain();
    bool check_tx_input(const txin_to_key& txin, const crypto::hash& tx_prefix_hash, const std::vector<crypto::signature>& sig, uint64_t* pmax_related_block_height = NULL);
    bool check_tx_inputs(const transaction& tx, const crypto::hash& tx_prefix_hash, uint64_t* pmax_used_block_height = NULL);
    bool check_tx_inputs(const transaction& tx, uint64_t* pmax_used_block_height = NULL);
    bool check_tx_inputs(const transaction& tx, uint64_t& pmax_used_block_height, crypto::hash& max_used_block_id);
    uint64_t get_current_comulative_blocksize_limit();
    uint64_t get_already_generated_coins(crypto::hash &hash, uint64_t &count);
    uint64_t get_already_donated_coins(crypto::hash &hash, uint64_t &count);
    bool get_block_containing_tx(const crypto::hash &txId, crypto::hash &blockId, uint64_t &blockHeight);
    uint64_t get_current_hashrate(size_t aprox_count);
    bool extport_scratchpad_to_file(const std::string& path);
    //bool print_transactions_statistics();
    bool update_spent_tx_flags_for_input(uint64_t amount, uint64_t global_index, bool spent);
    bool update_spent_tx_flags_for_input(const crypto::hash& tx_id, size_t n, bool spent);
    bool clear();
    bool is_storing_blockchain(){ return m_is_blockchain_storing; }
    wide_difficulty_type block_difficulty(size_t i);
    bool copy_scratchpad(std::vector<crypto::hash>& dst);//TODO: not the best way, add later update method instead of full copy    
    bool copy_scratchpad_as_blob(std::string& dst);
    bool prune_aged_alt_blocks();
    bool get_transactions_daily_stat(uint64_t& daily_cnt, uint64_t& daily_volume);
    bool check_keyimages(const std::list<crypto::key_image>& images, std::list<bool>& images_stat);//true - unspent, false - spent
    void initialize_db_solo_options_values();
    bool get_block_extended_info_by_hash(const crypto::hash &h, block_extended_info &blk) const;
    bool get_block_extended_info_by_height(uint64_t h, block_extended_info &blk) const;
    bool lookfor_donation(const transaction& tx, uint64_t& donation, uint64_t& royalty);

    template<class t_ids_container, class t_blocks_container, class t_missed_container>
    bool get_blocks(const t_ids_container& block_ids, t_blocks_container& blocks, t_missed_container& missed_bs)
    {
      CRITICAL_REGION_LOCAL(m_blockchain_lock);

      BOOST_FOREACH(const auto& bl_id, block_ids)
      {
        auto block_ind_ptr = m_db_blocks_index.find(bl_id);
        if (!block_ind_ptr)
          missed_bs.push_back(bl_id);
        else
        {
          CHECK_AND_ASSERT_MES(*block_ind_ptr < m_db_blocks.size(), false, "Internal error: bl_id=" << string_tools::pod_to_hex(bl_id)
            << " have index record with offset=" << *block_ind_ptr << ", bigger then m_blocks.size()=" << m_db_blocks.size());
          blocks.push_back(m_db_blocks[*block_ind_ptr]->bl);
        }
      }
      return true;
    }

    template<class t_ids_container, class t_tx_container, class t_missed_container>
    bool get_transactions(const t_ids_container& txs_ids, t_tx_container& txs, t_missed_container& missed_txs)const
    {
      CRITICAL_REGION_LOCAL(m_blockchain_lock);

      BOOST_FOREACH(const auto& tx_id, txs_ids)
      {
        auto tx_ptr = m_db_transactions.find(tx_id);
        if (!tx_ptr)
        {
          transaction tx;
          if (!m_tx_pool.get_transaction(tx_id, tx))
            missed_txs.push_back(tx_id);
          else
            txs.push_back(tx);
        }
        else
          txs.push_back(tx_ptr->tx);
      }
      return true;
    }
    //debug functions
    void print_blockchain(uint64_t start_index, uint64_t end_index);
    void print_blockchain_index();
    void print_blockchain_outs(const std::string& file);

  private:
    //-------------- DB containers --------------
    typedef db::key_value_accessor_base<crypto::hash, uint64_t, false> blocks_by_id_index; //typedef std::unordered_map<crypto::hash, size_t> blocks_by_id_index;
    typedef db::key_value_accessor_base<crypto::hash, transaction_chain_entry, true> transactions_container; //typedef std::unordered_map<crypto::hash, transaction_chain_entry> transactions_container;

    typedef db::key_value_accessor_base<crypto::key_image, bool, false> key_images_container; //typedef std::unordered_set<crypto::key_image> key_images_container;
    typedef db::array_accessor<block_extended_info, true> blocks_container;


    typedef db::key_value_accessor_base<std::string, std::list<alias_info_base>, true> aliases_container; //typedef std::map<std::string, std::list<extra_alias_entry_base>> aliases_container; //alias can be address address address + view key
    typedef db::key_value_accessor_base<account_public_address, std::set<std::string>, true> address_to_aliases_container;//typedef std::unordered_map<account_public_address, std::set<std::string> > address_to_aliases_container;
    typedef db::key_value_accessor_base<crypto::hash, std::pair<crypto::hash, uint64_t>, false> multisig_outs_container;//  typedef std::unordered_map<crypto::hash, std::pair<crypto::hash, size_t>> multisig_outs_container;// hash key - multisig output id, pair<tx_id, n> - reference to tx id + output in transaction
    typedef db::key_value_accessor_base<uint64_t, uint64_t, false> solo_options_container;


    //------
    typedef std::unordered_map<crypto::hash, block_extended_info> blocks_ext_by_hash;

    tx_memory_pool& m_tx_pool;

    //main accessor
    std::shared_ptr<db::lmdb_adapter> m_lmdb_adapter;
    db::db_bridge_base m_db;
    //containers
    blocks_container m_db_blocks;
    blocks_by_id_index m_db_blocks_index;
    transactions_container m_db_transactions;
    key_images_container m_db_spent_keys;
    solo_options_container m_db_solo_options;
    db::single_value<uint64_t, uint64_t, solo_options_container> m_db_current_block_cumul_sz_limit;
    db::single_value<uint64_t, uint64_t, solo_options_container> m_db_current_pruned_rs_height;
    db::single_value<uint64_t, std::string, solo_options_container, true> m_db_last_worked_version;
    db::single_value<uint64_t, uint64_t, solo_options_container> m_db_storage_major_compability_version;
    outputs_container m_db_outputs;
    aliases_container m_db_aliases;
    address_to_aliases_container m_db_addr_to_alias;
    
    scratchpad_wrapper::scratchpad_container m_db_scratchpad_internal;
    scratchpad_wrapper m_scratchpad_wr;


    // state members 

    // all alternative chains
    blocks_ext_by_hash m_invalid_blocks;     // crypto::hash -> block_extended_info
    blocks_ext_by_hash m_alternative_chains; // crypto::hash -> block_extended_info

    std::atomic<bool> m_is_in_checkpoint_zone;
    std::atomic<bool> m_is_blockchain_storing;

    std::string m_config_folder;
    account_keys m_donations_account;
    account_keys m_royalty_account;
    //events
    checkpoints m_checkpoints;

    epee::file_io_utils::native_filesystem_handle m_locker_file;

    // mutable members
    mutable critical_section m_blockchain_lock; // TODO: add here reader/writer lock

    bool switch_to_alternative_blockchain(std::list<blocks_ext_by_hash::iterator>& alt_chain);
    bool pop_block_from_blockchain();
    bool purge_block_data_from_blockchain(const block& b, size_t processed_tx_count);
    bool purge_transaction_from_blockchain(const crypto::hash& tx_id);
    bool purge_transaction_keyimages_from_blockchain(const transaction& tx, bool strict_check);

    bool handle_block_to_main_chain(const block& bl, block_verification_context& bvc);
    bool handle_block_to_main_chain(const block& bl, const crypto::hash& id, block_verification_context& bvc);
    bool handle_alternative_block(const block& b, const crypto::hash& id, block_verification_context& bvc);
    wide_difficulty_type get_next_difficulty_for_alternative_chain(const std::list<blocks_ext_by_hash::iterator>& alt_chain, block_extended_info& bei);
    bool prevalidate_miner_transaction(const block& b, uint64_t height);
    bool validate_miner_transaction(const block& b, size_t cumulative_block_size, uint64_t fee, uint64_t& base_reward, uint64_t already_generated_coins, uint64_t already_donated_coins, uint64_t& donation_total);
    bool validate_transaction(const block& b, uint64_t height, const transaction& tx);
    bool rollback_blockchain_switching(std::list<block>& original_chain, size_t rollback_height);
    bool add_transaction_from_block(const transaction& tx, const crypto::hash& tx_id, const crypto::hash& bl_id, uint64_t bl_height);
    bool push_transaction_to_global_outs_index(const transaction& tx, const crypto::hash& tx_id, std::vector<uint64_t>& global_indexes);
    bool pop_transaction_from_global_index(const transaction& tx, const crypto::hash& tx_id);
    bool get_last_n_blocks_sizes(std::vector<size_t>& sz, size_t count);
    bool add_out_to_get_random_outs(COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount& result_outs, uint64_t amount, size_t i, uint64_t mix_count, bool use_only_forced_to_mix = false);
    bool is_tx_spendtime_unlocked(uint64_t unlock_time);
    bool add_block_as_invalid(const block& bl, const crypto::hash& h);
    bool add_block_as_invalid(const block_extended_info& bei, const crypto::hash& h);
    size_t find_end_of_allowed_index(uint64_t amount);
    bool check_block_timestamp_main(const block& b);
    bool check_block_timestamp(std::vector<uint64_t> timestamps, const block& b);
    uint64_t get_adjusted_time();
    bool complete_timestamps_vector(uint64_t start_height, std::vector<uint64_t>& timestamps);
    bool update_next_comulative_size_limit();
    bool get_block_for_scratchpad_alt(uint64_t connection_height, uint64_t block_index, std::list<blockchain_storage::blocks_ext_by_hash::iterator>& alt_chain, block & b);
    bool process_blockchain_tx_extra(const transaction& tx);
    bool unprocess_blockchain_tx_extra(const transaction& tx);
    bool pop_alias_info(const alias_info& ai);
    bool put_alias_info(const alias_info& ai);
    bool validate_donations_value(uint64_t donation, uint64_t royalty);
    //uint64_t get_block_avr_donation_vote(const block& b);
    bool get_required_donations_value_for_next_block(uint64_t& don_am); //applicable only for each CURRENCY_DONATIONS_INTERVAL-th block
    //void fill_addr_to_alias_dict();
    //bool resync_spent_tx_flags();
    bool prune_ring_signatures_if_need();
    bool prune_ring_signatures(uint64_t height, uint64_t& transactions_pruned, uint64_t& signatures_pruned);
    bool check_instance(const std::string& data_dir);
  };

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/

#define CURRENT_BLOCKCHAIN_STORAGE_ARCHIVE_VER          27
#define CURRENT_TRANSACTION_CHAIN_ENTRY_ARCHIVE_VER     3
#define CURRENT_BLOCK_EXTENDED_INFO_ARCHIVE_VER         1

  //------------------------------------------------------------------
  template<class visitor_t>
  bool blockchain_storage::scan_outputkeys_for_indexes(const txin_to_key& tx_in_to_key, visitor_t& vis, uint64_t* pmax_related_block_height)
  {
    CRITICAL_REGION_LOCAL(m_blockchain_lock);

    uint64_t outs_count_for_amount = m_db_outputs.get_item_size(tx_in_to_key.amount);

    if (!outs_count_for_amount)
      return false;

    std::vector<uint64_t> absolute_offsets = relative_output_offsets_to_absolute(tx_in_to_key.key_offsets);

    //std::vector<std::pair<crypto::hash, size_t> >& amount_outs_vec = it->second;

    size_t count = 0;
    BOOST_FOREACH(uint64_t i, absolute_offsets)
    {
      crypto::hash tx_id = null_hash;
      size_t n = 0;
      if (i >= outs_count_for_amount)
      {
        LOG_ERROR("Wrong index in transaction inputs: " << i << ", expected maximum " << outs_count_for_amount - 1);
        return false;
      }
      tx_id = m_db_outputs.get_subitem(tx_in_to_key.amount, i)->first;
      n = m_db_outputs.get_subitem(tx_in_to_key.amount, i)->second;


      auto tx_ptr = m_db_transactions.find(tx_id);
      CHECK_AND_ASSERT_MES(tx_ptr, false, "Wrong transaction id in output indexes: " << string_tools::pod_to_hex(tx_id));
      CHECK_AND_ASSERT_MES(n < tx_ptr->tx.vout.size(), false,
        "Wrong index in transaction outputs: " << n << ", expected less then " << tx_ptr->tx.vout.size());
      //check mix_attr

      CHECKED_GET_SPECIFIC_VARIANT(tx_ptr->tx.vout[n].target, const txout_to_key, outtk, false);
      if (outtk.mix_attr > 1)
        CHECK_AND_ASSERT_MES(tx_in_to_key.key_offsets.size() >= outtk.mix_attr, false, "transaction out[" << count << "] is marked to be used minimum with " << static_cast<uint32_t>(outtk.mix_attr) << "parts in ring signature, but input used only " << tx_in_to_key.key_offsets.size());
      else if (outtk.mix_attr == CURRENCY_TO_KEY_OUT_FORCED_NO_MIX)
        CHECK_AND_ASSERT_MES(tx_in_to_key.key_offsets.size() == 1, false, "transaction out[" << count << "] is marked to be used without mixins in ring signature, but input used is " << tx_in_to_key.key_offsets.size());

      if (!vis.handle_output(tx_ptr->tx, tx_ptr->tx.vout[n]))
      {
        LOG_PRINT_L0("Failed to handle_output for output id = " << tx_id << ", no " << n);
        return false;
      }
      if (pmax_related_block_height)
      {
        if (*pmax_related_block_height < tx_ptr->m_keeper_block_height)
          *pmax_related_block_height = tx_ptr->m_keeper_block_height;
      }
    }

    return true;
  }
}




BOOST_CLASS_VERSION(currency::blockchain_storage, CURRENT_BLOCKCHAIN_STORAGE_ARCHIVE_VER)
BOOST_CLASS_VERSION(currency::blockchain_storage::transaction_chain_entry, CURRENT_TRANSACTION_CHAIN_ENTRY_ARCHIVE_VER)
BOOST_CLASS_VERSION(currency::blockchain_storage::block_extended_info, CURRENT_BLOCK_EXTENDED_INFO_ARCHIVE_VER)

