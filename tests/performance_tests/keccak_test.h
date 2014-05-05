// Copyright (c) 2012-2013 The XXX developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "crypto/crypto.h"
#include "currency_core/currency_basic.h"

extern "C" {
#include "crypto/keccak.h"
}

#include "crypto/keccak_modified.h"





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
    return true;
  }
};

class test_keccak_m: public test_keccak_base
{
public:
  bool test()
  {
    pretest();
    crypto::hash h;
    keccak_m<regular_f>(reinterpret_cast<const uint8_t*>(&m_buff[0]), m_buff.size(), reinterpret_cast<uint8_t*>(&h), sizeof(h));
    return true;
  }
};

class test_keccak_m_mul: public test_keccak_base
{
public:
  bool test()
  {
    pretest();
    crypto::hash h;
    keccak_m<mul_f>(reinterpret_cast<const uint8_t*>(&m_buff[0]), m_buff.size(), reinterpret_cast<uint8_t*>(&h), sizeof(h));
    return true;
  }
};

template<int scratchpad_size>
class test_keccak_m_mul_rand: public test_keccak_base
{
public:
  bool init()
  {
    m_scratchpad_vec.resize(scratchpad_size/sizeof(crypto::hash));
    for(auto& h: m_scratchpad_vec)
      h = crypto::rand<crypto::hash>();

    return test_keccak_base::init();
  }

  crypto::hash xor_h(const crypto::hash& a, const crypto::hash& b)
  {
    crypto::hash r;
    for(size_t i = 0; i != 4; i++)
    {
      ((uint64_t*)&r)[i] = ((const uint64_t*)&a)[i] ^ ((const uint64_t*)&b)[i];
    }
    return r;
  }
#define XOR_2(A, B) xor_h(A, B)
#define XOR_3(A, B, C) xor_h(A, XOR_2(B, C))
#define XOR_4(A, B, C, D) xor_h(A, XOR_3(B, C, D))
#define XOR_5(A, B, C, D, E) xor_h(A, XOR_4(B, C, D, E))
#define XOR_8(A, B, C, D, F, G, H, I) xor_h(XOR_4(A, B, C, D), XOR_4(F, G, H, I))

  bool test()
  {
    pretest();
    crypto::hash h;
    keccak_kecack_m_rnd<mul_f>(reinterpret_cast<const uint8_t*>(&m_buff[0]), m_buff.size(), reinterpret_cast<uint8_t*>(&h), sizeof(h), [&](state_t_m& st, mixin_t& mix)
    {
#define SCR_I(i) m_scratchpad_vec[st[i]%m_scratchpad_vec.size()]
      for(size_t i = 0; i!=6; i++)
      {
        *(crypto::hash*)&mix[i*4]  = XOR_5(SCR_I(i*5), SCR_I(i*5+1), SCR_I(i*5+2), SCR_I(i*5+3), SCR_I(i*5+4));  
      }
    });

    return true;
  }
 protected:
  std::vector<crypto::hash> m_scratchpad_vec;
};