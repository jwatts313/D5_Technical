/*
 * Copyright (c) Antmicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* This file should be only included at the end of 'cpu.h'. */

// These headers require CPUState, cpu_mmu_index etc.
#include "exec-all.h"

// Math helpers.
#define ALIGN_DOWN(a, b)         ((a) - ((a) % (b)))
#define ALIGN_UP(a, b)           (IS_MULTIPLE_OF(a,b) ? (a) : (ALIGN_DOWN(a,b) + (b)))
#define DIV_ROUND_UP(a, b)       ((a) / (b) + (IS_MULTIPLE_OF(a,b) ? 0 : 1))
#define IS_MULTIPLE_OF(a, b)     (((a) % (b)) == 0)
#define MAX(x, y)                (x > y ? x : y)
#define MIN(a, b)                (a > b ? b : a)

// We don't need to extract non-specific CPUStates from ARM-specific CPUArchState.
// It's the same for us.
#define env_cpu(env)             env

// We have these as consts.
#define float16_default_nan(...) float16_default_nan
#define float32_default_nan(...) float32_default_nan
#define float64_default_nan(...) float64_default_nan

// Adjust assertion calls.
#define g_assert_not_reached() tlib_assert_not_reached()

// The define is used to avoid replacing all the 'pc_next' uses.
#define pc_next pc

#define sizeof_field(TYPE, MEMBER) sizeof(((TYPE*)NULL)->MEMBER)

// TCG function call adjustments.
#define tcg_constant_i32    tcg_const_i32
#define tcg_constant_i64    tcg_const_i64
#if HOST_LONG_BITS == 32
#define tcg_constant_ptr(x) tcg_const_ptr((int32_t)x)
#else
#define tcg_constant_ptr(x) tcg_const_ptr((int64_t)x)
#endif
#define tcg_constant_tl     tcg_const_tl

#define ARM_GETPC()         ((uintptr_t)GETPC())

// DISAS_NEXT and DISAS_JUMP (and some unused 'DISAS_') are defined in the common 'exec-all.h'.
#define DISAS_NORETURN  4
#define DISAS_TOO_MANY  5

#define DISAS_TARGET_1  11
#define DISAS_TARGET_2  12
#define DISAS_TARGET_3  13
#define DISAS_TARGET_4  14
#define DISAS_TARGET_5  15
#define DISAS_TARGET_6  16
#define DISAS_TARGET_7  17
#define DISAS_TARGET_8  18
#define DISAS_TARGET_9  19
#define DISAS_TARGET_10 20

// This is the same as our 'EXCP_WFI'.
#define EXCP_HLT        EXCP_WFI

// Ignore YIELD exception.
#define EXCP_NONE       -1
#define EXCP_YIELD      EXCP_NONE

// Double-check `ldgm` and `stgm` MTE helpers before changing this value.
// Both these helpers contained static asserts to make sure it's 6.
#define GMID_EL1_BS     6

// Adjust Memory Operation names.
#define MO_BEUQ         MO_BEQ
#define MO_LEUQ         MO_LEQ
#define MO_UQ           MO_Q

// TODO: 'tcg_gen_mb' and these constants need to be implemented if parallel
//       execution gets added in the future. 'TCG_MO_*' and 'TCG_BAR_*'
//       constants are only passed to 'tcg_gen_mb'.
#define TCG_BAR_LDAQ 0
#define TCG_BAR_SC   0
#define TCG_BAR_STRL 0
#define TCG_MO_ALL   0
#define TCG_MO_LD_LD 0
#define TCG_MO_LD_ST 0
#define TCG_MO_ST_ST 0

enum fprounding {
    FPROUNDING_TIEEVEN,
    FPROUNDING_POSINF,
    FPROUNDING_NEGINF,
    FPROUNDING_ZERO,
    FPROUNDING_TIEAWAY,
    FPROUNDING_ODD
};

// Combines MemOp with MMU index.
typedef int MemOpIdx;

// Defined in 'include/hw/core/cpu.h' (GPL); recreated based on usage.
typedef enum MMUAccessType {
    MMU_DATA_STORE,
    MMU_DATA_LOAD,
} MMUAccessType;

