// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <boost/variant.hpp>
#include <boost/functional/hash/hash.hpp>
#include <vector>
#include <cstring>  // memcmp
#include <sstream>
#include "serialization/crypto.h"
#include "serialization/vector.h"
#include "serialization/serialization.h"
#include "serialization/variant.h"
#include "serialization/binary_archive.h"
#include "serialization/json_archive.h"
#include "serialization/debug_archive.h"
#include "serialization/keyvalue_serialization.h" // epee key-value serialization
#include "string_tools.h"
#include "currency_config.h"
#include "crypto/crypto.h"
#include "crypto/hash.h"
#include "misc_language.h"
#include "tx_extra.h"
#include "block_flags.h"

namespace currency
{

  const static crypto::hash null_hash = AUTO_VAL_INIT(null_hash);
  const static crypto::public_key null_pkey = AUTO_VAL_INIT(null_pkey);
  const static crypto::secret_key null_skey = AUTO_VAL_INIT(null_skey);
  const static crypto::signature null_sig = AUTO_VAL_INIT(null_sig);


  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  struct account_public_address
  {
    crypto::public_key m_spend_public_key;
    crypto::public_key m_view_public_key;

    BEGIN_SERIALIZE_OBJECT()
      FIELD(m_spend_public_key)
      FIELD(m_view_public_key)
      END_SERIALIZE()

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_VAL_POD_AS_BLOB_FORCE(m_spend_public_key)
        KV_SERIALIZE_VAL_POD_AS_BLOB_FORCE(m_view_public_key)
      END_KV_SERIALIZE_MAP()
  };


  typedef std::vector<crypto::signature> ring_signature;

  /* outputs */
  struct txout_to_script
  {
    std::vector<crypto::public_key> keys;
    std::vector<uint8_t> script;

    BEGIN_SERIALIZE_OBJECT()
      FIELD(keys)
      FIELD(script)
    END_SERIALIZE()
  };

  struct txout_to_scripthash
  {
    crypto::hash hash;
  };

  #pragma pack(push, 1)
  struct txout_to_key
  {
    txout_to_key() { }
    txout_to_key(const crypto::public_key &_key) : key(_key) { }

    crypto::public_key key;
    uint8_t mix_attr;
  };
  #pragma pack(pop)
  /* inputs */

  struct txin_gen
  {
    size_t height;

    BEGIN_SERIALIZE_OBJECT()
      VARINT_FIELD(height)
    END_SERIALIZE()
  };

  struct txin_to_script
  {
    crypto::hash prev;
    size_t prevout;
    std::vector<uint8_t> sigset;

    BEGIN_SERIALIZE_OBJECT()
      FIELD(prev)
      VARINT_FIELD(prevout)
      FIELD(sigset)
    END_SERIALIZE()
  };

  struct txin_to_scripthash
  {
    crypto::hash prev;
    size_t prevout;
    txout_to_script script;
    std::vector<uint8_t> sigset;

    BEGIN_SERIALIZE_OBJECT()
      FIELD(prev)
      VARINT_FIELD(prevout)
      FIELD(script)
      FIELD(sigset)
    END_SERIALIZE()
  };

  struct txin_to_key
  {
    uint64_t amount;
    std::vector<uint64_t> key_offsets;
    crypto::key_image k_image;      // double spending protection

    BEGIN_SERIALIZE_OBJECT()
      VARINT_FIELD(amount)
      FIELD(key_offsets)
      FIELD(k_image)
    END_SERIALIZE()
  };


  typedef boost::variant<txin_gen, txin_to_script, txin_to_scripthash, txin_to_key> txin_v;

  typedef boost::variant<txout_to_script, txout_to_scripthash, txout_to_key> txout_target_v;

  //typedef std::pair<uint64_t, txout> out_t;
  struct tx_out
  {
    uint64_t amount;
    txout_target_v target;

