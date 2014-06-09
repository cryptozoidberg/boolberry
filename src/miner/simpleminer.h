// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once 
#include "net/http_client.h"
#include "currency_protocol/blobdatatype.h"
#include "rpc/mining_protocol_defs.h"
#include "currency_core/difficulty.h"
#include <boost/atomic.hpp>
#include <atomic>
namespace mining
{
  class  simpleminer
  {
  public:
    static void init_options(boost::program_options::options_description& desc);
    bool init(const boost::program_options::variables_map& vm);
    bool run();
  private: 

    struct height_info_native
    {
      uint64_t height;
      crypto::hash id;
    };
    
    struct job_details_native
    {
      currency::blobdata blob;
      currency::difficulty_type difficulty;
      std::string job_id;
      height_info_native prev_hi;
    };

    static bool text_job_details_to_native_job_details(const job_details& job, job_details_native& native_details);
    static bool text_height_info_to_native_height_info(const height_info& job, height_info_native& hi_native);
    static bool native_height_info_to_text_height_info(height_info& job, const height_info_native& hi_native);

    bool get_job();
    bool get_whole_scratchpad();
    bool apply_addendums(const std::list<addendum>& addms);
    bool pop_addendum(const addendum& add);
    bool push_addendum(const addendum& add);
    void worker_thread(uint64_t start_nonce, uint32_t nonce_offset, std::atomic<uint32_t> *result);

    std::vector<mining::addendum> m_blocks_addendums; //need to handle splits without re-downloading whole scratchpad
    height_info_native m_hi;
    std::vector<crypto::hash> m_scratchpad;
    uint64_t m_last_job_ticks;
    uint32_t m_threads_total;
    std::string m_pool_session_id;
    simpleminer::job_details_native m_job;

    std::string m_pool_ip;
    std::string m_pool_port;
    std::string m_login;
    std::string m_pass;
    epee::net_utils::http::http_simple_client m_http_client;
  };
}
