// Copyright (c) 2012-2013 The Cryptonote developers
// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once 

#include <boost/atomic.hpp>
#include <boost/program_options.hpp>
#include <atomic>
#include "currency_basic.h"
#include "difficulty.h"
#include "math_helper.h"
#include "blockchain_storage.h"

namespace currency
{

  struct i_miner_handler
  {
    virtual bool handle_block_found(block& b) = 0;
    virtual bool get_block_template(block& b, const account_public_address& adr, wide_difficulty_type& diffic, uint64_t& height, const blobdata& ex_nonce, bool vote_for_donation, const alias_info& ai) = 0;
  protected:
    ~i_miner_handler(){};
  };

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  class miner
  {
  public: 
    miner(i_miner_handler* phandler, blockchain_storage& bc);
    ~miner();
    bool init(const boost::program_options::variables_map& vm);
    bool deinit();
    static void init_options(boost::program_options::options_description& desc);
    bool on_block_chain_update();
    bool start(const account_public_address& adr, size_t threads_count);
    uint64_t get_speed();
    void send_stop_signal();
    bool stop();
    bool is_mining();
    bool on_idle();
    void on_synchronized();
    void set_do_donations(bool do_donations);
    //synchronous analog (for fast calls)
    void pause();
    void resume();
    void do_print_hashrate(bool do_hr);
    bool set_alias_info(const alias_info& ai);

    template<typename callback_t>
    static bool find_nonce_for_given_block(block& bl, const wide_difficulty_type& diffic, uint64_t height, callback_t scratch_accessor)
    {
      blobdata bd = get_block_hashing_blob(bl);
      for(; bl.nonce != std::numeric_limits<uint32_t>::max(); bl.nonce++)
      {
        crypto::hash h;
        *reinterpret_cast<uint64_t*>(&bd[1]) = bl.nonce;
        get_blob_longhash(bd, h, height, scratch_accessor);

        if(check_hash(h, diffic))
        {
          LOG_PRINT_L0("Found nonce for block: " << get_block_hash(bl) << "[" << height << "]: PoW:" << h << "(diff:" << diffic << ")");
          return true;
        }
      }
      return false;
    }

  private:
    bool set_block_template(const block& bl, const wide_difficulty_type& diffic, uint64_t height);
    bool worker_thread();
    bool request_block_template();
    void  merge_hr();
    bool validate_alias_info();
    bool update_scratchpad();
    
    struct miner_config
    {
      uint64_t current_extra_message_index;
      bool donation_decision_made;
      bool donation_decision;
      //TODO: add alias que here

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(current_extra_message_index)
        KV_SERIALIZE(donation_decision_made)
        KV_SERIALIZE_N(donation_decision, "donation_descision")//  misprint fix
        KV_SERIALIZE(donation_decision)
      END_KV_SERIALIZE_MAP()
    };


    volatile uint32_t m_stop;
    ::critical_section m_template_lock;
    block m_template;
    std::atomic<uint32_t> m_template_no;
    std::atomic<uint32_t> m_starter_nonce;
    wide_difficulty_type m_diffic;
    uint64_t m_height;
    volatile uint32_t m_thread_index; 
    volatile uint32_t m_threads_total;
    std::atomic<int32_t> m_pausers_count;
    ::critical_section m_miners_count_lock;

    std::list<boost::thread> m_threads;
    ::critical_section m_threads_lock;
    i_miner_handler* m_phandler;
    blockchain_storage& m_bc;
    account_public_address m_mine_address;
    math_helper::once_a_time_seconds<5> m_update_block_template_interval;
    math_helper::once_a_time_seconds<2> m_update_merge_hr_interval;
    math_helper::once_a_time_seconds<30, false> m_update_scratchpad_interval;//hotfix for network stopped to finding blocks
    std::vector<blobdata> m_extra_messages;
    miner_config m_config;
    std::string m_config_folder;    
    std::atomic<uint64_t> m_current_hash_rate;
    std::atomic<uint64_t> m_last_hr_merge_time;
    std::atomic<uint64_t> m_hashes;
    bool m_do_print_hashrate;
    bool m_do_mining;
    alias_info m_alias_to_apply_in_block;
    critical_section m_aliace_to_apply_in_block_lock;
    
    boost::shared_mutex m_scratchpad_access;
    std::vector<crypto::hash> m_scratchpad;
  };
}



