// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <memory>
#include <boost/serialization/list.hpp>
#include <boost/serialization/vector.hpp>
#include <atomic>

#include "include_base_utils.h"
#include "profile_tools.h"


#include "currency_core/currency_boost_serialization.h"
#include "currency_core/account_boost_serialization.h"

#include "wallet_rpc_server_commans_defs.h"
#include "currency_core/currency_format_utils.h"
#include "common/unordered_containers_boost_serialization.h"
#include "storages/portable_storage_template_helper.h"
#include "crypto/chacha8.h"
#include "crypto/hash.h"
#include "core_rpc_proxy.h"
#include "core_default_rpc_proxy.h"
#include "wallet_errors.h"
#include "eos/portable_archive.hpp"
#include "currency_core/core_runtime_config.h"

#define WALLET_DEFAULT_TX_SPENDABLE_AGE                               10
#define WALLET_POS_MINT_CHECK_HEIGHT_INTERVAL                         10
#define WALLET_FILE_SERIALIZATION_VERSION                             22
namespace tools
{
#pragma pack(push, 1)
  struct wallet_file_binary_header
  {
    uint64_t m_signature;
    uint16_t m_cb_keys;
    uint64_t m_cb_body;
  };
#pragma pack (pop)


  struct money_transfer2_details
  {
    std::vector<size_t> receive_indices;
    std::vector<size_t> spent_indices;
  };

  class i_wallet2_callback
  {
  public:
    virtual void on_new_block(uint64_t /*height*/, const currency::block& /*block*/) {}
    virtual void on_money_received(uint64_t /*height*/, const currency::transaction& /*tx*/, size_t /*out_index*/) {}
    virtual void on_money_spent(uint64_t /*height*/, const currency::transaction& /*in_tx*/, size_t /*out_index*/, const currency::transaction& /*spend_tx*/) {}
    virtual void on_transfer2(const wallet_rpc::wallet_transfer_info& wti) {}
    virtual void on_pos_block_found(const currency::block& /*block*/) {}
    virtual void on_sync_progress(const uint64_t& /*percents*/) {}
  };

  struct tx_dust_policy
  {
    uint64_t dust_threshold;
    bool add_to_fee;
    currency::account_public_address addr_for_dust;

    tx_dust_policy(uint64_t a_dust_threshold = 0, bool an_add_to_fee = true, currency::account_public_address an_addr_for_dust = currency::account_public_address())
      : dust_threshold(a_dust_threshold)
      , add_to_fee(an_add_to_fee)
      , addr_for_dust(an_addr_for_dust)
    {
    }
  };


  class test_generator;

  class wallet2
  {
    wallet2(const wallet2&) : m_stop(false),
                              m_wcallback(new i_wallet2_callback()), 
                              height_of_start_sync(0), 
                              last_sync_percent(0)
    {};
  public:
    wallet2() : m_stop(false), 
                m_wcallback(new i_wallet2_callback()), //stub
                m_core_proxy(new default_http_core_proxy()), 
                m_upper_transaction_size_limit(0), 
                height_of_start_sync(0), 
                last_sync_percent(0)
    {
      m_core_runtime_config = currency::get_default_core_runtime_config();
    };
    struct transfer_details
    {
      uint64_t m_block_height;
      uint64_t m_block_timestamp;
      currency::transaction m_tx;
      size_t m_internal_output_index;
      uint64_t m_global_output_index;
      bool m_spent;
      crypto::key_image m_key_image; //TODO: key_image stored twice :(


      uint64_t amount() const { return m_tx.vout[m_internal_output_index].amount; }
    };


    struct payment_details
    {
      crypto::hash m_tx_hash;
      uint64_t m_amount;
      uint64_t m_block_height;
      uint64_t m_unlock_time;
    };

    struct mining_context
    {
      currency::COMMAND_RPC_SCAN_POS::request sp;
      currency::COMMAND_RPC_SCAN_POS::response rsp;
      currency::wide_difficulty_type basic_diff;
      currency::stake_modifier_type sm;
    };


    typedef std::unordered_multimap<crypto::hash, payment_details> payment_container;

