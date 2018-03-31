// Copyright (c) 2012-2018 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <thread>
#include <atomic>

extern "C"
{
  #define USE_INSECURE_RANDOM_RPNG_ROUTINES
  #include "crypto/random.h"
}

#include "crypto/crypto.h"
#include "gtest/gtest.h"
#include "common/db_bridge.h"
#include "common/db_lmdb_adapter.h"


namespace
{
  crypto::hash null_hash = AUTO_VAL_INIT(null_hash);

  template<typename T>
  T random_t_from_range(T from, T to)
  {
    if (from >= to)
      return from;
    T result;
    generate_random_bytes(sizeof result, &result);
    return from + result % (to - from + 1);
  }

  TEST(lmdb, basic_test)
  {
    std::shared_ptr<db::lmdb_adapter> lmdb_ptr = std::make_shared<db::lmdb_adapter>();
    db::db_bridge_base dbb(lmdb_ptr);

    bool r = false;
    
    r = dbb.open("test_lmdb");
    ASSERT_TRUE(r);

    db::table_id tid_decapod;
    r = lmdb_ptr->open_table("decapod", tid_decapod);
    ASSERT_TRUE(r);

    ASSERT_TRUE(lmdb_ptr->begin_transaction());

    ASSERT_TRUE(lmdb_ptr->clear_table(tid_decapod));

    uint64_t key = 10;
    std::string buf = "nxjdu47flrp20soam19e7nfhxbcy48owks03of92sbf31n1oqkanmdb47";

    r = lmdb_ptr->set(tid_decapod, (char*)&key, sizeof key, buf.c_str(), buf.size());
    ASSERT_TRUE(r);

    ASSERT_TRUE(lmdb_ptr->commit_transaction());

    r = dbb.close();
    ASSERT_TRUE(r);


    r = dbb.open("test_lmdb");
    ASSERT_TRUE(r);
    r = lmdb_ptr->open_table("decapod", tid_decapod);
    ASSERT_TRUE(r);

    ASSERT_TRUE(lmdb_ptr->begin_transaction());

    std::string out_buffer;
    r = lmdb_ptr->get(tid_decapod, (char*)&key, sizeof key, out_buffer);
    ASSERT_TRUE(r);
    ASSERT_EQ(buf, out_buffer);

    ASSERT_TRUE(lmdb_ptr->commit_transaction());

    r = dbb.close();
    ASSERT_TRUE(r);

  }

  //////////////////////////////////////////////////////////////////////////////
  // multithread_test_1
  //////////////////////////////////////////////////////////////////////////////
  struct multithread_test_1_t
  {
    enum { c_keys_count = 128 };
    crypto::hash m_keys[c_keys_count];
    size_t m_randomly_mixed_indexes_1[c_keys_count];
    size_t m_randomly_mixed_indexes_2[c_keys_count];
    std::shared_ptr<db::lmdb_adapter> m_lmdb_adapter;
    db::db_bridge_base m_dbb;
    db::table_id m_table_id;

    multithread_test_1_t()
      : m_lmdb_adapter(std::make_shared<db::lmdb_adapter>())
      , m_dbb(m_lmdb_adapter)
      , m_table_id(0)
    {
      for (size_t i = 0; i < c_keys_count; ++i)
      {
        m_keys[i] = i == 0 ? null_hash : crypto::cn_fast_hash(&m_keys[i - 1], sizeof crypto::hash);
        m_randomly_mixed_indexes_1[i] = i;
        m_randomly_mixed_indexes_2[i] = i;
      }

      // prepare m_randomly_mixed_indexes_1 and m_randomly_mixed_indexes_2
      for (size_t i = 0; i < c_keys_count * 100; ++i)
      {
        size_t a = random_t_from_range<size_t>(0, c_keys_count - 1);
        size_t b = random_t_from_range<size_t>(0, c_keys_count - 1);
        size_t c = random_t_from_range<size_t>(0, c_keys_count - 1);
        size_t d = random_t_from_range<size_t>(0, c_keys_count - 1);

        size_t tmp = m_randomly_mixed_indexes_1[a];
        m_randomly_mixed_indexes_1[a] = m_randomly_mixed_indexes_1[b];
        m_randomly_mixed_indexes_1[b] = tmp;

        tmp = m_randomly_mixed_indexes_2[c];
        m_randomly_mixed_indexes_2[c] = m_randomly_mixed_indexes_2[d];
        m_randomly_mixed_indexes_2[d] = tmp;
      }
    }

