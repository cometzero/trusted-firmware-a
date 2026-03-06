/*
 * Copyright (c) 2025-2026, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PLATFORM_DEF_H
#define PLATFORM_DEF_H

#include <lib/utils_def.h>
#include <lib/xlat_tables/xlat_tables_defs.h>

#define PLAT_ARM_SHARED_RAM_SIZE		UL(0x2000)	/* 8 KB */

#define PLAT_ARM_MHU_NS_SHARED_SRAM_BASE	UL(0x104000)
#define PLAT_ARM_MHU_NS_SHARED_SRAM_SIZE	UL(0x00001000)

#define PLAT_ARM_TRUSTED_SRAM_BASE		UL(0x0)
#define PLAT_ARM_TRUSTED_SRAM_SIZE		UL(0x00100000)

/*
 * SMD region assignment
 * AP accesses devices located in SMD via AP SMD region starting from
 * 0x4000_0000.
 */
/* CNTControl frame 64KB */
#define PLAT_ARM_SYS_CNTCTL_BASE		UL(0x40000000)
/* CNTRead frame 64KB */
#define PLAT_ARM_SYS_CNTREAD_BASE		UL(0x40010000)
/* SI MHU regions 6MB in total */
#define SI_MHU_REGION_BASE			UL(0x40020000)
/* AP MHU secure regions 384KB in total */
#define RDASPEN_CSS_AP_RSE_SECURE_MHU_BASE	UL(0x40680000)

/* AP<->SI MHUv3 non-secure Tx and Rx frame base */
#define AP_SI_NON_SECURE_MHU_V3_TX_BASE		SI_MHU_REGION_BASE
#define AP_SI_NON_SECURE_MHU_V3_RX_BASE		(SI_MHU_REGION_BASE + \
							0x40000U)
#define MHU_V3_TX_RX_FRAME_SIZE			UL(0x30000)

/* AP access AP_REFLCK generic timers via Peripheral Block */
#define PLAT_ARM_SYS_TIMCTL_BASE		UL(0x1A810000)
#define PLAT_ARM_SYS_CNT_BASE_S			UL(0x1A820000)
#define PLAT_ARM_SYS_CNT_BASE_NS		UL(0x1A830000)
#define PLAT_ARM_NSTIMER_FRAME_ID		U(0)

#include <plat/arm/common/arm_def.h>
#include <plat/arm/css/common/css_def.h>
#include <plat/common/common_def.h>

/* Set default topology value if not passed from platform's makefile */
#ifndef PLATFORM_CORE_COUNT
#define PLATFORM_CORE_COUNT			U(16)
#endif /* PLATFORM_CORE_COUNT */

#if (PLATFORM_CORE_COUNT > 16) || (PLATFORM_CORE_COUNT < 1)
#error "Invalid number of platform's cores was passed."
#endif /* 1 <= PLATFORM_CORE_COUNT <= 16 */

#if (PLATFORM_CORE_COUNT <= 4)
#define PLATFORM_CLUSTER_0_CORE_COUNT		U(PLATFORM_CORE_COUNT)
#define PLATFORM_CLUSTER_1_CORE_COUNT		U(0)
#define PLATFORM_CLUSTER_2_CORE_COUNT		U(0)
#define PLATFORM_CLUSTER_3_CORE_COUNT		U(0)

#else /* PLATFORM_CORE_COUNT <= 4 */
#define PLATFORM_CLUSTER_0_CORE_COUNT		U(4)

#if (PLATFORM_CORE_COUNT <= 8)
#define PLATFORM_CLUSTER_1_CORE_COUNT		U(PLATFORM_CORE_COUNT - 4)
#define PLATFORM_CLUSTER_2_CORE_COUNT		U(0)
#define PLATFORM_CLUSTER_3_CORE_COUNT		U(0)

#else /* PLATFORM_CORE_COUNT <= 8 */
#define PLATFORM_CLUSTER_1_CORE_COUNT		U(4)

#if (PLATFORM_CORE_COUNT <= 12)
#define PLATFORM_CLUSTER_2_CORE_COUNT		U(PLATFORM_CORE_COUNT - 8)
#define PLATFORM_CLUSTER_3_CORE_COUNT		U(0)

