// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "daemon_backend.h"
#include "currency_core/alias_helper.h"


daemon_backend::daemon_backend():m_pview(&m_view_stub),
                                 m_stop_singal_sent(false),
                                 m_ccore(&m_cprotocol),
                                 m_cprotocol(m_ccore, &m_p2psrv),
                                 m_p2psrv(m_cprotocol),
                                 m_rpc_server(m_ccore, m_p2psrv),
                                 m_rpc_proxy(m_rpc_server),
                                 m_last_daemon_height(0),
                                 m_last_wallet_synch_height(0)

{
  m_wallet.reset(new tools::wallet2());
  m_wallet->callback(this);
}

const command_line::arg_descriptor<bool> arg_alloc_win_console = {"alloc-win-clonsole", "Allocates debug console with GUI", false};
const command_line::arg_descriptor<std::string> arg_html_folder = {"html-path", "Manually set GUI html folder path", "",  true};

daemon_backend::~daemon_backend()
{
  stop();
}

bool daemon_backend::start(int argc, char* argv[], view::i_view* pview_handler)
{
  m_stop_singal_sent = false;
  if(pview_handler)
    m_pview = pview_handler;

  view::daemon_status_info dsi = AUTO_VAL_INIT(dsi);
  dsi.difficulty = "---";
  dsi.text_state = "Initializing...";
  pview_handler->update_daemon_status(dsi);

  //#ifdef WIN32
  //_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
  //#endif

  log_space::get_set_log_detalisation_level(true, LOG_LEVEL_0);
  LOG_PRINT_L0("Initing...");

  TRY_ENTRY();
  po::options_description desc_cmd_only("Command line options");
  po::options_description desc_cmd_sett("Command line options and settings options");

  command_line::add_arg(desc_cmd_only, command_line::arg_help);
  command_line::add_arg(desc_cmd_only, command_line::arg_version);
  command_line::add_arg(desc_cmd_only, command_line::arg_os_version);
  // tools::get_default_data_dir() can't be called during static initialization
  command_line::add_arg(desc_cmd_only, command_line::arg_data_dir, tools::get_default_data_dir());
  command_line::add_arg(desc_cmd_only, command_line::arg_config_file);

  command_line::add_arg(desc_cmd_sett, command_line::arg_log_file);
  command_line::add_arg(desc_cmd_sett, command_line::arg_log_level);
  command_line::add_arg(desc_cmd_sett, command_line::arg_console);
  command_line::add_arg(desc_cmd_sett, command_line::arg_show_details);
  command_line::add_arg(desc_cmd_sett, arg_alloc_win_console);
  command_line::add_arg(desc_cmd_sett, arg_html_folder);
  


  currency::core::init_options(desc_cmd_sett);
  currency::core_rpc_server::init_options(desc_cmd_sett);
  nodetool::node_server<currency::t_currency_protocol_handler<currency::core> >::init_options(desc_cmd_sett);
  currency::miner::init_options(desc_cmd_sett);

  po::options_description desc_options("Allowed options");
  desc_options.add(desc_cmd_only).add(desc_cmd_sett);

  po::variables_map vm;
  bool r = command_line::handle_error_helper(desc_options, [&]()
  {
    po::store(po::parse_command_line(argc, argv, desc_options), vm);

    if (command_line::get_arg(vm, command_line::arg_help))
    {
      std::cout << CURRENCY_NAME << " v" << PROJECT_VERSION_LONG << ENDL << ENDL;
      std::cout << desc_options << std::endl;
      return false;
    }

    m_data_dir = command_line::get_arg(vm, command_line::arg_data_dir);
    std::string config = command_line::get_arg(vm, command_line::arg_config_file);

    boost::filesystem::path data_dir_path(m_data_dir);
    boost::filesystem::path config_path(config);
    if (!config_path.has_parent_path())
    {
      config_path = data_dir_path / config_path;
    }

    boost::system::error_code ec;
    if (boost::filesystem::exists(config_path, ec))
    {
      po::store(po::parse_config_file<char>(config_path.string<std::string>().c_str(), desc_cmd_sett), vm);
    }
    po::notify(vm);

    return true;
  });
  if (!r)
    return false;

  //set up logging options
  if (command_line::has_arg(vm, arg_alloc_win_console))
  {
     log_space::log_singletone::add_logger(LOGGER_CONSOLE, NULL, NULL);
  }

  std::string path_to_html;
  if (!command_line::has_arg(vm, arg_html_folder))
  {
    path_to_html = string_tools::get_current_module_folder() + "/html";
  }
  else
  {
    path_to_html = command_line::get_arg(vm, arg_html_folder);
  }

  m_pview->set_html_path(path_to_html);

  boost::filesystem::path log_file_path(command_line::get_arg(vm, command_line::arg_log_file));
  if (log_file_path.empty())
    log_file_path = log_space::log_singletone::get_default_log_file();
  std::string log_dir;
  log_dir = log_file_path.has_parent_path() ? log_file_path.parent_path().string() : log_space::log_singletone::get_default_log_folder();

  log_space::log_singletone::add_logger(LOGGER_FILE, log_file_path.filename().string().c_str(), log_dir.c_str());
  LOG_PRINT_L0(CURRENCY_NAME << " v" << PROJECT_VERSION_LONG);

  LOG_PRINT("Module folder: " << argv[0], LOG_LEVEL_0);

  bool res = true;
  currency::checkpoints checkpoints;
  res = currency::create_checkpoints(checkpoints);
  CHECK_AND_ASSERT_MES(res, false, "Failed to initialize checkpoints");
  m_ccore.set_checkpoints(std::move(checkpoints));

  m_main_worker_thread = std::thread([this, vm](){main_worker(vm);});

  return true;
  CATCH_ENTRY_L0("main", 1);
 }

