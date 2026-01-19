/*
 * Copyright (c) 2025-2026, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <inttypes.h>
#include <drivers/arm/pfdi_mod.h>
#include <drivers/delay_timer.h>
#include <plat/arm/common/plat_arm.h>
#include <plat/common/platform.h>
#include <services/pfdi_svc.h>

#ifndef PFDI_CPU_OFF_RETRY
/* 0 = wait forever; >0 = retry count (retries * PFDI_OFF_RETRY_US µs) */
#define PFDI_CPU_OFF_RETRY   0U
#endif
#ifndef PFDI_OFF_RETRY_US
#define PFDI_OFF_RETRY_US    10U
#endif

/**
 * PFDI Force error records
 */
typedef struct {
	bool enabled;
	uint32_t fid;
	pfdi_status_t error_id;
} force_err_inject_t;

/* Update number of FORCE_ERROR slots supported per core when new feature is added*/
#define PFDI_FORCE_ERR_SLOTS_PER_CORE  8U
static force_err_inject_t error_state[PLATFORM_CORE_COUNT][PFDI_FORCE_ERR_SLOTS_PER_CORE];

static force_err_inject_t *pfdi_find_or_alloc_force_slot(uint32_t core, uint32_t fid)
{
	force_err_inject_t *free_slot = NULL;

	for (unsigned int i = 0; i < PFDI_FORCE_ERR_SLOTS_PER_CORE; i++) {
		force_err_inject_t *s = &error_state[core][i];

		/* If already have an entry for this fid, update it */
		if (s->enabled && s->fid == fid) {
			return s;
		}

		/* Track first free slot */
		if (!s->enabled && free_slot == NULL) {
			free_slot = s;
		}
	}

	return free_slot;
}

/*
 * Wait until the CPU is OFF.
 * Policy: return only on OFF; panic on PSCI error or bounded-timeout.
 */
static void wait_cpu_off(u_register_t mpidr, int cpu_num)
{
	unsigned int retries = PFDI_CPU_OFF_RETRY;
	int state = AFF_STATE_ON;

	for (;;) {
		state = psci_affinity_info(mpidr, MPIDR_AFFLVL0);

		if (state == AFF_STATE_OFF) {
			return;
		}
		if (state < 0) {
			ERROR("PFDI: CPU %d (mpidr=0x%lx) PSCI error %d\n",
			      cpu_num, mpidr, state);
			panic();
		}

		/* Optional timeout: only counts down if nonzero, Platform can
		 * set PFDI_CPU_OFF_RETRY == 0 for infinite wait
		 */
		if (retries && --retries == 0) {
			ERROR("PFDI: timeout waiting for Core %d (mpidr=0x%lx) to go OFF (state=%d)\n",
				cpu_num, mpidr, state);
			panic();
		}

		udelay(PFDI_OFF_RETRY_US);
	}
}

void plat_pfdi_pe_init(void)
{
	pfdi_status_t pfdi_status;

	assert(pfdi_func_desc.name != NULL);
	assert(pfdi_func_desc.run != NULL);
	assert(pfdi_func_desc.count != NULL);
	assert(pfdi_func_desc.result != NULL);

	NOTICE("PFDI: Initializing Platform Fault Detection Interface.\n");

	NOTICE("PFDI: Running OoR tests on primary core.\n");
	pfdi_status = pfdi_pe_oor_test_run();
	if (pfdi_status != PFDI_SUCCESS) {
		ERROR("PFDI: OoR tests on primary core failed.\n");
		panic();
	} else {
		NOTICE("PFDI: OoR tests on primary core succeeded.\n");
	}

	NOTICE("PFDI: Running OoR tests on secondary cores.\n");

	for (uint8_t cluster_id = 0; cluster_id < PLAT_ARM_CLUSTER_COUNT; cluster_id++) {
		for (uint8_t cpu_id = 0; cpu_id < PLAT_MAX_CPUS_PER_CLUSTER; cpu_id++) {
			uint64_t ft_id;
			int psci_ret;
			u_register_t mpidr = ((cpu_id & MPIDR_AFFLVL_MASK) << MPIDR_AFF1_SHIFT) |
					((cluster_id & MPIDR_AFFLVL_MASK) << MPIDR_AFF2_SHIFT);

			if (plat_arm_get_cluster_core_count(mpidr) <= cpu_id) {
				break;
			}

			int cpu_num = plat_core_pos_by_mpidr(mpidr);

			/*
			 * This function is only called from the primary core,
			 * so it must be the primary core if the mpidr matches the current core.
			 */
			if (plat_my_core_pos() == cpu_num) {
				continue;
			}

			/*
			 * Entrypoint is not used but the platform specific validation
			 * makes sure that the address is within the DRAM.
			 */
			psci_ret = psci_cpu_on(mpidr, (uintptr_t)ARM_DRAM1_BASE, 0U);
			if (psci_ret == PSCI_E_SUCCESS) {
				do {
					pfdi_status = pfdi_func_desc.result(cpu_num, &ft_id);
				} while (pfdi_status == PFDI_NOT_RUN);
			} else {
				ERROR("PFDI: Failed to turn on core %d.\n", cpu_num);
				continue;
			}

			if (pfdi_status != PFDI_SUCCESS) {
				ERROR("PFDI: OoR tests on core %d failed at test %ld.\n",
					cpu_num, ft_id);
			} else {
				INFO("PFDI: OoR tests on core %d succeeded.\n", cpu_num);
			}

			/* proceed only if  PE is fully powered down (AFF_STATE_OFF) */
			wait_cpu_off(mpidr, cpu_num);
		}
	}
}

