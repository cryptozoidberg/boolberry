// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "daemon_backend.h"
#include "currency_core/alias_helper.h"
#include "core_fast_rpc_proxy.h"

#define GET_WALLET_OPT_BY_ID(wallet_id, name)       \
  CRITICAL_REGION_LOCAL(m_wallets_lock);    \
  auto it = m_wallets.find(wallet_id);      \
if (it == m_wallets.end())                \
  return API_RETURN_CODE_WALLET_WRONG_ID; \
  auto& name = it->second;

#define GET_WALLET_BY_ID(wallet_id, name)       \
CRITICAL_REGION_LOCAL(m_wallets_lock);    \
auto it = m_wallets.find(wallet_id);      \
if (it == m_wallets.end())                \
  return API_RETURN_CODE_WALLET_WRONG_ID; \
auto& name = it->second.w;



daemon_backend::daemon_backend():m_pview(&m_view_stub),
                                 m_stop_singal_sent(false),
                                 m_ccore(&m_cprotocol),
                                 m_cprotocol(m_ccore, &m_p2psrv),
                                 m_p2psrv(m_cprotocol),
                                 m_rpc_server(m_ccore, m_p2psrv),
                                 m_rpc_proxy(new tools::core_fast_rpc_proxy(m_rpc_server)),
                                 m_last_daemon_height(0),
                                 m_wallet_id_counter(0)
{}

const command_line::arg_descriptor<bool> arg_alloc_win_console = {"alloc-win-console", "Allocates debug console with GUI", false};
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
  dsi.pos_difficulty = dsi.pow_difficulty = "---";
  pview_handler->update_daemon_status(dsi);

  log_space::get_set_log_detalisation_level(true, LOG_LEVEL_2);
