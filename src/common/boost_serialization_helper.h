// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/stream_buffer.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/device/back_inserter.hpp>

#define CHECK_PROJECT_NAME()    std::string project_name = CURRENCY_NAME; ar & project_name;  if(project_name != CURRENCY_NAME) {throw std::runtime_error(std::string("wrong storage file: project name in file: ") + project_name + ", expected: " + CURRENCY_NAME );}


namespace tools
{
  template<class t_object>
  bool serialize_obj_to_file(t_object& obj, const std::string& file_path)
  {
    TRY_ENTRY();
    std::ofstream data_file;
    data_file.open( file_path , std::ios_base::binary | std::ios_base::out| std::ios::trunc);
    if(data_file.fail())
      return false;

    boost::archive::binary_oarchive a(data_file);
    a << obj;

    return !data_file.fail();
    CATCH_ENTRY_L0("serialize_obj_to_file", false);
  }

  template<class t_object>
  bool unserialize_obj_from_file(t_object& obj, const std::string& file_path)
  {
    TRY_ENTRY();

    std::ifstream data_file;  
    data_file.open( file_path, std::ios_base::binary | std::ios_base::in);
    if(data_file.fail())
      return false;
    boost::archive::binary_iarchive a(data_file);

    a >> obj;
    return !data_file.fail();
    CATCH_ENTRY_L0("unserialize_obj_from_file", false);
  }

  template<class t_object>
  bool unserialize_obj_from_buff(t_object& obj, const std::string& buff)

  {
    TRY_ENTRY();
    //unserialize fro buff
    // wrap buffer inside a stream and deserialize serial_str into obj
    boost::iostreams::basic_array_source<char> device(buff.data(), buff.size());
    boost::iostreams::stream<boost::iostreams::basic_array_source<char> > s(device);
    boost::archive::binary_iarchive ia(s);
    ia >> obj;
    return true;
    CATCH_ENTRY_L0("unserialize_obj_from_buff", false);
  }

  template<class t_object>
  bool serialize_obj_to_buff(t_object& obj, std::string& buff)
  {
    TRY_ENTRY();
    
    boost::iostreams::back_insert_device<std::string> inserter(buff);
    boost::iostreams::stream<boost::iostreams::back_insert_device<std::string> > s(inserter);
    boost::archive::binary_oarchive a(s);
    a << obj;
    return true;
    CATCH_ENTRY_L0("serialize_obj_to_file", false);
  }


}
