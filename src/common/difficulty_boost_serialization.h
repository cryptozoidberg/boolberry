// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <boost/multiprecision/cpp_int.hpp>

namespace boost
{
  namespace serialization
  {
    //---------------------------------------------------
    template <class archive_t>
    inline void serialize(archive_t &a, currency::wide_difficulty_type &x, const boost::serialization::version_type ver)
    {
      if(archive_t::is_loading::value)
      {
        //load high part
        uint64_t v = 0;        
        a & v; 
        x = v;
        //load low part
        x = x << 64;
        a & v;
        x += v;
      }else
      {
        std::cout << "storing" << ENDL;
        //store high part
        currency::wide_difficulty_type x_ = x;
        x_ = x_ >> 64;
        uint64_t v = x_.convert_to<uint64_t>();
        a & v;         
        //store low part
        x_ = x;
        x_ << 64;
        x_ >> 64;
        v = x_.convert_to<uint64_t>();
        a & v;
      }      
    }
  }
}
