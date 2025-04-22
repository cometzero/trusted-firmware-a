/*
 * Copyright (c) 2025, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>

#include <arch_helpers.h>
#include <common/debug.h>
#include <drivers/arm/css/scmi.h>
#include <drivers/arm/pfdi_mod.h>
#include <plat/arm/common/plat_arm.h>

#include "scmi_private.h"

/* SCMI PFDI Monitor Protocol Version */
#define SCMI_PFDI_MONITOR_PROTOCOL_VERSION MAKE_SCMI_VERSION   (2, 0)
/* SCMI PFDI Monitor protocol ID */
#define SCMI_PFDI_MONITOR_PROTOCOL_ID 			      0x90
/* SCMI PFDI Monitor Message ID */
#define SCMI_PFDI_MONITOR_OOR_STATUS_MESSAGE_ID		      0x3
#define SCMI_PFDI_MONITOR_ONL_STATUS_MESSAGE_ID		      0x4
/* SCMI PFDI Monitor Message Length */
#define SCMI_PFDI_MONITOR_OOR_STATUS_MESSAGE_LEN	      (2 * sizeof(uint32_t))
#define SCMI_PFDI_MONITOR_ONL_STATUS_MESSAGE_LEN	      (2 * sizeof(uint32_t))

#if HW_ASSISTED_COHERENCY
#define scmi_lock_init(lock)
#define scmi_lock_get(lock)		spin_lock(lock)
#define scmi_lock_release(lock)		spin_unlock(lock)
#else
#define scmi_lock_init(lock)		bakery_lock_init(lock)
#define scmi_lock_get(lock)		bakery_lock_get(lock)
#define scmi_lock_release(lock)		bakery_lock_release(lock)
#endif

#if HW_ASSISTED_COHERENCY
static spinlock_t scmi_pfdi_monitor_lock[PLAT_ARM_SCMI_PFDI_MONITOR_CHANNEL_COUNT];
#else
static DEFINE_BAKERY_LOCK(scmi_pfdi_monitor_lock[PLAT_ARM_SCMI_PFDI_MONITOR_CHANNEL_COUNT]);
#endif

/*
 * The global handles for invoking the SCMI PFDI Monitor APIs after the driver
 * has been initialized.
 */
static void *scmi_pfdi_monitor_handles[PLAT_ARM_SCMI_PFDI_MONITOR_CHANNEL_COUNT];

/* The global SCMI PFDI Monitor channels array */
static scmi_channel_t scmi_pfdi_monitor_channels[PLAT_ARM_SCMI_PFDI_MONITOR_CHANNEL_COUNT];


static void scmi_pfdi_monitor_oor_status(uint32_t cpu_num, pfdi_status_t status)
{
	mailbox_mem_t *mbx_mem;
	int token = 0;
	scmi_channel_t *ch = (scmi_channel_t *)scmi_pfdi_monitor_handles[cpu_num];

	validate_scmi_channel(ch);

	scmi_get_channel(ch);

	mbx_mem = (mailbox_mem_t *)(ch->info->scmi_mbx_mem);
	mbx_mem->msg_header = SCMI_MSG_CREATE(SCMI_PFDI_MONITOR_PROTOCOL_ID,
			SCMI_PFDI_MONITOR_OOR_STATUS_MESSAGE_ID, token);
	mbx_mem->len = SCMI_PFDI_MONITOR_OOR_STATUS_MESSAGE_LEN;
	mbx_mem->flags = SCMI_FLAG_RESP_POLL;
	mbx_mem->payload[0] = status;

	scmi_send_sync_command(ch);

	scmi_put_channel(ch);
}

static void scmi_pfdi_monitor_onl_status(uint32_t cpu_num, pfdi_status_t status)
{
	mailbox_mem_t *mbx_mem;
	int token = 0;
	scmi_channel_t *ch = (scmi_channel_t *)scmi_pfdi_monitor_handles[cpu_num];

	validate_scmi_channel(ch);

	scmi_get_channel(ch);

	mbx_mem = (mailbox_mem_t *)(ch->info->scmi_mbx_mem);
	mbx_mem->msg_header = SCMI_MSG_CREATE(SCMI_PFDI_MONITOR_PROTOCOL_ID,
			SCMI_PFDI_MONITOR_ONL_STATUS_MESSAGE_ID, token);
	mbx_mem->len = SCMI_PFDI_MONITOR_ONL_STATUS_MESSAGE_LEN;
	mbx_mem->flags = SCMI_FLAG_RESP_POLL;
	mbx_mem->payload[0] = status;

	scmi_send_sync_command(ch);

	scmi_put_channel(ch);
}

static void *scmi_pfdi_monitor_init(scmi_channel_t *ch)
{
	uint32_t version;
	int ret;

	assert(ch && ch->info);
	assert(ch->info->db_reg_addr);
	assert(ch->info->db_modify_mask);
	assert(ch->info->db_preserve_mask);
	assert(ch->info->ring_doorbell != NULL);

	assert(ch->lock);

	scmi_lock_init(ch->lock);

	ch->is_initialized = 1;

	ret = scmi_proto_version(ch, SCMI_PFDI_MONITOR_PROTOCOL_ID, &version);
	if (ret != SCMI_E_SUCCESS) {
		WARN("SCMI PFDI Monitor protocol version message failed\n");
		goto error;
	}

	if (!is_scmi_version_compatible(SCMI_PFDI_MONITOR_PROTOCOL_VERSION, version)) {
		WARN("SCMI PFDI Monitor protocol version 0x%x incompatible with driver version 0x%x\n",
			version, SCMI_AP_CORE_PROTO_VER);
		goto error;
	}
	VERBOSE("SCMI PFDI Monitor protocol version 0x%x detected\n", version);

	return (void *)ch;

error:
	ch->is_initialized = 0;
	return NULL;
}

static void scmi_pfdi_monitor_post_run(pfdi_status_t status, uint64_t start __unused,
				uint64_t end __unused, uint64_t mode,
				uint64_t *ft_id __unused)
{
	uint64_t cpu_num = plat_my_core_pos();

	if (mode == PFDI_OOR_MODE) {
		scmi_pfdi_monitor_oor_status(cpu_num, status);
	} else {
		scmi_pfdi_monitor_onl_status(cpu_num, status);
	}
}


void plat_scmi_pfdi_monitor_setup(void)
{
	unsigned int idx;

	NOTICE("Initializing SCMI PFDI Monitor driver\n");

	for (idx = 0; idx < PLAT_ARM_SCMI_PFDI_MONITOR_CHANNEL_COUNT; idx++) {
		scmi_pfdi_monitor_channels[idx].info = plat_css_get_scmi_pfdi_monitor_info(idx);
		scmi_pfdi_monitor_channels[idx].lock = &scmi_pfdi_monitor_lock[idx];
		scmi_pfdi_monitor_handles[idx] = scmi_pfdi_monitor_init(&scmi_pfdi_monitor_channels[idx]);

		if (scmi_pfdi_monitor_handles[idx] == NULL) {
			ERROR("SCMI PFDI Monitor Initialization failed on channel %d\n", idx);
			panic();
		}
	}

	plat_pfdi_func_desc.post_run = scmi_pfdi_monitor_post_run;
}
