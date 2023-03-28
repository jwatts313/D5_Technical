/*
 * Copyright (c) Antmicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SYSTEM_REGISTERS_H_
#define SYSTEM_REGISTERS_H_

#include <stdint.h>
#include "cpu.h"

// Types of ARMCPRegInfo.
// Each bit is a different type.
#define ARM_CP_NOP             (1 << 0)
#define ARM_CP_CURRENTEL       (1 << 1)
// Special regs
#define ARM_CP_NZCV            (1 << 2)
#define ARM_CP_DC_ZVA          (1 << 3)
#define ARM_CP_DC_GVA          (1 << 4)
#define ARM_CP_DC_GZVA         (1 << 5)
#define ARM_CP_WFI             (1 << 6)
// Mask used on above type
#define ARM_CP_SPECIAL_MASK    0x000F

#define ARM_CP_64BIT           (1 << 8)
#define ARM_CP_CONST           (1 << 9)
#define ARM_CP_FPU             (1 << 10)
#define ARM_CP_IO              (1 << 11)
#define ARM_CP_RAISES_EXC      (1 << 12)
#define ARM_CP_RO              (1 << 13)  // Read-only
#define ARM_CP_SME             (1 << 14)
#define ARM_CP_SUPPRESS_TB_END (1 << 15)
#define ARM_CP_SVE             (1 << 16)
#define ARM_CP_WO              (1 << 17)  // Write-Only

// Minimum EL access
#define ARM_CP_EL_SHIFT        20
#define ARM_CP_EL_MASK         (3 << ARM_CP_EL_SHIFT)
#define ARM_CP_EL_0            (0 << ARM_CP_EL_SHIFT)
#define ARM_CP_EL_1            (1 << ARM_CP_EL_SHIFT)
#define ARM_CP_EL_2            (2 << ARM_CP_EL_SHIFT)
#define ARM_CP_EL_3            (3 << ARM_CP_EL_SHIFT)

#define ARM_CP_READABLE(ri_type)   (!(ri_type & ARM_CP_WO))
#define ARM_CP_WRITABLE(ri_type)   (!(ri_type & ARM_CP_RO))
#define ARM_CP_GET_MIN_EL(ri_type) ((ri_type & ARM_CP_EL_MASK) >> ARM_CP_EL_SHIFT)

// This doesn't seem to be of any real use. It's passed to ENCODE_AA64_CP_REG in 'translate-a64.c:handle_sys'
// as 'cp' so it seems to be a coprocessor ID. However, there's no information about coprocessor for AArch64
// registers and instructions in the manual. There's only an information that some instruction type encodings
// are "equivalent to the registers in the AArch32 (coproc == XX) encoding space" (XX=15 for op0=(1 or 3) and
// XX=14 for op0=2). Perhaps let's use 16 to distinguish it from CP15 used for AArch32 and ARMv7 encodings.
#define CP_REG_ARM64_SYSREG_CP        16

// From C5.1.2
#define CP_REG_ARM64_SYSREG_OP2_SHIFT 0
#define CP_REG_ARM64_SYSREG_CRM_SHIFT (CP_REG_ARM64_SYSREG_OP2_SHIFT + 3)
#define CP_REG_ARM64_SYSREG_CRN_SHIFT (CP_REG_ARM64_SYSREG_CRM_SHIFT + 4)
#define CP_REG_ARM64_SYSREG_OP1_SHIFT (CP_REG_ARM64_SYSREG_CRN_SHIFT + 4)
#define CP_REG_ARM64_SYSREG_OP0_SHIFT (CP_REG_ARM64_SYSREG_OP1_SHIFT + 3)
// op0 is a 2-bit field
#define CP_REG_ARM_COPROC_SHIFT       (CP_REG_ARM64_SYSREG_OP0_SHIFT + 2)

// Always pass an actual array directly; otherwise 'sizeof(array)' will be a pointer size.
#define ARM_CP_ARRAY_COUNT(array) (sizeof(array) / sizeof(ARMCPRegInfo))

typedef enum {
    CP_ACCESS_EL0,
    CP_ACCESS_EL1,
    CP_ACCESS_EL2,
    CP_ACCESS_EL3,
    CP_ACCESS_OK                 = 0x10,
    CP_ACCESS_TRAP_EL2           = 0x20,
    CP_ACCESS_TRAP_UNCATEGORIZED = 0x30,
    CP_ACCESS_TRAP               = 0x40,
} CPAccessResult;
#define CP_ACCESS_EL_MASK 3

typedef struct ARMCPRegInfo ARMCPRegInfo;
typedef CPAccessResult AccessFn(CPUState *, const ARMCPRegInfo *, bool isread);
typedef uint64_t ReadFn(CPUState *, const ARMCPRegInfo *);
typedef void WriteFn(CPUState *, const ARMCPRegInfo *, uint64_t);

struct ARMCPRegInfo
{
    const char *name;
    // Register coprocessor, in AArch64 always CP_REG_ARM64_SYSREG_CP
    uint32_t cp;
    // type of register, if require special handling
    uint32_t type;

    // from C5.1.2, only 2 lower bits used
    uint8_t op0;
    // from C5.1.1, only 3 lower bits used
    uint8_t op1;
    // from C5.1.3, only 4 lower bits used
    uint8_t crn;
    // from C5.1.3, only 4 lower bits used
    uint8_t crm;
    // from C5.1.3, only 4 lower bits used
    uint8_t op2;
    // offset from CPUState struct when there is no readfn/writefn
    uint32_t fieldoffset;
    // resetvalue of the register
    uint64_t resetvalue;
    // fuction that checks if access to the register should be granted
    AccessFn *accessfn;
    // read function (required when fieldoffset and type is missing)
    ReadFn *readfn;
    // write function (required when fieldoffset and type is missing)
    WriteFn *writefn;
};

// Only EL and RO/WO are checked here. Traps etc. are checked in the 'access_check_cp_reg' helper.
static inline bool cp_access_ok(int current_el, const ARMCPRegInfo *reg_info, bool isread)
{
    uint32_t ri_type = reg_info->type;

    if (current_el < ARM_CP_GET_MIN_EL(ri_type)) {
        tlib_printf(LOG_LEVEL_ERROR, "The '%s' register shouldn't be accessed on EL%d", reg_info->name, current_el);
        return false;
    }

    // Rule IWCXDT
    if ((isread && !ARM_CP_READABLE(ri_type)) || (!isread && !ARM_CP_WRITABLE(ri_type))) {
        tlib_printf(LOG_LEVEL_ERROR, "The '%s' register shouldn't be %s", reg_info->name, isread ? "read from" : "written to");
        return false;
    }
    return true;
}

void system_instructions_and_registers_reset(CPUState *env);
void system_instructions_and_registers_init(CPUState *env, uint32_t cpu_model_id);

/* Functions for accessing system registers by their names. */

