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

#include <string.h>
#include <stdlib.h>
#include "brg_endian.h"
#include "KeccakF-1600-opt64-settings.h"
#include "KeccakF-1600-interface.h"

#include <immintrin.h>

typedef unsigned char UINT8;


#if defined(__GNUC__)
#define ALIGN __attribute__ ((aligned(32)))
#elif defined(_MSC_VER)
#define ALIGN __declspec(align(32))
#else
#define ALIGN
#endif

#if defined(UseSSE)
    //#include <x86intrin.h>
    typedef __m128i V64;
    typedef __m128i V128;
    typedef union {
        V128 v128;
        UINT64 v64[2];
    } V6464;

    #define ANDnu64(a, b)       _mm_andnot_si128(a, b)
    #define LOAD64(a)           _mm_loadl_epi64((const V64 *)&(a))
    #define CONST64(a)          _mm_loadl_epi64((const V64 *)&(a))
    #define ROL64(a, o)         _mm_or_si128(_mm_slli_epi64(a, o), _mm_srli_epi64(a, 64-(o)))
    #define STORE64(a, b)       _mm_storel_epi64((V64 *)&(a), b)
    #define XOR64(a, b)         _mm_xor_si128(a, b)
    #define XOReq64(a, b)       a = _mm_xor_si128(a, b)
    #define SHUFFLEBYTES128(a, b)   _mm_shuffle_epi8(a, b)

    #define ANDnu128(a, b)      _mm_andnot_si128(a, b)
    #define LOAD6464(a, b)      _mm_set_epi64((__m64)(a), (__m64)(b))
    #define CONST128(a)         _mm_load_si128((const V128 *)&(a))
    #define LOAD128(a)          _mm_load_si128((const V128 *)&(a))
    #define LOAD128u(a)         _mm_loadu_si128((const V128 *)&(a))
    #define ROL64in128(a, o)    _mm_or_si128(_mm_slli_epi64(a, o), _mm_srli_epi64(a, 64-(o)))
    #define STORE128(a, b)      _mm_store_si128((V128 *)&(a), b)
    #define XOR128(a, b)        _mm_xor_si128(a, b)
    #define XOReq128(a, b)      a = _mm_xor_si128(a, b)
    #define GET64LOLO(a, b)     _mm_unpacklo_epi64(a, b)
    #define GET64HIHI(a, b)     _mm_unpackhi_epi64(a, b)
    #define COPY64HI2LO(a)      _mm_shuffle_epi32(a, 0xEE)
    #define COPY64LO2HI(a)      _mm_shuffle_epi32(a, 0x44)
    #define ZERO128()           _mm_setzero_si128()

    #ifdef UseOnlySIMD64
    #include "KeccakF-1600-simd64.macros"
    #else
ALIGN const UINT64 rho8_56[2] = {0x0605040302010007, 0x080F0E0D0C0B0A09};
    #include "KeccakF-1600-simd128.macros"
    #endif

    #ifdef UseBebigokimisa
    #error "UseBebigokimisa cannot be used in combination with UseSSE"
    #endif
#elif defined(UseXOP)
    #include <x86intrin.h>
    typedef __m128i V64;
    typedef __m128i V128;
   
    #define LOAD64(a)           _mm_loadl_epi64((const V64 *)&(a))
    #define CONST64(a)          _mm_loadl_epi64((const V64 *)&(a))
    #define STORE64(a, b)       _mm_storel_epi64((V64 *)&(a), b)
    #define XOR64(a, b)         _mm_xor_si128(a, b)
    #define XOReq64(a, b)       a = _mm_xor_si128(a, b)

    #define ANDnu128(a, b)      _mm_andnot_si128(a, b)
    #define LOAD6464(a, b)      _mm_set_epi64((__m64)(a), (__m64)(b))
    #define CONST128(a)         _mm_load_si128((const V128 *)&(a))
    #define LOAD128(a)          _mm_load_si128((const V128 *)&(a))
    #define LOAD128u(a)         _mm_loadu_si128((const V128 *)&(a))
    #define STORE128(a, b)      _mm_store_si128((V128 *)&(a), b)
    #define XOR128(a, b)        _mm_xor_si128(a, b)
    #define XOReq128(a, b)      a = _mm_xor_si128(a, b)
    #define ZERO128()           _mm_setzero_si128()

    #define SWAP64(a)           _mm_shuffle_epi32(a, 0x4E)
    #define GET64LOLO(a, b)     _mm_unpacklo_epi64(a, b)
    #define GET64HIHI(a, b)     _mm_unpackhi_epi64(a, b)
    #define GET64LOHI(a, b)     ((__m128i)_mm_blend_pd((__m128d)a, (__m128d)b, 2))
    #define GET64HILO(a, b)     SWAP64(GET64LOHI(b, a))
    #define COPY64HI2LO(a)      _mm_shuffle_epi32(a, 0xEE)
    #define COPY64LO2HI(a)      _mm_shuffle_epi32(a, 0x44)
 
    #define ROL6464same(a, o)   _mm_roti_epi64(a, o)
    #define ROL6464(a, r1, r2)  _mm_rot_epi64(a, CONST128( rot_##r1##_##r2 ))