    typedef std::vector<transfer_details> transfer_container;

    struct keys_file_data
    {
      crypto::chacha8_iv iv;
      std::string account_data;

      BEGIN_SERIALIZE_OBJECT()
        FIELD(iv)
        FIELD(account_data)
      END_SERIALIZE()
    };
    void assign_account(const currency::account_base& acc);
    void generate(const std::string& wallet, const std::string& password);
    void restore(const std::string& path, const std::string& pass, const std::string& restore_key);
    void load(const std::string& wallet, const std::string& password);    
    void store();
    std::string get_wallet_path(){ return m_wallet_file; }
    currency::account_base& get_account(){return m_account;}

    void get_recent_transfers_history(std::vector<wallet_rpc::wallet_transfer_info>& trs, size_t offset, size_t count);
    void get_unconfirmed_transfers(std::vector<wallet_rpc::wallet_transfer_info>& trs);
    void init(const std::string& daemon_address = "http://localhost:8080");
    bool deinit();

    void stop() { m_stop.store(true, std::memory_order_relaxed); }

    //i_wallet2_callback* callback() const { return m_wcallback; }
    //void callback(i_wallet2_callback* callback) { m_callback = callback; }
    void callback(std::shared_ptr<i_wallet2_callback> callback){ m_wcallback = callback;}

    void scan_tx_pool();
    void refresh();
    void refresh(size_t & blocks_fetched);
    void refresh(size_t & blocks_fetched, bool& received_money, std::atomic<bool>& stop);
    bool refresh(size_t & blocks_fetched, bool& received_money, bool& ok, std::atomic<bool>& stop);
    void refresh(std::atomic<bool>& stop);
    
    void push_offer(const currency::offer_details_ex& od, currency::transaction& res_tx);
    void cancel_offer_by_id(const crypto::hash& tx_id, uint64_t of_ind, currency::transaction& tx);
    void request_alias_registration(const currency::alias_info& ai, currency::transaction& res_tx, uint64_t fee);


    bool set_core_proxy(const std::shared_ptr<i_core_proxy>& proxy);
    std::shared_ptr<i_core_proxy> get_core_proxy();
    uint64_t balance();
    uint64_t balance(uint64_t& unloked);
    uint64_t unlocked_balance();
    template<typename T>
    void transfer(const std::vector<currency::tx_destination_entry>& dsts,
                  size_t fake_outputs_count, 
                  uint64_t unlock_time, 
                  uint64_t fee, 
                  const std::vector<currency::extra_v>& extra, 
                  const std::vector<currency::attachment_v> attachments, 
                  T destination_split_strategy, 
                  const tx_dust_policy& dust_policy);
    template<typename T>
    void transfer(const std::vector<currency::tx_destination_entry>& dsts,
                  size_t fake_outputs_count, 
                  uint64_t unlock_time, 
                  uint64_t fee, 
                  const std::vector<currency::extra_v>& extra, 
                  const std::vector<currency::attachment_v> attachments, 
                  T destination_split_strategy, 
                  const tx_dust_policy& dust_policy, 
                  currency::transaction &tx,
                  uint8_t tx_outs_attr = CURRENCY_TO_KEY_OUT_RELAXED);
    void transfer(const std::vector<currency::tx_destination_entry>& dsts, 
                  size_t fake_outputs_count, 
                  uint64_t unlock_time, 
                  uint64_t fee, 
                  const std::vector<currency::extra_v>& extra, 
                  const std::vector<currency::attachment_v> attachments);
    void transfer(const std::vector<currency::tx_destination_entry>& dsts, 
                  size_t fake_outputs_count, 
                  uint64_t unlock_time, 
                  uint64_t fee, 
                  const std::vector<currency::extra_v>& extra, 
                  const std::vector<currency::attachment_v> attachments, 
                  currency::transaction& tx);
    bool check_connection();
    template<typename idle_condition_cb_t> //do refresh as external callback
    static bool scan_pos(mining_context& cxt, std::atomic<bool>& stop, idle_condition_cb_t idle_condition_cb);
    bool fill_mining_context(mining_context& ctx);
    void get_transfers(wallet2::transfer_container& incoming_transfers) const;
    void get_payments(const crypto::hash& payment_id, std::list<payment_details>& payments) const;
    bool get_transfer_address(const std::string& adr_str, currency::account_public_address& addr);
    uint64_t get_blockchain_current_height() const { return m_local_bc_height; }
    template <class t_archive>
    inline void serialize(t_archive &a, const unsigned int ver)
    {
      if (ver < WALLET_FILE_SERIALIZATION_VERSION)
      {
        LOG_PRINT_BLUE("[WALLET STORAGE] data truncated cz new version", LOG_LEVEL_0);
        return;
      } 
      a & m_blockchain;
      a & m_transfers;
      a & m_account_public_address;
      a & m_key_images;
      a & m_unconfirmed_txs;
      a & m_payments;
      a & m_transfer_history;
      a & m_offers_secret_keys;

    }
    static uint64_t select_indices_for_transfer(std::list<size_t>& ind, std::map<uint64_t, std::list<size_t> >& found_free_amounts, uint64_t needed_money);

