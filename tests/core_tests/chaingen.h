// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <vector>
#include <iostream>
#include <stdint.h>

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/program_options.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/variant.hpp>

#include "include_base_utils.h"
#include "common/boost_serialization_helper.h"
#include "common/command_line.h"

#include "currency_core/account_boost_serialization.h"
#include "currency_core/currency_basic.h"
#include "currency_core/currency_basic_impl.h"
#include "currency_core/currency_format_utils.h"
#include "currency_core/currency_core.h"
#include "currency_core/currency_boost_serialization.h"
#include "misc_language.h"


namespace concolor
{
  inline std::basic_ostream<char, std::char_traits<char> >& bright_white(std::basic_ostream<char, std::char_traits<char> >& ostr)
  {
    epee::log_space::set_console_color(epee::log_space::console_color_white, true);
    return ostr;
  }

  inline std::basic_ostream<char, std::char_traits<char> >& red(std::basic_ostream<char, std::char_traits<char> >& ostr)
  {
    epee::log_space::set_console_color(epee::log_space::console_color_red, true);
    return ostr;
  }

  inline std::basic_ostream<char, std::char_traits<char> >& green(std::basic_ostream<char, std::char_traits<char> >& ostr)
  {
    epee::log_space::set_console_color(epee::log_space::console_color_green, true);
    return ostr;
  }

  inline std::basic_ostream<char, std::char_traits<char> >& magenta(std::basic_ostream<char, std::char_traits<char> >& ostr)
  {
    epee::log_space::set_console_color(epee::log_space::console_color_magenta, true);
    return ostr;
  }

  inline std::basic_ostream<char, std::char_traits<char> >& yellow(std::basic_ostream<char, std::char_traits<char> >& ostr)
  {
    epee::log_space::set_console_color(epee::log_space::console_color_yellow, true);
    return ostr;
  }

  inline std::basic_ostream<char, std::char_traits<char> >& normal(std::basic_ostream<char, std::char_traits<char> >& ostr)
  {
    epee::log_space::reset_console_color();
    return ostr;
  }
}


struct callback_entry
{
  std::string callback_name;
  BEGIN_SERIALIZE_OBJECT()
    FIELD(callback_name)
  END_SERIALIZE()

private:
  friend class boost::serialization::access;

  template<class Archive>
  void serialize(Archive & ar, const unsigned int /*version*/)
  {
    ar & callback_name;
  }
};

template<typename T>
struct serialized_object
{
  serialized_object() { }

  serialized_object(const currency::blobdata& a_data)
    : data(a_data)
  {
  }

  currency::blobdata data;
  BEGIN_SERIALIZE_OBJECT()
    FIELD(data)
    END_SERIALIZE()

private:
  friend class boost::serialization::access;

  template<class Archive>
  void serialize(Archive & ar, const unsigned int /*version*/)
  {
    ar & data;
  }
};

typedef serialized_object<currency::block> serialized_block;
typedef serialized_object<currency::transaction> serialized_transaction;

struct event_visitor_settings
{
  int valid_mask;
  bool txs_keeped_by_block;

  enum settings
  {
    set_txs_keeped_by_block = 1 << 0
  };

  event_visitor_settings(int a_valid_mask = 0, bool a_txs_keeped_by_block = false)
    : valid_mask(a_valid_mask)
    , txs_keeped_by_block(a_txs_keeped_by_block)
  {
  }

private:
  friend class boost::serialization::access;

  template<class Archive>
  void serialize(Archive & ar, const unsigned int /*version*/)
  {
    ar & valid_mask;
    ar & txs_keeped_by_block;
  }
};

VARIANT_TAG(binary_archive, callback_entry, 0xcb);
VARIANT_TAG(binary_archive, currency::account_base, 0xcc);
VARIANT_TAG(binary_archive, serialized_block, 0xcd);
VARIANT_TAG(binary_archive, serialized_transaction, 0xce);
VARIANT_TAG(binary_archive, event_visitor_settings, 0xcf);

