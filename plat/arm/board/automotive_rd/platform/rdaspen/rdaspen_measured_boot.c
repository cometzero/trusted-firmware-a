/*
 * Copyright (c) 2025-2026, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>
#include <drivers/arm/css/sds.h>
#include <drivers/measured_boot/metadata.h>
#include <drivers/measured_boot/rse/rse_measured_boot.h>
#include <plat/arm/common/plat_arm.h>
#include <tools_share/tbbr_oid.h>
#include <tools_share/zero_oid.h>

#include "rdaspen_rse_comms.h"

#ifndef SDS_SCP_AP_REGION_ID
#define SDS_SCP_AP_REGION_ID U(0)
#endif

static bool rdaspen_skip_mboot;

/*
 * Platform specific table with image IDs and metadata. Intentionally not a
 * const struct, some members might set by bootloaders during trusted boot.
 */
struct rse_mboot_metadata rdaspen_rse_mboot_metadata[] = {
	{
		.id = FW_CONFIG_ID,
		.slot = U(8),
		.signer_id_size = SIGNER_ID_MIN_SIZE,
		.sw_type = MBOOT_FW_CONFIG_STRING,
		.lock_measurement = true,
		.pk_oid = ZERO_OID
	},
	{
		.id = HW_CONFIG_ID,
		.slot = U(9),
		.signer_id_size = SIGNER_ID_MIN_SIZE,
		.sw_type = MBOOT_HW_CONFIG_STRING,
		.lock_measurement = true,
		.pk_oid = HW_CONFIG_KEY_OID
	},
	{
		.id = BL31_IMAGE_ID,
		.slot = U(10),
		.signer_id_size = SIGNER_ID_MIN_SIZE,
		.sw_type = MBOOT_BL31_IMAGE_STRING,
		.lock_measurement = true,
		.pk_oid = BL31_IMAGE_KEY_OID
	},
	{
		.id = BL32_IMAGE_ID,
		.slot = U(11),
		.signer_id_size = SIGNER_ID_MIN_SIZE,
		.sw_type = MBOOT_BL32_IMAGE_STRING,
		.lock_measurement = true,
		.pk_oid = BL32_IMAGE_KEY_OID
	},
	{
		.id = BL33_IMAGE_ID,
		.slot = U(12),
		.signer_id_size = SIGNER_ID_MIN_SIZE,
		.sw_type = MBOOT_BL33_IMAGE_STRING,
		.lock_measurement = true,
		.pk_oid = BL33_IMAGE_KEY_OID
	},
	{
		.id = RSE_MBOOT_INVALID_ID
	}
};

static bool rdaspen_is_measured_boot_skippable(void)
{
	int ret;
	uint32_t reset_syndrome = 0;

	ret = sds_init(SDS_SCP_AP_REGION_ID);
	if (ret != SDS_OK) {
		WARN("SDS init failed (%d), continuing measured boot\n", ret);
		return false;
	}

	ret = sds_struct_read(SDS_SCP_AP_REGION_ID,
			      SDS_RESET_SYNDROME_STRUCT_ID,
			      SDS_RESET_SYNDROME_OFFSET,
			      &reset_syndrome,
			      SDS_RESET_SYNDROME_SIZE,
			      SDS_ACCESS_MODE_NON_CACHED);
	if (ret != SDS_OK) {
		WARN("SDS reset syndrome read failed (%d), continuing measured boot\n",
		     ret);
		return false;
	}

	INFO("SDS reset syndrome = 0x%x\n", reset_syndrome);

	if ((reset_syndrome & SDS_RESET_SYNDROME_SYS_RESET_REQ_BIT) != 0U) {
		INFO("Warm reset syndrome detected, measured boot will be skipped\n");
		return true;
	}

	return false;
}

void bl2_plat_mboot_init(void)
{
	/* Initialize the communication channel between AP and RSE */
	(void)plat_rse_comms_init();

	/* Evaluate skip condition according to reset syndrome value */
	rdaspen_skip_mboot = rdaspen_is_measured_boot_skippable();

	rse_measured_boot_init(rdaspen_rse_mboot_metadata);
}

void bl2_plat_mboot_finish(void)
{
	/* Nothing to do. */
}

int plat_mboot_measure_image(unsigned int image_id, image_info_t *image_data)
{
	int err;

	/* Skipping image measurement in case of warm reset */
	if (rdaspen_skip_mboot) {
		return 0;
	}

	/* Calculate image hash and record data in RSE */
	err = rse_mboot_measure_and_record(rdaspen_rse_mboot_metadata,
					   image_data->image_base,
					   image_data->image_size,
					   image_id);
	if (err != 0) {
		ERROR("Measure and record failed for image id %u, err (%i)\n",
		      image_id, err);
	}

	return err;
}

int plat_mboot_measure_key(const void *pk_oid, const void *pk_ptr,
			   size_t pk_len)
{
	return rse_mboot_set_signer_id(rdaspen_rse_mboot_metadata, pk_oid, pk_ptr,
				       pk_len);
}