bool daemon_backend::send_stop_signal()
{
  m_stop_singal_sent = true;
  return true;
}

bool daemon_backend::stop()
{
  send_stop_signal();
  if(m_main_worker_thread.joinable())
    m_main_worker_thread.join();

  return true;
}

std::string daemon_backend::get_config_folder()
{
  return m_data_dir;
}

void daemon_backend::main_worker(const po::variables_map& vm)
{

#define CHECK_AND_ASSERT_AND_SET_GUI(cond, res, mess) \
  if (!cond) \
  { \
    LOG_ERROR(mess); \
    dsi.daemon_network_state = 4; \
    dsi.text_state = mess; \
    m_pview->update_daemon_status(dsi); \
    m_pview->on_backend_stopped(); \
    return res; \
  }

  view::daemon_status_info dsi = AUTO_VAL_INIT(dsi);
  dsi.difficulty = "---";
  m_pview->update_daemon_status(dsi);

  //initialize objects
  LOG_PRINT_L0("Initializing p2p server...");
  dsi.text_state = "Initializing p2p server";
  m_pview->update_daemon_status(dsi);
  bool res = m_p2psrv.init(vm);
  CHECK_AND_ASSERT_AND_SET_GUI(res, void(), "Failed to initialize p2p server.");
  LOG_PRINT_L0("P2p server initialized OK on port: " << m_p2psrv.get_this_peer_port());

  //LOG_PRINT_L0("Starting UPnP");
  //upnp_helper.run_port_mapping_loop(p2psrv.get_this_peer_port(), p2psrv.get_this_peer_port(), 20*60*1000);

  LOG_PRINT_L0("Initializing currency protocol...");
  dsi.text_state = "Initializing currency protocol";
  m_pview->update_daemon_status(dsi);
  res = m_cprotocol.init(vm);
  CHECK_AND_ASSERT_AND_SET_GUI(res, void(), "Failed to initialize currency protocol.");
  LOG_PRINT_L0("Currency protocol initialized OK");

  LOG_PRINT_L0("Initializing core rpc server...");
  dsi.text_state = "Initializing core rpc server";
  m_pview->update_daemon_status(dsi);
  res = m_rpc_server.init(vm);
  CHECK_AND_ASSERT_AND_SET_GUI(res, void(), "Failed to initialize core rpc server.");
  LOG_PRINT_GREEN("Core rpc server initialized OK on port: " << m_rpc_server.get_binded_port(), LOG_LEVEL_0);

  //initialize core here
  LOG_PRINT_L0("Initializing core...");
  dsi.text_state = "Initializing core";
  m_pview->update_daemon_status(dsi);
  res = m_ccore.init(vm);
  CHECK_AND_ASSERT_AND_SET_GUI(res, void(), "Failed to initialize core");
  LOG_PRINT_L0("Core initialized OK");

  LOG_PRINT_L0("Starting core rpc server...");
  dsi.text_state = "Starting core rpc server";
  m_pview->update_daemon_status(dsi);
  res = m_rpc_server.run(2, false);
  CHECK_AND_ASSERT_AND_SET_GUI(res, void(), "Failed to initialize core rpc server.");
  LOG_PRINT_L0("Core rpc server started ok");

  LOG_PRINT_L0("Starting p2p net loop...");
  dsi.text_state = "Starting network loop";
  m_pview->update_daemon_status(dsi);
  res = m_p2psrv.run(false);
  CHECK_AND_ASSERT_AND_SET_GUI(res, void(), "Failed to run p2p loop.");
  LOG_PRINT_L0("p2p net loop stopped");

  //go to monitoring view loop
  loop();

  dsi.daemon_network_state = 3;

  CRITICAL_REGION_BEGIN(m_wallet_lock);
  if(m_wallet->get_wallet_path().size())
  {
    LOG_PRINT_L0("Storing wallet data...");
    dsi.text_state = "Storing wallet data...";
    m_pview->update_daemon_status(dsi);
    m_wallet->store();
  }
  CRITICAL_REGION_END();

  LOG_PRINT_L0("Stopping core p2p server...");
  dsi.text_state = "Stopping p2p network server";
  m_pview->update_daemon_status(dsi);
  m_p2psrv.send_stop_signal();
  m_p2psrv.timed_wait_server_stop(10);

  //stop components
  LOG_PRINT_L0("Stopping core rpc server...");
  dsi.text_state = "Stopping rpc network server";
  m_pview->update_daemon_status(dsi);

  m_rpc_server.send_stop_signal();
  m_rpc_server.timed_wait_server_stop(5000);

  //deinitialize components

  LOG_PRINT_L0("Deinitializing core...");
  dsi.text_state = "Deinitializing core";
  m_pview->update_daemon_status(dsi);
  m_ccore.deinit();


  LOG_PRINT_L0("Deinitializing rpc server ...");
  dsi.text_state = "Deinitializing rpc server";
  m_pview->update_daemon_status(dsi);
  m_rpc_server.deinit();


  LOG_PRINT_L0("Deinitializing currency_protocol...");
  dsi.text_state = "Deinitializing currency_protocol";
  m_pview->update_daemon_status(dsi);
  m_cprotocol.deinit();


  LOG_PRINT_L0("Deinitializing p2p...");
  dsi.text_state = "Deinitializing p2p";
  m_pview->update_daemon_status(dsi);

  m_p2psrv.deinit();

  m_ccore.set_currency_protocol(NULL);
  m_cprotocol.set_p2p_endpoint(NULL);

  LOG_PRINT("Node stopped.", LOG_LEVEL_0);
  dsi.text_state = "Node stopped";
  m_pview->update_daemon_status(dsi);

  m_pview->on_backend_stopped();
}

