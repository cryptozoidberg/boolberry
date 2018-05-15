// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "serialization/keyvalue_serialization.h"

namespace gui
{

  struct addressbook_entry
  {
    std::string name;
    std::string address;
    std::string alias;
    std::string paymentId;

    bool operator==(const addressbook_entry& abe) const
    {
      return (
        (name == abe.name) && (address == abe.address) &&
        (alias == abe.alias) && (paymentId == abe.paymentId)
        );
    }

    bool operator!=(const addressbook_entry& abe) const
    {
      return !(operator==(abe));
    }

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(name)
      KV_SERIALIZE(address)
      KV_SERIALIZE(alias)
      KV_SERIALIZE(paymentId)
    END_KV_SERIALIZE_MAP()
  };

  struct addressbookstorage
  {
    std::vector<addressbook_entry> entries;
    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(entries)
    END_KV_SERIALIZE_MAP()
  };

  struct gui_config
  {
    std::string wallets_last_used_dir;
    addressbookstorage address_book;
    std::string gui_lang;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(wallets_last_used_dir)
      KV_SERIALIZE(address_book)
      KV_SERIALIZE(gui_lang)
    END_KV_SERIALIZE_MAP()
  };
}
