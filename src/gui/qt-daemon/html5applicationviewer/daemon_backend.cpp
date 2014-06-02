// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/program_options.hpp>
#include "daemon_backend.h"

namespace po = boost::program_options;

bool daemon_backend::init(int argc, char* argv[])
{
  #ifdef WIN32
  _CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
  #endif

  log_space::get_set_log_detalisation_level(true, LOG_LEVEL_0);
  log_space::log_singletone::add_logger(LOGGER_CONSOLE, NULL, NULL);
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

  //create objects and link them
  currency::core ccore(NULL);
  ccore.set_checkpoints(std::move(checkpoints));
  currency::t_currency_protocol_handler<currency::core> cprotocol(ccore, NULL);
  nodetool::node_server<currency::t_currency_protocol_handler<currency::core> > p2psrv(cprotocol);
  currency::core_rpc_server rpc_server(ccore, p2psrv);
  cprotocol.set_p2p_endpoint(&p2psrv);
  ccore.set_currency_protocol(&cprotocol);
  daemon_cmmands_handler dch(p2psrv);
  tools::miniupnp_helper upnp_helper;

  //initialize objects
  LOG_PRINT_L0("Initializing p2p server...");
  res = p2psrv.init(vm);
  CHECK_AND_ASSERT_MES(res, false, "Failed to initialize p2p server.");
  LOG_PRINT_L0("P2p server initialized OK on port: " << p2psrv.get_this_peer_port());

  LOG_PRINT_L0("Starting UPnP");
  upnp_helper.run_port_mapping_loop(p2psrv.get_this_peer_port(), p2psrv.get_this_peer_port(), 20*60*1000);

  LOG_PRINT_L0("Initializing currency protocol...");
  res = cprotocol.init(vm);
  CHECK_AND_ASSERT_MES(res, false, "Failed to initialize currency protocol.");
  LOG_PRINT_L0("Currency protocol initialized OK");

  LOG_PRINT_L0("Initializing core rpc server...");
  res = rpc_server.init(vm);
  CHECK_AND_ASSERT_MES(res, false, "Failed to initialize core rpc server.");
  LOG_PRINT_GREEN("Core rpc server initialized OK on port: " << rpc_server.get_binded_port(), LOG_LEVEL_0);

  //initialize core here
  LOG_PRINT_L0("Initializing core...");
  res = ccore.init(vm);
  CHECK_AND_ASSERT_MES(res, false, "Failed to initialize core");
  LOG_PRINT_L0("Core initialized OK");

  LOG_PRINT_L0("Starting core rpc server...");
  res = rpc_server.run(2, false);
  CHECK_AND_ASSERT_MES(res, false, "Failed to initialize core rpc server.");
  LOG_PRINT_L0("Core rpc server started ok");

  LOG_PRINT_L0("Starting p2p net loop...");
  p2psrv.run();
  LOG_PRINT_L0("p2p net loop stopped");

  //stop components
  LOG_PRINT_L0("Stopping core rpc server...");
  rpc_server.send_stop_signal();
  rpc_server.timed_wait_server_stop(5000);

  //deinitialize components
  LOG_PRINT_L0("Deinitializing core...");
  ccore.deinit();
  LOG_PRINT_L0("Deinitializing rpc server ...");
  rpc_server.deinit();
  LOG_PRINT_L0("Deinitializing currency_protocol...");
  cprotocol.deinit();
  LOG_PRINT_L0("Deinitializing p2p...");
  p2psrv.deinit();


  ccore.set_currency_protocol(NULL);
  cprotocol.set_p2p_endpoint(NULL);

  LOG_PRINT("Node stopped.", LOG_LEVEL_0);
  return true;

  CATCH_ENTRY_L0("main", 1);
 }