static pfdi_status_t check_force_error(uint32_t fid)
{
	uint32_t core = plat_my_core_pos();

	for (unsigned int i = 0; i < PFDI_FORCE_ERR_SLOTS_PER_CORE; i++) {
		force_err_inject_t *state = &error_state[core][i];

		if (state->enabled && state->fid == fid) {
			pfdi_status_t err = state->error_id;

			state->enabled = false;
			state->fid = 0U;
			state->error_id = RESERVED_ERROR_ID;

			if (PFDI_HAS_PLAT_FUNC(check_plat_err)) {
				return plat_pfdi_func_desc.check_plat_err(fid, err);
			}
			return err;
		}
	}

	return RESERVED_ERROR_ID;
}

pfdi_status_t pfdi_pe_test_part_count(uint64_t *tc_size)
{
	pfdi_status_t ret;

	ret = check_force_error(PFDI_PE_TEST_PART_COUNT);
	if (ret != RESERVED_ERROR_ID)
		return ret;

	if (tc_size == NULL)
		return PFDI_INVALID_PARAMETERS;

	return pfdi_func_desc.count(tc_size);
}

pfdi_status_t pfdi_pe_test_run(uint64_t start, uint64_t end, uint64_t mode,
				uint64_t *ft_id)
{
	uint64_t test_count;
	pfdi_status_t ret;

	if (ft_id == NULL)
		return PFDI_INVALID_PARAMETERS;

	ret = check_force_error(PFDI_PE_TEST_RUN);
	if (ret != RESERVED_ERROR_ID) {
		if (ret == PFDI_FAULT_FOUND)
			*ft_id = 0;

		goto exit;
	}

	if (pfdi_func_desc.count(&test_count) != PFDI_SUCCESS) {
		ret = PFDI_ERROR;
		goto exit;
	}

	if ((((int64_t)start == -1) != ((int64_t)end == -1)) ||
		((int64_t)start < -1) ||
		((int64_t)end < -1) ||
		((int64_t)start >= 0 && (int64_t)end >= 0 &&
			(start > end || start >= test_count || end >= test_count)) ||
			!IS_VALID_MODE(mode)) {

		ret = PFDI_INVALID_PARAMETERS;
		ERROR("PFDI: Invalid parameters: start=%" PRId64 ", end=%" PRId64
			", mode=%" PRIu64 "\n",
			(int64_t)start, (int64_t)end, mode);
		goto exit;
	}

	if ((int64_t)start == -1 && (int64_t)end == -1) {
		start = 0;
		end = test_count - 1;
	}

	ret = pfdi_func_desc.run(start, end, mode, ft_id);

exit:
	if (PFDI_HAS_PLAT_FUNC(post_run)) {
		plat_pfdi_func_desc.post_run(ret, start, end, mode, ft_id);
	}

	return ret;
}

pfdi_status_t pfdi_pe_test_id(uint64_t *lib_version)
{
	pfdi_status_t ret;

	if (lib_version)
		*lib_version = 0ULL;

	ret = check_force_error(PFDI_PE_TEST_ID);
	if (ret != RESERVED_ERROR_ID)
		return ret;

	if (lib_version == NULL)
		return PFDI_INVALID_PARAMETERS;

#ifndef PACK_VENDOR_ID
	return PFDI_UNKNOWN;
#endif
	PACK_VENDOR_ID(lib_version);

	return PFDI_SUCCESS;
}