typedef boost::variant<currency::block, currency::transaction, currency::account_base, callback_entry, serialized_block, serialized_transaction, event_visitor_settings> test_event_entry;
typedef std::unordered_map<crypto::hash, const currency::transaction*> map_hash2tx_t;

class test_chain_unit_base
{
public:
  typedef boost::function<bool (currency::core& c, size_t ev_index, const std::vector<test_event_entry> &events)> verify_callback;
  typedef std::map<std::string, verify_callback> callbacks_map;

  void register_callback(const std::string& cb_name, verify_callback cb);
  bool verify(const std::string& cb_name, currency::core& c, size_t ev_index, const std::vector<test_event_entry> &events);
private:
  callbacks_map m_callbacks;
};


class test_generator
{
public:
  struct block_info
  {
    block_info()
      : b()
      , already_generated_coins(0)
      , already_donated(0)
      , block_size(0)
      , cumul_difficulty(0)
    {
    }

    block_info(const currency::block& b_, uint64_t an_already_generated_coins, uint64_t an_already_donated_coins, size_t a_block_size, currency::wide_difficulty_type diff)
      : b(b_)
      , already_generated_coins(an_already_generated_coins)
      , already_donated(an_already_donated_coins)
      , block_size(a_block_size)
      , cumul_difficulty(diff)
    {
    }

    currency::block b;
    uint64_t already_generated_coins;
    uint64_t already_donated;
    size_t block_size;
    currency::wide_difficulty_type cumul_difficulty;
  };

  enum block_fields
  {
    bf_none      = 0,
    bf_major_ver = 1 << 0,
    bf_minor_ver = 1 << 1,
    bf_timestamp = 1 << 2,
    bf_prev_id   = 1 << 3,
    bf_miner_tx  = 1 << 4,
    bf_tx_hashes = 1 << 5,
    bf_diffic    = 1 << 6
  };

  currency::wide_difficulty_type get_difficulty_for_next_block(const std::vector<const block_info*>& blocks);
  currency::wide_difficulty_type get_difficulty_for_next_block(const crypto::hash& head_id);
  void get_block_chain(std::vector<const block_info*>& blockchain, const crypto::hash& head, size_t n) const;
  void get_last_n_block_sizes(std::vector<size_t>& block_sizes, const crypto::hash& head, size_t n) const;
  
  uint64_t get_already_donated_coins(const crypto::hash& blk_id) const;
  uint64_t get_already_generated_coins(const crypto::hash& blk_id) const;
  uint64_t get_already_generated_coins(const currency::block& blk) const;
  currency::wide_difficulty_type get_block_difficulty(const crypto::hash& blk_id) const;

  void add_block(const currency::block& blk, size_t tsx_size, std::vector<size_t>& block_sizes, uint64_t already_generated_coins, uint64_t already_donated_coins, currency::wide_difficulty_type cum_diff);
  bool construct_block(currency::block& blk, uint64_t height, const crypto::hash& prev_id,
    const currency::account_base& miner_acc, uint64_t timestamp, uint64_t already_generated_coins, uint64_t already_donated_coins, 
    std::vector<size_t>& block_sizes, const std::list<currency::transaction>& tx_list, const currency::alias_info& ai = currency::alias_info());
  bool construct_block(currency::block& blk, const currency::account_base& miner_acc, uint64_t timestamp, const currency::alias_info& ai = currency::alias_info());
  bool construct_block(currency::block& blk, const currency::block& blk_prev, const currency::account_base& miner_acc,
    const std::list<currency::transaction>& tx_list = std::list<currency::transaction>(), const currency::alias_info& ai = currency::alias_info());

