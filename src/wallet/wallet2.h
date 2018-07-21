// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <memory>
#include <boost/serialization/list.hpp>
#include <boost/serialization/vector.hpp>
#include <atomic>

#include "include_base_utils.h"

#include "currency_core/account_boost_serialization.h"
#include "currency_core/currency_basic_impl.h"
#include "wallet_rpc_server_commans_defs.h"
#include "currency_core/currency_format_utils.h"
#include "common/unordered_containers_boost_serialization.h"
#include "storages/portable_storage_template_helper.h"
#include "crypto/chacha8.h"
#include "crypto/hash.h"
#include "core_rpc_proxy.h"
#include "core_default_rpc_proxy.h"
#include "wallet_errors.h"

#define DEFAULT_TX_SPENDABLE_AGE                               10

namespace tools
{

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
    virtual void on_money_sent(const wallet_rpc::wallet_transfer_info& wti) {}
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

  class wallet2
  {
    wallet2(const wallet2&) : m_run(true), m_is_view_only(false), m_callback(0), m_unconfirmed_balance(0) {};
  public:
    wallet2() : m_run(true), m_callback(0), m_is_view_only(false), m_core_proxy(new default_http_core_proxy()), m_unconfirmed_balance(0)
    {};
    struct transfer_details
    {
      uint64_t m_block_height;
      currency::transaction m_tx;
      size_t m_internal_output_index;
      uint64_t m_global_output_index;
      bool m_spent;
      crypto::key_image m_key_image; //TODO: key_image stored twice :(

      uint64_t amount() const { return m_tx.vout[m_internal_output_index].amount; }
    };

    struct unconfirmed_transfer_details
    {
      currency::transaction m_tx;
      uint64_t      m_change;
      time_t        m_sent_time; 
      std::string   m_recipient;
      std::string   m_recipient_alias;
    };

    struct payment_details
    {
      crypto::hash m_tx_hash;
      uint64_t m_amount;
      uint64_t m_block_height;
      uint64_t m_unlock_time;
    };
    
    typedef std::unordered_multimap<currency::payment_id_t, payment_details> payment_container;

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

    std::vector<unsigned char> generate(const std::string& wallet, const std::string& password);
    void restore(const std::string& wallet, const std::vector<unsigned char>& restore_seed, const std::string& password);
    void load(const std::string& wallet, const std::string& password);    
    void store();
    std::string get_wallet_path(){ return m_keys_file; }
    currency::account_base& get_account(){return m_account;}

    void get_recent_transfers_history(std::vector<wallet_rpc::wallet_transfer_info>& trs, size_t offset, size_t count);
    void get_unconfirmed_transfers(std::vector<wallet_rpc::wallet_transfer_info>& trs);
    void init(const std::string& daemon_address = "http://localhost:8080");
    bool deinit();

    void stop() { m_run.store(false, std::memory_order_relaxed); }

    i_wallet2_callback* callback() const { return m_callback; }
    void callback(i_wallet2_callback* callback) { m_callback = callback; }

    void scan_tx_pool();
    void refresh();
    void refresh(size_t & blocks_fetched);
    void refresh(size_t & blocks_fetched, bool& received_money);
    bool refresh(size_t & blocks_fetched, bool& received_money, bool& ok);

    void sign_transfer(const std::string& tx_sources_file, const std::string& signed_tx_file, currency::transaction& tx);
    void submit_transfer(const std::string& tx_sources_file, const std::string& target_file, currency::transaction& tx);


    void sign_text(const std::string& text, crypto::signature& sig);
    std::string validate_signed_text(const std::string& addr, const std::string& text, const crypto::signature& sig);
    bool generate_view_wallet(const std::string new_name, const std::string& password);
    bool is_view_only(){ return m_is_view_only;}
    
