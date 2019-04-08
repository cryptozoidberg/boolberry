// Copyright (c) 2019 Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "gtest/gtest.h"

#include "include_base_utils.h"
#include "common/pod_array_file_container.h"

struct test_item_t
{
  test_item_t()
  {
    memset(data, 0, sizeof data);
  }

  uint8_t data[64];
};


TEST(utils, pod_array_file_container)
{
  typedef tools::pod_array_file_container<test_item_t> cont_t;

  cont_t container;

  std::wstring filename = L"test_pod_array_file_container.dat";

  boost::filesystem::remove(filename);

  bool corrupted = false;
  std::string reason;
  ASSERT_TRUE(container.open(filename, true, &corrupted, &reason));
  ASSERT_FALSE(corrupted);
  ASSERT_EQ(reason, std::string());

  std::vector<test_item_t> items;
  
  test_item_t item;
  item.data[0] = 0xa0;
  ASSERT_TRUE(container.push_back(item));
  items.push_back(item);

  item.data[0] = 0xa1;
  ASSERT_TRUE(container.push_back(item));
  items.push_back(item);

  item.data[0] = 0xa2;
  ASSERT_TRUE(container.push_back(item));
  items.push_back(item);

  ASSERT_EQ(container.size(), 3);
  ASSERT_EQ(container.size_bytes(), 3 * sizeof item);

  container.close();

  ////////////////////////////////

  ASSERT_TRUE(container.open(filename, false, &corrupted, &reason));
  ASSERT_FALSE(corrupted);
  ASSERT_EQ(reason, std::string());

  ASSERT_EQ(container.size(), 3);
  ASSERT_EQ(container.size_bytes(), 3 * sizeof item);

  for (size_t i = 0; i < container.size(); ++i)
  {
    ASSERT_TRUE(container.get_item(i, item));
    for (size_t j = 0; j < sizeof item; ++j)
      ASSERT_EQ(items[i].data[j], item.data[j]);
  }

  item.data[0] = 0xa3;
  ASSERT_TRUE(container.push_back(item));
  items.push_back(item);

  item.data[0] = 0xa4;
  ASSERT_TRUE(container.push_back(item));
  items.push_back(item);

  item.data[0] = 0xa5;
  ASSERT_TRUE(container.push_back(item));
  items.push_back(item);

  ASSERT_EQ(container.size(), 6);
  ASSERT_EQ(container.size_bytes(), 6 * sizeof item);

  container.close();

  ////////////////////////////////

  ASSERT_TRUE(container.open(filename, false, &corrupted, &reason));
  ASSERT_FALSE(corrupted);
  ASSERT_EQ(reason, std::string());

  ASSERT_EQ(container.size(), 6);
  ASSERT_EQ(container.size_bytes(), 6 * sizeof item);

  for (size_t i = 0; i < container.size(); ++i)
  {
    ASSERT_TRUE(container.get_item(i, item));
    for (size_t j = 0; j < sizeof item; ++j)
      ASSERT_EQ(items[i].data[j], item.data[j]);
  }

  item.data[0] = 0xa6;
  ASSERT_TRUE(container.push_back(item));
  items.push_back(item);

  item.data[0] = 0xa7;
  ASSERT_TRUE(container.push_back(item));
  items.push_back(item);

  item.data[0] = 0xa8;
  ASSERT_TRUE(container.push_back(item));
  items.push_back(item);

  ASSERT_EQ(container.size(), 9);
  ASSERT_EQ(container.size_bytes(), 9 * sizeof item);

  container.close();

  ////////////////////////////////

  ASSERT_TRUE(container.open(filename, false, &corrupted, &reason));
  ASSERT_FALSE(corrupted);
  ASSERT_EQ(reason, std::string());

  ASSERT_EQ(container.size(), 9);
  ASSERT_EQ(container.size_bytes(), 9 * sizeof item);

  for (size_t i = 0; i < container.size(); ++i)
  {
    ASSERT_TRUE(container.get_item(i, item));
    for (size_t j = 0; j < sizeof item; ++j)
      ASSERT_EQ(items[i].data[j], item.data[j]);
  }

  container.close();

}
