/*
 * Copyright (c) Antmicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "arch_callbacks.h"
#include "cpu.h"
#include "cpu_names.h"
#include "system_registers.h"
#include "ttable.h"

/* This file is named 'system_registers.h' but it also contains definitions of the system instructions.
 *
 * Beware the 'register name' vs 'instruction mnemonic' ambiguity because even ARMCPRegInfo.name is in fact an instruction mnemonic not a register name.
 *
 * For example, an 'MRS ELR_EL1' instruction is a read with a ELR_EL1 mnemonic but it doesn't always read the ELR_EL1 register. In certain situations, i.e., if EL == 2 and HCR_EL2.E2H is set, it should return value of the ELR_EL2 register.
 *
 * Basically all the mnemonics used in MRS/MSR (AArch64), MRC/MCR (AArch32), AT, DC, IC, TLBI etc. instructions should have their entry in 'cp_regs'.
 */

/* Helpers for mnemonics with a complex mnemonic->register translation. */

static inline uint64_t *cpacr_el1_register_pointer(CPUState *env)
{
    return el2_and_hcr_el2_e2h_set(env) ? &env->cp15.cptr_el[2] : &env->cp15.cpacr_el1;
}

static inline uint64_t *mpidr_el1_register_pointer(CPUState *env)
{
    if (arm_current_el(env) == 1 && arm_is_el2_enabled(env)) {
        return &env->cp15.vmpidr_el2;
    }
    return &env->arm_core_config->mpidr;
}

static inline uint64_t *spsr_el1_register_pointer(CPUState *env)
{
    uint32_t spsr_idx = el2_and_hcr_el2_e2h_set(env) ? SPSR_EL2 : SPSR_EL1;
    return &env->banked_spsr[spsr_idx];
}

/* Other helpers */

static inline uint64_t get_id_aa64pfr0_value(CPUState *env)
{
    uint64_t return_value = env->arm_core_config->isar.id_aa64pfr0;

    if (!env->arm_core_config->has_el3) {
        return_value = FIELD_DP64(return_value, ID_AA64PFR0, EL3, 0);
    }

    if (!env->arm_core_config->has_el2) {
        return_value = FIELD_DP64(return_value, ID_AA64PFR0, EL2, 0);
    }

    return return_value;
}

/* Read/write functions. */

#define READ_FUNCTION(width, mnemonic, value)                                      \
    uint ## width ## _t read_ ## mnemonic(CPUState *env, const ARMCPRegInfo *info) \
    {                                                                              \
        return value;                                                              \
    }

#define WRITE_FUNCTION(width, mnemonic, write_statement)                                        \
    void write_ ## mnemonic(CPUState *env, const ARMCPRegInfo *info, uint ## width ## _t value) \
    {                                                                                           \
        write_statement;                                                                        \
    }

#define RW_FUNCTIONS(width, mnemonic, read_value, write_statement) \
    READ_FUNCTION(width, mnemonic, read_value)                     \
    WRITE_FUNCTION(width, mnemonic, write_statement)

#define RW_FUNCTIONS_PTR(width, mnemonic, pointer) \
    RW_FUNCTIONS(width, mnemonic, *pointer, *pointer = value)

// Many 'MRS/MSR *_EL1' instructions access '*_EL2' registers if EL is 2 and HCR_EL2's E2H bit is set.
#define RW_FUNCTIONS_EL1_ACCESSING_EL2_IF_E2H_SET(width, mnemonic, field_base) \
    RW_FUNCTIONS_PTR(width, mnemonic, &field_base[el2_and_hcr_el2_e2h_set(env) ? 2 : 1])

static inline uint32_t encode_system_register_id(const ARMCPRegInfo *info)
{
    return (info->op0 << CP_REG_ARM64_SYSREG_OP0_SHIFT) |
        (info->op1 << CP_REG_ARM64_SYSREG_OP1_SHIFT) |
        (info->crn << CP_REG_ARM64_SYSREG_CRN_SHIFT) |
        (info->crm << CP_REG_ARM64_SYSREG_CRM_SHIFT) |
        (info->op2 << CP_REG_ARM64_SYSREG_OP2_SHIFT);
}

READ_FUNCTION(64, mpidr_el1, *mpidr_el1_register_pointer(env))

RW_FUNCTIONS(64, fpcr,  vfp_get_fpcr(env), vfp_set_fpcr(env, value))
RW_FUNCTIONS(64, fpsr,  vfp_get_fpsr(env), vfp_set_fpsr(env, value))

RW_FUNCTIONS(64, generic_timer,
             tlib_read_system_register_generic_timer(encode_system_register_id(info)),
             tlib_write_system_register_generic_timer(encode_system_register_id(info), value))

RW_FUNCTIONS(64, interrupt_cpu_interface,
             tlib_read_system_register_interrupt_cpu_interface(encode_system_register_id(info)),
             tlib_write_system_register_interrupt_cpu_interface(encode_system_register_id(info), value))

RW_FUNCTIONS_PTR(64, cpacr_el1,     cpacr_el1_register_pointer(env))
RW_FUNCTIONS_PTR(64, spsr_el1,      spsr_el1_register_pointer(env))

// TODO: For all of them their EL12 mnemonic should be undefined unless E2H is set.
RW_FUNCTIONS_EL1_ACCESSING_EL2_IF_E2H_SET(64, contextidr_el1,   env->cp15.contextidr_el)
RW_FUNCTIONS_EL1_ACCESSING_EL2_IF_E2H_SET(64, elr_el1,          env->elr_el)
RW_FUNCTIONS_EL1_ACCESSING_EL2_IF_E2H_SET(64, esr_el1,          env->cp15.esr_el)
RW_FUNCTIONS_EL1_ACCESSING_EL2_IF_E2H_SET(64, far_el1,          env->cp15.far_el)
RW_FUNCTIONS_EL1_ACCESSING_EL2_IF_E2H_SET(64, mair_el1,         env->cp15.mair_el)
RW_FUNCTIONS_EL1_ACCESSING_EL2_IF_E2H_SET(64, sctlr_el1,        env->cp15.sctlr_el)
RW_FUNCTIONS_EL1_ACCESSING_EL2_IF_E2H_SET(64, scxtnum_el1,      env->scxtnum_el)
RW_FUNCTIONS_EL1_ACCESSING_EL2_IF_E2H_SET(64, tcr_el1,          env->cp15.tcr_el)
RW_FUNCTIONS_EL1_ACCESSING_EL2_IF_E2H_SET(64, tfsr_el1,         env->cp15.tfsr_el)
RW_FUNCTIONS_EL1_ACCESSING_EL2_IF_E2H_SET(64, ttbr0_el1,        env->cp15.ttbr0_el)
RW_FUNCTIONS_EL1_ACCESSING_EL2_IF_E2H_SET(64, ttbr1_el1,        env->cp15.ttbr1_el)
RW_FUNCTIONS_EL1_ACCESSING_EL2_IF_E2H_SET(64, vbar_el1,         env->cp15.vbar_el)
RW_FUNCTIONS_EL1_ACCESSING_EL2_IF_E2H_SET(64, zcr_el1,          env->vfp.zcr_el)

/* PSTATE accessors */

#define RW_PSTATE_FUNCTIONS(mnemonic, pstate_field)         \
    RW_FUNCTIONS(64, mnemonic, pstate_read(env) & pstate_field, \
        pstate_write_masked(env, value, pstate_field))

RW_PSTATE_FUNCTIONS(allint, PSTATE_ALLINT)
RW_PSTATE_FUNCTIONS(dit,    PSTATE_DIT)
RW_PSTATE_FUNCTIONS(pan,    PSTATE_PAN)
RW_PSTATE_FUNCTIONS(spsel,  PSTATE_SP)
RW_PSTATE_FUNCTIONS(ssbs,   PSTATE_SSBS)
RW_PSTATE_FUNCTIONS(tco,    PSTATE_TCO)
RW_PSTATE_FUNCTIONS(uao,    PSTATE_UAO)

/* 'arm_core_config'-reading functions */

#define READ_CONFIG(name, config_field_name) \
    READ_FUNCTION(64, name, env->arm_core_config->config_field_name)

READ_CONFIG(ccsidr_el1,        ccsidr[env->cp15.csselr_el[1]])
READ_CONFIG(ccsidr2_el1,       ccsidr[env->cp15.csselr_el[1]]   >>  32)
READ_CONFIG(clidr_el1,         clidr)
READ_CONFIG(ctr_el0,           ctr)
READ_CONFIG(dczid,             dcz_blocksize)
READ_CONFIG(id_aa64afr0_el1,   id_aa64afr0)
READ_CONFIG(id_aa64afr1_el1,   id_aa64afr1)
READ_CONFIG(id_aa64dfr0_el1,   isar.id_aa64dfr0)
READ_CONFIG(id_aa64isar0_el1,  isar.id_aa64isar0)
READ_CONFIG(id_aa64isar1_el1,  isar.id_aa64isar1)
READ_CONFIG(id_aa64mmfr0_el1,  isar.id_aa64mmfr0)
READ_CONFIG(id_aa64mmfr1_el1,  isar.id_aa64mmfr1)
READ_CONFIG(id_aa64mmfr2_el1,  isar.id_aa64mmfr2)
READ_FUNCTION(64, id_aa64pfr0_el1, get_id_aa64pfr0_value(env))
READ_CONFIG(id_aa64pfr1_el1,   isar.id_aa64pfr1)
READ_CONFIG(id_aa64smfr0_el1,  isar.id_aa64smfr0)
READ_CONFIG(id_aa64zfr0_el1,   isar.id_aa64zfr0)
READ_CONFIG(id_afr0,           id_afr0)
READ_CONFIG(id_dfr0,           isar.id_dfr0)
READ_CONFIG(id_dfr1,           isar.id_dfr1)
READ_CONFIG(id_isar0,          isar.id_isar0)
READ_CONFIG(id_isar1,          isar.id_isar1)
READ_CONFIG(id_isar2,          isar.id_isar2)
READ_CONFIG(id_isar3,          isar.id_isar3)
READ_CONFIG(id_isar4,          isar.id_isar4)
READ_CONFIG(id_isar5,          isar.id_isar5)
READ_CONFIG(id_isar6,          isar.id_isar6)
READ_CONFIG(id_mmfr0,          isar.id_mmfr0)
READ_CONFIG(id_mmfr1,          isar.id_mmfr1)
READ_CONFIG(id_mmfr2,          isar.id_mmfr2)
READ_CONFIG(id_mmfr3,          isar.id_mmfr3)
READ_CONFIG(id_mmfr4,          isar.id_mmfr4)
READ_CONFIG(id_mmfr5,          isar.id_mmfr5)
READ_CONFIG(id_pfr0,           isar.id_pfr0)
READ_CONFIG(id_pfr1,           isar.id_pfr1)
READ_CONFIG(id_pfr2,           isar.id_pfr2)
READ_CONFIG(midr,              midr)
READ_CONFIG(mvfr0_el1,         isar.mvfr0)
READ_CONFIG(mvfr1_el1,         isar.mvfr1)
READ_CONFIG(mvfr2_el1,         isar.mvfr2)
READ_CONFIG(revidr_el1,        revidr)

/* Macros creating ARMCPRegInfo entries. */

// The parameters have to start with an underscore because preprocessing replaces '.PARAMETER' too.
// The 'extra_type' parameter is any type besides 'ARM_CP_64BIT' and 'ARM_CP_EL*' since those are set automatically.
#define ARM_CP_REG_DEFINE(_name, _cp, _op0, _op1, _crn, _crm, _op2, width, el, extra_type, ...) \
    {                                                           \
        .name        = #_name,                                  \
        .cp          = _cp,                                     \
        .op0         = _op0,                                    \
        .op1         = _op1,                                    \
        .crn         = _crn,                                    \
        .crm         = _crm,                                    \
        .op2         = _op2,                                    \
        .type        = (extra_type                              \
                | (el << ARM_CP_EL_SHIFT)                       \
                | (width == 64 ? ARM_CP_64BIT : 0x0)            \
                ),                                              \
        __VA_ARGS__                                             \
    },

// All ARM64 (AArch64) registers use the same CP value. Width can always be 64 since ARM_CP_64BIT only matters for AArch32 registers.
#define ARM64_CP_REG_DEFINE(name, op0, op1, crn, crm, op2, el, extra_type, ...) \
    ARM_CP_REG_DEFINE(name, CP_REG_ARM64_SYSREG_CP, op0, op1, crn, crm, op2, 64, el, extra_type, __VA_ARGS__)

/* Macros for the most common types used in 'extra_type'.
 *
 * Reading/writing the register specified as WO/RO (respectively) will trigger the 'Undefined instruction' exception.
 * Therefore CONST can be used with RO if the instruction to write the given register doesn't exist.
 * Writes to a CONST register are simply ignored unless RO is used too.
 * 
 * CONST has to be used as the last one in 'extra_type', e.g., 'RW | CONST(0xCAFE)'.
 * IGNORED silences the unhandled warning.
 */

#define CONST(resetvalue)  ARM_CP_CONST, RESETVALUE(resetvalue)
#define IGNORED            ARM_CP_NOP
#define RO                 ARM_CP_RO
#define RW                 0x0
#define WO                 ARM_CP_WO

// Optional macro arguments.
#define ACCESSFN(name)         .accessfn = access_ ## name
#define FIELD(cpu_state_field) .fieldoffset = offsetof(CPUState, cpu_state_field)
#define READFN(name)           .readfn = read_ ## name
#define RESETVALUE(value)      .resetvalue = value
#define RW_FNS(name)           READFN(name), WRITEFN(name)
#define WRITEFN(name)          .writefn = write_ ## name