    BEGIN_SERIALIZE_OBJECT()
      VARINT_FIELD(amount)
      FIELD(target)
    END_SERIALIZE()
  };

#define OFFER_TYPE_LUI_TO_ETC   0
#define OFFER_TYPE_ETC_TO_LUI   1

  /************************************************************************/
  /* attachment structures                                                */
  /************************************************************************/
  struct offer_details
  {
    uint8_t offer_type;
    uint64_t amount_lui;       //amount of lui
    uint64_t amount_etc;       //amount of other currency or goods
    std::string target;        //[max 30 characters] currency / goods
    std::string location;      //[max 50 characters] geo-location
    std::string contacts;      //[max 140 characters] (Skype, mail, ICQ, etc., website)
    std::string comment;       //[max 160 characters]
    std::string payment_types; //[max 20 characters ]money accept type(bank transaction, internet money, cash, etc)
    uint8_t expiration_time;   //n-days

    BEGIN_SERIALIZE_OBJECT()
      VALUE(offer_type)
      VARINT_FIELD(amount_lui)
      VARINT_FIELD(amount_etc)
      VALUE(target)
      VALUE(location)
      VALUE(contacts)
      VALUE(comment)
      VALUE(payment_types)
      VALUE(expiration_time)
    END_SERIALIZE()
  };



  struct tx_comment
  {
    std::string comment;

    BEGIN_SERIALIZE()
      FIELD(comment)
      END_SERIALIZE()
  };

  struct tx_payer
  {
    account_public_address acc_addr;

    BEGIN_SERIALIZE()
      FIELD(comment)
      END_SERIALIZE()
  };

  typedef boost::variant<offer_details, tx_comment, tx_payer, std::string> attachment_v;

  /************************************************************************/
  /* extra structures                                                     */
  /************************************************************************/
  struct extra_attachment_info
  {
    uint64_t sz;
    crypto::hash hsh;

    BEGIN_SERIALIZE()
      VARINT_FIELD(sz)
      FIELD(hsh)
    END_SERIALIZE()
  };

  struct extra_user_data
  {
    std::vector<uint8_t> buff;
    
    BEGIN_SERIALIZE()
      FIELD(buff)
    END_SERIALIZE()
  };

  struct extra_alias_entry
  {
    std::vector<uint8_t> buff; //manual parse
    BEGIN_SERIALIZE()
      FIELD(buff)
    END_SERIALIZE()
  };

  struct extra_padding
  {
    std::vector<uint8_t> buff; //stub
    
    BEGIN_SERIALIZE()
      FIELD(buff)
    END_SERIALIZE()
  };

  typedef boost::variant<crypto::public_key, extra_attachment_info, extra_alias_entry, extra_user_data, extra_padding> extra_v;

  class transaction_prefix
  {

  public:
    // tx information
    size_t   version;
    uint64_t unlock_time;  //number of block (or time), used as a limitation like: spend this tx not early then block/time

    std::vector<txin_v> vin;
    std::vector<tx_out> vout;
    //extra
    std::vector<extra_v> extra;

    BEGIN_SERIALIZE()
      VARINT_FIELD(version)
      if(CURRENT_TRANSACTION_VERSION < version) return false;
      VARINT_FIELD(unlock_time)
      FIELD(vin)
      FIELD(vout)
      FIELD(extra)
    END_SERIALIZE()

  protected:
    transaction_prefix(){}
  };


  class transaction: public transaction_prefix
  {
  public:
    std::vector<std::vector<crypto::signature> > signatures; //count signatures  always the same as inputs count
    std::vector<attachment_v> attachment;

    transaction();
    //virtual ~transaction();
    //void set_null();

    BEGIN_SERIALIZE_OBJECT()
      FIELDS(*static_cast<transaction_prefix *>(this))
      FIELD(signatures)
      FIELD(attachment)
    END_SERIALIZE()