    //PoS
    //synchronous version of function 
    bool try_mint_pos();
    //for unit tests
    friend class test_generator;
    
    //next functions in public area only because of test_generator
    //TODO: Need refactoring - remove it back to private zone 
    bool prepare_and_sign_pos_block(currency::block& b,
      const currency::pos_entry& pos_info,
      const crypto::public_key& source_tx_pub_key,
      uint64_t in_tx_output_index,
      const std::vector<const crypto::public_key*>& keys_ptrs);
    void process_new_blockchain_entry(const currency::block& b, 
      currency::block_complete_entry& bche, 
      const crypto::hash& bl_id,
      uint64_t height);
    bool get_pos_entries(currency::COMMAND_RPC_SCAN_POS::request& req);
    bool build_minted_block(const currency::COMMAND_RPC_SCAN_POS::request& req, const currency::COMMAND_RPC_SCAN_POS::response& rsp);
    bool reset_history();
    bool is_transfer_unlocked(const transfer_details& td) const;
    void get_mining_history(wallet_rpc::mining_history& hist);
    void set_core_runtime_config(const currency::core_runtime_config& pc);  
    currency::core_runtime_config& get_core_runtime_config();
    bool backup_keys(const std::string& path);
    bool reset_password(const std::string& pass);
private:
    bool store_keys(std::string& buff, const std::string& password);
    void load_keys(const std::string& keys_file_name, const std::string& password);
    void process_new_transaction(const currency::transaction& tx, uint64_t height, const currency::block& b);
    void detach_blockchain(uint64_t height);
    void get_short_chain_history(std::list<crypto::hash>& ids);
    bool is_tx_spendtime_unlocked(uint64_t unlock_time) const;
    bool clear();
    void pull_blocks(size_t& blocks_added, std::atomic<bool>& stop);
    uint64_t select_transfers(uint64_t needed_money, size_t fake_outputs_count, uint64_t dust, std::list<transfer_container::iterator>& selected_transfers);
    bool prepare_file_names(const std::string& file_path);
    void process_unconfirmed(const currency::transaction& tx, std::string& recipient, std::string& recipient_alias, std::string& comment);
    void add_sent_unconfirmed_tx(const currency::transaction& tx, 
                                 uint64_t change_amount, 
                                 const std::string& recipient,
                                 const std::string& comment);
    void update_current_tx_limit();
    void prepare_wti(wallet_rpc::wallet_transfer_info& wti, uint64_t height, uint64_t timestamp, const currency::transaction& tx, uint64_t amount, const money_transfer2_details& td);
    void prepare_wti_decrypted_attachments(wallet_rpc::wallet_transfer_info& wti, const std::vector<currency::attachment_v>& decrypted_att);
    void handle_money_received2(const currency::block& b,
                                const currency::transaction& tx, 
                                uint64_t amount, 
                                const money_transfer2_details& td);
    void handle_money_spent2(const currency::block& b,  
                             const currency::transaction& in_tx, 
                             uint64_t amount, 
                             const money_transfer2_details& td, 
                             const std::string& recipient, 
                             const std::string& recipient_alias,
                             const std::string& comment);
    std::string get_alias_for_address(const std::string& addr);
    bool is_coin_age_okay(const transfer_details& tr);
    static bool build_kernel(const currency::pos_entry& pe, const currency::stake_modifier_type& stake_modifier, currency::stake_kernel& kernel, uint64_t& coindays_weight, uint64_t timestamp);
    bool is_connected_to_net();
    void drop_offer_keys();    
    bool is_coin_age_okay_for_pos(const transfer_details& tr);