  bool construct_block_manually(currency::block& blk, const currency::block& prev_block,
    const currency::account_base& miner_acc, int actual_params = bf_none, uint8_t major_ver = 0,
    uint8_t minor_ver = 0, uint64_t timestamp = 0, const crypto::hash& prev_id = crypto::hash(),
    const currency::wide_difficulty_type& diffic = 1, const currency::transaction& miner_tx = currency::transaction(),
    const std::vector<crypto::hash>& tx_hashes = std::vector<crypto::hash>(), size_t txs_sizes = 0);
  bool construct_block_manually_tx(currency::block& blk, const currency::block& prev_block,
    const currency::account_base& miner_acc, const std::vector<crypto::hash>& tx_hashes, size_t txs_size);
  bool find_nounce(currency::block& blk, std::vector<const block_info*>& blocks, currency::wide_difficulty_type dif, uint64_t height);
  //bool find_nounce(currency::block& blk, currency::wide_difficulty_type dif, uint64_t height);

private:
  std::unordered_map<crypto::hash, block_info> m_blocks_info;
};

inline currency::wide_difficulty_type get_test_difficulty() {return 1;}
void fill_nonce(currency::block& blk, const currency::wide_difficulty_type& diffic, uint64_t height);

bool construct_miner_tx_manually(size_t height, uint64_t already_generated_coins,
                                 const currency::account_public_address& miner_address, currency::transaction& tx,
                                 uint64_t fee, currency::keypair* p_txkey = 0);
bool construct_tx_to_key(const std::vector<test_event_entry>& events, currency::transaction& tx,
                         const currency::block& blk_head, const currency::account_base& from, const currency::account_base& to,
                         uint64_t amount, uint64_t fee, size_t nmix, uint8_t mix_attr = CURRENCY_TO_KEY_OUT_RELAXED, bool check_for_spends = true);
currency::transaction construct_tx_with_fee(std::vector<test_event_entry>& events, const currency::block& blk_head,
                                            const currency::account_base& acc_from, const currency::account_base& acc_to,
                                            uint64_t amount, uint64_t fee);

void get_confirmed_txs(const std::vector<currency::block>& blockchain, const map_hash2tx_t& mtx, map_hash2tx_t& confirmed_txs);
bool find_block_chain(const std::vector<test_event_entry>& events, std::vector<currency::block>& blockchain, map_hash2tx_t& mtx, const crypto::hash& head);
void fill_tx_sources_and_destinations(const std::vector<test_event_entry>& events, const currency::block& blk_head,
                                      const currency::account_base& from, const currency::account_base& to,
                                      uint64_t amount, uint64_t fee, size_t nmix,
                                      std::vector<currency::tx_source_entry>& sources,
                                      std::vector<currency::tx_destination_entry>& destinations, 
                                      bool check_for_spends = true);
uint64_t get_balance(const currency::account_base& addr, const std::vector<currency::block>& blockchain, const map_hash2tx_t& mtx);