// Mnemonics that can be used in MRS and MSR instructions.
enum {
    SPSR_ABT,
    SPSR_EL1,  // SPSR_SVC in AArch32
    SPSR_EL12,
    SPSR_EL2,  // SPSR_HYP in AArch32
    SPSR_EL3,
    SPSR_FIQ,
    SPSR_IRQ,
    SPSR_UND,
};

// Provide missing prototypes.
int arm_rmode_to_sf(int rmode);
uint64_t crc32(uint64_t crc, const uint8_t *data, uint32_t n_bytes);
int exception_target_el(CPUARMState *env);
uint64_t mte_check(CPUARMState *env, uint32_t desc, uint64_t ptr, uintptr_t ra);
bool mte_probe(CPUARMState *env, uint32_t desc, uint64_t ptr);
void raise_exception(CPUARMState *env, uint32_t excp, uint32_t syndrome, uint32_t target_el);
void raise_exception_ra(CPUARMState *env, uint32_t excp, uint32_t syndrome, uint32_t target_el, uintptr_t ra);

void cpu_init_v8(CPUState *env, uint32_t id);
void cpu_reset_state(CPUState *env);
void cpu_reset_v8_a64(CPUState *env);
void cpu_reset_vfp(CPUState *env);
void do_interrupt_v8a(CPUState *env);
int get_phys_addr(CPUState *env, target_ulong address, int access_type, int mmu_idx, target_ulong *phys_ptr, int *prot,
                  target_ulong *page_size, int no_page_fault);
int get_phys_addr_v8(CPUState *env, target_ulong address, int access_type, int mmu_idx, target_ulong *phys_ptr, int *prot,
                     target_ulong *page_size, bool suppress_faults, bool at_instruction_or_cache_maintenance);
int process_interrupt_v8a(int interrupt_request, CPUState *env);

// TODO: Implement this properly. It's much more complicated for SPSR_EL1 and SPSR_EL2. See:
// https://developer.arm.com/documentation/ddi0601/2022-09/AArch64-Registers/SPSR-EL1--Saved-Program-Status-Register--EL1-
static inline unsigned int aarch64_banked_spsr_index(int el)
{
    switch (el) {
    case 1:
        return SPSR_EL1;
    case 2:
        return SPSR_EL2;
    case 3:
        return SPSR_EL3;
    default:
        tlib_abortf("aarch64_banked_spsr_index: Invalid el: %d", el);
        __builtin_unreachable();
    }
}

static inline int get_sp_el_idx(CPUState *env)
{
    // EL0's SP is used if PSTATE_SP (SPSel in AArch64) isn't set.
    return env->pstate & PSTATE_SP ? arm_current_el(env) : 0;
}

static inline void aarch64_save_sp(CPUState *env)
{
    int sp_el_idx = get_sp_el_idx(env);
    env->sp_el[sp_el_idx] = env->xregs[31];
}

static inline void aarch64_restore_sp(CPUState *env)
{
    int sp_el_idx = get_sp_el_idx(env);
    env->xregs[31] = env->sp_el[sp_el_idx];
}

static inline void arm_clear_exclusive(CPUState *env)
{
    // Based on 'gen_clrex' and 'gen_store_exclusive' it seems -1 means the address isn't valid.
    env->exclusive_addr = -1;

    env->exclusive_high = 0;
    env->exclusive_val = 0;
}

