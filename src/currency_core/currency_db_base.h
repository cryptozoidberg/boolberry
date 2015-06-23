// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "include_base_utils.h"

#include "currency_db_base.h"
#include "leveldb/db.h"
#include "currency_boost_serialization.h"

namespace currency
{
  namespace db
  {
    typedef leveldb::DB* db_handle;

    static const db_handle err_handle = nullptr;

    class basic_db
    {
      std::string m_path;
      db_handle m_pdb;
    public:
      basic_db(){}
      ~basic_db(){ close(); }
      bool close()
      {
        if (m_pdb)
          delete m_pdb;
        m_pdb = nullptr;

        return true;
      }
      bool open(const std::string& path)
      {
        m_path = path;
        close();
        leveldb::Options options;
        options.create_if_missing = true;
        leveldb::Status status = leveldb::DB::Open(options, path, &m_pdb);
        if (!status.ok())
        {
          LOG_ERROR("Unable to open/create test database " << path << ", error: " << status.ToString());
          return err_handle;
        }
        return true;
      }

      template<class t_pod_key>
      bool erase(const t_pod_key& k)
      {
        TRY_ENTRY();
        leveldb::WriteOptions wo;
        wo.sync = true;
        std::string res_buff;
        leveldb::Status s = m_pdb->Delete(wo, leveldb::Slice((const char*)&k, sizeof(k)));
        if (!s.ok())
          return false;

        return true;
        CATCH_ENTRY_L0("get_t_object_from_db", false);
      }


      template<class t_pod_key, class t_object>
      bool get_t_object(const t_pod_key& k, t_object& obj)
      {
        TRY_ENTRY();
        leveldb::ReadOptions ro;
        std::string res_buff;
        leveldb::Status s = m_pdb->Get(ro, leveldb::Slice((const char*)&k, sizeof(k)), &res_buff);
        if (!s.ok())
          return false;

        return tools::unserialize_obj_from_buff(obj, res_buff);
        CATCH_ENTRY_L0("get_t_object_from_db", false);
      }

      bool clear()
      {
        close();
        boost::system::error_code ec;
        bool res = boost::filesystem::remove(m_path, ec);
        if (!res)
        {
          LOG_ERROR("Failed to remove db file " << m_path);
          return false;
        }
        return open(m_path);
      }

      template<class t_pod_key, class t_object>
      bool set_t_object(const t_pod_key& k, t_object& obj)
      {
        TRY_ENTRY();
        leveldb::WriteOptions wo;
        wo.sync = true;
        std::string obj_buff;
        tools::serialize_obj_to_buff(obj, obj_buff);

        leveldb::Status s = m_pdb->Put(wo, leveldb::Slice((const char*)&k, sizeof(k)), leveldb::Slice(obj_buff));
        if (!s.ok())
          return false;

        return true;
        CATCH_ENTRY_L0("set_t_object_to_db", false);
      }

      template<class t_pod_key, class t_pod_object>
      bool get_pod_object(const t_pod_key& k, t_pod_object& obj)
      {
        static_assert(std::is_pod<t_pod_object>::value, "t_pod_object must be a POD type.");

        TRY_ENTRY();
        leveldb::ReadOptions ro;
        std::string res_buff;
        leveldb::Status s = m_pdb->Get(ro, leveldb::Slice((const char*)&k, sizeof(k)), &res_buff);
        if (!s.ok())
          return false;

        CHECK_AND_ASSERT_MES(sizeof(t_pod_object) == res_buff.size(), false, "sizes missmath at get_pod_object_from_db(). returned size = " 
          << res_buff.size() << "expected: " << sizeof(t_pod_object));
        
        obj = *(t_pod_object*)res_buff.data();
        return true;
        CATCH_ENTRY_L0("get_t_object_from_db", false);
      }

      template<class t_pod_key, class t_pod_object>
      bool set_pod_object(const t_pod_key& k, const t_pod_object& obj)
      {
        static_assert(std::is_pod<t_pod_object>::value, "t_pod_object must be a POD type.");

        TRY_ENTRY();
        leveldb::WriteOptions wo;
        wo.sync = true;
        std::string obj_buff((const char*)&obj, sizeof(obj));

        leveldb::Status s = m_pdb->Put(wo, leveldb::Slice((const char*)&k, sizeof(k)), leveldb::Slice(obj_buff));
        if (!s.ok())
          return false;

        return true;
        CATCH_ENTRY_L0("get_t_object_from_db", false);
      }


    };


#define DB_COUNTER_KEY_NAME  "counter"

    class blockchain_vector_to_db_adapter
    {
    public: 
      basic_db bdb;
      currency::block_extended_info local_copy;
      size_t index_of_local_copy;
      size_t count;

      blockchain_vector_to_db_adapter() :count(0), 
                                         index_of_local_copy(0)
      {
      }

      bool init(const std::string& db_path)
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

      void push_back(const currency::block_extended_info& bei)
      {
        bdb.set_t_object(count++, bei);
        bdb.set_pod_object(DB_COUNTER_KEY_NAME, count);
      }
      size_t size()
      {
        return  count;
      }

      size_t clear()
      {
        bdb.clear();
        count = 0;
        bdb.set_pod_object(DB_COUNTER_KEY_NAME, count);
        return true;
      }

      void pop_back()
      {
        --count;
        bdb.erase(count);
        bdb.set_pod_object(DB_COUNTER_KEY_NAME, count);
      }

      const currency::block_extended_info& operator[] (size_t i)
      {
        if (i == index_of_local_copy)
          return local_copy;          
          
        if (!bdb.get_t_object(i, local_copy))
        {
          LOG_ERROR("WRONG INDEX " << i << " IN DB");
          local_copy = AUTO_VAL_INIT(local_copy);
          return local_copy;
        }
        index_of_local_copy = i;
        return local_copy;
      }
      const currency::block_extended_info& back()
      {
        return this->operator [](count-1);
      }

    };


//     /**/
//     void f()
//     {
//       leveldb::DB* db;
//       leveldb::Options options;
//       options.create_if_missing = true;
//       leveldb::Status status = leveldb::DB::Open(options, "./testdb", &db);
//       if (false == status.ok())
//       {
//         cerr << "Unable to open/create test database './testdb'" << endl;
//         cerr << status.ToString() << endl;
//         return -1;
//       }
//       // Add 256 values to the database
//       leveldb::WriteOptions writeOptions;
//       for (unsigned int i = 0; i < 256; ++i)
//       {
//         ostringstream keyStream;
//         keyStream << "Key" << i;
//         ostringstream valueStream;
//         valueStream << "Test data value: " << i;
//         db->Put(writeOptions, keyStream.str(), valueStream.str());
//       }
//       // Iterate over each item in the database and print them
//       leveldb::Iterator* it = db->NewIterator(leveldb::ReadOptions());
//       for (it->SeekToFirst(); it->Valid(); it->Next())
//       {
//         cout << it->key().ToString() << " : " << it->value().ToString() << endl;
//       }
//       if (false == it->status().ok())
//       {
//         cerr << "An error was found during the scan" << endl;
//         cerr << it->status().ToString() << endl;
//       }
//       delete it;
//       // Close the database
//       delete db;
// 
//     }

  }
}