// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "serialization/keyvalue_serialization.h"


struct gui_config
{
  std::string wallets_last_used_dir;

  struct addressbook_entry
  {
	  std::string name;
	  std::string address;
	  std::string alias;

	  BEGIN_KV_SERIALIZE_MAP()
		  KV_SERIALIZE(name)
		  KV_SERIALIZE(address)
		  KV_SERIALIZE(alias)
	  END_KV_SERIALIZE_MAP()
  };

  struct addressbookstorage
  {
	  std::vector<addressbook_entry> entries;
	  BEGIN_KV_SERIALIZE_MAP()
		  KV_SERIALIZE(entries)
	  END_KV_SERIALIZE_MAP()
  };
  
  addressbookstorage address_book;

  BEGIN_KV_SERIALIZE_MAP()
	  KV_SERIALIZE(wallets_last_used_dir)
	  KV_SERIALIZE(address_book)
  END_KV_SERIALIZE_MAP()
};