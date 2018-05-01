// Copyright (c) 2012-2018 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once

#include <boost/optional.hpp>
#include <thread>

namespace epee
{
  namespace misc_utils  
  {

    class exclusive_access_helper
    {
    public:
      exclusive_access_helper()
        : m_lock(m_own_lock)
      {}

      exclusive_access_helper(epee::critical_section& foreign_lock)
        : m_lock(foreign_lock)
      {}

      template<typename result_t, typename callback_t>
      result_t run(callback_t cb) const 
      {
        CRITICAL_REGION_LOCAL(m_lock);
        if (m_exclusive_mode_thread_id.is_initialized() && m_exclusive_mode_thread_id.get() != std::this_thread::get_id())
            return cb(false);

        return cb(true);
      }

      template<typename result_t, typename callback_t>
      result_t run_exclusively(callback_t cb) const 
      {
        return run<result_t>([&](bool exclusive_mode)
        {
          if (!exclusive_mode)
          {
            ASSERT_MES_AND_THROW("internal error: exclusive access is not allowed");
          }
          return cb();
        });
      }

      void set_exclusive_mode_for_this_thread()
      {
        CRITICAL_REGION_LOCAL(m_lock);
        CHECK_AND_ASSERT_THROW_MES(!m_exclusive_mode_thread_id.is_initialized(), "Exclusive mode has been already set");
        m_exclusive_mode_thread_id = std::this_thread::get_id();
      }

      void clear_exclusive_mode_for_this_thread()
      {
        CRITICAL_REGION_LOCAL(m_lock);
        CHECK_AND_ASSERT_THROW_MES(m_exclusive_mode_thread_id.is_initialized(), "Exclusive mode has been already cleared or has never been set");
        m_exclusive_mode_thread_id = boost::optional<std::thread::id>();
      }

    private: 
      epee::critical_section m_own_lock;
      epee::critical_section& m_lock;
      boost::optional<std::thread::id> m_exclusive_mode_thread_id;
    };




  }
}
