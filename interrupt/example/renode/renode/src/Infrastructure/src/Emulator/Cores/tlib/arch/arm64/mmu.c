/*
 * Copyright (c) Antmicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cpu.h"
#include "mmu.h"
#include "syndrome.h"

const int ips_bits[] = { 32, 36, 40, 42, 44, 48, 52 };

/* Check section/page access permissions.
   Returns the page protection flags, or zero if the access is not
   permitted.  */
static inline int check_ap(int ap, int is_user)
{
    int prot = 0;

    switch (ap) {
    case 0:
        prot |= is_user ? 0 : PAGE_READ | PAGE_WRITE;
        break;
    case 1:
        prot |= PAGE_READ | PAGE_WRITE;
        break;
    case 2:
        prot |= is_user ? 0 : PAGE_READ;
        break;
    case 3:
        prot |= PAGE_READ;
        break;
    default:
        tlib_assert_not_reached();
    }

    return prot;
}

static uint64_t get_table_address(CPUState *env, target_ulong address, uint64_t base_addr, int page_size_shift, int level)
{
    uint64_t table_offset = 0;
    uint64_t mask = (1 << MMU_Ln_XLAT_VA_SIZE_SHIFT(page_size_shift)) - 1;
    table_offset = (address >> MMU_LEVEL_TO_VA_SIZE_SHIFT(level, page_size_shift)) & mask;

    return base_addr + (table_offset * 8);
}

static inline void parse_desc(uint32_t va_size_shift, uint64_t desc, uint32_t ips, target_ulong address, int is_user,
                              target_ulong *phys_ptr, int *prot, target_ulong *page_size)
{
    int ap, uxn, pxn;

    *phys_ptr = extract64(desc, va_size_shift, ips_bits[ips] - va_size_shift) << va_size_shift | extract64(address, 0,
                                                                                                           va_size_shift);

    tlib_printf(LOG_LEVEL_NOISY, "%s: phys_addr=0x%" PRIx64, __func__, *phys_ptr);

    ap = extract64(desc, 6, 2);
    uxn = extract64(desc, 54, 1);
    pxn = extract64(desc, 53, 1);

    *prot = check_ap(ap, is_user);
    if ((is_user && uxn == 0) || (!is_user && pxn == 0)) {
        *prot |= PAGE_EXEC;
    }

    *page_size = 1 << va_size_shift;
}

void handle_mmu_fault_v8(CPUState *env, target_ulong address, int access_type, bool suppress_faults,
                         SyndromeDataFaultStatusCode syn_dfsc, bool at_instruction_or_cache_maintenance,
                         bool s1ptw /* "stage 2 fault on an access made for a stage 1 translation table walk" */)
{
    // The 'suppress_faults' (AKA 'no_page_fault') argument can be used to skip translation failure
    // handling like it's done, e.g., in case of accessing UART's physical address directly from EL0.
    if (unlikely(suppress_faults)) {
        return;
    }

    uint32_t target_el = exception_target_el(env);
    bool same_el = target_el == arm_current_el(env);

    uint32_t exception_type;
    uint64_t syndrome;
    if (access_type == ACCESS_INST_FETCH) {
        exception_type = EXCP_PREFETCH_ABORT;

        // TODO: Set the remaining syndrome fields.
        SyndromeExceptionClass fetch_abort_ec = same_el ? SYN_EC_INSTRUCTION_ABORT_SAME_EL : SYN_EC_INSTRUCTION_ABORT_LOWER_EL;
        syn_set_ec(&syndrome, fetch_abort_ec);
    } else {
        exception_type = EXCP_DATA_ABORT;

        bool is_write = access_type == ACCESS_DATA_STORE;
        uint32_t wnr = is_write || at_instruction_or_cache_maintenance;

        // TODO: Get partial syndrome from insn_start params instead.
        syndrome = syn_data_abort_no_iss(same_el, 0, 0, at_instruction_or_cache_maintenance, s1ptw, wnr, syn_dfsc);

        if (!syndrome) {
            // TODO: Make sure an empty syndrome without 'suppress_faults' doesn't make sense here.
            tlib_assert_not_reached();
        }
        // Let's set the remaining fields if it's only a partial syndrome created during translation with 'syn_data_abort_with_iss'.
        else if (syn_get_ec(syndrome) == 0x0) {
            // TODO: SAME_EL should also be used for "Data Abort exceptions taken to EL2 as a result of accesses
            //       generated associated with VNCR_EL2 as part of nested virtualization support.".
            SyndromeExceptionClass data_abort_ec = same_el ? SYN_EC_DATA_ABORT_SAME_EL : SYN_EC_DATA_ABORT_LOWER_EL;
            syn_set_ec(&syndrome, data_abort_ec);

            syndrome = deposit32(syndrome, 6, 1, wnr);
            syndrome = deposit32(syndrome, 0, 6, syn_dfsc);
        }
    }

    env->exception.vaddress = address;
    raise_exception(env, exception_type, syndrome, target_el);
}

