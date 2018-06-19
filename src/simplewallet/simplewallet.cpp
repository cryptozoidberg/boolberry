// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <thread>
#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include "include_base_utils.h"
#include "common/command_line.h"
#include "common/util.h"
#include "p2p/net_node.h"
#include "currency_protocol/currency_protocol_handler.h"
#include "simplewallet.h"
#include "currency_core/currency_format_utils.h"
#include "storages/http_abstract_invoke.h"
#include "rpc/core_rpc_server_commands_defs.h"
#include "wallet/wallet_rpc_server.h"
#include "crypto/mnemonic-encoding.h"
#include "version.h"

#if defined(WIN32)
#include <crtdbg.h>
#endif

using namespace std;
using namespace epee;
using namespace currency;
using boost::lexical_cast;
namespace po = boost::program_options;

#define EXTENDED_LOGS_FILE "wallet_details.log"
#define DEFAULT_PAYMENT_ID_SIZE_BYTES 8


namespace
{
  const command_line::arg_descriptor<std::string> arg_wallet_file = {"wallet-file", "Use wallet <arg>", ""};
  const command_line::arg_descriptor<std::string> arg_generate_new_wallet = {"generate-new-wallet", "Generate new wallet and save it to <arg> or <address>.wallet by default", ""};
  const command_line::arg_descriptor<std::string> arg_restore_wallet = { "restore-wallet", "Restore wallet from restore seed and save it to <arg> or <address>.wallet by default", "" };
  const command_line::arg_descriptor<std::string> arg_daemon_address = { "daemon-address", "Use daemon instance at <host>:<port>", "" };
  const command_line::arg_descriptor<std::string> arg_daemon_host = {"daemon-host", "Use daemon instance at host <arg> instead of localhost", ""};
  const command_line::arg_descriptor<std::string> arg_password = {"password", "Wallet password", "", true};
  const command_line::arg_descriptor<std::string> arg_restore_seed = { "restore-seed", "Restore wallet from the 24-word seed", ""};
  const command_line::arg_descriptor<int> arg_daemon_port = { "daemon-port", "Use daemon instance at port <arg> instead of default", 0 };
  const command_line::arg_descriptor<uint32_t> arg_log_level = {"set_log", "", 0, true};

  const command_line::arg_descriptor< std::vector<std::string> > arg_command = {"command", ""};

  inline std::string interpret_rpc_response(bool ok, const std::string& status)
  {
    std::string err;
    if (ok)
    {
      if (status == CORE_RPC_STATUS_BUSY)
      {
        err = "daemon is busy. Please try later";
      }
      else if (status != CORE_RPC_STATUS_OK)
      {
        err = status;
      }
    }
    else
    {
      err = "possible lost connection to daemon";
    }
    return err;
  }

  class message_writer
  {
  public:
    message_writer(epee::log_space::console_colors color = epee::log_space::console_color_default, bool bright = false,
      std::string&& prefix = std::string(), int log_level = LOG_LEVEL_2)
      : m_flush(true)
      , m_color(color)
      , m_bright(bright)
      , m_log_level(log_level)
    {
      m_oss << prefix;
    }

    message_writer(message_writer&& rhs)
      : m_flush(std::move(rhs.m_flush))
#if defined(_MSC_VER)
      , m_oss(std::move(rhs.m_oss))
#else
      // GCC bug: http://gcc.gnu.org/bugzilla/show_bug.cgi?id=54316
      , m_oss(rhs.m_oss.str(), ios_base::out | ios_base::ate) 
#endif
      , m_color(std::move(rhs.m_color))
      , m_log_level(std::move(rhs.m_log_level))
    {
      rhs.m_flush = false;
    }

    template<typename T>
    std::ostream& operator<<(const T& val)
    {
      m_oss << val;
      return m_oss;
    }

    ~message_writer()
    {
      if (m_flush)
      {
        m_flush = false;

        LOG_PRINT(m_oss.str(), m_log_level)

        if (epee::log_space::console_color_default == m_color)
        {
          std::cout << m_oss.str();
        }
        else
        {
          epee::log_space::set_console_color(m_color, m_bright);
          std::cout << m_oss.str();
          epee::log_space::reset_console_color();
        }
        std::cout << std::endl;
      }
    }

  private:
    message_writer(message_writer& rhs);
    message_writer& operator=(message_writer& rhs);
    message_writer& operator=(message_writer&& rhs);

  private:
    bool m_flush;
    std::stringstream m_oss;
    epee::log_space::console_colors m_color;
    bool m_bright;
    int m_log_level;
  };

  message_writer success_msg_writer(bool color = false)
  {
    return message_writer(color ? epee::log_space::console_color_green : epee::log_space::console_color_default, false, std::string(), LOG_LEVEL_2);
  }

  message_writer fail_msg_writer()
  {
    return message_writer(epee::log_space::console_color_red, true, "Error: ", LOG_LEVEL_0);
  }
}

std::string simple_wallet::get_commands_str()
{
  std::stringstream ss;
  ss << "Commands: " << ENDL;
  std::string usage = m_cmd_binder.get_usage();
  boost::replace_all(usage, "\n", "\n  ");
  usage.insert(0, "  ");
  ss << usage << ENDL;
  return ss.str();
}

bool simple_wallet::help(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  success_msg_writer() << get_commands_str();
  return true;
}

