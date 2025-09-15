/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <drivers/arm/pfdi_mod.h>
#include <plat/arm/common/plat_arm.h>
#include <plat/common/platform.h>
#include <services/pfdi_svc.h>

/**
 * PFDI Force error records
 */
typedef struct {
	bool enabled;
	uint32_t fid;
	pfdi_status_t error_id;
} force_err_inject_t;

static force_err_inject_t error_state[PLATFORM_CORE_COUNT];

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
		}
	}
}

static pfdi_status_t check_force_error(uint32_t fid)
{
	force_err_inject_t *state = &error_state[plat_my_core_pos()];

	if (state->enabled && state->fid == fid) {
		state->enabled = false;
		state->fid = 0;
		if (PFDI_HAS_PLAT_FUNC(check_plat_err))
			return plat_pfdi_func_desc.check_plat_err(fid, state->error_id);

		return state->error_id;
	}

	return RESERVED_ERROR_ID;
}

pfdi_status_t pfdi_pe_test_part_count(uint64_t *tc_size)
{
	pfdi_status_t ret;

	ret = check_force_error(PFDI_PE_TEST_PART_COUNT);
	if (ret != RESERVED_ERROR_ID)
		return ret;

	return pfdi_func_desc.count(tc_size);
}

pfdi_status_t pfdi_pe_test_run(uint64_t start, uint64_t end, uint64_t mode,
				uint64_t *ft_id)
{
	uint64_t test_count;
	pfdi_status_t ret;

	ret = check_force_error(PFDI_PE_TEST_RUN);
	if (ret != RESERVED_ERROR_ID)
		goto exit;

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
		ERROR("PFDI: Invalid parameters: start=%lld, end=%lld, mode=%llu\n",
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

	ret = check_force_error(PFDI_PE_TEST_ID);
	if (ret != RESERVED_ERROR_ID)
		return ret;

	if (lib_version == NULL)
		return PFDI_UNKNOWN;

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
		return PFDI_NOT_SUPPORTED;

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
	force_err_inject_t *state = &error_state[plat_my_core_pos()];
	pfdi_status_t ret;

	if (!IS_FEATURE_SUPPORTED(fid))
		return PFDI_ERROR;

	if (error_id < PFDI_TEST_COUNT_ZERO ||
		error_id > PFDI_SUCCESS ||
		error_id == RESERVED_ERROR_ID)
		return PFDI_ERROR;

	ret = check_force_error(PFDI_FORCE_ERROR);
	if (ret != RESERVED_ERROR_ID)
		return ret;

	if (PFDI_HAS_PLAT_FUNC(force_plat_err)) {
		if (plat_pfdi_func_desc.force_plat_err(fid, error_id) == PFDI_SUCCESS) {
			state->enabled = true;
			state->fid = fid;
			return PFDI_SUCCESS;
		}
		return PFDI_ERROR;
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
