/*
The Keccak sponge function, designed by Guido Bertoni, Joan Daemen,
Michaël Peeters and Gilles Van Assche. For more information, feedback or
questions, please refer to our website: http://keccak.noekeon.org/

Implementation by the designers,
hereby denoted as "the implementer".

To the extent possible under law, the implementer has waived all copyright
and related or neighboring rights to the source code in this file.
http://creativecommons.org/publicdomain/zero/1.0/
*/

#ifndef _KeccakPermutationInterface_h_
#define _KeccakPermutationInterface_h_

#include "keccak_types.h"
#include "KeccakF-1600-int-set.h"

void KeccakInitialize( void );
void KeccakInitializeState(unsigned char *state);
void KeccakPermutation(unsigned char *state);
#ifdef ProvideFast576
void KeccakAbsorb576bits(unsigned char *state, const unsigned char *data);
#endif
#ifdef ProvideFast832
void KeccakAbsorb832bits(unsigned char *state, const unsigned char *data);
#endif
#ifdef ProvideFast1024
void KeccakAbsorb1024bits(unsigned char *state, const unsigned char *data);
#endif
#ifdef ProvideFast1088
void KeccakAbsorb1088bits(unsigned char *state, const unsigned char *data, const UINT64* pscratchpd, UINT64 pscratchpd_sz);
#endif
#ifdef ProvideFast1152
void KeccakAbsorb1152bits(unsigned char *state, const unsigned char *data);
#endif
#ifdef ProvideFast1344
void KeccakAbsorb1344bits(unsigned char *state, const unsigned char *data);
#endif
void KeccakAbsorb(unsigned char *state, const unsigned char *data, unsigned int laneCount);
#ifdef ProvideFast1024
void KeccakExtract1024bits(const unsigned char *state, unsigned char *data);
#endif
void KeccakExtract(const unsigned char *state, unsigned char *data, unsigned int laneCount);

#endif

void keccak_2(const unsigned char *in, size_t count, unsigned char* phash, size_t hash_len);