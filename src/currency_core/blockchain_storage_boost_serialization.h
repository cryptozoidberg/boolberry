// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once 

namespace boost
{
  namespace serialization
  {


    template<class archive_t>
    void serialize(archive_t & ar, currency::blockchain_storage::transaction_chain_entry& te, const unsigned int version)
    {
      ar & te.tx;
      ar & te.m_keeper_block_height;
      if(version < 2)
      {
        size_t fake;
        ar & fake;
      }
      ar & te.m_global_output_indexes;
      if(version < 3)
      {
        te.m_spent_flags.resize(te.tx.vout.size(), false);
        return;
      }
      ar & te.m_spent_flags;
    }

    template<class archive_t>
    void serialize(archive_t & ar, currency::blockchain_storage::block_extended_info& ei, const unsigned int version)
    {
      ar & ei.bl;
      ar & ei.height;
      if(version < 1)
      {
        CHECK_AND_ASSERT_THROW_MES(archive_t::is_loading::value, "Wrong version at storing blockchain storage file");
        currency::difficulty_type old_dif = 0;
        ar & old_dif;
        ei.cumulative_difficulty = old_dif;
      }else
      {
        ar & ei.cumulative_difficulty;
      }
      ar & ei.block_cumulative_size;
      ar & ei.already_generated_coins;
      ar & ei.already_donated_coins;
      ar & ei.scratch_offset;
    }

    template<class archive_t>
    void serialize(archive_t & ar, currency::alias_info_base& ai, const unsigned int version)
    {
      ar & ai.m_address.m_spend_public_key;
      ar & ai.m_address.m_view_public_key;
      ar & ai.m_view_key;
      ar & ai.m_sign;
      ar & ai.m_text_comment;
    }
  }
}
