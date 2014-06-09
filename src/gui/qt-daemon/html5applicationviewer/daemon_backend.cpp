// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "daemon_backend.h"



daemon_backend::daemon_backend():m_pview(&m_view_stub),
                                 m_stop_singal_sent(false),
                                 m_ccore(&m_cprotocol),
                                 m_cprotocol(m_ccore, &m_p2psrv),
                                 m_p2psrv(m_cprotocol),
                                 m_rpc_server(m_ccore, m_p2psrv),
                                 m_rpc_proxy(m_rpc_server)
{}

const command_line::arg_descriptor<bool> arg_alloc_win_console   = {"alloc-win-clonsole", "Allocates debug console with GUI", false};

daemon_backend::~daemon_backend()
{
  stop();
}

bool daemon_backend::start(int argc, char* argv[], view::i_view* pview_handler)
{
  m_stop_singal_sent = false;
  if(pview_handler)
    m_pview = pview_handler;

  view::daemon_status_info dsi;// = AUTO_VAL_INIT(dsi);
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

    std::string data_dir = command_line::get_arg(vm, command_line::arg_data_dir);
    std::string config = command_line::get_arg(vm, command_line::arg_config_file);

    boost::filesystem::path data_dir_path(data_dir);
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
  if(command_line::has_arg(vm, arg_alloc_win_console))
  {
     log_space::log_singletone::add_logger(LOGGER_CONSOLE, NULL, NULL);
  }

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

void daemon_backend::main_worker(const po::variables_map& vm)
{
  view::daemon_status_info dsi;// = AUTO_VAL_INIT(dsi);
  dsi.difficulty = "---";
  m_pview->update_daemon_status(dsi);

  //initialize objects
  LOG_PRINT_L0("Initializing p2p server...");
  dsi.text_state = "Initializing p2p server";
  m_pview->update_daemon_status(dsi);
  bool res = m_p2psrv.init(vm);
  CHECK_AND_ASSERT_MES(res, void(), "Failed to initialize p2p server.");
  LOG_PRINT_L0("P2p server initialized OK on port: " << m_p2psrv.get_this_peer_port());

  //LOG_PRINT_L0("Starting UPnP");
  //upnp_helper.run_port_mapping_loop(p2psrv.get_this_peer_port(), p2psrv.get_this_peer_port(), 20*60*1000);

  LOG_PRINT_L0("Initializing currency protocol...");
  dsi.text_state = "Initializing currency protocol";
  m_pview->update_daemon_status(dsi);
  res = m_cprotocol.init(vm);
  CHECK_AND_ASSERT_MES(res, void(), "Failed to initialize currency protocol.");
  LOG_PRINT_L0("Currency protocol initialized OK");

  LOG_PRINT_L0("Initializing core rpc server...");
  dsi.text_state = "Initializing core rpc server";
  m_pview->update_daemon_status(dsi);
  res = m_rpc_server.init(vm);
  CHECK_AND_ASSERT_MES(res, void(), "Failed to initialize core rpc server.");
  LOG_PRINT_GREEN("Core rpc server initialized OK on port: " << m_rpc_server.get_binded_port(), LOG_LEVEL_0);

  //initialize core here
  LOG_PRINT_L0("Initializing core...");
  dsi.text_state = "Initializing core";
  m_pview->update_daemon_status(dsi);
  res = m_ccore.init(vm);
  CHECK_AND_ASSERT_MES(res, void(), "Failed to initialize core");
  LOG_PRINT_L0("Core initialized OK");

  LOG_PRINT_L0("Starting core rpc server...");
  dsi.text_state = "Starting core rpc server";
  m_pview->update_daemon_status(dsi);
  res = m_rpc_server.run(2, false);
  CHECK_AND_ASSERT_MES(res, void(), "Failed to initialize core rpc server.");
  LOG_PRINT_L0("Core rpc server started ok");

  LOG_PRINT_L0("Starting p2p net loop...");
  dsi.text_state = "Starting network loop";
  m_pview->update_daemon_status(dsi);
  m_p2psrv.run(false);
  LOG_PRINT_L0("p2p net loop stopped");

  //go to monitoring view loop
  loop();
  dsi.daemon_network_state = 3;
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
  view::daemon_status_info dsi;// = AUTO_VAL_INIT(dsi);
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
  dsi.height = inf.height;

  m_pview->update_daemon_status(dsi);
  return true;
}

void daemon_backend::loop()
{
  while(!m_stop_singal_sent)
  {
    update_state_info();
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }
}
