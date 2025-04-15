/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <drivers/arm/css/css_mhu_doorbell.h>
#include <drivers/arm/css/css_scp.h>
#include <drivers/arm/css/scmi.h>
#include <drivers/arm/dsu.h>
#include <drivers/arm/pfdi_mod.h>
#include <plat/common/platform.h>

static scmi_channel_plat_info_t plat_rd_scmi_info[] = {
	{
		.scmi_mbx_mem = CSS_SCMI_PAYLOAD_BASE,
		.db_reg_addr = PLAT_CSS_MHU_BASE + MHU_V3_SENDER_REG_SET(0),
		.db_preserve_mask = 0xfffffffe,
		.db_modify_mask = 0x1,
		.ring_doorbell = &mhu_ring_doorbell,
	},
};

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
	return css_scmi_override_pm_ops(ops);
}

const dsu_driver_data_t plat_dsu_data = {
	.clusterpwrdwn_pwrdn = false,
	.clusterpwrdwn_memret = false,
	.clusterpwrctlr_cachepwr = CLUSTERPWRCTLR_CACHEPWR_RESET,
	.clusterpwrctlr_funcret = CLUSTERPWRCTLR_FUNCRET_RESET
};

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