// TODO: Calculate effective values for all bits.
// The returned value is currently valid for all the bits used in tlib:
//       HCR_TGE, HCR_TWE, HCR_TWI, HCR_E2H, HCR_TSC, HCR_AMO,
//       HCR_VSE, HCR_TID0, HCR_TID3, HCR_API, HCR_E2H
static inline uint64_t arm_hcr_el2_eff(CPUARMState *env)
{
    uint64_t hcr = env->cp15.hcr_el2;
    uint64_t effective_hcr = hcr;

    // TODO: Really check if FEAT_VHE is implemented.
    bool feat_vhe = true;
    bool el2_enabled = arm_is_el2_enabled(env);

    bool tge = hcr & HCR_TGE;
    bool e2h = hcr & HCR_E2H;

    if (tge) {
        effective_hcr &= ~HCR_FB;
        effective_hcr &= ~HCR_TSC;

        if (el2_enabled) {
            effective_hcr &= ~HCR_TID3;
        }
    }

    if (feat_vhe && tge && e2h) {
        effective_hcr &= ~HCR_TWI;
        effective_hcr &= ~HCR_TWE;

        if (el2_enabled) {
            effective_hcr &= ~(HCR_AMO | HCR_FMO | HCR_IMO);
        }

        effective_hcr &= ~HCR_TID0;
        effective_hcr |= HCR_RW;
    } else {
        if (el2_enabled) {
            effective_hcr |= (HCR_AMO | HCR_FMO | HCR_IMO);
        }
    }
    bool amo = effective_hcr & HCR_AMO;

    if (tge || !amo) {
        // TODO: Should VSE bit be set in the 'else' case? The VSE description
        //       isn't super precise in this matter: "enabled only when the
        //       value of HCR_EL2.{TGE, AMO} is {0, 1}.".
        effective_hcr &= ~HCR_VSE;
    }

    return effective_hcr;
}

static inline int arm_mmu_idx_to_el(ARMMMUIdx arm_mmu_idx)
{
    // TODO: M-Profile.
    tlib_assert(arm_mmu_idx & ARM_MMU_IDX_A);

    switch (arm_mmu_idx & ~ARM_MMU_IDX_A_NS) {
    case ARMMMUIdx_SE3:
        return 3;
    case ARMMMUIdx_SE2:
    case ARMMMUIdx_SE20_2:
    case ARMMMUIdx_SE20_2_PAN:
        return 2;
    case ARMMMUIdx_SE10_1:
    case ARMMMUIdx_SE10_1_PAN:
        return 1;
    case ARMMMUIdx_SE10_0:
    case ARMMMUIdx_SE20_0:
        return 0;
    default:
        tlib_abortf("Unsupported arm_mmu_idx: %d", arm_mmu_idx);
        __builtin_unreachable();
    }
}

static inline uint32_t arm_to_core_mmu_idx(ARMMMUIdx arm_mmu_idx)
{
    return arm_mmu_idx & ARM_MMU_IDX_COREIDX_MASK;
}

static inline ARMMMUIdx core_to_aa64_mmu_idx(int core_mmu_idx)
{
    return core_mmu_idx | ARM_MMU_IDX_A;
}

static inline void cpu_get_tb_cpu_state(CPUState *env, target_ulong *pc, target_ulong *cs_base, int *flags)
{
    if (unlikely(!env->aarch64)) {
        tlib_abort("CPU not in AArch64 state; AArch32 unimplemented.");
    }
    *pc = CPU_PC(env);

    // See 'arm_tbflags_from_tb' in 'translate.h'.
    *flags = (int)env->hflags.flags;
    *cs_base = env->hflags.flags2;
}

static inline bool cpu_has_work(CPUState *env)
{
    // clear WFI if waking up condition is met
    env->wfi &= !(is_interrupt_pending(env, CPU_INTERRUPT_HARD));
    return !env->wfi;
}

static inline void cpu_pc_from_tb(CPUState *env, TranslationBlock *tb)
{
    env->pc = tb->pc;
}

static inline bool el2_and_hcr_el2_e2h_set(CPUState *env)
{
    return arm_current_el(env) == 2 && (arm_hcr_el2_eff(env) & HCR_E2H);
}

static inline ARMCoreConfig *env_archcpu(CPUState *env)
{
    return env->arm_core_config;
}

static inline bool excp_is_internal(uint32_t excp)
{
    switch (excp) {
    case EXCP_EXCEPTION_EXIT:
    case EXCP_SEMIHOST:
        return true;
    default:
        return excp >= 0x10000;  // All the 0x1000X exceptions are internal.
    }
}

// The position of the current instruction in the translation block (first is 1).
static inline int get_dcbase_num_insns(DisasContextBase base)
{
    return base.tb->icount;
}