    currency::account_base m_account;
    std::string m_wallet_file;
    std::string m_password;
    std::vector<crypto::hash> m_blockchain;
    std::atomic<uint64_t> m_local_bc_height; //temporary workaround 
    std::atomic<uint64_t> m_last_bc_timestamp; 

    transfer_container m_transfers;
    payment_container m_payments;
    std::unordered_map<crypto::key_image, size_t> m_key_images;
    currency::account_public_address m_account_public_address;
    uint64_t m_upper_transaction_size_limit; //TODO: auto-calc this value or request from daemon, now use some fixed value

    std::atomic<bool> m_stop;
    std::vector<wallet_rpc::wallet_transfer_info> m_transfer_history;
    std::unordered_map<crypto::hash, currency::transaction> m_unconfirmed_in_transfers;
    std::unordered_map<crypto::hash, tools::wallet_rpc::wallet_transfer_info> m_unconfirmed_txs;
    std::unordered_map<crypto::hash, std::pair<currency::keypair, uint64_t> > m_offers_secret_keys;

    std::shared_ptr<i_core_proxy> m_core_proxy;
    std::shared_ptr<i_wallet2_callback> m_wcallback;
    uint64_t height_of_start_sync;
    uint64_t last_sync_percent;
    currency::core_runtime_config m_core_runtime_config;
    
 
  };

}


BOOST_CLASS_VERSION(tools::wallet2, WALLET_FILE_SERIALIZATION_VERSION)
BOOST_CLASS_VERSION(tools::wallet_rpc::wallet_transfer_info, 4)


namespace boost
{
  namespace serialization
  {
    template <class Archive>
    inline void serialize(Archive &a, tools::wallet2::transfer_details &x, const boost::serialization::version_type ver)
    {
      a & x.m_block_height;
      a & x.m_block_timestamp;
      a & x.m_global_output_index;
      a & x.m_internal_output_index;
      a & x.m_tx;
      a & x.m_spent;
      a & x.m_key_image;
    }

    template <class Archive>
    inline void serialize(Archive& a, tools::wallet2::payment_details& x, const boost::serialization::version_type ver)
    {
      a & x.m_tx_hash;
      a & x.m_amount;
      a & x.m_block_height;
      a & x.m_unlock_time;
    }
    
    template <class Archive>
    inline void serialize(Archive& a, tools::wallet_rpc::wallet_transfer_info_details& x, const boost::serialization::version_type ver)
    {
      a & x.rcv;
      a & x.spn;
    }

    template <class Archive>
    inline void serialize(Archive& a, tools::wallet_rpc::wallet_transfer_info& x, const boost::serialization::version_type ver)
    {      
      x.fee = 0;

      a & x.amount;
      a & x.timestamp;
      a & x.tx_hash;
      a & x.height;
      a & x.tx_blob_size;
      a & x.payment_id;
      a & x.remote_address; 
      a & x.is_income;
      a & x.td;
      a & x.tx;
      a & x.recipient_alias;
      a & x.comment;
      a & x.fee;
      a & x.is_service;
      a & x.is_anonymous;

      //do not store unlock_time
      if (Archive::is_loading::value)
        x.unlock_time = x.tx.unlock_time;
    }

  }
}