#else /* PLATFORM_CORE_COUNT <= 12 */
#define PLATFORM_CLUSTER_2_CORE_COUNT		U(4)
#define PLATFORM_CLUSTER_3_CORE_COUNT		U(PLATFORM_CORE_COUNT - 12)

#endif /* PLATFORM_CORE_COUNT <= 12 */
#endif /* PLATFORM_CORE_COUNT <= 8 */
#endif /* PLATFORM_CORE_COUNT <= 4 */

#define PLAT_MAX_CPUS_PER_CLUSTER		U(4)
#define PLAT_MAX_PE_PER_CPU			U(1)
#define PLAT_ARM_CLUSTER_COUNT			((PLATFORM_CORE_COUNT + \
						  PLAT_MAX_CPUS_PER_CLUSTER - 1) / \
						  PLAT_MAX_CPUS_PER_CLUSTER)

#define PLATFORM_STACK_SIZE			UL(0x1000)

/* BL1 is not supported */
#define PLAT_ARM_TRUSTED_ROM_BASE		UL(0x0)
#define PLAT_ARM_TRUSTED_ROM_SIZE		UL(0x0)
#define PLAT_ARM_MAX_BL1_RW_SIZE		UL(0x0)

/* USE_ROMLIB is not supported */
#define PLAT_ARM_MAX_ROMLIB_RW_SIZE		U(0)
#define PLAT_ARM_MAX_ROMLIB_RO_SIZE		U(0)

#define PLAT_ARM_MAX_BL31_SIZE			UL(0xFC000)

#undef ARM_DRAM2_BASE
#undef ARM_DRAM2_SIZE
#define ARM_DRAM2_BASE				ULL(0x20000000000)

#ifdef RD_ASPEN_VARIANT_FVP
#define ARM_DRAM2_SIZE		 		SZ_2G
#elif defined(RD_ASPEN_VARIANT_RTL)
#define ARM_DRAM2_SIZE		 		SZ_1G * 30
#endif

/*
 * MHU for SCMI communication with SCP on Safety Island CL0
 * Offset to SI_MHU_REGION_BASE: 0x0008_0000.
 */
#define PLAT_CSS_MHU_BASE			UL(0x400A0000)

/*
 * MHU for PFDI Monitor communication with SCP on Safety Island CL0
 * Offset to SI_MHU_REGION_BASE: 0x0038_0000.
 */
#define PFDI_MONITOR_MHU_BASE			UL(0x403A0000)

/* UART Related Constants */
#define SOC_CSS_SEC_UART_BASE			UL(0x1A410000)
#define SOC_CSS_UART_SIZE			SZ_64K
#define SOC_CSS_UART_CLK_IN_HZ			UL(24000000)
#define PLAT_ARM_BOOT_UART_BASE			SOC_CSS_SEC_UART_BASE
#define PLAT_ARM_BOOT_UART_CLK_IN_HZ		SOC_CSS_UART_CLK_IN_HZ
#define PLAT_ARM_RUN_UART_BASE			SOC_CSS_SEC_UART_BASE
#define PLAT_ARM_RUN_UART_CLK_IN_HZ		SOC_CSS_UART_CLK_IN_HZ
#define PLAT_ARM_CRASH_UART_BASE		SOC_CSS_SEC_UART_BASE
#define PLAT_ARM_CRASH_UART_CLK_IN_HZ		SOC_CSS_UART_CLK_IN_HZ

/* 256 TB */
#define SZ_256T					(1ULL << 48)
/* Physical and virtual address space limits for MMU */
#define PLAT_PHY_ADDR_SPACE_SIZE		SZ_256T
#define PLAT_VIRT_ADDR_SPACE_SIZE		SZ_256T

#define PLAT_ARM_GICD_BASE			UL(0x20800000)
#define PLAT_ARM_GICR_BASE			UL(0x20880000)
#define GICR_SIZE                 		0x40000