pfdi_status_t pfdi_pe_test_result(uint64_t *ft_id)
{
	pfdi_status_t ret;
	uint64_t cpu_num;

	ret = check_force_error(PFDI_PE_TEST_RESULT);
	if (ret != RESERVED_ERROR_ID)
		return ret;

	if (ft_id == NULL)
		return PFDI_INVALID_PARAMETERS;

	/* Initialize to no fault found */
	*ft_id = 0ULL;
	cpu_num = plat_my_core_pos();
	return pfdi_func_desc.result(cpu_num, ft_id);
}

pfdi_status_t pfdi_version(uint64_t *pfdi_version)
{
	pfdi_status_t ret;

	ret = check_force_error(PFDI_VERSION);
	if (ret != RESERVED_ERROR_ID)
		return ret;

	if (pfdi_version == NULL)
		return PFDI_INVALID_PARAMETERS;

#ifndef PFDI_VENDOR_VERSION
	return PFDI_NOT_SUPPORTED;
#endif
	*pfdi_version = PFDI_VENDOR_VERSION;

	return PFDI_SUCCESS;
}

pfdi_status_t pfdi_pe_features(uint32_t fid)
{
	pfdi_status_t ret;

	ret = check_force_error(PFDI_FEATURES);
	if (ret != RESERVED_ERROR_ID)
		return ret;

	if (!is_pfdi_fid(fid)) {
		return PFDI_INVALID_PARAMETERS;
	}

	if (!IS_FEATURE_SUPPORTED(fid))
		return PFDI_NOT_SUPPORTED;

	return PFDI_SUCCESS;
}

pfdi_status_t pfdi_pe_fw_check(void)
{
	pfdi_status_t ret;

	ret = check_force_error(PFDI_FW_CHECK);
	if (ret != RESERVED_ERROR_ID)
		return ret;

	/* No firmware checks implemented yet */
	return PFDI_SUCCESS;
}

pfdi_status_t pfdi_pe_force_error(const uint32_t fid, const pfdi_status_t error_id)
{
	pfdi_status_t ret;
	uint32_t core = plat_my_core_pos();
	force_err_inject_t *state;

	if (!IS_FEATURE_SUPPORTED(fid))
		return PFDI_INVALID_PARAMETERS;

	if (!is_pfdi_fid(fid)) {
		return PFDI_INVALID_PARAMETERS;
	}

	if (error_id < PFDI_TEST_COUNT_ZERO ||
		error_id > PFDI_SUCCESS ||
		error_id == RESERVED_ERROR_ID)
		return PFDI_INVALID_PARAMETERS;

	ret = check_force_error(PFDI_FORCE_ERROR);
	if (ret != RESERVED_ERROR_ID)
		return ret;

	/* allocate/find slot for this fid on this core */
	state = pfdi_find_or_alloc_force_slot(core, fid);
	if (state == NULL) {
		return PFDI_ERROR;
	}
	if (PFDI_HAS_PLAT_FUNC(force_plat_err)) {
		if (plat_pfdi_func_desc.force_plat_err(fid, error_id) != PFDI_SUCCESS) {
			return PFDI_ERROR;
		}
	}

	state->fid = fid;
	state->enabled = true;
	state->error_id = error_id;

	return PFDI_SUCCESS;
}

pfdi_status_t pfdi_pe_oor_test_run(void)
{
	pfdi_status_t ret;
	uint64_t ft_id, tc_size;

	ret = check_force_error(PFDI_FW_CHECK);
	if (ret != RESERVED_ERROR_ID)
		return ret;

	/*
	 * Make sure that the OoR PFDI was not ran before,
	 * whether it succeeded or failed the last time.
	 */
	ret = pfdi_pe_test_result(&ft_id);
	if (ret != PFDI_NOT_RUN) {
		return ret;
	}

	ret = pfdi_pe_test_part_count(&tc_size);
	if (ret != PFDI_SUCCESS) {
		goto exit;
	}

	ret = pfdi_pe_test_run(0UL, tc_size - 1UL, PFDI_OOR_MODE, &ft_id);

exit:
	/*
	 * Out-of-Reset PFDI for secondary cores are triggered
	 * by primary core, so put the core back to off state.
	 */
	if (!plat_is_my_cpu_primary()) {
		return psci_cpu_off();
	}

	return ret;
}
