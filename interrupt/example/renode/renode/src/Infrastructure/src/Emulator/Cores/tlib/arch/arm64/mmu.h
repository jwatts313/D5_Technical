// Header based on Zephyr ARM64 MMU implementation:
// https://github.com/zephyrproject-rtos/zephyr/blob/zephyr-v3.2.0/arch/arm64/core/mmu.h

#ifndef MMU_H_
#define MMU_H_

#define MMU_XLAT_LAST_LEVEL  3U

#define MMU_Ln_XLAT_VA_SIZE_SHIFT(page_size_shift)    (page_size_shift - 3)
#define MMU_L3_XLAT_VA_SIZE_SHIFT(page_size_shift)    page_size_shift
#define MMU_L2_XLAT_VA_SIZE_SHIFT(page_size_shift)    (MMU_L3_XLAT_VA_SIZE_SHIFT(page_size_shift) + MMU_Ln_XLAT_VA_SIZE_SHIFT(page_size_shift))
#define MMU_L1_XLAT_VA_SIZE_SHIFT(page_size_shift)    (MMU_L2_XLAT_VA_SIZE_SHIFT(page_size_shift) + MMU_Ln_XLAT_VA_SIZE_SHIFT(page_size_shift))
#define MMU_L0_XLAT_VA_SIZE_SHIFT(page_size_shift)    (MMU_L1_XLAT_VA_SIZE_SHIFT(page_size_shift) + MMU_Ln_XLAT_VA_SIZE_SHIFT(page_size_shift))

// The MMU_GET_XLAT_VA_SIZE_SHIFT macro returns VA size shift for given level/page size
// as per the table in https://developer.arm.com/documentation/101811/0102/Translation-granule
#define MMU_GET_XLAT_VA_SIZE_SHIFT(level, page_size_shift)       \
    ( (level == 3) ? MMU_L3_XLAT_VA_SIZE_SHIFT(page_size_shift)  \
    : (level == 2) ? MMU_L2_XLAT_VA_SIZE_SHIFT(page_size_shift)  \
    : (level == 1) ? MMU_L1_XLAT_VA_SIZE_SHIFT(page_size_shift)  \
    : MMU_L0_XLAT_VA_SIZE_SHIFT(page_size_shift))

#define MMU_GET_BASE_XLAT_LEVEL(va_bits, page_size_shift)                \
    ( (va_bits > MMU_L0_XLAT_VA_SIZE_SHIFT(page_size_shift)) ? 0U        \
    : (va_bits > MMU_L1_XLAT_VA_SIZE_SHIFT(page_size_shift)) ? 1U        \
    : (va_bits > MMU_L2_XLAT_VA_SIZE_SHIFT(page_size_shift)) ? 2U : 3U)

#define MMU_LEVEL_TO_VA_SIZE_SHIFT(level, page_size_shift)            \
    (page_size_shift + (MMU_Ln_XLAT_VA_SIZE_SHIFT(page_size_shift) *  \
    (MMU_XLAT_LAST_LEVEL - (level))))

enum
{
    DESCRIPTOR_TYPE_INVALID_0,
    DESCRIPTOR_TYPE_BLOCK_ENTRY,
    DESCRIPTOR_TYPE_INVALID_2,
    DESCRIPTOR_TYPE_TABLE_DESCRIPTOR_OR_ENTRY,
};

ARMMMUIdx get_current_arm_mmu_idx(CPUState *env);

static inline uint32_t address_translation_el(CPUState *env, uint32_t el)
{
    if(el == 0)
    {
        return arm_is_el2_enabled(env) && hcr_e2h_and_tge_set(env) ? 2 : 1;
    }
    return el;
}

static inline uint64_t arm_ttbr0(CPUState *env, int el)
{
    tlib_assert(el >= 0 && el <= 3);
    el = address_translation_el(env, el);
    return env->cp15.ttbr0_el[el];
}

static inline uint64_t arm_ttbr1(CPUState *env, int el)
{
    tlib_assert(el >= 0 && el <= 3);
    el = address_translation_el(env, el);
    return env->cp15.ttbr1_el[el];
}

static inline uint64_t arm_tcr(CPUState *env, int el)
{
    tlib_assert(el >= 0 && el <= 3);
    el = address_translation_el(env, el);
    return env->cp15.tcr_el[el];
}

#endif /* MMU_H_ */