static inline uint64_t *sysreg_field_ptr(CPUState *env, const ARMCPRegInfo *ri)
{
    // Fieldoffset is in bytes hence 'env' is cast to 'uint8_t' before the addition.
    return (uint64_t *)(((uint8_t *)env) + ri->fieldoffset);
}

static bool ttable_compare_sysreg_name(TTable_entry entry, const void *sysreg_name)
{
    ARMCPRegInfo *ri = (ARMCPRegInfo *)entry.value;
    return strcasecmp(ri->name, sysreg_name) == 0;
}

static const ARMCPRegInfo *sysreg_find_by_name(CPUState *env, const char *name)
{
    const char *lookup_name = name;
    if (strcasecmp(lookup_name, "DBGDTRRX_EL0") == 0 || strcasecmp(lookup_name, "DBGDTRTX_EL0") == 0) {
        lookup_name = "DBGDTR_RX_TX_EL0";
    }

    TTable_entry *entry = ttable_lookup_custom(env->arm_core_config->cp_regs, ttable_compare_sysreg_name, lookup_name);
    if (entry != NULL) {
        return (ARMCPRegInfo *)entry->value;
    }
    return NULL;
}

static void sysreg_access_nzcv()
{
    // For now let's just inform that it can be handled through PSTATE.
    tlib_printf(LOG_LEVEL_INFO, "Use '<cpu_name> PSTATE' to access NZCV.");
}

static inline uint64_t sysreg_get_by_name(CPUState *env, const char *name)
{
    const ARMCPRegInfo *ri = sysreg_find_by_name(env, name);
    if (ri == NULL) {
        tlib_printf(LOG_LEVEL_WARNING, "Reading from system register failure. No such register: %s", name);
        return 0x0;
    }

    if (ri->type & ARM_CP_NZCV) {
        sysreg_access_nzcv();
        return 0x0;
    }

    if (ri->readfn) {
        return ri->readfn(env, ri);
    } else if (ri->fieldoffset != 0) {
        if (ri->type & ARM_CP_64BIT) {
            return *sysreg_field_ptr(env, ri);
        } else {
            return *(uint32_t *)sysreg_field_ptr(env, ri);
        }
    } else {
        log_unhandled_sysreg_read(ri->name);
        return 0x0;
    }
}

static inline void sysreg_set_by_name(CPUState *env, const char *name, uint64_t value)
{
    const ARMCPRegInfo *ri = sysreg_find_by_name(env, name);
    if (ri == NULL) {
        tlib_printf(LOG_LEVEL_WARNING, "Writing to system register failure. No such register: %s", name);
        return;
    }

    if (ri->type & ARM_CP_NZCV) {
        sysreg_access_nzcv();
    }

    if (ri->writefn) {
        ri->writefn(env, ri, value);
    } else if (ri->fieldoffset != 0) {
        if (ri->type & ARM_CP_64BIT) {
            *sysreg_field_ptr(env, ri) = value;
        } else {
            *(uint32_t *)sysreg_field_ptr(env, ri) = (uint32_t)value;
        }
    } else {
        log_unhandled_sysreg_write(ri->name);
        return;
    }
}

#endif // SYSTEM_REGISTERS_H_
