/*
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <platform_def.h>
#include <bl31/interrupt_mgmt.h>
#include <cper.h>
#include <drivers/delay_timer.h>
#include <plat/common/platform.h>
#include <plat/arm/css/common/css_pm.h>
#include <platform_def.h>
#include <rdaspen_ras.h>
#include <lib/extensions/ras.h>

#define CORE_RAM_ERR_RECORD		U(1)
#define TFP_ERROR_SERR			U(0x1A)
#define TFP_ERROR_STRING_OFFSET		U(0x4)

/* Flop parity Error strings */
static const char *tfp_error_strings[] = {
	"DSIDE",
	"VECTOR_UNIT",
	"MMU",
	"LEVEL_2",
	"GIC_CPU_INTERFACE",
	"DBG_TRACE",
	"ISIDE",
	"DECODE",
	"RENAME",
	"COMMIT",
	"ISSUE",
	"IEXECUTE",
	"AXIS_BRIDGE"
};

/* Check if the TFP error source value is within valid range */
static inline bool rdaspen_is_valid_tfp_ierr(uint64_t tfp_source)
{
	return ((tfp_source >= TFP_ERROR_STRING_OFFSET) &&
			(tfp_source < (ARRAY_SIZE(tfp_error_strings)
					+ TFP_ERROR_STRING_OFFSET)));
}

static void rdaspen_check_tfp_error(uint64_t err_status)
{
	/* Check if the error status indicates a Transient Fault error */
	if ((err_status & ERX_STATUS_V) &&
		(ERX_STATUS_SERR(err_status) == TFP_ERROR_SERR)) {
		/* Check if source of TFP is valid */
		if (!rdaspen_is_valid_tfp_ierr(ERX_STATUS_IERR(err_status))) {
			WARN("CPU RAS: TFP Error Detected : Unknown Error\n");
		} else {
			/* Prints the TFP error source */
			WARN("CPU RAS: TFP Error Detected : %s\n",
				tfp_error_strings[ERX_STATUS_IERR(err_status)
					- TFP_ERROR_STRING_OFFSET]);
		}
	}
}

void plat_handle_uncontainable_ea(void)
{
	uint64_t esr = read_esr_el3();

	INFO("RAS: Uncontainable RAS Error Handler (EL3)\n");
	VERBOSE("RAS: ESR_EL3 = 0x%lx (EC = 0x%lx, FSC = 0x%lx)\n",
		esr, (esr >> 26) & 0x3F, esr & 0x3F);

	int ret = ras_ea_handler(0, esr, NULL, NULL, 0);

	if (ret == 0) {
		ERROR("RAS: Uncontainable RAS Error has no registered handler.\n");
		panic();
	}
}

/* This is a dummy implementation of probing on Uncontainable Errors required for handling EA */
static int32_t rdaspen_ras_record_probe(const struct err_record_info *info,
					int *probe_data)
{
	/* Skip probing since fault is probed in Handler */
	return 1;
}


/* Initialise CPU RAS features for FHI configuration */
static void rdaspen_setup_cpu_ras_config(void)
{
        uint64_t reg_erxctlr_el1 = 0;
        unsigned int core_pos = plat_my_core_pos();

	/* Select Error Record 1, Error record 0 is for the DSU */
	write_errselr_el1(CORE_RAM_ERR_RECORD);
	reg_erxctlr_el1 = read_erxctlr_el1();

	reg_erxctlr_el1 |= ERX_CTRL_FI_ENABLE | ERX_CTRL_CFI_ENABLE | ERX_CTRL_ED_ENABLE;
	/* Enable Transient Fault Protection error reporting */
	reg_erxctlr_el1 |= ERX_CTRL_TFPEN_ENABLE;
	VERBOSE("RAS: Transient Fault Protection enabled\n");

	write_erxctlr_el1(reg_erxctlr_el1);
	VERBOSE("RAS: Platform RAS Init on CPU %u : ERXCTLR_EL1=0x%lx successful\n",
		core_pos, reg_erxctlr_el1);
}

/*
 * Initialization function for the framework.
 *
 * Registers RAS config provided by the platform and then configures and
 * enables interrupt for each registered error.
 */
void rdaspen_ras_init_per_cpu(void)
{
	rdaspen_setup_cpu_ras_config();
}

/*
 * Extend default CPU power-on behavior.
 *
 * Wraps the default CSS pwr_domain_on_finish handler to ensure
 * platform-specific RAS initialization is performed on each CPU as it comes
 * online via PSCI.
 *
 */
void rdaspen_css_pwr_domain_on_finish(const psci_power_state_t *target_state)
{
	css_pwr_domain_on_finish(target_state);
	rdaspen_ras_init_per_cpu();
}

/*
 * Rising-edge SPI: clear any stale pending,
 * then set to trigger a new edge.
 */
static void rdaspen_set_error_notification_irq(unsigned int irq)
{
	plat_ic_clear_interrupt_pending(irq);
	plat_ic_set_interrupt_pending(irq);
	VERBOSE("RAS: IRQ %u triggered to EL1\n", irq);
}

