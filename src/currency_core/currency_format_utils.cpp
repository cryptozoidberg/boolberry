// Copyright (c) 2012-2013 The Cryptonote developers
// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "include_base_utils.h"
using namespace epee;

#include "currency_format_utils.h"
#include <boost/foreach.hpp>
#include "currency_config.h"
#include "miner.h"
#include "crypto/crypto.h"
#include "crypto/hash.h"

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
    return t_unserializable_object_from_blob(tx, tx_blob);
  }
  //---------------------------------------------------------------
  bool parse_and_validate_tx_from_blob(const blobdata& tx_blob, transaction& tx, crypto::hash& tx_hash, crypto::hash& tx_prefix_hash)
  {

    bool r = t_unserializable_object_from_blob(tx, tx_blob);
    CHECK_AND_ASSERT_MES(r, false, "Failed to parse transaction from blob");
    //TODO: validate tx

    //crypto::cn_fast_hash(tx_blob.data(), tx_blob.size(), tx_hash);
    get_transaction_prefix_hash(tx, tx_prefix_hash);
    tx_hash = tx_prefix_hash;
    return true;
  }
  //---------------------------------------------------------------
  bool get_donation_accounts(account_keys &donation_acc, account_keys &royalty_acc)
  {
    bool r = get_account_address_from_str(donation_acc.m_account_address, CURRENCY_DONATIONS_ADDRESS);
    CHECK_AND_ASSERT_MES(r, false, "failed to get_account_address_from_str from P2P_DONATIONS_ADDRESS");

    r = string_tools::parse_tpod_from_hex_string(CURRENCY_DONATIONS_ADDRESS_TRACKING_KEY, donation_acc.m_view_secret_key);
    CHECK_AND_ASSERT_MES(r, false, "failed to parse_tpod_from_hex_string from P2P_DONATIONS_ADDRESS_TRACKING_KEY");

    r = get_account_address_from_str(royalty_acc.m_account_address, CURRENCY_ROYALTY_ADDRESS);
    CHECK_AND_ASSERT_MES(r, false, "failed to get_account_address_from_str from P2P_ROYALTY_ADDRESS");

    r = string_tools::parse_tpod_from_hex_string(CURRENCY_ROYALTY_ADDRESS_TRACKING_KEY, royalty_acc.m_view_secret_key);
    CHECK_AND_ASSERT_MES(r, false, "failed to parse_tpod_from_hex_string from P2P_ROYALTY_ADDRESS_TRACKING_KEY");
    return true;
  }
  //---------------------------------------------------------------
  bool construct_miner_tx(size_t height, size_t median_size, uint64_t already_generated_coins, 
                                                             size_t current_block_size, 
                                                             uint64_t fee, 
                                                             const account_public_address &miner_address, 
                                                             transaction& tx, 
                                                             const blobdata& extra_nonce, 
                                                             size_t max_outs)
  {
    account_keys donation_acc = AUTO_VAL_INIT(donation_acc);
    account_keys royalty_acc = AUTO_VAL_INIT(royalty_acc);
    get_donation_accounts(donation_acc, royalty_acc);
    alias_info alias = AUTO_VAL_INIT(alias);
    return construct_miner_tx(height, median_size, already_generated_coins, 0, 
                                                                            current_block_size, 
                                                                            fee, 
                                                                            miner_address, 
                                                                            donation_acc.m_account_address, 
                                                                            royalty_acc.m_account_address, 
                                                                            tx, 
                                                                            extra_nonce, 
                                                                            max_outs, 
                                                                            0, 
                                                                            alias);
  }
  //---------------------------------------------------------------
  bool construct_miner_tx(size_t height, size_t median_size, uint64_t already_generated_coins, 
                                                             uint64_t already_donated_coins, 
                                                             size_t current_block_size, 
                                                             uint64_t fee, 
                                                             const account_public_address &miner_address, 
                                                             const account_public_address &donation_address, 
                                                             const account_public_address &royalty_address, 
                                                             transaction& tx, 
                                                             const blobdata& extra_nonce, 
                                                             size_t max_outs, 
                                                             size_t amount_to_donate, 
                                                             const alias_info& alias)
  {
    tx.vin.clear();
    tx.vout.clear();
    tx.extra.clear();

    keypair txkey = keypair::generate();
    add_tx_pub_key_to_extra(tx, txkey.pub);
    if(extra_nonce.size())
      if(!add_tx_extra_nonce(tx, extra_nonce))
        return false;
    if(alias.m_alias.size())
    {
      if(!add_tx_extra_alias(tx, alias))
        return false;
    }

    txin_gen in;
    in.height = height;

    uint64_t block_reward;
    uint64_t max_donation = 0;
    if(!get_block_reward(median_size, current_block_size, already_generated_coins, already_donated_coins, block_reward, max_donation))
    {
      LOG_PRINT_L0("Block is too big");
      return false;
    }
    block_reward += fee;
    uint64_t total_donation_amount = 0;//(max_donation * percents_to_donate)/100;
    if(height && !(height%CURRENCY_DONATIONS_INTERVAL))
      total_donation_amount = amount_to_donate;

    uint64_t royalty = 0;
    uint64_t donations = 0;
    get_donation_parts(total_donation_amount, royalty, donations);

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
    size_t no = 0;
    for (; no < out_amounts.size(); no++)
    {
      bool r = construct_tx_out(miner_address, txkey.sec, no, out_amounts[no], tx);
      CHECK_AND_ASSERT_MES(r, false, "Failed to contruct miner tx out");
      summary_amounts += out_amounts[no];
    }

    CHECK_AND_ASSERT_MES(summary_amounts == block_reward, false, "Failed to construct miner tx, summary_amounts = " << summary_amounts << " not equal block_reward = " << block_reward);

    //add donation if need
    if(donations)
    {
      bool r = construct_tx_out(donation_address, txkey.sec, no, donations, tx);
      CHECK_AND_ASSERT_MES(r, false, "Failed to contruct miner tx out");
      ++no;
    }

    if(royalty)
    {
      bool r = construct_tx_out(royalty_address, txkey.sec, no, royalty, tx);
      CHECK_AND_ASSERT_MES(r, false, "Failed to contruct miner tx out");
      ++no;
    }

    tx.version = CURRENT_TRANSACTION_VERSION;
    //lock
    tx.unlock_time = height + CURRENCY_MINED_MONEY_UNLOCK_WINDOW;
    tx.vin.push_back(in);
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
  bool add_tx_extra_alias(transaction& tx, const alias_info& alinfo)
  {
    std::string buff;
    bool r = make_tx_extra_alias_entry(buff, alinfo);
    if(!r) return false;
    tx.extra.resize(tx.extra.size() + buff.size());
    memcpy(&tx.extra[tx.extra.size() - buff.size()], buff.data(), buff.size());
    return true;
  }
  //---------------------------------------------------------------
  bool parse_tx_extra_alias_entry(const transaction& tx, size_t start, alias_info& alinfo, size_t& whole_entry_len)
  {
    whole_entry_len = 0;
    size_t i = start;
    /************************************************************************************************************************************************************
               first byte counter+                                                                           first byte counter+
    1 byte          bytes[]             sizeof(crypto::public_key)*2           sizeof(crypto::secret_key)         bytes[]                 sizeof(crypto::signature)           
    |--flags--|c---alias name----|--------- account public address --------|[----account tracking key----]|[c--- text comment ---][----- signature(poof of alias owning) ------]

    ************************************************************************************************************************************************************/
    CHECK_AND_ASSERT_MES(tx.extra.size()-1-i >= sizeof(crypto::public_key)*2+1, false, "Failed to parse transaction extra (TX_EXTRA_TAG_ALIAS have not enough bytes) in tx " << get_transaction_hash(tx));
    ++i;
    uint8_t alias_flags = tx.extra[i];
    ++i;
    uint8_t alias_name_len = tx.extra[i];
    CHECK_AND_ASSERT_MES(tx.extra.size()-1-i >= tx.extra[i]+sizeof(crypto::public_key)*2, false, "Failed to parse transaction extra (TX_EXTRA_TAG_ALIAS have wrong name bytes counter) in tx " << get_transaction_hash(tx));
    
    alinfo.m_alias.assign((const char*)&tx.extra[i+1], static_cast<size_t>(alias_name_len));
    i += tx.extra[i] + 1;
    alinfo.m_address.m_spend_public_key = *reinterpret_cast<const crypto::public_key*>(&tx.extra[i]);
    i += sizeof(const crypto::public_key);
    alinfo.m_address.m_view_public_key = *reinterpret_cast<const crypto::public_key*>(&tx.extra[i]);
    i += sizeof(const crypto::public_key);
    if(alias_flags&TX_EXTRA_TAG_ALIAS_FLAGS_ADDR_WITH_TRACK)
    {//address aliased with tracking key
      CHECK_AND_ASSERT_MES(tx.extra.size()-i >= sizeof(crypto::secret_key), false, "Failed to parse transaction extra (TX_EXTRA_TAG_ALIAS have not enough bytes) in tx " << get_transaction_hash(tx));
      alinfo.m_view_key = *reinterpret_cast<const crypto::secret_key*>(&tx.extra[i]);
      i += sizeof(const crypto::secret_key);
    }
    uint8_t comment_len = tx.extra[i];
    if(comment_len)
    {
      CHECK_AND_ASSERT_MES(tx.extra.size() - i >=tx.extra[i], false, "Failed to parse transaction extra (TX_EXTRA_TAG_ALIAS have not enough bytes) in tx " << get_transaction_hash(tx));
      alinfo.m_text_comment.assign((const char*)&tx.extra[i+1], static_cast<size_t>(tx.extra[i]));
      i += tx.extra[i] + 1;
    }
    if(alias_flags&TX_EXTRA_TAG_ALIAS_FLAGS_OP_UPDATE)
    {
      CHECK_AND_ASSERT_MES(tx.extra.size()-i >= sizeof(crypto::secret_key), false, "Failed to parse transaction extra (TX_EXTRA_TAG_ALIAS have not enough bytes) in tx " << get_transaction_hash(tx));
      alinfo.m_sign = *reinterpret_cast<const crypto::signature*>(&tx.extra[i]);
      i += sizeof(const crypto::signature);
    }
    whole_entry_len = i - start;
    return true;
  }
  //---------------------------------------------------------------
  bool parse_and_validate_tx_extra(const transaction& tx, tx_extra_info& extra)
  {
    extra.m_tx_pub_key = null_pkey;
    bool padding_started = false; //let the padding goes only at the end
    bool tx_extra_tag_pubkey_found = false;
    bool tx_extra_user_data_found = false;
    bool tx_alias_found = false;
    for(size_t i = 0; i != tx.extra.size();)
    {
      if(padding_started)
      {
        CHECK_AND_ASSERT_MES(!tx.extra[i], false, "Failed to parse transaction extra (not 0 after padding) in tx " << get_transaction_hash(tx));
      }
      else if(tx.extra[i] == TX_EXTRA_TAG_PUBKEY)
      {
        CHECK_AND_ASSERT_MES(sizeof(crypto::public_key) <= tx.extra.size()-1-i, false, "Failed to parse transaction extra (TX_EXTRA_TAG_PUBKEY have not enough bytes) in tx " << get_transaction_hash(tx));
        CHECK_AND_ASSERT_MES(!tx_extra_tag_pubkey_found, false, "Failed to parse transaction extra (duplicate TX_EXTRA_TAG_PUBKEY entry) in tx " << get_transaction_hash(tx));
        extra.m_tx_pub_key = *reinterpret_cast<const crypto::public_key*>(&tx.extra[i+1]);
        i += 1 + sizeof(crypto::public_key);
        tx_extra_tag_pubkey_found = true;
        continue;
      }else if(tx.extra[i] == TX_EXTRA_TAG_USER_DATA)
      {
        //CHECK_AND_ASSERT_MES(is_coinbase(tx), false, "Failed to parse transaction extra (TX_EXTRA_NONCE can be only in coinbase) in tx " << get_transaction_hash(tx));
        CHECK_AND_ASSERT_MES(!tx_extra_user_data_found, false, "Failed to parse transaction extra (duplicate TX_EXTRA_NONCE entry) in tx " << get_transaction_hash(tx));
        CHECK_AND_ASSERT_MES(tx.extra.size()-1-i >= 1, false, "Failed to parse transaction extra (TX_EXTRA_NONCE have not enough bytes) in tx " << get_transaction_hash(tx));
        ++i;
        CHECK_AND_ASSERT_MES(tx.extra.size()-1-i >= tx.extra[i], false, "Failed to parse transaction extra (TX_EXTRA_NONCE have wrong bytes counter) in tx " << get_transaction_hash(tx));
        tx_extra_user_data_found = true;
        if(tx.extra[i])
          extra.m_user_data_blob.assign(reinterpret_cast<const char*>(&tx.extra[i+1]), static_cast<size_t>(tx.extra[i]));
        i += tx.extra[i];//actually don't need to extract it now, just skip
      }else if(tx.extra[i] == TX_EXTRA_TAG_ALIAS)
      {
        CHECK_AND_ASSERT_MES(is_coinbase(tx), false, "Failed to parse transaction extra (TX_EXTRA_TAG_ALIAS can be only in coinbase) in tx " << get_transaction_hash(tx));
        CHECK_AND_ASSERT_MES(!tx_alias_found, false, "Failed to parse transaction extra (duplicate TX_EXTRA_TAG_ALIAS entry) in tx " << get_transaction_hash(tx));
        size_t aliac_entry_len = 0;
        if(!parse_tx_extra_alias_entry(tx, i, extra.m_alias, aliac_entry_len))
          return false;

        tx_alias_found = true;
        i += aliac_entry_len-1;
      }
      else if(!tx.extra[i])
      {
        padding_started = true;
        continue;
      }
      ++i;
    }
    return true;
  }
  //---------------------------------------------------------------
  bool parse_payment_id_from_hex_str(const std::string& payment_id_str, payment_id_t& payment_id)
  {
    return string_tools::parse_hexstr_to_binbuff(payment_id_str, payment_id);
  }
  //---------------------------------------------------------------
  bool add_tx_pub_key_to_extra(transaction& tx, const crypto::public_key& tx_pub_key)
  {
    tx.extra.resize(tx.extra.size() + 1 + sizeof(crypto::public_key));
    tx.extra[tx.extra.size() - 1 - sizeof(crypto::public_key)] = TX_EXTRA_TAG_PUBKEY;
    *reinterpret_cast<crypto::public_key*>(&tx.extra[tx.extra.size() - sizeof(crypto::public_key)]) = tx_pub_key;
    return true;
  }
  //---------------------------------------------------------------
  bool add_tx_extra_nonce(transaction& tx, const blobdata& extra_nonce)
  {
    CHECK_AND_ASSERT_MES(extra_nonce.size() <=255, false, "extra nonce could be 255 bytes max");
    size_t start_pos = tx.extra.size();
    tx.extra.resize(tx.extra.size() + 2 + extra_nonce.size());
    //write tag
    tx.extra[start_pos] = TX_EXTRA_TAG_USER_DATA;
    //write len
    ++start_pos;
    tx.extra[start_pos] = static_cast<uint8_t>(extra_nonce.size());
    //write data
    ++start_pos;
    memcpy(&tx.extra[start_pos], extra_nonce.data(), extra_nonce.size());
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
  bool construct_tx(const account_keys& keys, const create_tx_arg& arg, create_tx_res& rsp)
  {
    return construct_tx(keys, arg.sources, arg.splitted_dsts, arg.extra, rsp.tx, rsp.txkey, arg.unlock_time, arg.tx_outs_attr);
  }
  //---------------------------------------------------------------
  bool construct_tx(const account_keys& sender_account_keys, const std::vector<tx_source_entry>& sources, 
                                                             const std::vector<tx_destination_entry>& destinations, 
                                                             transaction& tx,
                                                             keypair& txkey,
                                                             uint64_t unlock_time, 
                                                             uint8_t tx_outs_attr)
  {
    return construct_tx(sender_account_keys, sources, destinations, std::vector<uint8_t>(), tx, txkey, unlock_time, tx_outs_attr);
  }
  bool construct_tx(const account_keys& sender_account_keys, const std::vector<tx_source_entry>& sources, 
                                                             const std::vector<tx_destination_entry>& destinations, 
                                                             const std::vector<uint8_t>& extra,
                                                             transaction& tx,
                                                             keypair& txkey,
                                                             uint64_t unlock_time,
                                                             uint8_t tx_outs_attr)
  {
    tx.vin.clear();
    tx.vout.clear();
    tx.signatures.clear();
    tx.extra = extra;

    tx.version = CURRENT_TRANSACTION_VERSION;
    tx.unlock_time = unlock_time;

    txkey = keypair::generate();
    add_tx_pub_key_to_extra(tx, txkey.pub);

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
    CHECK_AND_ASSERT_MES(b.miner_tx.vin.size() == 1, 0, "wrong miner tx in block: " << get_block_hash(b) << ", b.miner_tx.vin.size() != 1");
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
  bool set_payment_id_to_tx_extra(std::vector<uint8_t>& extra, const payment_id_t& payment_id)
  {
    if(!payment_id.size() || payment_id.size() >= TX_MAX_PAYMENT_ID_SIZE)
      return false;

    if(std::find(extra.begin(), extra.end(), TX_EXTRA_TAG_USER_DATA) != extra.end())
      return false;

    extra.push_back(TX_EXTRA_TAG_USER_DATA);
    extra.push_back(static_cast<uint8_t>(payment_id.size()+2));
    extra.push_back(TX_USER_DATA_TAG_PAYMENT_ID);
    extra.push_back(static_cast<uint8_t>(payment_id.size()));
    
    const uint8_t* payment_id_ptr = reinterpret_cast<const uint8_t*>(payment_id.data());
    std::copy(payment_id_ptr, payment_id_ptr + payment_id.size(), std::back_inserter(extra));
    return true;
  }
  //---------------------------------------------------------------
  bool get_payment_id_from_user_data(const std::string& user_data, payment_id_t& payment_id)
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
  crypto::hash get_blob_longhash(const blobdata& bd, uint64_t height, const std::vector<crypto::hash>& scratchpad)
  {
    crypto::hash h = null_hash;
    get_blob_longhash(bd, h, height, [&](uint64_t index) -> const crypto::hash&
    {
      return scratchpad[index%scratchpad.size()];
    });
    return h;
  }
  //---------------------------------------------------------------
  crypto::hash get_blob_longhash_opt(const std::string& blob, const std::vector<crypto::hash>& scratchpad)
  {
    if(!scratchpad.size())
      return get_blob_longhash(blob, 0, scratchpad);
    crypto::hash h2 = null_hash;
    crypto::wild_keccak_dbl_opt(reinterpret_cast<const uint8_t*>(&blob[0]), blob.size(), reinterpret_cast<uint8_t*>(&h2), sizeof(h2), (const UINT64*)&scratchpad[0], scratchpad.size()*4);
    return h2;
  }


  //------------------------------------------------------------------
  bool validate_alias_name(const std::string& al)
  {
    CHECK_AND_ASSERT_MES(al.size() <= MAX_ALIAS_LEN, false, "Too long alias name, please use name no longer than " << MAX_ALIAS_LEN );
    /*allowed symbols "a-z", "-", "." */
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
  std::string dump_scratchpad(const std::vector<crypto::hash>& scr)
  {
    std::stringstream ss;
    for(size_t i = 0; i!=scr.size(); i++)
    {
      ss << "[" << i << "]" << scr[i] << ENDL;
    }
    return ss.str();
  }
  //------------------------------------------------------------------
  bool addendum_to_hexstr(const std::vector<crypto::hash>& add, std::string& hex_buff)
  {
    for(const auto& h: add)
      hex_buff += string_tools::pod_to_hex(h);
    return true;
  }
  //------------------------------------------------------------------
  bool hexstr_to_addendum(const std::string& hex_buff, std::vector<crypto::hash>& add)
  {
    CHECK_AND_ASSERT_MES(!(hex_buff.size()%(sizeof(crypto::hash)*2)), false, "wrong hex_buff size=" << hex_buff.size());
    for(size_t i = 0; i + sizeof(crypto::hash)*2 <= hex_buff.size(); i += sizeof(crypto::hash)*2)
    {
      crypto::hash h = null_hash;
      bool r = string_tools::hex_to_pod(hex_buff.substr(i, sizeof(crypto::hash)*2), h);
      CHECK_AND_ASSERT_MES(r, false, "wrong scratchpad hex buff: " << hex_buff.substr(i, sizeof(crypto::hash)*2));
      add.push_back(h);
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

    uint64_t already_generated_coins = 0;
    uint64_t already_donated_coins = 0;
    
    uint64_t total_money_supply = TOTAL_MONEY_SUPPLY;
    for(uint64_t day = 0; day != 365*10; ++day)
    {
      uint64_t emission_reward = 0;
      uint64_t stub = 0;
      get_block_reward(0, 0, already_generated_coins, 0, emission_reward, stub);
      if(!(day%183))
      {
        std::cout << std::left 
          << std::setw(10) << day 
          << std::setw(19) << print_money(emission_reward) 
          << std::setw(4) << std::string(std::to_string(GET_PERECENTS_BIG_NUMBERS((already_generated_coins + already_donated_coins), total_money_supply)) + "%") 
          << print_money(already_generated_coins + already_donated_coins) 
          << std::endl;
      }

      
      for(size_t i = 0; i != 720; i++)
      {
        get_block_reward(0, 0, already_generated_coins, 0, emission_reward, stub);
        already_generated_coins += emission_reward;
      }
      std::vector<bool> votes(CURRENCY_DONATIONS_INTERVAL, true);
      uint64_t max_possible_donation_reward = get_donations_anount_for_day(already_donated_coins, votes);
      already_donated_coins += max_possible_donation_reward;
    }
  }
  //------------------------------------------------------------------
  void print_currency_details()
  {   
    //for future forks 
    
    std::cout << "Currency name: \t\t" << CURRENCY_NAME <<"(" << CURRENCY_NAME_SHORT << ")" << std::endl;
    std::cout << "Money supply: \t\t" << print_money(TOTAL_MONEY_SUPPLY) << " coins"
                  << "(" << print_money(EMISSION_SUPPLY) << " + " << print_money(DONATIONS_SUPPLY) << "), dev bounties is " << GET_PERECENTS_BIG_NUMBERS((DONATIONS_SUPPLY+ANTI_OVERFLOW_AMOUNT), (TOTAL_MONEY_SUPPLY)) << "%" << std::endl;

    std::cout << "Block interval: \t" << DIFFICULTY_TARGET << " seconds" << std::endl;
    std::cout << "Default p2p port: \t" << P2P_DEFAULT_PORT << std::endl;
    std::cout << "Default rpc port: \t" << RPC_DEFAULT_PORT << std::endl;
    print_reward_halwing();
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
    
    /*account_public_address ac = boost::value_initialized<account_public_address>();
    std::vector<size_t> sz;
    //proof 
#ifndef TESTNET
    std::string proof = "The Times, May 16 2014: Richest 10% own almost half the nation's wealth";
#else 
    std::string proof = "The Times, May 13 2014: Fear of public exposure shames stars into paying tax";
#endif

    alias_info ai = AUTO_VAL_INIT(ai);
    ai.m_alias = "zoidberg";
    ai.m_text_comment = "Let's go!";
    get_account_address_from_str(ai.m_address, "1HNJjUsofq5LYLoXem119dd491yFAb5g4bCHkecV4sPqigmuxw57Ci9am71fEN4CRmA9jgnvo5PDNfaq8QnprWmS5uLqnbq"); 
    construct_miner_tx(0, 0, 0, 0, 0, 0, ac, ac, ac, bl.miner_tx, proof, 11, 0, ai); // zero profit in genesis
    blobdata txb = tx_to_blob(bl.miner_tx);
    std::string hex_tx_represent = string_tools::buff_to_hex_nodelimer(txb);*/
    
    //hard code coinbase tx in genesis block, because "true" generating tx use random, but genesis should be always the same
#ifndef TESTNET
    std::string genesis_coinbase_tx_hex = "010a01ff00088092f401029b2e4c0281c0b02e7c53291a94d1d0cbff8883f8024f5142ee494ffbbd08807100808ece1c022a74a3c4c36d32e95633d44ba9a7b8188297b2ac91afecab826b86fabaa70916008084af5f0252d128bc9913d5ee8b702c37609917c2357b2f587e5de5622348a3acd718e5d60080f882ad1602b8ed916c56b3a99c9cdf22c7be7ec4e85587e5d40bc46bf6995313c288ad841e0080c8afa025021b452b4ac6c6419e06181f8c9f0734bd5bb132d8b75b44bbcd07dd8f553acba60080c0ee8ed20b02b10ba13e303cbe9abf7d5d44f1d417727abcc14903a74e071abd652ce1bf76dd0080e08d84ddcb010205e440069d10646f1bbfaeee88a2db218017941c5fa7280849126d2372fc64340080c0caf384a302029cad2882bba92fb7ecc8136475dae03169839eee05ff3ee3232d0136712f08b700bf0101bb53d7b4504db7dae116ef7f13e636fca8f0f62598ff286e9ed97a1719f957ea02475468652054696d65732c204d617920313620323031343a205269636865737420313025206f776e20616c6d6f73742068616c6620746865206e6174696f6e2773207765616c74680300087a6f696462657267afe8323edbd46c74d3010d32e98454d78dad266b8e5f09cc6fb5ae058e080cf9391048c006da8ec9d71d379037ff9036b53e62693bf045e6ac9fc44605f71d2b094c6574277320676f2100";
#else 
    std::string genesis_coinbase_tx_hex = "010a01ff00088092f401029b2e4c0281c0b02e7c53291a94d1d0cbff8883f8024f5142ee494ffbbd08807100808ece1c022a74a3c4c36d32e95633d44ba9a7b8188297b2ac91afecab826b86fabaa70916008084af5f0252d128bc9913d5ee8b702c37609917c2357b2f587e5de5622348a3acd718e5d60080f882ad1602b8ed916c56b3a99c9cdf22c7be7ec4e85587e5d40bc46bf6995313c288ad841e0080c8afa025021b452b4ac6c6419e06181f8c9f0734bd5bb132d8b75b44bbcd07dd8f553acba60080c0ee8ed20b02b10ba13e303cbe9abf7d5d44f1d417727abcc14903a74e071abd652ce1bf76dd0080e08d84ddcb010205e440069d10646f1bbfaeee88a2db218017941c5fa7280849126d2372fc64340080c0caf384a302029cad2882bba92fb7ecc8136475dae03169839eee05ff3ee3232d0136712f08b700c401013696374739ea10a92aeed86b210c57c5b3540991f335b9f7686c5aef40a716b3024c5468652054696d65732c204d617920313320323031343a2046656172206f66207075626c6963206578706f73757265207368616d657320737461727320696e746f20706179696e67207461780300087a6f696462657267afe8323edbd46c74d3010d32e98454d78dad266b8e5f09cc6fb5ae058e080cf9391048c006da8ec9d71d379037ff9036b53e62693bf045e6ac9fc44605f71d2b094c6574277320676f2100";                                          
#endif

    blobdata tx_bl;
    string_tools::parse_hexstr_to_binbuff(genesis_coinbase_tx_hex, tx_bl);
    bool r = parse_and_validate_tx_from_blob(tx_bl, bl.miner_tx);
    CHECK_AND_ASSERT_MES(r, false, "failed to parse coinbase tx from hard coded blob");
    bl.major_version = CURRENT_BLOCK_MAJOR_VERSION;
    bl.minor_version = CURRENT_BLOCK_MINOR_VERSION;
    bl.timestamp = 0;
#ifndef TESTNET
    bl.nonce = 1010101020; //bender's nightmare
#else
    bl.nonce = 1010101012; //another bender's nightmare
#endif
    //miner::find_nonce_for_given_block(bl, 1, 0,);
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
}
