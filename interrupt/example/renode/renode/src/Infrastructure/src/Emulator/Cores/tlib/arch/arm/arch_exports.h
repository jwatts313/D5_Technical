#ifndef ARCH_EXPORTS_H_
#define ARCH_EXPORTS_H_

#include <stdint.h>

uint32_t tlib_get_cpu_id(void);
uint32_t tlib_get_it_state(void);
uint32_t tlib_evaluate_condition_code(uint32_t);

void tlib_set_cpu_id(uint32_t value);
void tlib_toggle_fpu(int32_t enabled);
void tlib_set_sev_on_pending(int32_t);
void tlib_set_event_flag(int value);

#ifdef TARGET_PROTO_ARM_M
void tlib_set_interrupt_vector_base(uint32_t address);
uint32_t tlib_get_interrupt_vector_base(void);
void tlib_set_fpu_interrupt_number(int32_t enabled);

uint32_t tlib_get_xpsr(void);
uint32_t tlib_get_fault_status(void);
void tlib_set_fault_status(uint32_t value);
uint32_t tlib_get_memory_fault_address(void);
void tlib_enable_mpu(int32_t enabled);
void tlib_set_mpu_region_base_address(uint32_t value);
void tlib_set_mpu_region_size_and_enable(uint32_t value);
void tlib_set_mpu_region_access_control(uint32_t value);
void tlib_set_mpu_region_number(uint32_t value);
uint32_t tlib_is_mpu_enabled();
uint32_t tlib_get_mpu_region_base_address();
uint32_t tlib_get_mpu_region_size_and_enable();
uint32_t tlib_get_mpu_region_access_control();
uint32_t tlib_get_mpu_region_number();
#endif

#endif
