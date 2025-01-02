/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <drivers/arm/pfdi_mod.h>


void plat_pfdi_pe_init(void)
{
	assert(pfdi_func_desc.name != NULL);
	assert(pfdi_func_desc.run != NULL);
	assert(pfdi_func_desc.count != NULL);
	assert(pfdi_func_desc.result != NULL);

	NOTICE("PFDI: Initializing Platform Fault Detection Interface.\n");
}

pfdi_status_t pfdi_pe_test_part_count(uint64_t *tc_size)
{
	return pfdi_func_desc.count(tc_size);
}

pfdi_status_t pfdi_pe_test_run(uint64_t start, uint64_t end, uint64_t mode,
				uint64_t *ft_id)
{
	uint64_t test_count;

	if (pfdi_func_desc.count(&test_count) != PFDI_SUCCESS)
		return PFDI_ERROR;

	if (start > end || end > test_count || (!IS_VALID_MODE(mode))) {
		return PFDI_INVALID_PARAMETERS;
	}

	return pfdi_func_desc.run(start, end, mode, ft_id);
}

pfdi_status_t pfdi_pe_test_id(uint64_t *lib_version)
{
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
	return pfdi_func_desc.result(ft_id);
}

pfdi_status_t pfdi_version(uint64_t *pfdi_version)
{
	if (pfdi_version == NULL)
		return PFDI_NOT_SUPPORTED;

#ifndef PFDI_VENDOR_VERSION
	return PFDI_NOT_SUPPORTED;
#endif
	*pfdi_version = PFDI_VENDOR_VERSION;

	return PFDI_SUCCESS;
}
