// Copyright (c) 2014-2015 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "currency_basic.h"

#define CORE_EVENT_ADD_OFFER         "CORE_EVENT_ADD_OFFER"


namespace currency
{
  template<class t_event_details>
  struct t_core_event
  {
    std::string method;
    t_event_details details;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(method)
      KV_SERIALIZE(details)
    END_KV_SERIALIZE_MAP()

  };

  typedef boost::variant<offer_details_ex> core_event_v;

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  struct i_core_event_handler
  {
    virtual void on_core_event(const std::string event_name, const core_event_v& e){};
  };
}