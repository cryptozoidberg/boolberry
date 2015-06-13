// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "currency_db_base.h"
#include "leveldb/db.h"


namespace currency
{
  namespace db
  {
    static const db_handle err_handle = nullptr;
    typedef leveldb::DB* db_handle;

    db_handle open_db(const std::string& path)
    {
      db_handle h = AUTO_VAL_INIT(h);

      leveldb::Options options;
      options.create_if_missing = true;
      leveldb::Status status = leveldb::DB::Open(options, path, &db_handle);
      if (!status.ok())
      {
        LOG_ERROR("Unable to open/create test database " << path << ", error: " << status.ToString());
        return err_handle;
      }



    }


    void f()
    {
      leveldb::DB* db;
      leveldb::Options options;
      options.create_if_missing = true;
      leveldb::Status status = leveldb::DB::Open(options, "./testdb", &db);
      if (false == status.ok())
      {
        cerr << "Unable to open/create test database './testdb'" << endl;
        cerr << status.ToString() << endl;
        return -1;
      }
      // Add 256 values to the database
      leveldb::WriteOptions writeOptions;
      for (unsigned int i = 0; i < 256; ++i)
      {
        ostringstream keyStream;
        keyStream << "Key" << i;
        ostringstream valueStream;
        valueStream << "Test data value: " << i;
        db->Put(writeOptions, keyStream.str(), valueStream.str());
      }
      // Iterate over each item in the database and print them
      leveldb::Iterator* it = db->NewIterator(leveldb::ReadOptions());
      for (it->SeekToFirst(); it->Valid(); it->Next())
      {
        cout << it->key().ToString() << " : " << it->value().ToString() << endl;
      }
      if (false == it->status().ok())
      {
        cerr << "An error was found during the scan" << endl;
        cerr << it->status().ToString() << endl;
      }
      delete it;
      // Close the database
      delete db;

    }

  }
}