ALIGN const UINT64 rot_0_20[2]  = { 0, 20};
ALIGN const UINT64 rot_44_3[2]  = {44,  3};
ALIGN const UINT64 rot_43_45[2] = {43, 45};
ALIGN const UINT64 rot_21_61[2] = {21, 61};
ALIGN const UINT64 rot_14_28[2] = {14, 28};
ALIGN const UINT64 rot_1_36[2]  = { 1, 36};
ALIGN const UINT64 rot_6_10[2]  = { 6, 10};
ALIGN const UINT64 rot_25_15[2] = {25, 15};
ALIGN const UINT64 rot_8_56[2]  = { 8, 56};
ALIGN const UINT64 rot_18_27[2] = {18, 27};
ALIGN const UINT64 rot_62_55[2] = {62, 55};
ALIGN const UINT64 rot_39_41[2] = {39, 41};

#if defined(UseSimulatedXOP)
    // For debugging purposes, when XOP is not available
    #undef ROL6464
    #undef ROL6464same
    #define ROL6464same(a, o)   _mm_or_si128(_mm_slli_epi64(a, o), _mm_srli_epi64(a, 64-(o)))
    V128 ROL6464(V128 a, int r0, int r1)
    {
        V128 a0 = ROL64(a, r0);
        V128 a1 = COPY64HI2LO(ROL64(a, r1));
        return GET64LOLO(a0, a1);
    }
#endif
    
    #include "KeccakF-1600-xop.macros"

    #ifdef UseBebigokimisa
    #error "UseBebigokimisa cannot be used in combination with UseXOP"
    #endif
#elif defined(UseMMX)
    #include <mmintrin.h>
    typedef __m64 V64;
    #define ANDnu64(a, b)       _mm_andnot_si64(a, b)

    #if (defined(_MSC_VER) || defined (__INTEL_COMPILER))
        #define LOAD64(a)       *(V64*)&(a)
        #define CONST64(a)      *(V64*)&(a)
        #define STORE64(a, b)   *(V64*)&(a) = b
    #else
        #define LOAD64(a)       (V64)a
        #define CONST64(a)      (V64)a
        #define STORE64(a, b)   a = (UINT64)b
    #endif
    #define ROL64(a, o)         _mm_or_si64(_mm_slli_si64(a, o), _mm_srli_si64(a, 64-(o)))
    #define XOR64(a, b)         _mm_xor_si64(a, b)
    #define XOReq64(a, b)       a = _mm_xor_si64(a, b)

    #include "KeccakF-1600-simd64.macros"

    #ifdef UseBebigokimisa
    #error "UseBebigokimisa cannot be used in combination with UseMMX"
    #endif
#else
    #if defined(_MSC_VER)
    #define ROL64(a, offset) _rotl64(a, offset)
    #elif defined(UseSHLD)
      #define ROL64(x,N) ({ \
        register UINT64 __out; \
        register UINT64 __in = x; \
        __asm__ ("shld %2,%0,%0" : "=r"(__out) : "0"(__in), "i"(N)); \
        __out; \
      })
    #else
    #define ROL64(a, offset) ((((UINT64)a) << offset) ^ (((UINT64)a) >> (64-offset)))
    #endif

    #include "KeccakF-1600-64.inl"
#endif

#include "KeccakF-1600-unrolling.inl"

