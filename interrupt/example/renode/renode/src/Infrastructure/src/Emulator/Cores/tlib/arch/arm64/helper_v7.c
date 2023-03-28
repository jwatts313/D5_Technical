#include "cpu.h"
#include "cpu_names.h"
#include "helper.h"
#include "syndrome.h"

/* All these helpers have been based on tlib's 'arm/helper.c'. */

// Exracted from cpu_reset.
void cpu_reset_vfp(CPUState *env)
{
    set_flush_to_zero(1, &env->vfp.standard_fp_status);
    set_flush_inputs_to_zero(1, &env->vfp.standard_fp_status);
    set_default_nan_mode(1, &env->vfp.standard_fp_status);
    set_float_detect_tininess(float_tininess_before_rounding, &env->vfp.fp_status);
    set_float_detect_tininess(float_tininess_before_rounding, &env->vfp.standard_fp_status);
}

/* return 0 if not found */
uint32_t cpu_arm_find_by_name(const char *name)
{
    int i;
    uint32_t id;

    id = 0;
    for (i = 0; arm_cpu_names[i].name; i++) {
        if (strcmp(name, arm_cpu_names[i].name) == 0) {
            id = arm_cpu_names[i].id;
            break;
        }
    }
    return id;
}

int cpu_init(const char *cpu_model)
{
    uint32_t id;

    id = cpu_arm_find_by_name(cpu_model);
    if (id == ARM_CPUID_NOT_FOUND) {
        tlib_printf(LOG_LEVEL_ERROR, "Unknown CPU model: %s", cpu_model);
        return -1;
    }
    env->cp15.c0_cpuid = id;

    cpu_init_v8(cpu, id);
    cpu_reset(cpu);
    return 0;
}

inline int get_phys_addr(CPUState *env, target_ulong address, int access_type, int mmu_idx, target_ulong *phys_ptr, int *prot,
                                target_ulong *page_size, int no_page_fault)
{
    if(unlikely(cpu->external_mmu_enabled))
    {
        return get_external_mmu_phys_addr(env, address, access_type, phys_ptr, prot, no_page_fault);
    }

    ARMMMUIdx arm_mmu_idx = core_to_aa64_mmu_idx(mmu_idx);
    uint32_t el = arm_mmu_idx_to_el(arm_mmu_idx);
    if ((arm_sctlr(env, el) & SCTLR_M) == 0) {
        /* MMU/MPU disabled.  */
        *phys_ptr = address;
        *prot = PAGE_READ | PAGE_WRITE | PAGE_EXEC;
        *page_size = TARGET_PAGE_SIZE;
        return TRANSLATE_SUCCESS;
    } else {
        return get_phys_addr_v8(env, address, access_type, mmu_idx, phys_ptr, prot, page_size, no_page_fault, false);
    }
}

target_phys_addr_t cpu_get_phys_page_debug(CPUState *env, target_ulong addr)
{
    target_ulong phys_addr = 0;
    target_ulong page_size = 0;
    int prot = 0;
    int ret;

    ret = get_phys_addr(env, addr, 0, 0, &phys_addr, &prot, &page_size, 1);

    if (ret != 0) {
        return -1;
    }

    return phys_addr;
}

// The name of the function is a little misleading. It doesn't handle MMU faults as much as TLB misses.
int cpu_handle_mmu_fault (CPUState *env, target_ulong address, int access_type, int mmu_idx, int no_page_fault)
{
    target_ulong phys_addr = 0;
    target_ulong page_size = 0;
    int prot = 0;
    int ret;

    ret = get_phys_addr(env, address, access_type, mmu_idx, &phys_addr, &prot, &page_size, no_page_fault);
    if (ret == TRANSLATE_SUCCESS) {
        /* Map a single [sub]page.  */
        phys_addr &= TARGET_PAGE_MASK;
        address &= TARGET_PAGE_MASK;
        tlb_set_page(env, address, phys_addr, prot, mmu_idx, page_size);
    }
    return ret;
}

/* try to fill the TLB and return an exception if error. If retaddr is
   NULL, it means that the function was called in C code (i.e. not
   from generated code or from helper.c) */
/* XXX: fix it to restore all registers */
int tlb_fill(CPUState *env1, target_ulong addr, int access_type, int mmu_idx, void *retaddr, int no_page_fault, int access_width)
{
    CPUState *saved_env;
    int ret;

    saved_env = env;
    env = env1;
    ret = cpu_handle_mmu_fault(env, addr, access_type, mmu_idx, no_page_fault);
    if (unlikely(ret == TRANSLATE_FAIL && !no_page_fault)) {
        // access_type == CODE ACCESS - do not fire block_end hooks!
        cpu_loop_exit_restore(env, (uintptr_t)retaddr, access_type != ACCESS_INST_FETCH);
    }
    env = saved_env;
    return ret;
}

/* Sign/zero extend */
uint32_t HELPER(sxtb16)(uint32_t x)
{
    uint32_t res;
    res = (uint16_t)(int8_t)x;
    res |= (uint32_t)(int8_t)(x >> 16) << 16;
    return res;
}

uint32_t HELPER(uxtb16)(uint32_t x)
{
    uint32_t res;
    res = (uint16_t)(uint8_t)x;
    res |= (uint32_t)(uint8_t)(x >> 16) << 16;
    return res;
}

// TODO: 'cpu_env' is the first argument now upstream. Why?
int32_t HELPER(sdiv)(int32_t num, int32_t den)
{
    if (den == 0) {
        return 0;
    }
    if (num == INT_MIN && den == -1) {
        return INT_MIN;
    }
    return num / den;
}

// TODO: 'cpu_env' is the first argument now upstream. Why?
uint32_t HELPER(udiv)(uint32_t num, uint32_t den)
{
    if (den == 0) {
        return 0;
    }
    return num / den;
}

uint32_t HELPER(rbit)(uint32_t x)
{
    x =  ((x & 0xff000000) >> 24) | ((x & 0x00ff0000) >> 8) | ((x & 0x0000ff00) << 8) | ((x & 0x000000ff) << 24);
    x =  ((x & 0xf0f0f0f0) >> 4) | ((x & 0x0f0f0f0f) << 4);
    x =  ((x & 0x88888888) >> 3) | ((x & 0x44444444) >> 1) | ((x & 0x22222222) << 1) | ((x & 0x11111111) << 3);
    return x;
}

static inline uint8_t do_usad(uint8_t a, uint8_t b)
{
    if (a > b) {
        return a - b;
    } else {
        return b - a;
    }
}

/* Unsigned sum of absolute byte differences.  */
uint32_t HELPER(usad8)(uint32_t a, uint32_t b)
{
    uint32_t sum;
    sum = do_usad(a, b);
    sum += do_usad(a >> 8, b >> 8);
    sum += do_usad(a >> 16, b >> 16);
    sum += do_usad(a >> 24, b >> 24);
    return sum;
}

/* For ARMv6 SEL instruction.  */
uint32_t HELPER(sel_flags)(uint32_t flags, uint32_t a, uint32_t b)
{
    uint32_t mask;

    mask = 0;
    if (flags & 1) {
        mask |= 0xff;
    }
    if (flags & 2) {
        mask |= 0xff00;
    }
    if (flags & 4) {
        mask |= 0xff0000;
    }
    if (flags & 8) {
        mask |= 0xff000000;
    }
    return (a & mask) | (b & ~mask);
}