bool daemon_backend::update_state_info()
{
  view::daemon_status_info dsi = AUTO_VAL_INIT(dsi);
  dsi.difficulty = "---";
  currency::COMMAND_RPC_GET_INFO::response inf = AUTO_VAL_INIT(inf);
  if(!m_rpc_proxy.get_info(inf))
  {
    dsi.text_state = "get_info failed";
    m_pview->update_daemon_status(dsi);
    LOG_ERROR("Failed to call get_info");
    return false;
  }
  dsi.difficulty = std::to_string(inf.difficulty);
  dsi.hashrate = inf.current_network_hashrate_350;
  dsi.inc_connections_count = inf.incoming_connections_count;
  dsi.out_connections_count = inf.outgoing_connections_count;
  switch(inf.daemon_network_state)
  {
  case currency::COMMAND_RPC_GET_INFO::daemon_network_state_connecting:     dsi.text_state = "Connecting";break;
  case currency::COMMAND_RPC_GET_INFO::daemon_network_state_online:         dsi.text_state = "Online";break;
  case currency::COMMAND_RPC_GET_INFO::daemon_network_state_synchronizing:  dsi.text_state = "Synchronizing";break;
  default: dsi.text_state = "unknown";break;
  }
  dsi.daemon_network_state = inf.daemon_network_state;
  dsi.synchronization_start_height = inf.synchronization_start_height;
  dsi.max_net_seen_height = inf.max_net_seen_height;

  dsi.last_build_available = std::to_string(inf.mi.ver_major)
    + "." + std::to_string(inf.mi.ver_minor)
    + "." + std::to_string(inf.mi.ver_revision)
    + "." + std::to_string(inf.mi.build_no);

  if (inf.mi.mode)
  {
    dsi.last_build_displaymode = inf.mi.mode + 1;
  }
  else
  {
    if (inf.mi.build_no > PROJECT_VERSION_BUILD_NO)
      dsi.last_build_displaymode = view::ui_last_build_displaymode::ui_lb_dm_new;
    else
    {
      dsi.last_build_displaymode = view::ui_last_build_displaymode::ui_lb_dm_actual;
    }
  }


  m_last_daemon_height = dsi.height = inf.height;

  m_pview->update_daemon_status(dsi);
  return true;
}