ARMCPRegInfo aarch64_registers[] = {
    // The params are:   name                   op0, op1, crn, crm, op2, el, extra_type, ...
    ARM64_CP_REG_DEFINE(CurrentEL,               3,   0,   4,   2,   2,  0, ARM_CP_CURRENTEL)
    ARM64_CP_REG_DEFINE(ACCDATA_EL1,             3,   0,  11,   0,   5,  1, RW)
    ARM64_CP_REG_DEFINE(ACTLR_EL1,               3,   0,   1,   0,   1,  1, RW)
    ARM64_CP_REG_DEFINE(ACTLR_EL2,               3,   4,   1,   0,   1,  2, RW)
    ARM64_CP_REG_DEFINE(ACTLR_EL3,               3,   6,   1,   0,   1,  3, RW)
    ARM64_CP_REG_DEFINE(AFSR0_EL1,               3,   0,   5,   1,   0,  1, RW)
    ARM64_CP_REG_DEFINE(AFSR0_EL12,              3,   5,   5,   1,   0,  2, RW)
    ARM64_CP_REG_DEFINE(AFSR0_EL2,               3,   4,   5,   1,   0,  2, RW)
    ARM64_CP_REG_DEFINE(AFSR0_EL3,               3,   6,   5,   1,   0,  3, RW)
    ARM64_CP_REG_DEFINE(AFSR1_EL1,               3,   0,   5,   1,   1,  1, RW)
    ARM64_CP_REG_DEFINE(AFSR1_EL12,              3,   5,   5,   1,   1,  2, RW)
    ARM64_CP_REG_DEFINE(AFSR1_EL2,               3,   4,   5,   1,   1,  2, RW)
    ARM64_CP_REG_DEFINE(AFSR1_EL3,               3,   6,   5,   1,   1,  3, RW)
    ARM64_CP_REG_DEFINE(AIDR_EL1,                3,   1,   0,   0,   7,  1, RO)
    ARM64_CP_REG_DEFINE(ALLINT,                  3,   0,   4,   3,   0,  1, RW, RW_FNS(allint))
    ARM64_CP_REG_DEFINE(AMAIR_EL1,               3,   0,  10,   3,   0,  1, RW)
    ARM64_CP_REG_DEFINE(AMAIR_EL12,              3,   5,  10,   3,   0,  2, RW)
    ARM64_CP_REG_DEFINE(AMAIR_EL2,               3,   4,  10,   3,   0,  2, RW)
    ARM64_CP_REG_DEFINE(AMAIR_EL3,               3,   6,  10,   3,   0,  3, RW)
    ARM64_CP_REG_DEFINE(AMCFGR_EL0,              3,   3,  13,   2,   1,  0, RO)
    ARM64_CP_REG_DEFINE(AMCG1IDR_EL0,            3,   3,  13,   2,   6,  0, RO)
    ARM64_CP_REG_DEFINE(AMCGCR_EL0,              3,   3,  13,   2,   2,  0, RO)
    ARM64_CP_REG_DEFINE(AMCNTENCLR0_EL0,         3,   3,  13,   2,   4,  0, RW)
    ARM64_CP_REG_DEFINE(AMCNTENCLR1_EL0,         3,   3,  13,   3,   0,  0, RW)
    ARM64_CP_REG_DEFINE(AMCNTENSET0_EL0,         3,   3,  13,   2,   5,  0, RW)
    ARM64_CP_REG_DEFINE(AMCNTENSET1_EL0,         3,   3,  13,   3,   1,  0, RW)
    ARM64_CP_REG_DEFINE(AMCR_EL0,                3,   3,  13,   2,   0,  0, RW)
    ARM64_CP_REG_DEFINE(AMEVCNTR00_EL0,          3,   3,  13,   4,   0,  0, RW)
    ARM64_CP_REG_DEFINE(AMEVCNTR01_EL0,          3,   3,  13,   4,   1,  0, RW)
    ARM64_CP_REG_DEFINE(AMEVCNTR02_EL0,          3,   3,  13,   4,   2,  0, RW)
    ARM64_CP_REG_DEFINE(AMEVCNTR03_EL0,          3,   3,  13,   4,   3,  0, RW)
    ARM64_CP_REG_DEFINE(AMEVCNTR10_EL0,          3,   3,  13,  12,   0,  0, RW)
    ARM64_CP_REG_DEFINE(AMEVCNTR11_EL0,          3,   3,  13,  12,   1,  0, RW)
    ARM64_CP_REG_DEFINE(AMEVCNTR12_EL0,          3,   3,  13,  12,   2,  0, RW)
    ARM64_CP_REG_DEFINE(AMEVCNTR13_EL0,          3,   3,  13,  12,   3,  0, RW)
    ARM64_CP_REG_DEFINE(AMEVCNTR14_EL0,          3,   3,  13,  12,   4,  0, RW)
    ARM64_CP_REG_DEFINE(AMEVCNTR15_EL0,          3,   3,  13,  12,   5,  0, RW)
    ARM64_CP_REG_DEFINE(AMEVCNTR16_EL0,          3,   3,  13,  12,   6,  0, RW)
    ARM64_CP_REG_DEFINE(AMEVCNTR17_EL0,          3,   3,  13,  12,   7,  0, RW)
    ARM64_CP_REG_DEFINE(AMEVCNTR18_EL0,          3,   3,  13,  13,   0,  0, RW)
    ARM64_CP_REG_DEFINE(AMEVCNTR19_EL0,          3,   3,  13,  13,   1,  0, RW)
    ARM64_CP_REG_DEFINE(AMEVCNTR110_EL0,         3,   3,  13,  13,   2,  0, RW)
    ARM64_CP_REG_DEFINE(AMEVCNTR111_EL0,         3,   3,  13,  13,   3,  0, RW)
    ARM64_CP_REG_DEFINE(AMEVCNTR112_EL0,         3,   3,  13,  13,   4,  0, RW)
    ARM64_CP_REG_DEFINE(AMEVCNTR113_EL0,         3,   3,  13,  13,   5,  0, RW)
    ARM64_CP_REG_DEFINE(AMEVCNTR114_EL0,         3,   3,  13,  13,   6,  0, RW)
    ARM64_CP_REG_DEFINE(AMEVCNTR115_EL0,         3,   3,  13,  13,   7,  0, RW)
    ARM64_CP_REG_DEFINE(AMEVCNTVOFF00_EL2,       3,   4,  13,   8,   0,  2, RW)
    ARM64_CP_REG_DEFINE(AMEVCNTVOFF01_EL2,       3,   4,  13,   8,   1,  2, RW)
    ARM64_CP_REG_DEFINE(AMEVCNTVOFF02_EL2,       3,   4,  13,   8,   2,  2, RW)
    ARM64_CP_REG_DEFINE(AMEVCNTVOFF03_EL2,       3,   4,  13,   8,   3,  2, RW)
    ARM64_CP_REG_DEFINE(AMEVCNTVOFF04_EL2,       3,   4,  13,   8,   4,  2, RW)
    ARM64_CP_REG_DEFINE(AMEVCNTVOFF05_EL2,       3,   4,  13,   8,   5,  2, RW)
    ARM64_CP_REG_DEFINE(AMEVCNTVOFF06_EL2,       3,   4,  13,   8,   6,  2, RW)
    ARM64_CP_REG_DEFINE(AMEVCNTVOFF07_EL2,       3,   4,  13,   8,   7,  2, RW)
    ARM64_CP_REG_DEFINE(AMEVCNTVOFF08_EL2,       3,   4,  13,   9,   0,  2, RW)
    ARM64_CP_REG_DEFINE(AMEVCNTVOFF09_EL2,       3,   4,  13,   9,   1,  2, RW)
    ARM64_CP_REG_DEFINE(AMEVCNTVOFF010_EL2,      3,   4,  13,   9,   2,  2, RW)
    ARM64_CP_REG_DEFINE(AMEVCNTVOFF011_EL2,      3,   4,  13,   9,   3,  2, RW)
    ARM64_CP_REG_DEFINE(AMEVCNTVOFF012_EL2,      3,   4,  13,   9,   4,  2, RW)
    ARM64_CP_REG_DEFINE(AMEVCNTVOFF013_EL2,      3,   4,  13,   9,   5,  2, RW)
    ARM64_CP_REG_DEFINE(AMEVCNTVOFF014_EL2,      3,   4,  13,   9,   6,  2, RW)
    ARM64_CP_REG_DEFINE(AMEVCNTVOFF015_EL2,      3,   4,  13,   9,   7,  2, RW)
    ARM64_CP_REG_DEFINE(AMEVCNTVOFF10_EL2,       3,   4,  13,  10,   0,  2, RW)
    ARM64_CP_REG_DEFINE(AMEVCNTVOFF11_EL2,       3,   4,  13,  10,   1,  2, RW)
    ARM64_CP_REG_DEFINE(AMEVCNTVOFF12_EL2,       3,   4,  13,  10,   2,  2, RW)
    ARM64_CP_REG_DEFINE(AMEVCNTVOFF13_EL2,       3,   4,  13,  10,   3,  2, RW)
    ARM64_CP_REG_DEFINE(AMEVCNTVOFF14_EL2,       3,   4,  13,  10,   4,  2, RW)
    ARM64_CP_REG_DEFINE(AMEVCNTVOFF15_EL2,       3,   4,  13,  10,   5,  2, RW)
    ARM64_CP_REG_DEFINE(AMEVCNTVOFF16_EL2,       3,   4,  13,  10,   6,  2, RW)
    ARM64_CP_REG_DEFINE(AMEVCNTVOFF17_EL2,       3,   4,  13,  10,   7,  2, RW)
    ARM64_CP_REG_DEFINE(AMEVCNTVOFF18_EL2,       3,   4,  13,  11,   0,  2, RW)
    ARM64_CP_REG_DEFINE(AMEVCNTVOFF19_EL2,       3,   4,  13,  11,   1,  2, RW)
    ARM64_CP_REG_DEFINE(AMEVCNTVOFF110_EL2,      3,   4,  13,  11,   2,  2, RW)
    ARM64_CP_REG_DEFINE(AMEVCNTVOFF111_EL2,      3,   4,  13,  11,   3,  2, RW)
    ARM64_CP_REG_DEFINE(AMEVCNTVOFF112_EL2,      3,   4,  13,  11,   4,  2, RW)
    ARM64_CP_REG_DEFINE(AMEVCNTVOFF113_EL2,      3,   4,  13,  11,   5,  2, RW)
    ARM64_CP_REG_DEFINE(AMEVCNTVOFF114_EL2,      3,   4,  13,  11,   6,  2, RW)
    ARM64_CP_REG_DEFINE(AMEVCNTVOFF115_EL2,      3,   4,  13,  11,   7,  2, RW)
    ARM64_CP_REG_DEFINE(AMEVTYPER00_EL0,         3,   3,  13,   6,   0,  0, RW)
    ARM64_CP_REG_DEFINE(AMEVTYPER10_EL0,         3,   3,  13,  14,   0,  0, RW)
    ARM64_CP_REG_DEFINE(AMEVTYPER11_EL0,         3,   3,  13,  14,   1,  0, RW)
    ARM64_CP_REG_DEFINE(AMEVTYPER12_EL0,         3,   3,  13,  14,   2,  0, RW)
    ARM64_CP_REG_DEFINE(AMEVTYPER13_EL0,         3,   3,  13,  14,   3,  0, RW)
    ARM64_CP_REG_DEFINE(AMEVTYPER14_EL0,         3,   3,  13,  14,   4,  0, RW)
    ARM64_CP_REG_DEFINE(AMEVTYPER15_EL0,         3,   3,  13,  14,   5,  0, RW)
    ARM64_CP_REG_DEFINE(AMEVTYPER16_EL0,         3,   3,  13,  14,   6,  0, RW)
    ARM64_CP_REG_DEFINE(AMEVTYPER17_EL0,         3,   3,  13,  14,   7,  0, RW)
    ARM64_CP_REG_DEFINE(AMEVTYPER18_EL0,         3,   3,  13,  15,   0,  0, RW)
    ARM64_CP_REG_DEFINE(AMEVTYPER19_EL0,         3,   3,  13,  15,   1,  0, RW)
    ARM64_CP_REG_DEFINE(AMEVTYPER110_EL0,        3,   3,  13,  15,   2,  0, RW)
    ARM64_CP_REG_DEFINE(AMEVTYPER111_EL0,        3,   3,  13,  15,   3,  0, RW)
    ARM64_CP_REG_DEFINE(AMEVTYPER112_EL0,        3,   3,  13,  15,   4,  0, RW)
    ARM64_CP_REG_DEFINE(AMEVTYPER113_EL0,        3,   3,  13,  15,   5,  0, RW)
    ARM64_CP_REG_DEFINE(AMEVTYPER114_EL0,        3,   3,  13,  15,   6,  0, RW)
    ARM64_CP_REG_DEFINE(AMEVTYPER115_EL0,        3,   3,  13,  15,   7,  0, RW)
    ARM64_CP_REG_DEFINE(AMUSERENR_EL0,           3,   3,  13,   2,   3,  0, RW)
    ARM64_CP_REG_DEFINE(APDAKeyHi_EL1,           3,   0,   2,   2,   1,  1, RW, FIELD(keys.apda.hi))
    ARM64_CP_REG_DEFINE(APDAKeyLo_EL1,           3,   0,   2,   2,   0,  1, RW, FIELD(keys.apda.lo))
    ARM64_CP_REG_DEFINE(APDBKeyHi_EL1,           3,   0,   2,   2,   3,  1, RW, FIELD(keys.apdb.hi))
    ARM64_CP_REG_DEFINE(APDBKeyLo_EL1,           3,   0,   2,   2,   2,  1, RW, FIELD(keys.apdb.lo))
    ARM64_CP_REG_DEFINE(APGAKeyHi_EL1,           3,   0,   2,   3,   1,  1, RW, FIELD(keys.apga.hi))
    ARM64_CP_REG_DEFINE(APGAKeyLo_EL1,           3,   0,   2,   3,   0,  1, RW, FIELD(keys.apga.lo))
    ARM64_CP_REG_DEFINE(APIAKeyHi_EL1,           3,   0,   2,   1,   1,  1, RW, FIELD(keys.apia.hi))
    ARM64_CP_REG_DEFINE(APIAKeyLo_EL1,           3,   0,   2,   1,   0,  1, RW, FIELD(keys.apia.lo))
    ARM64_CP_REG_DEFINE(APIBKeyHi_EL1,           3,   0,   2,   1,   3,  1, RW, FIELD(keys.apib.hi))
    ARM64_CP_REG_DEFINE(APIBKeyLo_EL1,           3,   0,   2,   1,   2,  1, RW, FIELD(keys.apib.lo))
    ARM64_CP_REG_DEFINE(CCSIDR_EL1,              3,   1,   0,   0,   0,  1, RO, READFN(ccsidr_el1))
    ARM64_CP_REG_DEFINE(CCSIDR2_EL1,             3,   1,   0,   0,   2,  1, RO, READFN(ccsidr2_el1))
    ARM64_CP_REG_DEFINE(CLIDR_EL1,               3,   1,   0,   0,   1,  1, RO, READFN(clidr_el1))
    // TODO: Implement trap on access to CNT* registers
    // The configuration of traping depends on flags from CNTHCTL_EL2 and CNTKCTL_EL1 registers
    ARM64_CP_REG_DEFINE(CNTFRQ_EL0,              3,   3,  14,   0,   0,  0, RW, RW_FNS(generic_timer))
    ARM64_CP_REG_DEFINE(CNTHCTL_EL2,             3,   4,  14,   1,   0,  2, RW, RW_FNS(generic_timer))
    ARM64_CP_REG_DEFINE(CNTHP_CTL_EL2,           3,   4,  14,   2,   1,  2, RW, RW_FNS(generic_timer))
    ARM64_CP_REG_DEFINE(CNTHP_CVAL_EL2,          3,   4,  14,   2,   2,  2, RW, RW_FNS(generic_timer))
    ARM64_CP_REG_DEFINE(CNTHP_TVAL_EL2,          3,   4,  14,   2,   0,  2, RW, RW_FNS(generic_timer))
    ARM64_CP_REG_DEFINE(CNTHPS_CTL_EL2,          3,   4,  14,   5,   1,  2, RW, RW_FNS(generic_timer))
    ARM64_CP_REG_DEFINE(CNTHPS_CVAL_EL2,         3,   4,  14,   5,   2,  2, RW, RW_FNS(generic_timer))
    ARM64_CP_REG_DEFINE(CNTHPS_TVAL_EL2,         3,   4,  14,   5,   0,  2, RW, RW_FNS(generic_timer))
    ARM64_CP_REG_DEFINE(CNTHV_CTL_EL2,           3,   4,  14,   3,   1,  2, RW, RW_FNS(generic_timer))
    ARM64_CP_REG_DEFINE(CNTHV_CVAL_EL2,          3,   4,  14,   3,   2,  2, RW, RW_FNS(generic_timer))
    ARM64_CP_REG_DEFINE(CNTHV_TVAL_EL2,          3,   4,  14,   3,   0,  2, RW, RW_FNS(generic_timer))
    ARM64_CP_REG_DEFINE(CNTHVS_CTL_EL2,          3,   4,  14,   4,   1,  2, RW, RW_FNS(generic_timer))
    ARM64_CP_REG_DEFINE(CNTHVS_CVAL_EL2,         3,   4,  14,   4,   2,  2, RW, RW_FNS(generic_timer))
    ARM64_CP_REG_DEFINE(CNTHVS_TVAL_EL2,         3,   4,  14,   4,   0,  2, RW, RW_FNS(generic_timer))
    ARM64_CP_REG_DEFINE(CNTKCTL_EL1,             3,   0,  14,   1,   0,  1, RW, RW_FNS(generic_timer))
    ARM64_CP_REG_DEFINE(CNTKCTL_EL12,            3,   5,  14,   1,   0,  2, RW, RW_FNS(generic_timer))
    ARM64_CP_REG_DEFINE(CNTP_CTL_EL0,            3,   3,  14,   2,   1,  0, RW, RW_FNS(generic_timer))
    ARM64_CP_REG_DEFINE(CNTP_CTL_EL02,           3,   5,  14,   2,   1,  0, RW, RW_FNS(generic_timer))
    ARM64_CP_REG_DEFINE(CNTP_CVAL_EL0,           3,   3,  14,   2,   2,  0, RW, RW_FNS(generic_timer))
    ARM64_CP_REG_DEFINE(CNTP_CVAL_EL02,          3,   5,  14,   2,   2,  0, RW, RW_FNS(generic_timer))
    ARM64_CP_REG_DEFINE(CNTP_TVAL_EL0,           3,   3,  14,   2,   0,  0, RW, RW_FNS(generic_timer))
    ARM64_CP_REG_DEFINE(CNTP_TVAL_EL02,          3,   5,  14,   2,   0,  0, RW, RW_FNS(generic_timer))
    ARM64_CP_REG_DEFINE(CNTPCT_EL0,              3,   3,  14,   0,   1,  0, RO, READFN(generic_timer))
    ARM64_CP_REG_DEFINE(CNTPCTSS_EL0,            3,   3,  14,   0,   5,  0, RO, READFN(generic_timer))
    ARM64_CP_REG_DEFINE(CNTPOFF_EL2,             3,   4,  14,   0,   6,  2, RW, RW_FNS(generic_timer))
    ARM64_CP_REG_DEFINE(CNTPS_CTL_EL1,           3,   7,  14,   2,   1,  1, RW, RW_FNS(generic_timer))
    ARM64_CP_REG_DEFINE(CNTPS_CVAL_EL1,          3,   7,  14,   2,   2,  1, RW, RW_FNS(generic_timer))
    ARM64_CP_REG_DEFINE(CNTPS_TVAL_EL1,          3,   7,  14,   2,   0,  1, RW, RW_FNS(generic_timer))
    ARM64_CP_REG_DEFINE(CNTV_CTL_EL0,            3,   3,  14,   3,   1,  0, RW, RW_FNS(generic_timer))
    ARM64_CP_REG_DEFINE(CNTV_CTL_EL02,           3,   5,  14,   3,   1,  0, RW, RW_FNS(generic_timer))
    ARM64_CP_REG_DEFINE(CNTV_CVAL_EL0,           3,   3,  14,   3,   2,  0, RW, RW_FNS(generic_timer))
    ARM64_CP_REG_DEFINE(CNTV_CVAL_EL02,          3,   5,  14,   3,   2,  0, RW, RW_FNS(generic_timer))
    ARM64_CP_REG_DEFINE(CNTV_TVAL_EL0,           3,   3,  14,   3,   0,  0, RW, RW_FNS(generic_timer))
    ARM64_CP_REG_DEFINE(CNTV_TVAL_EL02,          3,   5,  14,   3,   0,  0, RW, RW_FNS(generic_timer))
    ARM64_CP_REG_DEFINE(CNTVCT_EL0,              3,   3,  14,   0,   2,  0, RO, READFN(generic_timer))
    ARM64_CP_REG_DEFINE(CNTVCTSS_EL0,            3,   3,  14,   0,   6,  0, RO, READFN(generic_timer))
    ARM64_CP_REG_DEFINE(CNTVOFF_EL2,             3,   4,  14,   0,   3,  2, RW, RW_FNS(generic_timer))
    ARM64_CP_REG_DEFINE(CONTEXTIDR_EL1,          3,   0,  13,   0,   1,  1, RW, RW_FNS(contextidr_el1))
    ARM64_CP_REG_DEFINE(CONTEXTIDR_EL12,         3,   5,  13,   0,   1,  2, RW, FIELD(cp15.contextidr_el[1]))
    ARM64_CP_REG_DEFINE(CONTEXTIDR_EL2,          3,   4,  13,   0,   1,  2, RW, FIELD(cp15.contextidr_el[2]))
    ARM64_CP_REG_DEFINE(CPACR_EL1,               3,   0,   1,   0,   2,  1, RW, RW_FNS(cpacr_el1))
    ARM64_CP_REG_DEFINE(CPACR_EL12,              3,   5,   1,   0,   2,  2, RW, FIELD(cp15.cpacr_el1))
    ARM64_CP_REG_DEFINE(CPTR_EL2,                3,   4,   1,   1,   2,  2, RW, FIELD(cp15.cptr_el[2]))
    ARM64_CP_REG_DEFINE(CPTR_EL3,                3,   6,   1,   1,   2,  3, RW, FIELD(cp15.cptr_el[3]))
    ARM64_CP_REG_DEFINE(CSSELR_EL1,              3,   2,   0,   0,   0,  1, RW, FIELD(cp15.csselr_el[1]))
    ARM64_CP_REG_DEFINE(CTR_EL0,                 3,   3,   0,   0,   1,  0, RO, READFN(ctr_el0))
    ARM64_CP_REG_DEFINE(DACR32_EL2,              3,   4,   3,   0,   0,  2, RW, FIELD(cp15.dacr32_el2))
    ARM64_CP_REG_DEFINE(DAIF,                    3,   3,   4,   2,   1,  0, RW, FIELD(daif))
    ARM64_CP_REG_DEFINE(DBGAUTHSTATUS_EL1,       2,   0,   7,  14,   6,  1, RO)
    ARM64_CP_REG_DEFINE(DBGBCR0_EL1,             2,   0,   0,   0,   5,  1, RW, FIELD(cp15.dbgbcr[0]))
    ARM64_CP_REG_DEFINE(DBGBCR1_EL1,             2,   0,   0,   1,   5,  1, RW, FIELD(cp15.dbgbcr[1]))
    ARM64_CP_REG_DEFINE(DBGBCR2_EL1,             2,   0,   0,   2,   5,  1, RW, FIELD(cp15.dbgbcr[2]))
    ARM64_CP_REG_DEFINE(DBGBCR3_EL1,             2,   0,   0,   3,   5,  1, RW, FIELD(cp15.dbgbcr[3]))
    ARM64_CP_REG_DEFINE(DBGBCR4_EL1,             2,   0,   0,   4,   5,  1, RW, FIELD(cp15.dbgbcr[4]))
    ARM64_CP_REG_DEFINE(DBGBCR5_EL1,             2,   0,   0,   5,   5,  1, RW, FIELD(cp15.dbgbcr[5]))
    ARM64_CP_REG_DEFINE(DBGBCR6_EL1,             2,   0,   0,   6,   5,  1, RW, FIELD(cp15.dbgbcr[6]))
    ARM64_CP_REG_DEFINE(DBGBCR7_EL1,             2,   0,   0,   7,   5,  1, RW, FIELD(cp15.dbgbcr[7]))
    ARM64_CP_REG_DEFINE(DBGBCR8_EL1,             2,   0,   0,   8,   5,  1, RW, FIELD(cp15.dbgbcr[8]))
    ARM64_CP_REG_DEFINE(DBGBCR9_EL1,             2,   0,   0,   9,   5,  1, RW, FIELD(cp15.dbgbcr[9]))
    ARM64_CP_REG_DEFINE(DBGBCR10_EL1,            2,   0,   0,  10,   5,  1, RW, FIELD(cp15.dbgbcr[10]))
    ARM64_CP_REG_DEFINE(DBGBCR11_EL1,            2,   0,   0,  11,   5,  1, RW, FIELD(cp15.dbgbcr[11]))
    ARM64_CP_REG_DEFINE(DBGBCR12_EL1,            2,   0,   0,  12,   5,  1, RW, FIELD(cp15.dbgbcr[12]))
    ARM64_CP_REG_DEFINE(DBGBCR13_EL1,            2,   0,   0,  13,   5,  1, RW, FIELD(cp15.dbgbcr[13]))
    ARM64_CP_REG_DEFINE(DBGBCR14_EL1,            2,   0,   0,  14,   5,  1, RW, FIELD(cp15.dbgbcr[14]))
    ARM64_CP_REG_DEFINE(DBGBCR15_EL1,            2,   0,   0,  15,   5,  1, RW, FIELD(cp15.dbgbcr[15]))
    ARM64_CP_REG_DEFINE(DBGBVR0_EL1,             2,   0,   0,   0,   4,  1, RW, FIELD(cp15.dbgbvr[0]))
    ARM64_CP_REG_DEFINE(DBGBVR1_EL1,             2,   0,   0,   1,   4,  1, RW, FIELD(cp15.dbgbvr[1]))
    ARM64_CP_REG_DEFINE(DBGBVR2_EL1,             2,   0,   0,   2,   4,  1, RW, FIELD(cp15.dbgbvr[2]))
    ARM64_CP_REG_DEFINE(DBGBVR3_EL1,             2,   0,   0,   3,   4,  1, RW, FIELD(cp15.dbgbvr[3]))
    ARM64_CP_REG_DEFINE(DBGBVR4_EL1,             2,   0,   0,   4,   4,  1, RW, FIELD(cp15.dbgbvr[4]))
    ARM64_CP_REG_DEFINE(DBGBVR5_EL1,             2,   0,   0,   5,   4,  1, RW, FIELD(cp15.dbgbvr[5]))
    ARM64_CP_REG_DEFINE(DBGBVR6_EL1,             2,   0,   0,   6,   4,  1, RW, FIELD(cp15.dbgbvr[6]))
    ARM64_CP_REG_DEFINE(DBGBVR7_EL1,             2,   0,   0,   7,   4,  1, RW, FIELD(cp15.dbgbvr[7]))
    ARM64_CP_REG_DEFINE(DBGBVR8_EL1,             2,   0,   0,   8,   4,  1, RW, FIELD(cp15.dbgbvr[8]))
    ARM64_CP_REG_DEFINE(DBGBVR9_EL1,             2,   0,   0,   9,   4,  1, RW, FIELD(cp15.dbgbvr[9]))
    ARM64_CP_REG_DEFINE(DBGBVR10_EL1,            2,   0,   0,  10,   4,  1, RW, FIELD(cp15.dbgbvr[10]))
    ARM64_CP_REG_DEFINE(DBGBVR11_EL1,            2,   0,   0,  11,   4,  1, RW, FIELD(cp15.dbgbvr[11]))
    ARM64_CP_REG_DEFINE(DBGBVR12_EL1,            2,   0,   0,  12,   4,  1, RW, FIELD(cp15.dbgbvr[12]))
    ARM64_CP_REG_DEFINE(DBGBVR13_EL1,            2,   0,   0,  13,   4,  1, RW, FIELD(cp15.dbgbvr[13]))
    ARM64_CP_REG_DEFINE(DBGBVR14_EL1,            2,   0,   0,  14,   4,  1, RW, FIELD(cp15.dbgbvr[14]))
    ARM64_CP_REG_DEFINE(DBGBVR15_EL1,            2,   0,   0,  15,   4,  1, RW, FIELD(cp15.dbgbvr[15]))
    ARM64_CP_REG_DEFINE(DBGCLAIMCLR_EL1,         2,   0,   7,   9,   6,  1, RW)
    ARM64_CP_REG_DEFINE(DBGCLAIMSET_EL1,         2,   0,   7,   8,   6,  1, RW)
    // Both 'DBGDTRRX_EL0' (RO) and 'DBGDTRTX_EL0' (WO) use the same encoding apart from the read/write bit.
    // We can't have two registers with the same op0+op1+crn+crm+op2 value so let's combine their names.
    ARM64_CP_REG_DEFINE(DBGDTR_EL0,              2,   3,   0,   4,   0,  0, RW)
    ARM64_CP_REG_DEFINE(DBGDTR_RX_TX_EL0,        2,   3,   0,   5,   0,  0, RW)
    ARM64_CP_REG_DEFINE(DBGPRCR_EL1,             2,   0,   1,   4,   4,  1, RW)
    ARM64_CP_REG_DEFINE(DBGVCR32_EL2,            2,   4,   0,   7,   0,  2, RW)
    ARM64_CP_REG_DEFINE(DBGWCR0_EL1,             2,   0,   0,   0,   7,  1, RW, FIELD(cp15.dbgwcr[0]))
    ARM64_CP_REG_DEFINE(DBGWCR1_EL1,             2,   0,   0,   1,   7,  1, RW, FIELD(cp15.dbgwcr[1]))
    ARM64_CP_REG_DEFINE(DBGWCR2_EL1,             2,   0,   0,   2,   7,  1, RW, FIELD(cp15.dbgwcr[2]))
    ARM64_CP_REG_DEFINE(DBGWCR3_EL1,             2,   0,   0,   3,   7,  1, RW, FIELD(cp15.dbgwcr[3]))
    ARM64_CP_REG_DEFINE(DBGWCR4_EL1,             2,   0,   0,   4,   7,  1, RW, FIELD(cp15.dbgwcr[4]))
    ARM64_CP_REG_DEFINE(DBGWCR5_EL1,             2,   0,   0,   5,   7,  1, RW, FIELD(cp15.dbgwcr[5]))
    ARM64_CP_REG_DEFINE(DBGWCR6_EL1,             2,   0,   0,   6,   7,  1, RW, FIELD(cp15.dbgwcr[6]))
    ARM64_CP_REG_DEFINE(DBGWCR7_EL1,             2,   0,   0,   7,   7,  1, RW, FIELD(cp15.dbgwcr[7]))
    ARM64_CP_REG_DEFINE(DBGWCR8_EL1,             2,   0,   0,   8,   7,  1, RW, FIELD(cp15.dbgwcr[8]))
    ARM64_CP_REG_DEFINE(DBGWCR9_EL1,             2,   0,   0,   9,   7,  1, RW, FIELD(cp15.dbgwcr[9]))
    ARM64_CP_REG_DEFINE(DBGWCR10_EL1,            2,   0,   0,  10,   7,  1, RW, FIELD(cp15.dbgwcr[10]))
    ARM64_CP_REG_DEFINE(DBGWCR11_EL1,            2,   0,   0,  11,   7,  1, RW, FIELD(cp15.dbgwcr[11]))
    ARM64_CP_REG_DEFINE(DBGWCR12_EL1,            2,   0,   0,  12,   7,  1, RW, FIELD(cp15.dbgwcr[12]))
    ARM64_CP_REG_DEFINE(DBGWCR13_EL1,            2,   0,   0,  13,   7,  1, RW, FIELD(cp15.dbgwcr[13]))
    ARM64_CP_REG_DEFINE(DBGWCR14_EL1,            2,   0,   0,  14,   7,  1, RW, FIELD(cp15.dbgwcr[14]))
    ARM64_CP_REG_DEFINE(DBGWCR15_EL1,            2,   0,   0,  15,   7,  1, RW, FIELD(cp15.dbgwcr[15]))
    ARM64_CP_REG_DEFINE(DBGWVR0_EL1,             2,   0,   0,   0,   6,  1, RW, FIELD(cp15.dbgwvr[0]))
    ARM64_CP_REG_DEFINE(DBGWVR1_EL1,             2,   0,   0,   1,   6,  1, RW, FIELD(cp15.dbgwvr[1]))
    ARM64_CP_REG_DEFINE(DBGWVR2_EL1,             2,   0,   0,   2,   6,  1, RW, FIELD(cp15.dbgwvr[2]))
    ARM64_CP_REG_DEFINE(DBGWVR3_EL1,             2,   0,   0,   3,   6,  1, RW, FIELD(cp15.dbgwvr[3]))
    ARM64_CP_REG_DEFINE(DBGWVR4_EL1,             2,   0,   0,   4,   6,  1, RW, FIELD(cp15.dbgwvr[4]))
    ARM64_CP_REG_DEFINE(DBGWVR5_EL1,             2,   0,   0,   5,   6,  1, RW, FIELD(cp15.dbgwvr[5]))
    ARM64_CP_REG_DEFINE(DBGWVR6_EL1,             2,   0,   0,   6,   6,  1, RW, FIELD(cp15.dbgwvr[6]))
    ARM64_CP_REG_DEFINE(DBGWVR7_EL1,             2,   0,   0,   7,   6,  1, RW, FIELD(cp15.dbgwvr[7]))
    ARM64_CP_REG_DEFINE(DBGWVR8_EL1,             2,   0,   0,   8,   6,  1, RW, FIELD(cp15.dbgwvr[8]))
    ARM64_CP_REG_DEFINE(DBGWVR9_EL1,             2,   0,   0,   9,   6,  1, RW, FIELD(cp15.dbgwvr[9]))
    ARM64_CP_REG_DEFINE(DBGWVR10_EL1,            2,   0,   0,  10,   6,  1, RW, FIELD(cp15.dbgwvr[10]))
    ARM64_CP_REG_DEFINE(DBGWVR11_EL1,            2,   0,   0,  11,   6,  1, RW, FIELD(cp15.dbgwvr[11]))
    ARM64_CP_REG_DEFINE(DBGWVR12_EL1,            2,   0,   0,  12,   6,  1, RW, FIELD(cp15.dbgwvr[12]))
    ARM64_CP_REG_DEFINE(DBGWVR13_EL1,            2,   0,   0,  13,   6,  1, RW, FIELD(cp15.dbgwvr[13]))
    ARM64_CP_REG_DEFINE(DBGWVR14_EL1,            2,   0,   0,  14,   6,  1, RW, FIELD(cp15.dbgwvr[14]))
    ARM64_CP_REG_DEFINE(DBGWVR15_EL1,            2,   0,   0,  15,   6,  1, RW, FIELD(cp15.dbgwvr[15]))
    ARM64_CP_REG_DEFINE(DCZID_EL0,               3,   3,   0,   0,   7,  0, RO, READFN(dczid))
    ARM64_CP_REG_DEFINE(DISR_EL1,                3,   0,  12,   1,   1,  1, RW, FIELD(cp15.disr_el1))
    ARM64_CP_REG_DEFINE(DIT,                     3,   3,   4,   2,   5,  0, RW, RW_FNS(dit))
    ARM64_CP_REG_DEFINE(DLR_EL0,                 3,   3,   4,   5,   1,  0, RW)
    ARM64_CP_REG_DEFINE(DSPSR_EL0,               3,   3,   4,   5,   0,  0, RW)
    ARM64_CP_REG_DEFINE(ELR_EL1,                 3,   0,   4,   0,   1,  1, RW, RW_FNS(elr_el1))
    ARM64_CP_REG_DEFINE(ELR_EL12,                3,   5,   4,   0,   1,  2, RW, FIELD(elr_el[1]))
    ARM64_CP_REG_DEFINE(ELR_EL2,                 3,   4,   4,   0,   1,  2, RW, FIELD(elr_el[2]))
    ARM64_CP_REG_DEFINE(ELR_EL3,                 3,   6,   4,   0,   1,  3, RW, FIELD(elr_el[3]))
    ARM64_CP_REG_DEFINE(ERRIDR_EL1,              3,   0,   5,   3,   0,  1, RO)
    ARM64_CP_REG_DEFINE(ERRSELR_EL1,             3,   0,   5,   3,   1,  1, RW)
    ARM64_CP_REG_DEFINE(ERXADDR_EL1,             3,   0,   5,   4,   3,  1, RW)
    ARM64_CP_REG_DEFINE(ERXCTLR_EL1,             3,   0,   5,   4,   1,  1, RW)
    ARM64_CP_REG_DEFINE(ERXFR_EL1,               3,   0,   5,   4,   0,  1, RO)
    ARM64_CP_REG_DEFINE(ERXMISC0_EL1,            3,   0,   5,   5,   0,  1, RW)
    ARM64_CP_REG_DEFINE(ERXMISC1_EL1,            3,   0,   5,   5,   1,  1, RW)
    ARM64_CP_REG_DEFINE(ERXMISC2_EL1,            3,   0,   5,   5,   2,  1, RW)
    ARM64_CP_REG_DEFINE(ERXMISC3_EL1,            3,   0,   5,   5,   3,  1, RW)
    ARM64_CP_REG_DEFINE(ERXPFGCDN_EL1,           3,   0,   5,   4,   6,  1, RW)
    ARM64_CP_REG_DEFINE(ERXPFGCTL_EL1,           3,   0,   5,   4,   5,  1, RW)
    ARM64_CP_REG_DEFINE(ERXPFGF_EL1,             3,   0,   5,   4,   4,  1, RO)
    ARM64_CP_REG_DEFINE(ERXSTATUS_EL1,           3,   0,   5,   4,   2,  1, RW)
    ARM64_CP_REG_DEFINE(ESR_EL1,                 3,   0,   5,   2,   0,  1, RW, RW_FNS(esr_el1))
    ARM64_CP_REG_DEFINE(ESR_EL12,                3,   5,   5,   2,   0,  2, RW, FIELD(cp15.esr_el[1]))
    ARM64_CP_REG_DEFINE(ESR_EL2,                 3,   4,   5,   2,   0,  2, RW, FIELD(cp15.esr_el[2]))
    ARM64_CP_REG_DEFINE(ESR_EL3,                 3,   6,   5,   2,   0,  3, RW, FIELD(cp15.esr_el[3]))
    ARM64_CP_REG_DEFINE(FAR_EL1,                 3,   0,   6,   0,   0,  1, RW, RW_FNS(far_el1))
    ARM64_CP_REG_DEFINE(FAR_EL12,                3,   5,   6,   0,   0,  2, RW, FIELD(cp15.far_el[1]))
    ARM64_CP_REG_DEFINE(FAR_EL2,                 3,   4,   6,   0,   0,  2, RW, FIELD(cp15.far_el[2]))
    ARM64_CP_REG_DEFINE(FAR_EL3,                 3,   6,   6,   0,   0,  3, RW, FIELD(cp15.far_el[3]))
    ARM64_CP_REG_DEFINE(FPCR,                    3,   3,   4,   4,   0,  0, RW, RW_FNS(fpcr))
    ARM64_CP_REG_DEFINE(FPEXC32_EL2,             3,   4,   5,   3,   0,  2, RW)
    ARM64_CP_REG_DEFINE(FPSR,                    3,   3,   4,   4,   1,  0, RW, RW_FNS(fpsr))
    ARM64_CP_REG_DEFINE(GCR_EL1,                 3,   0,   1,   0,   6,  1, RW, FIELD(cp15.gcr_el1))
    // TODO: find out the correct value, possible values:
    // Log2 of the block size in words. The minimum supported size is 16B (value == 2) and the maximum is 256B (value == 6).
    ARM64_CP_REG_DEFINE(GMID_EL1,                3,   1,   0,   0,   4,  1, RO | CONST(0x6))
    ARM64_CP_REG_DEFINE(HACR_EL2,                3,   4,   1,   1,   7,  2, RW)
    ARM64_CP_REG_DEFINE(HAFGRTR_EL2,             3,   4,   3,   1,   6,  2, RW)
    ARM64_CP_REG_DEFINE(HCR_EL2,                 3,   4,   1,   1,   0,  2, RW, FIELD(cp15.hcr_el2))
    ARM64_CP_REG_DEFINE(HCRX_EL2,                3,   4,   1,   2,   2,  2, RW, FIELD(cp15.hcrx_el2))
    ARM64_CP_REG_DEFINE(HDFGRTR_EL2,             3,   4,   3,   1,   4,  2, RW)
    ARM64_CP_REG_DEFINE(HDFGWTR_EL2,             3,   4,   3,   1,   5,  2, RW)
    ARM64_CP_REG_DEFINE(HFGITR_EL2,              3,   4,   1,   1,   6,  2, RW)
    ARM64_CP_REG_DEFINE(HFGRTR_EL2,              3,   4,   1,   1,   4,  2, RW)
    ARM64_CP_REG_DEFINE(HFGWTR_EL2,              3,   4,   1,   1,   5,  2, RW)
    ARM64_CP_REG_DEFINE(HPFAR_EL2,               3,   4,   6,   0,   4,  2, RW, FIELD(cp15.hpfar_el2))
    ARM64_CP_REG_DEFINE(HSTR_EL2,                3,   4,   1,   1,   3,  2, RW, FIELD(cp15.hstr_el2))
    // TODO: Implement trap on access to ICC_* registers
    // The configuration of traping depends on flags from ICC_SRE_EL* registers
    //
    // The 'ICV_*' registers are accessed using their equivalent 'ICC_*' mnemonics depending on the HCR_EL2's FMO/IMO bits.
    ARM64_CP_REG_DEFINE(ICC_AP0R0_EL1,           3,   0,  12,   8,   4,  1, RW, RW_FNS(interrupt_cpu_interface))
    ARM64_CP_REG_DEFINE(ICC_AP0R1_EL1,           3,   0,  12,   8,   5,  1, RW, RW_FNS(interrupt_cpu_interface))
    ARM64_CP_REG_DEFINE(ICC_AP0R2_EL1,           3,   0,  12,   8,   6,  1, RW, RW_FNS(interrupt_cpu_interface))
    ARM64_CP_REG_DEFINE(ICC_AP0R3_EL1,           3,   0,  12,   8,   7,  1, RW, RW_FNS(interrupt_cpu_interface))
    ARM64_CP_REG_DEFINE(ICC_AP1R0_EL1,           3,   0,  12,   9,   0,  1, RW, RW_FNS(interrupt_cpu_interface))
    ARM64_CP_REG_DEFINE(ICC_AP1R1_EL1,           3,   0,  12,   9,   1,  1, RW, RW_FNS(interrupt_cpu_interface))
    ARM64_CP_REG_DEFINE(ICC_AP1R2_EL1,           3,   0,  12,   9,   2,  1, RW, RW_FNS(interrupt_cpu_interface))
    ARM64_CP_REG_DEFINE(ICC_AP1R3_EL1,           3,   0,  12,   9,   3,  1, RW, RW_FNS(interrupt_cpu_interface))
    ARM64_CP_REG_DEFINE(ICC_ASGI1R_EL1,          3,   0,  12,  11,   6,  1, RW, RW_FNS(interrupt_cpu_interface))
    ARM64_CP_REG_DEFINE(ICC_BPR0_EL1,            3,   0,  12,   8,   3,  1, RW, RW_FNS(interrupt_cpu_interface))
    ARM64_CP_REG_DEFINE(ICC_BPR1_EL1,            3,   0,  12,  12,   3,  1, RW, RW_FNS(interrupt_cpu_interface))
    ARM64_CP_REG_DEFINE(ICC_CTLR_EL1,            3,   0,  12,  12,   4,  1, RW, RW_FNS(interrupt_cpu_interface))
    ARM64_CP_REG_DEFINE(ICC_CTLR_EL3,            3,   6,  12,  12,   4,  3, RW, RW_FNS(interrupt_cpu_interface))
    ARM64_CP_REG_DEFINE(ICC_DIR_EL1,             3,   0,  12,  11,   1,  1, RW, RW_FNS(interrupt_cpu_interface))
    ARM64_CP_REG_DEFINE(ICC_EOIR0_EL1,           3,   0,  12,   8,   1,  1, RW, RW_FNS(interrupt_cpu_interface))
    ARM64_CP_REG_DEFINE(ICC_EOIR1_EL1,           3,   0,  12,  12,   1,  1, RW, RW_FNS(interrupt_cpu_interface))
    ARM64_CP_REG_DEFINE(ICC_HPPIR0_EL1,          3,   0,  12,   8,   2,  1, RW, RW_FNS(interrupt_cpu_interface))
    ARM64_CP_REG_DEFINE(ICC_HPPIR1_EL1,          3,   0,  12,  12,   2,  1, RW, RW_FNS(interrupt_cpu_interface))
    ARM64_CP_REG_DEFINE(ICC_IAR0_EL1,            3,   0,  12,   8,   0,  1, RW, RW_FNS(interrupt_cpu_interface))
    ARM64_CP_REG_DEFINE(ICC_IAR1_EL1,            3,   0,  12,  12,   0,  1, RW, RW_FNS(interrupt_cpu_interface))
    ARM64_CP_REG_DEFINE(ICC_IGRPEN0_EL1,         3,   0,  12,  12,   6,  1, RW, RW_FNS(interrupt_cpu_interface))
    ARM64_CP_REG_DEFINE(ICC_IGRPEN1_EL1,         3,   0,  12,  12,   7,  1, RW, RW_FNS(interrupt_cpu_interface))
    ARM64_CP_REG_DEFINE(ICC_IGRPEN1_EL3,         3,   6,  12,  12,   7,  3, RW, RW_FNS(interrupt_cpu_interface))
    ARM64_CP_REG_DEFINE(ICC_NMIAR1_EL1,          3,   0,  12,   9,   5,  1, RW, RW_FNS(interrupt_cpu_interface))
    ARM64_CP_REG_DEFINE(ICC_PMR_EL1,             3,   0,   4,   6,   0,  1, RW, RW_FNS(interrupt_cpu_interface))
    ARM64_CP_REG_DEFINE(ICC_RPR_EL1,             3,   0,  12,  11,   3,  1, RW, RW_FNS(interrupt_cpu_interface))
    ARM64_CP_REG_DEFINE(ICC_SGI0R_EL1,           3,   0,  12,  11,   7,  1, RW, RW_FNS(interrupt_cpu_interface))
    ARM64_CP_REG_DEFINE(ICC_SGI1R_EL1,           3,   0,  12,  11,   5,  1, RW, RW_FNS(interrupt_cpu_interface))
    ARM64_CP_REG_DEFINE(ICC_SRE_EL1,             3,   0,  12,  12,   5,  1, RW, RW_FNS(interrupt_cpu_interface))
    ARM64_CP_REG_DEFINE(ICC_SRE_EL2,             3,   4,  12,   9,   5,  2, RW, RW_FNS(interrupt_cpu_interface))
    ARM64_CP_REG_DEFINE(ICC_SRE_EL3,             3,   6,  12,  12,   5,  3, RW, RW_FNS(interrupt_cpu_interface))
    ARM64_CP_REG_DEFINE(ICH_AP0R0_EL2,           3,   4,  12,   8,   0,  2, RW)
    ARM64_CP_REG_DEFINE(ICH_AP0R1_EL2,           3,   4,  12,   8,   1,  2, RW)
    ARM64_CP_REG_DEFINE(ICH_AP0R2_EL2,           3,   4,  12,   8,   2,  2, RW)
    ARM64_CP_REG_DEFINE(ICH_AP0R3_EL2,           3,   4,  12,   8,   3,  2, RW)
    ARM64_CP_REG_DEFINE(ICH_AP1R0_EL2,           3,   4,  12,   9,   0,  2, RW)
    ARM64_CP_REG_DEFINE(ICH_AP1R1_EL2,           3,   4,  12,   9,   1,  2, RW)
    ARM64_CP_REG_DEFINE(ICH_AP1R2_EL2,           3,   4,  12,   9,   2,  2, RW)
    ARM64_CP_REG_DEFINE(ICH_AP1R3_EL2,           3,   4,  12,   9,   3,  2, RW)
    ARM64_CP_REG_DEFINE(ICH_EISR_EL2,            3,   4,  12,  11,   3,  2, RW)
    ARM64_CP_REG_DEFINE(ICH_ELRSR_EL2,           3,   4,  12,  11,   5,  2, RW)
    ARM64_CP_REG_DEFINE(ICH_HCR_EL2,             3,   4,  12,  11,   0,  2, RW)
    ARM64_CP_REG_DEFINE(ICH_LR0_EL2,             3,   4,  12,  12,   0,  2, RW)
    ARM64_CP_REG_DEFINE(ICH_LR1_EL2,             3,   4,  12,  12,   1,  2, RW)
    ARM64_CP_REG_DEFINE(ICH_LR2_EL2,             3,   4,  12,  12,   2,  2, RW)
    ARM64_CP_REG_DEFINE(ICH_LR3_EL2,             3,   4,  12,  12,   3,  2, RW)
    ARM64_CP_REG_DEFINE(ICH_LR4_EL2,             3,   4,  12,  12,   4,  2, RW)
    ARM64_CP_REG_DEFINE(ICH_LR5_EL2,             3,   4,  12,  12,   5,  2, RW)
    ARM64_CP_REG_DEFINE(ICH_LR6_EL2,             3,   4,  12,  12,   6,  2, RW)
    ARM64_CP_REG_DEFINE(ICH_LR7_EL2,             3,   4,  12,  12,   7,  2, RW)
    ARM64_CP_REG_DEFINE(ICH_LR8_EL2,             3,   4,  12,  13,   0,  2, RW)
    ARM64_CP_REG_DEFINE(ICH_LR9_EL2,             3,   4,  12,  13,   1,  2, RW)
    ARM64_CP_REG_DEFINE(ICH_LR10_EL2,            3,   4,  12,  13,   2,  2, RW)
    ARM64_CP_REG_DEFINE(ICH_LR11_EL2,            3,   4,  12,  13,   3,  2, RW)
    ARM64_CP_REG_DEFINE(ICH_LR12_EL2,            3,   4,  12,  13,   4,  2, RW)
    ARM64_CP_REG_DEFINE(ICH_LR13_EL2,            3,   4,  12,  13,   5,  2, RW)
    ARM64_CP_REG_DEFINE(ICH_LR14_EL2,            3,   4,  12,  13,   6,  2, RW)
    ARM64_CP_REG_DEFINE(ICH_LR15_EL2,            3,   4,  12,  13,   7,  2, RW)
    ARM64_CP_REG_DEFINE(ICH_MISR_EL2,            3,   4,  12,  11,   2,  2, RW)
    ARM64_CP_REG_DEFINE(ICH_VMCR_EL2,            3,   4,  12,  11,   7,  2, RW)
    ARM64_CP_REG_DEFINE(ICH_VTR_EL2,             3,   4,  12,  11,   1,  2, RW)
    ARM64_CP_REG_DEFINE(ID_AA64AFR0_EL1,         3,   0,   0,   5,   4,  1, RO, READFN(id_aa64afr0_el1))
    ARM64_CP_REG_DEFINE(ID_AA64AFR1_EL1,         3,   0,   0,   5,   5,  1, RO, READFN(id_aa64afr1_el1))
    ARM64_CP_REG_DEFINE(ID_AA64DFR0_EL1,         3,   0,   0,   5,   0,  1, RO, READFN(id_aa64dfr0_el1))
    ARM64_CP_REG_DEFINE(ID_AA64DFR1_EL1,         3,   0,   0,   5,   1,  1, RO)
    ARM64_CP_REG_DEFINE(ID_AA64ISAR0_EL1,        3,   0,   0,   6,   0,  1, RO, READFN(id_aa64isar0_el1))
    ARM64_CP_REG_DEFINE(ID_AA64ISAR1_EL1,        3,   0,   0,   6,   1,  1, RO, READFN(id_aa64isar1_el1))
    // TODO: Unimplemented
    // Prior to the introduction of the features described by this register, this register was unnamed and reserved, RES0 from EL1, EL2, and EL3.
    ARM64_CP_REG_DEFINE(ID_AA64ISAR2_EL1,        3,   0,   0,   6,   2,  1, RO)
    ARM64_CP_REG_DEFINE(ID_AA64MMFR0_EL1,        3,   0,   0,   7,   0,  1, RO, READFN(id_aa64mmfr0_el1))
    ARM64_CP_REG_DEFINE(ID_AA64MMFR1_EL1,        3,   0,   0,   7,   1,  1, RO, READFN(id_aa64mmfr1_el1))
    ARM64_CP_REG_DEFINE(ID_AA64MMFR2_EL1,        3,   0,   0,   7,   2,  1, RO, READFN(id_aa64mmfr2_el1))
    ARM64_CP_REG_DEFINE(ID_AA64PFR0_EL1,         3,   0,   0,   4,   0,  1, RO, READFN(id_aa64pfr0_el1))
    ARM64_CP_REG_DEFINE(ID_AA64PFR1_EL1,         3,   0,   0,   4,   1,  1, RO, READFN(id_aa64pfr1_el1))
    ARM64_CP_REG_DEFINE(ID_AA64SMFR0_EL1,        3,   0,   0,   4,   5,  1, RO, READFN(id_aa64smfr0_el1))
    ARM64_CP_REG_DEFINE(ID_AA64ZFR0_EL1,         3,   0,   0,   4,   4,  1, RO, READFN(id_aa64zfr0_el1))
    ARM64_CP_REG_DEFINE(ID_AFR0_EL1,             3,   0,   0,   1,   3,  1, RO, READFN(id_afr0))
    ARM64_CP_REG_DEFINE(ID_DFR0_EL1,             3,   0,   0,   1,   2,  1, RO, READFN(id_dfr0))
    ARM64_CP_REG_DEFINE(ID_DFR1_EL1,             3,   0,   0,   3,   5,  1, RO, READFN(id_dfr1))
    ARM64_CP_REG_DEFINE(ID_ISAR0_EL1,            3,   0,   0,   2,   0,  1, RO, READFN(id_isar0))
    ARM64_CP_REG_DEFINE(ID_ISAR1_EL1,            3,   0,   0,   2,   1,  1, RO, READFN(id_isar1))
    ARM64_CP_REG_DEFINE(ID_ISAR2_EL1,            3,   0,   0,   2,   2,  1, RO, READFN(id_isar2))
    ARM64_CP_REG_DEFINE(ID_ISAR3_EL1,            3,   0,   0,   2,   3,  1, RO, READFN(id_isar3))
    ARM64_CP_REG_DEFINE(ID_ISAR4_EL1,            3,   0,   0,   2,   4,  1, RO, READFN(id_isar4))
    ARM64_CP_REG_DEFINE(ID_ISAR5_EL1,            3,   0,   0,   2,   5,  1, RO, READFN(id_isar5))
    ARM64_CP_REG_DEFINE(ID_ISAR6_EL1,            3,   0,   0,   2,   7,  1, RO, READFN(id_isar6))
    ARM64_CP_REG_DEFINE(ID_MMFR0_EL1,            3,   0,   0,   1,   4,  1, RO, READFN(id_mmfr0))
    ARM64_CP_REG_DEFINE(ID_MMFR1_EL1,            3,   0,   0,   1,   5,  1, RO, READFN(id_mmfr1))
    ARM64_CP_REG_DEFINE(ID_MMFR2_EL1,            3,   0,   0,   1,   6,  1, RO, READFN(id_mmfr2))
    ARM64_CP_REG_DEFINE(ID_MMFR3_EL1,            3,   0,   0,   1,   7,  1, RO, READFN(id_mmfr3))
    ARM64_CP_REG_DEFINE(ID_MMFR4_EL1,            3,   0,   0,   2,   6,  1, RO, READFN(id_mmfr4))
    ARM64_CP_REG_DEFINE(ID_MMFR5_EL1,            3,   0,   0,   3,   6,  1, RO, READFN(id_mmfr5))
    ARM64_CP_REG_DEFINE(ID_PFR0_EL1,             3,   0,   0,   1,   0,  1, RO, READFN(id_pfr0))
    ARM64_CP_REG_DEFINE(ID_PFR1_EL1,             3,   0,   0,   1,   1,  1, RO, READFN(id_pfr1))
    ARM64_CP_REG_DEFINE(ID_PFR2_EL1,             3,   0,   0,   3,   4,  1, RO, READFN(id_pfr2))
    ARM64_CP_REG_DEFINE(IFSR32_EL2,              3,   4,   5,   0,   1,  2, RW, FIELD(cp15.ifsr32_el2))
    ARM64_CP_REG_DEFINE(ISR_EL1,                 3,   0,  12,   1,   0,  1, RO)
    ARM64_CP_REG_DEFINE(LORC_EL1,                3,   0,  10,   4,   3,  1, RW)
    ARM64_CP_REG_DEFINE(LOREA_EL1,               3,   0,  10,   4,   1,  1, RW)
    ARM64_CP_REG_DEFINE(LORID_EL1,               3,   0,  10,   4,   7,  1, RO)
    ARM64_CP_REG_DEFINE(LORN_EL1,                3,   0,  10,   4,   2,  1, RW)
    ARM64_CP_REG_DEFINE(LORSA_EL1,               3,   0,  10,   4,   0,  1, RW)
    ARM64_CP_REG_DEFINE(MAIR_EL1,                3,   0,  10,   2,   0,  1, RW, RW_FNS(mair_el1))
    ARM64_CP_REG_DEFINE(MAIR_EL12,               3,   5,  10,   2,   0,  2, RW, FIELD(cp15.mair_el[1]))
    ARM64_CP_REG_DEFINE(MAIR_EL2,                3,   4,  10,   2,   0,  2, RW, FIELD(cp15.mair_el[2]))
    ARM64_CP_REG_DEFINE(MAIR_EL3,                3,   6,  10,   2,   0,  3, RW, FIELD(cp15.mair_el[3]))
    ARM64_CP_REG_DEFINE(MDCCINT_EL1,             2,   0,   0,   2,   0,  1, RW)
    ARM64_CP_REG_DEFINE(MDCCSR_EL0,              2,   3,   0,   1,   0,  0, RO)
    ARM64_CP_REG_DEFINE(MDCR_EL2,                3,   4,   1,   1,   1,  2, RW, FIELD(cp15.mdcr_el2))
    ARM64_CP_REG_DEFINE(MDCR_EL3,                3,   6,   1,   3,   1,  3, RW, FIELD(cp15.mdcr_el3))
    ARM64_CP_REG_DEFINE(MDRAR_EL1,               2,   0,   1,   0,   0,  1, RO)
    ARM64_CP_REG_DEFINE(MDSCR_EL1,               2,   0,   0,   2,   2,  1, RW, FIELD(cp15.mdscr_el1))
    ARM64_CP_REG_DEFINE(MIDR_EL1,                3,   0,   0,   0,   0,  1, RO, READFN(midr))
    ARM64_CP_REG_DEFINE(MPAM0_EL1,               3,   0,  10,   5,   1,  1, RW)
    ARM64_CP_REG_DEFINE(MPAM1_EL1,               3,   0,  10,   5,   0,  1, RW)
    ARM64_CP_REG_DEFINE(MPAM2_EL2,               3,   4,  10,   5,   0,  2, RW)
    ARM64_CP_REG_DEFINE(MPAM3_EL3,               3,   6,  10,   5,   0,  3, RW)
    ARM64_CP_REG_DEFINE(MPAMHCR_EL2,             3,   4,  10,   4,   0,  2, RW)
    ARM64_CP_REG_DEFINE(MPAMIDR_EL1,             3,   0,  10,   4,   4,  1, RW)
    ARM64_CP_REG_DEFINE(MPAMVPM0_EL2,            3,   4,  10,   6,   0,  2, RW)
    ARM64_CP_REG_DEFINE(MPAMVPM1_EL2,            3,   4,  10,   6,   1,  2, RW)
    ARM64_CP_REG_DEFINE(MPAMVPM2_EL2,            3,   4,  10,   6,   2,  2, RW)
    ARM64_CP_REG_DEFINE(MPAMVPM3_EL2,            3,   4,  10,   6,   3,  2, RW)
    ARM64_CP_REG_DEFINE(MPAMVPM4_EL2,            3,   4,  10,   6,   4,  2, RW)
    ARM64_CP_REG_DEFINE(MPAMVPM5_EL2,            3,   4,  10,   6,   5,  2, RW)
    ARM64_CP_REG_DEFINE(MPAMVPM6_EL2,            3,   4,  10,   6,   6,  2, RW)
    ARM64_CP_REG_DEFINE(MPAMVPM7_EL2,            3,   4,  10,   6,   7,  2, RW)
    ARM64_CP_REG_DEFINE(MPAMVPMV_EL2,            3,   4,  10,   4,   1,  2, RW)
    ARM64_CP_REG_DEFINE(MPIDR_EL1,               3,   0,   0,   0,   5,  1, RO, READFN(mpidr_el1))
    ARM64_CP_REG_DEFINE(MVFR0_EL1,               3,   0,   0,   3,   0,  1, RO, READFN(mvfr0_el1))
    ARM64_CP_REG_DEFINE(MVFR1_EL1,               3,   0,   0,   3,   1,  1, RO, READFN(mvfr1_el1))
    ARM64_CP_REG_DEFINE(MVFR2_EL1,               3,   0,   0,   3,   2,  1, RO, READFN(mvfr2_el1))
    ARM64_CP_REG_DEFINE(NZCV,                    3,   3,   4,   2,   0,  0, RW | ARM_CP_NZCV)
    ARM64_CP_REG_DEFINE(OSDLR_EL1,               2,   0,   1,   3,   4,  1, RW, FIELD(cp15.osdlr_el1))
    ARM64_CP_REG_DEFINE(OSDTRRX_EL1,             2,   0,   0,   0,   2,  1, RW)
    ARM64_CP_REG_DEFINE(OSDTRTX_EL1,             2,   0,   0,   3,   2,  1, RW)
    ARM64_CP_REG_DEFINE(OSECCR_EL1,              2,   0,   0,   6,   2,  1, RW)
    ARM64_CP_REG_DEFINE(OSLAR_EL1,               2,   0,   1,   0,   4,  1, WO)
    ARM64_CP_REG_DEFINE(OSLSR_EL1,               2,   0,   1,   1,   4,  1, RW, FIELD(cp15.oslsr_el1))
    ARM64_CP_REG_DEFINE(PAN,                     3,   0,   4,   2,   3,  1, RW, RW_FNS(pan))
    ARM64_CP_REG_DEFINE(PAR_EL1,                 3,   0,   7,   4,   0,  1, RW, FIELD(cp15.par_el[1]))
    ARM64_CP_REG_DEFINE(PMBIDR_EL1,              3,   0,   9,  10,   7,  1, RO)
    ARM64_CP_REG_DEFINE(PMBLIMITR_EL1,           3,   0,   9,  10,   0,  1, RW)
    ARM64_CP_REG_DEFINE(PMBPTR_EL1,              3,   0,   9,  10,   1,  1, RW)
    ARM64_CP_REG_DEFINE(PMBSR_EL1,               3,   0,   9,  10,   3,  1, RW)
    ARM64_CP_REG_DEFINE(PMCCFILTR_EL0,           3,   3,  14,  15,   7,  0, RW)
    ARM64_CP_REG_DEFINE(PMCCNTR_EL0,             3,   3,   9,  13,   0,  0, RW)
    ARM64_CP_REG_DEFINE(PMCEID0_EL0,             3,   3,   9,  12,   6,  0, RO)
    ARM64_CP_REG_DEFINE(PMCEID1_EL0,             3,   3,   9,  12,   7,  0, RO)
    ARM64_CP_REG_DEFINE(PMCNTENCLR_EL0,          3,   3,   9,  12,   2,  0, RW, FIELD(cp15.c9_pmcnten))
    ARM64_CP_REG_DEFINE(PMCNTENSET_EL0,          3,   3,   9,  12,   1,  0, RW, FIELD(cp15.c9_pmcnten))
    ARM64_CP_REG_DEFINE(PMCR_EL0,                3,   3,   9,  12,   0,  0, RW, FIELD(cp15.c9_pmcr))
    ARM64_CP_REG_DEFINE(PMEVCNTR0_EL0,           3,   3,  14,   8,   0,  0, RW)
    ARM64_CP_REG_DEFINE(PMEVCNTR1_EL0,           3,   3,  14,   8,   1,  0, RW)
    ARM64_CP_REG_DEFINE(PMEVCNTR2_EL0,           3,   3,  14,   8,   2,  0, RW)
    ARM64_CP_REG_DEFINE(PMEVCNTR3_EL0,           3,   3,  14,   8,   3,  0, RW)
    ARM64_CP_REG_DEFINE(PMEVCNTR4_EL0,           3,   3,  14,   8,   4,  0, RW)
    ARM64_CP_REG_DEFINE(PMEVCNTR5_EL0,           3,   3,  14,   8,   5,  0, RW)
    ARM64_CP_REG_DEFINE(PMEVCNTR6_EL0,           3,   3,  14,   8,   6,  0, RW)
    ARM64_CP_REG_DEFINE(PMEVCNTR7_EL0,           3,   3,  14,   8,   7,  0, RW)
    ARM64_CP_REG_DEFINE(PMEVCNTR8_EL0,           3,   3,  14,   9,   0,  0, RW)
    ARM64_CP_REG_DEFINE(PMEVCNTR9_EL0,           3,   3,  14,   9,   1,  0, RW)
    ARM64_CP_REG_DEFINE(PMEVCNTR10_EL0,          3,   3,  14,   9,   2,  0, RW)
    ARM64_CP_REG_DEFINE(PMEVCNTR11_EL0,          3,   3,  14,   9,   3,  0, RW)
    ARM64_CP_REG_DEFINE(PMEVCNTR12_EL0,          3,   3,  14,   9,   4,  0, RW)
    ARM64_CP_REG_DEFINE(PMEVCNTR13_EL0,          3,   3,  14,   9,   5,  0, RW)
    ARM64_CP_REG_DEFINE(PMEVCNTR14_EL0,          3,   3,  14,   9,   6,  0, RW)
    ARM64_CP_REG_DEFINE(PMEVCNTR15_EL0,          3,   3,  14,   9,   7,  0, RW)
    ARM64_CP_REG_DEFINE(PMEVCNTR16_EL0,          3,   3,  14,  10,   0,  0, RW)
    ARM64_CP_REG_DEFINE(PMEVCNTR17_EL0,          3,   3,  14,  10,   1,  0, RW)
    ARM64_CP_REG_DEFINE(PMEVCNTR18_EL0,          3,   3,  14,  10,   2,  0, RW)
    ARM64_CP_REG_DEFINE(PMEVCNTR19_EL0,          3,   3,  14,  10,   3,  0, RW)
    ARM64_CP_REG_DEFINE(PMEVCNTR20_EL0,          3,   3,  14,  10,   4,  0, RW)
    ARM64_CP_REG_DEFINE(PMEVCNTR21_EL0,          3,   3,  14,  10,   5,  0, RW)
    ARM64_CP_REG_DEFINE(PMEVCNTR22_EL0,          3,   3,  14,  10,   6,  0, RW)
    ARM64_CP_REG_DEFINE(PMEVCNTR23_EL0,          3,   3,  14,  10,   7,  0, RW)
    ARM64_CP_REG_DEFINE(PMEVCNTR24_EL0,          3,   3,  14,  11,   0,  0, RW)
    ARM64_CP_REG_DEFINE(PMEVCNTR25_EL0,          3,   3,  14,  11,   1,  0, RW)
    ARM64_CP_REG_DEFINE(PMEVCNTR26_EL0,          3,   3,  14,  11,   2,  0, RW)
    ARM64_CP_REG_DEFINE(PMEVCNTR27_EL0,          3,   3,  14,  11,   3,  0, RW)
    ARM64_CP_REG_DEFINE(PMEVCNTR28_EL0,          3,   3,  14,  11,   4,  0, RW)
    ARM64_CP_REG_DEFINE(PMEVCNTR29_EL0,          3,   3,  14,  11,   5,  0, RW)
    ARM64_CP_REG_DEFINE(PMEVCNTR30_EL0,          3,   3,  14,  11,   6,  0, RW)
    ARM64_CP_REG_DEFINE(PMEVTYPER0_EL0,          3,   3,  14,  12,   0,  0, RW)
    ARM64_CP_REG_DEFINE(PMEVTYPER1_EL0,          3,   3,  14,  12,   1,  0, RW)
    ARM64_CP_REG_DEFINE(PMEVTYPER2_EL0,          3,   3,  14,  12,   2,  0, RW)
    ARM64_CP_REG_DEFINE(PMEVTYPER3_EL0,          3,   3,  14,  12,   3,  0, RW)
    ARM64_CP_REG_DEFINE(PMEVTYPER4_EL0,          3,   3,  14,  12,   4,  0, RW)
    ARM64_CP_REG_DEFINE(PMEVTYPER5_EL0,          3,   3,  14,  12,   5,  0, RW)
    ARM64_CP_REG_DEFINE(PMEVTYPER6_EL0,          3,   3,  14,  12,   6,  0, RW)
    ARM64_CP_REG_DEFINE(PMEVTYPER7_EL0,          3,   3,  14,  12,   7,  0, RW)
    ARM64_CP_REG_DEFINE(PMEVTYPER8_EL0,          3,   3,  14,  13,   0,  0, RW)
    ARM64_CP_REG_DEFINE(PMEVTYPER9_EL0,          3,   3,  14,  13,   1,  0, RW)
    ARM64_CP_REG_DEFINE(PMEVTYPER10_EL0,         3,   3,  14,  13,   2,  0, RW)
    ARM64_CP_REG_DEFINE(PMEVTYPER11_EL0,         3,   3,  14,  13,   3,  0, RW)
    ARM64_CP_REG_DEFINE(PMEVTYPER12_EL0,         3,   3,  14,  13,   4,  0, RW)
    ARM64_CP_REG_DEFINE(PMEVTYPER13_EL0,         3,   3,  14,  13,   5,  0, RW)
    ARM64_CP_REG_DEFINE(PMEVTYPER14_EL0,         3,   3,  14,  13,   6,  0, RW)
    ARM64_CP_REG_DEFINE(PMEVTYPER15_EL0,         3,   3,  14,  13,   7,  0, RW)
    ARM64_CP_REG_DEFINE(PMEVTYPER16_EL0,         3,   3,  14,  14,   0,  0, RW)
    ARM64_CP_REG_DEFINE(PMEVTYPER17_EL0,         3,   3,  14,  14,   1,  0, RW)
    ARM64_CP_REG_DEFINE(PMEVTYPER18_EL0,         3,   3,  14,  14,   2,  0, RW)
    ARM64_CP_REG_DEFINE(PMEVTYPER19_EL0,         3,   3,  14,  14,   3,  0, RW)
    ARM64_CP_REG_DEFINE(PMEVTYPER20_EL0,         3,   3,  14,  14,   4,  0, RW)
    ARM64_CP_REG_DEFINE(PMEVTYPER21_EL0,         3,   3,  14,  14,   5,  0, RW)
    ARM64_CP_REG_DEFINE(PMEVTYPER22_EL0,         3,   3,  14,  14,   6,  0, RW)
    ARM64_CP_REG_DEFINE(PMEVTYPER23_EL0,         3,   3,  14,  14,   7,  0, RW)
    ARM64_CP_REG_DEFINE(PMEVTYPER24_EL0,         3,   3,  14,  15,   0,  0, RW)
    ARM64_CP_REG_DEFINE(PMEVTYPER25_EL0,         3,   3,  14,  15,   1,  0, RW)
    ARM64_CP_REG_DEFINE(PMEVTYPER26_EL0,         3,   3,  14,  15,   2,  0, RW)
    ARM64_CP_REG_DEFINE(PMEVTYPER27_EL0,         3,   3,  14,  15,   3,  0, RW)
    ARM64_CP_REG_DEFINE(PMEVTYPER28_EL0,         3,   3,  14,  15,   4,  0, RW)
    ARM64_CP_REG_DEFINE(PMEVTYPER29_EL0,         3,   3,  14,  15,   5,  0, RW)
    ARM64_CP_REG_DEFINE(PMEVTYPER30_EL0,         3,   3,  14,  15,   6,  0, RW)
    ARM64_CP_REG_DEFINE(PMINTENCLR_EL1,          3,   0,   9,  14,   2,  1, RW, FIELD(cp15.c9_pminten))
    ARM64_CP_REG_DEFINE(PMINTENSET_EL1,          3,   0,   9,  14,   1,  1, RW, FIELD(cp15.c9_pminten))
    ARM64_CP_REG_DEFINE(PMMIR_EL1,               3,   0,   9,  14,   6,  1, RO)
    ARM64_CP_REG_DEFINE(PMOVSCLR_EL0,            3,   3,   9,  12,   3,  0, RW, FIELD(cp15.c9_pmovsr))
    ARM64_CP_REG_DEFINE(PMOVSSET_EL0,            3,   3,   9,  14,   3,  0, RW, FIELD(cp15.c9_pmovsr))
    ARM64_CP_REG_DEFINE(PMSCR_EL1,               3,   0,   9,   9,   0,  1, RW)
    ARM64_CP_REG_DEFINE(PMSCR_EL12,              3,   5,   9,   9,   0,  2, RW)
    ARM64_CP_REG_DEFINE(PMSCR_EL2,               3,   4,   9,   9,   0,  2, RW)
    ARM64_CP_REG_DEFINE(PMSELR_EL0,              3,   3,   9,  12,   5,  0, RW, FIELD(cp15.c9_pmselr))
    ARM64_CP_REG_DEFINE(PMSEVFR_EL1,             3,   0,   9,   9,   5,  1, RW)
    ARM64_CP_REG_DEFINE(PMSFCR_EL1,              3,   0,   9,   9,   4,  1, RW)
    ARM64_CP_REG_DEFINE(PMSIDR_EL1,              3,   0,   9,   9,   7,  1, RO)
    ARM64_CP_REG_DEFINE(PMSIRR_EL1,              3,   0,   9,   9,   3,  1, RW)
    ARM64_CP_REG_DEFINE(PMSLATFR_EL1,            3,   0,   9,   9,   6,  1, RW)
    ARM64_CP_REG_DEFINE(PMSNEVFR_EL1,            3,   0,   9,   9,   1,  1, RW)
    ARM64_CP_REG_DEFINE(PMSWINC_EL0,             3,   3,   9,  12,   4,  0, WO)
    ARM64_CP_REG_DEFINE(PMUSERENR_EL0,           3,   3,   9,  14,   0,  0, RW, FIELD(cp15.c9_pmuserenr))
    ARM64_CP_REG_DEFINE(PMXEVCNTR_EL0,           3,   3,   9,  13,   2,  0, RW)
    ARM64_CP_REG_DEFINE(PMXEVTYPER_EL0,          3,   3,   9,  13,   1,  0, RW)
    ARM64_CP_REG_DEFINE(REVIDR_EL1,              3,   0,   0,   0,   6,  1, RO, READFN(revidr_el1))
    ARM64_CP_REG_DEFINE(RGSR_EL1,                3,   0,   1,   0,   5,  1, RW, FIELD(cp15.rgsr_el1))
    ARM64_CP_REG_DEFINE(RMR_EL1,                 3,   0,  12,   0,   2,  1, RW)
    ARM64_CP_REG_DEFINE(RMR_EL2,                 3,   4,  12,   0,   2,  2, RW)
    ARM64_CP_REG_DEFINE(RMR_EL3,                 3,   6,  12,   0,   2,  3, RW)
    ARM64_CP_REG_DEFINE(RNDR,                    3,   3,   2,   4,   0,  0, RO)
    ARM64_CP_REG_DEFINE(RNDRRS,                  3,   3,   2,   4,   1,  0, RO)
    // TODO: Only one of RVBAR_ELx should be present -- the one for the highest available EL.
    ARM64_CP_REG_DEFINE(RVBAR_EL1,               3,   0,  12,   0,   1,  1, RO, FIELD(cp15.rvbar))
    ARM64_CP_REG_DEFINE(RVBAR_EL2,               3,   4,  12,   0,   1,  2, RO, FIELD(cp15.rvbar))
    ARM64_CP_REG_DEFINE(RVBAR_EL3,               3,   6,  12,   0,   1,  3, RO, FIELD(cp15.rvbar))
    ARM64_CP_REG_DEFINE(SCR_EL3,                 3,   6,   1,   1,   0,  3, RW, FIELD(cp15.scr_el3))
    ARM64_CP_REG_DEFINE(SCTLR_EL1,               3,   0,   1,   0,   0,  1, RW, RW_FNS(sctlr_el1))
    ARM64_CP_REG_DEFINE(SCTLR_EL12,              3,   5,   1,   0,   0,  2, RW, FIELD(cp15.sctlr_el[1]))
    ARM64_CP_REG_DEFINE(SCTLR_EL2,               3,   4,   1,   0,   0,  2, RW, FIELD(cp15.sctlr_el[2]))
    ARM64_CP_REG_DEFINE(SCTLR_EL3,               3,   6,   1,   0,   0,  3, RW, FIELD(cp15.sctlr_el[3]))
    ARM64_CP_REG_DEFINE(SCXTNUM_EL0,             3,   3,  13,   0,   7,  0, RW, FIELD(scxtnum_el[0]))
    ARM64_CP_REG_DEFINE(SCXTNUM_EL1,             3,   0,  13,   0,   7,  1, RW, RW_FNS(scxtnum_el1))
    ARM64_CP_REG_DEFINE(SCXTNUM_EL12,            3,   5,  13,   0,   7,  2, RW, FIELD(scxtnum_el[1]))
    ARM64_CP_REG_DEFINE(SCXTNUM_EL2,             3,   4,  13,   0,   7,  2, RW, FIELD(scxtnum_el[2]))
    ARM64_CP_REG_DEFINE(SCXTNUM_EL3,             3,   6,  13,   0,   7,  3, RW, FIELD(scxtnum_el[3]))
    ARM64_CP_REG_DEFINE(SDER32_EL2,              3,   4,   1,   3,   1,  2, RW, FIELD(cp15.sder))
    ARM64_CP_REG_DEFINE(SDER32_EL3,              3,   6,   1,   1,   1,  3, RW, FIELD(cp15.sder))
    ARM64_CP_REG_DEFINE(SP_EL0,                  3,   0,   4,   1,   0,  0, RW, FIELD(sp_el[0]))
    ARM64_CP_REG_DEFINE(SP_EL1,                  3,   4,   4,   1,   0,  1, RW, FIELD(sp_el[1]))
    ARM64_CP_REG_DEFINE(SP_EL2,                  3,   6,   4,   1,   0,  3, RW, FIELD(sp_el[2]))
    ARM64_CP_REG_DEFINE(SPSel,                   3,   0,   4,   2,   0,  1, RW, RW_FNS(spsel))
    ARM64_CP_REG_DEFINE(SPSR_EL1,                3,   0,   4,   0,   0,  1, RW, RW_FNS(spsr_el1))
    ARM64_CP_REG_DEFINE(SPSR_EL12,               3,   5,   4,   0,   0,  2, RW, FIELD(banked_spsr[SPSR_EL1]))
    ARM64_CP_REG_DEFINE(SPSR_EL2,                3,   4,   4,   0,   0,  2, RW, FIELD(banked_spsr[SPSR_EL2]))
    ARM64_CP_REG_DEFINE(SPSR_EL3,                3,   6,   4,   0,   0,  3, RW, FIELD(banked_spsr[SPSR_EL3]))
    ARM64_CP_REG_DEFINE(SPSR_abt,                3,   4,   4,   3,   1,  2, RW, FIELD(banked_spsr[SPSR_ABT]))
    ARM64_CP_REG_DEFINE(SPSR_fiq,                3,   4,   4,   3,   3,  2, RW, FIELD(banked_spsr[SPSR_FIQ]))
    ARM64_CP_REG_DEFINE(SPSR_irq,                3,   4,   4,   3,   0,  2, RW, FIELD(banked_spsr[SPSR_IRQ]))
    ARM64_CP_REG_DEFINE(SPSR_und,                3,   4,   4,   3,   2,  2, RW, FIELD(banked_spsr[SPSR_UND]))
    ARM64_CP_REG_DEFINE(SSBS,                    3,   3,   4,   2,   6,  0, RW, RW_FNS(ssbs))
    ARM64_CP_REG_DEFINE(TCO,                     3,   3,   4,   2,   7,  0, RW, RW_FNS(tco))
    ARM64_CP_REG_DEFINE(TCR_EL1,                 3,   0,   2,   0,   2,  1, RW, RW_FNS(tcr_el1))
    ARM64_CP_REG_DEFINE(TCR_EL12,                3,   5,   2,   0,   2,  2, RW, FIELD(cp15.tcr_el[1]))
    ARM64_CP_REG_DEFINE(TCR_EL2,                 3,   4,   2,   0,   2,  2, RW, FIELD(cp15.tcr_el[2]))
    ARM64_CP_REG_DEFINE(TCR_EL3,                 3,   6,   2,   0,   2,  3, RW, FIELD(cp15.tcr_el[3]))
    ARM64_CP_REG_DEFINE(TFSR_EL1,                3,   0,   5,   6,   0,  1, RW, RW_FNS(tfsr_el1))
    ARM64_CP_REG_DEFINE(TFSR_EL12,               3,   5,   5,   6,   0,  2, RW, FIELD(cp15.tfsr_el[1]))
    ARM64_CP_REG_DEFINE(TFSR_EL2,                3,   4,   5,   6,   0,  2, RW, FIELD(cp15.tfsr_el[2]))
    ARM64_CP_REG_DEFINE(TFSR_EL3,                3,   6,   5,   6,   0,  3, RW, FIELD(cp15.tfsr_el[3]))
    ARM64_CP_REG_DEFINE(TFSRE0_EL1,              3,   0,   5,   6,   1,  1, RW, FIELD(cp15.tfsr_el[0]))
    ARM64_CP_REG_DEFINE(TPIDR_EL0,               3,   3,  13,   0,   2,  0, RW, FIELD(cp15.tpidr_el[0]))
    ARM64_CP_REG_DEFINE(TPIDR_EL1,               3,   0,  13,   0,   4,  1, RW, FIELD(cp15.tpidr_el[1]))
    ARM64_CP_REG_DEFINE(TPIDR_EL2,               3,   4,  13,   0,   2,  2, RW, FIELD(cp15.tpidr_el[2]))
    ARM64_CP_REG_DEFINE(TPIDR_EL3,               3,   6,  13,   0,   2,  3, RW, FIELD(cp15.tpidr_el[3]))
    ARM64_CP_REG_DEFINE(TPIDRRO_EL0,             3,   3,  13,   0,   3,  0, RW, FIELD(cp15.tpidrro_el[0]))
    ARM64_CP_REG_DEFINE(TTBR0_EL1,               3,   0,   2,   0,   0,  1, RW, RW_FNS(ttbr0_el1))
    ARM64_CP_REG_DEFINE(TTBR0_EL12,              3,   5,   2,   0,   0,  2, RW, FIELD(cp15.ttbr0_el[1]))
    ARM64_CP_REG_DEFINE(TTBR0_EL2,               3,   4,   2,   0,   0,  2, RW, FIELD(cp15.ttbr0_el[2]))
    ARM64_CP_REG_DEFINE(TTBR0_EL3,               3,   6,   2,   0,   0,  3, RW, FIELD(cp15.ttbr0_el[3]))
    ARM64_CP_REG_DEFINE(TTBR1_EL1,               3,   0,   2,   0,   1,  1, RW, RW_FNS(ttbr1_el1))
    ARM64_CP_REG_DEFINE(TTBR1_EL12,              3,   5,   2,   0,   1,  2, RW, FIELD(cp15.ttbr1_el[1]))
    ARM64_CP_REG_DEFINE(TTBR1_EL2,               3,   4,   2,   0,   1,  2, RW, FIELD(cp15.ttbr1_el[2]))
    ARM64_CP_REG_DEFINE(UAO,                     3,   0,   4,   2,   4,  1, RW, RW_FNS(uao))
    ARM64_CP_REG_DEFINE(VBAR_EL1,                3,   0,  12,   0,   0,  1, RW, RW_FNS(vbar_el1))
    ARM64_CP_REG_DEFINE(VBAR_EL12,               3,   5,  12,   0,   0,  2, RW, FIELD(cp15.vbar_el[1]))
    ARM64_CP_REG_DEFINE(VBAR_EL2,                3,   4,  12,   0,   0,  2, RW, FIELD(cp15.vbar_el[2]))
    ARM64_CP_REG_DEFINE(VBAR_EL3,                3,   6,  12,   0,   0,  3, RW, FIELD(cp15.vbar_el[3]))
    ARM64_CP_REG_DEFINE(VDISR_EL2,               3,   4,  12,   1,   1,  2, RW, FIELD(cp15.disr_el1))
    ARM64_CP_REG_DEFINE(VMPIDR_EL2,              3,   4,   0,   0,   5,  2, RW, FIELD(cp15.vmpidr_el2))
    ARM64_CP_REG_DEFINE(VNCR_EL2,                3,   4,   2,   2,   0,  2, RW)
    ARM64_CP_REG_DEFINE(VPIDR_EL2,               3,   4,   0,   0,   0,  2, RW, FIELD(cp15.vpidr_el2))
    ARM64_CP_REG_DEFINE(VSESR_EL2,               3,   4,   5,   2,   3,  2, RW, FIELD(cp15.vsesr_el2))
    ARM64_CP_REG_DEFINE(VSTCR_EL2,               3,   4,   2,   6,   2,  2, RW, FIELD(cp15.vstcr_el2))
    ARM64_CP_REG_DEFINE(VSTTBR_EL2,              3,   4,   2,   6,   0,  2, RW, FIELD(cp15.vsttbr_el2))
    ARM64_CP_REG_DEFINE(VTCR_EL2,                3,   4,   2,   1,   2,  2, RW, FIELD(cp15.vtcr_el2))
    ARM64_CP_REG_DEFINE(VTTBR_EL2,               3,   4,   2,   1,   0,  2, RW, FIELD(cp15.vttbr_el2))
    ARM64_CP_REG_DEFINE(ZCR_EL1,                 3,   0,   1,   2,   0,  1, RW, RW_FNS(zcr_el1))
    ARM64_CP_REG_DEFINE(ZCR_EL12,                3,   5,   1,   2,   0,  2, RW, FIELD(vfp.zcr_el[1]))
    ARM64_CP_REG_DEFINE(ZCR_EL2,                 3,   4,   1,   2,   0,  2, RW, FIELD(vfp.zcr_el[2]))
    ARM64_CP_REG_DEFINE(ZCR_EL3,                 3,   6,   1,   2,   0,  3, RW, FIELD(vfp.zcr_el[3]))
};