//#if !defined(NDEBUG)
//  log_space::log_singletone::add_logger(LOGGER_DEBUGGER, nullptr, nullptr);
//#endif
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
  if (command_line::has_arg(vm, command_line::arg_log_level))
  {
    log_space::log_singletone::get_set_log_detalisation_level(true, command_line::get_arg(vm, command_line::arg_log_level));
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

  if (log_file_path.has_parent_path())
  {
    log_dir = log_file_path.parent_path().string();
  }else
  {
#if defined(MACOSX)
    //osx firewall issue
    log_dir = string_tools::get_user_home_dir();
    log_dir += "Library/" CURRENCY_NAME;
    boost::system::error_code ec;
    boost::filesystem::create_directories(log_dir, ec);
    if (!boost::filesystem::is_directory(log_dir, ec))
    {
      log_dir = log_space::log_singletone::get_default_log_folder();
    }
#else
    //load process name
    log_dir = log_space::log_singletone::get_default_log_folder();
#endif      
  }



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
    dsi.daemon_network_state = currency::COMMAND_RPC_GET_INFO::daemon_network_state_internal_error; \
    m_pview->update_daemon_status(dsi); \
    m_pview->on_backend_stopped(); \
    return res; \
  }

  view::daemon_status_info dsi = AUTO_VAL_INIT(dsi);
  dsi.pos_difficulty = dsi.pos_difficulty = "---";
  dsi.daemon_network_state = currency::COMMAND_RPC_GET_INFO::daemon_network_state_loading_core;
  m_pview->update_daemon_status(dsi);

  //initialize objects
  LOG_PRINT_L0("Initializing p2p server...");
  m_pview->update_daemon_status(dsi);
  bool res = m_p2psrv.init(vm);
  CHECK_AND_ASSERT_AND_SET_GUI(res, void(), "Failed to initialize p2p server.");
  LOG_PRINT_L0("P2p server initialized OK on port: " << m_p2psrv.get_this_peer_port());

  //LOG_PRINT_L0("Starting UPnP");
  //upnp_helper.run_port_mapping_loop(p2psrv.get_this_peer_port(), p2psrv.get_this_peer_port(), 20*60*1000);

  LOG_PRINT_L0("Initializing currency protocol...");
  m_pview->update_daemon_status(dsi);
  res = m_cprotocol.init(vm);
  CHECK_AND_ASSERT_AND_SET_GUI(res, void(), "Failed to initialize currency protocol.");
  LOG_PRINT_L0("Currency protocol initialized OK");

  LOG_PRINT_L0("Initializing core rpc server...");
  //dsi.text_state = "Initializing core rpc server";
  m_pview->update_daemon_status(dsi);
  res = m_rpc_server.init(vm);
  CHECK_AND_ASSERT_AND_SET_GUI(res, void(), "Failed to initialize core rpc server.");
  LOG_PRINT_GREEN("Core rpc server initialized OK on port: " << m_rpc_server.get_binded_port(), LOG_LEVEL_0);

  //initialize core here
  LOG_PRINT_L0("Initializing core...");
  //dsi.text_state = "Initializing core";
  m_pview->update_daemon_status(dsi);
  res = m_ccore.init(vm);
  CHECK_AND_ASSERT_AND_SET_GUI(res, void(), "Failed to initialize core");
  LOG_PRINT_L0("Core initialized OK");

  LOG_PRINT_L0("Starting core rpc server...");
  //dsi.text_state = "Starting core rpc server";
  m_pview->update_daemon_status(dsi);
  res = m_rpc_server.run(2, false);
  CHECK_AND_ASSERT_AND_SET_GUI(res, void(), "Failed to initialize core rpc server.");
  LOG_PRINT_L0("Core rpc server started ok");

  LOG_PRINT_L0("Starting p2p net loop...");
  //dsi.text_state = "Starting network loop";
  m_pview->update_daemon_status(dsi);
  res = m_p2psrv.run(false);
  CHECK_AND_ASSERT_AND_SET_GUI(res, void(), "Failed to run p2p loop.");
  LOG_PRINT_L0("p2p net loop stopped");

  //go to monitoring view loop
  loop();

  dsi.daemon_network_state = currency::COMMAND_RPC_GET_INFO::daemon_network_state_unloading_core;
  

  CRITICAL_REGION_BEGIN(m_wallets_lock);
  for (auto& wo : m_wallets)
  {
    LOG_PRINT_L0("Storing wallet data...");
    //dsi.text_state = "Storing wallets data...";
    m_pview->update_daemon_status(dsi);
    wo.second.w->get()->store();
  }
  CRITICAL_REGION_END();

  LOG_PRINT_L0("Stopping core p2p server...");
  //dsi.text_state = "Stopping p2p network server";
  m_pview->update_daemon_status(dsi);
  m_p2psrv.send_stop_signal();
  m_p2psrv.timed_wait_server_stop(10);

  //stop components
  LOG_PRINT_L0("Stopping core rpc server...");
  //dsi.text_state = "Stopping rpc network server";
  m_pview->update_daemon_status(dsi);

  m_rpc_server.send_stop_signal();
  m_rpc_server.timed_wait_server_stop(5000);

  //deinitialize components

  LOG_PRINT_L0("Deinitializing core...");
  //dsi.text_state = "Deinitializing core";
  m_pview->update_daemon_status(dsi);
  m_ccore.deinit();


  LOG_PRINT_L0("Deinitializing rpc server ...");
  //dsi.text_state = "Deinitializing rpc server";
  m_pview->update_daemon_status(dsi);
  m_rpc_server.deinit();


  LOG_PRINT_L0("Deinitializing currency_protocol...");
  //dsi.text_state = "Deinitializing currency_protocol";
  m_pview->update_daemon_status(dsi);
  m_cprotocol.deinit();


  LOG_PRINT_L0("Deinitializing p2p...");
  //dsi.text_state = "Deinitializing p2p";
  m_pview->update_daemon_status(dsi);

  m_p2psrv.deinit();

  m_ccore.set_currency_protocol(NULL);
  m_cprotocol.set_p2p_endpoint(NULL);

  LOG_PRINT("Node stopped.", LOG_LEVEL_0);
  //dsi.text_state = "Node stopped";
  m_pview->update_daemon_status(dsi);

  m_pview->on_backend_stopped();
}