/* GIC redistributors */
#define GICR_BASE_VIEW0_0_0       		PLAT_ARM_GICR_BASE
#define GICR_BASE_VIEW0_0_1       		(GICR_BASE_VIEW0_0_0 + GICR_SIZE)
#define GICR_BASE_VIEW0_0_2       		(GICR_BASE_VIEW0_0_1 + GICR_SIZE)
#define GICR_BASE_VIEW0_0_3       		(GICR_BASE_VIEW0_0_2 + GICR_SIZE)
#define GICR_BASE_VIEW0_1_0       		(GICR_BASE_VIEW0_0_3 + GICR_SIZE)
#define GICR_BASE_VIEW0_1_1       		(GICR_BASE_VIEW0_1_0 + GICR_SIZE)
#define GICR_BASE_VIEW0_1_2       		(GICR_BASE_VIEW0_1_1 + GICR_SIZE)
#define GICR_BASE_VIEW0_1_3       		(GICR_BASE_VIEW0_1_2 + GICR_SIZE)
#define GICR_BASE_VIEW0_2_0       		(GICR_BASE_VIEW0_1_3 + GICR_SIZE)
#define GICR_BASE_VIEW0_2_1       		(GICR_BASE_VIEW0_2_0 + GICR_SIZE)
#define GICR_BASE_VIEW0_2_2      		(GICR_BASE_VIEW0_2_1 + GICR_SIZE)
#define GICR_BASE_VIEW0_2_3       		(GICR_BASE_VIEW0_2_2 + GICR_SIZE)
#define GICR_BASE_VIEW0_3_0       		(GICR_BASE_VIEW0_2_3 + GICR_SIZE)
#define GICR_BASE_VIEW0_3_1       		(GICR_BASE_VIEW0_3_0 + GICR_SIZE)
#define GICR_BASE_VIEW0_3_2       		(GICR_BASE_VIEW0_3_1 + GICR_SIZE)
#define GICR_BASE_VIEW0_3_3       		(GICR_BASE_VIEW0_3_2 + GICR_SIZE)

#define PLAT_ARM_G1S_IRQ_PROPS(grp)		CSS_G1S_IRQ_PROPS(grp)
#define PLAT_ARM_G0_IRQ_PROPS(grp)		ARM_G0_IRQ_PROPS(grp)

/* Secure Watchdog Constants */
#define SBSA_SECURE_WDOG_BASE			UL(0x1A460000)
#define SBSA_SECURE_WDOG_TIMEOUT		UL(100)

/* Virtual address used by dynamic mem_protect for chunk_base */
#define PLAT_ARM_MEM_PROTEC_VA_FRAME		UL(0xC0000000)

/* SCMI Related Constants */
#define PLAT_ARM_SCMI_CHANNEL_COUNT		U(1)
#define CSS_SYSTEM_PWR_DMN_LVL			ARM_PWR_LVL2
#define PLAT_MAX_PWR_LVL			ARM_PWR_LVL1

/* SCMI PFDI Monitor Related Constants */
#define PLAT_ARM_SCMI_PFDI_MONITOR_CHANNEL_COUNT	U(16)

#define MAX_IO_DEVICES				U(3)
#define MAX_IO_HANDLES				U(4)

#define RDASPEN_CPER_BUF_BASE			UL(0xFFA00000)
#define RDASPEN_CPER_BUF_SIZE			UL(0x00100000)

#define RDASPEN_MAP_CPER_BUF			MAP_REGION_FLAT(RDASPEN_CPER_BUF_BASE, \
						RDASPEN_CPER_BUF_SIZE, \
						MT_MEMORY | MT_RW | MT_NS)

#ifdef PFDI_SUPPORT
#define PLAT_PFDI_ENTRIES 			U(4)
#else
#define PLAT_PFDI_ENTRIES 			U(0)
#endif /* PFDI_SUPPORT */

#if IMAGE_BL2
#define PLAT_ARM_MMAP_ENTRIES			(U(10) + PLAT_PFDI_ENTRIES)
#elif IMAGE_BL31
#define PLAT_ARM_MMAP_ENTRIES			(U(9) + PLAT_PFDI_ENTRIES)
#endif

