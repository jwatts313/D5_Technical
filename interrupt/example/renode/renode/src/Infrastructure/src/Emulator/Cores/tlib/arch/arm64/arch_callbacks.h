/*
 * Copyright (c) Antmicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ARCH_CALLBACKS_H_
#define ARCH_CALLBACKS_H_

#include <stdint.h>

uint64_t tlib_read_system_register_interrupt_cpu_interface(uint32_t offset);
void tlib_write_system_register_interrupt_cpu_interface(uint32_t offset, uint64_t value);

uint64_t tlib_read_system_register_generic_timer(uint32_t offset);
void tlib_write_system_register_generic_timer(uint32_t offset, uint64_t value);

#endif
