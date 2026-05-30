/*
 * Copyright (c) 2025-2026, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>

#include <plat/arm/common/plat_arm.h>
#include <plat/arm/css/common/css_pm.h>
#include <plat/common/platform.h>

#include <drivers/arm/css/css_mhu_doorbell.h>
#include <drivers/arm/css/css_scp.h>
#include <drivers/arm/css/scmi.h>
#include <drivers/arm/dsu.h>
#include <drivers/arm/mhu.h>
#include <drivers/arm/pfdi_mod.h>
#include <drivers/generic_delay_timer.h>
#include <apollo_fvp_ras.h>

/*
 * MHUv3 Trustzone Extension Support macros
 */
#define MHU_FEAT_SPT_OFFSET0		0x10
#define MHU_PBX_FEAT_TZE_SPT_SHIFT	12
#define MHU_PBX_FEAT_TZE_SPT_MASK	GENMASK_32(15, 12)
#define MHU_PBX_FEAT_TZE_SPT_VAL(val)	\
	(((val) & MHU_PBX_FEAT_TZE_SPT_MASK) >> MHU_PBX_FEAT_TZE_SPT_SHIFT)

#define SCMI_PFDI_MONITOR_INFO(channel_id)	\
{							\
		.scmi_mbx_mem = APOLLO_FVP_SCMI_PFDI_MONITOR_BASE + \
			(APOLLO_FVP_SCMI_PFDI_MONITOR_SIZE_PER_CHANNEL * channel_id), \
		.db_reg_addr = PFDI_MONITOR_MHU_BASE + MHU_V3_SENDER_REG_SET(channel_id), \
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

#if USE_GIC_DRIVER == 3
static const uintptr_t apollo_fvp_gicr_base_addrs[] = {
	GICR_BASE_VIEW0_0_0,
	GICR_BASE_VIEW0_0_1,
	GICR_BASE_VIEW0_0_2,
	GICR_BASE_VIEW0_0_3,
	GICR_BASE_VIEW0_1_0,
	GICR_BASE_VIEW0_1_1,
	GICR_BASE_VIEW0_1_2,
	GICR_BASE_VIEW0_1_3,
	GICR_BASE_VIEW0_2_0,
	GICR_BASE_VIEW0_2_1,
	GICR_BASE_VIEW0_2_2,
	GICR_BASE_VIEW0_2_3,
	GICR_BASE_VIEW0_3_0,
	GICR_BASE_VIEW0_3_1,
	GICR_BASE_VIEW0_3_2,
	GICR_BASE_VIEW0_3_3,
	0U				/* Zero Termination */
};
#endif

static bool is_mhuv3_tze_supported(uintptr_t mhuv3_dev_base)
{
	uint32_t feat_spt0 = mmio_read_32(mhuv3_dev_base + MHU_FEAT_SPT_OFFSET0);

	if (MHU_PBX_FEAT_TZE_SPT_VAL(feat_spt0) == 0x1)
		return true;
	return false;
}

scmi_channel_plat_info_t *plat_css_get_scmi_info(unsigned int channel_id)
{
	assert(channel_id == 0U);

	/* If TZ Extension support enabled, update PLAT_CSS_MHU_BASE offset */
	if (is_mhuv3_tze_supported(PLAT_CSS_MHU_BASE)) {
		plat_rd_scmi_info[channel_id].db_reg_addr +=
			MHU_SECURITY_CONTROL_BLOCK_OFFSET;
	}

	return &plat_rd_scmi_info[channel_id];
}

#if PFDI_SUPPORT
static int apollo_fvp_pwr_domain_on(u_register_t mpidr)
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
	ops->pwr_domain_on = apollo_fvp_pwr_domain_on;
#endif
	ops->pwr_domain_on_finish = apollo_fvp_css_pwr_domain_on_finish;
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
	gic_set_gicr_frames(apollo_fvp_gicr_base_addrs);
#endif
	generic_delay_timer_init();
	apollo_fvp_ras_init_per_cpu();
}

void apollo_fvp_bl31_plat_runtime_setup(void)
{
	/* Initialize the runtime console */
	arm_console_runtime_init();

	/* Configure the warm reboot SGI for primary core */
	css_setup_cpu_pwr_down_intr();

#if CSS_SYSTEM_GRACEFUL_RESET
	/* Register priority level handlers for reboot */
	ehf_register_priority_handler(PLAT_REBOOT_PRI,
			css_reboot_interrupt_handler);
#endif
}

void bl31_plat_runtime_setup(void)
{
	apollo_fvp_bl31_plat_runtime_setup();
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

	if (is_mhuv3_tze_supported(PFDI_MONITOR_MHU_BASE)) {
		plat_rd_scmi_pfdi_monitor_info[channel_id].db_reg_addr +=
			MHU_SECURITY_CONTROL_BLOCK_OFFSET;
	}

	return &plat_rd_scmi_pfdi_monitor_info[channel_id];
}
#endif