#define MAX_XLAT_TABLES				(U(10) + PLAT_PFDI_ENTRIES)

#define PLAT_FW_CONFIG_MAX_SIZE			(ARM_FW_CONFIG_LIMIT - \
						 ARM_FW_CONFIG_BASE)
#define PLAT_FW_CONFIG_BASE			ARM_FW_CONFIG_BASE

/*
 * Map peripherals till GIC regions and devices in Rest of System
 */
#define RDASPEN_DEVICE_BASE		UL(0x10000000)
#define RDASPEN_DEVICE_SIZE		UL(0x28000000)
#define RDASPEN_MAP_DEVICE		MAP_REGION_FLAT(RDASPEN_DEVICE_BASE, \
							RDASPEN_DEVICE_SIZE, \
							MT_DEVICE | MT_RW | \
							MT_SECURE)

/* Flash in Rest of the System */
#define EXT_FLASH_BASE			UL(0x38000000)
#define EXT_FLASH_SIZE			SZ_128M
#define EXT_FLASH_BLOCK_SIZE		SZ_256K
#define PLAT_ARM_FLASH_IMAGE_BASE	EXT_FLASH_BASE
#define PLAT_ARM_FLASH_IMAGE_MAX_SIZE	(EXT_FLASH_SIZE - \
					 EXT_FLASH_BLOCK_SIZE)

#define RDASPEN_MAP_EXTERNAL_FLASH	MAP_REGION_FLAT(EXT_FLASH_BASE, \
							PLAT_ARM_FLASH_IMAGE_MAX_SIZE, \
							MT_DEVICE | MT_RO | \
							MT_SECURE)

/* SMD ATU region */
#define RDASPEN_SMD_ATU_REGION_BASE		UL(0x40000000)
#define RDASPEN_SMD_ATU_REGION_SIZE		UL(0x10000000)
#define RDASPEN_MAP_SMD_ATU_REGION		MAP_REGION_FLAT(RDASPEN_SMD_ATU_REGION_BASE, \
							RDASPEN_SMD_ATU_REGION_SIZE, \
							MT_DEVICE | MT_RW | \
							MT_SECURE)

/* Trusted OS Config region */
#define PLAT_TOS_FW_CONFIG_BASE		UL(0x2800)
#define PLAT_TOS_FW_CONFIG_SIZE		UL(0x1000)

/* SPMC region */
#define PLAT_ARM_SPMC_BASE		UL(0xFFC00000)
#define PLAT_ARM_SPMC_SIZE		UL(0x00400000)

/*
 * NS DRAM
 * DRAM1 consists of Non-secure partition and SPMC Secure partition.
 */
#define NS_DRAM_SIZE			(ARM_DRAM1_SIZE - PLAT_ARM_SPMC_SIZE)

#define RDASPEN_MAP_NS_DRAM1		MAP_REGION_FLAT(ARM_DRAM1_BASE, \
							NS_DRAM_SIZE, \
							MT_MEMORY | MT_RW | \
							MT_NS)

#ifdef RD_ASPEN_VARIANT_FVP
#define RDASPEN_MAP_NS_DRAM2		MAP_REGION_FLAT(ARM_DRAM2_BASE, \
							ARM_DRAM2_SIZE,	\
							MT_MEMORY | MT_RW | \
							MT_NS)
#endif

/* Secure DRAM */
#define RDASPEN_MAP_S_DRAM		MAP_REGION_FLAT(PLAT_ARM_SPMC_BASE, \
							PLAT_ARM_SPMC_SIZE, \
							MT_MEMORY | MT_RW | \
							MT_SECURE)

/* load address of DTB */
#define PLAT_HW_CONFIG_DTB_BASE		ARM_DRAM1_BASE
#define PLAT_ARM_HW_CONFIG_SIZE		UL(0x8000)

/*
 * PSCI memory protect definitions:
 * This variable is stored in a non-secure flash because some ARM reference
 * platforms do not have secure NVRAM. Real systems that provided MEM_PROTECT
 * support must use a secure NVRAM to store the PSCI MEM_PROTECT definitions.
 */