bool daemon_backend::update_wallets()
{
  CRITICAL_REGION_LOCAL(m_wallet_lock);
  if(m_wallet->get_wallet_path().size())
  {//wallet is opened
    if(m_last_daemon_height != m_last_wallet_synch_height)
    {
      view::wallet_status_info wsi = AUTO_VAL_INIT(wsi);
      wsi.wallet_state = view::wallet_status_info::wallet_state_synchronizing;
      m_pview->update_wallet_status(wsi);
      try
      {
        m_wallet->refresh();
      }
      
      catch (const tools::error::daemon_busy& /*e*/)
      {
        LOG_PRINT_L0("Daemon busy while wallet refresh");
        return true;
      }

      catch (const std::exception& e)
      {
        LOG_PRINT_L0("Failed to refresh wallet: " << e.what());
        return false;
      }

      catch (...)
      {
        LOG_PRINT_L0("Failed to refresh wallet, unknownk exception");
        return false;
      }

      m_last_wallet_synch_height = m_ccore.get_current_blockchain_height();
      wsi.wallet_state = view::wallet_status_info::wallet_state_ready;
      m_pview->update_wallet_status(wsi);
      update_wallet_info();
    }

    // scan for unconfirmed trasactions
    try
    {
      m_wallet->scan_tx_pool();
    }

    catch (const tools::error::daemon_busy& /*e*/)
    {
      LOG_PRINT_L0("Daemon busy while wallet refresh");
      return true;
    }

    catch (const std::exception& e)
    {
      LOG_PRINT_L0("Failed to refresh wallet: " << e.what());
      return false;
    }

    catch (...)
    {
      LOG_PRINT_L0("Failed to refresh wallet, unknownk exception");
      return false;
    }
  }
  return true;
}

void daemon_backend::loop()
{
  while(!m_stop_singal_sent)
  {
    update_state_info();
    update_wallets();
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }
}

bool daemon_backend::open_wallet(const std::string& path, const std::string& password)
{
  CRITICAL_REGION_LOCAL(m_wallet_lock);
  try
  {
    if (m_wallet->get_wallet_path().size())
    {
      m_wallet->store();
      m_wallet.reset(new tools::wallet2());
      m_wallet->callback(this);
    }
    
    m_wallet->load(path, password);
  }
  catch (const std::exception& e)
  {
    m_pview->show_msg_box(std::string("Failed to load wallet: ") + e.what());
    m_wallet.reset(new tools::wallet2());
    m_wallet->callback(this);
    return false;
  }

  m_wallet->init(std::string("127.0.0.1:") + std::to_string(m_rpc_server.get_binded_port()));
  update_wallet_info();
  m_pview->show_wallet();
  load_recent_transfers();
  m_last_wallet_synch_height = 0;  
  return true;
}

bool daemon_backend::load_recent_transfers()
{
  view::transfers_array tr_hist;
  m_wallet->get_recent_transfers_history(tr_hist.history, 0, 1000);
  m_wallet->get_unconfirmed_transfers(tr_hist.unconfirmed);
  //workaround for missed fee
  for (auto & he : tr_hist.history)
    if (!he.fee && !currency::is_coinbase(he.tx)) 
       he.fee = currency::get_tx_fee(he.tx);

  for (auto & he : tr_hist.unconfirmed)
    if (!he.fee && !currency::is_coinbase(he.tx)) 
       he.fee = currency::get_tx_fee(he.tx);

  return m_pview->set_recent_transfers(tr_hist);
}

bool daemon_backend::generate_wallet(const std::string& path, const std::string& password)
{
  CRITICAL_REGION_LOCAL(m_wallet_lock);
  try
  {
    if (m_wallet->get_wallet_path().size())
    {
      m_wallet->store();
      m_wallet.reset(new tools::wallet2());
      m_wallet->callback(this);
    }

    m_wallet->generate(path, password);
  }
  catch (const std::exception& e)
  {
    m_pview->show_msg_box(std::string("Failed to generate wallet: ") + e.what());
    m_wallet.reset(new tools::wallet2());
    m_wallet->callback(this);
    return false;
  }

  m_wallet->init(std::string("127.0.0.1:") + std::to_string(m_rpc_server.get_binded_port()));
  update_wallet_info();
  m_last_wallet_synch_height = 0;
  m_pview->show_wallet();
  return true;

}