/* TLBI helpers */

typedef enum {
    TLBI_IS,
    TLBI_NS,
    TLBI_OS,
} TLBIShareability;

static inline uint16_t tlbi_get_mmu_indexes_mask(CPUState *env, const ARMCPRegInfo *ri)
{
    uint16_t el1_map, el2_map;
    if (arm_is_secure_below_el3(env)) {
        el1_map = ARMMMUIdxBit_SE10_1 | ARMMMUIdxBit_SE10_1_PAN | ARMMMUIdxBit_SE10_0;
        el2_map = ARMMMUIdxBit_SE20_2 | ARMMMUIdxBit_SE20_2_PAN | ARMMMUIdxBit_SE20_0;
    } else {
        el1_map = ARMMMUIdxBit_E10_1 | ARMMMUIdxBit_E10_1_PAN | ARMMMUIdxBit_E10_0;
        el2_map = ARMMMUIdxBit_E20_2 | ARMMMUIdxBit_E20_2_PAN | ARMMMUIdxBit_E20_0;
    }

    // Fortunately the instruction's min. access EL matches the target EL, e.g. it's 2 for VAE2.
    uint32_t tlbi_target_el = ARM_CP_GET_MIN_EL(ri->type);
    switch (tlbi_target_el) {
    case 1:
        return arm_is_el2_enabled(env) && hcr_e2h_and_tge_set(env) ? el2_map : el1_map;
    case 2:
        return el2_map;
    case 3:
        return ARMMMUIdxBit_SE3;
    default:
        tlib_assert_not_reached();
    }
}

