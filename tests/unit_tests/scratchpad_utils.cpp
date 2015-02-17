// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "gtest/gtest.h"

#include "currency_core/currency_format_utils.h"

using namespace currency;

#define WILD_KECCAK_SCRATCHPAD_BUFFSIZE  1000000000  //100MB
struct scratchpad_hi
{
  unsigned char prevhash[32];
  uint64_t height;
};

#define WILD_KECCAK_ADDENDUMS_ARRAY_SIZE  10
struct addendums_array_entry
{
  struct scratchpad_hi prev_hi;
  uint64_t add_size;
};

uint64_t* pscratchpad_buff = NULL;
volatile uint64_t  scratchpad_size = 0;
struct scratchpad_hi current_scratchpad_hi;
struct addendums_array_entry add_arr[WILD_KECCAK_ADDENDUMS_ARRAY_SIZE];

bool patch_scratchpad_with_addendum(uint64_t* pscratchpad_buff, uint64_t global_add_startpoint, uint64_t* padd_buff, size_t count/*uint64 units*/)
{
  for(int i = 0; i < count; i += 4)
  {
    uint64_t global_offset = (padd_buff[i]%(global_add_startpoint/4))*4;
    for(int j = 0; j != 4; j++)
      pscratchpad_buff[global_offset + j] ^= padd_buff[i + j];
  }
  return true;
}

bool patch_scratchpad_with_addendum(uint64_t global_add_startpoint, uint64_t* padd_buff, size_t count/*uint64 units*/)
{
  return patch_scratchpad_with_addendum(pscratchpad_buff, global_add_startpoint, padd_buff,count);
}

bool apply_addendum(uint64_t* padd_buff, size_t count/*uint64 units*/)
{
  if(WILD_KECCAK_SCRATCHPAD_BUFFSIZE <= (scratchpad_size+ count)*8 )
  {
    LOG_ERROR("!!!!!!! WILD_KECCAK_SCRATCHPAD_BUFFSIZE is overflowed !!!!!!!! please increase this constant! ");
    return false;
  }

  if(!patch_scratchpad_with_addendum(pscratchpad_buff, scratchpad_size, padd_buff, count) )
  {
    LOG_ERROR("patch_scratchpad_with_addendum is broken, reseting scratchpad");
    current_scratchpad_hi.height = 0;
    scratchpad_size = 0;
    return false;
  }
  for(int k = 0; k != count; k++)
    pscratchpad_buff[scratchpad_size+k] =   padd_buff[k];

  scratchpad_size += count;
  return true;
}

bool pop_addendum(addendums_array_entry* padd_entry)
{
  if(!padd_entry)
    return false;

  if(!padd_entry->add_size || !padd_entry->prev_hi.height)
  {
    LOG_ERROR("wrong parameters");
    return false;
  }
  patch_scratchpad_with_addendum(scratchpad_size - padd_entry->add_size, &pscratchpad_buff[scratchpad_size - padd_entry->add_size], padd_entry->add_size);
  scratchpad_size = scratchpad_size - padd_entry->add_size;
  memcpy(&current_scratchpad_hi, &padd_entry->prev_hi, sizeof(padd_entry->prev_hi));

  memset(padd_entry, 0, sizeof(addendums_array_entry));
  return true;
}

bool revert_scratchpad()
{
  //playback scratchpad addendums for whole add_arr
  size_t i_ = 0;
  size_t i = 0;
  size_t arr_size = sizeof(add_arr)/sizeof(add_arr[0]);

  for(i_=0; i_ != arr_size; i_++)
  {
    i = arr_size-(i_+1);
    if(!add_arr[i].prev_hi.height)
      continue;
    pop_addendum(&add_arr[i]);
  }
  return true;
}


