/*
 * Copyright (c) 2026, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef CPU_SELF_TEST_H
#define CPU_SELF_TEST_H

#include <drivers/arm/pfdi_mod.h>

/*
 * Initialize CPU self-test infrastructure for a given CPU.
 *
 * Returns:
 *  - PFDI_SUCCESS on success
 *  - PFDI_ERROR on failure
 */
pfdi_status_t cpu_self_test_init(uint32_t cpu_num);

/*
 * Run CPU self-test.
 *
 * Parameters:
 *  - cpu_num: platform CPU index
 *  - mode:    PFDI_ONL_MODE or PFDI_OOR_MODE
 *  - start/end: test range (implementation-defined)
 *  - ft_id: optional; set to 0 on pass, set to failure/test identifier on failure
 *
 * Returns:
 *  - PFDI_SUCCESS on pass
 *  - PFDI_ERROR on failure
 */
pfdi_status_t cpu_self_test_run(uint32_t cpu_num, uint32_t mode,
				uint64_t start, uint64_t end,
				uint64_t *ft_id);

#endif /* CPU_SELF_TEST_H */
