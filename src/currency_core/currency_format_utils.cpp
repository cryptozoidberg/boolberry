// Copyright (c) 2012-2013 The Cryptonote developers
// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "include_base_utils.h"
#include <boost/foreach.hpp>
using namespace epee;

#include "currency_format_utils.h"
#include "serialization/binary_utils.h"
#include "serialization/vector.h"
#include "currency_config.h"
#include "miner.h"
#include "crypto/crypto.h"
#include "crypto/hash.h"
#include "common/int-util.h"
#include "common/base58.h"

namespace currency
{
  //---------------------------------------------------------------
  void get_transaction_prefix_hash(const transaction_prefix& tx, crypto::hash& h)
  {
    std::ostringstream s;
    binary_archive<true> a(s);
    ::serialization::serialize(a, const_cast<transaction_prefix&>(tx));
    crypto::cn_fast_hash(s.str().data(), s.str().size(), h);
  }
  //---------------------------------------------------------------
  crypto::hash get_transaction_prefix_hash(const transaction_prefix& tx)
  {
    crypto::hash h = null_hash;
    get_transaction_prefix_hash(tx, h);
    return h;
  }
  //---------------------------------------------------------------
  bool parse_and_validate_tx_from_blob(const blobdata& tx_blob, transaction& tx)
  {
    std::stringstream ss;
    ss << tx_blob;
    binary_archive<false> ba(ss);
    bool r = ::serialization::serialize(ba, tx);
    CHECK_AND_ASSERT_MES(r, false, "Failed to parse transaction from blob");
    return true;
  }
  //---------------------------------------------------------------
  bool parse_and_validate_tx_from_blob(const blobdata& tx_blob, transaction& tx, crypto::hash& tx_hash, crypto::hash& tx_prefix_hash)
  {
    std::stringstream ss;
    ss << tx_blob;
    binary_archive<false> ba(ss);
    bool r = ::serialization::serialize(ba, tx);
    CHECK_AND_ASSERT_MES(r, false, "Failed to parse transaction from blob");
    //TODO: validate tx

    //crypto::cn_fast_hash(tx_blob.data(), tx_blob.size(), tx_hash);
    get_transaction_prefix_hash(tx, tx_prefix_hash);
    tx_hash = tx_prefix_hash;
    return true;
  }
  //---------------------------------------------------------------
  /*
  bool construct_miner_tx(size_t height, size_t median_size, uint64_t already_generated_coins, 
                                                             size_t current_block_size, 
                                                             uint64_t fee, 
                                                             const account_public_address &miner_address, 
                                                             transaction& tx, 
                                                             const blobdata& extra_nonce, 
                                                             size_t max_outs)
  {

    alias_info alias = AUTO_VAL_INIT(alias);
    return construct_miner_tx(height, median_size, already_generated_coins, current_block_size, 
                                                                            fee, 
                                                                            miner_address, 
                                                                            tx, 
                                                                            extra_nonce, 
                                                                            max_outs,                                                                             
                                                                            alias, 
                                                                            false, 
                                                                            pos_entry());
  }*/
  //---------------------------------------------------------------
  uint64_t get_coinday_weight(uint64_t amount)
  {
    return amount;
  }
  //---------------------------------------------------------------
  wide_difficulty_type correct_difficulty_with_sequence_factor(size_t sequence_factor, wide_difficulty_type diff)
  {
    //delta=delta*(0.75^n)
    for (size_t i = 0; i != sequence_factor; i++)
    {
      diff = diff - diff / 4;
    }
    return diff;
  }
  //------------------------------------------------------------------
  bool construct_miner_tx(size_t height, size_t median_size, uint64_t already_generated_coins,
    const wide_difficulty_type pos_diff,
    size_t current_block_size,
    uint64_t fee,
    const account_public_address &miner_address,
    transaction& tx,
    const blobdata& extra_nonce,
    size_t max_outs,
    const alias_info& alias,
    bool pos,
    const pos_entry& pe)
  {
    uint64_t block_reward;
    if (!get_block_reward(median_size, current_block_size, already_generated_coins, block_reward, height, pos_diff))
    {
      LOG_PRINT_L0("Block is too big");
      return false;
    }
    block_reward += fee / 2;

    //send PoS entry back to owner
    if (pos)
    {
      block_reward += pe.amount;
    }

    std::vector<size_t> out_amounts;
    decompose_amount_into_digits(block_reward, DEFAULT_DUST_THRESHOLD,
      [&out_amounts](uint64_t a_chunk) { out_amounts.push_back(a_chunk); },
      [&out_amounts](uint64_t a_dust) { out_amounts.push_back(a_dust); });

    CHECK_AND_ASSERT_MES(1 <= max_outs, false, "max_out must be non-zero");
    while (max_outs < out_amounts.size())
    {
      out_amounts[out_amounts.size() - 2] += out_amounts.back();
      out_amounts.resize(out_amounts.size() - 1);
    }


    size_t summary_amounts = 0;
    std::vector<tx_destination_entry> destinations;
    for (auto a : out_amounts)
    {
      tx_destination_entry de;
      de.addr = miner_address;
      de.amount = a;
      destinations.push_back(de);
      summary_amounts += a;
    }

//     size_t summary_amounts = 0;
//     size_t no = 0;
//     for (; no < out_amounts.size(); no++)
//     {
//       bool r = construct_tx_out(miner_address, txkey.sec, no, out_amounts[no], tx);
//       CHECK_AND_ASSERT_MES(r, false, "Failed to contruct miner tx out");
//       summary_amounts += out_amounts[no];
//     }
// 
//     CHECK_AND_ASSERT_MES(summary_amounts == block_reward, false, "Failed to construct miner tx, summary_amounts = " << summary_amounts << " not equal block_reward = " << block_reward);
    return construct_miner_tx(height, median_size, already_generated_coins, current_block_size, fee, destinations, tx, extra_nonce, max_outs, alias, pos, pe);
  }
  //------------------------------------------------------------------
  bool construct_miner_tx(size_t height, size_t median_size, uint64_t already_generated_coins,
                                                             size_t current_block_size, 
                                                             uint64_t fee, 
                                                             const std::vector<tx_destination_entry>& destinations,
                                                             transaction& tx, 
                                                             const blobdata& extra_nonce, 
                                                             size_t max_outs, 
                                                             const alias_info& alias, 
                                                             bool pos,
                                                             const pos_entry& pe)
  {
    tx.vin.clear();
    tx.vout.clear();
    tx.extra.clear();

    keypair txkey = keypair::generate();
    add_tx_pub_key_to_extra(tx, txkey.pub);
    if(extra_nonce.size())
      if(!add_tx_extra_userdata(tx, extra_nonce))
        return false;
    if(alias.m_alias.size())
    {
      if(!add_tx_extra_alias(tx, alias))
        return false;
    }

    txin_gen in;
    in.height = height;
    tx.vin.push_back(in);

    if(pos)
    {
      txin_to_key posin;
      posin.amount = pe.amount;
      posin.key_offsets.push_back(pe.index);
      posin.k_image = pe.keyimage;
      tx.vin.push_back(posin);
      //reserve place for ring signature
      tx.signatures.resize(1);
      tx.signatures[0].resize(posin.key_offsets.size());
    }

    uint64_t no = 0;
    for (auto& d : destinations)
     {
      bool r = construct_tx_out(d.addr, txkey.sec, no, d.amount, tx);
       CHECK_AND_ASSERT_MES(r, false, "Failed to contruct miner tx out");
       no++;
     }


    tx.version = CURRENT_TRANSACTION_VERSION;
    //lock
    tx.unlock_time = height + CURRENCY_MINED_MONEY_UNLOCK_WINDOW;
    return true;
  }
  //---------------------------------------------------------------
  bool generate_key_image_helper(const account_keys& ack, const crypto::public_key& tx_public_key, size_t real_output_index, keypair& in_ephemeral, crypto::key_image& ki)
  {
    crypto::key_derivation recv_derivation = AUTO_VAL_INIT(recv_derivation);
    bool r = crypto::generate_key_derivation(tx_public_key, ack.m_view_secret_key, recv_derivation);
    CHECK_AND_ASSERT_MES(r, false, "key image helper: failed to generate_key_derivation(" << tx_public_key << ", " << ack.m_view_secret_key << ")");

    r = crypto::derive_public_key(recv_derivation, real_output_index, ack.m_account_address.m_spend_public_key, in_ephemeral.pub);
    CHECK_AND_ASSERT_MES(r, false, "key image helper: failed to derive_public_key(" << recv_derivation << ", " << real_output_index <<  ", " << ack.m_account_address.m_spend_public_key << ")");

    crypto::derive_secret_key(recv_derivation, real_output_index, ack.m_spend_secret_key, in_ephemeral.sec);

    crypto::generate_key_image(in_ephemeral.pub, in_ephemeral.sec, ki);
    return true;
  }
  //---------------------------------------------------------------
  uint64_t power_integral(uint64_t a, uint64_t b)
  {
    if(b == 0)
      return 1;
    uint64_t total = a;
    for(uint64_t i = 1; i != b; i++)
      total *= a;
    return total;
  }
  //---------------------------------------------------------------
  bool is_mixattr_applicable_for_fake_outs_counter(uint8_t mix_attr, uint64_t fake_attr_count)
  {
    if(mix_attr > 1)
      return fake_attr_count+1 >= mix_attr;
    else if(mix_attr == CURRENCY_TO_KEY_OUT_FORCED_NO_MIX)
      return fake_attr_count == 0;
    else 
      return true;
  }  
  //---------------------------------------------------------------
  bool parse_amount(uint64_t& amount, const std::string& str_amount_)
  {
    std::string str_amount = str_amount_;
    boost::algorithm::trim(str_amount);

    size_t point_index = str_amount.find_first_of('.');
    size_t fraction_size;
    if (std::string::npos != point_index)
    {
      fraction_size = str_amount.size() - point_index - 1;
      while (CURRENCY_DISPLAY_DECIMAL_POINT < fraction_size && '0' == str_amount.back())
      {
        str_amount.erase(str_amount.size() - 1, 1);
        --fraction_size;
      }
      if (CURRENCY_DISPLAY_DECIMAL_POINT < fraction_size)
        return false;
      str_amount.erase(point_index, 1);
    }
    else
    {
      fraction_size = 0;
    }

    if (str_amount.empty())
      return false;

    if (fraction_size < CURRENCY_DISPLAY_DECIMAL_POINT)
    {
      str_amount.append(CURRENCY_DISPLAY_DECIMAL_POINT - fraction_size, '0');
    }

    return string_tools::get_xtype_from_string(amount, str_amount);
  }
  //--------------------------------------------------------------------------------
  std::string print_stake_kernel_info(const stake_kernel& sk)
  {
    std::stringstream ss;
    ss << "block_timestampL " << sk.block_timestamp << ENDL
      << "kimage: " << sk.kimage << ENDL
      << "sk.stake_modifier.last_pos_kernel_id" << sk.stake_modifier.last_pos_kernel_id << ENDL
      << "sk.stake_modifier.last_pow_id" << sk.stake_modifier.last_pow_id << ENDL;
    return ss.str();
  }