//--------------------------------------------------------------------------
template<class t_test_class>
auto do_check_tx_verification_context(const currency::tx_verification_context& tvc, bool tx_added, size_t event_index, const currency::transaction& tx, t_test_class& validator, int)
  -> decltype(validator.check_tx_verification_context(tvc, tx_added, event_index, tx))
{
  return validator.check_tx_verification_context(tvc, tx_added, event_index, tx);
}
//--------------------------------------------------------------------------
template<class t_test_class>
bool do_check_tx_verification_context(const currency::tx_verification_context& tvc, bool tx_added, size_t /*event_index*/, const currency::transaction& /*tx*/, t_test_class&, long)
{
  // Default block verification context check
  if (tvc.m_verifivation_failed)
    throw std::runtime_error("Transaction verification failed");
  return true;
}
//--------------------------------------------------------------------------
template<class t_test_class>
bool check_tx_verification_context(const currency::tx_verification_context& tvc, bool tx_added, size_t event_index, const currency::transaction& tx, t_test_class& validator)
{
  // SFINAE in action
  return do_check_tx_verification_context(tvc, tx_added, event_index, tx, validator, 0);
}
//--------------------------------------------------------------------------
template<class t_test_class>
auto do_check_block_verification_context(const currency::block_verification_context& bvc, size_t event_index, const currency::block& blk, t_test_class& validator, int)
  -> decltype(validator.check_block_verification_context(bvc, event_index, blk))
{
  return validator.check_block_verification_context(bvc, event_index, blk);
}
//--------------------------------------------------------------------------
template<class t_test_class>
bool do_check_block_verification_context(const currency::block_verification_context& bvc, size_t /*event_index*/, const currency::block& /*blk*/, t_test_class&, long)
{
  // Default block verification context check
  if (bvc.m_verifivation_failed)
    throw std::runtime_error("Block verification failed");
  return true;
}
//--------------------------------------------------------------------------
template<class t_test_class>
bool check_block_verification_context(const currency::block_verification_context& bvc, size_t event_index, const currency::block& blk, t_test_class& validator)
{
  // SFINAE in action
  return do_check_block_verification_context(bvc, event_index, blk, validator, 0);
}

/************************************************************************/
/*                                                                      */
/************************************************************************/
template<class t_test_class>
struct push_core_event_visitor: public boost::static_visitor<bool>
{
private:
  currency::core& m_c;
  const std::vector<test_event_entry>& m_events;
  t_test_class& m_validator;
  size_t m_ev_index;

  bool m_txs_keeped_by_block;

public:
  push_core_event_visitor(currency::core& c, const std::vector<test_event_entry>& events, t_test_class& validator)
    : m_c(c)
    , m_events(events)
    , m_validator(validator)
    , m_ev_index(0)
    , m_txs_keeped_by_block(false)
  {
  }

  void event_index(size_t ev_index)
  {
    m_ev_index = ev_index;
  }

  bool operator()(const event_visitor_settings& settings)
  {
    log_event("event_visitor_settings");

    if (settings.valid_mask & event_visitor_settings::set_txs_keeped_by_block)
    {
      m_txs_keeped_by_block = settings.txs_keeped_by_block;
    }

    return true;
  }

  bool operator()(const currency::transaction& tx) const
  {
    log_event("currency::transaction");

    currency::tx_verification_context tvc = AUTO_VAL_INIT(tvc);
    size_t pool_size = m_c.get_pool_transactions_count();
    m_c.handle_incoming_tx(t_serializable_object_to_blob(tx), tvc, m_txs_keeped_by_block);
    bool tx_added = pool_size + 1 == m_c.get_pool_transactions_count();
    bool r = check_tx_verification_context(tvc, tx_added, m_ev_index, tx, m_validator);
    CHECK_AND_NO_ASSERT_MES(r, false, "tx verification context check failed");
    return true;
  }

  bool operator()(const currency::block& b) const
  {
    log_event("currency::block");

    currency::block_verification_context bvc = AUTO_VAL_INIT(bvc);
    m_c.handle_incoming_block(t_serializable_object_to_blob(b), bvc);
    bool r = check_block_verification_context(bvc, m_ev_index, b, m_validator);
    CHECK_AND_NO_ASSERT_MES(r, false, "block verification context check failed");
    return r;
  }

  bool operator()(const callback_entry& cb) const
  {
    log_event(std::string("callback_entry ") + cb.callback_name);
    return m_validator.verify(cb.callback_name, m_c, m_ev_index, m_events);
  }

  bool operator()(const currency::account_base& ab) const
  {
    log_event("currency::account_base");
    return true;
  }

