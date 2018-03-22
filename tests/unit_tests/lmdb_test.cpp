// Copyright (c) 2012-2018 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "gtest/gtest.h"
#include "common/db_bridge.h"
#include "common/db_lmdb_adapter.h"

namespace
{
  TEST(lmdb, test1)
  {
    db::lmdb_adapter lmdb;
    db::db_bridge_base dbb(&lmdb);

    bool r = false;
    
    r = dbb.open("test_lmdb");
    ASSERT_TRUE(r);

    r = dbb.close();
    ASSERT_TRUE(r);
  }
}
