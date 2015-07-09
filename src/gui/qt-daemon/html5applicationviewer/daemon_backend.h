// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <boost/program_options.hpp>
#include "warnings.h"
PUSH_WARNINGS
DISABLE_VS_WARNINGS(4100)
DISABLE_VS_WARNINGS(4503)
#include "include_base_utils.h"
#include "version.h"

using namespace epee;

#include "console_handler.h"
#include "p2p/net_node.h"
#include "currency_core/checkpoints_create.h"
#include "currency_core/currency_core.h"
#include "rpc/core_rpc_server.h"
#include "currency_protocol/currency_protocol_handler.h"
#include "daemon/daemon_commands_handler.h"
//#include "common/miniupnp_helper.h"
#include "view_iface.h"
#include "core_fast_rpc_proxy.h"
#include "wallet/wallet2.h"
#include "wallet_id_adapter.h"
POP_WARNINGS

namespace po = boost::program_options;

#if defined(WIN32)
#include <crtdbg.h>
#endif

//TODO: need refactoring here. (template classes can't be used in BOOST_CLASS_VERSION)
BOOST_CLASS_VERSION(nodetool::node_server<currency::t_currency_protocol_handler<currency::core> >, CURRENT_P2P_STORAGE_ARCHIVE_VER);


class daemon_backend : public i_backend_wallet_callback
{

public:
  struct wallet_vs_options
  {
    epee::locked_object<std::shared_ptr<tools::wallet2>> w;
    std::atomic<bool> do_mining;
    std::atomic<bool> stop;
    std::atomic<bool> break_mining_loop;
    std::atomic<uint64_t> last_wallet_synch_height;
    std::atomic<uint64_t>* plast_daemon_height;
    view::i_view* pview;
    uint64_t wallet_id;

    std::thread miner_thread;
    void worker_func();
    ~wallet_vs_options();
  };

  daemon_backend();
  ~daemon_backend();
  bool start(int argc, char* argv[], view::i_view* pview_handler);
  bool stop();
  bool send_stop_signal();
  std::string open_wallet(const std::string& path, const std::string& password, view::open_wallet_response& owr);
  std::string generate_wallet(const std::string& path, const std::string& password, view::open_wallet_response& owr);
  std::string restore_wallet(const std::string& path, const std::string& password, const std::string& restore_key, view::open_wallet_response& owr);
  std::string run_wallet(uint64_t wallet_id);
  std::string get_recent_transfers(size_t wallet_id, view::transfers_array& tr_hist);
  std::string get_wallet_info(size_t wallet_id, view::wallet_info& wi);
  std::string get_wallet_info(wallet_vs_options& w, view::wallet_info& wi);
  std::string close_wallet(size_t wallet_id);
  std::string push_offer(size_t wallet_id, const currency::offer_details_ex& od, currency::transaction& res_tx);
  std::string cancel_offer(const view::cancel_offer_param& co, currency::transaction& res_tx);
  std::string push_update_offer(const view::update_offer_param& uo, currency::transaction& res_tx);
  std::string get_all_offers(currency::COMMAND_RPC_GET_ALL_OFFERS::response& od);
  std::string get_aliases(view::alias_set& al_set);
  std::string request_alias_registration(const currency::alias_rpc_details& al, uint64_t wallet_id, uint64_t fee, currency::transaction& res_tx);
  std::string validate_address(const std::string& addr);
  std::string resync_wallet(uint64_t wallet_id);
  std::string start_pos_mining(uint64_t wallet_id);
  std::string stop_pos_mining(uint64_t wallet_id);
  std::string get_mining_history(uint64_t wallet_id, tools::wallet_rpc::mining_history& wrpc);
  std::string get_wallet_restore_info(uint64_t wallet_id, std::string& restore_key);
  std::string backup_wallet(uint64_t wallet_id, std::string& path);
  std::string reset_wallet_password(uint64_t wallet_id, const std::string& pass);
  std::string get_mining_estimate(uint64_t amuont_coins, 
    uint64_t time, 
    uint64_t& estimate_result, 
    uint64_t& all_coins_and_pos_diff_rate, 
    std::vector<uint64_t>& days);
  std::string is_pos_allowed();
  void toggle_pos_mining();
  std::string transfer(size_t wallet_id, const view::transfer_params& tp, currency::transaction& res_tx);
  std::string get_config_folder();

private:
  void main_worker(const po::variables_map& vm);
  bool update_state_info();
  bool update_wallets();
  void loop();
  bool get_transfer_address(const std::string& adr_str, currency::account_public_address& addr);
  bool get_last_blocks(view::daemon_status_info& dsi);
  void update_wallets_info();
  bool alias_rpc_details_to_alias_info(const currency::alias_rpc_details& ard, currency::alias_info& ai);
  void init_wallet_entry(wallet_vs_options& wo, uint64_t id);
  static void prepare_wallet_status_info(wallet_vs_options& wo, view::wallet_status_info& wsi);
  //----- i_backend_wallet_callback ------
  virtual void on_new_block(size_t wallet_id, uint64_t height, const currency::block& block);
  virtual void on_transfer2(size_t wallet_id, const tools::wallet_rpc::wallet_transfer_info& wti);
  virtual void on_pos_block_found(size_t wallet_id, const currency::block& /*block*/);
  virtual void on_sync_progress(size_t wallet_id, const uint64_t& /*percents*/);


  std::thread m_main_worker_thread;
  std::atomic<bool> m_stop_singal_sent;
  view::i_view m_view_stub;
  view::i_view* m_pview;
  std::shared_ptr<tools::i_core_proxy> m_rpc_proxy;
  critical_section m_wallets_lock;


  std::map<size_t, wallet_vs_options> m_wallets;
  std::atomic<uint64_t> m_last_daemon_height;
//  std::atomic<uint64_t> m_last_wallet_synch_height;
  std::atomic<uint64_t> m_wallet_id_counter;

  std::string m_data_dir;

  //daemon stuff
  currency::core m_ccore;
  currency::t_currency_protocol_handler<currency::core> m_cprotocol;
  nodetool::node_server<currency::t_currency_protocol_handler<currency::core> > m_p2psrv;
  currency::core_rpc_server m_rpc_server;

};
