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


#define readScratchPadData(base_addr, off) pscratchpd[base_addr##_0 + off] ^ pscratchpd[base_addr##_1 + off] ^ pscratchpd[base_addr##_2 + off] ^ pscratchpd[base_addr##_3 + off]

#define prepareScratchPadAddr(base_addr, no, Xx) base_addr##_##no = ((Xx)%scr_hashes_size)<<2 

#define updateWildStateBlock(block_no, A, B, C, D) \
  prepareScratchPadAddr(base_addr_##block_no, 0, A); \
  prepareScratchPadAddr(base_addr_##block_no, 1, B); \
  prepareScratchPadAddr(base_addr_##block_no, 2, C); \
  prepareScratchPadAddr(base_addr_##block_no, 3, D); \
  A ^= readScratchPadData(base_addr_##block_no, 0); \
  B ^= readScratchPadData(base_addr_##block_no, 1); \
  C ^= readScratchPadData(base_addr_##block_no, 2); \
  D ^= readScratchPadData(base_addr_##block_no, 3); 


#define updateWildToState(X) \
  { \
  updateWildStateBlock(0, X##ba, X##be, X##bi, X##bo); \
  updateWildStateBlock(1, X##bu, X##ga, X##ge, X##gi); \
  updateWildStateBlock(2, X##go, X##gu, X##ka, X##ke); \
  updateWildStateBlock(3, X##ki, X##ko, X##ku, X##ma); \
  updateWildStateBlock(4, X##me, X##mi, X##mo, X##mu); \
  updateWildStateBlock(5, X##sa, X##se, X##si, X##so); \
  }


#define wild_round( S, T) \
  updateWildToState(S) \
  prepareWildTheta(S) \
  thetaRhoPiChiIota(0, S, T) \
  


#define wild_rounds \
  prepareWildTheta(A)  \
  thetaRhoPiChiIota( 0, A, E) \
  wild_round(E, A) \
  wild_round(A, E) \
  wild_round(E, A) \
  wild_round(A, E) \
  wild_round(E, A) \
  wild_round(A, E) \
  wild_round(E, A) \
  wild_round(A, E) \
  wild_round(E, A) \
  wild_round(A, E) \
  wild_round(E, A) \
  wild_round(A, E) \
  wild_round(E, A) \
  wild_round(A, E) \
  wild_round(E, A) \
  wild_round(A, E) \
  wild_round(E, A) \
  wild_round(A, E) \
  wild_round(E, A) \
  wild_round(A, E) \
  wild_round(E, A) \
  wild_round(A, E) \
  wild_round(E, A) \
  copyToState(state, A) \


#if (Unrolling == 24)
#define rounds \
    prepareTheta \
    thetaRhoPiChiIotaPrepareTheta( 0, A, E) \
    thetaRhoPiChiIotaPrepareTheta( 1, E, A) \
    thetaRhoPiChiIotaPrepareTheta( 2, A, E) \
    thetaRhoPiChiIotaPrepareTheta( 3, E, A) \
    thetaRhoPiChiIotaPrepareTheta( 4, A, E) \
    thetaRhoPiChiIotaPrepareTheta( 5, E, A) \
    thetaRhoPiChiIotaPrepareTheta( 6, A, E) \
    thetaRhoPiChiIotaPrepareTheta( 7, E, A) \
    thetaRhoPiChiIotaPrepareTheta( 8, A, E) \
    thetaRhoPiChiIotaPrepareTheta( 9, E, A) \
    thetaRhoPiChiIotaPrepareTheta(10, A, E) \
    thetaRhoPiChiIotaPrepareTheta(11, E, A) \
    thetaRhoPiChiIotaPrepareTheta(12, A, E) \
    thetaRhoPiChiIotaPrepareTheta(13, E, A) \
    thetaRhoPiChiIotaPrepareTheta(14, A, E) \
    thetaRhoPiChiIotaPrepareTheta(15, E, A) \
    thetaRhoPiChiIotaPrepareTheta(16, A, E) \
    thetaRhoPiChiIotaPrepareTheta(17, E, A) \
    thetaRhoPiChiIotaPrepareTheta(18, A, E) \
    thetaRhoPiChiIotaPrepareTheta(19, E, A) \
    thetaRhoPiChiIotaPrepareTheta(20, A, E) \
    thetaRhoPiChiIotaPrepareTheta(21, E, A) \
    thetaRhoPiChiIotaPrepareTheta(22, A, E) \
    thetaRhoPiChiIota(23, E, A) \
    copyToState(state, A)
#elif (Unrolling == 12)
#define rounds \
    prepareTheta \
    for(i=0; i<24; i+=12) { \
        thetaRhoPiChiIotaPrepareTheta(i   , A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+ 1, E, A) \
        thetaRhoPiChiIotaPrepareTheta(i+ 2, A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+ 3, E, A) \
        thetaRhoPiChiIotaPrepareTheta(i+ 4, A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+ 5, E, A) \
        thetaRhoPiChiIotaPrepareTheta(i+ 6, A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+ 7, E, A) \
        thetaRhoPiChiIotaPrepareTheta(i+ 8, A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+ 9, E, A) \
        thetaRhoPiChiIotaPrepareTheta(i+10, A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+11, E, A) \
    } \
    copyToState(state, A)
#elif (Unrolling == 8)
#define rounds \
    prepareTheta \
    for(i=0; i<24; i+=8) { \
        thetaRhoPiChiIotaPrepareTheta(i  , A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+1, E, A) \
        thetaRhoPiChiIotaPrepareTheta(i+2, A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+3, E, A) \
        thetaRhoPiChiIotaPrepareTheta(i+4, A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+5, E, A) \
        thetaRhoPiChiIotaPrepareTheta(i+6, A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+7, E, A) \
    } \
    copyToState(state, A)
#elif (Unrolling == 6)
#define rounds \
    prepareTheta \
    for(i=0; i<24; i+=6) { \
        thetaRhoPiChiIotaPrepareTheta(i  , A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+1, E, A) \
        thetaRhoPiChiIotaPrepareTheta(i+2, A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+3, E, A) \
        thetaRhoPiChiIotaPrepareTheta(i+4, A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+5, E, A) \
    } \
    copyToState(state, A)
#elif (Unrolling == 4)
#define rounds \
    prepareTheta \
    for(i=0; i<24; i+=4) { \
        thetaRhoPiChiIotaPrepareTheta(i  , A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+1, E, A) \
        thetaRhoPiChiIotaPrepareTheta(i+2, A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+3, E, A) \
    } \
    copyToState(state, A)
#elif (Unrolling == 3)
#define rounds \
    prepareTheta \
    for(i=0; i<24; i+=3) { \
        thetaRhoPiChiIotaPrepareTheta(i  , A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+1, E, A) \
        thetaRhoPiChiIotaPrepareTheta(i+2, A, E) \
        copyStateVariables(A, E) \
    } \
    copyToState(state, A)
#elif (Unrolling == 2)
#define rounds \
    prepareTheta \
    for(i=0; i<24; i+=2) { \
        thetaRhoPiChiIotaPrepareTheta(i  , A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+1, E, A) \
    } \
    copyToState(state, A)
#elif (Unrolling == 1)
#define rounds \
    prepareTheta \
    for(i=0; i<24; i++) { \
        thetaRhoPiChiIotaPrepareTheta(i  , A, E) \
        copyStateVariables(A, E) \
    } \
    copyToState(state, A)
#else
#error "Unrolling is not correctly specified!"
#endif