    void adder_thread(std::atomic<bool>& stop_flag)
    {
      //epee::misc_utils::sleep_no_w(1000);

      size_t i = 0;
      for(size_t n = 0; n < 10000; ++n)
      {
        // get pseudorandom key index
        size_t key_index = m_randomly_mixed_indexes_1[i];
        i = (i + 1) % c_keys_count;
        const crypto::hash& key = m_keys[key_index];

        bool r = m_lmdb_adapter->begin_transaction();
        CHECK_AND_ASSERT_MES_NO_RET(r, "begin_transaction");

        // get a value by the given key
        std::string value;
        if (!m_lmdb_adapter->get(m_table_id, (const char*)&key, sizeof key, value))
        {
          // if such key does not exist -- add it
          char buffer[128];
          generate_random_bytes(sizeof buffer, buffer);
          r = m_lmdb_adapter->set(m_table_id, (const char*)&key, sizeof key, buffer, sizeof buffer);
          CHECK_AND_ASSERT_MES_NO_RET(r, "set");

          uint64_t table_size = m_lmdb_adapter->get_table_size(m_table_id);

          r = m_lmdb_adapter->commit_transaction();
          CHECK_AND_ASSERT_MES_NO_RET(r, "commit_transaction");

          LOG_PRINT_L1("added key index " << key_index << ", table size: " << table_size);
        }
        else
        {
          // if key exists in the table - do nothing
          m_lmdb_adapter->abort_transaction();
        }
        epee::misc_utils::sleep_no_w(1);
      }
      LOG_PRINT_L0("adder_thread stopped");
    }

    void deleter_thread(std::atomic<bool>& stop_flag)
    {
      epee::misc_utils::sleep_no_w(1000);

      // get pseudorandom key index
      size_t i = 0;

      while(!stop_flag)
      {
        size_t key_index = m_randomly_mixed_indexes_2[i];
        i = (i + 1) % c_keys_count;
        const crypto::hash& key = m_keys[key_index];
        bool r = m_lmdb_adapter->begin_transaction();
        CHECK_AND_ASSERT_MES_NO_RET(r, "begin_transaction");

        // get a value by the given key
        std::string value;
        if (m_lmdb_adapter->get(m_table_id, (const char*)&key, sizeof key, value))
        {
          // if key exists in the table -- remove it
          char buffer[128];
          generate_random_bytes(sizeof buffer, buffer);
          r = m_lmdb_adapter->erase(m_table_id, (const char*)&key, sizeof key);
          CHECK_AND_ASSERT_MES_NO_RET(r, "erase");

          uint64_t table_size = m_lmdb_adapter->get_table_size(m_table_id);

          r = m_lmdb_adapter->commit_transaction();
          CHECK_AND_ASSERT_MES_NO_RET(r, "commit_transaction");

          LOG_PRINT_L1("erased key index " << key_index << ", table size: " << table_size);
          if (table_size == 0)
            break;
        }
        else
        {
          // if no such key exists in the table - do nothing
          m_lmdb_adapter->abort_transaction();
        }
        epee::misc_utils::sleep_no_w(1);
      }
      LOG_PRINT_L0("deleter_thread stopped");
    }

    bool check()
    {
      return true;
    }

    bool run()
    {
      bool r = m_dbb.open("multithread_test_1_t");
      CHECK_AND_ASSERT_MES(r, false, "m_dbb.open");
      r = m_lmdb_adapter->open_table("table1_", m_table_id);
      CHECK_AND_ASSERT_MES(r, false, "open_table");
      r = m_lmdb_adapter->begin_transaction();
      CHECK_AND_ASSERT_MES(r, false, "begin_transaction");
      r = m_lmdb_adapter->clear_table(m_table_id);
      CHECK_AND_ASSERT_MES(r, false, "clear_table");
      r = m_lmdb_adapter->commit_transaction();
      CHECK_AND_ASSERT_MES(r, false, "commit_transaction");

      std::atomic<bool> stop_adder(false), stop_deleter(false);
      std::thread adder_t(&multithread_test_1_t::adder_thread, this, std::ref(stop_adder));
      std::thread deleter_t(&multithread_test_1_t::deleter_thread, this, std::ref(stop_deleter));
      adder_t.join();
      //stop_deleter = true;
      deleter_t.join();
      r = check();
      m_dbb.close();
      return r;
    }
  };

  TEST(lmdb, multithread_test_1)
  {
    char prng_state[200] = {};
    random_prng_get_state(prng_state, sizeof prng_state); // store current RPNG state
    random_prng_initialize_with_seed(0); // this mades this test deterministic

    bool result = false;
    try
    {
      multithread_test_1_t t;
      result = t.run();
    }
    catch (std::exception& e)
    {
      LOG_ERROR("Caught exception: " << e.what());
    }

    // restore PRNG state to keep other tests unaffected
    random_prng_get_state(prng_state, sizeof prng_state);

    ASSERT_TRUE(result);
  }


}