int get_phys_addr_v8(CPUState *env, target_ulong address, int access_type, int mmu_idx, target_ulong *phys_ptr, int *prot,
                     target_ulong *page_size, bool suppress_faults, bool at_instruction_or_cache_maintenance)
{
    ARMMMUIdx arm_mmu_idx = core_to_aa64_mmu_idx(mmu_idx);
    uint32_t current_el = arm_mmu_idx_to_el(arm_mmu_idx);

    uint64_t tcr = arm_tcr(env, current_el);
    uint64_t ttbr = 0;
    uint64_t tsz = 0;
    uint64_t tg = 0;
    uint32_t ips = 0;
    uint32_t page_size_shift = 0;

    // Check bit 55 to determine which TTBR to use
    // Bit 55 is used instead of checking all top bits above TxSZ, as bits >55
    // can be used for tagged pointers, and bit 55 is valid for all region sizes.
    if (extract64(address, 55, 1)) {
        ttbr = arm_ttbr1(env, current_el);
        tsz = extract64(tcr, 16, 6); // T1SZ
        tg =  extract64(tcr, 30, 2); // TG1
        switch (tg) {
        case 1:
            page_size_shift = 14; // 16kB
            break;
        case 2:
            page_size_shift = 12; // 4kB
            break;
        case 3:
            page_size_shift = 16; // 64kB
            break;
        default:
            tlib_abortf("Incorrect TG1 value: %d", tg);
            break;
        }
    } else {
        ttbr = arm_ttbr0(env, current_el);
        tsz = extract64(tcr, 0, 6);  // T0SZ
        tg =  extract64(tcr, 14, 2); // TG0
        switch (tg) {
        case 0:
            page_size_shift = 12; // 4kB
            break;
        case 1:
            page_size_shift = 16; // 64kB
            break;
        case 2:
            page_size_shift = 14; // 16kB
            break;
        default:
            tlib_abortf("Incorrect TG0 value: %d", tg);
            break;
        }
    }

    ips = (tcr >> 32) & 0x7;

    tlib_printf(LOG_LEVEL_NOISY, "%s: vaddr=0x%" PRIx64 " attbr=0x%" PRIx64 ", tsz=%d, tg=%d, page_size_shift=%d", __func__,
                address, ttbr, tsz, tg, page_size_shift);

    // Table address is low 48 bits of TTBR
    uint64_t table_addr = extract64(ttbr, 0, 48);

    for (int level = MMU_GET_BASE_XLAT_LEVEL(64 - tsz, page_size_shift); level <= MMU_XLAT_LAST_LEVEL; level++) {
        uint64_t desc_addr = get_table_address(env, address, table_addr, page_size_shift, level);
        uint64_t desc = ldq_phys(desc_addr);

        tlib_printf(LOG_LEVEL_NOISY, "%s: level=%d, desc=0x%" PRIx64 " (addr: 0x%" PRIx64 ")", __func__, level, desc, desc_addr);

        uint32_t desc_type = extract64(desc, 0, 2);
        switch (desc_type) {
        case DESCRIPTOR_TYPE_BLOCK_ENTRY:

            if (level == 1 && page_size_shift != 12) {
                tlib_printf(LOG_LEVEL_ERROR, "%s: block entry allowed on level 1 only with 4K pages!", __func__);
                return TRANSLATE_FAIL;
            }

            if (level > 2) {
                tlib_printf(LOG_LEVEL_ERROR, "%s: block descriptor not allowed on level %d!", __func__, level);
                return TRANSLATE_FAIL;
            }

            parse_desc(MMU_GET_XLAT_VA_SIZE_SHIFT(level, page_size_shift), desc, ips, address, current_el == 0, phys_ptr, prot,
                       page_size);
            return TRANSLATE_SUCCESS;

        case DESCRIPTOR_TYPE_TABLE_DESCRIPTOR_OR_ENTRY:
            if (level == 3) {
                parse_desc(MMU_GET_XLAT_VA_SIZE_SHIFT(level, page_size_shift), desc, ips, address, current_el == 0, phys_ptr,
                           prot, page_size);
                return TRANSLATE_SUCCESS;
            } else {
                table_addr = extract64(desc, page_size_shift, ips_bits[ips] - page_size_shift) << page_size_shift;
            }
            break;
        default:
            // It's debug because translation failures can be caused by a valid software behaviour.
            // For example Coreboot uses them to find out the memory size.
            tlib_printf(LOG_LEVEL_DEBUG, "%s: Invalid descriptor type %d!", __func__, desc_type);

            SyndromeDataFaultStatusCode syn_dfsc = SYN_DFSC_TRANSLATION_FAULT_LEVEL0 + level;
            handle_mmu_fault_v8(env, address, access_type, suppress_faults, syn_dfsc, at_instruction_or_cache_maintenance, false);
            return TRANSLATE_FAIL;
        }
    }
    return TRANSLATE_SUCCESS;
}