static int rdaspen_ras_cpu_intr_handler(
	const struct err_record_info *err_rec,
	int probe_data,
	const struct err_handler_data *const data)
{
	uint32_t errx_status, mhuv3_poll_status;
	(void)err_rec;
	(void)probe_data;

	if (data == NULL)
		return -1;

	WARN("CPU RAS: Interrupt Received ID: 0x%x\n", data->interrupt);

	/* Warning: Incase Error Record is already cleared this prevents queued interrupts */
	if ((read_erxstatus_el1() & (ERX_STATUS_V)) == 0) {
		WARN("CPU RAS: Spurious RAS interrupt observed %x\n", data->interrupt);
		plat_ic_end_of_interrupt(data->interrupt);
		return -1;
	}

	WARN("CPU RAS: Error Status value : 0x%lx\n", read_erxstatus_el1());

	size_t esb_len =
		cper_write_cpu_record((void *)RDASPEN_CPER_BUF_BASE, RDASPEN_CPER_BUF_SIZE);

	if (esb_len) {
		print_cper((const void *)RDASPEN_CPER_BUF_BASE);
		flush_dcache_range(RDASPEN_CPER_BUF_BASE, esb_len);
		VERBOSE("RAS: ESB written len=%zu @0x%lx\n",
				esb_len, (unsigned long)RDASPEN_CPER_BUF_BASE);
	}

	rdaspen_check_tfp_error(read_erxstatus_el1());

	/* Initialise Timer with the Timeout value */
	uint64_t timeout = timeout_init_us(RAS_SYNC_TIMEOUT_US);

	/* Wait for Doorbell from SI - Make sure the SI have read the record */
	do {
		mhuv3_poll_status = mmio_read_32(RAS_SYNC_MHU_DB_STAT_REG_ADDR);
		if (mhuv3_poll_status & RAS_MHU_DB_MOD_MASK) {
			/* Clear the door bell immediately */
			/*
			 * NOTE: On FVP platforms, SI Cluster 0 doorbell interrupt handler is
			 * appreciably slower than on real hardware (it runs serially with
			 * the AP rather than in (true parallel), which can cause SI Cluster 0
			 * to remain blocked until timeout.
			 *
			 * if ACK (doorbell) is delayed by any verbose commands. To avoid
			 * this, we clear and then send the MHU doorbell ,immediately
			 * before issuing further commands.
			 *
			 * This is an FVP-specific timing optimization; real hardware
			 * implementations
			 * may not require it. Later on we can guard this behavior with a macro
			 * (e.g. CONFIG_PLATFORM_FVP)
			 * So that on non-FVP platforms the doorbell send can occur later if
			 * desired.
			 */
			mmio_write_32(RAS_SYNC_MHU_DB_STAT_CLR_ADDR, RAS_MHU_DB_MOD_MASK);
			errx_status = read_erxstatus_el1();
			write_erxstatus_el1(errx_status);
			clear_cpu_erx_misc0_register();
			/* Send an Ack to SI Cluster 0 about clearing the error record */
			MHU_RING_DOORBELL(RAS_SYNC_MHU_DB_SEND_REG_ADDR, RAS_MHU_DB_MOD_MASK,
					  RAS_MHU_DB_PRESERVE_MASK);
			WARN("CPU RAS: Doorbell rung from SI0 0x%x\n", mhuv3_poll_status);
			break;
		}

	} while (!timeout_elapsed(timeout));

	/* Clear the Error incase the error wasn't */
	if ((mhuv3_poll_status & RAS_MHU_DB_MOD_MASK) != RAS_MHU_DB_MOD_MASK) {
		errx_status = read_erxstatus_el1();
		write_erxstatus_el1(errx_status);
		clear_cpu_erx_misc0_register();
	}

#if FAULT_INJECTION_SUPPORT
	/* Pseudo generation registers are cleared to avoid interrupt flood from NS */
	clear_cpu_pfg_ctrl_register();
	/* Injected Errors cannot be stopped until these registers are cleared */
	clear_cpu_pfg_cdn_register();
#endif /* FAULT_INJECTION_SUPPORT */

	WARN("CPU RAS: Error Status Clear Value  : 0x%lx\n", read_erxstatus_el1());

	timeout = timeout_init_us(RAS_SYNC_TIMEOUT_US);
	/* Wait for SI clearing the SI door bell*/
	do {
		mhuv3_poll_status = mmio_read_32(RAS_SYNC_MHU_DB_STAT_SEND_ADDR);
		if (!(mhuv3_poll_status & RAS_MHU_DB_MOD_MASK)) {
			WARN("CPU RAS: SI Acknowledges doorbell\n");
			break;
		}
	} while (!timeout_elapsed(timeout));

	plat_ic_end_of_interrupt(data->interrupt);
	rdaspen_set_error_notification_irq(ERROR_NOTIFICATION_IRQ);
	return 0;
}

/* RAS error record list definition, used by the common RAS framework. */
static struct err_record_info plat_err_records[] = {
	ERR_RECORD_SYSREG_V1(0, 1, rdaspen_ras_record_probe, &rdaspen_ras_cpu_intr_handler, 0),
};

/* RAS error interrupt list definition, used by the common RAS framework. */
static struct ras_interrupt plat_ras_interrupts[] = {
	{
		/* This Fault IRQ is common for all Arm platforms - PPI 17 */
		.intr_number = CPU_FAULT_IRQ,
		.err_record = &plat_err_records[0],
	},
};

/* Registers the RAS error record list with common RAS framework. */
REGISTER_ERR_RECORD_INFO(plat_err_records);
/* Registers the RAS error interrupt info list with common RAS framework. */
REGISTER_RAS_INTERRUPTS(plat_ras_interrupts);