bool daemon_backend::update_state_info()
{
  view::daemon_status_info dsi = AUTO_VAL_INIT(dsi);
  dsi.pos_difficulty = dsi.pow_difficulty = "---";
  currency::COMMAND_RPC_GET_INFO::request req = AUTO_VAL_INIT(req);
  currency::COMMAND_RPC_GET_INFO::response inf = AUTO_VAL_INIT(inf);
  if (!m_rpc_proxy->call_COMMAND_RPC_GET_INFO(req, inf))
  {
    //dsi.text_state = "get_info failed";
    m_pview->update_daemon_status(dsi);
    LOG_ERROR("Failed to call get_info");
    return false;
  }
  dsi.alias_count = inf.alias_count;
  dsi.pow_difficulty = std::to_string(inf.pow_difficulty);
  dsi.pos_difficulty = inf.pos_difficulty;
  dsi.hashrate = inf.current_network_hashrate_350;
  dsi.inc_connections_count = inf.incoming_connections_count;
  dsi.out_connections_count = inf.outgoing_connections_count;
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
  
  get_last_blocks(dsi);

  m_last_daemon_height = dsi.height = inf.height;
  m_pview->update_daemon_status(dsi);
  return true;
}

void daemon_backend::update_wallets_info()
{
  CRITICAL_REGION_LOCAL(m_wallets_lock);
  view::wallets_summary_info wsi;
  for (auto& w : m_wallets)
  {
    wsi.wallets.push_back(view::wallet_entry_info());
    view::wallet_entry_info& l = wsi.wallets.back();
    get_wallet_info(w.second, l.wi);
    l.wallet_id = w.first;
  }
  m_pview->update_wallets_info(wsi);
}

bool daemon_backend::get_last_blocks(view::daemon_status_info& dsi)
{
  //dsi.last_blocks
  currency::COMMAND_RPC_GET_BLOCKS_DETAILS::request req = AUTO_VAL_INIT(req);
  req.height_start = m_last_daemon_height >= GUI_BLOCKS_DISPLAY_COUNT ? m_last_daemon_height - GUI_BLOCKS_DISPLAY_COUNT : 0;
  req.count = GUI_BLOCKS_DISPLAY_COUNT;
  currency::COMMAND_RPC_GET_BLOCKS_DETAILS::response rsp = AUTO_VAL_INIT(rsp);
  m_rpc_proxy->call_COMMAND_RPC_GET_BLOCKS_DETAILS(req, rsp);

  for (auto it = rsp.blocks.rbegin(); it != rsp.blocks.rend(); it++)
  {
    dsi.last_blocks.push_back(view::block_info());
    view::block_info& bi = dsi.last_blocks.back();
    currency::block b = AUTO_VAL_INIT(b);
    bool r = currency::parse_and_validate_block_from_blob(it->b, b);
    CHECK_AND_ASSERT_MES(r, false, "Failed to parse_and_validate_block_from_blob");
    bi.date = b.timestamp;
    bi.diff = it->diff;
    bi.h = it->h;
    if (is_pos_block(b))
      bi.type = "PoS";
    else
      bi.type = "PoW";
  }
  return true;
}



void daemon_backend::toggle_pos_mining()
{
  //m_do_mint = !m_do_mint;
  //update_wallets_info();
}

void daemon_backend::loop()
{
  while(!m_stop_singal_sent)
  {
    update_state_info();
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }
}

void daemon_backend::init_wallet_entry(wallet_vs_options& wo, uint64_t id)
{
  wo.wallet_id = id;
  wo.do_mining = false;
  wo.stop = false;
  wo.plast_daemon_height = &m_last_daemon_height;
  wo.pview = m_pview;
  wo.miner_thread = std::thread(boost::bind(&daemon_backend::wallet_vs_options::worker_func, &wo));
  
}