namespace tools
{
  namespace detail
  {
    //----------------------------------------------------------------------------------------------------
    inline void digit_split_strategy(const std::vector<currency::tx_destination_entry>& dsts,
      const currency::tx_destination_entry& change_dst, uint64_t dust_threshold,
      std::vector<currency::tx_destination_entry>& splitted_dsts, uint64_t& dust)
    {
      splitted_dsts.clear();
      dust = 0;

      BOOST_FOREACH(auto& de, dsts)
      {
        currency::decompose_amount_into_digits(de.amount, dust_threshold,
          [&](uint64_t chunk) { splitted_dsts.push_back(currency::tx_destination_entry(chunk, de.addr)); },
          [&](uint64_t a_dust) { splitted_dsts.push_back(currency::tx_destination_entry(a_dust, de.addr)); } );
      }

      currency::decompose_amount_into_digits(change_dst.amount, dust_threshold,
        [&](uint64_t chunk) { splitted_dsts.push_back(currency::tx_destination_entry(chunk, change_dst.addr)); },
        [&](uint64_t a_dust) { dust = a_dust; } );
    }
    //----------------------------------------------------------------------------------------------------
    inline void null_split_strategy(const std::vector<currency::tx_destination_entry>& dsts,
      const currency::tx_destination_entry& change_dst, uint64_t dust_threshold,
      std::vector<currency::tx_destination_entry>& splitted_dsts, uint64_t& dust)
    {
      splitted_dsts = dsts;

      dust = 0;
      uint64_t change = change_dst.amount;
      if (0 < dust_threshold)
      {
        for (uint64_t order = 10; order <= 10 * dust_threshold; order *= 10)
        {
          uint64_t dust_candidate = change_dst.amount % order;
          uint64_t change_candidate = (change_dst.amount / order) * order;
          if (dust_candidate <= dust_threshold)
          {
            dust = dust_candidate;
            change = change_candidate;
          }
          else
          {
            break;
          }
        }
      }

      if (0 != change)
      {
        splitted_dsts.push_back(currency::tx_destination_entry(change, change_dst.addr));
      }
    }
    //----------------------------------------------------------------------------------------------------
    inline void print_source_entry(const currency::tx_source_entry& src)
    {
      std::string indexes;
      std::for_each(src.outputs.begin(), src.outputs.end(), [&](const currency::tx_source_entry::output_entry& s_e) { indexes += boost::to_string(s_e.first) + " "; });
      LOG_PRINT_L0("amount=" << currency::print_money(src.amount) << ", real_output=" <<src.real_output << ", real_output_in_tx_index=" << src.real_output_in_tx_index << ", indexes: " << indexes);
    }
    //----------------------------------------------------------------------------------------------------
  }
  //----------------------------------------------------------------------------------------------------
  template<typename T>
  void wallet2::transfer(const std::vector<currency::tx_destination_entry>& dsts, size_t fake_outputs_count,
    uint64_t unlock_time, uint64_t fee, const std::vector<currency::extra_v>& extra, const std::vector<currency::attachment_v> attachments, T destination_split_strategy, const tx_dust_policy& dust_policy)
  {
    currency::transaction tx;
    transfer(dsts, fake_outputs_count, unlock_time, fee, extra, attachments, destination_split_strategy, dust_policy, tx);
  }

