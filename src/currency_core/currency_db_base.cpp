// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "include_base_utils.h"
using namespace epee;

#include "currency_db_base.h"
#include "currency_format_utils.h"

namespace currency
{
  namespace db
  {
    blockchain_vector_to_db_adapter::blockchain_vector_to_db_adapter() :count(0),
      index_of_local_copy(std::numeric_limits<size_t>::max())
    {
    }

    bool blockchain_vector_to_db_adapter::init(const std::string& db_path)
    {
      bool res = bdb.open(db_path);
      CHECK_AND_ASSERT_MES(res, false, "Failed to open db at path " << db_path);
      if (!bdb.get_pod_object(DB_COUNTER_KEY_NAME, count))
      {
        res = bdb.set_pod_object(DB_COUNTER_KEY_NAME, count);
        CHECK_AND_ASSERT_MES(res, false, "Failed to set counter key to db");
      }
      return true;
    }

    void blockchain_vector_to_db_adapter::push_back(const currency::block_extended_info& bei)
    {
      bdb.set_t_object(count++, bei);
      bdb.set_pod_object(DB_COUNTER_KEY_NAME, count);
      //update cache
      if (index_of_local_copy == count - 1)
      {
        local_copy = bei;
      }
      LOG_PRINT_MAGENTA("[BLOCKS.PUSH_BACK], block id: " << currency::get_block_hash(bei.bl) << "[" << count - 1 << "]", LOG_LEVEL_4);
    }
    size_t blockchain_vector_to_db_adapter::size()
    {
      return  count;
    }

    size_t blockchain_vector_to_db_adapter::clear()
    {
      bdb.clear();
      count = 0;
      bdb.set_pod_object(DB_COUNTER_KEY_NAME, count);
      return true;
    }

    void blockchain_vector_to_db_adapter::pop_back()
    {
      --count;
      bdb.erase(count);
      bdb.set_pod_object(DB_COUNTER_KEY_NAME, count);
    }

    const currency::block_extended_info& blockchain_vector_to_db_adapter::operator[] (size_t i)
    {
      if (i != index_of_local_copy)
      {
        if (!bdb.get_t_object(i, local_copy))
        {
          LOG_ERROR("WRONG INDEX " << i << " IN DB");
          local_copy = AUTO_VAL_INIT(local_copy);
          return local_copy;
        }
        index_of_local_copy = i;
      }
      LOG_PRINT_MAGENTA("[BLOCKS.[ " << i << " ]], block id: " << currency::get_block_hash(local_copy.bl), LOG_LEVEL_4);
      return local_copy;
    }
    const currency::block_extended_info& blockchain_vector_to_db_adapter::back()
    {
      return this->operator [](count - 1);
    }
  }
}