    bool set_core_proxy(std::shared_ptr<i_core_proxy>& proxy);
    std::shared_ptr<i_core_proxy> get_core_proxy();
    uint64_t balance();
    uint64_t unlocked_balance();
    int64_t unconfirmed_balance();
    template<typename T>
    void transfer(const std::vector<currency::tx_destination_entry>& dsts, size_t fake_outputs_count, uint64_t unlock_time, uint64_t fee, const std::vector<uint8_t>& extra, T destination_split_strategy, const tx_dust_policy& dust_policy);
    template<typename T>
    void transfer(const std::vector<currency::tx_destination_entry>& dsts, size_t fake_outputs_count, uint64_t unlock_time, uint64_t fee, const std::vector<uint8_t>& extra, T destination_split_strategy, const tx_dust_policy& dust_policy, currency::transaction &tx, uint8_t tx_outs_attr = CURRENCY_TO_KEY_OUT_RELAXED);
    template<typename T>
    void transfer(const std::vector<currency::tx_destination_entry>& dsts, size_t fake_outputs_count, uint64_t unlock_time, uint64_t fee, const std::vector<uint8_t>& extra, T destination_split_strategy, const tx_dust_policy& dust_policy, currency::transaction &tx, uint8_t tx_outs_attr, currency::blobdata& relay_blob, bool do_not_relay = false);
    void transfer(const std::vector<currency::tx_destination_entry>& dsts, size_t fake_outputs_count, uint64_t unlock_time, uint64_t fee, const std::vector<uint8_t>& extra);
    void transfer(const std::vector<currency::tx_destination_entry>& dsts, size_t fake_outputs_count, uint64_t unlock_time, uint64_t fee, const std::vector<uint8_t>& extra, currency::transaction& tx);
    void transfer(const std::vector<currency::tx_destination_entry>& dsts, size_t fake_outputs_count, uint64_t unlock_time, uint64_t fee, const std::vector<uint8_t>& extra, currency::transaction& tx, currency::blobdata& relay_blob, bool do_not_relay = false);
    
    bool get_tx_key(const crypto::hash &txid, crypto::secret_key &tx_key) const;
    bool check_connection();
    void get_transfers(wallet2::transfer_container& incoming_transfers) const;
    bool get_transfers(const wallet_rpc::COMMAND_RPC_GET_TRANSFERS::request& req, wallet_rpc::COMMAND_RPC_GET_TRANSFERS::response& res) const;
    void get_payments(const currency::payment_id_t& payment_id, std::list<payment_details>& payments, uint64_t min_height = 0) const;
    bool get_transfer_address(const std::string& adr_str, currency::account_public_address& addr, currency::payment_id_t& payment_id);
    bool store_keys(const std::string& keys_file_name, const std::string& password, bool save_as_view_wallet = false);
    uint64_t get_blockchain_current_height() const { return m_local_bc_height; }
    template <class t_archive>
    inline void serialize(t_archive &a, const unsigned int ver)
    {
      if(ver < 5)
        return;
      a & m_blockchain;
      a & m_transfers;
      a & m_account_public_address;
      a & m_key_images;
      if(ver < 6)
        return;
      a & m_unconfirmed_txs;
      if(ver < 7)
        return;
      a & m_payments;
      if (ver < 8)
        return;
      a & m_transfer_history;
      if (ver < 9)
          return;
      a & m_tx_keys;

    }
    static uint64_t select_indices_for_transfer(std::list<size_t>& ind, std::map<uint64_t, std::list<size_t> >& found_free_amounts, uint64_t needed_money);
  private:

    void load_keys(const std::string& keys_file_name, const std::string& password);
    void process_new_transaction(const currency::transaction& tx, uint64_t height, const currency::block& b);
    void process_new_blockchain_entry(const currency::block& b, currency::block_complete_entry& bche, crypto::hash& bl_id, uint64_t height);
    void detach_blockchain(uint64_t height);
    void get_short_chain_history(std::list<crypto::hash>& ids);
    bool is_tx_spendtime_unlocked(uint64_t unlock_time) const;
    bool is_transfer_unlocked(const transfer_details& td) const;
    bool clear();
    void pull_blocks(size_t& blocks_added);
    uint64_t select_transfers(uint64_t needed_money, size_t fake_outputs_count, uint64_t dust, std::list<transfer_container::iterator>& selected_transfers);
    bool prepare_file_names(const std::string& file_path);
    void process_unconfirmed(const currency::transaction& tx, std::string& recipient, std::string& recipient_alias);
    void add_sent_unconfirmed_tx(const currency::transaction& tx, uint64_t change_amount, std::string recipient);
    void update_current_tx_limit();
    void prepare_wti(wallet_rpc::wallet_transfer_info& wti, uint64_t height, uint64_t timestamp, const currency::transaction& tx, uint64_t amount, const money_transfer2_details& td)const;
    void handle_money_received2(const currency::block& b, const currency::transaction& tx, uint64_t amount, const money_transfer2_details& td);
    void handle_money_spent2(const currency::block& b, const currency::transaction& in_tx, uint64_t amount, const money_transfer2_details& td, const std::string& recipient, const std::string& recipient_alias);
    std::string get_alias_for_address(const std::string& addr);
    void wallet_transfer_info_from_unconfirmed_transfer_details(const unconfirmed_transfer_details& utd, wallet_rpc::wallet_transfer_info& wti)const;
    void finalize_transaction(const currency::create_tx_arg& create_tx_param, const currency::create_tx_res& create_tx_result, bool do_not_relay = false);
    void resend_unconfirmed();

    currency::account_base m_account;
    bool m_is_view_only;
    std::string m_wallet_file;
    std::string m_keys_file;
    std::vector<crypto::hash> m_blockchain;
    std::atomic<uint64_t> m_local_bc_height; //temporary workaround 
    std::unordered_map<crypto::hash, unconfirmed_transfer_details> m_unconfirmed_txs;

    transfer_container m_transfers;
    payment_container m_payments;
    std::unordered_map<crypto::key_image, size_t> m_key_images;
    currency::account_public_address m_account_public_address;
    uint64_t m_upper_transaction_size_limit; //TODO: auto-calc this value or request from daemon, now use some fixed value

    std::atomic<bool> m_run;
    std::vector<wallet_rpc::wallet_transfer_info> m_transfer_history;
    std::unordered_map<crypto::hash, wallet_rpc::wallet_transfer_info> m_unconfirmed_in_transfers;
    uint64_t m_unconfirmed_balance;
    std::shared_ptr<i_core_proxy> m_core_proxy;
    i_wallet2_callback* m_callback;
    std::unordered_map<crypto::hash, crypto::secret_key> m_tx_keys;
  };
}


BOOST_CLASS_VERSION(tools::wallet2, 10)
BOOST_CLASS_VERSION(tools::wallet2::unconfirmed_transfer_details, 3)
BOOST_CLASS_VERSION(tools::wallet_rpc::wallet_transfer_info, 3)


namespace boost
{
  namespace serialization
  {
    template <class Archive>
    inline void serialize(Archive &a, tools::wallet2::transfer_details &x, const boost::serialization::version_type ver)
    {
      a & x.m_block_height;
      a & x.m_global_output_index;
      a & x.m_internal_output_index;
      a & x.m_tx;
      a & x.m_spent;
      a & x.m_key_image;
    }