bool daemon_backend::close_wallet()
{
  CRITICAL_REGION_LOCAL(m_wallet_lock);
  try
  {
    if (m_wallet->get_wallet_path().size())
    {
      m_wallet->store();
      m_wallet.reset(new tools::wallet2());
      m_wallet->callback(this);
    }
  }

  catch (const std::exception& e)
  {
    m_pview->show_msg_box(std::string("Failed to close wallet: ") + e.what());
    return false;
  }
  m_pview->hide_wallet();
  return true;
}

bool daemon_backend::get_aliases(view::alias_set& al_set)
{
  currency::COMMAND_RPC_GET_ALL_ALIASES::response aliases = AUTO_VAL_INIT(aliases);
  if (m_rpc_proxy.get_aliases(aliases) && aliases.status == CORE_RPC_STATUS_OK)
  {
    al_set.aliases = aliases.aliases;
    return true;
  }

  return false;
}


bool daemon_backend::transfer(const view::transfer_params& tp, currency::transaction& res_tx)
{
  std::vector<currency::tx_destination_entry> dsts;
  if(!tp.destinations.size())
  {
    m_pview->show_msg_box("Internal error: empty destinations");
    return false;
  }
  uint64_t fee = 0;
  if (!currency::parse_amount(fee, tp.fee))
  {
    m_pview->show_msg_box("Failed to send transaction: wrong fee amount");
    return false;
  }

  for(auto& d: tp.destinations)
  {
    dsts.push_back(currency::tx_destination_entry());
    if (!tools::get_transfer_address(d.address, dsts.back().addr, m_rpc_proxy))
    {
      m_pview->show_msg_box("Failed to send transaction: invalid address");
      return false;
    }
    if(!currency::parse_amount(dsts.back().amount, d.amount))
    {
      m_pview->show_msg_box("Failed to send transaction: wrong amount");
      return false;
    }
  }
  //payment_id
  std::vector<uint8_t> extra;
  if(tp.payment_id.size())
  {

    crypto::hash payment_id;
    if(!currency::parse_payment_id_from_hex_str(tp.payment_id, payment_id))
    {
      m_pview->show_msg_box("Failed to send transaction: wrong payment_id");
      return false;
    }
    if(!currency::set_payment_id_to_tx_extra(extra, payment_id))
    {
      m_pview->show_msg_box("Failed to send transaction: internal error, failed to set payment id");
      return false;
    }
  }

  try
  { 
    //set transaction unlock time if it was specified by user 
    uint64_t unlock_time = 0;
    if (tp.lock_time)
      unlock_time = m_wallet->get_blockchain_current_height() + tp.lock_time;

    m_wallet->transfer(dsts, tp.mixin_count, unlock_time ? unlock_time + 1:0, fee, extra, res_tx);
    update_wallet_info();
  }
  catch (const std::exception& e)
  {
    LOG_ERROR("Transfer error: " << e.what());
    m_pview->show_msg_box(std::string("Failed to send transaction: ") + e.what());
    return false;
  }
  catch (...)
  {
    LOG_ERROR("Transfer error: unknown error");
    m_pview->show_msg_box("Failed to send transaction: unknown error");
    return false;
  }

  return true;
}

bool daemon_backend::update_wallet_info()
{
  CRITICAL_REGION_LOCAL(m_wallet_lock);
  view::wallet_info wi = AUTO_VAL_INIT(wi);
  wi.address = m_wallet->get_account().get_public_address_str();
  wi.tracking_hey = string_tools::pod_to_hex(m_wallet->get_account().get_keys().m_view_secret_key);
  wi.balance = m_wallet->balance();
  wi.unlocked_balance = m_wallet->unlocked_balance();
  wi.path = m_wallet->get_wallet_path();
  m_pview->update_wallet_info(wi);
  return true;
}

void daemon_backend::on_new_block(uint64_t /*height*/, const currency::block& /*block*/)
{

}

void daemon_backend::on_transfer2(const tools::wallet_rpc::wallet_transfer_info& wti)
{
  view::transfer_event_info tei = AUTO_VAL_INIT(tei);
  tei.ti = wti;
  tei.balance = m_wallet->balance();
  tei.unlocked_balance = m_wallet->unlocked_balance();
  m_pview->money_transfer(tei);
}