  //---------------------------------------------------------------
  bool get_tx_fee(const transaction& tx, uint64_t & fee)
  {
    uint64_t amount_in = 0;
    uint64_t amount_out = 0;
    BOOST_FOREACH(auto& in, tx.vin)
    {
      CHECK_AND_ASSERT_MES(in.type() == typeid(txin_to_key), 0, "unexpected type id in transaction");
      amount_in += boost::get<txin_to_key>(in).amount;
    }
    BOOST_FOREACH(auto& o, tx.vout)
      amount_out += o.amount;

    CHECK_AND_ASSERT_MES(amount_in >= amount_out, false, "transaction spend (" <<amount_in << ") more than it has (" << amount_out << ")");
    fee = amount_in - amount_out;
    return true;
  }
  //---------------------------------------------------------------
  uint64_t get_tx_fee(const transaction& tx)
  {
    uint64_t r = 0;
    if(!get_tx_fee(tx, r))
      return 0;
    return r;
  }
  //---------------------------------------------------------------
  crypto::public_key get_tx_pub_key_from_extra(const transaction& tx)
  {
    crypto::public_key pk = null_pkey;
    parse_and_validate_tx_extra(tx, pk);
    return pk;
  }
  //---------------------------------------------------------------
  bool parse_and_validate_tx_extra(const transaction& tx, crypto::public_key& tx_pub_key)
  {
    tx_extra_info e = AUTO_VAL_INIT(e);
    bool r = parse_and_validate_tx_extra(tx, e);
    tx_pub_key = e.m_tx_pub_key;
    return r;
  }
  //---------------------------------------------------------------
  bool sign_update_alias(alias_info& ai, const crypto::public_key& pkey, const crypto::secret_key& skey)
  {
    std::string buf;
    bool r = make_tx_extra_alias_entry(buf, ai, true);
    CHECK_AND_ASSERT_MES(r, false, "failed to make_tx_extra_alias_entry");
    crypto::generate_signature(get_blob_hash(buf), pkey, skey, ai.m_sign);
    return true;
  }
  //---------------------------------------------------------------
  bool make_tx_extra_alias_entry(std::string& buff, const alias_info& alinfo, bool make_buff_to_sign)
  {
    CHECK_AND_ASSERT_MES(alinfo.m_alias.size(), false, "alias cant be empty");
    CHECK_AND_ASSERT_MES(alinfo.m_alias.size() <= std::numeric_limits<uint8_t>::max(), false, "alias is too big size");
    CHECK_AND_ASSERT_MES(alinfo.m_text_comment.size() <= std::numeric_limits<uint8_t>::max(), false, "alias comment is too big size");
    buff.resize(3);
    buff[0] = TX_EXTRA_TAG_ALIAS;
    buff[1] = 0;
    buff[2] = static_cast<uint8_t>(alinfo.m_alias.size());
    buff += alinfo.m_alias;
    string_tools::apped_pod_to_strbuff(buff, alinfo.m_address.m_spend_public_key);
    string_tools::apped_pod_to_strbuff(buff, alinfo.m_address.m_view_public_key);
    if(alinfo.m_view_key != null_skey)
    {
      buff[1] |= TX_EXTRA_TAG_ALIAS_FLAGS_ADDR_WITH_TRACK;
      string_tools::apped_pod_to_strbuff(buff, alinfo.m_view_key);
    }
    buff.resize(buff.size()+1);
    buff.back() = static_cast<uint8_t>(alinfo.m_text_comment.size());
    buff += alinfo.m_text_comment;
    if(!make_buff_to_sign && alinfo.m_sign != null_sig )
    {
      buff[1] |= TX_EXTRA_TAG_ALIAS_FLAGS_OP_UPDATE;
      string_tools::apped_pod_to_strbuff(buff, alinfo.m_sign);
    }
    return true;
  }
  //---------------------------------------------------------------
  bool make_tx_extra_alias_entry(std::vector<uint8_t>& buff, const alias_info& alinfo, bool make_buff_to_sign)
  {
    std::string buff_str;
    if (!make_tx_extra_alias_entry(buff_str, alinfo, make_buff_to_sign))
      return false;
    
    buff.resize(buff_str.size());
    memcpy(&buff[0], buff_str.data(), buff_str.size());
    return true;
  }
  //---------------------------------------------------------------
  bool add_tx_extra_alias(transaction& tx, const alias_info& alinfo)
  {
    std::string buff;
    bool r = make_tx_extra_alias_entry(buff, alinfo);
    if(!r) return false;
    extra_alias_entry eae = AUTO_VAL_INIT(eae);
    eae.buff.resize(buff.size());
    memcpy(&eae.buff[0], buff.data(), buff.size());
    tx.extra.push_back(eae);
    return true;
  }
  //---------------------------------------------------------------
  bool parse_tx_extra_alias_entry(const std::vector<uint8_t>& alias_buff, const transaction& tx, size_t start, alias_info& alinfo, size_t& whole_entry_len)
  {
    whole_entry_len = 0;
    size_t i = start;
    /************************************************************************************************************************************************************
               first byte counter+                                                                           first byte counter+
    1 byte          bytes[]             sizeof(crypto::public_key)*2           sizeof(crypto::secret_key)         bytes[]                 sizeof(crypto::signature)           
    |--flags--|c---alias name----|--------- account public address --------|[----account tracking key----]|[c--- text comment ---][----- signature(poof of alias owning) ------]

    ************************************************************************************************************************************************************/
    CHECK_AND_ASSERT_MES(alias_buff.size()-1-i >= sizeof(crypto::public_key)*2+1, false, "Failed to parse transaction extra (TX_EXTRA_TAG_ALIAS have not enough bytes) in tx " << get_transaction_hash(tx));
    ++i;
    uint8_t alias_flags = alias_buff[i];
    ++i;
    uint8_t alias_name_len = alias_buff[i];
    CHECK_AND_ASSERT_MES(alias_buff.size()-1-i >= alias_buff[i]+sizeof(crypto::public_key)*2, false, "Failed to parse transaction extra (TX_EXTRA_TAG_ALIAS have wrong name bytes counter) in tx " << get_transaction_hash(tx));
    
    alinfo.m_alias.assign((const char*)&alias_buff[i+1], static_cast<size_t>(alias_name_len));
    i += alias_buff[i] + 1;
    alinfo.m_address.m_spend_public_key = *reinterpret_cast<const crypto::public_key*>(&alias_buff[i]);
    i += sizeof(const crypto::public_key);
    alinfo.m_address.m_view_public_key = *reinterpret_cast<const crypto::public_key*>(&alias_buff[i]);
    i += sizeof(const crypto::public_key);
    if(alias_flags&TX_EXTRA_TAG_ALIAS_FLAGS_ADDR_WITH_TRACK)
    {//address aliased with tracking key
      CHECK_AND_ASSERT_MES(alias_buff.size()-i >= sizeof(crypto::secret_key), false, "Failed to parse transaction extra (TX_EXTRA_TAG_ALIAS have not enough bytes) in tx " << get_transaction_hash(tx));
      alinfo.m_view_key = *reinterpret_cast<const crypto::secret_key*>(&alias_buff[i]);
      i += sizeof(const crypto::secret_key);
    }
    uint8_t comment_len = alias_buff[i];
    if(comment_len)
    {
      CHECK_AND_ASSERT_MES(alias_buff.size() - i >=alias_buff[i], false, "Failed to parse transaction extra (TX_EXTRA_TAG_ALIAS have not enough bytes) in tx " << get_transaction_hash(tx));
      alinfo.m_text_comment.assign((const char*)&alias_buff[i+1], static_cast<size_t>(alias_buff[i]));
      i += alias_buff[i] + 1;
    }
    if(alias_flags&TX_EXTRA_TAG_ALIAS_FLAGS_OP_UPDATE)
    {
      CHECK_AND_ASSERT_MES(alias_buff.size()-i >= sizeof(crypto::secret_key), false, "Failed to parse transaction extra (TX_EXTRA_TAG_ALIAS have not enough bytes) in tx " << get_transaction_hash(tx));
      alinfo.m_sign = *reinterpret_cast<const crypto::signature*>(&alias_buff[i]);
      i += sizeof(const crypto::signature);
    }
    whole_entry_len = i - start;
    return true;
  }
  //---------------------------------------------------------------
  bool parse_and_validate_tx_extra(const transaction& tx, tx_extra_info& extra)
  {
    extra.m_tx_pub_key = null_pkey;


    struct tx_extra_handler: public boost::static_visitor<bool>
    {
      bool was_padding; //let the padding goes only at the end
      bool was_pubkey;
      bool was_attachment;
      bool was_userdata;
      bool was_alias;

      tx_extra_info& rei;
      const transaction& rtx;
      
      tx_extra_handler(tx_extra_info& ei, const transaction& tx) :rei(ei), rtx(tx)
      {
        was_padding = was_pubkey = was_attachment = was_userdata = was_alias = false;
      }

#define ENSURE_ONETIME(varname, entry_name) CHECK_AND_ASSERT_MES(varname == false, false, "double entry in tx_extra: " entry_name);


      bool operator()(const crypto::public_key& k) const
      {
        ENSURE_ONETIME(was_pubkey, "public_key");
        rei.m_tx_pub_key = k;
        return true;
      }
      bool operator()(const extra_attachment_info& ai) const
      {
        ENSURE_ONETIME(was_attachment, "attachment");
        rei.m_attachment_info = ai;
        return true;
      }
      bool operator()(const extra_alias_entry& ae) const
      {
        ENSURE_ONETIME(was_alias, "alias");
        size_t len = 0;
        bool r = parse_tx_extra_alias_entry(ae.buff, rtx, 0, rei.m_alias, len);
        CHECK_AND_ASSERT_MES(r, false, "failed to parse_tx_extra_alias_entry");

        return true;
      }
      bool operator()(const extra_user_data& ud) const
      {
        ENSURE_ONETIME(was_userdata, "userdata");
        rei.m_user_data_blob.assign((const char*)&ud.buff[0], ud.buff.size());
        return true;
      }
      bool operator()(const extra_padding& k) const
      {
        ENSURE_ONETIME(was_userdata, "padding");
        return true;
      }
    };

    tx_extra_handler vtr(extra, tx);

    for (const auto& ex_v : tx.extra)
    {
      if (!boost::apply_visitor(vtr, ex_v))
        return false;
    }
    return true;
  }
  //---------------------------------------------------------------
  bool parse_payment_id_from_hex_str(const std::string& payment_id_str, crypto::hash& payment_id)
  {
    blobdata payment_id_data;
    if(!string_tools::parse_hexstr_to_binbuff(payment_id_str, payment_id_data))
      return false;

    if(sizeof(crypto::hash) != payment_id_data.size())
      return false;

    payment_id = *reinterpret_cast<const crypto::hash*>(payment_id_data.data());
    return true;
  }
  //---------------------------------------------------------------
  bool add_tx_pub_key_to_extra(transaction& tx, const crypto::public_key& tx_pub_key)
  {
    tx.extra.push_back(tx_pub_key);
    return true;
  }
  //---------------------------------------------------------------
  bool add_attachments_info_to_extra(transaction& tx)
  {
    extra_attachment_info eai = AUTO_VAL_INIT(eai);
    get_type_in_variant_container_details(tx, eai);
    tx.extra.push_back(eai);
    return true;
  } 
  //---------------------------------------------------------------
  bool add_tx_extra_userdata(transaction& tx, const blobdata& extra_nonce)
  {
    CHECK_AND_ASSERT_MES(extra_nonce.size() <=255, false, "extra nonce could be 255 bytes max");
    extra_user_data eud = AUTO_VAL_INIT(eud);
    eud.buff.resize(extra_nonce.size());
    memcpy(&eud.buff[0], extra_nonce.data(), extra_nonce.size());
    tx.extra.push_back(eud);
    return true;
  }
  //---------------------------------------------------------------
  bool construct_tx_out(const account_public_address& destination_addr, const crypto::secret_key& tx_sec_key, size_t output_index, uint64_t amount, transaction& tx, uint8_t tx_outs_attr)
  {
    crypto::key_derivation derivation = AUTO_VAL_INIT(derivation);
    crypto::public_key out_eph_public_key = AUTO_VAL_INIT(out_eph_public_key);
    bool r = crypto::generate_key_derivation(destination_addr.m_view_public_key, tx_sec_key, derivation);
    CHECK_AND_ASSERT_MES(r, false, "at creation outs: failed to generate_key_derivation(" << destination_addr.m_view_public_key << ", " << tx_sec_key << ")");

    r = crypto::derive_public_key(derivation, output_index, destination_addr.m_spend_public_key, out_eph_public_key);
    CHECK_AND_ASSERT_MES(r, false, "at creation outs: failed to derive_public_key(" << derivation << ", " << output_index << ", "<< destination_addr.m_view_public_key << ")");

    tx_out out;
    out.amount = amount;
    txout_to_key tk;
    tk.key = out_eph_public_key;
    tk.mix_attr = tx_outs_attr;
    out.target = tk;
    tx.vout.push_back(out);
    return true;
  }
  //---------------------------------------------------------------
  void get_type_in_variant_container_details(const transaction& tx, extra_attachment_info& eai)
  {
    eai = extra_attachment_info();
    if (!tx.attachment.size())
      return;

    //put hash into extra
    std::stringstream ss;
    binary_archive<true> oar(ss);
    ::do_serialize(oar, const_cast<std::vector<attachment_v>&>(tx.attachment));
    std::string buff = ss.str();
    eai.sz = buff.size();
    eai.hsh = get_blob_hash(buff);
  }
  //---------------------------------------------------------------
  bool construct_tx(const account_keys& sender_account_keys, const std::vector<tx_source_entry>& sources, 
                                                             const std::vector<tx_destination_entry>& destinations, 
                                                             const std::vector<attachment_v>& attachments,
                                                             transaction& tx, 
                                                             uint64_t unlock_time, 
                                                             uint8_t tx_outs_attr)
  {
    crypto::secret_key one_time_secrete_key = AUTO_VAL_INIT(one_time_secrete_key);
    return construct_tx(sender_account_keys, sources, destinations, std::vector<extra_v>(), attachments, tx, one_time_secrete_key, unlock_time, tx_outs_attr);
  }
  //---------------------------------------------------------------
  struct encrypt_attach_visitor : public boost::static_visitor<void>
  {
    bool& m_was_crypted_entries;
    const crypto::key_derivation& m_key;
    encrypt_attach_visitor(bool& was_crypted_entries, const crypto::key_derivation& key) :m_was_crypted_entries(was_crypted_entries), m_key(key)
    {}
    void operator()(tx_comment& comment)
    {
      crypto::chacha_crypt(comment.comment, m_key);
      m_was_crypted_entries = true;
    }
    void operator()(tx_payer& pr)
    {
      crypto::chacha_crypt(pr.acc_addr, m_key);
      m_was_crypted_entries = true;
    }
    void operator()(tx_message& m)
    {
      crypto::chacha_crypt(m.msg, m_key);
      m_was_crypted_entries = true;
    }
    template<typename attachment_t>
    void operator()(attachment_t& comment)
    {}
  };