    template <class Archive>
    inline void serialize(Archive &a, tools::wallet2::unconfirmed_transfer_details &x, const boost::serialization::version_type ver)
    {
      a & x.m_change;
      a & x.m_sent_time;
      a & x.m_tx;
      if (ver < 2)
        return;
      a & x.m_recipient;
      if (ver < 3)
        return;
      a & x.m_recipient_alias;
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
      a & x.destinations; 
      a & x.is_income;
      a & x.td;
      a & x.tx;
      if (ver < 2)
        return;
      a & x.destination_alias;
      if (ver < 3)
        return;
      a & x.fee;

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
    uint64_t unlock_time, uint64_t fee, const std::vector<uint8_t>& extra, T destination_split_strategy, const tx_dust_policy& dust_policy)
  {
    currency::transaction tx;
    transfer(dsts, fake_outputs_count, unlock_time, fee, extra, destination_split_strategy, dust_policy, tx);
  }
  //----------------------------------------------------------------------------------------------------
  template<typename T>
  void wallet2::transfer(const std::vector<currency::tx_destination_entry>& dsts, size_t fake_outputs_count,
    uint64_t unlock_time, uint64_t fee, const std::vector<uint8_t>& extra, T destination_split_strategy, const tx_dust_policy& dust_policy, currency::transaction &tx, uint8_t tx_outs_attr)
  {
    currency::blobdata dummy;
    return transfer(dsts, fake_outputs_count, unlock_time, fee, extra, destination_split_strategy, dust_policy, tx, tx_outs_attr, dummy, false);
  }
  //----------------------------------------------------------------------------------------------------
  template<typename T>
  void wallet2::transfer(const std::vector<currency::tx_destination_entry>& dsts, size_t fake_outputs_count,
    uint64_t unlock_time, uint64_t fee, const std::vector<uint8_t>& extra, T destination_split_strategy, const tx_dust_policy& dust_policy, currency::transaction &tx, uint8_t tx_outs_attr, currency::blobdata& relay_blob, bool do_not_relay)
  {
    using namespace currency;
    CHECK_AND_THROW_WALLET_EX(dsts.empty(), error::zero_destination);

    create_tx_context ctc = AUTO_VAL_INIT(ctc);
    create_tx_arg& create_tx_param = ctc.arg;
    create_tx_param.extra = extra;
    create_tx_param.unlock_time = unlock_time;
    create_tx_param.tx_outs_attr = tx_outs_attr;
    create_tx_param.spend_pub_key = m_account.get_keys().m_account_address.m_spend_public_key;
    create_tx_res& create_tx_result = ctc.res;
    for (auto& d : dsts)
      create_tx_param.recipients.push_back(d.addr);

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
    std::vector<currency::tx_source_entry>& sources = create_tx_param.sources;
    BOOST_FOREACH(transfer_container::iterator it, selected_transfers)
    {
      sources.resize(sources.size()+1);
      currency::tx_source_entry& src = sources.back();
      transfer_details& td = *it;
      src.transfer_index = it - m_transfers.begin();
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
      auto inserted_it = src.outputs.insert(it_to_insert, real_oe);
      src.real_out_tx_key = get_tx_pub_key_from_extra(td.m_tx);
      src.real_output = inserted_it - src.outputs.begin();
      src.real_output_in_tx_index = td.m_internal_output_index;
      detail::print_source_entry(src);
      ++i;
    }

    currency::tx_destination_entry change_dts = AUTO_VAL_INIT(change_dts);
    if (needed_money < found_money)
    {
      change_dts.addr = m_account.get_keys().m_account_address;
      change_dts.amount = found_money - needed_money;
      create_tx_param.change_amount = change_dts.amount;
    }

    uint64_t& dust = create_tx_param.dust;
    std::vector<currency::tx_destination_entry>& splitted_dsts = create_tx_param.splitted_dsts;
    destination_split_strategy(dsts, change_dts, dust_policy.dust_threshold, splitted_dsts, dust);
    CHECK_AND_THROW_WALLET_EX(dust_policy.dust_threshold < dust, error::wallet_internal_error, "invalid dust value: dust = " +
      std::to_string(dust) + ", dust_threshold = " + std::to_string(dust_policy.dust_threshold));
    if (0 != dust && !dust_policy.add_to_fee)
    {
      splitted_dsts.push_back(currency::tx_destination_entry(dust, dust_policy.addr_for_dust));
    }
    
    if (m_is_view_only)
    {
      //mark outputs as spent 
      BOOST_FOREACH(transfer_container::iterator it, selected_transfers)
        it->m_spent = true;
      //do offline sig
      blobdata bl = t_serializable_object_to_blob(create_tx_param);
      crypto::do_chacha_crypt(bl, m_account.get_keys().m_view_secret_key);
      epee::file_io_utils::save_string_to_file("unsigned_boolberry_tx", bl);
      LOG_PRINT_L0("Transaction stored to unsigned_boolberry_tx. Take this file to offline wallet to sign it and then transfer it usign this wallet");
      return;
    }
    bool r = currency::construct_tx(m_account.get_keys(), create_tx_param, create_tx_result);
    CHECK_AND_THROW_WALLET_EX(!r, error::tx_not_constructed, sources, splitted_dsts, unlock_time);
    tx = create_tx_result.tx;
    
    relay_blob = t_serializable_object_to_blob(tx);
    finalize_transaction(create_tx_param, create_tx_result, do_not_relay);
  
  }


}
