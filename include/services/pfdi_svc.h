/*
 * Copyright (c) 2025, ARM Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PFDI_SVC_H
#define PFDI_SVC_H

#include <stdint.h>

#include <lib/utils_def.h>
#include <smccc_helpers.h>

/*
 * SMC function IDs for PFDI Service
 * Upper word bits set: Fast call, SMC64, Standard Secure SVC. Call (OEN = 4)
 */
#define PFDI_FID(func_num)			\
	((SMC_TYPE_FAST << FUNCID_TYPE_SHIFT) |	\
	 (SMC_64 << FUNCID_CC_SHIFT) |		\
	 (OEN_STD_START << FUNCID_OEN_SHIFT) |	\
	 ((func_num) << FUNCID_NUM_SHIFT))

/* Platform Fault Detection Interface(PFDI) functions */
#define PFDI_FNUM_VERSION		U(0x2D0)
#define PFDI_FNUM_FEATURES		U(0x2D1)
#define PFDI_FNUM_PE_TEST_ID		U(0x2D2)
#define PFDI_FNUM_PE_TEST_PART_COUNT	U(0x2D3)
#define PFDI_FNUM_PE_TEST_RUN		U(0x2D4)
#define PFDI_FNUM_PE_TEST_RESULT	U(0x2D5)
#define PFDI_FNUM_FW_CHECK		U(0x2D6)
#define PFDI_FNUM_FORCE_ERROR		U(0x2D7)

#define PFDI_VERSION			PFDI_FID(PFDI_FNUM_VERSION)
#define PFDI_FEATURES			PFDI_FID(PFDI_FNUM_FEATURES)
#define PFDI_PE_TEST_ID			PFDI_FID(PFDI_FNUM_PE_TEST_ID)
#define PFDI_PE_TEST_PART_COUNT		PFDI_FID(PFDI_FNUM_PE_TEST_PART_COUNT)
#define PFDI_PE_TEST_RUN		PFDI_FID(PFDI_FNUM_PE_TEST_RUN)
#define PFDI_PE_TEST_RESULT		PFDI_FID(PFDI_FNUM_PE_TEST_RESULT)
#define PFDI_FW_CHECK			PFDI_FID(PFDI_FNUM_FW_CHECK)
#define PFDI_FORCE_ERROR		PFDI_FID(PFDI_FNUM_FORCE_ERROR)

/*
 * The macros below are used to identify (Platform Fault Detection Interface)
 * PFDI calls from the SMC function ID
 */
#define PFDI_FID_MASK U(0xFF0)
#define PFDI_FID_VALUE U(0x2D0)
#define is_pfdi_fid(_fid)					\
	((((_fid) & PFDI_FID_MASK) == PFDI_FID_VALUE) &&	\
	 (((_fid >> FUNCID_CC_SHIFT) & FUNCID_CC_MASK) == SMC_64))

uint64_t pfdi_smc_handler(uint32_t smc_fid,
			  u_register_t x1, u_register_t x2,
			  u_register_t x3, u_register_t x4,
			  void *cookie, void *handle,
			  u_register_t flags);

#endif /* PFDI_SVC_H */
