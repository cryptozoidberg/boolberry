// Copyright (c) 2012-2018 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <boost/program_options.hpp>

#include "net/http_client.h"
#include "md5_l.h"

//TODO: this will work only for lmdb 
#define LMDB_DATA_FILE_NAME       "data.mdb"

namespace tools
{
  struct pre_download_entrie
  {
    std::string url;
    std::string md5;
    uint64_t packed_size;
    uint64_t unpacked_size;
  };

  const static pre_download_entrie pre_download = { "http://88.99.193.104/downloads/data.mdb.pak", "8145a1817b5bfe60f68165681681e611", 4874525210, 6718447616 };

  template<class callback_t>
  bool process_predownload(const po::variables_map& vm, callback_t is_stop)
  {

    std::string config_folder = command_line::get_arg(vm, command_line::arg_data_dir);
    std::string working_folder = config_folder + "/" CURRENCY_BLOCKCHAINDATA_FOLDERNAME;
    boost::system::error_code ec;
    uint64_t sz = boost::filesystem::file_size(working_folder + "/" LMDB_DATA_FILE_NAME, ec);
    if (!(ec || pre_download.unpacked_size > sz * 2 || command_line::has_arg(vm, command_line::arg_explicit_predownload)) )
    {
      LOG_PRINT_MAGENTA("Pre-download not needed(db size=" << sz << ")", LOG_LEVEL_0);
      return true;
    }

    std::string local_path = working_folder + "/" LMDB_DATA_FILE_NAME ".download";

    LOG_PRINT_MAGENTA("Trying to download blockchain database file from " << pre_download.url << "...", LOG_LEVEL_0);
    epee::net_utils::http::interruptible_http_client cl;

    //MD5 stream calculator
    md5::MD5_CTX md5_state = AUTO_VAL_INIT(md5_state);
    md5::MD5Init(&md5_state);


    auto cb = [&](const std::string& buff, uint64_t total_bytes, uint64_t received_bytes)
    {
      std::cout << "Recieved " << received_bytes << " from " << total_bytes << "\r";
      nodetool::i_p2p_endpoint<currency::t_currency_protocol_handler<currency::core>::connection_context>* ptr = &p2psrv;
      if (is_stop())
      {
        LOG_PRINT_MAGENTA(ENDL << "Interrupting download", LOG_LEVEL_0);
        return false;
      }

      md5::MD5Update(&md5_state, reinterpret_cast<const unsigned char *>(buff.data()), buff.size());
      return true;
    };
    bool r = cl.download_and_unzip(cb, local_path, pre_download.url, 1000);
    if (!r)
    {
      LOG_PRINT_RED("Download failed", LOG_LEVEL_0);
      return false;
    }

    unsigned char md5_signature[16] = { 0 };
    md5::MD5Final(md5_signature, &md5_state);
    if (epee::string_tools::pod_to_hex(md5_signature) != pre_download.md5)
    {
      LOG_ERROR("Md5 missmatch in downloaded file: " << epee::string_tools::pod_to_hex(md5_signature) << ", expected: " << pre_download.md5);
      return false;
    }

    LOG_PRINT_GREEN("Download succssed, md5 " << pre_download.md5 << " confirmed." , LOG_LEVEL_0);
    if (!command_line::has_arg(vm, command_line::arg_validate_predownload))
    {
      boost::filesystem::remove(working_folder + "/" LMDB_DATA_FILE_NAME, ec);
      boost::filesystem::rename(local_path, working_folder + "/" LMDB_DATA_FILE_NAME, ec);
      if (ec)
      {
        LOG_ERROR("Failed to rename pre-downloaded blockchain database file");
        return false;
      }
      LOG_PRINT_GREEN("Blockchain replaced with new pre-downloaded data file.", LOG_LEVEL_0);
      return true;
    }

    //paranoid mode
    //move downloaded blockchain into temporary folder
    std::string path_to_temp_blockchain = config_folder + "/TEMP/" CURRENCY_BLOCKCHAINDATA_FOLDERNAME;
    std::string path_to_temp_blockchain_file = "/" LMDB_DATA_FILE_NAME;
    tools::create_directories_if_necessary(path_to_temp_blockchain);
    boost::filesystem::rename(local_path, path_to_temp_blockchain_file, ec);
    if (!ec)
    {
      LOG_ERROR("Failed to rename pre-downloaded blockchain database file from " << local_path << " to " << path_to_temp_blockchain_file);
      return false;
    }

    //remove old files
    boost::filesystem::remove_all(config_folder + "/" CURRENCY_BLOCKCHAINDATA_FOLDERNAME, ec);
    boost::filesystem::remove(config_folder + "/" CURRENCY_BLOCKCHAINDATA_SCRATCHPAD_CACHE, ec);
    boost::filesystem::remove(config_folder + "/" CURRENCY_POOLDATA_FILENAME, ec);

    currency::core source_core(nullptr);
    po::variables_map source_core_vm;
    source_core_vm.insert(std::make_pair(DATA_DIR_PARAM_NAME, boost::program_options::variable_value(path_to_temp_blockchain, false)));
    r = source_core.init(source_core_vm);
    CHECK_AND_ASSERT_MES(r, false, "Failed to init source core");

    po::variables_map vm_with_fast_sync(vm);
    vm_with_fast_sync.insert(std::make_pair("db-sync-mode", boost::program_options::variable_value(std::string("fast"), false)));

    currency::core target_core(nullptr);

    r = target_core.init(vm_with_fast_sync);
    CHECK_AND_ASSERT_MES(r, false, "Failed to init target core");

    CHECK_AND_ASSERT_MES(target_core.get_current_blockchain_height() == 1, false, "Target blockchain initialized not empty");
    uint64_t total_blocks = source_core.get_current_blockchain_height();

    LOG_PRINT_GREEN("Manually processing blocks from 1 to " << total_blocks << "...");

    for (uint64_t i = 1; i != total_blocks; i++)
    { 
      std::list<block> blocks;
      std::list<transaction> txs;
      bool r = source_core.get_blocks(i, 1, blocks, txs);
      CHECK_AND_ASSERT_MES(r && blocks.size()==1, false, "Filed to get block " << i << " from core");
      currency::tx_verification_context tvc = AUTO_VAL_INIT(tvc);
      for (auto& tx : txs)
      {
        r = target_core.add_new_tx(tx, tvc, true);
        CHECK_AND_ASSERT_MES(r && tvc.m_added_to_pool == true, false, "Filed to get block " << i << " from core");
      }
      currency::block_verification_context bvc = AUTO_VAL_INIT(bvc);
      r = target_core.add_new_block(*blocks.begin(), bvc);
      CHECK_AND_ASSERT_MES(r && bvc.m_added_to_main_chain == true, false, "Filed to add block " << i << " to core");
      if (i % 100)
        std::cout << "Block " << i << "(" << (i * 100) / total_blocks << "%) \r";
    }
    
    LOG_PRINT_GREEN("Processing finished, " << total_blocks << " successfully added.");
    target_core.deinit();
    source_core.deinit();
    return true;
  }
}
