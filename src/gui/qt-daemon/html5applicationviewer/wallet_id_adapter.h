// Copyright (c) 2012-2013 The Lui developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "wallet/wallet2.h"

class i_backend_wallet_callback
{
public:
  virtual void on_new_block(size_t wallet_id, uint64_t /*height*/, const currency::block& /*block*/) {}
  virtual void on_money_received(size_t wallet_id, uint64_t /*height*/, const currency::transaction& /*tx*/, size_t /*out_index*/) {}
  virtual void on_money_spent(size_t wallet_id, uint64_t /*height*/, const currency::transaction& /*in_tx*/, size_t /*out_index*/, const currency::transaction& /*spend_tx*/) {}
  virtual void on_transfer2(size_t wallet_id, const tools::wallet_rpc::wallet_transfer_info& wti) {}
  virtual void on_money_sent(size_t wallet_id, const tools::wallet_rpc::wallet_transfer_info& wti) {}
  virtual void on_pos_block_found(size_t wallet_id, const currency::block& /*block*/) {}
  virtual void on_sync_progress(size_t wallet_id, const uint64_t& /*percents*/) {}

};

struct i_wallet_to_i_backend_adapter: public tools::i_wallet2_callback
{
  i_wallet_to_i_backend_adapter(i_backend_wallet_callback* pbackend, size_t wallet_id) :m_pbackend(pbackend),
                                                                                        m_wallet_id(wallet_id)
  {}

  virtual void on_new_block(uint64_t height, const currency::block& block) {
    m_pbackend->on_new_block(m_wallet_id, height, block);
  }
  virtual void on_money_received(uint64_t height, const currency::transaction& tx, size_t out_index) {
    m_pbackend->on_money_received(m_wallet_id, height, tx, out_index);
  }
  virtual void on_money_spent(uint64_t height, const currency::transaction& in_tx, size_t out_index, const currency::transaction& spend_tx) {
    m_pbackend->on_money_spent(m_wallet_id, height, in_tx, out_index, spend_tx);
  }
  virtual void on_transfer2(const tools::wallet_rpc::wallet_transfer_info& wti) {
    m_pbackend->on_transfer2(m_wallet_id, wti);
  }
  virtual void on_money_sent(const tools::wallet_rpc::wallet_transfer_info& wti) {
    m_pbackend->on_money_sent(m_wallet_id, wti);
  }
  virtual void on_pos_block_found(const currency::block& wti) {
    m_pbackend->on_pos_block_found(m_wallet_id, wti);
  }
  virtual void on_sync_progress(const uint64_t& progress) {
    m_pbackend->on_sync_progress(m_wallet_id, progress);
  }
private:
  i_backend_wallet_callback* m_pbackend;
  size_t m_wallet_id;
};
