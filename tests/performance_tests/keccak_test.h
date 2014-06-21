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

    crypto::hash h;
    crypto::wild_keccak_dbl<crypto::mul_f>(reinterpret_cast<const uint8_t*>(&m_buff[0]), m_buff.size(), reinterpret_cast<uint8_t*>(&h), sizeof(h), [&](crypto::state_t_m& st, crypto::mixin_t& mix)
    {
#define SCR_I(i) m_scratchpad_vec[st[i]%m_scratchpad_vec.size()]
      for(size_t i = 0; i!=6; i++)
      {
        *(crypto::hash*)&mix[i*4]  = XOR_4(SCR_I(i*4), SCR_I(i*4+1), SCR_I(i*4+2), SCR_I(i*4+3)); 
      }
    });

    return true;
  }
 protected:
  std::vector<crypto::hash> m_scratchpad_vec;
};


template<int scratchpad_size>
class test_wild_keccak2: public test_keccak_base
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

      crypto::hash h2;
      crypto::wild_keccak_dbl_opt(reinterpret_cast<const uint8_t*>(&m_buff[0]), m_buff.size(), reinterpret_cast<uint8_t*>(&h2), sizeof(h2), (const UINT64*)&m_scratchpad_vec[0], m_scratchpad_vec.size()*4);
      LOG_PRINT_L4("HASH:" << h2);
      return true;
      }
  protected:
    std::vector<crypto::hash> m_scratchpad_vec;
  };

#define max_measere_scratchpad 1000000000
#define measere_rounds 100000
void measure_keccak_over_scratchpad()
{
  std::cout << std::setw(20) << std::left << "sz\t" << 
    std::setw(10) << "original\t" <<
    std::setw(10) << "opt" << ENDL;   

  std::vector<crypto::hash> scratchpad_vec;
  scratchpad_vec.reserve(max_measere_scratchpad);
  std::string has_str = "Keccak is a family of sponge functions. The sponge function is a generalization of the concept of cryptographic hash function with infinite output and can perform quasi all symmetric cryptographic functions, from hashing to pseudo-random number generation to authenticated encryption";

  for(uint64_t i = 1000; i != max_measere_scratchpad; i += 100000)
  {
    uint64_t size_original = scratchpad_vec.size();
    scratchpad_vec.resize(i/sizeof(crypto::hash));
    for(size_t j = size_original; j!=scratchpad_vec.size();j++)
      scratchpad_vec[j] = crypto::rand<crypto::hash>();

    crypto::hash res_h = currency::null_hash;
    uint64_t ticks_a = epee::misc_utils::get_tick_count();
    for(size_t r = 0; r != measere_rounds; r++)
    {      
      res_h = currency::get_blob_longhash(has_str,  1, scratchpad_vec);
    }
    uint64_t ticks_b = epee::misc_utils::get_tick_count();
    for(size_t r = 0; r != measere_rounds; r++)
    {      
      res_h = currency::get_blob_longhash_opt(has_str, scratchpad_vec);
    } 
    uint64_t ticks_c = epee::misc_utils::get_tick_count();
    std::cout << std::setw(20) << std::left << scratchpad_vec.size()*sizeof(crypto::hash) << "\t" <<
                 std::setw(10) << ticks_b - ticks_a << "\t" <<
                 std::setw(10) << ticks_c - ticks_b << ENDL;   
  }
}
