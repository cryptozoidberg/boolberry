// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <memory>
#include <boost/serialization/list.hpp>
#include <boost/serialization/vector.hpp>
#include <atomic>

#include "include_base_utils.h"

#include "currency_core/currency_basic_impl.h"
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

#define DEFAULT_TX_SPENDABLE_AGE                               10

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
    virtual void on_money_sent(const wallet_rpc::wallet_transfer_info& wti) {}
    virtual void on_pos_block_found(const currency::block& /*block*/) {}
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
    wallet2(const wallet2&) : m_run(true), m_callback(0) {};
  public:
    wallet2() : m_run(true), m_callback(0), m_core_proxy(new default_http_core_proxy()), m_upper_transaction_size_limit(0)
    {};
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
    void load(const std::string& wallet, const std::string& password);    
    void store();
    std::string get_wallet_path(){ return m_wallet_file; }
    currency::account_base& get_account(){return m_account;}

    void get_recent_transfers_history(std::vector<wallet_rpc::wallet_transfer_info>& trs, size_t offset, size_t count);
    void get_unconfirmed_transfers(std::vector<wallet_rpc::wallet_transfer_info>& trs);
    void init(const std::string& daemon_address = "http://localhost:8080");
    bool deinit();

    void stop() { m_run.store(false, std::memory_order_relaxed); }

    i_wallet2_callback* callback() const { return m_callback; }
    void callback(i_wallet2_callback* callback) { m_callback = callback; }
    void callback(std::shared_ptr<i_wallet2_callback> callback){ m_callback_holder = callback; m_callback = m_callback_holder.get();}

    void scan_tx_pool();
    void refresh();
    void refresh(size_t & blocks_fetched);
    void refresh(size_t & blocks_fetched, bool& received_money);
    bool refresh(size_t & blocks_fetched, bool& received_money, bool& ok);
    
    void push_offer(const currency::offer_details& od);

    bool set_core_proxy(const std::shared_ptr<i_core_proxy>& proxy);
    std::shared_ptr<i_core_proxy> get_core_proxy();
    uint64_t balance();
    uint64_t unlocked_balance();
    template<typename T>
    void transfer(const std::vector<currency::tx_destination_entry>& dsts, size_t fake_outputs_count, uint64_t unlock_time, uint64_t fee, const std::vector<currency::extra_v>& extra, const std::vector<currency::attachment_v> attachments, T destination_split_strategy, const tx_dust_policy& dust_policy);
    template<typename T>
    void transfer(const std::vector<currency::tx_destination_entry>& dsts, size_t fake_outputs_count, uint64_t unlock_time, uint64_t fee, const std::vector<currency::extra_v>& extra, const std::vector<currency::attachment_v> attachments, T destination_split_strategy, const tx_dust_policy& dust_policy, currency::transaction &tx, uint8_t tx_outs_attr = CURRENCY_TO_KEY_OUT_RELAXED);
    void transfer(const std::vector<currency::tx_destination_entry>& dsts, size_t fake_outputs_count, uint64_t unlock_time, uint64_t fee, const std::vector<currency::extra_v>& extra, const std::vector<currency::attachment_v> attachments);
    void transfer(const std::vector<currency::tx_destination_entry>& dsts, size_t fake_outputs_count, uint64_t unlock_time, uint64_t fee, const std::vector<currency::extra_v>& extra, const std::vector<currency::attachment_v> attachments, currency::transaction& tx);
    bool check_connection();
    void get_transfers(wallet2::transfer_container& incoming_transfers) const;
    void get_payments(const crypto::hash& payment_id, std::list<payment_details>& payments) const;
    bool get_transfer_address(const std::string& adr_str, currency::account_public_address& addr);
    uint64_t get_blockchain_current_height() const { return m_local_bc_height; }
    template <class t_archive>
    inline void serialize(t_archive &a, const unsigned int ver)
    {
      if(ver < 9)
        return;
      a & m_blockchain;
      a & m_transfers;
      a & m_account_public_address;
      a & m_key_images;
      a & m_unconfirmed_txs;
      a & m_payments;
      a & m_transfer_history;

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
    bool scan_pos(const currency::COMMAND_RPC_SCAN_POS::request& sp, currency::COMMAND_RPC_SCAN_POS::response& rsp, std::atomic<bool>& is_stop);
    bool build_minted_block(const currency::COMMAND_RPC_SCAN_POS::request& req, const currency::COMMAND_RPC_SCAN_POS::response& rsp);

  private:
    bool store_keys(std::string& buff, const std::string& password);
    void load_keys(const std::string& keys_file_name, const std::string& password);
    void process_new_transaction(const currency::transaction& tx, uint64_t height, const currency::block& b);
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
    void prepare_wti(wallet_rpc::wallet_transfer_info& wti, uint64_t height, uint64_t timestamp, const currency::transaction& tx, uint64_t amount, const money_transfer2_details& td);
    void handle_money_received2(const currency::block& b, const currency::transaction& tx, uint64_t amount, const money_transfer2_details& td);
    void handle_money_spent2(const currency::block& b, const currency::transaction& in_tx, uint64_t amount, const money_transfer2_details& td, const std::string& recipient, const std::string& recipient_alias);
    std::string get_alias_for_address(const std::string& addr);
    void wallet_transfer_info_from_unconfirmed_transfer_details(const unconfirmed_transfer_details& utd, wallet_rpc::wallet_transfer_info& wti);
    bool is_coin_age_okay(const transfer_details& tr);
    bool build_kernel(const currency::pos_entry& pe, const currency::stake_modifier_type& stake_modifier, currency::stake_kernel& kernel, uint64_t& coindays_weight, uint64_t timestamp);
    bool is_connected_to_net();


    currency::account_base m_account;
    std::string m_wallet_file;
    std::string m_password;
    std::vector<crypto::hash> m_blockchain;
    std::atomic<uint64_t> m_local_bc_height; //temporary workaround 
    std::atomic<uint64_t> m_last_bc_timestamp; 
    std::unordered_map<crypto::hash, unconfirmed_transfer_details> m_unconfirmed_txs;

    transfer_container m_transfers;
    payment_container m_payments;
    std::unordered_map<crypto::key_image, size_t> m_key_images;
    currency::account_public_address m_account_public_address;
    uint64_t m_upper_transaction_size_limit; //TODO: auto-calc this value or request from daemon, now use some fixed value

    std::atomic<bool> m_run;
    std::vector<wallet_rpc::wallet_transfer_info> m_transfer_history;
    std::unordered_map<crypto::hash, currency::transaction> m_unconfirmed_in_transfers;
    std::shared_ptr<i_core_proxy> m_core_proxy;
    std::shared_ptr<i_wallet2_callback> m_callback_holder;
    i_wallet2_callback* m_callback;
  };
}