#define PLAT_ARM_MEM_PROT_ADDR		(EXT_FLASH_BASE + \
					 EXT_FLASH_SIZE - \
					 EXT_FLASH_BLOCK_SIZE)

/* Map mem_protect flash region with read and write permissions */
#define RDASPEN_MAP_MEM_PROTECT		MAP_REGION_FLAT(PLAT_ARM_MEM_PROT_ADDR,	\
							EXT_FLASH_BLOCK_SIZE,	\
							MT_DEVICE | MT_RW | \
							MT_SECURE)

/* Non-volatile counters */
#define RDASPEN_TRUSTED_NVCTR_BASE	UL(0x32030000)
#define RDASPEN_TRUSTED_NVCTR_SIZE	UL(0x10000)
#define TFW_NVCTR_BASE			RDASPEN_TRUSTED_NVCTR_BASE
#define TFW_NVCTR_SIZE			4
#define NTFW_CTR_BASE			(RDASPEN_TRUSTED_NVCTR_BASE + 0x0004)
#define NTFW_CTR_SIZE			4

#define RDASPEN_MAP_TRUSTED_NVCTR	MAP_REGION_FLAT(RDASPEN_TRUSTED_NVCTR_BASE,	\
							RDASPEN_TRUSTED_NVCTR_SIZE,	\
							MT_DEVICE | MT_RO | \
							MT_SECURE)

/* 125 MHz REFCLK to System Counter */
#define SYS_COUNTER_FREQ_IN_TICKS	UL(125000000)

/* The index of the primary CPU */
#define RDASPEN_PRIMARY_CPU		0x0

/*
 * In the current implementation, the RoT Service request that requires the
 * biggest message buffer is the RSE_DELEGATED_ATTEST_GET_PLATFORM_TOKEN. The
 * maximum required buffer size is calculated based on the platform-specific
 * needs of this request.
 */
 #define PLAT_RSE_COMMS_PAYLOAD_MAX_SIZE	UL(0x1000)

/*******************************************************************************
 * MHUv3 related definitions
 ******************************************************************************/

/* AP - RSE MHUv3 */
#define MHU_V3_MBX_FRAME_OFFSET		UL(0x30000)

/* MHUv3 secure Postbox and Mailbox register frame base */
#define AP_RSE_SECURE_MHU_V3_PBX	RDASPEN_CSS_AP_RSE_SECURE_MHU_BASE
#define AP_RSE_SECURE_MHU_V3_MBX	RDASPEN_CSS_AP_RSE_SECURE_MHU_BASE + \
						MHU_V3_MBX_FRAME_OFFSET

/*
 * In the current implementation, the RoT Service request that requires the
 * biggest message buffer is the RSE_DELEGATED_ATTEST_GET_PLATFORM_TOKEN. The
 * maximum required buffer size is calculated based on the platform-specific
 * needs of this request.
 */
 #define PLAT_RSE_COMMS_PAYLOAD_MAX_SIZE	UL(0x1000)

/*
 * The SCMI PFDI Monitor memory regions
 */
#define RDASPEN_SCMI_PFDI_MONITOR_BASE			\
	(CSS_SCMI_PAYLOAD_BASE + CSS_SCMI_PAYLOAD_SIZE_MAX)
/*
 * The size of reserved shared memory for each core =
 * sizeof(mailbox_mem_t) + SCMI PFDI Monitor payload size =
 * 32 + 8 = 40 bytes
 */
#define RDASPEN_SCMI_PFDI_MONITOR_SIZE_PER_CHANNEL	40
#define RDASPEN_SCMI_PFDI_MONITOR_SIZE_MAX		\
	(RDASPEN_SCMI_PFDI_MONITOR_SIZE_PER_CHANNEL * PLAT_ARM_SCMI_PFDI_MONITOR_CHANNEL_COUNT)

#define PLAT_REBOOT_PRI     GIC_HIGHEST_SEC_PRIORITY
#define PLAT_EHF_DESC       EHF_PRI_DESC(PLAT_PRI_BITS, PLAT_REBOOT_PRI)

#endif  /* PLATFORM_DEF_H */