static inline bool hcr_e2h_and_tge_set(CPUState *env)
{
    uint64_t hcr_el2_eff = arm_hcr_el2_eff(env);
    uint64_t hcr_e2h_tge = HCR_E2H | HCR_TGE;
    return (hcr_el2_eff & hcr_e2h_tge) == hcr_e2h_tge;
}

static inline void log_unhandled_sysreg_access(const char *sysreg_name, bool is_write)
{
    // The function is used for system instructions too.
    tlib_printf(LOG_LEVEL_WARNING, "Unhandled system instruction or register %s %s", is_write ? "write:" : "read: ", sysreg_name);
}

static inline void log_unhandled_sysreg_read(const char *sysreg_name)
{
    log_unhandled_sysreg_access(sysreg_name, false);
}

static inline void log_unhandled_sysreg_write(const char *sysreg_name)
{
    log_unhandled_sysreg_access(sysreg_name, true);
}

static inline void pstate_set_el(CPUARMState *env, uint32_t el)
{
    // The function is only valid for AArch64.
    tlib_assert(is_a64(env));
    tlib_assert(el < 4);

    env->pstate = deposit32(env->pstate, 2, 2, el);

    // Update cached MMUIdx.
    arm_rebuild_hflags(env);
}

static inline void pstate_write_with_sp_change(CPUARMState *env, uint32_t val)
{
    bool modes_differ = (env->pstate & PSTATE_M) != (val & PSTATE_M);
    if (modes_differ) {
        aarch64_save_sp(env);
    }

    pstate_write(env, val);

    if (modes_differ) {
        aarch64_restore_sp(env);

        // Mostly to update cached MMUIdx.
        arm_rebuild_hflags(env);
    }
}

static inline void pstate_write_masked(CPUARMState *env, uint32_t value, uint32_t mask)
{
    uint32_t new_pstate = (pstate_read(env) & ~mask) | (value & mask);
    pstate_write_with_sp_change(env, new_pstate);
}

static inline bool regime_has_2_ranges(ARMMMUIdx idx)
{
    // This might be incorrect since it's only based on the names.
    switch (idx) {
    case ARMMMUIdx_E10_0:
    case ARMMMUIdx_E20_0:
    case ARMMMUIdx_E10_1:
    case ARMMMUIdx_E20_2:
    case ARMMMUIdx_E10_1_PAN:
    case ARMMMUIdx_E20_2_PAN:
    case ARMMMUIdx_SE10_0:
    case ARMMMUIdx_SE20_0:
    case ARMMMUIdx_SE10_1:
    case ARMMMUIdx_SE20_2:
    case ARMMMUIdx_SE10_1_PAN:
    case ARMMMUIdx_SE20_2_PAN:
        return true;
    default:
        return false;
    }
}

static inline uint32_t tb_cflags(TranslationBlock *tb)
{
    return tb->cflags;
}

// TODO: Port 'tcg_gen_lookup_and_goto_ptr' for more efficient jumps?
//       The upstream function has no arguments.
static inline void tcg_gen_lookup_and_goto_ptr(DisasContext *dc)
{
    gen_exit_tb_no_chaining(dc->base.tb);
}

static inline ARMMMUIdx el_to_arm_mmu_idx(CPUState *env, int el)
{
    ARMMMUIdx idx;
    switch (el) {
    case 0:
        if (hcr_e2h_and_tge_set(env)) {
            idx = ARMMMUIdx_SE20_0;
        } else {
            idx = ARMMMUIdx_SE10_0;
        }
        break;
    case 1:
        idx = pstate_read(env) & PSTATE_PAN ? ARMMMUIdx_SE10_1_PAN : ARMMMUIdx_SE10_1;
        break;
    case 2:
        if (arm_hcr_el2_eff(env) & HCR_E2H) {
            idx = ARMMMUIdx_SE2;
        } else {
            idx = pstate_read(env) & PSTATE_PAN ? ARMMMUIdx_E20_2_PAN : ARMMMUIdx_SE20_2;
        }
        break;
    case 3:
        idx = ARMMMUIdx_SE3;
        break;
    default:
        tlib_assert_not_reached();
    }

    if (!arm_is_secure(env)) {
        idx |= ARM_MMU_IDX_A_NS;
    }
    return idx;
}
