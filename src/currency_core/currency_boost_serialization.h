// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <boost/serialization/vector.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/variant.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/map.hpp>
#include <boost/foreach.hpp>
#include <boost/serialization/is_bitwise_serializable.hpp>
#include "currency_basic.h"
#include "common/unordered_containers_boost_serialization.h"
#include "common/crypto_boost_serialization.h"

#define CURRENT_BLOCK_ARCHIVE_VER   2
BOOST_CLASS_VERSION(currency::block, CURRENT_BLOCK_ARCHIVE_VER)

namespace boost
{
  namespace serialization
  {

  template <class Archive>
  inline void serialize(Archive &a, currency::txout_to_script &x, const boost::serialization::version_type ver)
  {
    a & x.keys;
    a & x.script;
  }


  template <class Archive>
  inline void serialize(Archive &a, currency::txout_to_key &x, const boost::serialization::version_type ver)
  {
    a & x.key;
    a & x.mix_attr;
  }

  template <class Archive>
  inline void serialize(Archive &a, currency::txout_to_scripthash &x, const boost::serialization::version_type ver)
  {
    a & x.hash;
  }

  template <class Archive>
  inline void serialize(Archive &a, currency::txin_gen &x, const boost::serialization::version_type ver)
  {
    a & x.height;
  }

  template <class Archive>
  inline void serialize(Archive &a, currency::txin_to_script &x, const boost::serialization::version_type ver)
  {
    a & x.prev;
    a & x.prevout;
    a & x.sigset;
  }

  template <class Archive>
  inline void serialize(Archive &a, currency::txin_to_scripthash &x, const boost::serialization::version_type ver)
  {
    a & x.prev;
    a & x.prevout;
    a & x.script;
    a & x.sigset;
  }

  template <class Archive>
  inline void serialize(Archive &a, currency::txin_to_key &x, const boost::serialization::version_type ver)
  {
    a & x.amount;
    a & x.key_offsets;
    a & x.k_image;
  }

  template <class Archive>
  inline void serialize(Archive &a, currency::tx_out &x, const boost::serialization::version_type ver)
  {
    a & x.amount;
    a & x.target;
  }


  template <class Archive>
  inline void serialize(Archive &a, currency::transaction &x, const boost::serialization::version_type ver)
  {
    a & x.version;
    a & x.unlock_time;
    a & x.vin;
    a & x.vout;
    a & x.extra;
    a & x.signatures;
  }
  template <class Archive>
  inline void serialize(Archive &a, currency::block &b, const boost::serialization::version_type ver)
  {
    if(ver < CURRENT_BLOCK_ARCHIVE_VER)
    {
      throw std::runtime_error("wrong block serialization version");
    }
    a & b.major_version;
    a & b.minor_version;
    a & b.timestamp;
    a & b.prev_id;
    a & b.nonce;
    //------------------
    a & b.miner_tx;
    a & b.tx_hashes;
    a & b.flags;
  }
}
}

//}