  bool operator()(const serialized_block& sr_block) const
  {
    log_event("serialized_block");

    currency::block_verification_context bvc = AUTO_VAL_INIT(bvc);
    m_c.handle_incoming_block(sr_block.data, bvc);

    currency::block blk;
    std::stringstream ss;
    ss << sr_block.data;
    binary_archive<false> ba(ss);
    ::serialization::serialize(ba, blk);
    if (!ss.good())
    {
      blk = currency::block();
    }
    bool r = check_block_verification_context(bvc, m_ev_index, blk, m_validator);
    CHECK_AND_NO_ASSERT_MES(r, false, "block verification context check failed");
    return true;
  }

  bool operator()(const serialized_transaction& sr_tx) const
  {
    log_event("serialized_transaction");

    currency::tx_verification_context tvc = AUTO_VAL_INIT(tvc);
    size_t pool_size = m_c.get_pool_transactions_count();
    m_c.handle_incoming_tx(sr_tx.data, tvc, m_txs_keeped_by_block);
    bool tx_added = pool_size + 1 == m_c.get_pool_transactions_count();

    currency::transaction tx;
    std::stringstream ss;
    ss << sr_tx.data;
    binary_archive<false> ba(ss);
    ::serialization::serialize(ba, tx);
    if (!ss.good())
    {
      tx = currency::transaction();
    }

    bool r = check_tx_verification_context(tvc, tx_added, m_ev_index, tx, m_validator);
    CHECK_AND_NO_ASSERT_MES(r, false, "transaction verification context check failed");
    return true;
  }

private:
  void log_event(const std::string& event_type) const
  {
    std::cout << concolor::yellow << "=== EVENT # " << m_ev_index << ": " << event_type << concolor::normal << std::endl;
  }
};
//--------------------------------------------------------------------------
template<class t_test_class>
inline bool replay_events_through_core(currency::core& cr, const std::vector<test_event_entry>& events, t_test_class& validator)
{
  TRY_ENTRY();

  //init core here

  CHECK_AND_ASSERT_MES(typeid(currency::block) == events[0].type(), false, "First event must be genesis block creation");
  cr.set_genesis_block(boost::get<currency::block>(events[0]));

  bool r = true;
  push_core_event_visitor<t_test_class> visitor(cr, events, validator);
  for(size_t i = 1; i < events.size() && r; ++i)
  {
    visitor.event_index(i);
    r = boost::apply_visitor(visitor, events[i]);
  }

  return r;

  CATCH_ENTRY_L0("replay_events_through_core", false);
}
//--------------------------------------------------------------------------
template<class t_test_class>
inline bool do_replay_events(std::vector<test_event_entry>& events)
{
  boost::program_options::options_description desc("Allowed options");
  currency::core::init_options(desc);
  command_line::add_arg(desc, command_line::arg_data_dir);
  boost::program_options::variables_map vm;
  bool r = command_line::handle_error_helper(desc, [&]()
  {
    boost::program_options::store(boost::program_options::basic_parsed_options<char>(&desc), vm);
    boost::program_options::notify(vm);
    return true;
  });
  if (!r)
    return false;

  currency::currency_protocol_stub pr; //TODO: stub only for this kind of test, make real validation of relayed objects
  currency::core c(&pr);
  if (!c.init(vm))
  {
    std::cout << concolor::magenta << "Failed to init core" << concolor::normal << std::endl;
    return false;
  }
  t_test_class validator;
  return replay_events_through_core<t_test_class>(c, events, validator);
}
//--------------------------------------------------------------------------
template<class t_test_class>
inline bool do_replay_file(const std::string& filename)
{
  std::vector<test_event_entry> events;
  if (!tools::unserialize_obj_from_file(events, filename))
  {
    std::cout << concolor::magenta << "Failed to deserialize data from file: " << filename << concolor::normal << std::endl;
    return false;
  }
  return do_replay_events<t_test_class>(events);
}
//--------------------------------------------------------------------------
#define GENERATE_ACCOUNT(account) \
    currency::account_base account; \
    account.generate();

#define MAKE_ACCOUNT(VEC_EVENTS, account) \
  currency::account_base account; \
  account.generate(); \
  VEC_EVENTS.push_back(account);