std::string daemon_backend::open_wallet(const std::string& path, const std::string& password, view::open_wallet_response& owr)
{
  std::shared_ptr<tools::wallet2> w(new tools::wallet2());
  owr.wallet_id = m_wallet_id_counter++;

  w->callback(std::shared_ptr<tools::i_wallet2_callback>(new i_wallet_to_i_backend_adapter(this, wallet_id)));
  w->set_core_proxy(std::shared_ptr<tools::i_core_proxy>(new tools::core_fast_rpc_proxy(m_rpc_server)));
  std::string return_code = API_RETURN_CODE_OK;
  CRITICAL_REGION_LOCAL(m_wallets_lock);
  while (true)
  {
    try
    {
      w->load(path, password);  
      w->get_recent_transfers_history(owr.recent_history.history, 0, 1000);
      w->get_unconfirmed_transfers(owr.recent_history.unconfirmed);
      //workaround for missed fee
      for (auto & he : owr.recent_history.history)
      if (!he.fee && !currency::is_coinbase(he.tx))
        he.fee = currency::get_tx_fee(he.tx);

      for (auto & he : owr.recent_history.unconfirmed)
      if (!he.fee && !currency::is_coinbase(he.tx))
        he.fee = currency::get_tx_fee(he.tx);

      break;
    }
    catch (const tools::error::file_not_found& /**/)
    {
      return API_RETURN_CODE_FILE_NOT_FOUND;
    }
    catch (const tools::error::wallet_load_notice_wallet_restored& /**/)
    {
      return_code = API_RETURN_CODE_FILE_RESTORED;
      break;
    }
    catch (const std::exception& e)
    {
      return std::string(API_RETURN_CODE_WRONG_PASSWORD) + ":" + e.what();
    }
  }

  wallet_vs_options& wo = m_wallets[owr.wallet_id];
  **wo.w = w;
  init_wallet_entry(wo, owr.wallet_id);

  update_wallets_info();
  return return_code;
}
/*
std::string daemon_backend::get_recent_transfers(size_t wallet_id, view::transfers_array& tr_hist)
{
  GET_WALLET_BY_ID(wallet_id, w);
  auto wallet_locked = w.try_lock();
  if (!wallet_locked.get())
  {
    return API_RETURN_CODE_CORE_BUSY;
  }

  w->get()->get_recent_transfers_history(tr_hist.history, 0, 1000);
  w->get()->get_unconfirmed_transfers(tr_hist.unconfirmed);
  //workaround for missed fee
  for (auto & he : tr_hist.history)
    if (!he.fee && !currency::is_coinbase(he.tx)) 
       he.fee = currency::get_tx_fee(he.tx);

  for (auto & he : tr_hist.unconfirmed)
    if (!he.fee && !currency::is_coinbase(he.tx)) 
       he.fee = currency::get_tx_fee(he.tx);

  return API_RETURN_CODE_OK;
}
*/
std::string daemon_backend::generate_wallet(const std::string& path, const std::string& password, uint64_t& wallet_id)
{
  std::shared_ptr<tools::wallet2> w(new tools::wallet2());
  wallet_id = m_wallet_id_counter++;
  w->callback(std::shared_ptr<tools::i_wallet2_callback>(new i_wallet_to_i_backend_adapter(this, wallet_id)));
  w->set_core_proxy(std::shared_ptr<tools::i_core_proxy>(new tools::core_fast_rpc_proxy(m_rpc_server)));

  CRITICAL_REGION_LOCAL(m_wallets_lock);

  try
  {
    w->generate(path, password);
  }
  catch (const tools::error::file_exists/*& e*/)
  {
    return API_RETURN_CODE_ALREADY_EXISTS;
  }

  catch (const std::exception& e)
  {
    return std::string(API_RETURN_CODE_FAIL) + ":" + e.what();
  }
  wallet_vs_options& wo = m_wallets[wallet_id];
  **wo.w = w;
  init_wallet_entry(wo, wallet_id);
  //update_wallets_info();
  return API_RETURN_CODE_OK;
}

std::string daemon_backend::close_wallet(size_t wallet_id)
{
  CRITICAL_REGION_LOCAL(m_wallets_lock);
  
  auto it = m_wallets.find(wallet_id);
  if (it == m_wallets.end() || it->second.w->get() == nullptr)
    return API_RETURN_CODE_WALLET_WRONG_ID;


  try
  {
    it->second.w->get()->store();
    m_wallets.erase(it);
  }

  catch (const std::exception& e)
  {
    return std::string(API_RETURN_CODE_FAIL) + ":" + e.what();
  }
  //m_pview->hide_wallet();
  return API_RETURN_CODE_OK;
}

std::string daemon_backend::get_aliases(view::alias_set& al_set)
{
  currency::COMMAND_RPC_GET_ALL_ALIASES::response aliases = AUTO_VAL_INIT(aliases);
  if (m_rpc_proxy->call_COMMAND_RPC_GET_ALL_ALIASES(aliases) && aliases.status == CORE_RPC_STATUS_OK)
  {
    al_set.aliases = aliases.aliases;
    return API_RETURN_CODE_OK;
  }

  return API_RETURN_CODE_FAIL;
}

