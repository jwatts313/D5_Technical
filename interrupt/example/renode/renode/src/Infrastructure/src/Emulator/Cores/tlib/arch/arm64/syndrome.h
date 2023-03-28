/*
 * Copyright (c) Antmicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SYNDROME_H_
#define SYNDROME_H_

// D17.2.37
#define SYN_A64_COND         14 // b1110
#define SYN_A64_CV           1

#define SYN_EC_SHIFT         26
#define SYN_IL               1 << 25

#define SYN_DATA_ABORT_ISV   1 << 24
#define SYN_DATA_ABORT_S1PTW 1 << 7

// Based on EC field description in D17.2.37-39.
typedef enum {
    SYN_EC_UNKNOWN_REASON              = 0x00,
    SYN_EC_TRAPPED_WF                  = 0x01,
    SYN_EC_AA32_TRAPPED_MCR_MCR_CP15   = 0x03,
    SYN_EC_AA32_TRAPPED_MCRR_MRRC_CP15 = 0x04,
    SYN_EC_AA32_TRAPPED_MCR_MRC_CP14   = 0x05,
    SYN_EC_AA32_TRAPPED_LDC_STC        = 0x06,
    SYN_EC_TRAPPED_SME_SVE_SIMD_FP     = 0x07,
    SYN_EC_TRAPPED_VMRS                = 0x08,  // AKA FPID access trap
    SYN_EC_TRAPPED_PAUTH_USE           = 0x09,
    SYN_EC_TRAPPED_LD64B_OR_ST64B      = 0x0A,
    SYN_EC_AA32_TRAPPED_MRRC_CP14      = 0x0C,
    SYN_EC_BRANCH_TARGET               = 0x0D,
    SYN_EC_ILLEGAL_EXECUTION_STATE     = 0x0E,
    SYN_EC_AA32_SVC                    = 0x11,
    SYN_EC_AA32_HVC                    = 0x12,
    SYN_EC_AA32_SMC                    = 0x13,
    SYN_EC_AA64_SVC                    = 0x15,
    SYN_EC_AA64_HVC                    = 0x16,
    SYN_EC_AA64_SMC                    = 0x17,
    SYN_EC_TRAPPED_MSR_MRS_SYSTEM_INST = 0x18,
    SYN_EC_TRAPPED_SVE                 = 0x19,
    SYN_EC_TRAPPED_ERET_ERETAA_ERETAB  = 0x1A,
    SYN_EC_TME_TSTART                  = 0x1B,
    SYN_EC_POINTER_AUTHENTICATION      = 0x1C,
    SYN_EC_TRAPPED_SME                 = 0x1D,
    SYN_EC_GRANULE_PROTECTION_CHECK    = 0x1E,
    SYN_EC_IMPLEMENTATION_DEFINED_EL3  = 0x1F,
    SYN_EC_INSTRUCTION_ABORT_LOWER_EL  = 0x20,
    SYN_EC_INSTRUCTION_ABORT_SAME_EL   = 0x21,
    SYN_EC_PC_ALIGNMENT_FAULT          = 0x22,
    SYN_EC_DATA_ABORT_LOWER_EL         = 0x24,
    SYN_EC_DATA_ABORT_SAME_EL          = 0x25,
    SYN_EC_SP_ALIGNMENT_FAULT          = 0x26,
    SYN_EC_MEMORY_OPERATION            = 0x27,
    SYN_EC_AA32_TRAPPED_FLOATING_POINT = 0x28,
    SYN_EC_AA64_TRAPPED_FLOATING_POINT = 0x2C,
    SYN_EC_SERROR                      = 0x2F,
    SYN_EC_BREAKPOINT_LOWER_EL         = 0x30,
    SYN_EC_BREAKPOINT_SAME_EL          = 0x31,
    SYN_EC_SOFTWARESTEP_LOWER_EL       = 0x32,
    SYN_EC_SOFTWARESTEP_SAME_EL        = 0x33,
    SYN_EC_WATCHPOINT_LOWER_EL         = 0x34,
    SYN_EC_WATCHPOINT_SAME_EL          = 0x35,
    SYN_EC_AA32_BKPT                   = 0x38,
    SYN_EC_AA32_VECTOR_CATCH           = 0x3A,
    SYN_EC_AA64_BKPT                   = 0x3C,
} SyndromeExceptionClass;

// Based on DFSC description in "ISS encoding for an exception from a Data Abort" (D17.2.37-39).
typedef enum {
    // Lacks external abort, FEAT_LPA2, FEAT_RAS, FEAT_RME and FEAT_HAFDBS fault codes.
    SYN_DFSC_ADDRESS_SIZE_FAULT_LEVEL0,
    SYN_DFSC_ADDRESS_SIZE_FAULT_LEVEL1,
    SYN_DFSC_ADDRESS_SIZE_FAULT_LEVEL2,
    SYN_DFSC_ADDRESS_SIZE_FAULT_LEVEL3,
    SYN_DFSC_TRANSLATION_FAULT_LEVEL0,
    SYN_DFSC_TRANSLATION_FAULT_LEVEL1,
    SYN_DFSC_TRANSLATION_FAULT_LEVEL2,
    SYN_DFSC_TRANSLATION_FAULT_LEVEL3,
    SYN_DFSC_ACCESS_FLAG_FAULT_LEVEL0,  // When FEAT_LPA2 is implemented
    SYN_DFSC_ACCESS_FLAG_FAULT_LEVEL1,
    SYN_DFSC_ACCESS_FLAG_FAULT_LEVEL2,
    SYN_DFSC_ACCESS_FLAG_FAULT_LEVEL3,
    SYN_DFSC_PERMISSION_FAULT_LEVEL1     = 0x0D,
    SYN_DFSC_PERMISSION_FAULT_LEVEL2,
    SYN_DFSC_PERMISSION_FAULT_LEVEL3,
    SYN_DFSC_SYNCHRONOUS_TAG_CHECK_FAULT = 0x11,  // When FEAT_MTE2 is implemented
    SYN_DFSC_ALIGNMENT_FAULT             = 0x21,
    SYN_DFSC_TLB_CONFLICT_ABORT          = 0x30,
    SYN_DFSC_IMPLEMENTATION_DEFINED_0x34 = 0x34,
    SYN_DFSC_IMPLEMENTATION_DEFINED_0x35 = 0x35,
} SyndromeDataFaultStatusCode;

static inline uint64_t syndrome64_create(uint64_t instruction_specific_syndrome2, SyndromeExceptionClass exception_class,
                                         uint32_t instruction_length, uint32_t instruction_specific_syndrome)
{
    tlib_assert(instruction_specific_syndrome2 < (1 << 5));
    tlib_assert(exception_class < (1 << 6));
    tlib_assert(instruction_length < (1 << 1));
    tlib_assert(instruction_specific_syndrome < (1 << 25));

    // D17-5658: Some of the exceptions should always have IL bit set to 1.
    switch (exception_class) {
    case SYN_EC_SERROR:
    case SYN_EC_INSTRUCTION_ABORT_LOWER_EL:
    case SYN_EC_INSTRUCTION_ABORT_SAME_EL:
    case SYN_EC_PC_ALIGNMENT_FAULT:
    case SYN_EC_SP_ALIGNMENT_FAULT:
    case SYN_EC_ILLEGAL_EXECUTION_STATE:
    case SYN_EC_SOFTWARESTEP_LOWER_EL:
    case SYN_EC_SOFTWARESTEP_SAME_EL:
    case SYN_EC_AA64_BKPT:
    case SYN_EC_UNKNOWN_REASON:
        tlib_assert(instruction_length);
        break;
    case SYN_EC_DATA_ABORT_LOWER_EL:
    case SYN_EC_DATA_ABORT_SAME_EL:
        // "A Data Abort exception for which the value of the ISV bit is 0."
        if (!(instruction_specific_syndrome & SYN_DATA_ABORT_ISV)) {
            tlib_assert(instruction_length);
        }
        break;
    default:
        break;
    }
    return instruction_specific_syndrome2 << 32 | exception_class << SYN_EC_SHIFT | (instruction_length ? SYN_IL : 0)
                                                | instruction_specific_syndrome;
}

static inline uint32_t syndrome32_create(SyndromeExceptionClass exception_class, uint32_t instruction_length,
                                         uint32_t instruction_specific_syndrome)
{
    return (uint32_t)syndrome64_create(0, exception_class, instruction_length, instruction_specific_syndrome);
}

static inline uint32_t syn_aa64_sysregtrap(unsigned int op0, unsigned int op1, unsigned int op2, unsigned int crn,
                                           unsigned int crm, unsigned int rt, bool isread)
{
    // D17.2.37
    unsigned int iss = SYN_A64_CV << 24 | SYN_A64_COND << 20 | op2 << 17 | op1 << 14 | crn << 10 | rt << 5 | crm << 1 | isread;

    // TODO: iss2 if FEAT_LS64 is implemented
    return syndrome32_create(SYN_EC_TRAPPED_MSR_MRS_SYSTEM_INST, 0, iss);
}

static inline uint32_t syn_uncategorized()
{
    // D17.2.37, D17-5659
    return syndrome32_create(SYN_EC_UNKNOWN_REASON, 1, 0);
}

// param0 is always 0 so it doesn't change anything.
static inline uint32_t syn_data_abort_with_iss(unsigned int param0, unsigned int sas, unsigned int sse, unsigned int srt,
                                               unsigned int sf, unsigned int ar, unsigned int ea, unsigned int cm,
                                               unsigned int s1ptw, unsigned int wnr, unsigned int dfsc, bool is_16bit)
{
    // IL bit is 0 for 16-bit and 1 for 32-bit instruction trapped.
    bool il = !is_16bit;

    // FAR not valid for "External abort other..." (DFSC=0x10).
    // Let's assume FAR is always valid for such an abort so FnV will always be 0.
    unsigned int fnv = 0;

    // TODO: implement this properly
    // only applicable:
    // When (DFSC == 0b00xxxx || DFSC == 0b101011) && DFSC != 0b0000xx
    // When FEAT_RAS is implemented and DFSC == 0b010000
    unsigned int lst = 0;
    // TODO: implement this properly
    // only when FEAT_NV2 implemented
    unsigned int vncr = 0;
    unsigned int iss = SYN_DATA_ABORT_ISV | sas << 22 | sse << 21 | srt << 16 | sf << 15 | ar << 14 | vncr << 13 | lst << 11
                                          | fnv << 10 | ea << 9 | cm << 8 | s1ptw << 7 | wnr << 6 | dfsc;

    // A proper EC will be set when raising the exception.
    // It can't be established during translation whether it should be SYN_EC_DATA_ABORT_LOWER_EL or _SAME_EL.
    return syndrome32_create(0x0, il, iss);
}

// "No ISS" in the name only applies to ISS[23:14] bits (ISV=0 case).
static inline uint32_t syn_data_abort_no_iss(bool same_el, unsigned int fnv, unsigned int ea, unsigned int cm, unsigned int s1ptw,
                                             unsigned int wnr, unsigned int dfsc)
{
    // D17-5658: IL bit is 1 for "A Data Abort exception for which the value of the ISV bit is 0".
    bool il = 1;
    // Notice no ISV and instruction-specific bits (11:23).
    unsigned int iss = fnv << 10 | ea << 9 | cm << 8 | s1ptw << 7 | wnr << 6 | dfsc;

    // It seems we can set a proper EC straight away in such a case.
    return syndrome32_create(same_el ? SYN_EC_DATA_ABORT_SAME_EL : SYN_EC_DATA_ABORT_LOWER_EL, il, iss);
}

static inline SyndromeExceptionClass syn_get_ec(uint32_t syndrome)
{
    return syndrome >> SYN_EC_SHIFT;
}

static inline void syn_set_ec(uint64_t *syndrome, SyndromeExceptionClass new_ec)
{
    uint64_t ec_mask = 0x3F << SYN_EC_SHIFT;
    *syndrome &= ~ec_mask;
    *syndrome |= new_ec << SYN_EC_SHIFT;
}

static inline uint32_t syn_wfx(int cv, int cond, int ti, bool is_16bit)
{
    unsigned int iss = cv << 24 | cond << 20 | ti;

    return syndrome32_create(SYN_EC_TRAPPED_WF, is_16bit, iss);
}

static inline uint32_t syn_aa64_hvc(uint32_t imm16)
{
    return syndrome32_create(SYN_EC_AA64_HVC, 0, imm16);
}

static inline uint32_t syn_aa64_smc(uint32_t imm16)
{
    return syndrome32_create(SYN_EC_AA64_SMC, 0, imm16);
}

static inline uint32_t syn_aa64_svc(uint32_t imm16)
{
    return syndrome32_create(SYN_EC_AA64_SVC, 0, imm16);
}

static inline uint32_t syn_aa64_bkpt(uint32_t comment)
{
    return syndrome32_create(SYN_EC_AA64_BKPT, 1, comment);
}

static inline uint32_t syn_btitrap(uint32_t btype)
{
    return syndrome32_create(SYN_EC_BRANCH_TARGET, 0, btype);
}

static inline uint32_t syn_illegalstate()
{
    return syndrome32_create(SYN_EC_ILLEGAL_EXECUTION_STATE, 1, 0);
}

static inline uint32_t syn_smetrap(uint32_t smtc, bool is_16bit)
{
    return syndrome32_create(SYN_EC_TRAPPED_SME, is_16bit, smtc);
}

static inline uint32_t syn_sve_access_trap()
{
    return syndrome32_create(SYN_EC_TRAPPED_SVE, 0, 0);
}

static inline uint32_t syn_swstep(uint32_t param0, uint32_t isv, uint32_t ex)
{
    unsigned int iss = isv << 24 | ex << 6 | 0x22 /*Debug exception from data sheet*/;

    // TODO: Choose between LOWER_EL and SAME_EL.
    return syndrome32_create(SYN_EC_SOFTWARESTEP_LOWER_EL, 1, iss);
}

#endif  // SYNDROME_H_