#define DO_CALLBACK(VEC_EVENTS, CB_NAME) \
{ \
  callback_entry CALLBACK_ENTRY; \
  CALLBACK_ENTRY.callback_name = CB_NAME; \
  VEC_EVENTS.push_back(CALLBACK_ENTRY); \
}

#define REGISTER_CALLBACK(CB_NAME, CLBACK) \
  register_callback(CB_NAME, boost::bind(&CLBACK, this, _1, _2, _3));

#define REGISTER_CALLBACK_METHOD(CLASS, METHOD) \
  register_callback(#METHOD, boost::bind(&CLASS::METHOD, this, _1, _2, _3));

#define MAKE_GENESIS_BLOCK(VEC_EVENTS, BLK_NAME, MINER_ACC, TS)                       \
  test_generator generator;                                                           \
  currency::block BLK_NAME = AUTO_VAL_INIT(BLK_NAME);                                 \
  generator.construct_block(BLK_NAME, MINER_ACC, TS);                                 \
  VEC_EVENTS.push_back(BLK_NAME);

#define MAKE_NEXT_BLOCK(VEC_EVENTS, BLK_NAME, PREV_BLOCK, MINER_ACC)                  \
  currency::block BLK_NAME = AUTO_VAL_INIT(BLK_NAME);                                 \
  generator.construct_block(BLK_NAME, PREV_BLOCK, MINER_ACC);                         \
  VEC_EVENTS.push_back(BLK_NAME);

#define MAKE_NEXT_BLOCK_ALIAS(VEC_EVENTS, BLK_NAME, PREV_BLOCK, MINER_ACC, ALIAS)     \
  currency::block BLK_NAME = AUTO_VAL_INIT(BLK_NAME);                                 \
  generator.construct_block(BLK_NAME, PREV_BLOCK, MINER_ACC,                          \
                  std::list<currency::transaction>(), ALIAS);                         \
  VEC_EVENTS.push_back(BLK_NAME);


#define MAKE_NEXT_BLOCK_TX1(VEC_EVENTS, BLK_NAME, PREV_BLOCK, MINER_ACC, TX1)         \
  currency::block BLK_NAME = AUTO_VAL_INIT(BLK_NAME);                                 \
  {                                                                                   \
    std::list<currency::transaction> tx_list;                                         \
    tx_list.push_back(TX1);                                                           \
    generator.construct_block(BLK_NAME, PREV_BLOCK, MINER_ACC, tx_list);              \
  }                                                                                   \
  VEC_EVENTS.push_back(BLK_NAME);

#define MAKE_NEXT_BLOCK_TX_LIST(VEC_EVENTS, BLK_NAME, PREV_BLOCK, MINER_ACC, TXLIST)  \
  currency::block BLK_NAME = AUTO_VAL_INIT(BLK_NAME);                                 \
  generator.construct_block(BLK_NAME, PREV_BLOCK, MINER_ACC, TXLIST);                 \
  VEC_EVENTS.push_back(BLK_NAME);

#define REWIND_BLOCKS_N(VEC_EVENTS, BLK_NAME, PREV_BLOCK, MINER_ACC, COUNT)           \
  currency::block BLK_NAME = AUTO_VAL_INIT(BLK_NAME);                                 \
  {                                                                                   \
    currency::block blk_last = PREV_BLOCK;                                            \
    for (size_t i = 0; i < COUNT; ++i)                                                \
    {                                                                                 \
      MAKE_NEXT_BLOCK(VEC_EVENTS, blk, blk_last, MINER_ACC);                          \
      blk_last = blk;                                                                 \
    }                                                                                 \
    BLK_NAME = blk_last;                                                              \
  }

#define REWIND_BLOCKS(VEC_EVENTS, BLK_NAME, PREV_BLOCK, MINER_ACC) REWIND_BLOCKS_N(VEC_EVENTS, BLK_NAME, PREV_BLOCK, MINER_ACC, CURRENCY_MINED_MONEY_UNLOCK_WINDOW)


#define MAKE_TX_MIX_ATTR(VEC_EVENTS, TX_NAME, FROM, TO, AMOUNT, NMIX, HEAD, MIX_ATTR, CHECK_SPENDS)                   \
  currency::transaction TX_NAME;                                                                                      \
  construct_tx_to_key(VEC_EVENTS, TX_NAME, HEAD, FROM, TO, AMOUNT, TESTS_DEFAULT_FEE, NMIX, MIX_ATTR, CHECK_SPENDS);  \
  VEC_EVENTS.push_back(TX_NAME);

#define MAKE_TX_MIX(VEC_EVENTS, TX_NAME, FROM, TO, AMOUNT, NMIX, HEAD)   MAKE_TX_MIX_ATTR(VEC_EVENTS, TX_NAME, FROM, TO, AMOUNT, NMIX, HEAD, CURRENCY_TO_KEY_OUT_RELAXED, true)





#define MAKE_TX(VEC_EVENTS, TX_NAME, FROM, TO, AMOUNT, HEAD) MAKE_TX_MIX(VEC_EVENTS, TX_NAME, FROM, TO, AMOUNT, 0, HEAD)


#define MAKE_TX_MIX_LIST_MIX_ATTR(VEC_EVENTS, SET_NAME, FROM, TO, AMOUNT, NMIX, HEAD, MIX_ATTR)    \
  {                                                                                                \
    currency::transaction t;                                                                       \
    construct_tx_to_key(VEC_EVENTS, t, HEAD, FROM, TO, AMOUNT, TESTS_DEFAULT_FEE, NMIX, MIX_ATTR); \
    SET_NAME.push_back(t);                                                                         \
    VEC_EVENTS.push_back(t);                                                                       \
  }

#define MAKE_TX_MIX_LIST(VEC_EVENTS, SET_NAME, FROM, TO, AMOUNT, NMIX, HEAD) MAKE_TX_MIX_LIST_MIX_ATTR(VEC_EVENTS, SET_NAME, FROM, TO, AMOUNT, NMIX, HEAD, CURRENCY_TO_KEY_OUT_RELAXED)

#define MAKE_TX_LIST(VEC_EVENTS, SET_NAME, FROM, TO, AMOUNT, HEAD) MAKE_TX_MIX_LIST(VEC_EVENTS, SET_NAME, FROM, TO, AMOUNT, 0, HEAD)

#define MAKE_TX_LIST_MIX_ATTR(VEC_EVENTS, SET_NAME, FROM, TO, AMOUNT, HEAD, MIX_ATTR) MAKE_TX_MIX_LIST_MIX_ATTR(VEC_EVENTS, SET_NAME, FROM, TO, AMOUNT, 0, HEAD, MIX_ATTR)

#define MAKE_TX_LIST_START_MIX_ATTR(VEC_EVENTS, SET_NAME, FROM, TO, AMOUNT, HEAD, MIX_ATTR) \
    std::list<currency::transaction> SET_NAME; \
    MAKE_TX_MIX_LIST_MIX_ATTR(VEC_EVENTS, SET_NAME, FROM, TO, AMOUNT, 0, HEAD, MIX_ATTR);

#define MAKE_TX_LIST_START(VEC_EVENTS, SET_NAME, FROM, TO, AMOUNT, HEAD) MAKE_TX_LIST_START_MIX_ATTR(VEC_EVENTS, SET_NAME, FROM, TO, AMOUNT, HEAD, CURRENCY_TO_KEY_OUT_RELAXED)


#define MAKE_MINER_TX_AND_KEY_MANUALLY(TX, BLK, KEY)                                                      \
  transaction TX;                                                                                         \
  if (!construct_miner_tx_manually(get_block_height(BLK) + 1, generator.get_already_generated_coins(BLK), \
    miner_account.get_keys().m_account_address, TX, 0, KEY))                                              \
    return false;

#define MAKE_MINER_TX_MANUALLY(TX, BLK) MAKE_MINER_TX_AND_KEY_MANUALLY(TX, BLK, 0)

#define SET_EVENT_VISITOR_SETT(VEC_EVENTS, SETT, VAL) VEC_EVENTS.push_back(event_visitor_settings(SETT, VAL));

#define GENERATE(filename, genclass) \
    { \
        std::vector<test_event_entry> events; \
        genclass g; \
        g.generate(events); \
        if (!tools::serialize_obj_to_file(events, filename)) \
        { \
            std::cout << concolor::magenta << "Failed to serialize data to file: " << filename << concolor::normal << std::endl; \
            throw std::runtime_error("Failed to serialize data to file"); \
        } \
    }


#define PLAY(filename, genclass) \
    if(!do_replay_file<genclass>(filename)) \
    { \
      std::cout << concolor::magenta << "Failed to pass test : " << #genclass << concolor::normal << std::endl; \
      return 1; \
    }

#define GENERATE_AND_PLAY(genclass)                                                                        \
  {                                                                                                        \
    std::vector<test_event_entry> events;                                                                  \
    ++tests_count;                                                                                         \
    bool generated = false;                                                                                \
    try                                                                                                    \
    {                                                                                                      \
      genclass g;                                                                                          \
      generated = g.generate(events);;                                                                     \
    }                                                                                                      \
    catch (const std::exception& ex)                                                                       \
    {                                                                                                      \
      LOG_PRINT(#genclass << " generation failed: what=" << ex.what(), 0);                                 \
    }                                                                                                      \
    catch (...)                                                                                            \
    {                                                                                                      \
      LOG_PRINT(#genclass << " generation failed: generic exception", 0);                                  \
    }                                                                                                      \
    if (generated && do_replay_events< genclass >(events))                                                 \
    {                                                                                                      \
      std::cout << concolor::green << "#TEST# Succeeded " << #genclass << concolor::normal << '\n';        \
    }                                                                                                      \
    else                                                                                                   \
    {                                                                                                      \
      std::cout << concolor::magenta << "#TEST# Failed " << #genclass << concolor::normal << '\n';         \
      failed_tests.push_back(#genclass);                                                                   \
    }                                                                                                      \
    std::cout << std::endl;                                                                                \
  }

#define CALL_TEST(test_name, function)                                                                     \
  {                                                                                                        \
    if(!function())                                                                                        \
    {                                                                                                      \
      std::cout << concolor::magenta << "#TEST# Failed " << test_name << concolor::normal << std::endl;    \
      return 1;                                                                                            \
    }                                                                                                      \
    else                                                                                                   \
    {                                                                                                      \
      std::cout << concolor::green << "#TEST# Succeeded " << test_name << concolor::normal << std::endl;   \
    }                                                                                                      \
  }

#define QUOTEME(x) #x
#define CHECK_TEST_CONDITION(cond) CHECK_AND_ASSERT_MES(cond, false, "failed: \"" << QUOTEME(cond) << "\"")
#define CHECK_EQ(v1, v2) CHECK_AND_ASSERT_MES(v1 == v2, false, "failed: \"" << QUOTEME(v1) << " == " << QUOTEME(v2) << "\", " << v1 << " != " << v2)
#define CHECK_NOT_EQ(v1, v2) CHECK_AND_ASSERT_MES(!(v1 == v2), false, "failed: \"" << QUOTEME(v1) << " != " << QUOTEME(v2) << "\", " << v1 << " == " << v2)
#define MK_COINS(amount) (UINT64_C(amount) * COIN)
#define TESTS_DEFAULT_FEE TX_POOL_MINIMUM_FEE//((uint64_t)1000000) // pow(10, 6)