bool push_addendum_info(struct scratchpad_hi* pprev_hi, uint64_t size /* uint64 units count*/)
{
  //find last free entry
  size_t i = 0;
  size_t arr_size = sizeof(add_arr)/sizeof(add_arr[0]);

  for(i=0; i != arr_size; i++)
  {
    if(!add_arr[i].prev_hi.height)
      break;
  }

  if(i >= arr_size)
  {//shift array
    memmove(&add_arr[0], &add_arr[1], (arr_size-1)*sizeof(add_arr[0]));   
    i = arr_size - 1;
  }
  memcpy(&add_arr[i].prev_hi, pprev_hi, sizeof(add_arr[i].prev_hi));
  add_arr[i].add_size = size;

  return true;
}

TEST(scratchpad_tests, test_revert_scratchpad_cpuminer)
{

  std::vector<crypto::hash> scratchpad;
  scratchpad.resize(100);
  for(auto& h: scratchpad)
    h = crypto::rand<crypto::hash>();

  std::list<std::vector<crypto::hash> > addms; 
  for(int l = 0;  l != 10; l++)
  {
    std::vector<crypto::hash> addend;
    addend.resize(10);
    for(auto& h: addend)
      h = crypto::rand<crypto::hash>();
    addms.push_back(addend);
  }

  uint64_t inital_heigh = 1000;
  current_scratchpad_hi.height = inital_heigh;
  crypto::hash tmp_h = crypto::rand<crypto::hash>();
  memcpy(current_scratchpad_hi.prevhash, &tmp_h, sizeof(current_scratchpad_hi.prevhash));

  scratchpad_hi current_scratchpad_hi2 = current_scratchpad_hi;

  pscratchpad_buff = (uint64_t*)malloc(WILD_KECCAK_SCRATCHPAD_BUFFSIZE);
  memcpy(pscratchpad_buff, &scratchpad[0], scratchpad.size()*32);
  scratchpad_size = scratchpad.size()*4;

  //backup
  uint64_t* pscratchpad_buff2 = (uint64_t*)malloc(WILD_KECCAK_SCRATCHPAD_BUFFSIZE);
  memcpy(pscratchpad_buff2, &scratchpad[0], scratchpad.size()*32);
  uint64_t scratchpad_size2 = scratchpad.size()*4;

  //apply addendums
  for(auto & a: addms)
  {
    apply_addendum((uint64_t*)&a[0], a.size()*4);
    push_addendum_info(&current_scratchpad_hi, a.size()*4);
    current_scratchpad_hi.height = current_scratchpad_hi.height+1;
    crypto::hash tmp_hash =  crypto::rand<crypto::hash>();
    memcpy(current_scratchpad_hi.prevhash, &tmp_hash, sizeof(current_scratchpad_hi.prevhash));
  }

  //check that state different
  if(scratchpad_size == scratchpad_size2)
  {
    LOG_ERROR("Scratchpad size missmatch");
    ASSERT_TRUE(false);
  }

  if(!memcmp(pscratchpad_buff, pscratchpad_buff2, scratchpad_size*8))
  {
    LOG_ERROR("Scratchpad content missmatch");
    ASSERT_TRUE(false);
  }

  if(!memcmp(&current_scratchpad_hi, &current_scratchpad_hi2, sizeof(current_scratchpad_hi)))
  {
    LOG_ERROR("Scratchpad content missmatch");
    ASSERT_TRUE(false);
  }


  //
  revert_scratchpad();
  if(scratchpad_size != scratchpad_size2)
  {
    LOG_ERROR("Scratchpad size missmatch");
    ASSERT_TRUE(false);
  }

  if(memcmp(pscratchpad_buff, pscratchpad_buff2, scratchpad_size*8))
  {
    LOG_ERROR("Scratchpad content missmatch");
    ASSERT_TRUE(false);
  }

  if(memcmp(&current_scratchpad_hi, &current_scratchpad_hi2, sizeof(current_scratchpad_hi)))
  {
    LOG_ERROR("Scratchpad content missmatch");
    ASSERT_TRUE(false);
  }
}