void KeccakPermutationOnWords(UINT64 *state)
{
    declareABCDE
#if (Unrolling != 24)
    unsigned int i;
#endif

    copyFromState(A, state)
    rounds
#if defined(UseMMX)
    _mm_empty();
#endif
}

void KeccakPermutationOnWordsAfterXoring(UINT64 *state, const UINT64 *input, unsigned int laneCount)
{
    declareABCDE
#if (Unrolling != 24)
    unsigned int i;
#endif
	unsigned int j;

    for(j=0; j<laneCount; j++)
        state[j] ^= input[j];	
    copyFromState(A, state)
    rounds
#if defined(UseMMX)
    _mm_empty();
#endif
}

#ifdef ProvideFast576
void KeccakPermutationOnWordsAfterXoring576bits(UINT64 *state, const UINT64 *input)
{
    declareABCDE
#if (Unrolling != 24)
    unsigned int i;
#endif

    copyFromStateAndXor576bits(A, state, input)
    rounds
#if defined(UseMMX)
    _mm_empty();
#endif
}
#endif

#ifdef ProvideFast832
void KeccakPermutationOnWordsAfterXoring832bits(UINT64 *state, const UINT64 *input)
{
    declareABCDE
#if (Unrolling != 24)
    unsigned int i;
#endif

    copyFromStateAndXor832bits(A, state, input)
    rounds
#if defined(UseMMX)
    _mm_empty();
#endif
}
#endif

#ifdef ProvideFast1024
void KeccakPermutationOnWordsAfterXoring1024bits(UINT64 *state, const UINT64 *input)
{
    declareABCDE
#if (Unrolling != 24)
    unsigned int i;
#endif

    copyFromStateAndXor1024bits(A, state, input)
    rounds
#if defined(UseMMX)
    _mm_empty();
#endif
}
#endif