  template<typename T>
  void wallet2::transfer(const std::vector<currency::tx_destination_entry>& dsts, 
                         size_t fake_outputs_count,
                         uint64_t unlock_time, 
                         uint64_t fee, 
                         const std::vector<currency::extra_v>& extra, 
                         const std::vector<currency::attachment_v> attachments, 
                         T destination_split_strategy, 
                         const tx_dust_policy& dust_policy, 
                         currency::transaction &tx,
                         uint8_t tx_outs_attr)
  {
    if (!is_connected_to_net())
    {
      CHECK_AND_THROW_WALLET_EX(true, error::wallet_internal_error,
        "Transfer attempt while daemon offline");
    }

    using namespace currency;
    CHECK_AND_THROW_WALLET_EX(dsts.empty(), error::zero_destination);

    uint64_t needed_money = fee;
    BOOST_FOREACH(auto& dt, dsts)
    {
      CHECK_AND_THROW_WALLET_EX(0 == dt.amount, error::zero_destination);
      needed_money += dt.amount;
      CHECK_AND_THROW_WALLET_EX(needed_money < dt.amount, error::tx_sum_overflow, dsts, fee);
    }

    std::list<transfer_container::iterator> selected_transfers;
    uint64_t found_money = select_transfers(needed_money, fake_outputs_count, dust_policy.dust_threshold, selected_transfers);
    CHECK_AND_THROW_WALLET_EX(found_money < needed_money, error::not_enough_money, found_money, needed_money - fee, fee);

    typedef COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::out_entry out_entry;
    typedef currency::tx_source_entry::output_entry tx_output_entry;

    COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::response daemon_resp = AUTO_VAL_INIT(daemon_resp);
    if(fake_outputs_count)
    {
      COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::request req = AUTO_VAL_INIT(req);
      req.use_forced_mix_outs = false; //add this feature to UI later
      req.outs_count = fake_outputs_count + 1;// add one to make possible (if need) to skip real output key
      BOOST_FOREACH(transfer_container::iterator it, selected_transfers)
      {
        CHECK_AND_THROW_WALLET_EX(it->m_tx.vout.size() <= it->m_internal_output_index, error::wallet_internal_error,
          "m_internal_output_index = " + std::to_string(it->m_internal_output_index) +
          " is greater or equal to outputs count = " + std::to_string(it->m_tx.vout.size()));
        req.amounts.push_back(it->amount());
      }

      bool r = m_core_proxy->call_COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS(req, daemon_resp);
      CHECK_AND_THROW_WALLET_EX(!r, error::no_connection_to_daemon, "getrandom_outs.bin");
      CHECK_AND_THROW_WALLET_EX(daemon_resp.status == CORE_RPC_STATUS_BUSY, error::daemon_busy, "getrandom_outs.bin");
      CHECK_AND_THROW_WALLET_EX(daemon_resp.status != CORE_RPC_STATUS_OK, error::get_random_outs_error, daemon_resp.status);
      CHECK_AND_THROW_WALLET_EX(daemon_resp.outs.size() != selected_transfers.size(), error::wallet_internal_error,
        "daemon returned wrong response for getrandom_outs.bin, wrong amounts count = " +
        std::to_string(daemon_resp.outs.size()) + ", expected " +  std::to_string(selected_transfers.size()));

      std::vector<COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount> scanty_outs;
      BOOST_FOREACH(COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount& amount_outs, daemon_resp.outs)
      {
        if (amount_outs.outs.size() < fake_outputs_count)
        {
          scanty_outs.push_back(amount_outs);
        }
      }
      CHECK_AND_THROW_WALLET_EX(!scanty_outs.empty(), error::not_enough_outs_to_mix, scanty_outs, fake_outputs_count);
    }
 
    //prepare inputs
    size_t i = 0;
    std::vector<currency::tx_source_entry> sources;
    BOOST_FOREACH(transfer_container::iterator it, selected_transfers)
    {
      sources.resize(sources.size()+1);
      currency::tx_source_entry& src = sources.back();
      transfer_details& td = *it;
      src.amount = td.amount();
      //paste mixin transaction
      if(daemon_resp.outs.size())
      {
        daemon_resp.outs[i].outs.sort([](const out_entry& a, const out_entry& b){return a.global_amount_index < b.global_amount_index;});
        BOOST_FOREACH(out_entry& daemon_oe, daemon_resp.outs[i].outs)
        {
          if(td.m_global_output_index == daemon_oe.global_amount_index)
            continue;
          tx_output_entry oe;
          oe.first = daemon_oe.global_amount_index;
          oe.second = daemon_oe.out_key;
          src.outputs.push_back(oe);
          if(src.outputs.size() >= fake_outputs_count)
            break;
        }
      }

      //paste real transaction to the random index
      auto it_to_insert = std::find_if(src.outputs.begin(), src.outputs.end(), [&](const tx_output_entry& a)
      {
        return a.first >= td.m_global_output_index;
      });
      //size_t real_index = src.outputs.size() ? (rand() % src.outputs.size() ):0;
      tx_output_entry real_oe;
      real_oe.first = td.m_global_output_index;
      real_oe.second = boost::get<txout_to_key>(td.m_tx.vout[td.m_internal_output_index].target).key;
      auto interted_it = src.outputs.insert(it_to_insert, real_oe);
      src.real_out_tx_key = get_tx_pub_key_from_extra(td.m_tx);
      src.real_output = interted_it - src.outputs.begin();
      src.real_output_in_tx_index = td.m_internal_output_index;
      detail::print_source_entry(src);
      ++i;
    }

    currency::tx_destination_entry change_dts = AUTO_VAL_INIT(change_dts);
    if (needed_money < found_money)
    {
      change_dts.addr = m_account.get_keys().m_account_address;
      change_dts.amount = found_money - needed_money;
    }

    uint64_t dust = 0;
    std::vector<currency::tx_destination_entry> splitted_dsts;
    destination_split_strategy(dsts, change_dts, dust_policy.dust_threshold, splitted_dsts, dust);
    CHECK_AND_THROW_WALLET_EX(dust_policy.dust_threshold < dust, error::wallet_internal_error, "invalid dust value: dust = " +
      std::to_string(dust) + ", dust_threshold = " + std::to_string(dust_policy.dust_threshold));
    if (0 != dust && !dust_policy.add_to_fee)
    {
      splitted_dsts.push_back(currency::tx_destination_entry(dust, dust_policy.addr_for_dust));
    }

    currency::keypair one_time_key = AUTO_VAL_INIT(one_time_key);
    bool r = currency::construct_tx(m_account.get_keys(), sources, splitted_dsts, extra, attachments, tx, one_time_key.sec, unlock_time);
    CHECK_AND_THROW_WALLET_EX(!r, error::tx_not_constructed, sources, splitted_dsts, unlock_time);
    //update_current_tx_limit();
    CHECK_AND_THROW_WALLET_EX(CURRENCY_MAX_TRANSACTION_BLOB_SIZE <= get_object_blobsize(tx), error::tx_too_big, tx, m_upper_transaction_size_limit);
    one_time_key.pub = get_tx_pub_key_from_extra(tx);

    std::string key_images;
    bool all_are_txin_to_key = std::all_of(tx.vin.begin(), tx.vin.end(), [&](const txin_v& s_e) -> bool
    {
      CHECKED_GET_SPECIFIC_VARIANT(s_e, const txin_to_key, in, false);
      key_images += boost::to_string(in.k_image) + " ";
      return true;
    });
    CHECK_AND_THROW_WALLET_EX(!all_are_txin_to_key, error::unexpected_txin_type, tx);

    COMMAND_RPC_SEND_RAW_TX::request req;
    req.tx_as_hex = epee::string_tools::buff_to_hex_nodelimer(tx_to_blob(tx));
    COMMAND_RPC_SEND_RAW_TX::response daemon_send_resp;
    r = m_core_proxy->call_COMMAND_RPC_SEND_RAW_TX(req, daemon_send_resp);  
    CHECK_AND_THROW_WALLET_EX(!r, error::no_connection_to_daemon, "sendrawtransaction");
    CHECK_AND_THROW_WALLET_EX(daemon_send_resp.status == CORE_RPC_STATUS_BUSY, error::daemon_busy, "sendrawtransaction");
    CHECK_AND_THROW_WALLET_EX(daemon_send_resp.status != CORE_RPC_STATUS_OK, error::tx_rejected, tx, daemon_send_resp.status);
    
    std::string recipient;
    for (const auto& d : dsts)
    {
      if (recipient.size())
        recipient += ", ";
      recipient += get_account_address_as_str(d.addr);
    }
    currency::tx_comment cm;
    get_type_in_variant_container(attachments, cm);
    add_sent_unconfirmed_tx(tx, change_dts.amount, recipient, cm.comment);
    if (currency::have_type_in_variant_container<currency::offer_details>(tx.attachment))
    {
      //store private tx key
      m_offers_secret_keys[currency::get_transaction_hash(tx)] = std::make_pair(one_time_key, static_cast<uint64_t>(time(nullptr)));
    }

    LOG_PRINT_L2("transaction " << get_transaction_hash(tx) << " generated ok and sent to daemon, key_images: [" << key_images << "]");

    BOOST_FOREACH(transfer_container::iterator it, selected_transfers)
      it->m_spent = true;

    LOG_PRINT_L0("Transaction successfully sent. <" << get_transaction_hash(tx) << ">" << ENDL 
                  << "Commission: " << print_money(fee+dust) << " (dust: " << print_money(dust) << ")" << ENDL
                  << "Balance: " << print_money(balance()) << ENDL
                  << "Unlocked: " << print_money(unlocked_balance()) << ENDL
                  << "Please, wait for confirmation for your balance to be unlocked.");
  }
  //--------------------------------------------------------------------------------
  template<typename idle_condition_cb_t> //do refresh as external callback
  bool wallet2::scan_pos(mining_context& cxt,
    std::atomic<bool>& stop,
    idle_condition_cb_t idle_condition_cb)
  {
    cxt.rsp.status = CORE_RPC_STATUS_NOT_FOUND;
    uint64_t timstamp_start = time(nullptr);
    uint64_t timstamp_last_idle_call = time(nullptr);


    for (size_t i = 0; i != cxt.sp.pos_entries.size(); i++)
    {
      //set timestamp starting from timestamp%POS_SCAN_STEP = 0
      uint64_t adjusted_starter_timestamp = timstamp_start - POS_SCAN_STEP;
      adjusted_starter_timestamp = POS_SCAN_STEP * 2 - (adjusted_starter_timestamp%POS_SCAN_STEP) + adjusted_starter_timestamp;
      bool go_past = true;
      for (uint64_t step = 0; step <= POS_SCAN_WINDOW;)
      {

        if (time(nullptr) - timstamp_last_idle_call > WALLET_POS_MINT_CHECK_HEIGHT_INTERVAL)
        {
          if (!idle_condition_cb())
          {
            LOG_PRINT_L0("Detected new block, minting interrupted");
            break;
          }
          timstamp_last_idle_call = time(nullptr);
        }

        uint64_t ts = go_past ? adjusted_starter_timestamp - step : adjusted_starter_timestamp + step;
        PROFILE_FUNC("general_mining_iteration");
        if (stop)
          return false;
        currency::stake_kernel sk = AUTO_VAL_INIT(sk);
        uint64_t coindays_weight = 0;
        build_kernel(cxt.sp.pos_entries[i], cxt.sm, sk, coindays_weight, ts);
        crypto::hash kernel_hash;
        {
          PROFILE_FUNC("calc_hash");
          kernel_hash = crypto::cn_fast_hash(&sk, sizeof(sk));
        }

        currency::wide_difficulty_type this_coin_diff = cxt.basic_diff / coindays_weight;
        bool check_hash_res;
        {
          PROFILE_FUNC("check_hash");
          check_hash_res = currency::check_hash(kernel_hash, this_coin_diff);
        }
        if (!check_hash_res)
        {
          if (!step)
          {
            step += POS_SCAN_STEP;
            continue;
          }
          else if (go_past)
          {
            go_past = false;
          }
          else
          {
            go_past = true;
            step += POS_SCAN_STEP;
          }
        }
        else
        {
          //found kernel
          LOG_PRINT_GREEN("Found kernel: amount=" << cxt.sp.pos_entries[i].amount << ENDL
            << ", difficulty_basic=" << cxt.basic_diff << ", diff for this coin: " << this_coin_diff << ENDL
            << ", index=" << cxt.sp.pos_entries[i].index << ENDL
            << ", kernel info: " << ENDL
            << print_stake_kernel_info(sk),
            LOG_LEVEL_0);
          cxt.rsp.index = i;
          cxt.rsp.block_timestamp = ts;
          cxt.rsp.status = CORE_RPC_STATUS_OK;
          return true;
        }
      }
    }
    cxt.rsp.status = CORE_RPC_STATUS_NOT_FOUND;
    return false;
  }


}
