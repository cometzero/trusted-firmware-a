/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <drivers/arm/pfdi_mod.h>
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
	assert(pfdi_func_desc.name != NULL);
	assert(pfdi_func_desc.run != NULL);
	assert(pfdi_func_desc.count != NULL);
	assert(pfdi_func_desc.result != NULL);

	NOTICE("PFDI: Initializing Platform Fault Detection Interface.\n");
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
		return ret;

	if (pfdi_func_desc.count(&test_count) != PFDI_SUCCESS)
		return PFDI_ERROR;

	if (start > end || end > test_count || (!IS_VALID_MODE(mode))) {
		return PFDI_INVALID_PARAMETERS;
	}

	return pfdi_func_desc.run(start, end, mode, ft_id);
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
