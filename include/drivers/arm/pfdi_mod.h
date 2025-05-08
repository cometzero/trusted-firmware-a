/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PFDI_MOD_H
#define PFDI_MOD_H

#include <stdint.h>

#include <plat/common/platform.h>

/**
 * PFDI Return Codes
 */
typedef enum
{
	PFDI_TEST_COUNT_ZERO = -8,
	PFDI_UNKNOWN = -7,
	PFDI_NOT_RUN = -6,
	PFDI_ERROR = -5,
	PFDI_FAULT_FOUND = -4,
	PFDI_INVALID_PARAMETERS = -3,
	RESERVED_ERROR_ID = -2,
	PFDI_NOT_SUPPORTED = -1,
	PFDI_SUCCESS = 0,
} pfdi_status_t;

/**
 * PFDI Execution Modes
 */
enum pfdi_execution_mode
{
	PFDI_OOR_MODE = 1,
	PFDI_ONL_MODE = 2,
};

/**
 * PFDI Major and Minor version
 */
#define PFDI_VERSION_MAJOR		U(1)
#define PFDI_VERSION_MAJOR_SHIFT	16
#define PFDI_VERSION_MAJOR_MASK		U(0x7FFF)
#define PFDI_VERSION_MINOR		U(0)
#define PFDI_VERSION_MINOR_SHIFT	0
#define PFDI_VERSION_MINOR_MASK		U(0xFFFF)

#define PFDI_VENDOR_VERSION				\
		(((((uint64_t)PFDI_VERSION_MAJOR) &	\
		PFDI_VERSION_MAJOR_MASK)		\
		<< PFDI_VERSION_MAJOR_SHIFT) |		\
		((((uint64_t)PFDI_VERSION_MINOR) &	\
		PFDI_VERSION_MINOR_MASK)		\
		<< PFDI_VERSION_MINOR_SHIFT))

/* The fields are packed in the following order:
 * Bits [63:32] -> RESERVED
 * Bits [31:24] -> PFDI_VENDOR_ID
 * Bits [23:20] -> RESERVED
 * Bits [19:16] -> PFDI_LIBRARY_ID
 * Bits [15:08] -> PFDI_MAJOR_VERSION
 * Bits [07:00] -> PFDI_MINOR_VERSION
 */
#define PACK_VENDOR_ID(version)						\
	do								\
	{								\
		*(version) =						\
		    ((uint64_t)((PFDI_VENDOR_ID) & 0xFFUL) << 24) |	\
		    ((uint64_t)((PFDI_LIBRARY_ID) & 0xFUL) << 16) |	\
		    ((uint64_t)((PFDI_MAJOR_VERSION) & 0xFFUL) << 8) |	\
		    ((uint64_t)((PFDI_MINOR_VERSION) & 0xFFUL));	\
	} while (0)

#define IS_VALID_MODE(mode) \
	((mode) == PFDI_ONL_MODE || (mode) == PFDI_OOR_MODE)

/**
 * Platform Fault Detection Interface Function descriptor.
 */
typedef struct pfdi_func_desc_s
{
	/**
	 * Name of the PFDI function.
	 */
	const char *name;

	/**
	 * Run PFDI operations for a specific CPU.
	 *
	 * @param[in] start		The start test case number.
	 * @param[in] end		The end test case number.
	 * @param[in] mode		PFDI operation mode (online/out of reset)
	 * @param[out] ft_id		The failed test case id.
	 *
	 * @return		PFDI_SUCCESS on success or PFDI_ERROR on failure.
	 */
	pfdi_status_t (*run)(uint64_t start, uint64_t end, uint64_t mode,
				uint64_t *ft_id);

	/**
	 * Get the PFDI test case size.
	 *
	 * @param[out] tc_size	Pointer to the test case size.
	 *
	 * @return		Total number of test cases on success or PFDI_ERROR
	 * 			on failure.
	 */
	pfdi_status_t (*count)(uint64_t *tc_size);

	/**
	 * Get the PFDI test result.
	 *
	 * @param[out] ft_id		The failed test case id.
	 *
	 * @return			PFDI_SUCCESS on success or PFDI_ERROR on failure.
	 */
	pfdi_status_t (*result)(uint64_t *ft_id);

} pfdi_func_desc_t;

/* PFDI shared functions */

/**
 * Initialize the PFDI.
 */
void plat_pfdi_pe_init(void);

/**
 * Run PFDI operations for a specific CPU.
 *
 * @param[in] start		The start test case number.
 * @param[in] end		The end test case number.
 * @param[in] mode		PFDI operation mode (online/out of reset).
 * @param[out] ft_id		Pointer to the failed test case id.
 *
 * @return			PFDI_SUCCESS on success or ERROR on failure.
 */
