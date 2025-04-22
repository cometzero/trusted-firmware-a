/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <drivers/arm/css/css_mhu_doorbell.h>
#include <drivers/arm/css/css_scp.h>
#include <drivers/generic_delay_timer.h>
#include <drivers/arm/css/scmi.h>
#include <drivers/arm/dsu.h>
#include <drivers/arm/pfdi_mod.h>
#include <plat/arm/common/plat_arm.h>
#include <plat/common/platform.h>
#include <rdaspen_ras.h>

#define SCMI_PFDI_MONITOR_CHANNEL_BASE	1U

#define SCMI_PFDI_MONITOR_INFO(channel_id)	\
{							\
		.scmi_mbx_mem = RDASPEN_SCMI_PFDI_MONITOR_BASE + \
			(RDASPEN_SCMI_PFDI_MONITOR_SIZE_PER_CHANNEL * channel_id), \
		.db_reg_addr = PLAT_CSS_MHU_BASE + MHU_V3_SENDER_REG_SET(SCMI_PFDI_MONITOR_CHANNEL_BASE + channel_id), \
		.db_preserve_mask = 0xfffffffe, \
		.db_modify_mask = 0x1, \
		.ring_doorbell = &mhu_ring_doorbell,	\
}

static scmi_channel_plat_info_t plat_rd_scmi_info[] = {
	{
		.scmi_mbx_mem = CSS_SCMI_PAYLOAD_BASE,
		.db_reg_addr = PLAT_CSS_MHU_BASE + MHU_V3_SENDER_REG_SET(0),
		.db_preserve_mask = 0xfffffffe,
		.db_modify_mask = 0x1,
		.ring_doorbell = &mhu_ring_doorbell,
	},
};

#if SCMI_PFDI_MONITOR
static scmi_channel_plat_info_t plat_rd_scmi_pfdi_monitor_info[] = {
	SCMI_PFDI_MONITOR_INFO(0),
	SCMI_PFDI_MONITOR_INFO(1),
	SCMI_PFDI_MONITOR_INFO(2),
	SCMI_PFDI_MONITOR_INFO(3),
	SCMI_PFDI_MONITOR_INFO(4),
	SCMI_PFDI_MONITOR_INFO(5),
	SCMI_PFDI_MONITOR_INFO(6),
	SCMI_PFDI_MONITOR_INFO(7),
	SCMI_PFDI_MONITOR_INFO(8),
	SCMI_PFDI_MONITOR_INFO(9),
	SCMI_PFDI_MONITOR_INFO(10),
	SCMI_PFDI_MONITOR_INFO(11),
	SCMI_PFDI_MONITOR_INFO(12),
	SCMI_PFDI_MONITOR_INFO(13),
	SCMI_PFDI_MONITOR_INFO(14),
	SCMI_PFDI_MONITOR_INFO(15),
};
#endif

scmi_channel_plat_info_t *plat_css_get_scmi_info(unsigned int channel_id)
{
	assert(channel_id == 0U);
	return &plat_rd_scmi_info[channel_id];
}

#if PFDI_SUPPORT
static int rdaspen_pwr_domain_on(u_register_t mpidr)
{
	uint64_t ft_id;
	uint64_t cpu_num = plat_core_pos_by_mpidr(mpidr);
	pfdi_status_t pfdi_status = pfdi_func_desc.result(cpu_num, &ft_id);
	/*
	 * The core can only boot if the OoR PFDI tests have not failed
	 * or if the OoR PFDI tests have not been run yet.
	 */
	if ((pfdi_status != PFDI_SUCCESS) && (pfdi_status != PFDI_NOT_RUN)) {
		return PSCI_E_INTERN_FAIL;
	}

	css_scp_on(mpidr);

	return PSCI_E_SUCCESS;
}
#endif

const plat_psci_ops_t *plat_arm_psci_override_pm_ops(plat_psci_ops_t *ops)
{
#if PFDI_SUPPORT
	ops->pwr_domain_on = rdaspen_pwr_domain_on;
#endif
	ops->pwr_domain_on_finish = rdaspen_css_pwr_domain_on_finish;
	return css_scmi_override_pm_ops(ops);
}

const dsu_driver_data_t plat_dsu_data = {
	.clusterpwrdwn_pwrdn = false,
	.clusterpwrdwn_memret = false,
	.clusterpwrctlr_cachepwr = CLUSTERPWRCTLR_CACHEPWR_RESET,
	.clusterpwrctlr_funcret = CLUSTERPWRCTLR_FUNCRET_RESET
};

void bl31_platform_setup(void)
{
	arm_bl31_platform_setup();
#if USE_GIC_DRIVER == 3
	gic_set_gicr_frames(arm_gicr_base_addrs);
#endif
	generic_delay_timer_init();
	rdaspen_ras_init_per_cpu();
}

#if defined(SPD_spmd) && (SPMC_AT_EL3 == 0)
/*
 * A dummy implementation of the platform handler for Group0 secure interrupt.
 */
int plat_spmd_handle_group0_interrupt(uint32_t intid)
{
	(void)intid;
	return -1;
}
#endif /* defined(SPD_spmd) && (SPMC_AT_EL3 == 0) */

#if SCMI_PFDI_MONITOR
scmi_channel_plat_info_t *plat_css_get_scmi_pfdi_monitor_info(unsigned int channel_id)
{
	assert(channel_id < PLAT_ARM_SCMI_PFDI_MONITOR_CHANNEL_COUNT);
	return &plat_rd_scmi_pfdi_monitor_info[channel_id];
}
#endif
