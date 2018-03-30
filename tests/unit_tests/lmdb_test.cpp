// Copyright (c) 2012-2018 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
extern "C"
{
  #define USE_INSECURE_RANDOM_RPNG_ROUTINES
  #include "crypto/random.h"
}
#include "gtest/gtest.h"
#include "common/db_bridge.h"
#include "common/db_lmdb_adapter.h"

namespace
{
  TEST(lmdb, test1)
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
}