TLBIShareability tlbi_get_shareability(CPUState *env, const ARMCPRegInfo *ri)
{
    if (strstr(ri->name, "IS") != NULL) {
        return TLBI_IS;
    } else if (strstr(ri->name, "OS") != NULL) {
        return TLBI_OS;
    } else {
        // The HCR_EL2's FB bit forces inner shareability for EL1.
        if ((arm_current_el(env) == 1) && (arm_hcr_el2_eff(env) & HCR_FB)) {
            return TLBI_IS;
        }
        return TLBI_NS;
    }
}

void tlbi_print_stub_logs(CPUState *env, const ARMCPRegInfo *ri)
{
    TLBIShareability tlbi_shareability = tlbi_get_shareability(env, ri);
    if (tlbi_shareability != TLBI_NS) {
        tlib_printf(LOG_LEVEL_DEBUG, "[%s] %s Shareable domain not implemented yet; falling back to normal variant", ri->name,
                    tlbi_shareability == TLBI_IS ? "Inner" : "Outer");
    }
}

// TODO: Implement remaining TLBI instructions.
WRITE_FUNCTION(64, tlbi_flush_all,
{
    tlib_printf(LOG_LEVEL_DEBUG, "[%s] Using TLBI stub, forcing full flush", info->name);

    tlb_flush(env, 1);
})

