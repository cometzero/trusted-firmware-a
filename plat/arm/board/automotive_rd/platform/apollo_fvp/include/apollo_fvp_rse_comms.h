/*
 * Copyright (c) 2026, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef APOLLO_FVP_RSE_COMMS_H
#define APOLLO_FVP_RSE_COMMS_H

/*
 * Initialize the communication channel between AP and RSE.
 * This is a platform-specific implementation for Apollo FVP,
 * responsible for setting up the MHU-based mailbox used by
 * the AP to communicate with the RSE.
 */
int plat_rse_comms_init(void);

#endif /* APOLLO_FVP_RSE_COMMS_H */
