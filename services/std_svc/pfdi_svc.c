/*
 * Copyright (c) 2025, ARM Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <stdint.h>
#include <inttypes.h>
#include <common/debug.h>
#include <common/runtime_svc.h>
#include <drivers/arm/pfdi_mod.h>
#include <plat/common/platform.h>
#include <services/pfdi_svc.h>
#include <services/std_svc.h>
#include <smccc_helpers.h>

uint64_t pfdi_smc_handler(uint32_t smc_fid,
			  u_register_t x1,
			  u_register_t x2,
			  u_register_t x3,
			  u_register_t x4,
			  void *cookie,
			  void *handle,
			  u_register_t flags)
{
	switch (smc_fid) {
		uint64_t ft_id = 0ULL;
		pfdi_status_t ret;
	case PFDI_VERSION:
		uint64_t version = 0;

		ret = pfdi_version(&version);
		if (ret == PFDI_SUCCESS)
			SMC_RET5(handle, version, 0U, 0U, 0U, 0U);

		SMC_RET5(handle, ret, 0U, 0U, 0U, 0U);
		break;
	case PFDI_FEATURES:
		ret = pfdi_pe_features((uint32_t)x1);
		if (ret == PFDI_SUCCESS)
			SMC_RET5(handle, PFDI_SUCCESS, 0U, 0U, 0U, 0U);

		SMC_RET5(handle, ret, 0U, 0U, 0U, 0U);
		break;
	case PFDI_PE_TEST_ID:
		uint64_t lib_version = 0;

		ret = pfdi_pe_test_id(&lib_version);
		if (ret != PFDI_SUCCESS)
			SMC_RET5(handle, ret, 0U, 0U, 0U, 0U);

		SMC_RET5(handle, PFDI_SUCCESS, lib_version, 0U, 0U, 0U);
		break;
	case PFDI_PE_TEST_PART_COUNT:
		uint64_t count;

		ret = pfdi_pe_test_part_count(&count);
		if (ret == PFDI_SUCCESS)
			SMC_RET5(handle, count, 0U, 0U, 0U, 0U);

		SMC_RET5(handle, ret, 0U, 0U, 0U, 0U);
		break;
	case PFDI_PE_TEST_RUN:
		ret = pfdi_pe_test_run(x1, x2, PFDI_ONL_MODE, &ft_id);
		if (ret == PFDI_FAULT_FOUND)
			SMC_RET5(handle, PFDI_FAULT_FOUND, ft_id, 0U, 0U, 0U);

		SMC_RET5(handle, ret, 0U, 0U, 0U, 0U);
		break;
	case PFDI_PE_TEST_RESULT:
		ret = pfdi_pe_test_result(&ft_id);
		if (ret == PFDI_FAULT_FOUND)
			SMC_RET5(handle, PFDI_FAULT_FOUND, ft_id, 0U, 0U, 0U);

		SMC_RET5(handle, ret, 0U, 0U, 0U, 0U);
		break;
	case PFDI_FW_CHECK:
		ret = pfdi_pe_fw_check();
		SMC_RET5(handle, ret, 0U, 0U, 0U, 0U);
		break;
	case PFDI_FORCE_ERROR:
		ret = pfdi_pe_force_error((uint32_t)x1, (pfdi_status_t)x2);
		SMC_RET5(handle, ret, 0U, 0U, 0U, 0U);
		break;
	default:
		WARN("Unsupported PFDI Service Call: 0x%x\n", smc_fid);
		SMC_RET5(handle, PFDI_NOT_SUPPORTED, 0U, 0U, 0U, 0U);
		break;
	}
}
