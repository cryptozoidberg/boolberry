// Copyright (c) 2012-2018 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once 


#define SWAP_CURRENCY_PUBLIC_ADDRESS_BASE58_PREFIX           197   // addresses start with 'Z'
#define SWAP_CURRENCY_PUBLIC_INTEG_ADDRESS_BASE58_PREFIX     0x3678 // addresses start with 'iZ'

#if defined(TESTNET)
#  define SWAP_ADDRESS_ENCRYPTION_PUB_KEY                    "c1ddb94bfad165d51d803095ac593f8256a630ab08ab7964500a5d789f310484"
#  define SWAP_ADDRESS_ENCRYPTION_SEC_KEY                    "142283bc71ebdb90ec040fe1b118ebaa70cea2bd82b7cd1b361272b2c8bcfe07" // for tests only
#else
#  define SWAP_ADDRESS_ENCRYPTION_PUB_KEY                    "0000000000000000000000000000000000000000000000000000000000000000"
#endif
