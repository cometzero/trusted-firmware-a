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

typedef struct {
	pfdi_status_t status;
	uint64_t ft_id;
} pfdi_tests_results_t;

static volatile pfdi_tests_results_t oor_pfdi_tests_results[PLATFORM_CORE_COUNT] = {
	[0 ... PLATFORM_CORE_COUNT - 1] = {
		.status = PFDI_NOT_RUN,
		.ft_id = UINT64_MAX
	}
};

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

	/* Store result only in Out-of-Reset mode for later retrieval via result query */
	if (mode == PFDI_OOR_MODE) {
		oor_pfdi_tests_results[cpu_num].status = status;
		oor_pfdi_tests_results[cpu_num].ft_id = (ft_id != NULL) ? *ft_id : 0;
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

/*
 * Retrieves the result of the out-of-reset (OoR) CPU self-test for the specified core.
 *
 * @cpu_num:  Logical CPU/core number for which the OoR test result is requested.
 * @ft_id:    Output parameter used to return the fault test ID associated with this CPU.
 *        This may represent a specific test case ID recorded during the boot-time OOR test.
 *
 * Returns:
 *  - PFDI_SUCCESS if the result is available
 *  - PFDI_ERROR if the CPU index is invalid
 *
 * Note:
 *  The results returned reflect the test status collected during the boot sequence
 *  as part of platform-level fault detection.
 */
static pfdi_status_t pfdi_cpu_self_test_result(uint64_t cpu_num, uint64_t *ft_id)
{
	if (cpu_num >= PLATFORM_CORE_COUNT) {
		WARN("PFDI: Invalid CPU index: %llu\n", cpu_num);
		return PFDI_ERROR;
	}

	if (ft_id != NULL) {
		*ft_id = oor_pfdi_tests_results[cpu_num].ft_id;
	}

	return oor_pfdi_tests_results[cpu_num].status;
}

REGISTER_PFDI_FUNC(LIB_NAME, pfdi_cpu_self_test_run, pfdi_cpu_self_test_count,
	pfdi_cpu_self_test_result);