BOOST_CLASS_VERSION(tools::wallet2, 9)
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
      a & x.m_block_timestamp;
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
      a & x.recipient; 
      a & x.is_income;
      a & x.td;
      a & x.tx;
      if (ver < 2)
        return;
      a & x.recipient_alias;
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
    uint64_t unlock_time, uint64_t fee, const std::vector<currency::extra_v>& extra, const std::vector<currency::attachment_v> attachments, T destination_split_strategy, const tx_dust_policy& dust_policy)
  {
    currency::transaction tx;
    transfer(dsts, fake_outputs_count, unlock_time, fee, extra, attachments, destination_split_strategy, dust_policy, tx);
  }

  template<typename T>
  void wallet2::transfer(const std::vector<currency::tx_destination_entry>& dsts, size_t fake_outputs_count,
    uint64_t unlock_time, uint64_t fee, const std::vector<currency::extra_v>& extra, const std::vector<currency::attachment_v> attachments, T destination_split_strategy, const tx_dust_policy& dust_policy, currency::transaction &tx, uint8_t tx_outs_attr)
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


    bool r = currency::construct_tx(m_account.get_keys(), sources, splitted_dsts, extra, attachments, tx, unlock_time);
    CHECK_AND_THROW_WALLET_EX(!r, error::tx_not_constructed, sources, splitted_dsts, unlock_time);
    //update_current_tx_limit();
    CHECK_AND_THROW_WALLET_EX(CURRENCY_MAX_TRANSACTION_BLOB_SIZE <= get_object_blobsize(tx), error::tx_too_big, tx, m_upper_transaction_size_limit);

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
    add_sent_unconfirmed_tx(tx, change_dts.amount, recipient);

    LOG_PRINT_L2("transaction " << get_transaction_hash(tx) << " generated ok and sent to daemon, key_images: [" << key_images << "]");

    BOOST_FOREACH(transfer_container::iterator it, selected_transfers)
      it->m_spent = true;

    LOG_PRINT_L0("Transaction successfully sent. <" << get_transaction_hash(tx) << ">" << ENDL 
                  << "Commission: " << print_money(fee+dust) << " (dust: " << print_money(dust) << ")" << ENDL
                  << "Balance: " << print_money(balance()) << ENDL
                  << "Unlocked: " << print_money(unlocked_balance()) << ENDL
                  << "Please, wait for confirmation for your balance to be unlocked.");
  }
}
