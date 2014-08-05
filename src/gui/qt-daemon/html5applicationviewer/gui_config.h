// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "serialization/keyvalue_serialization.h"


struct gui_config
{
  std::string wallets_last_used_dir;

  BEGIN_KV_SERIALIZE_MAP()
    KV_SERIALIZE(wallets_last_used_dir)
  END_KV_SERIALIZE_MAP()
};