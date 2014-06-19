// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "crypto/crypto.h"
#include "currency_core/currency_basic.h"


extern "C" {
#include "crypto/keccak.h"
#include "crypto/alt/KeccakNISTInterface.h"
}


#include "crypto/wild_keccak.h"





#define TEST_BUFF_LEN 200

class test_keccak_base
{
public:
  static const size_t loop_count = 100000;

  bool init()
  {
    currency::block b;
    m_buff = currency::get_block_hashing_blob(b);
    m_buff.append(32*4, 0);
    return true;
  }

  bool pretest()
  {
    ++m_buff[0];
    if(!m_buff[0])
      ++m_buff[0];
    return true;
  }
protected:
  std::string m_buff; 
};

class test_keccak: public test_keccak_base
{
public:
  bool test()
  {
    pretest();
    crypto::hash h;
    keccak(reinterpret_cast<const uint8_t*>(&m_buff[0]), m_buff.size(), reinterpret_cast<uint8_t*>(&h), sizeof(h));
    LOG_PRINT_L4(h);
    return true;
  }
};

class test_keccak_alt1: public test_keccak_base
  {
  public:
    bool test()
      {
      pretest();
      crypto::hash h;
      //Hash(256, reinterpret_cast<const unsigned char*>(&m_buff[0]), m_buff.size()*8, reinterpret_cast<unsigned char*>(&h));
      //crypto::hash h2;
      //keccak(reinterpret_cast<const unsigned char*>(&m_buff[0]), m_buff.size(), reinterpret_cast<unsigned char*>(&h2), sizeof(h2));

      LOG_PRINT_L4(h);
      return true;
      }
  };




class test_keccak_generic: public test_keccak_base
{
public:
  bool test()
  {
    pretest();
    crypto::hash h;
    crypto::keccak_generic<crypto::regular_f>(reinterpret_cast<const uint8_t*>(&m_buff[0]), m_buff.size(), reinterpret_cast<uint8_t*>(&h), sizeof(h));
    LOG_PRINT_L4(h);
    return true;
  }
};

class test_keccak_generic_with_mul: public test_keccak_base
{
public:
  bool test()
  {
    pretest();
    crypto::hash h;
    crypto::keccak_generic<crypto::mul_f>(reinterpret_cast<const uint8_t*>(&m_buff[0]), m_buff.size(), reinterpret_cast<uint8_t*>(&h), sizeof(h));
    return true;
  }
};

template<int scratchpad_size>
class test_wild_keccak: public test_keccak_base
{
public:
  bool init()
  {
    m_scratchpad_vec.resize(scratchpad_size/sizeof(crypto::hash));
    for(auto& h: m_scratchpad_vec)
      h = crypto::rand<crypto::hash>();

    return test_keccak_base::init();
  }

  bool test()
  {
    pretest();
    uint64_t* pst =  (uint64_t*)&m_scratchpad_vec[0];
    uint64_t sz = m_scratchpad_vec.size()*4;
    for(size_t i = 0; i != sz; i++)
      pst[i] = i;

    crypto::hash h;
    crypto::wild_keccak_dbl<crypto::mul_f>(reinterpret_cast<const uint8_t*>(&m_buff[0]), m_buff.size(), reinterpret_cast<uint8_t*>(&h), sizeof(h), [&](crypto::state_t_m& st, crypto::mixin_t& mix)
    {
#define SCR_I(i) m_scratchpad_vec[st[i]%m_scratchpad_vec.size()]
      for(size_t i = 0; i!=6; i++)
      {
        *(crypto::hash*)&mix[i*4]  = XOR_4(SCR_I(i*4), SCR_I(i*4+1), SCR_I(i*4+2), SCR_I(i*4+3));  
      }
    });

    crypto::hash h2;
    crypto::wild_keccak_dbl_opt(reinterpret_cast<const uint8_t*>(&m_buff[0]), m_buff.size(), reinterpret_cast<uint8_t*>(&h2), sizeof(h2), (uint64_t*)&m_scratchpad_vec[0], m_scratchpad_vec.size()*4);
    if(h2 != h)
      return false;

    return true;
  }
 protected:
  std::vector<crypto::hash> m_scratchpad_vec;
};