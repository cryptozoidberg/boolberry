// Copyright (c) 2012-2018 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "currency_basic.h"
#include "common/util.h"
#include "currency_core/currency_format_utils.h"
#include "crypto/hash.h"
#include "common/db_bridge.h"


namespace currency
{

  class scratchpad_wrapper
  {
  public:
    typedef db::array_accessor_adapter_to_native<crypto::hash, false> scratchpad_container;

    scratchpad_wrapper(scratchpad_container& m_db_scratchpad);
    bool init(const std::string& config_folder);
    bool deinit();
    void clear();
    const std::vector<crypto::hash>& get_scratchpad();
    void set_scratchpad(const std::vector<crypto::hash>& sc);
    bool push_block_scratchpad_data(const block& b);
    bool pop_block_scratchpad_data(const block& b);

  private:
    std::vector<crypto::hash> m_scratchpad_cache;
    scratchpad_container& m_rdb_scratchpad;
    std::string m_config_folder;
  };
  //------------------------------------------------------------------
  template<typename pod_operand_a, typename pod_operand_b>
  crypto::hash hash_together(const pod_operand_a& a, const pod_operand_b& b)
  {
    std::string blob;
    epee::string_tools::apped_pod_to_strbuff(blob, a);
    epee::string_tools::apped_pod_to_strbuff(blob, b);
    return crypto::cn_fast_hash(blob.data(), blob.size());
  }
  //------------------------------------------------------------------
  template<class t_container>
  bool get_scratchpad_patch(size_t global_start_entry, size_t local_start_entry, size_t local_end_entry, const t_container& scratchpd, std::map<uint64_t, crypto::hash>& patch)
  {
    if (!global_start_entry)
      return true;
    for (size_t i = local_start_entry; i != local_end_entry; i++)
    {
      crypto::hash v = scratchpd[i];
      size_t rnd_upd_ind = reinterpret_cast<const uint64_t*>(&v)[0] % global_start_entry;
      patch[rnd_upd_ind] = crypto::xor_pod(patch[rnd_upd_ind], scratchpd[i]);
    }
    return true;
  }
  //------------------------------------------------------------------
  template<class t_container>
  bool get_block_scratchpad_addendum(const block& b, t_container& res)
  {
    if (get_block_height(b))
      res.push_back(b.prev_id);
    crypto::public_key tx_pub;
    bool r = parse_and_validate_tx_extra(b.miner_tx, tx_pub);
    CHECK_AND_ASSERT_MES(r, false, "wrong miner tx in put_block_scratchpad_data: no one-time tx pubkey");
    res.push_back(*reinterpret_cast<crypto::hash*>(&tx_pub));
    res.push_back(get_tx_tree_hash(b));
    for (const auto& out : b.miner_tx.vout)
    {
      CHECK_AND_ASSERT_MES(out.target.type() == typeid(txout_to_key), false, "wrong tx out type in coinbase!!!");
      /*
      tx outs possible to fill with nonrandom data, let's hash it with prev_tx to avoid nonrandom data in scratchpad
      */
      res.push_back(hash_together(b.prev_id, boost::get<txout_to_key>(out.target).key));
    }
    return true;
  }
  //---------------------------------------------------------------
  template<class t_container>
  bool push_block_scratchpad_data(size_t global_start_entry, const block& b, t_container& scratchpd, std::map<uint64_t, crypto::hash>& patch)
  {
    size_t start_offset = scratchpd.size();
    get_block_scratchpad_addendum(b, scratchpd);
    get_scratchpad_patch(global_start_entry, start_offset, scratchpd.size(), scratchpd, patch);
#ifdef ENABLE_HASHING_DEBUG
    LOG_PRINT2("patches.log", "PATCH FOR BID: " << get_block_hash(b)
      << ", scr_offset " << global_start_entry << ENDL
      << dump_patch(patch), LOG_LEVEL_3);
#endif

    return true;
  }
  //------------------------------------------------------------------
  inline void update_container_item(std::vector<crypto::hash>& cont, uint64_t i, const crypto::hash& h)
  {
    cont[i] = h;
  }
  template<class container_t>
  void update_container_item(container_t& cont, uint64_t i, const crypto::hash& h)
  {
    cont.set(i, h);
  }

  template<class t_container>
  bool apply_scratchpad_patch(t_container& scratchpd, std::map<uint64_t, crypto::hash>& patch)
  {
    for (auto& p : patch)
    {
      //scratchpd[p.first] = crypto::xor_pod(scratchpd[p.first], p.second);
      update_container_item(scratchpd, p.first, crypto::xor_pod(scratchpd[p.first], p.second));
    }
    return true;
  }
  //------------------------------------------------------------------
  template<class t_container>
  bool pop_block_scratchpad_data(const block& b, t_container& scratchpd)
  {
    std::map<uint64_t, crypto::hash> patch;
    std::vector<crypto::hash> block_scratch_addendum;
    if (!get_block_scratchpad_addendum(b, block_scratch_addendum))
      return false;

    get_scratchpad_patch(scratchpd.size() - block_scratch_addendum.size(), 0, block_scratch_addendum.size(), block_scratch_addendum, patch);
#ifdef ENABLE_HASHING_DEBUG
    LOG_PRINT2("patches.log", "PATCH FOR BID: " << get_block_hash(b)
      << ", scr_offset " << scratchpd.size() - block_scratch_addendum.size() << ENDL
      << dump_patch(patch), LOG_LEVEL_3);
#endif
    //apply patch
    apply_scratchpad_patch(scratchpd, patch);
    scratchpd.resize(scratchpd.size() - block_scratch_addendum.size());
    return true;
  }
  //------------------------------------------------------------------
  template<class t_container>
  bool push_block_scratchpad_data(const block& b, t_container& scratchpd)
  {
    std::map<uint64_t, crypto::hash> patch;
    size_t inital_sz = scratchpd.size();
    if (!push_block_scratchpad_data(scratchpd.size(), b, scratchpd, patch))
    {
      scratchpd.resize(inital_sz);
      return false;
    }
    //apply patch
    apply_scratchpad_patch(scratchpd, patch);
    return true;
  }

}