    static size_t get_signature_size(const txin_v& tx_in);
  };

  
  inline
  transaction::transaction()
  {
    version = 0;
    unlock_time = 0;
    vin.clear();
    vout.clear();
    extra.clear();
    signatures.clear();
    attachment.clear();
  }
  /*
  inline
  transaction::~transaction()
  {
    //set_null();
  }

  inline
  void transaction::set_null()
  {
    version = 0;
    unlock_time = 0;
    vin.clear();
    vout.clear();
    extra.clear();
    signatures.clear();
  }
  */
  inline
  size_t transaction::get_signature_size(const txin_v& tx_in)
  {
    struct txin_signature_size_visitor : public boost::static_visitor<size_t>
    {
      size_t operator()(const txin_gen& /*txin*/) const{return 0;}
      size_t operator()(const txin_to_script& /*txin*/) const{return 0;}
      size_t operator()(const txin_to_scripthash& /*txin*/) const{return 0;}
      size_t operator()(const txin_to_key& txin) const {return txin.key_offsets.size();}
    };

    return boost::apply_visitor(txin_signature_size_visitor(), tx_in);
  }




  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  struct block_header
  {
    uint8_t major_version;
    uint8_t minor_version;
    uint64_t timestamp;
    crypto::hash  prev_id;
    uint64_t nonce;
    uint8_t flags;

    BEGIN_SERIALIZE()
      FIELD(major_version)
      if(major_version > CURRENT_BLOCK_MAJOR_VERSION) return false;
      FIELD(nonce)
      FIELD(prev_id)
      VARINT_FIELD(minor_version)
      VARINT_FIELD(timestamp)
      FIELD(flags)
    END_SERIALIZE()
  };

  struct block: public block_header
  {
    transaction miner_tx;
    std::vector<crypto::hash> tx_hashes;
    crypto::signature pos_sig;
    
    BEGIN_SERIALIZE_OBJECT()
      FIELDS(*static_cast<block_header *>(this))
      FIELD(miner_tx)
      FIELD(tx_hashes)
      if (this->flags&CURRENCY_BLOCK_FLAG_POS_BLOCK)
      {
        FIELD(pos_sig)
      }
    END_SERIALIZE()
  };




  struct keypair
  {
    crypto::public_key pub;
    crypto::secret_key sec;

    static inline keypair generate()
    {
      keypair k;
      generate_keys(k.pub, k.sec);
      return k;
    }
  };
  //---------------------------------------------------------------
  //PoS
  //based from ppcoin/novacoin approach

  /*
  POS PROTOCOL, stake modifier

  - The kernel structure now includes last_pow_hash and last_pos_hash: blockID of the last
  PoW block and kernel hash of the last PoS block respectively. By that we create the true 
  chain of blocks. In PPC you can use your winning output whenever you want (within timestamp 
  boundaries), because kernel does not depend of the previous block (unlike PoW mining,
  where block header includes prev_id). Even use it twice (if a chain fork occurs). Our 
  protocol prevents it, making PoS mining more like PoW (but energy-efficient).

  - StakeModifier (a part of the kernel) is recalculated on each PoW block as follows: 
  StakeModifier on height h is equal to keccak(H20,H720), where H20 is the proof_hash of 
  the block on height h-20 (20 blocks ago) and H720 is the proof_hash of the block on height 
  h-720. proof_hash is blockID (for PoW) or kernel-hash (for PoS). Now StakeModifier is known 
  in advance, 20 blocks ahead. But due to the first change an attacker can't predict his 
  chances of creating consecutive chain of PoS blocks: he doesn't know pow_hash.

  - Timestamp boundaries become more narrow. Timestamp must be greater than the median of 
  the last 20 blocks (each block appears ~every 2 min, so the median is t minus 20 minutes).
  And it shouldn't be greater than t+20 minutes, where t is the current local time of a node. 
  The first rule is a mandatory one (we check it while verifying the blockchain), and the 
  second is a default local node policy (we can not check it for blockchain in the past). 
  This change is intended to decrease the power of PoS exhaustive search and make timestamps 
  more consistent.
  */

