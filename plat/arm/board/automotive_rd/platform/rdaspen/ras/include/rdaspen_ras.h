/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef RDASPEN_RAS_H
#define RDASPEN_RAS_H

#include <lib/extensions/ras.h>
#include <plat/common/platform.h>
#include <platform_def.h>
#include <drivers/arm/css/css_mhu_doorbell.h>

/*
 * CPU IRQ RAS Defines
 */
#define CPU_FAULT_IRQ				PLAT_CORE_FAULT_IRQ

/* CPU RAS Control Register Defines */
#define ERX_CTRL_CFI_ENABLE			U(1UL << 8)
#define ERX_CTRL_FI_ENABLE			U(1UL << 3)
#define ERX_CTRL_ED_ENABLE			U(1UL << 0)
#define ERX_CTRL_TFPEN_ENABLE			U(1UL << 33)

/* CPU RAS STATUS Register Defines */
#define ERX_STATUS_V				U(1UL << 30)
#define ERX_STATUS_CE				U(1UL << 25)

/* RAS Error status register IERR value */
#define ERX_STATUS_IERR(err_status)		U(((err_status) >> 8) & 0x1F)
/* RAS Error status register SERR value */
#define ERX_STATUS_SERR(err_status)		U((err_status) & 0x1F)

/*
 * Timeout Constants for RAS Event Handling
 */
#define RAS_SYNC_TIMEOUT_US			(1U * 1000U) /* 1 msec */

/*
 * RAS Sync MHU constants, Note: Flag 1 is used , Flag 0 is reserved for SCMI
 */
/* This offset is RD-Aspen Specific b/w Sender and Receiver Pair */
#define MHU_DEV_RECV_OFFSET		UL(0x40000)
#define RAS_SYNC_SEND_MHU_BASE		SI_MHU_REGION_BASE
#define RAS_SYNC_RCV_MHU_BASE		(SI_MHU_REGION_BASE + MHU_DEV_RECV_OFFSET)
#define RAS_SYNC_MHU_CHANNEL		U(0x0)
#define RAS_MHU_DB_SENDER_REG_SET	MHU_V3_SENDER_REG_SET(RAS_SYNC_MHU_CHANNEL)
#define RAS_MHU_DB_SENDER_REG_STAT	(MHU_V3_PBX_PDBCW_PAGE_OFFSET + \
					SENDER_REG_STAT(RAS_SYNC_MHU_CHANNEL))
#define RAS_MHU_DB_RECV_REG_CLR		(MHU_V3_PBX_PDBCW_PAGE_OFFSET + \
					RECIEVER_REG_CLR(RAS_SYNC_MHU_CHANNEL))

#define RAS_MHU_DB_MOD_MASK		U(0x2)
/* Based on Channel 0 - Bit 1 not being preserved */
#define RAS_MHU_DB_PRESERVE_MASK	UL(0xFFFFFFFD)
#define RAS_SYNC_MHU_DB_SEND_REG_ADDR	(RAS_SYNC_SEND_MHU_BASE + RAS_MHU_DB_SENDER_REG_SET)
#define RAS_SYNC_MHU_DB_STAT_REG_ADDR	(RAS_SYNC_RCV_MHU_BASE + RAS_MHU_DB_SENDER_REG_STAT)
#define RAS_SYNC_MHU_DB_STAT_CLR_ADDR	(RAS_SYNC_RCV_MHU_BASE + RAS_MHU_DB_RECV_REG_CLR)
#define RAS_SYNC_MHU_DB_STAT_SEND_ADDR	(RAS_SYNC_SEND_MHU_BASE + RAS_MHU_DB_SENDER_REG_STAT)

/* Assembly helpers for CPU RAS Registers */
void clear_cpu_pfg_ctrl_register(void);
void clear_cpu_pfg_cdn_register(void);
void clear_cpu_erx_misc0_register(void);

void rdaspen_ras_init_per_cpu(void);
void rdaspen_css_pwr_domain_on_finish(const psci_power_state_t *target_state);

#endif /* RDASPEN_RAS_H */