WRITE_FUNCTION(64, tlbi_va,
{
    tlbi_print_stub_logs(env, info);

    uint64_t pageaddr = sextract64(value << 12, 0, 56);
    uint32_t indexes_mask = tlbi_get_mmu_indexes_mask(env, info);
    tlb_flush_page_masked(env, pageaddr, indexes_mask);
})

WRITE_FUNCTION(64, tlbi_vmall,
{
    tlbi_print_stub_logs(env, info);

    uint16_t indexes_mask = tlbi_get_mmu_indexes_mask(env, info);
    tlb_flush_masked(env, indexes_mask);
})

ARMCPRegInfo aarch64_instructions[] = {
    // The params are:   name                   op0, op1, crn, crm, op2, el, extra_type, ...
    ARM64_CP_REG_DEFINE(AT S12E0R,               1,   4,   7,   8,   6,  0, WO)
    ARM64_CP_REG_DEFINE(AT S12E0W,               1,   4,   7,   8,   7,  0, WO)
    ARM64_CP_REG_DEFINE(AT S12E1R,               1,   4,   7,   8,   4,  1, WO)
    ARM64_CP_REG_DEFINE(AT S12E1W,               1,   4,   7,   8,   5,  1, WO)
    ARM64_CP_REG_DEFINE(AT S1E0R,                1,   0,   7,   8,   2,  0, WO)
    ARM64_CP_REG_DEFINE(AT S1E0W,                1,   0,   7,   8,   3,  0, WO)
    ARM64_CP_REG_DEFINE(AT S1E1R,                1,   0,   7,   8,   0,  1, WO)
    ARM64_CP_REG_DEFINE(AT S1E1RP,               1,   0,   7,   9,   0,  1, WO)
    ARM64_CP_REG_DEFINE(AT S1E1W,                1,   0,   7,   8,   1,  1, WO)
    ARM64_CP_REG_DEFINE(AT S1E1WP,               1,   0,   7,   9,   1,  1, WO)
    ARM64_CP_REG_DEFINE(AT S1E2R,                1,   4,   7,   8,   0,  2, WO)
    ARM64_CP_REG_DEFINE(AT S1E2W,                1,   4,   7,   8,   1,  2, WO)
    ARM64_CP_REG_DEFINE(AT S1E3R,                1,   6,   7,   8,   0,  3, WO)
    ARM64_CP_REG_DEFINE(AT S1E3W,                1,   6,   7,   8,   1,  3, WO)
    ARM64_CP_REG_DEFINE(CFP RCTX,                1,   3,   7,   3,   4,  0, WO)
    ARM64_CP_REG_DEFINE(CPP RCTX,                1,   3,   7,   3,   7,  0, WO)
    ARM64_CP_REG_DEFINE(DC CGDSW,                1,   0,   7,  10,   6,  0, WO | IGNORED)
    ARM64_CP_REG_DEFINE(DC CGDVAC,               1,   3,   7,  10,   5,  0, WO | IGNORED)
    ARM64_CP_REG_DEFINE(DC CGDVADP,              1,   3,   7,  13,   5,  0, WO | IGNORED)
    ARM64_CP_REG_DEFINE(DC CGDVAP,               1,   3,   7,  12,   5,  0, WO | IGNORED)
    ARM64_CP_REG_DEFINE(DC CGSW,                 1,   0,   7,  10,   4,  0, WO | IGNORED)
    ARM64_CP_REG_DEFINE(DC CGVAC,                1,   3,   7,  10,   3,  0, WO | IGNORED)
    ARM64_CP_REG_DEFINE(DC CGVADP,               1,   3,   7,  13,   3,  0, WO | IGNORED)
    ARM64_CP_REG_DEFINE(DC CGVAP,                1,   3,   7,  12,   3,  0, WO | IGNORED)
    ARM64_CP_REG_DEFINE(DC CIGDSW,               1,   0,   7,  14,   6,  0, WO | IGNORED)
    ARM64_CP_REG_DEFINE(DC CIGDVAC,              1,   3,   7,  14,   5,  0, WO | IGNORED)
    ARM64_CP_REG_DEFINE(DC CIGSW,                1,   0,   7,  14,   4,  0, WO | IGNORED)
    ARM64_CP_REG_DEFINE(DC CIGVAC,               1,   3,   7,  14,   3,  0, WO | IGNORED)
    ARM64_CP_REG_DEFINE(DC CISW,                 1,   0,   7,  14,   2,  0, WO | IGNORED)
    ARM64_CP_REG_DEFINE(DC CIVAC,                1,   3,   7,  14,   1,  0, WO | IGNORED)
    ARM64_CP_REG_DEFINE(DC CSW,                  1,   0,   7,  10,   2,  0, WO | IGNORED)
    ARM64_CP_REG_DEFINE(DC CVAC,                 1,   3,   7,  10,   1,  0, WO | IGNORED)
    ARM64_CP_REG_DEFINE(DC CVADP,                1,   3,   7,  13,   1,  0, WO | IGNORED)
    ARM64_CP_REG_DEFINE(DC CVAP,                 1,   3,   7,  12,   1,  0, WO | IGNORED)
    ARM64_CP_REG_DEFINE(DC CVAU,                 1,   3,   7,  11,   1,  0, WO | IGNORED)
    // DC GVA, DC GZVA and DC ZVA are handled differently in 'handle_sys'.
    ARM64_CP_REG_DEFINE(DC GVA,                  1,   3,   7,   4,   3,  0, WO | ARM_CP_DC_GVA)
    ARM64_CP_REG_DEFINE(DC GZVA,                 1,   3,   7,   4,   4,  0, WO | ARM_CP_DC_GZVA)
    ARM64_CP_REG_DEFINE(DC IGDSW,                1,   0,   7,   6,   6,  0, WO | IGNORED)
    ARM64_CP_REG_DEFINE(DC IGDVAC,               1,   0,   7,   6,   5,  0, WO | IGNORED)
    ARM64_CP_REG_DEFINE(DC IGSW,                 1,   0,   7,   6,   4,  0, WO | IGNORED)
    ARM64_CP_REG_DEFINE(DC IGVAC,                1,   0,   7,   6,   3,  0, WO | IGNORED)
    ARM64_CP_REG_DEFINE(DC ISW,                  1,   0,   7,   6,   2,  1, WO | IGNORED)
    ARM64_CP_REG_DEFINE(DC IVAC,                 1,   0,   7,   6,   1,  0, WO | IGNORED)
    ARM64_CP_REG_DEFINE(DC ZVA,                  1,   3,   7,   4,   1,  0, WO | ARM_CP_DC_ZVA)
    ARM64_CP_REG_DEFINE(DVP RCTX,                1,   3,   7,   3,   5,  0, WO)
    ARM64_CP_REG_DEFINE(IC IALLU,                1,   0,   7,   5,   0,  1, WO | IGNORED)
    ARM64_CP_REG_DEFINE(IC IALLUIS,              1,   0,   7,   1,   0,  0, WO | IGNORED)
    ARM64_CP_REG_DEFINE(IC IVAU,                 1,   3,   7,   5,   1,  0, WO | IGNORED)
    ARM64_CP_REG_DEFINE(TLBI ALLE1,              1,   4,   8,   7,   4,  1, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI ALLE1IS,            1,   4,   8,   3,   4,  1, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI ALLE1ISNXS,         1,   4,   9,   3,   4,  1, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI ALLE1NXS,           1,   4,   9,   7,   4,  1, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI ALLE1OS,            1,   4,   8,   1,   4,  1, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI ALLE1OSNXS,         1,   4,   9,   1,   4,  1, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI ALLE2,              1,   4,   8,   7,   0,  2, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI ALLE2IS,            1,   4,   8,   3,   0,  2, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI ALLE2ISNXS,         1,   4,   9,   3,   0,  2, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI ALLE2NXS,           1,   4,   9,   7,   0,  2, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI ALLE2OS,            1,   4,   8,   1,   0,  2, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI ALLE2OSNXS,         1,   4,   9,   1,   0,  2, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI ALLE3,              1,   6,   8,   7,   0,  3, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI ALLE3IS,            1,   6,   8,   3,   0,  3, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI ALLE3ISNXS,         1,   6,   9,   3,   0,  3, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI ALLE3NXS,           1,   6,   9,   7,   0,  3, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI ALLE3OS,            1,   6,   8,   1,   0,  3, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI ALLE3OSNXS,         1,   6,   9,   1,   0,  3, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI ASIDE1,             1,   0,   8,   7,   2,  1, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI ASIDE1IS,           1,   0,   8,   3,   2,  1, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI ASIDE1ISNXS,        1,   0,   9,   3,   2,  1, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI ASIDE1NXS,          1,   0,   9,   7,   2,  1, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI ASIDE1OS,           1,   0,   8,   1,   2,  1, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI ASIDE1OSNXS,        1,   0,   9,   1,   2,  1, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI IPAS2E1,            1,   4,   8,   4,   1,  1, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI IPAS2E1IS,          1,   4,   8,   0,   1,  1, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI IPAS2E1ISNXS,       1,   4,   9,   0,   1,  1, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI IPAS2E1NXS,         1,   4,   9,   4,   1,  1, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI IPAS2E1OS,          1,   4,   8,   4,   0,  1, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI IPAS2E1OSNXS,       1,   4,   9,   4,   0,  1, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI IPAS2LE1,           1,   4,   8,   4,   5,  1, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI IPAS2LE1IS,         1,   4,   8,   0,   5,  1, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI IPAS2LE1ISNXS,      1,   4,   9,   0,   5,  1, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI IPAS2LE1NXS,        1,   4,   9,   4,   5,  1, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI IPAS2LE1OS,         1,   4,   8,   4,   4,  1, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI IPAS2LE1OSNXS,      1,   4,   9,   4,   4,  1, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI RIPAS2E1,           1,   4,   8,   4,   2,  1, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI RIPAS2E1IS,         1,   4,   8,   0,   2,  1, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI RIPAS2E1ISNXS,      1,   4,   9,   0,   2,  1, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI RIPAS2E1NXS,        1,   4,   9,   4,   2,  1, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI RIPAS2E1OS,         1,   4,   8,   4,   3,  1, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI RIPAS2E1OSNXS,      1,   4,   9,   4,   3,  1, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI RIPAS2LE1,          1,   4,   8,   4,   6,  1, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI RIPAS2LE1IS,        1,   4,   8,   0,   6,  1, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI RIPAS2LE1ISNXS,     1,   4,   9,   0,   6,  1, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI RIPAS2LE1NXS,       1,   4,   9,   4,   6,  1, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI RIPAS2LE1OS,        1,   4,   8,   4,   7,  1, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI RIPAS2LE1OSNXS,     1,   4,   9,   4,   7,  1, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI RVAAE1,             1,   0,   8,   6,   3,  1, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI RVAAE1IS,           1,   0,   8,   2,   3,  1, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI RVAAE1ISNXS,        1,   0,   9,   2,   3,  1, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI RVAAE1NXS,          1,   0,   9,   6,   3,  1, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI RVAAE1OS,           1,   0,   8,   5,   3,  1, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI RVAAE1OSNXS,        1,   0,   9,   5,   3,  1, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI RVAALE1,            1,   0,   8,   6,   7,  1, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI RVAALE1IS,          1,   0,   8,   2,   7,  1, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI RVAALE1ISNXS,       1,   0,   9,   2,   7,  1, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI RVAALE1NXS,         1,   0,   9,   6,   7,  1, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI RVAALE1OS,          1,   0,   8,   5,   7,  1, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI RVAALE1OSNXS,       1,   0,   9,   5,   7,  1, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI RVAE1,              1,   0,   8,   6,   1,  1, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI RVAE1IS,            1,   0,   8,   2,   1,  1, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI RVAE1ISNXS,         1,   0,   9,   2,   1,  1, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI RVAE1NXS,           1,   0,   9,   6,   1,  1, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI RVAE1OS,            1,   0,   8,   5,   1,  1, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI RVAE1OSNXS,         1,   0,   9,   5,   1,  1, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI RVAE2,              1,   4,   8,   6,   1,  2, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI RVAE2IS,            1,   4,   8,   2,   1,  2, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI RVAE2ISNXS,         1,   4,   9,   2,   1,  2, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI RVAE2NXS,           1,   4,   9,   6,   1,  2, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI RVAE3,              1,   6,   8,   6,   1,  3, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI RVAE3IS,            1,   6,   8,   2,   1,  3, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI RVAE3ISNXS,         1,   6,   9,   2,   1,  3, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI RVAE3NXS,           1,   6,   9,   6,   1,  3, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI RVAE3OS,            1,   6,   8,   5,   1,  3, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI RVAE3OSNXS,         1,   6,   9,   5,   1,  3, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI RVALE1,             1,   0,   8,   6,   5,  1, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI RVALE1IS,           1,   0,   8,   2,   5,  1, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI RVALE1ISNXS,        1,   0,   9,   2,   5,  1, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI RVALE1NXS,          1,   0,   9,   6,   5,  1, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI RVALE1OS,           1,   0,   8,   5,   5,  1, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI RVALE1OSNXS,        1,   0,   9,   5,   5,  1, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI RVALE2,             1,   4,   8,   6,   5,  2, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI RVALE2IS,           1,   4,   8,   2,   5,  2, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI RVALE2ISNXS,        1,   4,   9,   2,   5,  2, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI RVALE2NXS,          1,   4,   9,   6,   5,  2, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI RVALE2OS,           1,   4,   8,   5,   5,  2, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI RVALE2OSNXS,        1,   4,   9,   5,   5,  2, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI RVALE3,             1,   6,   8,   6,   5,  3, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI RVALE3IS,           1,   6,   8,   2,   5,  3, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI RVALE3ISNXS,        1,   6,   9,   2,   5,  3, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI RVALE3NXS,          1,   6,   9,   6,   5,  3, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI RVALE3OS,           1,   6,   8,   5,   5,  3, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI RVALE3OSNXS,        1,   6,   9,   5,   5,  3, WO, WRITEFN(tlbi_flush_all))
    ARM64_CP_REG_DEFINE(TLBI VAAE1,              1,   0,   8,   7,   3,  1, WO, WRITEFN(tlbi_va))
    ARM64_CP_REG_DEFINE(TLBI VAAE1IS,            1,   0,   8,   3,   3,  1, WO, WRITEFN(tlbi_va))
    ARM64_CP_REG_DEFINE(TLBI VAAE1ISNXS,         1,   0,   9,   3,   3,  1, WO, WRITEFN(tlbi_va))
    ARM64_CP_REG_DEFINE(TLBI VAAE1NXS,           1,   0,   9,   7,   3,  1, WO, WRITEFN(tlbi_va))
    ARM64_CP_REG_DEFINE(TLBI VAAE1OS,            1,   0,   8,   1,   3,  1, WO, WRITEFN(tlbi_va))
    ARM64_CP_REG_DEFINE(TLBI VAAE1OSNXS,         1,   0,   9,   1,   3,  1, WO, WRITEFN(tlbi_va))
    ARM64_CP_REG_DEFINE(TLBI VAALE1,             1,   0,   8,   7,   7,  1, WO, WRITEFN(tlbi_va))
    ARM64_CP_REG_DEFINE(TLBI VAALE1IS,           1,   0,   8,   3,   7,  1, WO, WRITEFN(tlbi_va))
    ARM64_CP_REG_DEFINE(TLBI VAALE1ISNXS,        1,   0,   9,   3,   7,  1, WO, WRITEFN(tlbi_va))
    ARM64_CP_REG_DEFINE(TLBI VAALE1NXS,          1,   0,   9,   7,   7,  1, WO, WRITEFN(tlbi_va))
    ARM64_CP_REG_DEFINE(TLBI VAALE1OS,           1,   0,   8,   1,   7,  1, WO, WRITEFN(tlbi_va))
    ARM64_CP_REG_DEFINE(TLBI VAALE1OSNXS,        1,   0,   9,   1,   7,  1, WO, WRITEFN(tlbi_va))
    ARM64_CP_REG_DEFINE(TLBI VAE1,               1,   0,   8,   7,   1,  1, WO, WRITEFN(tlbi_va))
    ARM64_CP_REG_DEFINE(TLBI VAE1IS,             1,   0,   8,   3,   1,  1, WO, WRITEFN(tlbi_va))
    ARM64_CP_REG_DEFINE(TLBI VAE1ISNXS,          1,   0,   9,   3,   1,  1, WO, WRITEFN(tlbi_va))
    ARM64_CP_REG_DEFINE(TLBI VAE1NXS,            1,   0,   9,   7,   1,  1, WO, WRITEFN(tlbi_va))
    ARM64_CP_REG_DEFINE(TLBI VAE1OS,             1,   0,   8,   1,   1,  1, WO, WRITEFN(tlbi_va))
    ARM64_CP_REG_DEFINE(TLBI VAE1OSNXS,          1,   0,   9,   1,   1,  1, WO, WRITEFN(tlbi_va))
    ARM64_CP_REG_DEFINE(TLBI VAE2,               1,   4,   8,   7,   1,  2, WO, WRITEFN(tlbi_va))
    ARM64_CP_REG_DEFINE(TLBI VAE2IS,             1,   4,   8,   3,   1,  2, WO, WRITEFN(tlbi_va))
    ARM64_CP_REG_DEFINE(TLBI VAE2ISNXS,          1,   4,   9,   3,   1,  2, WO, WRITEFN(tlbi_va))
    ARM64_CP_REG_DEFINE(TLBI VAE2NXS,            1,   4,   9,   7,   1,  2, WO, WRITEFN(tlbi_va))
    ARM64_CP_REG_DEFINE(TLBI VAE2OS,             1,   4,   8,   1,   1,  2, WO, WRITEFN(tlbi_va))
    ARM64_CP_REG_DEFINE(TLBI VAE2OSNXS,          1,   4,   9,   1,   1,  2, WO, WRITEFN(tlbi_va))
    ARM64_CP_REG_DEFINE(TLBI VAE3,               1,   6,   8,   7,   1,  3, WO, WRITEFN(tlbi_va))
    ARM64_CP_REG_DEFINE(TLBI VAE3IS,             1,   6,   8,   3,   1,  3, WO, WRITEFN(tlbi_va))
    ARM64_CP_REG_DEFINE(TLBI VAE3ISNXS,          1,   6,   9,   3,   1,  3, WO, WRITEFN(tlbi_va))
    ARM64_CP_REG_DEFINE(TLBI VAE3NXS,            1,   6,   9,   7,   1,  3, WO, WRITEFN(tlbi_va))
    ARM64_CP_REG_DEFINE(TLBI VAE3OS,             1,   6,   8,   1,   1,  3, WO, WRITEFN(tlbi_va))
    ARM64_CP_REG_DEFINE(TLBI VAE3OSNXS,          1,   6,   9,   1,   1,  3, WO, WRITEFN(tlbi_va))
    ARM64_CP_REG_DEFINE(TLBI VALE1,              1,   0,   8,   7,   5,  1, WO, WRITEFN(tlbi_va))
    ARM64_CP_REG_DEFINE(TLBI VALE1IS,            1,   0,   8,   3,   5,  1, WO, WRITEFN(tlbi_va))
    ARM64_CP_REG_DEFINE(TLBI VALE1ISNXS,         1,   0,   9,   3,   5,  1, WO, WRITEFN(tlbi_va))
    ARM64_CP_REG_DEFINE(TLBI VALE1NXS,           1,   0,   9,   7,   5,  1, WO, WRITEFN(tlbi_va))
    ARM64_CP_REG_DEFINE(TLBI VALE1OS,            1,   0,   8,   1,   5,  1, WO, WRITEFN(tlbi_va))
    ARM64_CP_REG_DEFINE(TLBI VALE1OSNXS,         1,   0,   9,   1,   5,  1, WO, WRITEFN(tlbi_va))
    ARM64_CP_REG_DEFINE(TLBI VALE2,              1,   4,   8,   7,   5,  2, WO, WRITEFN(tlbi_va))
    ARM64_CP_REG_DEFINE(TLBI VALE2IS,            1,   4,   8,   3,   5,  2, WO, WRITEFN(tlbi_va))
    ARM64_CP_REG_DEFINE(TLBI VALE2ISNXS,         1,   4,   9,   3,   5,  2, WO, WRITEFN(tlbi_va))
    ARM64_CP_REG_DEFINE(TLBI VALE2NXS,           1,   4,   9,   7,   5,  2, WO, WRITEFN(tlbi_va))
    ARM64_CP_REG_DEFINE(TLBI VALE2OS,            1,   4,   8,   1,   5,  2, WO, WRITEFN(tlbi_va))
    ARM64_CP_REG_DEFINE(TLBI VALE2OSNXS,         1,   4,   9,   1,   5,  2, WO, WRITEFN(tlbi_va))
    ARM64_CP_REG_DEFINE(TLBI VALE3,              1,   6,   8,   7,   5,  3, WO, WRITEFN(tlbi_va))
    ARM64_CP_REG_DEFINE(TLBI VALE3IS,            1,   6,   8,   3,   5,  3, WO, WRITEFN(tlbi_va))
    ARM64_CP_REG_DEFINE(TLBI VALE3ISNXS,         1,   6,   9,   3,   5,  3, WO, WRITEFN(tlbi_va))
    ARM64_CP_REG_DEFINE(TLBI VALE3NXS,           1,   6,   9,   7,   5,  3, WO, WRITEFN(tlbi_va))
    ARM64_CP_REG_DEFINE(TLBI VALE3OS,            1,   6,   8,   1,   5,  3, WO, WRITEFN(tlbi_va))
    ARM64_CP_REG_DEFINE(TLBI VALE3OSNXS,         1,   6,   9,   1,   5,  3, WO, WRITEFN(tlbi_va))
    ARM64_CP_REG_DEFINE(TLBI VMALLE1,            1,   0,   8,   7,   0,  1, WO, WRITEFN(tlbi_vmall))
    ARM64_CP_REG_DEFINE(TLBI VMALLE1IS,          1,   0,   8,   3,   0,  1, WO, WRITEFN(tlbi_vmall))
    ARM64_CP_REG_DEFINE(TLBI VMALLE1ISNXS,       1,   0,   9,   3,   0,  1, WO, WRITEFN(tlbi_vmall))
    ARM64_CP_REG_DEFINE(TLBI VMALLE1NXS,         1,   0,   9,   7,   0,  1, WO, WRITEFN(tlbi_vmall))
    ARM64_CP_REG_DEFINE(TLBI VMALLE1OS,          1,   0,   8,   1,   0,  1, WO, WRITEFN(tlbi_vmall))
    ARM64_CP_REG_DEFINE(TLBI VMALLE1OSNXS,       1,   0,   9,   1,   0,  1, WO, WRITEFN(tlbi_vmall))
    ARM64_CP_REG_DEFINE(TLBI VMALLS12E1,         1,   4,   8,   7,   6,  1, WO, WRITEFN(tlbi_vmall))
    ARM64_CP_REG_DEFINE(TLBI VMALLS12E1IS,       1,   4,   8,   3,   6,  1, WO, WRITEFN(tlbi_vmall))
    ARM64_CP_REG_DEFINE(TLBI VMALLS12E1ISNXS,    1,   4,   9,   3,   6,  1, WO, WRITEFN(tlbi_vmall))
    ARM64_CP_REG_DEFINE(TLBI VMALLS12E1NXS,      1,   4,   9,   7,   6,  1, WO, WRITEFN(tlbi_vmall))
    ARM64_CP_REG_DEFINE(TLBI VMALLS12E1OS,       1,   4,   8,   1,   6,  1, WO, WRITEFN(tlbi_vmall))
    ARM64_CP_REG_DEFINE(TLBI VMALLS12E1OSNXS,    1,   4,   9,   1,   6,  1, WO, WRITEFN(tlbi_vmall))
};

