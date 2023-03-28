/*
 * Copyright (c) Antmicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include "cpu.h"
#include "system_registers.h"
#include "../../unwind.h"

uint32_t tlib_get_current_el()
{
    return arm_current_el(cpu);
}
EXC_INT_0(uint32_t, tlib_get_current_el)

uint64_t tlib_get_system_register(const char *name)
{
    return sysreg_get_by_name(cpu, name);
}
EXC_INT_1(uint64_t, tlib_get_system_register, const char *, name)

uint32_t tlib_is_secure_below_el3()
{
    return arm_is_secure_below_el3(cpu);
}
EXC_INT_0(uint32_t, tlib_is_secure_below_el3)

void tlib_set_available_els(bool el2_enabled, bool el3_enabled)
{
    if (cpu->instructions_count_total_value != 0) {
        tlib_printf(LOG_LEVEL_WARNING, "Available Exception Levels can only be set before running the simulation.");
        return;
    }

    if (el2_enabled) {
        set_feature(cpu, ARM_FEATURE_EL2);
    } else {
        unset_feature(cpu, ARM_FEATURE_EL2);
    }

    if (el3_enabled) {
        set_feature(cpu, ARM_FEATURE_EL3);
    } else {
        unset_feature(cpu, ARM_FEATURE_EL3);
    }

    env->arm_core_config->has_el2 = el2_enabled;
    env->arm_core_config->has_el3 = el3_enabled;

    cpu_reset(cpu);
}
EXC_VOID_2(tlib_set_available_els, bool, el2_enabled, bool, el3_enabled)

void tlib_set_current_el(uint32_t el)
{
    pstate_set_el(cpu, el);
}
EXC_VOID_1(tlib_set_current_el, uint32_t, el)

void tlib_set_system_register(const char *name, uint64_t value)
{
    sysreg_set_by_name(cpu, name, value);
}
EXC_VOID_2(tlib_set_system_register, const char *, name, uint64_t, value)