  struct decrypt_attach_visitor : public boost::static_visitor<void>
  {
    bool& rwas_crypted_entries;
    bool& rcheck_summ_validated;
    const crypto::key_derivation& rkey;
    std::vector<attachment_v>& rdecrypted_att;
    decrypt_attach_visitor(bool& check_summ_validated, 
                           bool& was_crypted_entries, 
                           const crypto::key_derivation& key, 
                           std::vector<attachment_v>& decrypted_att) :
      rwas_crypted_entries(was_crypted_entries),
      rkey(key),
      rcheck_summ_validated(check_summ_validated), 
      rdecrypted_att(decrypted_att)
    {}
    void operator()(const tx_comment& comment)
    {
      tx_comment local_comment = comment;
      crypto::chacha_crypt(local_comment.comment, rkey);
      rdecrypted_att.push_back(local_comment);
      rwas_crypted_entries = true;
    }

    void operator()(const tx_message& m)
    {
      tx_message local_msg = m;
      crypto::chacha_crypt(local_msg.msg, rkey);
      rdecrypted_att.push_back(local_msg);
      rwas_crypted_entries = true;
    }
    void operator()(const tx_payer& pr)
    {
      tx_payer payer_local = pr;
      crypto::chacha_crypt(payer_local.acc_addr, rkey);
      rdecrypted_att.push_back(payer_local);
      rwas_crypted_entries = true;
    }
    void operator()(const tx_crypto_checksum& chs)
    {
      crypto::hash hash_for_check_sum = crypto::cn_fast_hash(&rkey, sizeof(rkey));
      uint32_t chsumm =  *(uint32_t*)&hash_for_check_sum;
      if (chsumm != chs.summ)
      {
        LOG_ERROR("check summ missmatch in tx attachment decryption, checksumm_in_tx: " << chs.summ << ", expected: " << chsumm);
      }
      else
      {
        rcheck_summ_validated = true;
      }
    }
    template<typename attachment_t>
    void operator()(const attachment_t& comment)
    {}
  };
  //---------------------------------------------------------------
  bool decrypt_attachments(const transaction& tx, const account_keys& acc_keys, std::vector<attachment_v>& decrypted_att)
  {
    crypto::key_derivation derivation = AUTO_VAL_INIT(derivation);
    crypto::public_key tx_pub_key = currency::get_tx_pub_key_from_extra(tx);

    bool r = crypto::generate_key_derivation(tx_pub_key, acc_keys.m_view_secret_key, derivation);
    CHECK_AND_ASSERT_MES(r, false, "failed to generate_key_derivation");
    bool was_crypted_entries = false;
    bool check_summ_validated = false;
    LOG_PRINT_GREEN("DECRYPTING ON KEY: " << epee::string_tools::pod_to_hex(derivation), LOG_LEVEL_0);

    decrypt_attach_visitor v(check_summ_validated, was_crypted_entries, derivation, decrypted_att);
    for (auto& a : tx.attachment)
      boost::apply_visitor(v, a);

    if (was_crypted_entries && !check_summ_validated)
    {
      LOG_PRINT_RED_L0("Failed to decrypt attachments");
      decrypted_att.clear();
      return false;
    }
    return true;
  }
  //---------------------------------------------------------------
  void encrypt_attachments(transaction& tx, const account_public_address& destination_add, const keypair& tx_random_key)
  {
    crypto::key_derivation derivation = AUTO_VAL_INIT(derivation);
    bool r = crypto::generate_key_derivation(destination_add.m_view_public_key, tx_random_key.sec, derivation);
    CHECK_AND_ASSERT_MES(r, void(), "failed to generate_key_derivation");
    bool was_crypted_entries = false;
    LOG_PRINT_GREEN("ENCRYPTING ON KEY: " << epee::string_tools::pod_to_hex(derivation), LOG_LEVEL_0);


    encrypt_attach_visitor v(was_crypted_entries, derivation);
    for (auto& a : tx.attachment)
      boost::apply_visitor(v, a);

    if (was_crypted_entries)
    {
      crypto::hash hash_for_check_sum = crypto::cn_fast_hash(&derivation, sizeof(derivation));
      tx_crypto_checksum chs;
      chs.summ = *(uint32_t*)&hash_for_check_sum;
      tx.attachment.push_back(chs);
    }
  }
  //---------------------------------------------------------------
  bool construct_tx(const account_keys& sender_account_keys, const std::vector<tx_source_entry>& sources, 
                                                             const std::vector<tx_destination_entry>& destinations, 
                                                             const std::vector<extra_v>& extra,
                                                             const std::vector<attachment_v>& attachments,
                                                             transaction& tx, 
                                                             crypto::secret_key& one_time_secrete_key,
                                                             uint64_t unlock_time,
                                                             uint8_t tx_outs_attr)
  {
    tx.vin.clear();
    tx.vout.clear();
    tx.signatures.clear();
    tx.extra = extra;

    tx.version = CURRENT_TRANSACTION_VERSION;
    tx.unlock_time = unlock_time;

    keypair txkey = keypair::generate();
    add_tx_pub_key_to_extra(tx, txkey.pub);
    one_time_secrete_key = txkey.sec;
    
    //include offers if need
    tx.attachment = attachments;
    //encrypt attachments
    //find target account. for no we use just firs target account that is not sender, 
    //in case if there is no real targets we use sender credentials to encrypt attachments
    account_public_address crypt_destination_addr = AUTO_VAL_INIT(crypt_destination_addr);
    auto it = destinations.begin();
    for (; it != destinations.end(); ++it)
    {
      if (sender_account_keys.m_account_address != it->addr)
        break;
    }
    if (it == destinations.end())
      crypt_destination_addr = sender_account_keys.m_account_address;
    else
      crypt_destination_addr = it->addr;

    encrypt_attachments(tx, crypt_destination_addr, txkey);
    if (tx.attachment.size())
      add_attachments_info_to_extra(tx);


    // prepare inputs
    struct input_generation_context_data
    {
      keypair in_ephemeral;
    };
    std::vector<input_generation_context_data> in_contexts;


    uint64_t summary_inputs_money = 0;
    //fill inputs
    BOOST_FOREACH(const tx_source_entry& src_entr,  sources)
    {
      if(src_entr.real_output >= src_entr.outputs.size())
      {
        LOG_ERROR("real_output index (" << src_entr.real_output << ")bigger than output_keys.size()=" << src_entr.outputs.size());
        return false;
      }
      summary_inputs_money += src_entr.amount;

      //key_derivation recv_derivation;
      in_contexts.push_back(input_generation_context_data());
      keypair& in_ephemeral = in_contexts.back().in_ephemeral;
      crypto::key_image img;
      if(!generate_key_image_helper(sender_account_keys, src_entr.real_out_tx_key, src_entr.real_output_in_tx_index, in_ephemeral, img))
        return false;

      //check that derivated key is equal with real output key
      if( !(in_ephemeral.pub == src_entr.outputs[src_entr.real_output].second) )
      {
        LOG_ERROR("derived public key missmatch with output public key! "<< ENDL << "derived_key:"
          << string_tools::pod_to_hex(in_ephemeral.pub) << ENDL << "real output_public_key:"
          << string_tools::pod_to_hex(src_entr.outputs[src_entr.real_output].second) );
        return false;
      }

      //put key image into tx input
      txin_to_key input_to_key;
      input_to_key.amount = src_entr.amount;
      input_to_key.k_image = img;

      //fill outputs array and use relative offsets
      BOOST_FOREACH(const tx_source_entry::output_entry& out_entry, src_entr.outputs)
        input_to_key.key_offsets.push_back(out_entry.first);

      input_to_key.key_offsets = absolute_output_offsets_to_relative(input_to_key.key_offsets);
      tx.vin.push_back(input_to_key);
    }

    // "Shuffle" outs
    std::vector<tx_destination_entry> shuffled_dsts(destinations);
    std::sort(shuffled_dsts.begin(), shuffled_dsts.end(), [](const tx_destination_entry& de1, const tx_destination_entry& de2) { return de1.amount < de2.amount; } );

    uint64_t summary_outs_money = 0;
    //fill outputs
    size_t output_index = 0;
    BOOST_FOREACH(const tx_destination_entry& dst_entr,  shuffled_dsts)
    {
      CHECK_AND_ASSERT_MES(dst_entr.amount > 0, false, "Destination with wrong amount: " << dst_entr.amount);
      bool r = construct_tx_out(dst_entr.addr, txkey.sec, output_index, dst_entr.amount, tx,  tx_outs_attr);
      CHECK_AND_ASSERT_MES(r, false, "Failed to construc tx out");
      output_index++;
      summary_outs_money += dst_entr.amount;
    }

    //check money
    if(summary_outs_money > summary_inputs_money )
    {
      LOG_ERROR("Transaction inputs money ("<< summary_inputs_money << ") less than outputs money (" << summary_outs_money << ")");
      return false;
    }


    //generate ring signatures
    crypto::hash tx_prefix_hash;
    get_transaction_prefix_hash(tx, tx_prefix_hash);

    std::stringstream ss_ring_s;
    size_t i = 0;
    BOOST_FOREACH(const tx_source_entry& src_entr,  sources)
    {
      ss_ring_s << "pub_keys:" << ENDL;
      std::vector<const crypto::public_key*> keys_ptrs;
      BOOST_FOREACH(const tx_source_entry::output_entry& o, src_entr.outputs)
      {
        keys_ptrs.push_back(&o.second);
        ss_ring_s << o.second << ENDL;
      }

      tx.signatures.push_back(std::vector<crypto::signature>());
      std::vector<crypto::signature>& sigs = tx.signatures.back();
      sigs.resize(src_entr.outputs.size());
      crypto::generate_ring_signature(tx_prefix_hash, boost::get<txin_to_key>(tx.vin[i]).k_image, keys_ptrs, in_contexts[i].in_ephemeral.sec, src_entr.real_output, sigs.data());
      ss_ring_s << "signatures:" << ENDL;
      std::for_each(sigs.begin(), sigs.end(), [&](const crypto::signature& s){ss_ring_s << s << ENDL;});
      ss_ring_s << "prefix_hash:" << tx_prefix_hash << ENDL << "in_ephemeral_key: " << in_contexts[i].in_ephemeral.sec << ENDL << "real_output: " << src_entr.real_output;
      i++;
    }

    //if (!check_money_overflow(tx))
    //{
    //  LOG_ERROR("tx have money overflow after construct: " << get_transaction_hash(tx));
    //  return false;
    //}


    LOG_PRINT2("construct_tx.log", "transaction_created: " << get_transaction_hash(tx) << ENDL << obj_to_json_str(tx) << ENDL << ss_ring_s.str() , LOG_LEVEL_3);

    return true;
  }
  //---------------------------------------------------------------
  bool get_inputs_money_amount(const transaction& tx, uint64_t& money)
  {
    money = 0;
    BOOST_FOREACH(const auto& in, tx.vin)
    {
      CHECKED_GET_SPECIFIC_VARIANT(in, const txin_to_key, tokey_in, false);
      money += tokey_in.amount;
    }
    return true;
  }
  //---------------------------------------------------------------
  uint64_t get_block_height(const block& b)
  {
    CHECK_AND_ASSERT_MES(b.miner_tx.vin.size() == 1 || b.miner_tx.vin.size() == 2, 0, "wrong miner tx in block: " << get_block_hash(b) << ", b.miner_tx.vin.size() != 1");
    CHECKED_GET_SPECIFIC_VARIANT(b.miner_tx.vin[0], const txin_gen, coinbase_in, 0);
    return coinbase_in.height;
  }
  //---------------------------------------------------------------
  bool check_inputs_types_supported(const transaction& tx)
  {
    BOOST_FOREACH(const auto& in, tx.vin)
    {
      CHECK_AND_ASSERT_MES(in.type() == typeid(txin_to_key), false, "wrong variant type: "
        << in.type().name() << ", expected " << typeid(txin_to_key).name()
        << ", in transaction id=" << get_transaction_hash(tx));

    }
    return true;
  }
  //------------------------------------------------------------------
  bool add_padding_to_tx(transaction& tx, size_t count)
  {
    if (!count)
      return true;

    for (auto ex : tx.extra)
    {
      if (ex.type() == typeid(extra_padding))
      {
        boost::get<extra_padding>(ex).buff.insert(boost::get<extra_padding>(ex).buff.end(), count, 0);
        return true;
      }
    }
    extra_padding ex_padding;
    ex_padding.buff.resize(count - 1);
    tx.extra.push_back(ex_padding);
    return true;
  }
  //------------------------------------------------------------------
  bool is_tx_spendtime_unlocked(uint64_t unlock_time, uint64_t current_blockchain_height)
  {
    if (unlock_time < CURRENCY_MAX_BLOCK_NUMBER)
    {
      //interpret as block index
      if (current_blockchain_height - 1 + CURRENCY_LOCKED_TX_ALLOWED_DELTA_BLOCKS >= unlock_time)
        return true;
      else
        return false;
    }
    else
    {
      //interpret as time
      uint64_t current_time = static_cast<uint64_t>(time(NULL));
      if (current_time + CURRENCY_LOCKED_TX_ALLOWED_DELTA_SECONDS >= unlock_time)
        return true;
      else
        return false;
    }
    return false;
  }
  //-----------------------------------------------------------------------------------------------
  bool check_outs_valid(const transaction& tx)
  {
    BOOST_FOREACH(const tx_out& out, tx.vout)
    {
      CHECK_AND_ASSERT_MES(out.target.type() == typeid(txout_to_key), false, "wrong variant type: "
        << out.target.type().name() << ", expected " << typeid(txout_to_key).name()
        << ", in transaction id=" << get_transaction_hash(tx));

      CHECK_AND_NO_ASSERT_MES(0 < out.amount, false, "zero amount ouput in transaction id=" << get_transaction_hash(tx));

      if(!check_key(boost::get<txout_to_key>(out.target).key))
        return false;
    }
    return true;
  }
  //-----------------------------------------------------------------------------------------------
  bool check_money_overflow(const transaction& tx)
  {
    return check_inputs_overflow(tx) && check_outs_overflow(tx);
  }
  //---------------------------------------------------------------
  bool check_inputs_overflow(const transaction& tx)
  {
    uint64_t money = 0;
    BOOST_FOREACH(const auto& in, tx.vin)
    {
      CHECKED_GET_SPECIFIC_VARIANT(in, const txin_to_key, tokey_in, false);
      if(money > tokey_in.amount + money)
        return false;
      money += tokey_in.amount;
    }
    return true;
  }
  //---------------------------------------------------------------
  bool check_outs_overflow(const transaction& tx)
  {
    uint64_t money = 0;
    BOOST_FOREACH(const auto& o, tx.vout)
    {
      if(money > o.amount + money)
        return false;
      money += o.amount;
    }
    return true;
  }
  //---------------------------------------------------------------
  uint64_t get_outs_money_amount(const transaction& tx)
  {
    uint64_t outputs_amount = 0;
    BOOST_FOREACH(const auto& o, tx.vout)
      outputs_amount += o.amount;
    return outputs_amount;
  }
  //---------------------------------------------------------------
  std::string short_hash_str(const crypto::hash& h)
  {
    std::string res = string_tools::pod_to_hex(h);
    CHECK_AND_ASSERT_MES(res.size() == 64, res, "wrong hash256 with string_tools::pod_to_hex conversion");
    auto erased_pos = res.erase(8, 48);
    res.insert(8, "....");
    return res;
  }
  //---------------------------------------------------------------
  bool is_out_to_acc(const account_keys& acc, const txout_to_key& out_key, const crypto::public_key& tx_pub_key, size_t output_index)
  {
    crypto::key_derivation derivation;
    generate_key_derivation(tx_pub_key, acc.m_view_secret_key, derivation);
    crypto::public_key pk;
    derive_public_key(derivation, output_index, acc.m_account_address.m_spend_public_key, pk);
    return pk == out_key.key;
  }
  //---------------------------------------------------------------
  bool lookup_acc_outs(const account_keys& acc, const transaction& tx, std::vector<size_t>& outs, uint64_t& money_transfered)
  {
    crypto::public_key tx_pub_key = get_tx_pub_key_from_extra(tx);
    if(null_pkey == tx_pub_key)
      return false;
    return lookup_acc_outs(acc, tx, get_tx_pub_key_from_extra(tx), outs, money_transfered);
  }
  //---------------------------------------------------------------
  bool lookup_acc_outs(const account_keys& acc, const transaction& tx, const crypto::public_key& tx_pub_key, std::vector<size_t>& outs, uint64_t& money_transfered)
  {
    money_transfered = 0;
    size_t i = 0;
    BOOST_FOREACH(const tx_out& o,  tx.vout)
    {
      CHECK_AND_ASSERT_MES(o.target.type() ==  typeid(txout_to_key), false, "wrong type id in transaction out" );
      if(is_out_to_acc(acc, boost::get<txout_to_key>(o.target), tx_pub_key, i))
      {
        outs.push_back(i);
        money_transfered += o.amount;
      }
      i++;
    }
    return true;
  }
  //---------------------------------------------------------------
  bool set_payment_id_to_tx_extra(std::vector<extra_v>& extra, const std::string& payment_id)
  {

    if(!payment_id.size() || payment_id.size() >= TX_MAX_PAYMENT_ID_SIZE)
      return false;

    extra_user_data& ud = get_or_add_field_to_extra<extra_user_data>(extra);
    
    ud.buff.push_back(TX_EXTRA_TAG_USER_DATA);
    ud.buff.push_back(static_cast<uint8_t>(payment_id.size() + 2));
    ud.buff.push_back(TX_USER_DATA_TAG_PAYMENT_ID);
    ud.buff.push_back(static_cast<uint8_t>(payment_id.size()));
    
    const uint8_t* payment_id_ptr = reinterpret_cast<const uint8_t*>(payment_id.data());
    std::copy(payment_id_ptr, payment_id_ptr + payment_id.size(), std::back_inserter(ud.buff));
    return true;
  }
  //---------------------------------------------------------------
  bool get_payment_id_from_user_data(const std::string& user_data, std::string& payment_id)
  {
    if(!user_data.size())
      return false;
    if(user_data[0] != TX_USER_DATA_TAG_PAYMENT_ID)
      return false;
    if(user_data.size() < 2)
      return false;

    payment_id = user_data.substr(2, static_cast<size_t>(user_data[1]));
    return true;
  }
  //---------------------------------------------------------------
  bool get_payment_id_from_tx_extra(const transaction& tx, std::string& payment_id)
  {
    tx_extra_info tei = AUTO_VAL_INIT(tei);
    bool r = parse_and_validate_tx_extra(tx, tei);
    CHECK_AND_ASSERT_MES(r, false, "Failed to parse and validate extra");
    if(!tei.m_user_data_blob.size())
      return false;
    if(!get_payment_id_from_user_data(tei.m_user_data_blob, payment_id))
      return false;
    return true;
  }
  //---------------------------------------------------------------
  void get_blob_hash(const blobdata& blob, crypto::hash& res)
  {
    cn_fast_hash(blob.data(), blob.size(), res);
  }
  //---------------------------------------------------------------
  std::string print_money(uint64_t amount)
  {
    std::string s = std::to_string(amount);
    if(s.size() < CURRENCY_DISPLAY_DECIMAL_POINT+1)
    {
      s.insert(0, CURRENCY_DISPLAY_DECIMAL_POINT+1 - s.size(), '0');
    }
    s.insert(s.size() - CURRENCY_DISPLAY_DECIMAL_POINT, ".");
    return s;
  }
  //---------------------------------------------------------------
  crypto::hash get_blob_hash(const blobdata& blob)
  {
    crypto::hash h = null_hash;
    get_blob_hash(blob, h);
    return h;
  }
  //---------------------------------------------------------------
  crypto::hash get_transaction_hash(const transaction& t)
  {
    crypto::hash h = null_hash;
    size_t blob_size = 0;
    get_object_hash(static_cast<const transaction_prefix&>(t), h, blob_size);
    return h;
  }
  //---------------------------------------------------------------
  bool get_transaction_hash(const transaction& t, crypto::hash& res)
  {
    size_t blob_size = 0;
    return get_object_hash(static_cast<const transaction_prefix&>(t), res, blob_size);
  }
  //---------------------------------------------------------------
  /*bool get_transaction_hash(const transaction& t, crypto::hash& res, size_t& blob_size)
  {


    return get_object_hash(t, res, blob_size);
  }*/
  //------------------------------------------------------------------
  template<typename pod_operand_a, typename pod_operand_b>
  crypto::hash hash_together(const pod_operand_a& a, const pod_operand_b& b)
  {
    std::string blob;
    string_tools::apped_pod_to_strbuff(blob, a);
    string_tools::apped_pod_to_strbuff(blob, b);  
    return crypto::cn_fast_hash(blob.data(), blob.size());
  }
  //------------------------------------------------------------------
  crypto::hash get_blob_longhash(const blobdata& bd)
  {
    return crypto::x11hash(bd);
  }
  //---------------------------------------------------------------
  void get_block_longhash(const block& b, crypto::hash& res)
  {
    blobdata bd = get_block_hashing_blob(b);
    res = get_blob_longhash(bd);
  }
  //---------------------------------------------------------------
  crypto::hash get_block_longhash(const block& b)
  {
    crypto::hash p = null_hash;
    get_block_longhash(b, p);
    return p;
  }
  //------------------------------------------------------------------
  bool validate_alias_name(const std::string& al)
  {
    CHECK_AND_ASSERT_MES(al.size() <= MAX_ALIAS_LEN, false, "Too long alias name, please use name no longer than " << MAX_ALIAS_LEN );
    /*allowed symbols "0-9", "a-z", "-", "." */
    static bool alphabet[256] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                 0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,
                                 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                 0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,
                                 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}; 

