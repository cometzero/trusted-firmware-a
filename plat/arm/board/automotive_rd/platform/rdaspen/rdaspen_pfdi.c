/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * This file provides a placeholder (dummy) implementation of the PFDI CPU self-test
 * interfaces. It is intended for reference or simulation use only, and does not
 * perform actual fault detection.
 */

#include <stdint.h>

#include <common/debug.h>
#include <drivers/arm/pfdi_mod.h>
#include <plat/common/platform.h>

#define LIB_NAME "PFDI_TESTS"

#define MAX_NUM_TESTS			U(41)

/* Executes CPU-specific self-tests for the current core.
 *
 * @start: Start index of the test range to execute.
 * @end:   End index of the test range (exclusive).
 * @mode:  Test execution mode:
 *       - PFDI_OOR_MODE: Out-of-reset tests (run at boot).
 *       - PFDI_ONL_MODE: Online tests (run periodically at runtime).
 * @ft_id:  Output parameter. If a test fails, this is set to the ID of the
 *      failed test case. If all tests pass, this is set to UINT64_MAX.
 */
static pfdi_status_t pfdi_cpu_self_test_run(uint64_t start, uint64_t end,
					uint64_t mode, uint64_t *ft_id)
{
	uint64_t cpu_num = plat_my_core_pos();
	pfdi_status_t status = PFDI_SUCCESS;
	(void)start;
	(void)end;
	(void)mode;

	if (cpu_num >= PLATFORM_CORE_COUNT) {
		WARN("PFDI: Invalid CPU index: %llu\n", cpu_num);
		return PFDI_ERROR;
	}

	/*
	 * Placeholder for platform-specific CPU self-test integration.
	 *
	 * int run_cpu_self_test(uint64_t cpu_id, uint64_t start, uint64_t end, uint64_t mode)
	 * {
	 *   if (mode == PFDI_OOR_MODE) {
	 *     return run_oor_test(cpu_id, start, end);
	 *   } else if (mode == PFDI_ONL_MODE) {
	 *     return run_onl_test(cpu_id, start, end);
	 *   }
	 *   return -1;
	 * }
	 *
	 * // Real implementation example:
	 * int test_ret = run_cpu_self_test(cpu_num, start, end, mode);
	 * if (test_ret != UINT64_MAX && ft_id != NULL) {
	 *   status = PFDI_FAULT_FOUND;
	 *   *ft_id = (uint64_t)test_ret; // Assign ID of the failed test case
	 * }
	 */

	if (ft_id) {
		*ft_id = UINT64_MAX;	/* All tests passed */
	}

	return status;
}

/*
 * Returns the total number of CPU self-test cases available.
 *
 * @testcase_count: Output pointer to receive the test case count.
 * Returns:
 *   - PFDI_SUCCESS if count was set successfully
 *   - PFDI_TEST_COUNT_ZERO if no tests are defined
 *   - PFDI_ERROR if the input pointer is NULL
 */
static pfdi_status_t pfdi_cpu_self_test_count(uint64_t *testcase_count)
{
	if (testcase_count == NULL) {
		return PFDI_ERROR;
	}

	if (MAX_NUM_TESTS == 0) {
		return PFDI_TEST_COUNT_ZERO;
	}

	*testcase_count = MAX_NUM_TESTS;

	return PFDI_SUCCESS;
}

static pfdi_status_t pfdi_cpu_self_test_result(uint64_t *ft_id)
{
	return PFDI_SUCCESS;
}

REGISTER_PFDI_FUNC(LIB_NAME, pfdi_cpu_self_test_run, pfdi_cpu_self_test_count,
	pfdi_cpu_self_test_result);