#ifdef ProvideFast1088
void KeccakPermutationOnWordsAfterXoring1088bits(UINT64 *state, const UINT64 *input, const UINT64* pscratchpd, UINT64 pscratchpd_sz)
{
    declareWildABCDE
#if (Unrolling != 24)
    unsigned int i;
#endif

    //copyFromStateAndXor1088bits(A, state, input);
    Aba = state[ 0]^input[ 0]; 
    Abe = state[ 1]^input[ 1]; 
    Abi = state[ 2]^input[ 2];
    Abo = state[ 3]^input[ 3];
    Abu = state[ 4]^input[ 4];
    Aga = state[ 5]^input[ 5]; 
    Age = state[ 6]^input[ 6]; 
    Agi = state[ 7]^input[ 7]; 
    Ago = state[ 8]^input[ 8]; 
    Agu = state[ 9]^input[ 9]; 
    Aka = state[10]^input[10]; 
    Ake = state[11]^input[11]; 
    Aki = state[12]^input[12]; 
    Ako = state[13]^input[13]; 
    Aku = state[14]^input[14]; 
    Ama = state[15]^input[15]; 
    Ame = state[16]^input[16]; 
    Ami = state[17]; 
    Amo = state[18]; 
    Amu = state[19]; 
    Asa = state[20]; 
    Ase = state[21]; 
    Asi = state[22]; 
    Aso = state[23]; 
    Asu = state[24]; 

    /*Ca = Aba^Aga^Aka*Ama*Asa; 
    Ce = Abe^Age^Ake*Ame*Ase; 
    Ci = Abi^Agi^Aki*Ami*Asi; 
    Co = Abo^Ago^Ako*Amo*Aso; 
    Cu = Abu^Agu^Aku*Amu*Asu; 
    
    Da = Cu^ROL64(Ce, 1);
    De = Ca^ROL64(Ci, 1);
    Di = Ce^ROL64(Co, 1);
    Do = Ci^ROL64(Cu, 1);
    Du = Co^ROL64(Ca, 1);

    Aba ^= Da;
    Bba = Aba;
    Age ^= De;
    Bbe = ROL64(Age, 44);
    Aki ^= Di;
    Bbi = ROL64(Aki, 43);
    Amo ^= Do;
    Bbo = ROL64(Amo, 21);
    Asu ^= Du;
    Bbu = ROL64(Asu, 14);
    Eba =   Bba ^((~Bbe)&  Bbi );
    Eba ^= KeccakF1600RoundConstants[0];
    Ca = Eba;
    Ebe =   Bbe ^((~Bbi)&  Bbo );
    Ce = Ebe;
    Ebi =   Bbi ^((~Bbo)&  Bbu );
    Ci = Ebi;
    Ebo =   Bbo ^((~Bbu)&  Bba );
    Co = Ebo;
    Ebu =   Bbu ^((~Bba)&  Bbe );
    Cu = Ebu;

    Abo ^= Do;
    Bga = ROL64(Abo, 28);
    Agu ^= Du;
    Bge = ROL64(Agu, 20);
    Aka ^= Da;
    Bgi = ROL64(Aka, 3);
    Ame ^= De;
    Bgo = ROL64(Ame, 45);
    Asi ^= Di;
    Bgu = ROL64(Asi, 61);
    Ega =   Bga ^((~Bge)&  Bgi );
    Ca ^= Ega;
    Ege =   Bge ^((~Bgi)&  Bgo );
    Ce ^= Ege;
    Egi =   Bgi ^((~Bgo)&  Bgu );
    Ci ^= Egi;
    Ego =   Bgo ^((~Bgu)&  Bga );
    Co ^= Ego;
    Egu =   Bgu ^((~Bga)&  Bge );
    Cu ^= Egu;

    Abe ^= De;
    Bka = ROL64(Abe, 1);
    Agi ^= Di;
    Bke = ROL64(Agi, 6);
    Ako ^= Do;
    Bki = ROL64(Ako, 25);
    Amu ^= Du;
    Bko = ROL64(Amu, 8);
    Asa ^= Da;
    Bku = ROL64(Asa, 18);
    Eka =   Bka ^((~Bke)&  Bki );
    Cam = Eka;
    Eke =   Bke ^((~Bki)&  Bko );
    Cem = Eke;
    Eki =   Bki ^((~Bko)&  Bku );
    Cim = Eki;
    Eko =   Bko ^((~Bku)&  Bka );
    Com = Eko;
    Eku =   Bku ^((~Bka)&  Bke );
    Cum = Eku;

    Abu ^= Du;
    Bma = ROL64(Abu, 27);
    Aga ^= Da;
    Bme = ROL64(Aga, 36);
    Ake ^= De;
    Bmi = ROL64(Ake, 10);
    Ami ^= Di;
    Bmo = ROL64(Ami, 15);
    Aso ^= Do;
    Bmu = ROL64(Aso, 56);
    Ema =   Bma ^((~Bme)&  Bmi );
    Cam *= Ema;
    Eme =   Bme ^((~Bmi)&  Bmo );
    Cem *= Eme;
    Emi =   Bmi ^((~Bmo)&  Bmu );
    Cim *= Emi;
    Emo =   Bmo ^((~Bmu)&  Bma );
    Com *= Emo;
    Emu =   Bmu ^((~Bma)&  Bme );
    Cum *= Emu;

    Abi ^= Di;
    Bsa = ROL64(Abi, 62);
    Ago ^= Do;
    Bse = ROL64(Ago, 55);
    Aku ^= Du;
    Bsi = ROL64(Aku, 39);
    Ama ^= Da;
    Bso = ROL64(Ama, 41);
    Ase ^= De;
    Bsu = ROL64(Ase, 2);
    Esa =   Bsa ^((~Bse)&  Bsi );
    Cam *= Esa;
    Ca ^= Cam;
    Ese =   Bse ^((~Bsi)&  Bso );
    Cem *= Ese;
    Ce ^= Cem;
    Esi =   Bsi ^((~Bso)&  Bsu );
    Cim *= Esi;
    Ci ^= Cim;
    Eso =   Bso ^((~Bsu)&  Bsa );
    Com *= Eso;
    Co ^= Com;
    Esu =   Bsu ^((~Bsa)&  Bse );
    Cum *= Esu;
    Cu ^= Cum;*/

    prepareWildTheta
    wildThetaRhoPiChiIotaPrepareTheta( 0, A, E)
    copyToState(state, E)  
    updateWildToState(E)
    copyToState(state, E)  
    wildThetaRhoPiChiIotaPrepareTheta( 1, E, A)
    copyToState(state, E)  


    wild_rounds
#if defined(UseMMX)
    _mm_empty();
#endif
}
#endif

