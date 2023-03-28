// STUB HELPERS

#define FUNC_STUB(name)         \
static inline int name()        \
{                               \
    return stub_abort(#name);   \
}

#define FUNC_STUB_GENERIC(name, type)   \
static inline type name()               \
{                                       \
    return (type)stub_abort(#name);     \
}

// Call directly for consts.
static inline int64_t stub_abort(char *name)
{
    tlib_abortf("Stub encountered: %s", name);
    return 0;
}

#define unimplemented(...) tlib_abortf("%s unimplemented", __func__); __builtin_unreachable()

// STUBS emitting warnings instead of aborting simulation (e.g. used by Linux).
// TODO: Implement properly.

#define ARMCoreConfig void
static inline bool arm_is_psci_call(ARMCoreConfig *cpu, uint32_t excp)
{
    // Let's assume false.
    tlib_printf(LOG_LEVEL_DEBUG, "Stub encountered: arm_is_psci_call(cpu, 0x%x); returning false", excp);
    return false;
}
#undef ARMCoreConfig

// STUBS

// These were declared in 'cpu.h' and used by non-skipped sources but we have no
// implementations. Remember to reenable their declarations after implementing.
FUNC_STUB(cpsr_read)
FUNC_STUB(cpsr_write)
FUNC_STUB(write_v7m_exception)

typedef struct MemTxAttrs
{
    int secure;
    int target_tlb_bit0;
    int target_tlb_bit1;
} MemTxAttrs;

#define HAVE_CMPXCHG128 stub_abort("HAVE_CMPXCHG128")
FUNC_STUB(cpu_atomic_cmpxchgo_be_mmu)
FUNC_STUB(cpu_atomic_cmpxchgo_le_mmu)
FUNC_STUB(probe_access)
FUNC_STUB_GENERIC(probe_write, void*)
FUNC_STUB_GENERIC(tlb_vaddr_to_host, void*)

#define MO_128 stub_abort("MO_128")

FUNC_STUB(aarch32_cpsr_valid_mask)

// Couldn't have been ported easily because of a complex softfloat licensing.
#define float_muladd_halve_result   stub_abort("float_muladd_halve_result")
#define float16_one_point_five      stub_abort("float16_one_point_five")
#define float16_three               stub_abort("float16_three")
#define float16_two                 stub_abort("float16_two")
#define float64_one_point_five      stub_abort("float64_one_point_five")
#define float64_three               stub_abort("float64_three")
#define float64_two                 stub_abort("float64_two")
FUNC_STUB(float16_abs)
FUNC_STUB(float16_add)
FUNC_STUB(float16_chs)
FUNC_STUB(float16_compare_quiet)
FUNC_STUB(float16_compare)
FUNC_STUB(float16_div)
FUNC_STUB(float16_is_any_nan)
FUNC_STUB(float16_is_infinity)
FUNC_STUB(float16_is_zero)
FUNC_STUB(float16_max)
FUNC_STUB(float16_maxnum)
FUNC_STUB(float16_min)
FUNC_STUB(float16_minnum)
FUNC_STUB(float16_mul)
FUNC_STUB(float16_muladd)
FUNC_STUB(float16_round_to_int)
FUNC_STUB(float16_silence_nan)
FUNC_STUB(float16_sqrt)
FUNC_STUB(float16_squash_input_denormal)
FUNC_STUB(float16_sub)
FUNC_STUB(float16_to_int16)
FUNC_STUB(float16_to_uint16)
FUNC_STUB(float32_silence_nan)
FUNC_STUB(float32_squash_input_denormal)
FUNC_STUB(float64_silence_nan)
FUNC_STUB(float64_squash_input_denormal)

FUNC_STUB(crc32c)

FUNC_STUB(helper_rebuild_hflags_a32)

FUNC_STUB(cpu_stb_mmuidx_ra)

FUNC_STUB(rol32)
FUNC_STUB(rol64)
FUNC_STUB(ror32)
FUNC_STUB(ror64)

/* mte_helper.c */

typedef int AddressSpace;
typedef int Error;
typedef int hwaddr;

typedef struct
{
    int addr;
    void *container;
} MemoryRegion;

typedef struct
{
    MemTxAttrs attrs;
} CPUIOTLBEntry;

typedef struct
{
    CPUIOTLBEntry *iotlb;
} env_tlb_d_struct;

typedef struct
{
    env_tlb_d_struct *d;
} env_tlb_struct;

FUNC_STUB_GENERIC(address_space_translate, void*)
FUNC_STUB(address_with_allocation_tag)
FUNC_STUB(allocation_tag_from_addr)
FUNC_STUB(arm_cpu_do_unaligned_access)
FUNC_STUB(cpu_check_watchpoint)
FUNC_STUB_GENERIC(cpu_get_address_space, void*)
FUNC_STUB(cpu_physical_memory_set_dirty_flag)
FUNC_STUB_GENERIC(env_tlb, env_tlb_struct*)
FUNC_STUB(error_free)
FUNC_STUB(error_get_pretty)
FUNC_STUB_GENERIC(memory_region_from_host, void*)
FUNC_STUB(memory_region_get_ram_addr)
FUNC_STUB_GENERIC(memory_region_get_ram_ptr, void*)
FUNC_STUB(memory_region_is_ram)
FUNC_STUB(probe_access_flags)
FUNC_STUB(qatomic_cmpxchg)
FUNC_STUB(qatomic_read)
FUNC_STUB(qatomic_set)
FUNC_STUB(qemu_guest_getrandom)
FUNC_STUB(regime_el)
FUNC_STUB(tbi_check)
FUNC_STUB(tcma_check)
FUNC_STUB(tlb_index)
FUNC_STUB(useronly_clean_ptr)

#define BP_MEM_READ             stub_abort("BP_MEM_READ")
#define BP_MEM_WRITE            stub_abort("BP_MEM_WRITE")
#define DIRTY_MEMORY_MIGRATION  stub_abort("DIRTY_MEMORY_MIGRATION")
#define LOG2_TAG_GRANULE        stub_abort("LOG2_TAG_GRANULE")
#define TAG_GRANULE             stub_abort("TAG_GRANULE")
#define TLB_WATCHPOINT          stub_abort("TLB_WATCHPOINT")

#define HWADDR_PRIx "d"

/* op_helper.c */


typedef struct
{
    int type;
} ARMMMUFaultInfo;


#define  ARMFault_AsyncExternal        stub_abort("ARMFault_AsyncExternal")
#define  BANK_USRSYS                   stub_abort("BANK_USRSYS")

FUNC_STUB(arm_call_el_change_hook)
FUNC_STUB(arm_call_pre_el_change_hook)
FUNC_STUB(arm_cpreg_in_idspace)
FUNC_STUB(arm_fi_to_lfsc)
FUNC_STUB(arm_fi_to_sfsc)
FUNC_STUB(bank_number)
FUNC_STUB(extended_addresses_enabled)
FUNC_STUB(r14_bank_number)
FUNC_STUB(syn_bxjtrap)
FUNC_STUB(v7m_sp_limit)

/* crypto_helper.c */

static const uint8_t AES_sbox[] = {}, AES_isbox[] = {}, AES_shifts[] = {}, AES_ishifts[] = {};

FUNC_STUB(MAKE_64BIT_MASK)
FUNC_STUB(simd_data)
FUNC_STUB(simd_maxsz)
FUNC_STUB(simd_oprsz)

/* vfp_helper.c */

typedef int FloatRelation;
typedef int FloatRoundMode;

FUNC_STUB(float16_to_float64)
FUNC_STUB(float16_to_int16_scalbn)
FUNC_STUB(float16_to_int32_round_to_zero)
FUNC_STUB(float16_to_int32_scalbn)
FUNC_STUB(float16_to_int32)
FUNC_STUB(float16_to_int64_scalbn)
FUNC_STUB(float16_to_uint16_scalbn)
FUNC_STUB(float16_to_uint32_round_to_zero)
FUNC_STUB(float16_to_uint32_scalbn)
FUNC_STUB(float16_to_uint32)
FUNC_STUB(float16_to_uint64_scalbn)
FUNC_STUB(float32_to_int16_scalbn)
FUNC_STUB(float32_to_int32_scalbn)
FUNC_STUB(float32_to_int64_scalbn)
FUNC_STUB(float32_to_uint16_scalbn)
FUNC_STUB(float32_to_uint32_scalbn)
FUNC_STUB(float32_to_uint64_scalbn)
FUNC_STUB(float64_max)
FUNC_STUB(float64_min)
FUNC_STUB(float64_set_sign)
FUNC_STUB(float64_to_float16)
FUNC_STUB(float64_to_int16_scalbn)
FUNC_STUB(float64_to_int32_scalbn)
FUNC_STUB(float64_to_int64_scalbn)
FUNC_STUB(float64_to_uint16_scalbn)
FUNC_STUB(float64_to_uint32_scalbn)
FUNC_STUB(float64_to_uint64_scalbn)
FUNC_STUB(get_float_rounding_mode)
FUNC_STUB(get_flush_inputs_to_zero)
FUNC_STUB(get_flush_to_zero)
FUNC_STUB(int16_to_float16_scalbn)
FUNC_STUB(int16_to_float32_scalbn)
FUNC_STUB(int16_to_float64_scalbn)
FUNC_STUB(int32_to_float16_scalbn)
FUNC_STUB(int32_to_float32_scalbn)
FUNC_STUB(int32_to_float64_scalbn)
FUNC_STUB(int64_to_float16_scalbn)
FUNC_STUB(int64_to_float32_scalbn)
FUNC_STUB(int64_to_float64_scalbn)
FUNC_STUB(uint16_to_float16_scalbn)
FUNC_STUB(uint16_to_float32_scalbn)
FUNC_STUB(uint16_to_float64_scalbn)
FUNC_STUB(uint32_to_float16_scalbn)
FUNC_STUB(uint32_to_float32_scalbn)
FUNC_STUB(uint32_to_float64_scalbn)
FUNC_STUB(uint64_to_float16_scalbn)
FUNC_STUB(uint64_to_float32_scalbn)
FUNC_STUB(uint64_to_float64_scalbn)

/* vec_helper.c */

typedef int bfloat16;

#define  float16_zero            stub_abort("float16_zero")
#define  float_round_to_odd_inf  stub_abort("float_round_to_odd_inf")
#define  SIMD_DATA_SHIFT         stub_abort("SIMD_DATA_SHIFT")

FUNC_STUB(float16_eq_quiet)
FUNC_STUB(float16_le)
FUNC_STUB(float16_lt)
FUNC_STUB(float16_set_sign)
FUNC_STUB(float16_to_int16_round_to_zero)
FUNC_STUB(float16_to_uint16_round_to_zero)
FUNC_STUB(int16_to_float16)
FUNC_STUB(uint16_to_float16)

/* sve_helper.c */

#define  float16_infinity   stub_abort("float16_infinity")
#define  float16_one        stub_abort("float16_one")
#define  SVE_MTEDESC_SHIFT  stub_abort("SVE_MTEDESC_SHIFT")

FUNC_STUB(cpu_ldl_be_data_ra)
FUNC_STUB(cpu_ldl_le_data_ra)
FUNC_STUB(cpu_ldq_be_data_ra)
FUNC_STUB(cpu_ldq_le_data_ra)
FUNC_STUB(cpu_ldub_data_ra)
FUNC_STUB(cpu_lduw_be_data_ra)
FUNC_STUB(cpu_lduw_le_data_ra)
FUNC_STUB(cpu_stb_data_ra)
FUNC_STUB(cpu_stl_be_data_ra)
FUNC_STUB(cpu_stl_le_data_ra)
FUNC_STUB(cpu_stq_be_data_ra)
FUNC_STUB(cpu_stq_le_data_ra)
FUNC_STUB(cpu_stw_be_data_ra)
FUNC_STUB(cpu_stw_le_data_ra)
FUNC_STUB(cpu_watchpoint_address_matches)
FUNC_STUB(ctpop16)
FUNC_STUB(ctpop32)
FUNC_STUB(ctpop8)
FUNC_STUB(dup_const)
FUNC_STUB(float16_is_neg)
FUNC_STUB(float16_scalbn)
FUNC_STUB(float16_to_int64_round_to_zero)
FUNC_STUB(float16_to_uint64_round_to_zero)
FUNC_STUB(float32_to_bfloat16)
FUNC_STUB(float32_to_uint64_round_to_zero)
FUNC_STUB(hswap32)
FUNC_STUB(hswap64)
FUNC_STUB(int32_to_float16)
FUNC_STUB(int64_to_float16)
FUNC_STUB(pow2floor)
FUNC_STUB(uint32_to_float16)
FUNC_STUB(uint64_to_float16)
FUNC_STUB(wswap64)

/* translate-a64.c */

typedef void gen_helper_gvec_2;
typedef void gen_helper_gvec_3;
typedef void gen_helper_gvec_3_ptr;
typedef void gen_helper_gvec_4;
typedef void gen_helper_gvec_4_ptr;
typedef struct
{
    void *fni4;
    void *fni8;
    void *fniv;
    const void *opt_opc;
    void *fno;
    bool prefer_i64;
    int vece;
    bool load_dest;
} GVecGen3;
typedef int TCGBar;
typedef int TCGOp;
typedef int TCGv_vec;
typedef struct
{
    void *init_disas_context;
    void *tb_start;
    void *insn_start;
    void *translate_insn;
    void *tb_stop;
    void *disas_log;
} TranslatorOps;

#define container_of(var, type, base) (type*)(var)

#define  INDEX_op_rotli_vec                     0
#define  R_SVCR_SM_MASK                         stub_abort("R_SVCR_SM_MASK")
#define  R_SVCR_ZA_MASK                         stub_abort("R_SVCR_ZA_MASK")
#define  SME_ET_AccessTrap                      stub_abort("SME_ET_AccessTrap")
#define  SME_ET_InactiveZA                      stub_abort("SME_ET_InactiveZA")
#define  SME_ET_NotStreaming                    stub_abort("SME_ET_NotStreaming")
#define  SME_ET_Streaming                       stub_abort("SME_ET_Streaming")
FUNC_STUB_GENERIC(tcg_gen_gvec_abs, void)
FUNC_STUB_GENERIC(tcg_gen_gvec_add, void)
FUNC_STUB_GENERIC(tcg_gen_gvec_andc, void)
FUNC_STUB_GENERIC(tcg_gen_gvec_and, void)
FUNC_STUB_GENERIC(tcg_gen_gvec_bitsel, void)
FUNC_STUB_GENERIC(tcg_gen_gvec_mul, void)
FUNC_STUB_GENERIC(tcg_gen_gvec_neg, void)
FUNC_STUB_GENERIC(tcg_gen_gvec_not, void)
FUNC_STUB_GENERIC(tcg_gen_gvec_orc, void)
FUNC_STUB_GENERIC(tcg_gen_gvec_or, void)
FUNC_STUB_GENERIC(tcg_gen_gvec_sari, void)
FUNC_STUB_GENERIC(tcg_gen_gvec_shli, void)
FUNC_STUB_GENERIC(tcg_gen_gvec_shri, void)
FUNC_STUB_GENERIC(tcg_gen_gvec_smax, void)
FUNC_STUB_GENERIC(tcg_gen_gvec_smin, void)
FUNC_STUB_GENERIC(tcg_gen_gvec_sub, void)
FUNC_STUB_GENERIC(tcg_gen_gvec_umax, void)
FUNC_STUB_GENERIC(tcg_gen_gvec_umin, void)
FUNC_STUB_GENERIC(tcg_gen_gvec_xor, void)
FUNC_STUB_GENERIC(tcg_gen_gvec_ori, void)
FUNC_STUB_GENERIC(tcg_gen_gvec_andi, void)
FUNC_STUB_GENERIC(tcg_gen_atomic_fetch_add_i64, void)
FUNC_STUB_GENERIC(tcg_gen_atomic_fetch_and_i64, void)
FUNC_STUB_GENERIC(tcg_gen_atomic_fetch_or_i64, void)
FUNC_STUB_GENERIC(tcg_gen_atomic_fetch_smax_i64, void)
FUNC_STUB_GENERIC(tcg_gen_atomic_fetch_smin_i64, void)
FUNC_STUB_GENERIC(tcg_gen_atomic_fetch_umax_i64, void)
FUNC_STUB_GENERIC(tcg_gen_atomic_fetch_umin_i64, void)
FUNC_STUB_GENERIC(tcg_gen_atomic_fetch_xor_i64, void)
FUNC_STUB_GENERIC(tcg_gen_atomic_xchg_i64, void)
FUNC_STUB_GENERIC(tcg_gen_gvec_mov, void)
FUNC_STUB(arm_cpreg_encoding_in_idspace)
FUNC_STUB(gen_io_start)
FUNC_STUB_GENERIC(get_arm_cp_reginfo, void*)
FUNC_STUB(semihosting_enabled)
FUNC_STUB(syn_fp_access_trap)
FUNC_STUB(target_disas)
FUNC_STUB(tcg_gen_gvec_2_ool)
FUNC_STUB(tcg_gen_gvec_2_ptr)
FUNC_STUB(tcg_gen_gvec_3)
FUNC_STUB(tcg_gen_gvec_3_ool)
FUNC_STUB(tcg_gen_gvec_3_ptr)
FUNC_STUB(tcg_gen_gvec_4_ool)
FUNC_STUB(tcg_gen_gvec_4_ptr)
FUNC_STUB(tcg_gen_gvec_cmp)
FUNC_STUB(tcg_gen_gvec_dup_i64)
FUNC_STUB(tcg_gen_gvec_dup_imm)
FUNC_STUB(tcg_gen_gvec_dup_mem)
FUNC_STUB(tcg_gen_rotli_vec)
FUNC_STUB(tcg_gen_xor_vec)
FUNC_STUB_GENERIC(tcg_last_op, void *)
FUNC_STUB(tcg_set_insn_start_param)
FUNC_STUB_GENERIC(tlb_entry, void *)
FUNC_STUB(tlb_hit)
FUNC_STUB(translator_ldl_swap)
FUNC_STUB(translator_lduw_swap)
FUNC_STUB(translator_loop_temp_check)
FUNC_STUB(translator_use_goto_tb)
FUNC_STUB_GENERIC(gen_helper_crypto_sm3tt2b, void)
FUNC_STUB(gen_helper_autda)
FUNC_STUB(gen_helper_autdb)
FUNC_STUB(gen_helper_autia)
FUNC_STUB(gen_helper_autib)
FUNC_STUB(gen_helper_exception_pc_alignment)
FUNC_STUB(gen_helper_exception_swstep)
FUNC_STUB(gen_helper_exit_atomic)
FUNC_STUB(gen_helper_neon_acge_f64)
FUNC_STUB(gen_helper_neon_acgt_f64)
FUNC_STUB(gen_helper_neon_qabs_s64)
FUNC_STUB(gen_helper_neon_qneg_s64)
FUNC_STUB(gen_helper_neon_rbit_u8)
FUNC_STUB(gen_helper_neon_sqadd_u16)
FUNC_STUB(gen_helper_neon_sqadd_u32)
FUNC_STUB(gen_helper_neon_sqadd_u64)
FUNC_STUB(gen_helper_neon_sqadd_u8)
FUNC_STUB(gen_helper_neon_uqadd_s16)
FUNC_STUB(gen_helper_neon_uqadd_s32)
FUNC_STUB(gen_helper_neon_uqadd_s64)
FUNC_STUB(gen_helper_neon_uqadd_s8)
FUNC_STUB(gen_helper_pacda)
FUNC_STUB(gen_helper_pacdb)
FUNC_STUB(gen_helper_pacga)
FUNC_STUB(gen_helper_pacia)
FUNC_STUB(gen_helper_pacib)
FUNC_STUB(gen_helper_set_pstate_sm)
FUNC_STUB(gen_helper_set_pstate_za)
FUNC_STUB(gen_helper_xpacd)
FUNC_STUB(gen_helper_xpaci)

/* translate.c */

typedef struct
{
    void *fno;
    void *fni4;
    void *fni8;
    void *fniv;
    const void *opt_opc;
    bool prefer_i64;
    int vece;
} GVecGen2;
typedef struct
{
    void *fno;
    void *fni4;
    void (*fni8)();
    void *fniv;
    const void *opt_opc;
    bool load_dest;
    bool prefer_i64;
    int vece;
} GVecGen2i;
typedef struct
{
    void *fno;
    void *fni4;
    void (*fni8)();
    void *fniv;
    const void *opt_opc;
    bool prefer_i64;
    int vece;
    bool write_aofs;
} GVecGen4;

#define  ARM_CP_NEWEL                 stub_abort("ARM_CP_NEWEL")
#define  EXC_RETURN_MIN_MAGIC         stub_abort("EXC_RETURN_MIN_MAGIC")
#define  FNC_RETURN_MIN_MAGIC         stub_abort("FNC_RETURN_MIN_MAGIC")
#define  INDEX_op_add_vec             0
#define  INDEX_op_cmpsel_vec          0
#define  INDEX_op_cmp_vec             0
#define  INDEX_op_mul_vec             0
#define  INDEX_op_neg_vec             0
#define  INDEX_op_sari_vec            0
#define  INDEX_op_sarv_vec            0
#define  INDEX_op_shli_vec            0
#define  INDEX_op_shlv_vec            0
#define  INDEX_op_shri_vec            0
#define  INDEX_op_shrv_vec            0
#define  INDEX_op_smax_vec            0
#define  INDEX_op_smin_vec            0
#define  INDEX_op_ssadd_vec           0
#define  INDEX_op_sssub_vec           0
#define  INDEX_op_sub_vec             0
#define  INDEX_op_umax_vec            0
#define  INDEX_op_umin_vec            0
#define  INDEX_op_usadd_vec           0
#define  INDEX_op_ussub_vec           0
#define  TCG_TARGET_HAS_add2_i32      0  // TODO: Port add2_i32 from TCG

FUNC_STUB(core_to_arm_mmu_idx)
FUNC_STUB(regime_is_secure)
FUNC_STUB(syn_aa32_bkpt) // aarch32
FUNC_STUB(syn_aa32_hvc) // aarch32
FUNC_STUB(syn_aa32_smc) // aarch32
FUNC_STUB(syn_aa32_svc) // aarch32
FUNC_STUB(syn_cp14_rrt_trap) // aarch32
FUNC_STUB(syn_cp14_rt_trap) // aarch32
FUNC_STUB(syn_cp15_rrt_trap) // aarch32
FUNC_STUB(syn_cp15_rt_trap) // aarch32
FUNC_STUB(tcg_constant_vec_matching)
FUNC_STUB(tcg_gen_add_vec)
FUNC_STUB(tcg_gen_andc_vec)
FUNC_STUB(tcg_gen_and_vec)
FUNC_STUB(tcg_gen_atomic_xchg_i32)
FUNC_STUB(tcg_gen_cmpsel_vec)
FUNC_STUB(tcg_gen_cmp_vec)
FUNC_STUB(tcg_gen_dupi_vec)
FUNC_STUB(tcg_gen_gvec_2)
FUNC_STUB(tcg_gen_gvec_2i)
FUNC_STUB(tcg_gen_gvec_4)
FUNC_STUB(tcg_gen_mov_vec)
FUNC_STUB(tcg_gen_mul_vec)
FUNC_STUB(tcg_gen_neg_vec)
FUNC_STUB(tcg_gen_or_vec)
FUNC_STUB(tcg_gen_sari_vec)
FUNC_STUB(tcg_gen_sarv_vec)
FUNC_STUB(tcg_gen_shli_vec)
FUNC_STUB(tcg_gen_shlv_vec)
FUNC_STUB(tcg_gen_shri_vec)
FUNC_STUB(tcg_gen_shrv_vec)
FUNC_STUB(tcg_gen_smax_vec)
FUNC_STUB(tcg_gen_smin_vec)
FUNC_STUB(tcg_gen_ssadd_vec)
FUNC_STUB(tcg_gen_sssub_vec)
FUNC_STUB(tcg_gen_sub_vec)
FUNC_STUB(tcg_gen_umax_vec)
FUNC_STUB(tcg_gen_umin_vec)
FUNC_STUB(tcg_gen_usadd_vec)
FUNC_STUB(tcg_gen_ussub_vec)
FUNC_STUB(tcg_gen_vec_add16_i64)
FUNC_STUB(tcg_gen_vec_add8_i64)
FUNC_STUB(tcg_gen_vec_sar16i_i64)
FUNC_STUB(tcg_gen_vec_sar8i_i64)
FUNC_STUB(tcg_gen_vec_shr16i_i64)
FUNC_STUB(tcg_gen_vec_shr8i_i64)
FUNC_STUB(tcg_remove_ops_after)
FUNC_STUB(tcg_temp_free_vec)
FUNC_STUB(tcg_temp_new_vec_matching)
FUNC_STUB(translator_loop)

/* translate.c */


FUNC_STUB_GENERIC(gen_helper_mve_sqrshrl48, void)
FUNC_STUB_GENERIC(gen_helper_mve_sqrshrl, void)
FUNC_STUB_GENERIC(gen_helper_mve_sqrshr, void)
FUNC_STUB_GENERIC(gen_helper_mve_sshrl, void)
FUNC_STUB_GENERIC(gen_helper_mve_uqrshll48, void)
FUNC_STUB_GENERIC(gen_helper_mve_uqrshll, void)
FUNC_STUB_GENERIC(gen_helper_mve_uqrshl, void)
FUNC_STUB_GENERIC(gen_helper_mve_ushll, void)
FUNC_STUB_GENERIC(gen_helper_qadd16, void)
FUNC_STUB_GENERIC(gen_helper_qadd8, void)
FUNC_STUB_GENERIC(gen_helper_qaddsubx, void)
FUNC_STUB_GENERIC(gen_helper_qsub16, void)
FUNC_STUB_GENERIC(gen_helper_qsub8, void)
FUNC_STUB_GENERIC(gen_helper_qsubaddx, void)
FUNC_STUB_GENERIC(gen_helper_sadd16, void)
FUNC_STUB_GENERIC(gen_helper_sadd8, void)
FUNC_STUB_GENERIC(gen_helper_saddsubx, void)
FUNC_STUB_GENERIC(gen_helper_shadd16, void)
FUNC_STUB_GENERIC(gen_helper_shadd8, void)
FUNC_STUB_GENERIC(gen_helper_shaddsubx, void)
FUNC_STUB_GENERIC(gen_helper_shsub16, void)
FUNC_STUB_GENERIC(gen_helper_shsub8, void)
FUNC_STUB_GENERIC(gen_helper_shsubaddx, void)
FUNC_STUB_GENERIC(gen_helper_ssub16, void)
FUNC_STUB_GENERIC(gen_helper_ssub8, void)
FUNC_STUB_GENERIC(gen_helper_ssubaddx, void)
FUNC_STUB_GENERIC(gen_helper_uadd16, void)
FUNC_STUB_GENERIC(gen_helper_uadd8, void)
FUNC_STUB_GENERIC(gen_helper_uaddsubx, void)
FUNC_STUB_GENERIC(gen_helper_uhadd16, void)
FUNC_STUB_GENERIC(gen_helper_uhadd8, void)
FUNC_STUB_GENERIC(gen_helper_uhaddsubx, void)
FUNC_STUB_GENERIC(gen_helper_uhsub16, void)
FUNC_STUB_GENERIC(gen_helper_uhsub8, void)
FUNC_STUB_GENERIC(gen_helper_uhsubaddx, void)
FUNC_STUB_GENERIC(gen_helper_uqadd16, void)
FUNC_STUB_GENERIC(gen_helper_uqadd8, void)
FUNC_STUB_GENERIC(gen_helper_uqaddsubx, void)
FUNC_STUB_GENERIC(gen_helper_uqsub16, void)
FUNC_STUB_GENERIC(gen_helper_uqsub8, void)
FUNC_STUB_GENERIC(gen_helper_uqsubaddx, void)
FUNC_STUB_GENERIC(gen_helper_usub16, void)
FUNC_STUB_GENERIC(gen_helper_usub8, void)
FUNC_STUB_GENERIC(gen_helper_usubaddx, void)

FUNC_STUB(gen_helper_crc32)
FUNC_STUB(gen_helper_crc32c)
FUNC_STUB(gen_helper_mve_sqshl)
FUNC_STUB(gen_helper_mve_sqshll)
FUNC_STUB(gen_helper_mve_uqshl)
FUNC_STUB(gen_helper_mve_uqshll)
FUNC_STUB(gen_helper_mve_vctp)
FUNC_STUB(gen_helper_rebuild_hflags_a32)
FUNC_STUB(gen_helper_rebuild_hflags_a32_newel)
FUNC_STUB(gen_helper_rebuild_hflags_m32)
FUNC_STUB(gen_helper_rebuild_hflags_m32_newel)
FUNC_STUB(gen_helper_v7m_blxns)
FUNC_STUB(gen_helper_v7m_bxns)
FUNC_STUB(gen_helper_v7m_mrs)
FUNC_STUB(gen_helper_v7m_msr)
FUNC_STUB(gen_helper_v7m_tt)

/* translate-neon.c */

typedef void gen_helper_gvec_2_ptr;

FUNC_STUB_GENERIC(gen_helper_gvec_fceq0_h, void)
FUNC_STUB_GENERIC(gen_helper_gvec_fcge0_h, void)
FUNC_STUB_GENERIC(gen_helper_gvec_fcgt0_h, void)
FUNC_STUB_GENERIC(gen_helper_gvec_fcle0_h, void)
FUNC_STUB_GENERIC(gen_helper_gvec_fclt0_h, void)

FUNC_STUB(tcg_gen_gvec_dup_i32)
FUNC_STUB_GENERIC(tcg_gen_gvec_xori, void)

/* translate-sve.c */

typedef void gen_helper_gvec_2i;
typedef void gen_helper_gvec_5;
typedef void gen_helper_gvec_5_ptr;
typedef int GVecGen2s;
typedef int GVecGen3i;

#define  float16_half               stub_abort("float16_half")
#define  float_round_to_odd         stub_abort("float_round_to_odd")
FUNC_STUB_GENERIC(tcg_gen_gvec_addi, void)
FUNC_STUB_GENERIC(tcg_gen_gvec_adds, void)
FUNC_STUB_GENERIC(tcg_gen_gvec_muli, void)
FUNC_STUB_GENERIC(tcg_gen_gvec_ssadd, void)
FUNC_STUB_GENERIC(tcg_gen_gvec_sssub, void)
FUNC_STUB_GENERIC(tcg_gen_gvec_subs, void)
FUNC_STUB_GENERIC(tcg_gen_gvec_usadd, void)
FUNC_STUB_GENERIC(tcg_gen_gvec_ussub, void)
FUNC_STUB_GENERIC(tcg_gen_vec_sub16_i64, void)
FUNC_STUB_GENERIC(tcg_gen_vec_sub8_i64, void)
#define  TCG_TARGET_HAS_bitsel_vec  0  // TODO: Port bitsel_vec from TCG

FUNC_STUB(is_power_of_2)
FUNC_STUB(pow2ceil)
FUNC_STUB(simd_desc)
FUNC_STUB(tcg_const_local_ptr)
FUNC_STUB(tcg_gen_bitsel_vec)
FUNC_STUB(tcg_gen_brcondi_ptr)
FUNC_STUB(tcg_gen_ctpop_i64)
FUNC_STUB(tcg_gen_gvec_2i_ool)
FUNC_STUB(tcg_gen_gvec_2s)
FUNC_STUB(tcg_gen_gvec_3i)
FUNC_STUB(tcg_gen_gvec_5_ool)
FUNC_STUB(tcg_gen_gvec_5_ptr)
FUNC_STUB(tcg_gen_gvec_ands)
FUNC_STUB(tcg_gen_mov_ptr)
FUNC_STUB(tcg_gen_not_vec)
FUNC_STUB(tcg_gen_orc_vec)
FUNC_STUB(tcg_gen_rotri_vec)
FUNC_STUB(tcg_gen_trunc_i64_ptr)
FUNC_STUB(tcg_temp_local_new_ptr)

/* translate-vfp.c */

#define  ECI_A0A1A2B0             stub_abort("ECI_A0A1A2B0")
#define  ECI_A0A1A2               stub_abort("ECI_A0A1A2")
#define  ECI_A0A1                 stub_abort("ECI_A0A1")
#define  ECI_A0                   stub_abort("ECI_A0")
#define  ECI_NONE                 stub_abort("ECI_NONE")
#define  R_V7M_CONTROL_FPCA_MASK  stub_abort("R_V7M_CONTROL_FPCA_MASK")
#define  R_V7M_CONTROL_SFPA_MASK  stub_abort("R_V7M_CONTROL_SFPA_MASK")
#define  R_V7M_FPCCR_S_MASK       stub_abort("R_V7M_FPCCR_S_MASK")

FUNC_STUB(gen_helper_v7m_preserve_fp_state)

// In 'helper.c' there are additional stubs for functions declared but unimplemented.

/* decode-sve.c.inc included in translate-sve.c */

FUNC_STUB(trans_FADD_zpzi)
FUNC_STUB(trans_FSUB_zpzi)
FUNC_STUB(trans_FMUL_zpzi)
FUNC_STUB(trans_FSUBR_zpzi)
FUNC_STUB(trans_FMAXNM_zpzi)
FUNC_STUB(trans_FMINNM_zpzi)
FUNC_STUB(trans_FMAX_zpzi)
FUNC_STUB(trans_FMIN_zpzi)
FUNC_STUB(trans_SSHLLB)
FUNC_STUB(trans_SSHLLT)
FUNC_STUB(trans_USHLLB)
FUNC_STUB(trans_USHLLT)
FUNC_STUB(trans_SQXTNB)
FUNC_STUB(trans_SQXTNT)
FUNC_STUB(trans_UQXTNB)
FUNC_STUB(trans_UQXTNT)
FUNC_STUB(trans_SQXTUNB)
FUNC_STUB(trans_SQXTUNT)
FUNC_STUB(trans_SQSHRUNB)
FUNC_STUB(trans_SQSHRUNT)
FUNC_STUB(trans_SHRNB)
FUNC_STUB(trans_SHRNT)
FUNC_STUB(trans_SQSHRNB)
FUNC_STUB(trans_SQSHRNT)
FUNC_STUB(trans_UQSHRNB)
FUNC_STUB(trans_UQSHRNT)

/* mte_helper.c */


#define  __REGISTER_MTEDESC_MIDX_START    stub_abort("__REGISTER_MTEDESC_MIDX_START")
#define  __REGISTER_MTEDESC_MIDX_WIDTH    stub_abort("__REGISTER_MTEDESC_MIDX_WIDTH")
#define  __REGISTER_MTEDESC_SIZEM1_START  stub_abort("__REGISTER_MTEDESC_SIZEM1_START")
#define  __REGISTER_MTEDESC_SIZEM1_WIDTH  stub_abort("__REGISTER_MTEDESC_SIZEM1_WIDTH")
#define  __REGISTER_MTEDESC_WRITE_START   stub_abort("__REGISTER_MTEDESC_WRITE_START")
#define  __REGISTER_MTEDESC_WRITE_WIDTH   stub_abort("__REGISTER_MTEDESC_WRITE_WIDTH")


/* sve_helper.c */


#define  __REGISTER_PREDDESC_DATA_START   stub_abort("__REGISTER_PREDDESC_DATA_START")
#define  __REGISTER_PREDDESC_DATA_WIDTH   stub_abort("__REGISTER_PREDDESC_DATA_WIDTH")
#define  __REGISTER_PREDDESC_ESZ_START    stub_abort("__REGISTER_PREDDESC_ESZ_START")
#define  __REGISTER_PREDDESC_ESZ_WIDTH    stub_abort("__REGISTER_PREDDESC_ESZ_WIDTH")
#define  __REGISTER_PREDDESC_OPRSZ_START  stub_abort("__REGISTER_PREDDESC_OPRSZ_START")
#define  __REGISTER_PREDDESC_OPRSZ_WIDTH  stub_abort("__REGISTER_PREDDESC_OPRSZ_WIDTH")


/* translate-a64.c */


#define  __REGISTER_MTEDESC_TBI_START   stub_abort("__REGISTER_MTEDESC_TBI_START")
#define  __REGISTER_MTEDESC_TBI_WIDTH   stub_abort("__REGISTER_MTEDESC_TBI_WIDTH")
#define  __REGISTER_MTEDESC_TCMA_START  stub_abort("__REGISTER_MTEDESC_TCMA_START")
#define  __REGISTER_MTEDESC_TCMA_WIDTH  stub_abort("__REGISTER_MTEDESC_TCMA_WIDTH")

// Prototyped in translate-a32.h
FUNC_STUB(mve_eci_check)
FUNC_STUB(mve_update_eci)
FUNC_STUB(mve_update_and_store_eci)
