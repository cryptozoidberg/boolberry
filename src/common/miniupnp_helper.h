// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once 

#include <thread>
#include <string>
#include <boost/thread.hpp>
#include "include_base_utils.h"
extern "C" {
#include "miniupnpc/miniupnpc.h"
#include "miniupnpc/upnpcommands.h"
#include "miniupnpc/upnperrors.h"
}

#include "misc_language.h"
#include "currency_config.h"
#include "version.h"

namespace tools
{

  class miniupnp_helper
  {
    UPNPDev *m_devlist;
    UPNPUrls m_urls;
    IGDdatas m_data;
    char m_lanaddr[64];
    bool m_have_IGD;
    boost::thread m_mapper_thread;
  public: 
    miniupnp_helper():m_devlist(nullptr),
      m_urls(AUTO_VAL_INIT(m_urls)), 
      m_data(AUTO_VAL_INIT(m_data)),
      m_have_IGD(false)
    {
      m_lanaddr[0] = 0;
    }
    ~miniupnp_helper()
    {
      deinit();
    }

    bool start_regular_mapping(uint32_t internal_port, uint32_t external_port, uint32_t period_ms)
    {
      if(!init())
        return false;
      m_mapper_thread = boost::thread([=](){run_port_mapping_loop(internal_port, external_port, period_ms);});
      return true;
    }

    bool stop_mapping()
    {
      if(m_mapper_thread.joinable())
      {
        m_mapper_thread.interrupt();
        m_mapper_thread.join();
      }
      return true;
    }

    bool init()
    {
      deinit();
      const char * multicastif = 0;
      const char * minissdpdpath = 0;

      int error = 0;
      m_devlist = upnpDiscover(2000, multicastif, minissdpdpath, 0, 0, &error);
      if(error)
      {
        LOG_PRINT_L0("Failed to call upnpDiscover");
        return false;
      }
      
      int r = 0;
      r = UPNP_GetValidIGD(m_devlist, &m_urls, &m_data, m_lanaddr, sizeof(m_lanaddr));
      if(r!=1)
      {
        LOG_PRINT_L2("IGD not found");
        return false;
      }
      m_have_IGD = true;
      return true;
    }

    bool deinit()
    {
      stop_mapping();
      if(m_devlist)
      {
        freeUPNPDevlist(m_devlist); 
        m_devlist = 0;
      }

      if(m_have_IGD)
      {
        FreeUPNPUrls(&m_urls);
        m_have_IGD = false;
      }
      return true;
    }

    bool run_port_mapping_loop(uint32_t internal_port, uint32_t external_port, uint32_t period_ms)
    {
      if(m_have_IGD)
      {
        while(true)
        {
          do_port_mapping(external_port, internal_port);
          boost::this_thread::sleep_for(boost::chrono::milliseconds( period_ms ));
        }
      }
      return true;
    }

    bool do_port_mapping(uint32_t external_port, uint32_t internal_port)
    {
      std::string internal_port_str = std::to_string(internal_port);
      std::string external_port_str = std::to_string(external_port);
      std::string str_desc = CURRENCY_NAME " v" PROJECT_VERSION_LONG;

      int r = UPNP_AddPortMapping(m_urls.controlURL, m_data.first.servicetype,
              external_port_str.c_str(), internal_port_str.c_str(), m_lanaddr, str_desc.c_str(), "TCP", 0, "0");

      if(r!=UPNPCOMMAND_SUCCESS)
      {
        LOG_PRINT_L1("AddPortMapping with external_port_str= " << external_port_str << 
                                         ", internal_port_str="  << internal_port_str << 
                                         ", failed with code=" << r << "(" << strupnperror(r) << ")");
      }else
      {
        LOG_PRINT_L0("[upnp] port mapped successful (ext: " << external_port_str << ", int:"  << internal_port_str << ")");
      }
      return true;
    }
  };
}