void cp_regs_add(CPUState *env, ARMCPRegInfo *reg_info_array, uint32_t array_count)
{
    for (int i = 0; i < array_count; i++) {
        ARMCPRegInfo *reg_info = &reg_info_array[i];
        uint32_t *key = tlib_malloc(sizeof(uint32_t));
        *key = ENCODE_AA64_CP_REG(reg_info->cp, reg_info->crn, reg_info->crm, reg_info->op0, reg_info->op1, reg_info->op2);

        if (!ttable_insert_check(env->arm_core_config->cp_regs, key, reg_info)) {
            tlib_abortf("Duplicated system_register definition!: name: %s, cp: %d, crn: %d, op1: %d, crm: %d, op2: %d, op0: %d",
                        reg_info->name, reg_info->cp, reg_info->crn, reg_info->op1, reg_info->crm, reg_info->op2, reg_info->op0);

            reg_info = ttable_lookup_value_eq(env->arm_core_config->cp_regs, key);
            tlib_abortf("Previously defined as!:                 name: %s, cp: %d, crn: %d, op1: %d, crm: %d, op2: %d, op0: %d",
                        reg_info->name, reg_info->cp, reg_info->crn, reg_info->op1, reg_info->crm, reg_info->op2, reg_info->op0);
        }
    }
}