bool daemon_backend::alias_rpc_details_to_alias_info(const currency::alias_rpc_details& ard, currency::alias_info& ai)
{
  if (!currency::get_account_address_from_str(ai.m_address, ard.details.address))
  {
    LOG_ERROR("Failed to parse address: " << ard.details.address);
    return false;
  }
  if (ard.details.tracking_key.size())
  {
    if (!string_tools::parse_tpod_from_hex_string(ard.details.tracking_key, ai.m_view_key))
    {
      LOG_ERROR("Failed to parse tracking_key: " << ard.details.tracking_key );
      return false;
    }
  }
  //ai.m_sign; atm alias updating disabled
  ai.m_text_comment = ard.details.comment;
  ai.m_alias = ard.alias;
  return true;
}

std::string daemon_backend::request_alias_registration(const currency::alias_rpc_details& al, uint64_t wallet_id, currency::transaction& res_tx)
{
  currency::alias_info ai = AUTO_VAL_INIT(ai);
  if (!alias_rpc_details_to_alias_info(al, ai))
    return API_RETURN_CODE_BAD_ARG;

  if (!currency::validate_alias_name(ai.m_alias))
  {
    return API_RETURN_CODE_BAD_ARG;
  }

  currency::COMMAND_RPC_GET_ALIAS_DETAILS::request req;
  currency::COMMAND_RPC_GET_ALIAS_DETAILS::response rsp = AUTO_VAL_INIT(rsp);
  if (m_rpc_proxy->call_COMMAND_RPC_GET_ALIAS_DETAILS(req, rsp) && rsp.status != CORE_RPC_STATUS_OK)
  {
    GET_WALLET_BY_ID(wallet_id, w);
    try
    {
      w->get()->request_alias_registration(ai, res_tx);
      return API_RETURN_CODE_OK;
    }
    catch (const std::exception& e)
    {
      LOG_ERROR("request_alias_registration error: " << e.what());
      std::string err_code = API_RETURN_CODE_INTERNAL_ERROR;
      err_code += std::string(":") + e.what();
      return err_code;
    }
    catch (...)
    {
      LOG_ERROR("request_alias_registration error: unknown error");
      return API_RETURN_CODE_INTERNAL_ERROR;
    }
  }

  return API_RETURN_CODE_ALREADY_EXISTS;
}

std::string daemon_backend::transfer(size_t wallet_id, const view::transfer_params& tp, currency::transaction& res_tx)
{

  std::vector<currency::tx_destination_entry> dsts;
  if(!tp.destinations.size())
    return API_RETURN_CODE_BAD_ARG_EMPTY_DESTINATIONS;

  uint64_t fee = 0;
  if (!currency::parse_amount(fee, tp.fee))
    return API_RETURN_CODE_BAD_ARG_WRONG_FEE;

  for(auto& d: tp.destinations)
  {
    dsts.push_back(currency::tx_destination_entry());
    if (!tools::get_transfer_address(d.address, dsts.back().addr, m_rpc_proxy.get()))
    {
      return API_RETURN_CODE_BAD_ARG_INVALID_ADDRESS;
    }
    if(!currency::parse_amount(dsts.back().amount, d.amount))
    {
      return API_RETURN_CODE_BAD_ARG_WRONG_AMOUNT;
    }
  }
  //payment_id
  std::vector<currency::extra_v> extra;
  if(tp.payment_id.size())
  {

    crypto::hash payment_id;
    if(!currency::parse_payment_id_from_hex_str(tp.payment_id, payment_id))
    {      
      return API_RETURN_CODE_BAD_ARG_WRONG_PAYMENT_ID;
    }
    if(!currency::set_payment_id_to_tx_extra(extra, payment_id))
    {
      return API_RETURN_CODE_INTERNAL_ERROR;
    }
  }

  GET_WALLET_BY_ID(wallet_id, w);



  try
  { 
    //set transaction unlock time if it was specified by user 
    uint64_t unlock_time = 0;
    if (tp.lock_time)
    {
      if (tp.lock_time > CURRENCY_MAX_BLOCK_NUMBER)
        unlock_time = tp.lock_time;
      else
        unlock_time = w->get()->get_blockchain_current_height() + tp.lock_time;
    }
      
    
    //proces attachments
    std::vector<currency::attachment_v> attachments;
    if (tp.comment.size())
    {
      currency::tx_comment tc;
      tc.comment = tp.comment;
      attachments.push_back(tc);
    }
    if (tp.push_payer)
    {
      currency::tx_payer txp;
      txp.acc_addr = w->get()->get_account().get_keys().m_account_address;
      attachments.push_back(txp);
    }
    
    w->get()->transfer(dsts, tp.mixin_count, unlock_time ? unlock_time + 1 : 0, fee, extra, attachments, res_tx);
    update_wallets_info();
  }
  catch (const std::exception& e)
  {
    LOG_ERROR("Transfer error: " << e.what());
    std::string err_code = API_RETURN_CODE_INTERNAL_ERROR;
    err_code += std::string(":") + e.what();
    return err_code;
  }
  catch (...)
  {
    LOG_ERROR("Transfer error: unknown error");
    return API_RETURN_CODE_INTERNAL_ERROR;
  }

  return API_RETURN_CODE_OK;
}

