// Copyright (c) 2014-2019 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "db_abstract_accessor.h"

namespace tools
{
  namespace db
  {
    template<class value_t, bool value_type_is_serializable>
    class array_accessor_adapter_to_native : public array_accessor<value_t, value_type_is_serializable>
    {
    public:
      typedef array_accessor<value_t, value_type_is_serializable> super;

      array_accessor_adapter_to_native(basic_db_accessor& dbb) :array_accessor<value_t, value_type_is_serializable>(dbb)
      {}

      value_t operator[](const size_t& k) const
      {
        return *array_accessor<value_t, value_type_is_serializable>::operator [](k);
      }

      template<typename container_t>
      bool load_all_itmes_to_container(container_t& container) const
      {
        bdb.begin_readonly_transaction();

        size_t items_count = super::size();
        container.clear();
        container.resize(items_count);

        size_t items_added = 0;;
        bool result = true;
        auto lambda = [&](size_t item_idx, size_t key, const value_t& value) -> bool
        {
          if (key >= items_count)
          {
            LOG_ERROR("internal DB error during enumeration: items_count == " << items_count << ", item_idx == " << item_idx << ", key == " << key);
            result = false;
            return false;
          }
          container[key] = value;
          ++items_added;
          return true;
        };
        super::enumerate_items(lambda);

        bdb.commit_transaction();

        CHECK_AND_ASSERT_MES(items_added == items_count, false, "internal DB error: items_added == " << items_added << ", items_count == " << items_count);
        return result;
      }
      void resize(uint64_t new_size)
      {
        uint64_t current_size = super::size();
        if (current_size > new_size)
        {
          uint64_t count = current_size - new_size;
          //trim
          for (uint64_t i = 0; i != count; i++)
            this->pop_back();
        }
        else if (current_size < new_size)
        {
          uint64_t count = new_size - current_size;
          //trim
          for (uint64_t i = 0; i != count; i++)
            this->push_back(value_t());
        }
      }

    };
  }
}