/* Implementation defined registers.
 *
 * The 'op0' field is always 3 and 'crn' can only be either 11 or 15.
 */

ARMCPRegInfo cortex_a53_regs[] =
{
    // The params are:   name           op0, op1, crn, crm, op2, el, extra_type, ...
    ARM64_CP_REG_DEFINE(CBAR_EL1,        3,   1,  15,   3,   0,  1, RW)
    ARM64_CP_REG_DEFINE(CPUACTLR_EL1,    3,   1,  15,   2,   0,  1, RW)
    ARM64_CP_REG_DEFINE(CPUECTLR_EL1,    3,   1,  15,   2,   1,  1, RW)
    ARM64_CP_REG_DEFINE(CPUMERRSR_EL1,   3,   1,  15,   2,   2,  1, RW)
    ARM64_CP_REG_DEFINE(L2ACTLR_EL1,     3,   1,  15,   0,   0,  1, RW)
    ARM64_CP_REG_DEFINE(L2CTLR_EL1,      3,   1,  11,   0,   2,  1, RW)
    ARM64_CP_REG_DEFINE(L2ECTLR_EL1,     3,   1,  11,   0,   3,  1, RW)
    ARM64_CP_REG_DEFINE(L2MERRSR_EL1,    3,   1,  15,   2,   3,  1, RW)
};

