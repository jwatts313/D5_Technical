/*
 * Copyright (c) Antmicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include "cpu.h"
#include "cpu_registers.h"
#include "../../unwind.h"

#ifdef TARGET_ARM64

uint64_t *get_reg_pointer_64(int reg)
{
    switch (reg) {
    case X_0_64 ... X_31_64:
        return &(cpu->xregs[reg - X_0_64]);
    case PC_64:
        return &(cpu->pc);
    default:
        return NULL;
    }
}

uint64_t tlib_get_register_value_64(int reg_number)
{
    // TODO: AArch64 to AArch32 mappings (R8_fiq == W24, SP_irq == W17 etc.):
    //       https://developer.arm.com/documentation/den0024/a/ARMv8-Registers/Changing-execution-state--again-/Registers-at-AArch32
    switch (reg_number) {
    case FPCR_32:
        return vfp_get_fpcr(cpu);
    case FPSR_32:
        return vfp_get_fpsr(cpu);
    // In GDB the register is named 'cpsr' for both AArch32 and AArch64. AArch64 formally
    // has neither CPSR nor PSTATE register but PSTATE just gathers fields like SPSR does.
    case PSTATE_32:
        if (is_a64(cpu)) {
            return pstate_read(cpu);
        } else {
            tlib_abortf("%s: CPSR read unimplemented", __func__);
        }
    }

    uint64_t *ptr = get_reg_pointer_64(reg_number);
    if (ptr == NULL) {
        tlib_abortf("Read from undefined CPU register number %d detected", reg_number);
    }

    return *ptr;
}

EXC_INT_1(uint64_t, tlib_get_register_value_64, int, reg_number)

void tlib_set_register_value_64(int reg_number, uint64_t value)
{
    switch (reg_number) {
    case FPCR_32:
        return vfp_set_fpcr(cpu, (uint32_t)value);
    case FPSR_32:
        return vfp_set_fpsr(cpu, (uint32_t)value);
    // In GDB the register is named 'cpsr' for both AArch32 and AArch64. AArch64 formally
    // has neither CPSR nor PSTATE register but PSTATE just gathers fields like SPSR does.
    case PSTATE_32:
        if (is_a64(cpu)) {
            pstate_write(cpu, (uint32_t)value);
        } else {
            tlib_abortf("%s: CPSR write unimplemented", __func__);
        }
        arm_rebuild_hflags(cpu);
        return;
    }

    uint64_t *ptr = get_reg_pointer_64(reg_number);
    if (ptr == NULL) {
        tlib_abortf("Write to undefined CPU register number %d detected", reg_number);
    }

    *ptr = value;
}

EXC_VOID_2(tlib_set_register_value_64, int, reg_number, uint64_t, value)

uint32_t tlib_get_register_value_32(int reg_number)
{
    return (uint32_t)tlib_get_register_value_64(reg_number);
}

EXC_INT_1(uint32_t, tlib_get_register_value_32, int, reg_number)

void tlib_set_register_value_32(int reg_number, uint32_t value)
{
    tlib_set_register_value_64(reg_number, value);
}

EXC_VOID_2(tlib_set_register_value_32, int, reg_number, uint32_t, value)

#endif // TARGET_ARM64