  typedef crypto::hash stake_modifier_type;
#pragma pack(push, 1)
  struct stake_kernel
  {
    stake_modifier_type stake_modifier;
    uint64_t block_timestamp;             //this block timestamp
    crypto::key_image kimage;
    uint64_t tx_out_global_index;         //global index of output in source transaction
    crypto::hash last_pow_id;
    crypto::hash last_pos_kernel_hash;
  };
#pragma pack(pop)

  struct pos_entry
  {
    uint64_t amount;
    uint64_t index;
    crypto::key_image keyimage;
    uint64_t block_timestamp;
    //not for serialization
    uint64_t wallet_index;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(amount)
      KV_SERIALIZE(index)
      KV_SERIALIZE(block_timestamp)
      KV_SERIALIZE_VAL_POD_AS_BLOB_FORCE(keyimage)
    END_KV_SERIALIZE_MAP()
  };
}

BLOB_SERIALIZER(currency::txout_to_key);
BLOB_SERIALIZER(currency::txout_to_scripthash);


VARIANT_TAG(binary_archive, currency::txin_gen, 0xff);
VARIANT_TAG(binary_archive, currency::txin_to_script, 0x0);
VARIANT_TAG(binary_archive, currency::txin_to_scripthash, 0x1);
VARIANT_TAG(binary_archive, currency::txin_to_key, 0x2);
VARIANT_TAG(binary_archive, currency::txout_to_script, 0x0);
VARIANT_TAG(binary_archive, currency::txout_to_scripthash, 0x1);
VARIANT_TAG(binary_archive, currency::txout_to_key, 0x2);
VARIANT_TAG(binary_archive, currency::transaction, 0xcc);
VARIANT_TAG(binary_archive, currency::block, 0xbb);

VARIANT_TAG(binary_archive, currency::offer_details, 0x0);
VARIANT_TAG(binary_archive, currency::tx_comment, 0x1);
VARIANT_TAG(binary_archive, currency::tx_payer, 0x2);
VARIANT_TAG(binary_archive, std::string, 0x3);





VARIANT_TAG(json_archive, currency::txin_gen, "gen");
VARIANT_TAG(json_archive, currency::txin_to_script, "script");
VARIANT_TAG(json_archive, currency::txin_to_scripthash, "scripthash");
VARIANT_TAG(json_archive, currency::txin_to_key, "key");
VARIANT_TAG(json_archive, currency::txout_to_script, "script");
VARIANT_TAG(json_archive, currency::txout_to_scripthash, "scripthash");
VARIANT_TAG(json_archive, currency::txout_to_key, "key");
VARIANT_TAG(json_archive, currency::transaction, "tx");
VARIANT_TAG(json_archive, currency::block, "block");

VARIANT_TAG(json_archive, currency::offer_details, "offer");
VARIANT_TAG(json_archive, currency::tx_comment, "comment");
VARIANT_TAG(json_archive, currency::tx_payer, "payer");
VARIANT_TAG(json_archive, std::string, "string");




VARIANT_TAG(debug_archive, currency::txin_gen, "gen");
VARIANT_TAG(debug_archive, currency::txin_to_script, "script");
VARIANT_TAG(debug_archive, currency::txin_to_scripthash, "scripthash");
VARIANT_TAG(debug_archive, currency::txin_to_key, "key");
VARIANT_TAG(debug_archive, currency::txout_to_script, "script");
VARIANT_TAG(debug_archive, currency::txout_to_scripthash, "scripthash");
VARIANT_TAG(debug_archive, currency::txout_to_key, "key");
VARIANT_TAG(debug_archive, currency::transaction, "tx");
VARIANT_TAG(debug_archive, currency::block, "block");

VARIANT_TAG(debug_archive, currency::offer_details, "offer");
VARIANT_TAG(debug_archive, currency::tx_comment, "comment");
VARIANT_TAG(debug_archive, currency::tx_payer, "payer");
VARIANT_TAG(debug_archive, std::string, "string");
