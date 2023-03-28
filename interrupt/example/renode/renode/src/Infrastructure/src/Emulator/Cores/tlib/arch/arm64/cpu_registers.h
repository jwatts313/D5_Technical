#ifndef __ARM_CPU_REGISTERS__
#define __ARM_CPU_REGISTERS__

#include "cpu-defs.h"

// Arm64 numbers are in line with GDB's 'arch/aarch64.h'.
typedef enum {
#ifdef TARGET_ARM64
    X_0_64       = 0,
    X_1_64       = 1,
    X_2_64       = 2,
    X_3_64       = 3,
    X_4_64       = 4,
    X_5_64       = 5,
    X_6_64       = 6,
    X_7_64       = 7,
    X_8_64       = 8,
    X_9_64       = 9,
    X_10_64      = 10,
    X_11_64      = 11,
    X_12_64      = 12,
    X_13_64      = 13,
    X_14_64      = 14,
    X_15_64      = 15,
    X_16_64      = 16,
    X_17_64      = 17,
    X_18_64      = 18,
    X_19_64      = 19,
    X_20_64      = 20,
    X_21_64      = 21,
    X_22_64      = 22,
    X_23_64      = 23,
    X_24_64      = 24,
    X_25_64      = 25,
    X_26_64      = 26,
    X_27_64      = 27,
    X_28_64      = 28,
    X_29_64      = 29,
    X_30_64      = 30,
    LR_64        = 30,
    X_31_64      = 31,
    SP_64        = 31,
    PC_64        = 32,
    PSTATE_32    = 33,
    CPSR_32      = 33,
    FPSR_32      = 66,
    FPCR_32      = 67,
#else
    R_0_32       = 0,
    R_1_32       = 1,
    R_2_32       = 2,
    R_3_32       = 3,
    R_4_32       = 4,
    R_5_32       = 5,
    R_6_32       = 6,
    R_7_32       = 7,
    R_8_32       = 8,
    R_9_32       = 9,
    R_10_32      = 10,
    R_11_32      = 11,
    R_12_32      = 12,
    R_13_32      = 13,
    SP_32        = 13,
    R_14_32      = 14,
    LR_32        = 14,
    R_15_32      = 15,
    PC_32        = 15,
    CPSR_32      = 25,
  #ifdef TARGET_PROTO_ARM_M
    Control_32   = 18,
    BasePri_32   = 19,
    VecBase_32   = 20,
    CurrentSP_32 = 21,
    OtherSP_32   = 22,
    FPCCR_32     = 23,
    FPCAR_32     = 24,
    FPDSCR_32    = 26,
    CPACR_32     = 27,
    PRIMASK_32   = 28,
  #endif
#endif
} Registers;

/* The return address is stored here */
#if TARGET_LONG_BITS == 64
#define RA   X_30_64
#endif
#if TARGET_LONG_BITS == 32
#define RA   R_14_32
#endif

#endif /* #ifndef __ARM_CPU_REGISTERS__ */