#ifdef ProvideFast1152
void KeccakPermutationOnWordsAfterXoring1152bits(UINT64 *state, const UINT64 *input)
{
    declareABCDE
#if (Unrolling != 24)
    unsigned int i;
#endif

    copyFromStateAndXor1152bits(A, state, input)
    rounds
#if defined(UseMMX)
    _mm_empty();
#endif
}
#endif

#ifdef ProvideFast1344
void KeccakPermutationOnWordsAfterXoring1344bits(UINT64 *state, const UINT64 *input)
{
    declareABCDE
#if (Unrolling != 24)
    unsigned int i;
#endif

    copyFromStateAndXor1344bits(A, state, input)
    rounds
#if defined(UseMMX)
    _mm_empty();
#endif
}
#endif

void KeccakInitialize()
{
}

void KeccakInitializeState(unsigned char *state)
{
    memset(state, 0, 200);
#ifdef UseBebigokimisa
    ((UINT64*)state)[ 1] = ~(UINT64)0;
    ((UINT64*)state)[ 2] = ~(UINT64)0;
    ((UINT64*)state)[ 8] = ~(UINT64)0;
    ((UINT64*)state)[12] = ~(UINT64)0;
    ((UINT64*)state)[17] = ~(UINT64)0;
    ((UINT64*)state)[20] = ~(UINT64)0;
#endif
}

void KeccakPermutation(unsigned char *state)
{
    // We assume the state is always stored as words
    KeccakPermutationOnWords((UINT64*)state);
}

void fromBytesToWord(UINT64 *word, const UINT8 *bytes)
{
    unsigned int i;

    *word = 0;
    for(i=0; i<(64/8); i++)
        *word |= (UINT64)(bytes[i]) << (8*i);
}

#ifdef ProvideFast576
void KeccakAbsorb576bits(unsigned char *state, const unsigned char *data)
{
#if (PLATFORM_BYTE_ORDER == IS_LITTLE_ENDIAN)
    KeccakPermutationOnWordsAfterXoring576bits((UINT64*)state, (const UINT64*)data);
#else
    UINT64 dataAsWords[9];
    unsigned int i;

    for(i=0; i<9; i++)
        fromBytesToWord(dataAsWords+i, data+(i*8));
    KeccakPermutationOnWordsAfterXoring576bits((UINT64*)state, dataAsWords);
#endif
}
#endif

#ifdef ProvideFast832
void KeccakAbsorb832bits(unsigned char *state, const unsigned char *data)
{
#if (PLATFORM_BYTE_ORDER == IS_LITTLE_ENDIAN)
    KeccakPermutationOnWordsAfterXoring832bits((UINT64*)state, (const UINT64*)data);
#else
    UINT64 dataAsWords[13];
    unsigned int i;

    for(i=0; i<13; i++)
        fromBytesToWord(dataAsWords+i, data+(i*8));
    KeccakPermutationOnWordsAfterXoring832bits((UINT64*)state, dataAsWords);
#endif
}
#endif

#ifdef ProvideFast1024
void KeccakAbsorb1024bits(unsigned char *state, const unsigned char *data)
{
#if (PLATFORM_BYTE_ORDER == IS_LITTLE_ENDIAN)
    KeccakPermutationOnWordsAfterXoring1024bits((UINT64*)state, (const UINT64*)data);
#else
    UINT64 dataAsWords[16];
    unsigned int i;

    for(i=0; i<16; i++)
        fromBytesToWord(dataAsWords+i, data+(i*8));
    KeccakPermutationOnWordsAfterXoring1024bits((UINT64*)state, dataAsWords);
#endif
}
#endif

#ifdef ProvideFast1088
void KeccakAbsorb1088bits(unsigned char *state, const unsigned char *data, const UINT64* pscratchpd, UINT64 pscratchpd_sz)
{
#if (PLATFORM_BYTE_ORDER == IS_LITTLE_ENDIAN)
    KeccakPermutationOnWordsAfterXoring1088bits((UINT64*)state, (const UINT64*)data, pscratchpd, pscratchpd_sz);
#else
    UINT64 dataAsWords[17];
    unsigned int i;

    for(i=0; i<17; i++)
        fromBytesToWord(dataAsWords+i, data+(i*8));
    KeccakPermutationOnWordsAfterXoring1088bits((UINT64*)state, dataAsWords);
#endif
}
#endif

