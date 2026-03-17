/*
 * Copyright (c) 2025-2026, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>
#include <drivers/arm/css/sds.h>
#include <drivers/arm/sbsa.h>
#include <lib/fconf/fconf.h>
#include <lib/fconf/fconf_dyn_cfg_getter.h>
#include <plat/arm/common/plat_arm.h>
#include <plat/common/platform.h>
#ifdef PFDI_SUPPORT
#include "rdaspen_sbist.h"
#endif /* PFDI_SUPPORT */

const mmap_region_t plat_arm_mmap[] = {
	ARM_MAP_SHARED_RAM,
	RDASPEN_MAP_DEVICE,
	RDASPEN_MAP_EXTERNAL_FLASH,
	RDASPEN_MAP_SMD_ATU_REGION,
#if PLAT_ARM_MEM_PROT_ADDR
	RDASPEN_MAP_MEM_PROTECT,
#endif
#if IMAGE_BL2
	RDASPEN_MAP_NS_DRAM1,
	RDASPEN_MAP_NS_DRAM2,
	RDASPEN_MAP_TRUSTED_NVCTR,
#endif
#if IMAGE_BL31
	RDASPEN_MAP_CPER_BUF,
#if PFDI_SUPPORT
	MAP_SBIST_MEM(0),
	MAP_SBIST_MEM(1),
	MAP_SBIST_MEM(2),
	MAP_SBIST_MEM(3),
#endif
#endif
	RDASPEN_MAP_S_DRAM,
	{0}
};

void plat_arm_secure_wdt_start(void)
{
	sbsa_wdog_start(SBSA_SECURE_WDOG_BASE, SBSA_SECURE_WDOG_TIMEOUT);
}

void plat_arm_secure_wdt_stop(void)
{
	sbsa_wdog_stop(SBSA_SECURE_WDOG_BASE);
}

void plat_arm_security_setup(void)
{
}

unsigned int plat_get_syscnt_freq2(void)
{
	/* Returning the Generic Timer Frequency */
	return SYS_COUNTER_FREQ_IN_TICKS;
}

int plat_get_mbedtls_heap(void **heap_addr, size_t *heap_size)
{
	assert(heap_addr != NULL);
	assert(heap_size != NULL);

	return arm_get_mbedtls_heap(heap_addr, heap_size);
}

static sds_region_desc_t rdaspen_sds_regions[] = {
	{ .base = PLAT_ARM_SDS_MEM_BASE },
};

sds_region_desc_t* plat_sds_get_regions(unsigned int *region_count)
{
	*region_count = ARRAY_SIZE(rdaspen_sds_regions);
	return rdaspen_sds_regions;
}