std::string daemon_backend::get_wallet_info(size_t wallet_id, view::wallet_info& wi)
{
  GET_WALLET_OPT_BY_ID(wallet_id, w);
  return get_wallet_info(w, wi);
}

std::string daemon_backend::resync_wallet(uint64_t wallet_id)
{
  GET_WALLET_OPT_BY_ID(wallet_id, w);
  w.w->get()->reset_history();
  w.last_wallet_synch_height = 0;
  return API_RETURN_CODE_OK;
}
std::string daemon_backend::start_pos_mining(uint64_t wallet_id)
{
  GET_WALLET_OPT_BY_ID(wallet_id, wo);
  wo.do_mining = true;
  view::wallet_status_info wsi = AUTO_VAL_INIT(wsi);
  wsi.is_mining = true;
  wsi.wallet_id = wallet_id;
  wsi.wallet_state = view::wallet_status_info::wallet_state_ready;
  m_pview->update_wallet_status(wsi);
  return API_RETURN_CODE_OK;
}
std::string daemon_backend::stop_pos_mining(uint64_t wallet_id)
{
  GET_WALLET_OPT_BY_ID(wallet_id, wo);
  wo.do_mining = false;
  view::wallet_status_info wsi = AUTO_VAL_INIT(wsi);
  wsi.is_mining = false;
  wsi.wallet_id = wallet_id;
  wsi.wallet_state = view::wallet_status_info::wallet_state_ready;
  m_pview->update_wallet_status(wsi);
  return API_RETURN_CODE_OK;
}
std::string daemon_backend::get_wallet_info(wallet_vs_options& wo, view::wallet_info& wi)
{
  wi = view::wallet_info();
  wi.address = wo.w->get()->get_account().get_public_address_str();
  wi.tracking_hey = string_tools::pod_to_hex(wo.w->get()->get_account().get_keys().m_view_secret_key);
  wi.balance = wo.w->get()->balance();
  wi.unlocked_balance = wo.w->get()->unlocked_balance();
  wi.path = wo.w->get()->get_wallet_path();
  wi.do_mint = wo.do_mining;
  wi.mint_is_in_progress = false;//m_mint_is_running;
  return API_RETURN_CODE_OK;
}

std::string daemon_backend::push_offer(size_t wallet_id, const currency::offer_details& od)
{
  GET_WALLET_BY_ID(wallet_id, w);

  try
  {
    w->get()->push_offer(od);
    return API_RETURN_CODE_OK;
  }
  catch (const std::exception& e)
  {
    LOG_ERROR("Transfer error: " << e.what());
    std::string err_code = API_RETURN_CODE_INTERNAL_ERROR;
    err_code += std::string(":") + e.what();
    return err_code;
  }
  catch (...)
  {
    LOG_ERROR("Transfer error: unknown error");
    return API_RETURN_CODE_INTERNAL_ERROR;
  }
}

std::string daemon_backend::get_all_offers(currency::COMMAND_RPC_GET_ALL_OFFERS::response& od)
{
  currency::COMMAND_RPC_GET_ALL_OFFERS::request rq = AUTO_VAL_INIT(rq);
  m_rpc_proxy->call_COMMAND_RPC_GET_ALL_OFFERS(rq, od);
  return API_RETURN_CODE_OK;
}