    for(const auto ch: al)
    {
      CHECK_AND_ASSERT_MES( alphabet[static_cast<unsigned char>(ch)], false, "Wrong character in alias: '" << ch << "'");
    }
    return true;
  }


  //------------------------------------------------------------------
#define ANTI_OVERFLOW_AMOUNT       1000000
#define GET_PERECENTS_BIG_NUMBERS(per, total) (per/ANTI_OVERFLOW_AMOUNT)*100 / (total/ANTI_OVERFLOW_AMOUNT) 

  void print_reward_halwing()
  {
    std::cout << std::endl << "Reward halving for 10 years:" << std::endl;
    std::cout << std::setw(10) << std::left << "day" << std::setw(19) << "block reward" << std::setw(19) << "generated coins" << std::endl;

    uint64_t already_generated_coins = PREMINE_AMOUNT;
    //uint64_t total_money_supply = TOTAL_MONEY_SUPPLY;
    uint64_t h = 0;
    for(uint64_t day = 0; day != 365*10; ++day)
    {
      uint64_t emission_reward = 0;
      get_block_reward(0, 0, already_generated_coins, emission_reward, h, 0);
      if(!(day%183))
      {
        std::cout << std::left 
          << std::setw(10) << day 
          << std::setw(19) << print_money(emission_reward) 
          << std::setw(4) << print_money(already_generated_coins) //std::string(std::to_string(GET_PERECENTS_BIG_NUMBERS((already_generated_coins), total_money_supply)) + "%")  
          << std::endl;
      }

      
      for(size_t i = 0; i != 720; i++)
      {
        h++;
        get_block_reward(0, 0, already_generated_coins, emission_reward, h, 0);
        already_generated_coins += emission_reward;
        if (h < POS_START_HEIGHT && i > 360)
          break;
      }
    }
  }
  void print_reward_halwing_short()
  {
    std::cout << std::endl << "Reward halving for 20 days:" << std::endl;
    std::cout << std::setw(10) << std::left << "day" << std::setw(19) << "block reward" << std::setw(19) << "generated coins" << std::endl;

    uint64_t already_generated_coins = PREMINE_AMOUNT;
    //uint64_t total_money_supply = TOTAL_MONEY_SUPPLY;
    uint64_t h = 0;
    for (uint64_t day = 0; day != 20; ++day)
    {
      uint64_t emission_reward = 0;
      get_block_reward(0, 0, already_generated_coins, emission_reward, h, (already_generated_coins - PREMINE_AMOUNT) * 140);

      std::cout << std::left
        << std::setw(10) << day
        << std::setw(19) << print_money(emission_reward)
        << std::setw(4) << print_money(already_generated_coins)//std::string(std::to_string(GET_PERECENTS_BIG_NUMBERS((already_generated_coins), total_money_supply)) + "%")
        << std::endl;



      for (size_t i = 0; i != 720; i++)
      {
        h++;
        get_block_reward(0, 0, already_generated_coins, emission_reward, h, (already_generated_coins - PREMINE_AMOUNT) * 140);
        already_generated_coins += emission_reward;
        if (h < POS_START_HEIGHT && i > 360)
          break;
      }
    }
  }
  //------------------------------------------------------------------
  void print_currency_details()
  {   
    //for future forks 
    
    std::cout << "Currency name: \t\t" << CURRENCY_NAME <<"(" << CURRENCY_NAME_SHORT << ")" << std::endl;
    std::cout << "Money supply: \t\t" << print_money(TOTAL_MONEY_SUPPLY) << " coins"
      << "(" << print_money(TOTAL_MONEY_SUPPLY) << "), dev bounties is ???" << std::endl;

    std::cout << "PoS block interval: \t" << DIFFICULTY_POS_TARGET << " seconds" << std::endl;
    std::cout << "PoW block interval: \t" << DIFFICULTY_POW_TARGET << " seconds" << std::endl;
    std::cout << "Default p2p port: \t" << P2P_DEFAULT_PORT << std::endl;
    std::cout << "Default rpc port: \t" << RPC_DEFAULT_PORT << std::endl;
#ifndef TEST_FAST_EMISSION_CURVE
    print_reward_halwing();
#else
    print_reward_halwing_short();
#endif
  }
  //------------------------------------------------------------------
  std::string dump_patch(const std::map<uint64_t, crypto::hash>& patch)
  {
    std::stringstream ss;
    for(auto& p: patch)
    {
      ss << "[" << p.first << "]" << p.second << ENDL;
    }
    return ss.str();
  }
  //---------------------------------------------------------------
  blobdata get_block_hashing_blob(const block& b)
  {
    blobdata blob = t_serializable_object_to_blob(static_cast<block_header>(b));
    crypto::hash tree_root_hash = get_tx_tree_hash(b);
    blob.append((const char*)&tree_root_hash, sizeof(tree_root_hash ));
    blob.append(tools::get_varint_data(b.tx_hashes.size()+1));
    return blob;
  }
  //---------------------------------------------------------------
  bool get_block_hash(const block& b, crypto::hash& res)
  {
    return get_object_hash(get_block_hashing_blob(b), res);
  }
  //---------------------------------------------------------------
  crypto::hash get_block_hash(const block& b)
  {
    crypto::hash p = null_hash;
    get_block_hash(b, p);
    return p;
  }
  //---------------------------------------------------------------
  bool generate_genesis_block(block& bl)
  {
    //genesis block
    bl = boost::value_initialized<block>();
    /*
    //account_public_address ac = boost::value_initialized<account_public_address>();
    //std::vector<size_t> sz;
    //proof 
#ifndef TESTNET
    std::string proof = "TODO: Paste here some text";
#else 
    std::string proof = "TODO: Paste here some text";
#endif

     alias_info ai = AUTO_VAL_INIT(ai);
//     ai.m_alias = "zoidberg";
//     ai.m_text_comment = "Let's go!";
//     get_account_address_from_str(ai.m_address, "HgBGCZTVFVA3uaeQx1Tyi7Vz1StYcWofGF3seFfiduzwadHcj4ha7PGgLwgHzVbzmTV1vpEbDnpuaUF6CAcvwkM8GstFX5R"); 

    std::vector<tx_destination_entry> destinations;
    tx_destination_entry de = AUTO_VAL_INIT(de);

#define ADD_PREMINE_ADDRESS(addr_str, coins_amount) \
    {bool r = get_account_address_from_str(de.addr, addr_str); \
    CHECK_AND_ASSERT_MES(r, false, "Failed to get_account_address_from_str from address " << addr_str); \
    de.amount = coins_amount;  \
    destinations.push_back(de);   }

    uint64_t amount_per_wallet = PREMINE_AMOUNT/10;

    ADD_PREMINE_ADDRESS(PREMINE_WALLET_ADDRESS_0, amount_per_wallet);
    ADD_PREMINE_ADDRESS(PREMINE_WALLET_ADDRESS_1, amount_per_wallet);
    ADD_PREMINE_ADDRESS(PREMINE_WALLET_ADDRESS_2, amount_per_wallet);
    ADD_PREMINE_ADDRESS(PREMINE_WALLET_ADDRESS_3, amount_per_wallet);
    ADD_PREMINE_ADDRESS(PREMINE_WALLET_ADDRESS_4, amount_per_wallet);
    ADD_PREMINE_ADDRESS(PREMINE_WALLET_ADDRESS_5, amount_per_wallet);
    ADD_PREMINE_ADDRESS(PREMINE_WALLET_ADDRESS_6, amount_per_wallet);
    ADD_PREMINE_ADDRESS(PREMINE_WALLET_ADDRESS_7, amount_per_wallet);
    ADD_PREMINE_ADDRESS(PREMINE_WALLET_ADDRESS_8, amount_per_wallet);
    ADD_PREMINE_ADDRESS(PREMINE_WALLET_ADDRESS_9, amount_per_wallet);


    construct_miner_tx(0, 0, 0, 0, 0, destinations, bl.miner_tx, proof, 11, ai); // zero profit in genesis
    blobdata txb = tx_to_blob(bl.miner_tx);
    std::string hex_tx_represent = string_tools::buff_to_hex_nodelimer(txb);

    blobdata tx_bl2;
    string_tools::parse_hexstr_to_binbuff(hex_tx_represent, tx_bl2);
    parse_and_validate_tx_from_blob(tx_bl2, bl.miner_tx);
 */
    //hard code coinbase tx in genesis block, because "true" generating tx use random, but genesis should be always the same
#ifndef TESTNET
    std::string genesis_coinbase_tx_hex = "010a01ff000a80809aa6eaafe30102ded9bda9f8c4894e338466615e69048ffffa7d22781fbaee0fbcef590129cd0d0080809aa6eaafe30102f63e6721ec79025cb7acb65aad1c52ee49b6fd17c2c7fd2653961738327f4bd50080809aa6eaafe30102ea60a97ed4372256ba4827c11585ea3f956a6f4f0e210238f07a1740f2170c110080809aa6eaafe301029593253bd837204354fea87f21c52e346a6d8579de6150060c9902c678ce77560080809aa6eaafe301024e667bc7568f738fc39aaafd75e2adea2aad0a74b0c5ea282bf31052aedaaea60080809aa6eaafe30102a70806a10f1bdc99f9a65aad2cac70a567ae215425d24894f6215745a481da0a0080809aa6eaafe301027910e06b6c5ce5f4c607ceacbed613096e4e32a1ce103afb230fabe5d57fd2040080809aa6eaafe301026931355a4bd02e059f700a7e31b9591cfa4bf5f208c50f977bf8f5814a4a2bb30080809aa6eaafe30102d7245d281423408e6259d5c13fe7f54e1d1e26282a94b8152ef3f1e488a39a100080809aa6eaafe30102074bf73afd94ff05b5f2c9c516c59855321df8668c3cd8cc2506d064fe42a7c4000204c489880b0cbcf2d0a5179a5d123101f0a2ab82136ee3563871bbae95a992b71e011a544f444f3a205061737465206865726520736f6d6520746578740000";
#else 
    std::string genesis_coinbase_tx_hex = "";                                          
#endif

    blobdata tx_bl;
    string_tools::parse_hexstr_to_binbuff(genesis_coinbase_tx_hex, tx_bl);
    bool r = parse_and_validate_tx_from_blob(tx_bl, bl.miner_tx);
    CHECK_AND_ASSERT_MES(r, false, "failed to parse coinbase tx from hard coded blob");
    bl.major_version = CURRENT_BLOCK_MAJOR_VERSION;
    bl.minor_version = CURRENT_BLOCK_MINOR_VERSION;
    bl.timestamp = 0;
    bl.nonce = 101010121; //bender's nightmare
    //miner::find_nonce_for_given_block(bl, 1, 0,);
    LOG_PRINT_GREEN("Generated genesis: " << get_block_hash(bl), LOG_LEVEL_0);
    return true;
  }
  //---------------------------------------------------------------
  //---------------------------------------------------------------
  std::vector<uint64_t> relative_output_offsets_to_absolute(const std::vector<uint64_t>& off)
  {
    std::vector<uint64_t> res = off;
    for(size_t i = 1; i < res.size(); i++)
      res[i] += res[i-1];
    return res;
  }
  //---------------------------------------------------------------
  std::vector<uint64_t> absolute_output_offsets_to_relative(const std::vector<uint64_t>& off)
  {
    std::vector<uint64_t> res = off;
    if(!off.size())
      return res;
    std::sort(res.begin(), res.end());//just to be sure, actually it is already should be sorted
    for(size_t i = res.size()-1; i != 0; i--)
      res[i] -= res[i-1];

    return res;
  }
  //---------------------------------------------------------------
  bool parse_and_validate_block_from_blob(const blobdata& b_blob, block& b)
  {
    std::stringstream ss;
    ss << b_blob;
    binary_archive<false> ba(ss);
    bool r = ::serialization::serialize(ba, b);
    CHECK_AND_ASSERT_MES(r, false, "Failed to parse block from blob");
    return true;
  }
  //---------------------------------------------------------------
  size_t get_object_blobsize(const transaction& t)
  {
    size_t prefix_blob = get_object_blobsize(static_cast<const transaction_prefix&>(t));

    if(is_coinbase(t))    
      return prefix_blob;    

    for(const auto& in: t.vin)
    {
      size_t sig_count = transaction::get_signature_size(in);
      prefix_blob += 64*sig_count;
      prefix_blob += tools::get_varint_packed_size(sig_count);
    }
    prefix_blob += tools::get_varint_packed_size(t.vin.size());

    tx_extra_info tei = AUTO_VAL_INIT(tei);
    currency::parse_and_validate_tx_extra(t, tei);
    prefix_blob += tei.m_attachment_info.sz;
    prefix_blob += tools::get_varint_packed_size(tei.m_attachment_info.sz);

    return prefix_blob;
  }
  //---------------------------------------------------------------
  blobdata block_to_blob(const block& b)
  {
    return t_serializable_object_to_blob(b);
  }
  //---------------------------------------------------------------
  bool block_to_blob(const block& b, blobdata& b_blob)
  {
    return t_serializable_object_to_blob(b, b_blob);
  }
  //---------------------------------------------------------------
  blobdata tx_to_blob(const transaction& tx)
  {
    return t_serializable_object_to_blob(tx);
  }
  //---------------------------------------------------------------
  bool tx_to_blob(const transaction& tx, blobdata& b_blob)
  {
    return t_serializable_object_to_blob(tx, b_blob);
  }
  //---------------------------------------------------------------
  void get_tx_tree_hash(const std::vector<crypto::hash>& tx_hashes, crypto::hash& h)
  {
    tree_hash(tx_hashes.data(), tx_hashes.size(), h);
  }
  //---------------------------------------------------------------
  crypto::hash get_tx_tree_hash(const std::vector<crypto::hash>& tx_hashes)
  {
    crypto::hash h = null_hash;
    get_tx_tree_hash(tx_hashes, h);
    return h;
  }
  //---------------------------------------------------------------
  crypto::hash get_tx_tree_hash(const block& b)
  {
    std::vector<crypto::hash> txs_ids;
    crypto::hash h = null_hash;
    get_transaction_hash(b.miner_tx, h);
    txs_ids.push_back(h);
    BOOST_FOREACH(auto& th, b.tx_hashes)
      txs_ids.push_back(th);
    return get_tx_tree_hash(txs_ids);
  }
  //---------------------------------------------------------------
  bool is_service_tx(const transaction& tx)
  {
    for (const auto& e : tx.extra)
    {
      if (e.type() == typeid(extra_alias_entry))
        return true;
    }

    for (const auto& a: tx.attachment)
    {
      if (a.type() == typeid(offer_details))
        return true;
    }

    return false;
  }
  //---------------------------------------------------------------
  bool is_anonymous_tx(const transaction& tx)
  {
    for (const auto& e : tx.vin)
    {
      if (e.type() != typeid(txin_to_key))
        return false;
      if (boost::get<txin_to_key>(e).key_offsets.size() < 2)
        return false;
    }

    return true;
  }
  //---------------------------------------------------------------
  bool get_aliases_reward_account(account_public_address& acc)
  {
    bool r = string_tools::parse_tpod_from_hex_string(ALIAS_REWARDS_ACCOUNT_SPEND_PUB_KEY, acc.m_spend_public_key);
    r &= string_tools::parse_tpod_from_hex_string(ALIAS_REWARDS_ACCOUNT_VIEW_PUB_KEY, acc.m_view_public_key);
    return r;
  }
  //---------------------------------------------------------------
  bool get_aliases_reward_account(account_public_address& acc, crypto::secret_key& acc_view_key)
  {
    bool r = get_aliases_reward_account(acc);
    r &= string_tools::parse_tpod_from_hex_string(ALIAS_REWARDS_ACCOUNT_VIEW_SEC_KEY, acc_view_key);
    return r;
  }
  //---------------------------------------------------------------
  bool is_pos_block(const block& b)
  {
    if (!b.flags&CURRENCY_BLOCK_FLAG_POS_BLOCK)
      return false;
    return is_pos_block(b.miner_tx);
  }
  //---------------------------------------------------------------
  bool is_pos_block(const transaction& tx)
  {
    if (tx.vin.size() == 2 &&
        tx.vin[0].type() == typeid(txin_gen) &&
        tx.vin[1].type() == typeid(txin_to_key))
      return true;
    return false;
  }
  size_t get_max_block_size()
  {
    return CURRENCY_MAX_BLOCK_SIZE;
  }
  //-----------------------------------------------------------------------------------------------
  size_t get_max_tx_size()
  {
    return CURRENCY_MAX_TX_SIZE;
  }
  //-----------------------------------------------------------------------------------------------
  uint64_t get_base_block_reward(uint64_t already_generated_coins, uint64_t height, const wide_difficulty_type& pos_diff)
  {
    if (!height)
      return PREMINE_AMOUNT;
    
    uint64_t base_reward = 0;
    //emission curve
    if (height < SIGNIFICANT_EMISSION_SWICH_HEIGHT)
    {
      //------------------------------------------------------------
      //we did this tiny workaround to make easier life of coretests
      if (height < 100)
        height = 100;
      //------------------------------------------------------------
      base_reward = height*(SIGNIFICANT_EMISSION_RANGE - height) * SIGNIFICANT_EMISSION_REWARD_MULTIPLIER;
    }
    else
    {
      // adjustments
      uint64_t estimated_mining_coins = (pos_diff / 150).convert_to<uint64_t>();
      uint64_t already_generated_coins_actual = already_generated_coins - PREMINE_AMOUNT;

      if (estimated_mining_coins > already_generated_coins_actual)
        estimated_mining_coins = already_generated_coins_actual;
      
      //------

      base_reward = ((already_generated_coins_actual - estimated_mining_coins) / 50) / PERCENTS_PERIOD;
    }

    //crop dust
    base_reward = base_reward - base_reward%DEFAULT_DUST_THRESHOLD;
    return base_reward;
  }
  //-----------------------------------------------------------------------------------------------
  bool get_block_reward(size_t median_size, size_t current_block_size, uint64_t already_generated_coins, uint64_t &reward, uint64_t height, const wide_difficulty_type& pos_diff)
  {
    uint64_t base_reward = get_base_block_reward(already_generated_coins, height, pos_diff);


    //make it soft
    if (median_size < CURRENCY_BLOCK_GRANTED_FULL_REWARD_ZONE)
    {
      median_size = CURRENCY_BLOCK_GRANTED_FULL_REWARD_ZONE;
    }

    if (current_block_size <= median_size)
    {
      reward = base_reward;
      return true;
    }

    if (current_block_size > 2 * median_size)
    {
      LOG_PRINT_L4("Block cumulative size is too big: " << current_block_size << ", expected less than " << 2 * median_size);
      return false;
    }

    CHECK_AND_ASSERT_MES(median_size < std::numeric_limits<uint32_t>::max(), false, "median_size < std::numeric_limits<uint32_t>::max() asert failed");
    CHECK_AND_ASSERT_MES(current_block_size < std::numeric_limits<uint32_t>::max(), false, "current_block_size < std::numeric_limits<uint32_t>::max()");

    //uint64_t product_hi;
    //uint64_t product_lo = mul128(base_reward, current_block_size * (2 * median_size - current_block_size), &product_hi);
    uint128_tl product = uint128_tl(base_reward) * current_block_size * (2 * median_size - current_block_size);
    uint128_tl reward_wide = product / median_size;
//    uint64_t reward_hi;
//    uint64_t reward_lo;
//    div128_32(product_hi, product_lo, static_cast<uint32_t>(median_size), &reward_hi, &reward_lo);
//    div128_32(reward_hi, reward_lo, static_cast<uint32_t>(median_size), &reward_hi, &reward_lo);
//    CHECK_AND_ASSERT_MES(0 == reward_hi, false, "0 == reward_hi");
    CHECK_AND_ASSERT_MES(reward_wide < base_reward, false, "reward_lo < base_reward");

    reward = reward_wide.convert_to<uint64_t>();
    return true;
  }
  //------------------------------------------------------------------------------------
  uint8_t get_account_address_checksum(const public_address_outer_blob& bl)
  {
    const unsigned char* pbuf = reinterpret_cast<const unsigned char*>(&bl);
    uint8_t summ = 0;
    for (size_t i = 0; i != sizeof(public_address_outer_blob)-1; i++)
      summ += pbuf[i];

    return summ;
  }
  //-----------------------------------------------------------------------
  std::string get_account_address_as_str(const account_public_address& adr)
  {
    return tools::base58::encode_addr(CURRENCY_PUBLIC_ADDRESS_BASE58_PREFIX, t_serializable_object_to_blob(adr));
  }
  //-----------------------------------------------------------------------
  bool is_coinbase(const transaction& tx)
  {
    if (!tx.vin.size() || tx.vin.size() > 2)
      return false;

    if (tx.vin[0].type() != typeid(txin_gen))
      return false;

    return true;
  }
  //-----------------------------------------------------------------------
  bool get_account_address_from_str(account_public_address& adr, const std::string& str)
  {
    if (2 * sizeof(public_address_outer_blob) != str.size())
    {
      blobdata data;
      uint64_t prefix;
      if (!tools::base58::decode_addr(str, prefix, data))
      {
        LOG_PRINT_L1("Invalid address format");
        return false;
      }

      if (CURRENCY_PUBLIC_ADDRESS_BASE58_PREFIX != prefix)
      {
        LOG_PRINT_L1("Wrong address prefix: " << prefix << ", expected " << CURRENCY_PUBLIC_ADDRESS_BASE58_PREFIX);
        return false;
      }

      if (!::serialization::parse_binary(data, adr))
      {
        LOG_PRINT_L1("Account public address keys can't be parsed");
        return false;
      }

      if (!crypto::check_key(adr.m_spend_public_key) || !crypto::check_key(adr.m_view_public_key))
      {
        LOG_PRINT_L1("Failed to validate address keys");
        return false;
      }
    }
    else
    {
      // Old address format
      std::string buff;
      if (!string_tools::parse_hexstr_to_binbuff(str, buff))
        return false;

      if (buff.size() != sizeof(public_address_outer_blob))
      {
        LOG_PRINT_L1("Wrong public address size: " << buff.size() << ", expected size: " << sizeof(public_address_outer_blob));
        return false;
      }

      public_address_outer_blob blob = *reinterpret_cast<const public_address_outer_blob*>(buff.data());


      if (blob.m_ver > CURRENCY_PUBLIC_ADDRESS_TEXTBLOB_VER)
      {
        LOG_PRINT_L1("Unknown version of public address: " << blob.m_ver << ", expected " << CURRENCY_PUBLIC_ADDRESS_TEXTBLOB_VER);
        return false;
      }

      if (blob.check_sum != get_account_address_checksum(blob))
      {
        LOG_PRINT_L1("Wrong public address checksum");
        return false;
      }

      //we success
      adr = blob.m_address;
    }

    return true;
  }

  //--------------------------------------------------------------------------------
}
namespace currency
{
  bool operator ==(const currency::transaction& a, const currency::transaction& b) {
    return currency::get_transaction_hash(a) == currency::get_transaction_hash(b);
  }

  bool operator ==(const currency::block& a, const currency::block& b) {
    return currency::get_block_hash(a) == currency::get_block_hash(b);
  }

}

bool parse_hash256(const std::string str_hash, crypto::hash& hash)
{
  std::string buf;
  bool res = epee::string_tools::parse_hexstr_to_binbuff(str_hash, buf);
  if (!res || buf.size() != sizeof(crypto::hash))
  {
    std::cout << "invalid hash format: <" << str_hash << '>' << std::endl;
    return false;
  }
  else
  {
    buf.copy(reinterpret_cast<char *>(&hash), sizeof(crypto::hash));
    return true;
  }
}