TEST(scratchpad_tests, test_patch_cpuminer)
{
  std::vector<crypto::hash> scratchpad;
  scratchpad.resize(100);
  for(auto& h: scratchpad)
    h = crypto::rand<crypto::hash>();


  std::vector<crypto::hash> scratchpad2 = scratchpad;

  std::vector<crypto::hash> addend;
  addend.resize(10);
  for(auto& h: addend)
    h = crypto::rand<crypto::hash>();


  if(scratchpad != scratchpad2)
  {
    ASSERT_TRUE(false);
  }


  uint64_t* padd_buff = (uint64_t*)&addend[0];
  size_t count = addend.size() * 4;
  patch_scratchpad_with_addendum((uint64_t*)&scratchpad2[0], scratchpad2.size()*4, (uint64_t*)&addend[0], addend.size()*4 );



  std::map<uint64_t, crypto::hash> patch;
  get_scratchpad_patch(scratchpad.size(), 0, addend.size(), addend, patch);
  //apply patch
  apply_scratchpad_patch(scratchpad, patch);

  if(scratchpad != scratchpad2)
  {
    ASSERT_TRUE(false);
  }
}

TEST(scratchpad_tests, test_patch)
{
  std::vector<crypto::hash> scratchpad;

  account_base acc;
  acc.generate();
  block b = AUTO_VAL_INIT(b);
  
  std::vector<std::pair<block, std::vector<crypto::hash> > > s;
  for(size_t i = 0; i != 40; i++)
  {
    construct_miner_tx(0, 0, 0, 10000, 0, acc.get_keys().m_account_address, b.miner_tx);
    s.push_back(std::pair<block, std::vector<crypto::hash> >(b, scratchpad));
    push_block_scratchpad_data(b, scratchpad);
  }

  for(auto it = s.rbegin(); it!=s.rend(); it++)
  {
    pop_block_scratchpad_data(it->first, scratchpad);
    ASSERT_EQ(scratchpad, it->second);
  }
}

TEST(scratchpad_tests, test_alt_patch)
{
  std::vector<crypto::hash> scratchpad;

  account_base acc;
  acc.generate();

  std::vector<std::pair<block, std::vector<crypto::hash> > > s;
  for(size_t i = 0; i != 40; i++)
  {
    block b = AUTO_VAL_INIT(b);
    construct_miner_tx(0, 0, 0, 10000, 0, acc.get_keys().m_account_address, b.miner_tx);
    s.push_back(std::pair<block, std::vector<crypto::hash> >(b, scratchpad));
    push_block_scratchpad_data(b, scratchpad);
  }

  std::map<uint64_t, crypto::hash> patch;

  for(uint64_t i = 20; i != s.size(); i++)
  {
    std::vector<crypto::hash> block_addendum;
    bool res = get_block_scratchpad_addendum(s[i].first, block_addendum);
    CHECK_AND_ASSERT_MES(res, void(), "Failed to get_block_scratchpad_addendum for alt block");

    get_scratchpad_patch(s[i].second.size(), 
      0, 
      block_addendum.size(), 
      block_addendum, 
      patch);
  }
  apply_scratchpad_patch(scratchpad, patch);
  for(size_t i = 0; i!=s[20].second.size(); i++)
  {
    ASSERT_EQ(scratchpad[i], s[20].second[i]);
  }
}

TEST(test_appendum, test_appendum)
{
  std::vector<crypto::hash> scratchpad;
  scratchpad.resize(20);
  for(auto& h: scratchpad)
    h = crypto::rand<crypto::hash>();

  std::string hex_addendum;
  bool r = addendum_to_hexstr(scratchpad, hex_addendum);
  ASSERT_TRUE(r);

  std::vector<crypto::hash> scratchpad2;
  r = hexstr_to_addendum(hex_addendum, scratchpad2);
  ASSERT_TRUE(r);

  ASSERT_EQ(scratchpad, scratchpad2);
}