std::string daemon_backend::validate_address(const std::string& path)
{
  currency::account_public_address acc = AUTO_VAL_INIT(acc);
  if (currency::get_account_address_from_str(acc, path))
    return API_RETURN_CODE_TRUE;
  else
    return API_RETURN_CODE_FALSE;
}

void daemon_backend::on_new_block(uint64_t /*height*/, const currency::block& /*block*/)
{

}

void daemon_backend::on_transfer2(size_t wallet_id, const tools::wallet_rpc::wallet_transfer_info& wti)
{
  view::transfer_event_info tei = AUTO_VAL_INIT(tei);
  tei.ti = wti;

  CRITICAL_REGION_LOCAL(m_wallets_lock);
  auto& w = m_wallets[wallet_id].w;
  if (w->get() != nullptr)
  {
    tei.balance = w->get()->balance();
    tei.unlocked_balance = w->get()->unlocked_balance();
    tei.wallet_id = wallet_id;
  }else
  {
    LOG_ERROR("on_transfer() wallet with id = " << wallet_id <<  " not found");
  }
  m_pview->money_transfer(tei);
}
void daemon_backend::on_pos_block_found(const currency::block& b)
{
  m_pview->pos_block_found(b);
}

void daemon_backend::wallet_vs_options::worker_func()
{
  LOG_PRINT_GREEN("[POS_MINER] Wallet miner thread started", LOG_LEVEL_0);
  time_t last_mining_timestamp = 0;
  while (!stop)
  {
    //******************************************************************************************
    //sync zone
    if (*plast_daemon_height != last_wallet_synch_height)
    {
      view::wallet_status_info wsi = AUTO_VAL_INIT(wsi);
      wsi.is_mining = do_mining;
      wsi.wallet_state = view::wallet_status_info::wallet_state_synchronizing;
      wsi.wallet_id = wallet_id;
      pview->update_wallet_status(wsi);
      try
      {
        w->get()->refresh(stop);
        w->get()->scan_tx_pool();
      }
      catch (const tools::error::daemon_busy& /*e*/)
      {
        boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
        continue;
      }

      catch (const std::exception& e)
      {
        LOG_PRINT_L0("Failed to refresh wallet: " << e.what());
        wsi.wallet_state = view::wallet_status_info::wallet_state_error;
        pview->update_wallet_status(wsi);
        return;
      }

      catch (...)
      {
        LOG_PRINT_L0("Failed to refresh wallet, unknownk exception");
        wsi.wallet_state = view::wallet_status_info::wallet_state_error;
        pview->update_wallet_status(wsi);
        return;
      }
      wsi.wallet_state = view::wallet_status_info::wallet_state_error;
      pview->update_wallet_status(wsi);
      //do refresh
      last_wallet_synch_height = static_cast<uint64_t>(*plast_daemon_height);
    }
    if (stop)
      break;
    //******************************************************************************************
    //mining zone
    if (!do_mining || time(nullptr) - last_mining_timestamp < POS_WALLET_MINING_SCAN_INTERVAL)
    {
      boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
      continue;
    }
    tools::wallet2::mining_context ctx = AUTO_VAL_INIT(ctx);
    LOG_PRINT_L0("Starting PoS mint iteration");
    w->get()->fill_mining_context(ctx);
    LOG_PRINT_L0("POS_ENTRIES: " << ctx.sp.pos_entries.size());
    tools::wallet2::scan_pos(ctx, break_mining_loop, [](){return true;});
    
    if (ctx.rsp.status == CORE_RPC_STATUS_OK)
    {
      w->get()->build_minted_block(ctx.sp, ctx.rsp);
    }
    LOG_PRINT_L0("PoS mint iteration finished(" << ctx.rsp.status << ")");
    last_mining_timestamp = time(nullptr);

  }
  LOG_PRINT_GREEN("[POS_MINER] Wallet miner thread stopped", LOG_LEVEL_0);
}
daemon_backend::wallet_vs_options::~wallet_vs_options()
{
  do_mining = false;
  stop = true;
  break_mining_loop = true;
  if (miner_thread.joinable())
    miner_thread.join();
}