#ifdef ProvideFast1152
void KeccakAbsorb1152bits(unsigned char *state, const unsigned char *data)
{
#if (PLATFORM_BYTE_ORDER == IS_LITTLE_ENDIAN)
    KeccakPermutationOnWordsAfterXoring1152bits((UINT64*)state, (const UINT64*)data);
#else
    UINT64 dataAsWords[18];
    unsigned int i;

    for(i=0; i<18; i++)
        fromBytesToWord(dataAsWords+i, data+(i*8));
    KeccakPermutationOnWordsAfterXoring1152bits((UINT64*)state, dataAsWords);
#endif
}
#endif

#ifdef ProvideFast1344
void KeccakAbsorb1344bits(unsigned char *state, const unsigned char *data)
{
#if (PLATFORM_BYTE_ORDER == IS_LITTLE_ENDIAN)
    KeccakPermutationOnWordsAfterXoring1344bits((UINT64*)state, (const UINT64*)data);
#else
    UINT64 dataAsWords[21];
    unsigned int i;

    for(i=0; i<21; i++)
        fromBytesToWord(dataAsWords+i, data+(i*8));
    KeccakPermutationOnWordsAfterXoring1344bits((UINT64*)state, dataAsWords);
#endif
}
#endif

void KeccakAbsorb(unsigned char *state, const unsigned char *data, unsigned int laneCount)
{
#if (PLATFORM_BYTE_ORDER == IS_LITTLE_ENDIAN)
    KeccakPermutationOnWordsAfterXoring((UINT64*)state, (const UINT64*)data, laneCount);
#else
    UINT64 dataAsWords[25];
    unsigned int i;

    for(i=0; i<laneCount; i++)
        fromBytesToWord(dataAsWords+i, data+(i*8));
    KeccakPermutationOnWordsAfterXoring((UINT64*)state, dataAsWords, laneCount);
#endif
}

void fromWordToBytes(UINT8 *bytes, const UINT64 word)
{
    unsigned int i;

    for(i=0; i<(64/8); i++)
        bytes[i] = (word >> (8*i)) & 0xFF;
}

#ifdef ProvideFast1024
void KeccakExtract1024bits(const unsigned char *state, unsigned char *data)
{
#if (PLATFORM_BYTE_ORDER == IS_LITTLE_ENDIAN)
    memcpy(data, state, 128);
#else
    unsigned int i;

    for(i=0; i<16; i++)
        fromWordToBytes(data+(i*8), ((const UINT64*)state)[i]);
#endif
#ifdef UseBebigokimisa
    ((UINT64*)data)[ 1] = ~((UINT64*)data)[ 1];
    ((UINT64*)data)[ 2] = ~((UINT64*)data)[ 2];
    ((UINT64*)data)[ 8] = ~((UINT64*)data)[ 8];
    ((UINT64*)data)[12] = ~((UINT64*)data)[12];
#endif
}
#endif

void KeccakExtract(const unsigned char *state, unsigned char *data, unsigned int laneCount)
{
#if (PLATFORM_BYTE_ORDER == IS_LITTLE_ENDIAN)
    memcpy(data, state, laneCount*8);
#else
    unsigned int i;

    for(i=0; i<laneCount; i++)
        fromWordToBytes(data+(i*8), ((const UINT64*)state)[i]);
#endif
#ifdef UseBebigokimisa
    if (laneCount > 1) {
        ((UINT64*)data)[ 1] = ~((UINT64*)data)[ 1];
        if (laneCount > 2) {
            ((UINT64*)data)[ 2] = ~((UINT64*)data)[ 2];
            if (laneCount > 8) {
                ((UINT64*)data)[ 8] = ~((UINT64*)data)[ 8];
                if (laneCount > 12) {
                    ((UINT64*)data)[12] = ~((UINT64*)data)[12];
                    if (laneCount > 17) {
                        ((UINT64*)data)[17] = ~((UINT64*)data)[17];
                        if (laneCount > 20) {
                            ((UINT64*)data)[20] = ~((UINT64*)data)[20];
                        }
                    }
                }
            }
        }
    }
#endif
}

/*
void keccak_2(const unsigned char *in, size_t count, unsigned char* phash, size_t hash_len)
{
  unsigned char state[200];
  KeccakInitializeState(state); 
  KeccakAbsorb(state, in, count/8);
  KeccakExtract(state, phash, hash_len/8);
}*/