simple_wallet::simple_wallet()
  : m_daemon_port(0)
  , m_refresh_progress_reporter(*this)
{
  m_cmd_binder.set_handler("start_mining", boost::bind(&simple_wallet::start_mining, this, _1), "start_mining <threads_count> - Start mining in daemon");
  m_cmd_binder.set_handler("stop_mining", boost::bind(&simple_wallet::stop_mining, this, _1), "Stop mining in daemon");
  m_cmd_binder.set_handler("refresh", boost::bind(&simple_wallet::refresh, this, _1), "Resynchronize transactions and balance");
  m_cmd_binder.set_handler("balance", boost::bind(&simple_wallet::show_balance, this, _1), "Show current wallet balance");
  m_cmd_binder.set_handler("incoming_transfers", boost::bind(&simple_wallet::show_incoming_transfers, this, _1), "incoming_transfers [available|unavailable] - Show incoming transfers - all of them or filter them by availability");
  m_cmd_binder.set_handler("list_recent_transfers", boost::bind(&simple_wallet::list_recent_transfers, this, _1), "list_recent_transfers - Show recent maximum 1000 transfers");
  m_cmd_binder.set_handler("payments", boost::bind(&simple_wallet::show_payments, this, _1), "payments <payment_id_1> [<payment_id_2> ... <payment_id_N>] - Show payments <payment_id_1>, ... <payment_id_N>");
  m_cmd_binder.set_handler("bc_height", boost::bind(&simple_wallet::show_blockchain_height, this, _1), "Show blockchain height");
  m_cmd_binder.set_handler("transfer", boost::bind(&simple_wallet::transfer, this, _1), "transfer <mixin_count> <addr_1> <amount_1> [<addr_2> <amount_2> ... <addr_N> <amount_N>] [payment_id] - Transfer <amount_1>,... <amount_N> to <address_1>,... <address_N>, respectively. <mixin_count> is the number of transactions yours is indistinguishable from (from 0 to maximum available)");
  
  m_cmd_binder.set_handler("save_watch_only", boost::bind(&simple_wallet::save_watch_only, this, _1), "Save a watch-only keys file <filename> <password>.");
  m_cmd_binder.set_handler("sign_transfer", boost::bind(&simple_wallet::sign_transfer, this, _1), "Sign_transfer <tx_sources_file> <result_file>");
  m_cmd_binder.set_handler("submit_transfer", boost::bind(&simple_wallet::submit_transfer, this, _1), "Submit a signed transaction from a file <tx_sources_file> <result_file>");

  m_cmd_binder.set_handler("integrated_address", boost::bind(&simple_wallet::integrated_address, this, _1), "integrated_address [payment_id|integrated_address] Encode given payment_id into an integrated address (for this waller public address). Uses random id if not provided. Decode given integrated address into payment id and standard address.");

  m_cmd_binder.set_handler("get_tx_key", boost::bind(&simple_wallet::get_tx_key, this, _1), "Get transaction key (r) for a given <txid>");
  m_cmd_binder.set_handler("check_tx_key", boost::bind(&simple_wallet::check_tx_key, this, _1), "Check amount going to <address> in <txid>");
  m_cmd_binder.set_handler("set_log", boost::bind(&simple_wallet::set_log, this, _1), "set_log <level> - Change current log detalization level, <level> is a number 0-4");
  m_cmd_binder.set_handler("address", boost::bind(&simple_wallet::print_address, this, _1), "Show current wallet public address");
  m_cmd_binder.set_handler("sign_text", boost::bind(&simple_wallet::sign_text, this, _1), "Sign some random text as a proof");
  m_cmd_binder.set_handler("validate_text_signature", boost::bind(&simple_wallet::validate_text_signature, this, _1), "Validate signed text's proof: validate_text_signature <text> <address> <signature>");
  m_cmd_binder.set_handler("save", boost::bind(&simple_wallet::save, this, _1), "Save wallet synchronized data");
  m_cmd_binder.set_handler("help", boost::bind(&simple_wallet::help, this, _1), "Show this help");
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::set_log(const std::vector<std::string> &args)
{
  if(args.size() != 1) 
  {
    fail_msg_writer() << "use: set_log <log_level_number_0-4>";
    return true;
  }
  uint16_t l = 0;
  if(!string_tools::get_xtype_from_string(l, args[0]))
  {
    fail_msg_writer() << "wrong number format, use: set_log <log_level_number_0-4>";
    return true;
  }
  if(LOG_LEVEL_4 < l)
  {
    fail_msg_writer() << "wrong number range, use: set_log <log_level_number_0-4>";
    return true;
  }

  log_space::log_singletone::get_set_log_detalisation_level(true, l);
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::init(const boost::program_options::variables_map& vm)
{
  handle_command_line(vm);

  if (!m_daemon_address.empty() && !m_daemon_host.empty() && 0 != m_daemon_port)
  {
    fail_msg_writer() << "you can't specify daemon host or port several times";
    return false;
  }

  size_t c = 0; 
  if(!m_generate_new.empty()) ++c;
  if(!m_wallet_file.empty()) ++c;
  if (!m_restore_wallet.empty()) ++c;
  if (1 != c)
  {
    fail_msg_writer() << "you must specify --wallet-file or --generate-new-wallet or --restore-wallet params";
    return false;
  }

  if (m_daemon_host.empty())
    m_daemon_host = "localhost";
  if (!m_daemon_port)
    m_daemon_port = RPC_DEFAULT_PORT;
  if (m_daemon_address.empty())
    m_daemon_address = std::string("http://") + m_daemon_host + ":" + std::to_string(m_daemon_port);

  tools::password_container pwd_container;
  if (command_line::has_arg(vm, arg_password))
  {
    pwd_container.password(command_line::get_arg(vm, arg_password));
  }
  else
  {
    bool r = pwd_container.read_password();
    if (!r)
    {
      fail_msg_writer() << "failed to read wallet password";
      return false;
    }
  }

  if (!m_generate_new.empty())
  {
    bool r = new_wallet(m_generate_new, pwd_container.password());
    CHECK_AND_ASSERT_MES(r, false, "account creation failed");
  }
  else if (!m_restore_wallet.empty())
  {
	  if (m_restore_seed.empty())
	  {
		  fail_msg_writer() << "you must specify 24-word restore seed to restore wallet";
		  return false;
	  }
	  bool r = restore_wallet(m_restore_wallet, m_restore_seed, pwd_container.password());
	  CHECK_AND_ASSERT_MES(r, false, "account restore failed");
  }
  else
  {
    bool r = open_wallet(m_wallet_file, pwd_container.password());
    CHECK_AND_ASSERT_MES(r, false, "could not open account");
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::deinit()
{
  if (!m_wallet.get())
    return true;

  return close_wallet();
}
//----------------------------------------------------------------------------------------------------
void simple_wallet::handle_command_line(const boost::program_options::variables_map& vm)
{
  m_wallet_file    = command_line::get_arg(vm, arg_wallet_file);
  m_generate_new   = command_line::get_arg(vm, arg_generate_new_wallet);
  m_daemon_address = command_line::get_arg(vm, arg_daemon_address);
  m_daemon_host    = command_line::get_arg(vm, arg_daemon_host);
  m_daemon_port    = command_line::get_arg(vm, arg_daemon_port);
  m_restore_wallet = command_line::get_arg(vm, arg_restore_wallet);
  m_restore_seed   = command_line::get_arg(vm, arg_restore_seed);
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::try_connect_to_daemon()
{
  if (!m_wallet->check_connection())
  {
    fail_msg_writer() << "wallet failed to connect to daemon (" << m_daemon_address << "). " <<
      "Daemon either is not started or passed wrong port. " <<
      "Please, make sure that daemon is running or restart the wallet with correct daemon address.";
    return false;
  }
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::new_wallet(const string &wallet_file, const std::string& password)
{
  m_wallet_file = wallet_file;

  m_wallet.reset(new tools::wallet2());
  m_wallet->callback(this);
  std::vector<unsigned char> restore_seed;
  try
  {
    restore_seed = m_wallet->generate(wallet_file, password);
    message_writer(epee::log_space::console_color_white, true) << "Generated new wallet: " << m_wallet->get_account().get_public_address_str() << std::endl << "view key: " << string_tools::pod_to_hex(m_wallet->get_account().get_keys().m_view_secret_key);
  }
  catch (const std::exception& e)
  {
    fail_msg_writer() << "failed to generate new wallet: " << e.what();
    return false;
  }

  m_wallet->init(m_daemon_address);

  success_msg_writer() <<
	  "**********************************************************************\n" <<
	  "Your wallet has been generated.\n" <<
	  "To start synchronizing with the daemon use \"refresh\" command.\n" <<
	  "Use \"help\" command to see the list of available commands.\n" <<
	  "Always use \"exit\" command when closing simplewallet to save\n" <<
	  "current session's state. Otherwise, you will possibly need to synchronize \n" <<
	  "your wallet again. Your wallet key is NOT under risk anyway.\n"; 
  success_msg_writer(true) << 
	  "\nPLEASE NOTE: the following 24 words can be used to recover access to your wallet. Please write them down and store them somewhere safe and secure. Please do not store them in your email or on file storage services outside of your immediate control.\n";
  success_msg_writer() << crypto::mnemonic_encoding::binary2text(restore_seed) << "\n";
  success_msg_writer() << 
	  "**********************************************************************";
  return true;
}

//----------------------------------------------------------------------------------------------------
bool simple_wallet::restore_wallet(const std::string &wallet_file, const std::string &restore_seed, const std::string& password)
{
	m_wallet_file = wallet_file;

	m_wallet.reset(new tools::wallet2());
	m_wallet->callback(this);
	try
	{
		std::vector<unsigned char> seed = crypto::mnemonic_encoding::text2binary(restore_seed);
		m_wallet->restore(wallet_file, seed, password);
		message_writer(epee::log_space::console_color_white, true) << "Wallet restored: " << m_wallet->get_account().get_public_address_str() << std::endl << "view key: " << string_tools::pod_to_hex(m_wallet->get_account().get_keys().m_view_secret_key);
	}
	catch (const std::exception& e)
	{
		fail_msg_writer() << "failed to restore wallet: " << e.what();
		return false;
	}

	m_wallet->init(m_daemon_address);

	success_msg_writer() <<
		"**********************************************************************\n" <<
		"Your wallet has been restored.\n" <<
		"To start synchronizing with the daemon use \"refresh\" command.\n" <<
		"Use \"help\" command to see the list of available commands.\n" <<
		"Always use \"exit\" command when closing simplewallet to save\n" <<
		"current session's state. Otherwise, you will possibly need to synchronize \n" <<
		"your wallet again. Your wallet key is NOT under risk anyway.\n" <<
		"**********************************************************************";
	return true;
}

//----------------------------------------------------------------------------------------------------
bool simple_wallet::open_wallet(const string &wallet_file, const std::string& password)
{
  m_wallet_file = wallet_file;
  m_wallet.reset(new tools::wallet2());
  m_wallet->callback(this);

  try
  {
    m_wallet->load(m_wallet_file, password);
    message_writer(epee::log_space::console_color_white, true) << "Opened wallet" << (m_wallet->is_view_only() ? "watch-only" : "" ) << ": " << m_wallet->get_account().get_public_address_str();
  }
  catch (const std::exception& e)
  {
    fail_msg_writer() << "failed to load wallet: " << e.what();
    return false;
  }

  m_wallet->init(m_daemon_address);

  refresh(std::vector<std::string>());
  success_msg_writer() <<
    "**********************************************************************\n" <<
    "Use \"help\" command to see the list of available commands.\n" <<
    "**********************************************************************";
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::close_wallet()
{
  bool r = m_wallet->deinit();
  if (!r)
  {
    fail_msg_writer() << "failed to deinit wallet";
    return false;
  }

  try
  {
    m_wallet->store();
  }
  catch (const std::exception& e)
  {
    fail_msg_writer() << e.what();
    return false;
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::save(const std::vector<std::string> &args)
{
  try
  {
    m_wallet->store();
    success_msg_writer() << "Wallet data saved";
  }
  catch (const std::exception& e)
  {
    fail_msg_writer() << e.what();
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::start_mining(const std::vector<std::string>& args)
{
  if (!try_connect_to_daemon())
    return true;

  COMMAND_RPC_START_MINING::request req;
  req.miner_address = m_wallet->get_account().get_public_address_str();

  if (0 == args.size())
  {
    req.threads_count = 1;
  }
  else if (1 == args.size())
  {
    uint16_t num;
    bool ok = string_tools::get_xtype_from_string(num, args[0]);
    if(!ok || 0 == num)
    {
      fail_msg_writer() << "wrong number of mining threads: \"" << args[0] << "\"";
      return true;
    }
    req.threads_count = num;
  }
  else
  {
    fail_msg_writer() << "wrong number of arguments, expected the number of mining threads";
    return true;
  }

  COMMAND_RPC_START_MINING::response res;
  bool r = net_utils::invoke_http_json_remote_command2(m_daemon_address + "/start_mining", req, res, m_http_client);
  std::string err = interpret_rpc_response(r, res.status);
  if (err.empty())
    success_msg_writer() << "Mining started in daemon";
  else
    fail_msg_writer() << "mining has NOT been started: " << err;
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::stop_mining(const std::vector<std::string>& args)
{
  if (!try_connect_to_daemon())
    return true;

  COMMAND_RPC_STOP_MINING::request req;
  COMMAND_RPC_STOP_MINING::response res;
  bool r = net_utils::invoke_http_json_remote_command2(m_daemon_address + "/stop_mining", req, res, m_http_client);
  std::string err = interpret_rpc_response(r, res.status);
  if (err.empty())
    success_msg_writer() << "Mining stopped in daemon";
  else
    fail_msg_writer() << "mining has NOT been stopped: " << err;
  return true;
}
//----------------------------------------------------------------------------------------------------
void simple_wallet::on_new_block(uint64_t height, const currency::block& block)
{
  m_refresh_progress_reporter.update(height, false);
}
//----------------------------------------------------------------------------------------------------
void simple_wallet::on_money_received(uint64_t height, const currency::transaction& tx, size_t out_index)
{
  message_writer(epee::log_space::console_color_green, false) <<
    "Height " << height <<
    ", transaction " << get_transaction_hash(tx) <<
    ", received " << print_money(tx.vout[out_index].amount);
  m_refresh_progress_reporter.update(height, true);
}
//----------------------------------------------------------------------------------------------------
void simple_wallet::on_money_spent(uint64_t height, const currency::transaction& in_tx, size_t out_index, const currency::transaction& spend_tx)
{
  message_writer(epee::log_space::console_color_magenta, false) <<
    "Height " << height <<
    ", transaction " << get_transaction_hash(spend_tx) <<
    ", spent " << print_money(in_tx.vout[out_index].amount);
  m_refresh_progress_reporter.update(height, true);
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::refresh(const std::vector<std::string>& args)
{
  if (!try_connect_to_daemon())
    return true;

  message_writer() << "Starting refresh...";
  size_t fetched_blocks = 0;
  bool ok = false;
  std::ostringstream ss;
  try
  {
    m_wallet->refresh(fetched_blocks);
    ok = true;
    // Clear line "Height xxx of xxx"
    std::cout << "\r                                                                \r";
    success_msg_writer(true) << "Refresh done, blocks received: " << fetched_blocks;
    show_balance();
  }
  catch (const tools::error::daemon_busy&)
  {
    ss << "daemon is busy. Please try later";
  }
  catch (const tools::error::no_connection_to_daemon&)
  {
    ss << "no connection to daemon. Please, make sure daemon is running";
  }
  catch (const tools::error::wallet_rpc_error& e)
  {
    LOG_ERROR("Unknown RPC error: " << e.to_string());
    ss << "RPC error \"" << e.what() << '"';
  }
  catch (const tools::error::refresh_error& e)
  {
    LOG_ERROR("refresh error: " << e.to_string());
    ss << e.what();
  }
  catch (const tools::error::wallet_internal_error& e)
  {
    LOG_ERROR("internal error: " << e.to_string());
    ss << "internal error: " << e.what();
  }
  catch (const std::exception& e)
  {
    LOG_ERROR("unexpected error: " << e.what());
    ss << "unexpected error: " << e.what();
  }
  catch (...)
  {
    LOG_ERROR("Unknown error");
    ss << "unknown error";
  }

  if (!ok)
  {
    fail_msg_writer() << "refresh failed: " << ss.str() << ". Blocks received: " << fetched_blocks;
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::show_balance(const std::vector<std::string>& args/* = std::vector<std::string>()*/)
{
  success_msg_writer() << "balance: " << print_money(m_wallet->balance()) << ", unlocked balance: " << print_money(m_wallet->unlocked_balance());
  return true;
}
//----------------------------------------------------------------------------------------------------
bool print_wti(const tools::wallet_rpc::wallet_transfer_info& wti)
{
  epee::log_space::console_colors cl;
  if (wti.is_income)
    cl = epee::log_space::console_color_green;
  else
    cl = epee::log_space::console_color_magenta;

  std::string payment_id_placeholder;
  if (wti.payment_id.size())
    payment_id_placeholder = std::string("(payment_id:") + wti.payment_id + ")";

  message_writer(cl) << epee::misc_utils::get_time_str_v2(wti.timestamp) << " "
    << (wti.is_income ? "Received " : "Sent    ")
    << print_money(wti.amount) << "(fee:" << print_money(wti.fee) << ")  "
    << (wti.destination_alias.size() ? wti.destination_alias : wti.destinations)
    << " " << wti.tx_hash << payment_id_placeholder;
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::list_recent_transfers(const std::vector<std::string>& args)
{
  std::vector<tools::wallet_rpc::wallet_transfer_info> unconfirmed;
  std::vector<tools::wallet_rpc::wallet_transfer_info> recent;
  m_wallet->get_recent_transfers_history(recent, 0, 1000);
  m_wallet->get_unconfirmed_transfers(unconfirmed);
  //workaround for missed fee
  
  success_msg_writer() << "Unconfirmed transfers: ";
  for (auto & wti : unconfirmed)
  {
    if (!wti.fee)
      wti.fee = currency::get_tx_fee(wti.tx);
    print_wti(wti);
  }
  success_msg_writer() << "Recent transfers: ";
  for (auto & wti : recent)
  {
    if (!wti.fee)
      wti.fee = currency::get_tx_fee(wti.tx);
    print_wti(wti);
  }
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::show_incoming_transfers(const std::vector<std::string>& args)
{
  bool filter = false;
  bool available = false;
  if (!args.empty())
  {
    if (args[0] == "available")
    {
      filter = true;
      available = true;
    }
    else if (args[0] == "unavailable")
    {
      filter = true;
      available = false;
    }
  }

  tools::wallet2::transfer_container transfers;
  m_wallet->get_transfers(transfers);

  bool transfers_found = false;
  for (const auto& td : transfers)
  {
    if (!filter || available != td.m_spent)
    {
      if (!transfers_found)
      {
        message_writer() << "        amount       \tspent\tglobal index\t                              tx id";
        transfers_found = true;
      }
      message_writer(td.m_spent ? epee::log_space::console_color_magenta : epee::log_space::console_color_green, false) <<
        std::setw(21) << print_money(td.amount()) << '\t' <<
        std::setw(3) << (td.m_spent ? 'T' : 'F') << "  \t" <<
        std::setw(12) << td.m_global_output_index << '\t' <<
        get_transaction_hash(td.m_tx) << "[" << td.m_block_height << "]";
    }
  }

  if (!transfers_found)
  {
    if (!filter)
    {
      success_msg_writer() << "No incoming transfers";
    }
    else if (available)
    {
      success_msg_writer() << "No incoming available transfers";
    }
    else
    {
      success_msg_writer() << "No incoming unavailable transfers";
    }
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
bool simple_wallet::show_payments(const std::vector<std::string> &args)
{
  if(args.empty())
  {
    fail_msg_writer() << "expected at least one payment_id";
    return true;
  }

  message_writer() << "                            payment                             \t" <<
    "                          transaction                           \t" <<
    "  height\t       amount        \tunlock time";

  bool payments_found = false;
  for(std::string arg : args)
  {
    payment_id_t payment_id;
    if(parse_payment_id_from_hex_str(arg, payment_id))
    {
      std::list<tools::wallet2::payment_details> payments;
      m_wallet->get_payments(payment_id, payments);
      if(payments.empty())
      {
        success_msg_writer() << "No payments with id " << arg;
        continue;
      }

      for (const tools::wallet2::payment_details& pd : payments)
      {
        if(!payments_found)
        {
          payments_found = true;
        }
        success_msg_writer(true) <<
          std::left << std::setw(64) << arg << '\t' <<
          pd.m_tx_hash << '\t' <<
          std::setw(8)  << pd.m_block_height << '\t' <<
          std::setw(21) << print_money(pd.m_amount) << '\t' <<
          pd.m_unlock_time;
      }
    }
    else
    {
      fail_msg_writer() << "payment id has invalid format: \"" << arg << "\", expected 64-character string";
    }
  }

  return true;
}
//------------------------------------------------------------------------------------------------------------------
uint64_t simple_wallet::get_daemon_blockchain_height(std::string& err)
{
  COMMAND_RPC_GET_HEIGHT::request req;
  COMMAND_RPC_GET_HEIGHT::response res = boost::value_initialized<COMMAND_RPC_GET_HEIGHT::response>();
  bool r = net_utils::invoke_http_json_remote_command2(m_daemon_address + "/getheight", req, res, m_http_client);
  err = interpret_rpc_response(r, res.status);
  return res.height;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::show_blockchain_height(const std::vector<std::string>& args)
{
  if (!try_connect_to_daemon())
    return true;

  std::string err;
  uint64_t bc_height = get_daemon_blockchain_height(err);
  if (err.empty())
    success_msg_writer() << bc_height;
  else
    fail_msg_writer() << "failed to get blockchain height: " << err;
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::transfer(const std::vector<std::string> &args_)
{
  bool r = false;
  if (!try_connect_to_daemon())
    return true;

  std::vector<std::string> local_args = args_;
  if(local_args.size() < 3)
  {
    fail_msg_writer() << "wrong number of arguments, expected at least 3, got " << local_args.size();
    return true;
  }

  size_t fake_outs_count;
  if(!string_tools::get_xtype_from_string(fake_outs_count, local_args[0]))
  {
    fail_msg_writer() << "mixin_count should be non-negative integer, got " << local_args[0];
    return true;
  }
  local_args.erase(local_args.begin());

  std::vector<uint8_t> extra;
  payment_id_t payment_id;
  if (1 == local_args.size() % 2)
  {
    std::string payment_id_str = local_args.back();
    local_args.pop_back();

    r = parse_payment_id_from_hex_str(payment_id_str, payment_id);
    if(r)
    {
      r = set_payment_id_to_tx_extra(extra, payment_id);
    }

    if(!r)
    {
      fail_msg_writer() << "payment id has invalid format: \"" << payment_id_str << "\", expected hex string";
      return true;
    }
  }

  vector<currency::tx_destination_entry> dsts;
  for (size_t i = 0; i < local_args.size(); i += 2) 
  {
    currency::tx_destination_entry de;
    currency::payment_id_t integrated_payment_id;
    if(!m_wallet->get_transfer_address(local_args[i], de.addr, integrated_payment_id))
    {
      fail_msg_writer() << "wrong address: " << local_args[i];
      return true;
    }

    // handle integrated payment id
    if (!integrated_payment_id.empty())
    {
      if (!payment_id.empty())
      {
        fail_msg_writer() << "address " << local_args[i] << " has integrated payment id " << epee::string_tools::buff_to_hex_nodelimer(integrated_payment_id) <<
          " which is incompatible with payment id " << epee::string_tools::buff_to_hex_nodelimer(payment_id) << " that was already assigned to this transfer";
        return true;
      }

      if (!set_payment_id_to_tx_extra(extra, integrated_payment_id))
      {
        fail_msg_writer() << "address " << local_args[i] << " has invalid integrated payment id " << epee::string_tools::buff_to_hex_nodelimer(integrated_payment_id);
        return true;
      }

      payment_id = integrated_payment_id; // remember integrated payment id as the main payment id
      success_msg_writer() << "NOTE: using payment id " << epee::string_tools::buff_to_hex_nodelimer(payment_id) << " from integrated address " << local_args[i];
    }

    if (local_args.size() <= i + 1)
    {
      fail_msg_writer() << "amount for the last address " << local_args[i] << " is not specified";
      return true;
    }

    bool ok = currency::parse_amount(de.amount, local_args[i + 1]);
    if(!ok || 0 == de.amount)
    {
      fail_msg_writer() << "amount is wrong: " << local_args[i] << ' ' << local_args[i + 1] <<
        ", expected number from 0 to " << print_money(std::numeric_limits<uint64_t>::max());
      return true;
    }

    dsts.push_back(de);
  }

  try
  {
    currency::transaction tx;
    m_wallet->transfer(dsts, fake_outs_count, 0, DEFAULT_FEE, extra, tx);
    if (!m_wallet->is_view_only() )
      success_msg_writer(true) << "Money successfully sent, transaction " << get_transaction_hash(tx) << ", " << get_object_blobsize(tx) << " bytes";
    else
      success_msg_writer(true) << "Transaction prepared for signing and saved into \"unsigned_boolberry_tx\" file, use offline wallet top sign transfer and then use command \"submit_transfer\" on this wallet to transfer transaction into the network";

  }
  catch (const tools::error::daemon_busy&)
  {
    fail_msg_writer() << "daemon is busy. Please try later";
  }
  catch (const tools::error::no_connection_to_daemon&)
  {
    fail_msg_writer() << "no connection to daemon. Please, make sure daemon is running.";
  }
  catch (const tools::error::wallet_rpc_error& e)
  {
    LOG_ERROR("Unknown RPC error: " << e.to_string());
    fail_msg_writer() << "RPC error \"" << e.what() << '"';
  }
  catch (const tools::error::get_random_outs_error&)
  {
    fail_msg_writer() << "failed to get random outputs to mix";
  }
  catch (const tools::error::not_enough_money& e)
  {
    fail_msg_writer() << "not enough money to transfer, available only " << print_money(e.available()) <<
      ", transaction amount " << print_money(e.tx_amount() + e.fee()) << " = " << print_money(e.tx_amount()) <<
      " + " << print_money(e.fee()) << " (fee)";
  }
  catch (const tools::error::not_enough_outs_to_mix& e)
  {
    auto writer = fail_msg_writer();
    writer << "not enough outputs for specified mixin_count = " << e.mixin_count() << ":";
    for (const currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount& outs_for_amount : e.scanty_outs())
    {
      writer << "\noutput amount = " << print_money(outs_for_amount.amount) << ", fount outputs to mix = " << outs_for_amount.outs.size();
    }
  }
  catch (const tools::error::tx_not_constructed&)
  {
    fail_msg_writer() << "transaction was not constructed";
  }
  catch (const tools::error::tx_rejected& e)
  {
    fail_msg_writer() << "transaction " << get_transaction_hash(e.tx()) << " was rejected by daemon with status \"" << e.status() << '"';
  }
  catch (const tools::error::tx_sum_overflow& e)
  {
    fail_msg_writer() << e.what();
  }
  catch (const tools::error::tx_too_big& e)
  {
    currency::transaction tx = e.tx();
    fail_msg_writer() << "transaction " << get_transaction_hash(e.tx()) << " is too big. Transaction size: " <<
      get_object_blobsize(e.tx()) << " bytes, transaction size limit: " << e.tx_size_limit() << " bytes. Try to separate this payment into few smaller transfers.";
  }
  catch (const tools::error::zero_destination&)
  {
    fail_msg_writer() << "one of destinations is zero";
  }
  catch (const tools::error::transfer_error& e)
  {
    LOG_ERROR("unknown transfer error: " << e.to_string());
    fail_msg_writer() << "unknown transfer error: " << e.what();
  }
  catch (const tools::error::wallet_internal_error& e)
  {
    LOG_ERROR("internal error: " << e.to_string());
    fail_msg_writer() << "internal error: " << e.what();
  }
  catch (const std::exception& e)
  {
    LOG_ERROR("unexpected error: " << e.what());
    fail_msg_writer() << "unexpected error: " << e.what();
  }
  catch (...)
  {
    LOG_ERROR("Unknown error");
    fail_msg_writer() << "unknown error";
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::save_watch_only(const std::vector<std::string> &args)
{
  if (args.size() < 2)
  {
    fail_msg_writer() << "wrong parameters, expected filename and password";
    return true;
  }
  try
  {
    m_wallet->store_keys(args[0], args[1], true);
    success_msg_writer() << "Keys stored to " << args[0];
  }
  catch (const std::exception& e)
  {
    LOG_ERROR("unexpected error: " << e.what());
    fail_msg_writer() << "unexpected error: " << e.what();
  }
  catch (...)
  {
    LOG_ERROR("Unknown error");
    fail_msg_writer() << "unknown error";
  }
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::sign_transfer(const std::vector<std::string> &args)
{
  if (m_wallet->is_view_only())
  {
    fail_msg_writer() << "You can't sign transaction in view-only wallet";
    return true;

  }

  if (args.size() < 1)
  {
    fail_msg_writer() << "wrong parameters, expected filename";
    return true;
  }
  try
  {
    currency::transaction res_tx;
    m_wallet->sign_transfer(args[0], args[1], res_tx);
    success_msg_writer() << "transaction signed and stored to file: " << args[1] << ", transaction " << get_transaction_hash(res_tx) << ", " << get_object_blobsize(res_tx) << " bytes";
  }
  catch (const std::exception& e)
  {
    LOG_ERROR("unexpected error: " << e.what());
    fail_msg_writer() << "unexpected error: " << e.what();
  }
  catch (...)
  {
    LOG_ERROR("Unknown error");
    fail_msg_writer() << "unknown error";
  }
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::submit_transfer(const std::vector<std::string> &args)
{
  if (args.size() < 1)
  {
    fail_msg_writer() << "wrong parameters, expected filename";
    return true;
  }
  try
  {
    currency::transaction res_tx;
    m_wallet->submit_transfer(args[0], args[1], res_tx);
    success_msg_writer() << "transaction submitted, " << get_transaction_hash(res_tx) << ", " << get_object_blobsize(res_tx) << " bytes";
  }
  catch (const std::exception& e)
  {
    LOG_ERROR("unexpected error: " << e.what());
    fail_msg_writer() << "unexpected error: " << e.what();
  }
  catch (...)
  {
    LOG_ERROR("Unknown error");
    fail_msg_writer() << "unknown error";
  }
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::get_tx_key(const std::vector<std::string> &args_)
{
  std::vector<std::string> local_args = args_;

  if(local_args.size() != 1) {
    fail_msg_writer() << "usage: get_tx_key <txid>";
    return true;
  }

  currency::blobdata txid_data;
  if(!epee::string_tools::parse_hexstr_to_binbuff(local_args.front(), txid_data) || txid_data.size() != sizeof(crypto::hash))
  {
    fail_msg_writer() << "failed to parse txid";
    return true;
  }
  crypto::hash txid = *reinterpret_cast<const crypto::hash*>(txid_data.data());

  crypto::secret_key tx_key;
  std::vector<crypto::secret_key> amount_keys;
  if (m_wallet->get_tx_key(txid, tx_key))
  {
    success_msg_writer() << "Tx key: " << epee::string_tools::pod_to_hex(tx_key);
    return true;
  }
  else
  {
    fail_msg_writer() << "no tx keys found for this txid";
    return true;
  }
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::sign_text(const std::vector<std::string> &args)
{
  if (args.size() != 1) 
  {
    fail_msg_writer() << "usage: sign_text <text>";
    return true;
  }
  crypto::signature sig = AUTO_VAL_INIT(sig);
  m_wallet->sign_text(args[0], sig);

  success_msg_writer() << "Signature: " << epee::string_tools::pod_to_hex(sig);
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::validate_text_signature(const std::vector<std::string> &args)
{
  if (args.size() != 3)
  {
    fail_msg_writer() << "usage: validate_text_signature <text> <address> <signature>";
    return true;
  }
  crypto::signature sig = AUTO_VAL_INIT(sig);
  if (!epee::string_tools::parse_tpod_from_hex_string(args[2], sig))
  {
    fail_msg_writer() << "wrong signature format" << ENDL << "usage: validate_text_signature <text> <address> <signature>";
    return true;
  }

  std::string r = m_wallet->validate_signed_text(args[1], args[0], sig);

  success_msg_writer() << "Validation result: " << r;
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::check_tx_key(const std::vector<std::string> &args_)
{
  std::vector<std::string> local_args = args_;

  if(local_args.size() != 3) {
    fail_msg_writer() << "usage: check_tx_key <txid> <txkey> <address>";
    return true;
  }

  if (!try_connect_to_daemon())
    return true;

  if (!m_wallet)
  {
    fail_msg_writer() << "wallet is null";
    return true;
  }
  currency::blobdata txid_data;
  if(!epee::string_tools::parse_hexstr_to_binbuff(local_args[0], txid_data) || txid_data.size() != sizeof(crypto::hash))
  {
    fail_msg_writer() << "failed to parse txid";
    return true;
  }
  crypto::hash txid = *reinterpret_cast<const crypto::hash*>(txid_data.data());

  if (local_args[1].size() < 64 || local_args[1].size() % 64)
  {
    fail_msg_writer() << "failed to parse tx key";
    return true;
  }
  crypto::secret_key tx_key;
  currency::blobdata tx_key_data;
  if(!epee::string_tools::parse_hexstr_to_binbuff(local_args[1], tx_key_data) || tx_key_data.size() != sizeof(crypto::secret_key))
  {
    fail_msg_writer() << "failed to parse tx key";
    return true;
  }
  tx_key = *reinterpret_cast<const crypto::secret_key*>(tx_key_data.data());

  currency::account_public_address address;
  if(!currency::get_account_address_from_str(address, local_args[2]))
  {
    fail_msg_writer() << "failed to parse address";
    return true;
  }

  crypto::key_derivation derivation;
  if (!crypto::generate_key_derivation(address.m_view_public_key, tx_key, derivation))
  {
    fail_msg_writer() << "failed to generate key derivation from supplied parameters";
    return true;
  }

  return check_tx_key_helper(txid, address, derivation);
}

//----------------------------------------------------------------------------------------------------
bool simple_wallet::check_tx_key_helper(const crypto::hash &txid, const currency::account_public_address &address, const crypto::key_derivation &derivation)
{
  COMMAND_RPC_GET_TRANSACTIONS::request req;
  COMMAND_RPC_GET_TRANSACTIONS::response res;
  req.txs_hashes.push_back(epee::string_tools::pod_to_hex(txid));
  if (!net_utils::invoke_http_json_remote_command2(m_daemon_address + "/gettransactions", req, res, m_http_client) ||
      (res.txs_as_hex.size() != 1))
  {
    fail_msg_writer() << "failed to get transaction from daemon";
    return true;
  }
  currency::blobdata tx_data;
  bool ok;
  if (res.txs_as_hex.size() == 1)
    ok = string_tools::parse_hexstr_to_binbuff(res.txs_as_hex.front(), tx_data);
  else
    ok = string_tools::parse_hexstr_to_binbuff(res.txs_as_hex.front(), tx_data);
  if (!ok)
  {
    fail_msg_writer() << "failed to parse transaction from daemon";
    return true;
  }
  crypto::hash tx_hash, tx_prefix_hash;
  currency::transaction tx;
  if (!currency::parse_and_validate_tx_from_blob(tx_data, tx, tx_hash, tx_prefix_hash))
  {
    fail_msg_writer() << "failed to validate transaction from daemon";
    return true;
  }
  if (tx_hash != txid)
  {
    fail_msg_writer() << "failed to get the right transaction from daemon";
    return true;
  }

  uint64_t received = 0;
  try {
    for (size_t n = 0; n < tx.vout.size(); ++n)
    {
      if (typeid(txout_to_key) != tx.vout[n].target.type())
        continue;
      const txout_to_key tx_out_to_key = boost::get<txout_to_key>(tx.vout[n].target);
      crypto::public_key pubkey;
      derive_public_key(derivation, n, address.m_spend_public_key, pubkey);
      if (pubkey == tx_out_to_key.key)
      {
          uint64_t amount = 0;
        if (tx.version == 1)
        {
          amount = tx.vout[n].amount;
        }
        else
        {
            LOG_ERROR("Invalid transaction version: " << tx.version);
        }
        received += amount;
      }
    }
  }
  catch(const std::exception &e)
  {
    LOG_ERROR("error: " << e.what());
    fail_msg_writer() << "error: " << e.what();
    return true;
  }

  if (received > 0)
  {
    success_msg_writer() << get_account_address_as_str(address) << " " << "received" << " " << print_money(received) << " " << "in txid" << " " << txid;
  }
  else
  {
    fail_msg_writer() << get_account_address_as_str(address) << " " << "received nothing in txid" << " " << txid;
  }
  /* XXX TODO 
  if (res.txs.front().in_pool)
  {
    success_msg_writer() << "WARNING: this transaction is not yet included in the blockchain!";
  }
  else
  {

    std::string err;
    uint64_t bc_height = get_daemon_blockchain_height(err);
    if (err.empty())
    {
      uint64_t confirmations = bc_height - (res.txs.front().block_height + 1);
      success_msg_writer() << boost::format("This transaction has %u confirmations") % confirmations;
    }
    else
    {
      success_msg_writer() << "WARNING: failed to determine number of confirmations!";
    }
  }
  */

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::integrated_address(const std::vector<std::string> &args)
{
  if (args.size() > 1)
  {
    fail_msg_writer() << "usage: integrated_address [payment_id|integrated_address]";
    return true;
  }

  account_public_address addr = AUTO_VAL_INIT(addr);
  payment_id_t payment_id, integrated_payment_id;

  if (args.size() == 0)
  {
    char random_pid[DEFAULT_PAYMENT_ID_SIZE_BYTES];
    crypto::generate_random_bytes(sizeof random_pid, random_pid);
    payment_id = random_pid;
    success_msg_writer() << "Generated random payment id: " << epee::string_tools::buff_to_hex_nodelimer(payment_id);
  }
  else if (args.size() == 1)
  {
    if (currency::get_account_address_and_payment_id_from_str(addr, integrated_payment_id, args[0]))
    {
      if (integrated_payment_id.empty())
      {
        success_msg_writer() << args[0] << " is a standard address with no payment id encoded";
      }
      else
      {
        success_msg_writer() << args[0] << " is an integrated address:" << ENDL <<
          "  encoded payment id:    " << epee::string_tools::buff_to_hex_nodelimer(integrated_payment_id) << ENDL <<
          "  standard address:      " << currency::get_account_address_as_str(addr);
      }
      return true;
    }

    // arg[0] is not an address, tread it as hex-encoded payment id
    if (!epee::string_tools::parse_hexstr_to_binbuff(args[0], payment_id))
    {
      fail_msg_writer() << args[0] << " is invalid payment id. A hex-encoded string is expected.";
      return true;
    }
  }

  // encode payment_id into current wallet's public address
  std::string integrated_addr_str = currency::get_account_address_as_str(m_wallet->get_account().get_keys().m_account_address, payment_id);

  success_msg_writer() <<
    "    encoded payment id:    " << epee::string_tools::buff_to_hex_nodelimer(payment_id) << ENDL <<
    "    your standard address: " << m_wallet->get_account().get_public_address_str();
  
  success_msg_writer(true) << "your integrated address is " << integrated_addr_str;

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::run()
{
  std::string addr_start = m_wallet->get_account().get_public_address_str().substr(0, 6);
  return m_cmd_binder.run_handling("[" CURRENCY_NAME_BASE " wallet " + addr_start + "]: ", "");
}
//----------------------------------------------------------------------------------------------------
void simple_wallet::stop()
{
  m_cmd_binder.stop_handling();
  m_wallet->stop();
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::print_address(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  success_msg_writer() << m_wallet->get_account().get_public_address_str();
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::process_command(const std::vector<std::string> &args)
{
  return m_cmd_binder.process_command_vec(args);
}
//----------------------------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
#ifdef WIN32
  _CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

  //TRY_ENTRY();

  string_tools::set_module_name_and_folder(argv[0]);

  po::options_description desc_general("General options");
  command_line::add_arg(desc_general, command_line::arg_help);
  command_line::add_arg(desc_general, command_line::arg_version);

  po::options_description desc_params("Wallet options");
  command_line::add_arg(desc_params, arg_wallet_file);
  command_line::add_arg(desc_params, arg_generate_new_wallet);
  command_line::add_arg(desc_params, arg_restore_wallet);
  command_line::add_arg(desc_params, arg_restore_seed);
  command_line::add_arg(desc_params, arg_password);
  command_line::add_arg(desc_params, arg_daemon_address);
  command_line::add_arg(desc_params, arg_daemon_host);
  command_line::add_arg(desc_params, arg_daemon_port);
  command_line::add_arg(desc_params, arg_command);
  command_line::add_arg(desc_params, arg_log_level);
  tools::wallet_rpc_server::init_options(desc_params);

  po::positional_options_description positional_options;
  positional_options.add(arg_command.name, -1);

  po::options_description desc_all;
  desc_all.add(desc_general).add(desc_params);
  currency::simple_wallet w;
  po::variables_map vm;
  bool r = command_line::handle_error_helper(desc_all, [&]()
  {
    po::store(command_line::parse_command_line(argc, argv, desc_general, true), vm);

    if (command_line::get_arg(vm, command_line::arg_help))
    {
      success_msg_writer() << "Usage: simplewallet [--wallet-file=<file>|--generate-new-wallet=<file>|--restore-wallet=<file> --restore-seed=<24-word seed>] [--daemon-address=<host>:<port>] [<COMMAND>]";
      success_msg_writer() << desc_all << '\n' << w.get_commands_str();
      return false;
    }
    else if (command_line::get_arg(vm, command_line::arg_version))
    {
      success_msg_writer() << CURRENCY_NAME << " wallet v" << PROJECT_VERSION_LONG;
      return false;
    }

    auto parser = po::command_line_parser(argc, argv).options(desc_params).positional(positional_options);
    po::store(parser.run(), vm);
    po::notify(vm);
    return true;
  });
  if (!r)
    return 1;

  //set up logging options
  log_space::get_set_log_detalisation_level(true, LOG_LEVEL_2);
  //log_space::log_singletone::add_logger(LOGGER_CONSOLE, NULL, NULL, LOG_LEVEL_0);
  log_space::log_singletone::add_logger(LOGGER_FILE,
    log_space::log_singletone::get_default_log_file().c_str(),
    log_space::log_singletone::get_default_log_folder().c_str(), LOG_LEVEL_4);

  message_writer(epee::log_space::console_color_white, true) << CURRENCY_NAME << " wallet v" << PROJECT_VERSION_LONG;

  if(command_line::has_arg(vm, arg_log_level))
  {
    LOG_PRINT_L0("Setting log level = " << command_line::get_arg(vm, arg_log_level));
    log_space::get_set_log_detalisation_level(true, command_line::get_arg(vm, arg_log_level));
  }
  
  if(command_line::has_arg(vm, tools::wallet_rpc_server::arg_rpc_bind_port))
  {
    log_space::log_singletone::add_logger(LOGGER_CONSOLE, NULL, NULL, LOG_LEVEL_2);
    //runs wallet with rpc interface 
    if(!command_line::has_arg(vm, arg_wallet_file) )
    {
      LOG_ERROR("Wallet file not set.");
      return 1;
    }
    if(!command_line::has_arg(vm, arg_daemon_address) )
    {
      LOG_ERROR("Daemon address not set.");
      return 1;
    }
    if(!command_line::has_arg(vm, arg_password) )
    {
      LOG_ERROR("Wallet password not set.");
      return 1;
    }

    std::string wallet_file     = command_line::get_arg(vm, arg_wallet_file);    
    std::string wallet_password = command_line::get_arg(vm, arg_password);
    std::string daemon_address  = command_line::get_arg(vm, arg_daemon_address);
    std::string daemon_host = command_line::get_arg(vm, arg_daemon_host);
    int daemon_port = command_line::get_arg(vm, arg_daemon_port);
    if (daemon_host.empty())
      daemon_host = "localhost";
    if (!daemon_port)
      daemon_port = RPC_DEFAULT_PORT;
    if (daemon_address.empty())
      daemon_address = std::string("http://") + daemon_host + ":" + std::to_string(daemon_port);

    tools::wallet2 wal;
    try
    {
      LOG_PRINT_L0("Loading wallet...");
      wal.load(wallet_file, wallet_password);
      wal.init(daemon_address);
      wal.refresh();
      LOG_PRINT_GREEN("Loaded ok", LOG_LEVEL_0);
    }
    catch (const std::exception& e)
    {
      LOG_ERROR("Wallet initialize failed: " << e.what());
      return 1;
    }
    tools::wallet_rpc_server wrpc(wal);
    bool r = wrpc.init(vm);
    CHECK_AND_ASSERT_MES(r, 1, "Failed to initialize wallet rpc server");

    tools::signal_handler::install([&wrpc, &wal] {
      wrpc.send_stop_signal();
      wal.store();
    });
    LOG_PRINT_L0("Starting wallet rpc server");
    wrpc.run();
    LOG_PRINT_L0("Stopped wallet rpc server");
    try
    {
      LOG_PRINT_L0("Storing wallet...");
      wal.store();
      LOG_PRINT_GREEN("Stored ok", LOG_LEVEL_0);
    }
    catch (const std::exception& e)
    {
      LOG_ERROR("Failed to store wallet: " << e.what());
      return 1;
    }
  }else
  {
    //runs wallet with console interface 
    r = w.init(vm);
    CHECK_AND_ASSERT_MES(r, 1, "Failed to initialize wallet");

    std::vector<std::string> command = command_line::get_arg(vm, arg_command);
    if (!command.empty())
    {
      w.process_command(command);
      w.stop();
      w.deinit();
    }
    else
    {
      tools::signal_handler::install([&w] {
        w.stop();
      });
      w.run();

      w.deinit();
    }
  }
  return 1;
  //CATCH_ENTRY_L0("main", 1);
}
