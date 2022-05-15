/*
 *  linux/drivers/../../tusb422_notify.c
 *
 *  Copyright (C) 2006 Antonino Daplas <adaplas@pol.net>
 *
 *	2001 - Documented with DocBook
 *	- Brad Douglas <brad@neruo.com>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 */
#include <linux/notifier.h>

enum {
	/* TUSB TCP_NOTIFY_TYPE_MODE */
	TUSB_TCP_NOTIFY_ENTER_MODE,
	TUSB_TCP_NOTIFY_MODE_START = TUSB_TCP_NOTIFY_ENTER_MODE,
	TUSB_TCP_NOTIFY_EXIT_MODE,
	TUSB_TCP_NOTIFY_AMA_DP_STATE,
	TUSB_TCP_NOTIFY_AMA_DP_ATTENTION,
	TUSB_TCP_NOTIFY_AMA_DP_HPD_STATE,
	TUSB_TCP_NOTIFY_DC_EN_UNLOCK,
	TUSB_TCP_NOTIFY_UVDM,
	TUSB_TCP_NOTIFY_MODE_END = TUSB_TCP_NOTIFY_UVDM,

	/* TUSB TCP_NOTIFY_TYPE_VBUS */
	TUSB_TCP_NOTIFY_DIS_VBUS_CTRL,
	TUSB_TCP_NOTIFY_VBUS_START = TUSB_TCP_NOTIFY_DIS_VBUS_CTRL,
	TUSB_TCP_NOTIFY_SOURCE_VCONN,
	TUSB_TCP_NOTIFY_SOURCE_VBUS,
	TUSB_TCP_NOTIFY_SINK_VBUS,
	TUSB_TCP_NOTIFY_EXT_DISCHARGE,
	TUSB_TCP_NOTIFY_ATTACHWAIT_SNK,
	TUSB_TCP_NOTIFY_ATTACHWAIT_SRC,
	TUSB_TCP_NOTIFY_VBUS_END = TUSB_TCP_NOTIFY_ATTACHWAIT_SRC,

	/* TUSB TCP_NOTIFY_TYPE_USB */
	TUSB_TCP_NOTIFY_TYPEC_STATE,
	TUSB_TCP_NOTIFY_USB_START = TUSB_TCP_NOTIFY_TYPEC_STATE,
	TUSB_TCP_NOTIFY_PD_STATE,
	TUSB_TCP_NOTIFY_USB_END = TUSB_TCP_NOTIFY_PD_STATE,

	/* TUSB TCP_NOTIFY_TYPE_MISC */
	TUSB_TCP_NOTIFY_PR_SWAP,
	TUSB_TCP_NOTIFY_MISC_START = TUSB_TCP_NOTIFY_PR_SWAP,
	TUSB_TCP_NOTIFY_DR_SWAP,
	TUSB_TCP_NOTIFY_VCONN_SWAP,
	TUSB_TCP_NOTIFY_HARD_RESET_STATE,
	TUSB_TCP_NOTIFY_ALERT,
	TUSB_TCP_NOTIFY_STATUS,
	TUSB_TCP_NOTIFY_REQUEST_BAT_INFO,
	TUSB_TCP_NOTIFY_WD_STATUS,
	TUSB_TCP_NOTIFY_CABLE_TYPE,
	TUSB_TCP_NOTIFY_PLUG_OUT,
	TUSB_TCP_NOTIFY_MISC_END = TUSB_TCP_NOTIFY_PLUG_OUT,
};

enum tusb_typec_attach_type {
	TUSB_TYPEC_UNATTACHED = 0,
	TUSB_TYPEC_ATTACHED_SNK,
	TUSB_TYPEC_ATTACHED_SRC,
	TUSB_TYPEC_ATTACHED_AUDIO,
	TUSB_TYPEC_ATTACHED_DEBUG,			/* Rd, Rd */

/* CONFIG_TYPEC_CAP_DBGACC_SNK */
	TUSB_TYPEC_ATTACHED_DBGACC_SNK,		/* Rp, Rp */

/* CONFIG_TYPEC_CAP_CUSTOM_SRC */
	TUSB_TYPEC_ATTACHED_CUSTOM_SRC,		/* Same Rp */

/* CONFIG_TYPEC_CAP_NORP_SRC */
	TUSB_TYPEC_ATTACHED_NORP_SRC,		/* No Rp */
};
extern int tusb_register_client(struct notifier_block *nb);
extern int tusb_unregister_client(struct notifier_block *nb);
extern int tusb_notifier_call_chain(unsigned long val, void *v);