ARMCPRegInfo cortex_a75_a76_common_regs[] =
{
    // Beware that register summaries in the manual have the 'op0' parameter
    // named 'copro' and the 'op1'-'crn' order is reversed.
    //
    // The params are:   name                   op0, op1, crn, crm, op2, el, extra_type, ...
    ARM64_CP_REG_DEFINE(CPUACTLR_EL1,            3,   0,  15,   1,   0,  1, RW)
    ARM64_CP_REG_DEFINE(CPUACTLR2_EL1,           3,   0,  15,   1,   1,  1, RW)
    ARM64_CP_REG_DEFINE(CPUCFR_EL1,              3,   0,  15,   0,   0,  1, RO)
    ARM64_CP_REG_DEFINE(CPUECTLR_EL1,            3,   0,  15,   1,   4,  1, RW)
    ARM64_CP_REG_DEFINE(CPUPCR_EL3,              3,   6,  15,   8,   1,  3, RW)
    ARM64_CP_REG_DEFINE(CPUPMR_EL3,              3,   6,  15,   8,   3,  3, RW)
    ARM64_CP_REG_DEFINE(CPUPOR_EL3,              3,   6,  15,   8,   2,  3, RW)
    ARM64_CP_REG_DEFINE(CPUPSELR_EL3,            3,   6,  15,   8,   0,  3, RW)
    ARM64_CP_REG_DEFINE(CPUPWRCTLR_EL1,          3,   0,  15,   2,   7,  1, RW)
    ARM64_CP_REG_DEFINE(ERXPFGCDNR_EL1,          3,   0,  15,   2,   2,  1, RW)
    ARM64_CP_REG_DEFINE(ERXPFGCTLR_EL1,          3,   0,  15,   2,   1,  1, RW)
    ARM64_CP_REG_DEFINE(ERXPFGFR_EL1,            3,   0,  15,   2,   0,  1, RW)

    // Cluster registers
    ARM64_CP_REG_DEFINE(CLUSTERACPSID_EL1,       3,   0,  15,   4,   1,  1, RW)
    ARM64_CP_REG_DEFINE(CLUSTERACTLR_EL1,        3,   0,  15,   3,   3,  1, RW)
    ARM64_CP_REG_DEFINE(CLUSTERBUSQOS_EL1,       3,   0,  15,   4,   4,  1, RW)
    ARM64_CP_REG_DEFINE(CLUSTERCFR_EL1,          3,   0,  15,   3,   0,  1, RW)
    ARM64_CP_REG_DEFINE(CLUSTERECTLR_EL1,        3,   0,  15,   3,   4,  1, RW)
    ARM64_CP_REG_DEFINE(CLUSTEREVIDR_EL1,        3,   0,  15,   3,   2,  1, RW)
    ARM64_CP_REG_DEFINE(CLUSTERIDR_EL1,          3,   0,  15,   3,   1,  1, RW)
    ARM64_CP_REG_DEFINE(CLUSTERL3HIT_EL1,        3,   0,  15,   4,   5,  1, RW)
    ARM64_CP_REG_DEFINE(CLUSTERL3MISS_EL1,       3,   0,  15,   4,   6,  1, RW)
    ARM64_CP_REG_DEFINE(CLUSTERPARTCR_EL1,       3,   0,  15,   4,   3,  1, RW)
    ARM64_CP_REG_DEFINE(CLUSTERPMCEID0_EL1,      3,   0,  15,   6,   4,  1, RW)
    ARM64_CP_REG_DEFINE(CLUSTERPMCEID1_EL1,      3,   0,  15,   6,   5,  1, RW)
    ARM64_CP_REG_DEFINE(CLUSTERPMCLAIMCLR_EL1,   3,   0,  15,   6,   7,  1, RW)
    ARM64_CP_REG_DEFINE(CLUSTERPMCLAIMSET_EL1,   3,   0,  15,   6,   6,  1, RW)
    ARM64_CP_REG_DEFINE(CLUSTERPMCNTENCLR_EL1,   3,   0,  15,   5,   2,  1, RW)
    ARM64_CP_REG_DEFINE(CLUSTERPMCNTENSET_EL1,   3,   0,  15,   5,   1,  1, RW)
    ARM64_CP_REG_DEFINE(CLUSTERPMCR_EL1,         3,   0,  15,   5,   0,  1, RW)
    ARM64_CP_REG_DEFINE(CLUSTERPMDBGCFG_EL1,     3,   0,  15,   6,   3,  1, RW)
    ARM64_CP_REG_DEFINE(CLUSTERPMINTENCLR_EL1,   3,   0,  15,   5,   7,  1, RW)
    ARM64_CP_REG_DEFINE(CLUSTERPMINTENSET_EL1,   3,   0,  15,   5,   6,  1, RW)
    ARM64_CP_REG_DEFINE(CLUSTERPMOVSCLR_EL1,     3,   0,  15,   5,   4,  1, RW)
    ARM64_CP_REG_DEFINE(CLUSTERPMOVSSET_EL1,     3,   0,  15,   5,   3,  1, RW)
    ARM64_CP_REG_DEFINE(CLUSTERPMSELR_EL1,       3,   0,  15,   5,   5,  1, RW)
    ARM64_CP_REG_DEFINE(CLUSTERPMXEVCNTR_EL1,    3,   0,  15,   6,   2,  1, RW)
    ARM64_CP_REG_DEFINE(CLUSTERPMXEVTYPER_EL1,   3,   0,  15,   6,   1,  1, RW)
    ARM64_CP_REG_DEFINE(CLUSTERPWRCTLR_EL1,      3,   0,  15,   3,   5,  1, RW)
    ARM64_CP_REG_DEFINE(CLUSTERPWRDN_EL1,        3,   0,  15,   3,   6,  1, RW)
    ARM64_CP_REG_DEFINE(CLUSTERPWRSTAT_EL1,      3,   0,  15,   3,   7,  1, RW)
    ARM64_CP_REG_DEFINE(CLUSTERSTASHSID_EL1,     3,   0,  15,   4,   2,  1, RW)
    ARM64_CP_REG_DEFINE(CLUSTERTHREADSID_EL1,    3,   0,  15,   4,   0,  1, RW)
};

ARMCPRegInfo cortex_a76_regs[] = {
    // Beware that register summaries in the manual have the 'op0' parameter
    // named 'copro' and the 'op1'-'crn' order is reversed.
    //
    // The params are:   name                   op0, op1, crn, crm, op2, el, extra_type, ...
    ARM64_CP_REG_DEFINE(ATCR_EL1,                3,   0,  15,   7,   0,  1, RW)
    ARM64_CP_REG_DEFINE(ATCR_EL12,               3,   5,  15,   7,   0,  2, RW)
    ARM64_CP_REG_DEFINE(ATCR_EL2,                3,   4,  15,   7,   0,  2, RW)
    ARM64_CP_REG_DEFINE(ATCR_EL3,                3,   6,  15,   7,   0,  3, RW)
    ARM64_CP_REG_DEFINE(AVTCR_EL2,               3,   4,  15,   7,   1,  2, RW)
    ARM64_CP_REG_DEFINE(CLUSTERTHREADSIDOVR_EL1, 3,   0,  15,   4,   7,  1, RW)
    ARM64_CP_REG_DEFINE(CPUACTLR3_EL1,           3,   0,  15,   1,   2,  1, RW)
};

void add_implementation_defined_registers(CPUState *env, uint32_t cpu_model_id)
{
    switch (cpu_model_id) {
    case ARM_CPUID_CORTEXA53:
        cp_regs_add(env, cortex_a53_regs, ARM_CP_ARRAY_COUNT(cortex_a53_regs));
        break;
    case ARM_CPUID_CORTEXA75:
        cp_regs_add(env, cortex_a75_a76_common_regs, ARM_CP_ARRAY_COUNT(cortex_a75_a76_common_regs));
        break;
    case ARM_CPUID_CORTEXA76:
        cp_regs_add(env, cortex_a75_a76_common_regs, ARM_CP_ARRAY_COUNT(cortex_a75_a76_common_regs));
        cp_regs_add(env, cortex_a76_regs, ARM_CP_ARRAY_COUNT(cortex_a76_regs));
        break;
    default:
        tlib_assert_not_reached();
    }
}

uint32_t get_implementation_defined_registers_count(uint32_t cpu_model_id)
{
    switch (cpu_model_id) {
    case ARM_CPUID_CORTEXA53:
        return ARM_CP_ARRAY_COUNT(cortex_a53_regs);
    case ARM_CPUID_CORTEXA75:
        return ARM_CP_ARRAY_COUNT(cortex_a75_a76_common_regs);
    case ARM_CPUID_CORTEXA76:
        return ARM_CP_ARRAY_COUNT(cortex_a75_a76_common_regs) + ARM_CP_ARRAY_COUNT(cortex_a76_regs);
    default:
        tlib_assert_not_reached();
    }
}

// The keys are dynamically allocated so let's make TTable free them when removing the entry.
void entry_remove_callback(TTable_entry *entry)
{
    tlib_free(entry->key);
}

void system_instructions_and_registers_init(CPUState *env, uint32_t cpu_model_id)
{
    uint32_t aarch64_instructions_count = ARM_CP_ARRAY_COUNT(aarch64_instructions);
    uint32_t aarch64_registers_count = ARM_CP_ARRAY_COUNT(aarch64_registers);
    uint32_t implementation_defined_registers_count = get_implementation_defined_registers_count(cpu_model_id);

    uint32_t ttable_size = aarch64_instructions_count + aarch64_registers_count + implementation_defined_registers_count;
    env->arm_core_config->cp_regs = ttable_create(ttable_size, entry_remove_callback, ttable_compare_key_uint32);

    cp_regs_add(env, aarch64_instructions, aarch64_instructions_count);
    cp_regs_add(env, aarch64_registers, aarch64_registers_count);
    add_implementation_defined_registers(env, cpu_model_id);
}

void system_instructions_and_registers_reset(CPUState *env)
{
    TTable *cp_regs = env->arm_core_config->cp_regs;

    int i;
    for (i = 0; i < cp_regs->count; i++) {
        ARMCPRegInfo *ri = cp_regs->entries[i].value;

        // Nothing to be done for these because:
        // * all the backing fields except the 'arm_core_config' ones are always reset to zero,
        // * CONSTs have no backing fields and 'resetvalue' is always used when they're read.
        if ((ri->resetvalue == 0) || (ri->type & ARM_CP_CONST)) {
            continue;
        }

        uint32_t width = ri->cp == CP_REG_ARM64_SYSREG_CP || (ri->type & ARM_CP_64BIT) ? 64 : 32;
        uint64_t value = width == 64 ? ri->resetvalue : ri->resetvalue & UINT32_MAX;

        tlib_printf(LOG_LEVEL_NOISY, "Resetting value for '%s': 0x%" PRIx64, ri->name, value);
        if (ri->fieldoffset) {
            memcpy((void *)env + ri->fieldoffset, &value, (size_t)(width / 8));
        } else if (ri->writefn) {
            ri->writefn(env, ri, value);
        } else {
            // Shouldn't happen so let's make sure it doesn't.
            tlib_assert_not_reached();
        }
    }
}