pfdi_status_t pfdi_pe_test_run(uint64_t start, uint64_t end, uint64_t mode,
				 uint64_t *ft_id);

/**
 * Get the total number of PFDI test suites.
 *
 * @param[out] tc_size		Pointer to the test case size.
 *
 * @return			The total number of test suites on success or PFDI_ERROR on
 * 				failure.
 */
pfdi_status_t pfdi_pe_test_part_count(uint64_t *tc_size);

/**
 * Get the PFDI vendor Id.
 *
 * @param[out] lib_version	Pointer to store the PFDI test library version.
 *
 * @return			PFDI_SUCCESS on success or PFDI_ERROR on failure.
 */
pfdi_status_t pfdi_pe_test_id(uint64_t *lib_version);

/**
 * Get the PFDI test result.
 *
 * @param[out] ft_id		Pointer to the failed test case id.
 *
 * @return			PFDI_SUCCESS on success or PFDI_ERROR on failure.
 */
pfdi_status_t pfdi_pe_test_result(uint64_t *ft_id);

/**
 * Get the PFDI version.
 *
 * @param[out] pfdi_version	Pointer to store the PFDI version.
 *
 * @return			PFDI_SUCCESS on success or PFDI_ERROR on failure.
 */
pfdi_status_t pfdi_version(uint64_t *pfdi_version);

/**
 * Check supported features.
 *
 * @param[in] fid		Function identifier to check.
 *
 * @return			PFDI_SUCCESS on success or PFDI_ERROR on failure.
 */
pfdi_status_t pfdi_pe_features(uint32_t fid);

/**
 * Force error on request.
 *
 * @param[in] fid		Targeted smc function id.
 * @param[in] error_id		Targeted PFDI error id.
 *
 * @return			PFDI_SUCCESS on success or PFDI_ERROR on failure.
 */
pfdi_status_t pfdi_pe_force_error(const uint32_t fid, const pfdi_status_t error_id);

/**
 * Macro to register a callback with pfdi library.
 *
 * This macro defines and registers a PFDI function descriptor.
 *
 * @param _name		The name of the callback.
 * @param _run		The function pointer for running PFDI operations.
 * @param _count	The function pointer for retrieving test count.
 * @param _result	The function pointer for retrieving test result.
 */
#define REGISTER_PFDI_FUNC(_name, _run, _count, _result)	\
		const pfdi_func_desc_t pfdi_func_desc = {	\
			.name = _name,				\
			.run = _run,				\
			.count = _count,			\
			.result = _result			\
		}

/**
 * Declaration for a registered PFDI handlers
 */
extern const pfdi_func_desc_t pfdi_func_desc;

/**
 * Platform PFDI function descriptor (optional).
 */
typedef struct
{
	/**
	 * Name of the Platform PFDI function.
	 */
	const char *name;

	/**
	 * Optional function handler to force platform specific error.
	 *
	 * @param fid		Function Id to inject error.
	 * @param error_id	ERROR Id to force error.
	 *
	 * @return		PFDI_SUCCESS on success or PFDI_ERROR on failure.
	 */
	pfdi_status_t (*force_plat_err)(const uint32_t fid, const pfdi_status_t error_id);

	/**
	 * Optional function handler to check platform specific errors.
	 *
	 * @param fid		Function Id to check error status.
	 * @param error_id	ERROR Id to be expected.
	 *
	 * @return		return the platform error id.
	 */
	pfdi_status_t (*check_plat_err)(const uint32_t fid, const pfdi_status_t error_id);

} plat_pfdi_func_desc_t;

/**
 *  Optional platform pfdi function descriptor.
 */
#pragma weak plat_pfdi_func_desc
extern const plat_pfdi_func_desc_t plat_pfdi_func_desc;

/**
 * Macro to register a callback with platform specific pfdi functions.
 *
 * This macro defines and registers a Platform PFDI function descriptor.
 *
 * @param _name		The name of the callback.
 * @param _force_err	The function pointer for force platform pfdi error.
 * @param _check_err	THe function pointer to check platform error.
 */
#define REGISTER_PFDI_PLAT_FUNC(_name, _force_err, _check_err)	\
	const plat_pfdi_func_desc_t plat_pfdi_func_desc = {	\
		.name = _name,					\
		.force_plat_err = _force_err,			\
		.check_plat_err = _check_err			\
	}

/*
 * Returns true if the weak‐alias struct exists and the given callback
 * pointer is non-NULL.
 */
#define PFDI_HAS_PLAT_FUNC(func) \
	(((const void *)&plat_pfdi_func_desc != NULL)	\
	&& (plat_pfdi_func_desc.func != NULL))

#endif /* PFDI_MOD_H */
