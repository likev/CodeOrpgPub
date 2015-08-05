/************************************************************************
 *	hci_consts.h is the header file for common macros used by	*
 *	various HCI tasks.						*
 ************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2013/06/18 21:10:38 $
 * $Id: hci_consts.h,v 1.46 2013/06/18 21:10:38 steves Exp $
 * $Revision: 1.46 $
 * $State: Exp $
 */

#ifndef HCI_CONSTS_H
#define HCI_CONSTS_H

#define	BIT_0_MASK	0x0001
#define	BIT_1_MASK	0x0002
#define	BIT_2_MASK	0x0004
#define	BIT_3_MASK	0x0008
#define	BIT_4_MASK	0x0010
#define	BIT_5_MASK	0x0020
#define	BIT_6_MASK	0x0040
#define	BIT_7_MASK	0x0080
#define	BIT_8_MASK	0x0100
#define	BIT_9_MASK	0x0200
#define	BIT_10_MASK	0x0400
#define	BIT_11_MASK	0x0800
#define	BIT_12_MASK	0x1000
#define	BIT_13_MASK	0x2000
#define	BIT_14_MASK	0x4000
#define	BIT_15_MASK	0x8000

#define	HCI_ELEV_SR_BITMASK	VCP_HALFDEG_RAD

enum { NO, YES };

/*  Constants for messages in the ORPGDAT_SYSLOG_LATEST data store */

#define HCI_SYSLOG_LATEST_STATUS	1
#define HCI_SYSLOG_LATEST_ALARM		2

/*	The following list should contain a macro for all control	*
 *	panel objects.  The macro LAST_OBJECT should always terminate	*
 *	the list. This was taken out of hci_control_panel.h so other	*
 *	HCI tasks could access them.					*/

enum {TOP_WIDGET,
      DRAW_WIDGET,
      RDA_BUTTON,
      RDA_CONTROL_BUTTON,
      RDA_ALARMS_BUTTON,
      RPG_BUTTONS_BACKGROUND,
      RPG_ALGORITHMS_BUTTON,
      RPG_CONTROL_BUTTON,
      RPG_PRODUCTS_BUTTON,
      RPG_STATUS_BUTTON,
      RMS_BUTTON,
      RMS_CONTROL_BUTTON,
      PRODUCT_STATUS_BUTTON,
      PRODUCT_PARAMETERS_BUTTON,
      BASEDATA_BUTTON,
      CENSOR_ZONES_BUTTON,
      BYPASS_MAP_BUTTON,
      PRF_CONTROL_BUTTON,
      RDA_PERFORMANCE_BUTTON,
      CONSOLE_MESSAGE_BUTTON,
      ENVIRONMENTAL_WINDS_BUTTON,
      BLOCKAGE_BUTTON,
      MISC_BUTTON,
      ALERTS_BUTTON,
      ARCHIVE_III_BUTTON,
      USERS_BUTTON,
      COMMS_BUTTON,
      DISTRIBUTION_CONTROL_BUTTON,
      PUP_STATUS_BUTTON,
      RADOME_OBJECT,
      TOWER_OBJECT,
      POWER_OBJECT,
      PRECIP_STATUS_OBJECT,
      MODE_STATUS_OBJECT,
      PRFMODE_STATUS_OBJECT,
      SUPER_RES_STATUS_OBJECT,
      CMD_STATUS_OBJECT,
      AVSET_STATUS_OBJECT,
      ENW_STATUS_OBJECT,
      LOAD_SHED_OBJECT,
      RDA_ALARM1_OBJECT,
      RDA_ALARM2_OBJECT,
      RDA_ALARM3_OBJECT,
      RDA_ALARM4_OBJECT,
      RDA_ALARM5_OBJECT,
      RDA_ALARM6_OBJECT,
      RDA_ALARM7_OBJECT,
      RDA_ALARM8_OBJECT,
      WIDEBAND_OBJECT,
      NARROWBAND_OBJECT,
      VCP_CONTROL_OBJECT,
      RDA_INHIBIT_OBJECT,
      FAA_REDUNDANT_OBJECT,
      CLEAR_AIR_SWITCH_OBJECT,
      PRECIP_SWITCH_OBJECT,
      RESTORE_ADAPT_BUTTON,
      SAVE_ADAPT_BUTTON,
      MERGE_ADAPT_BUTTON,
      HCI_PASSWORD_BUTTON,
      SAVE_LOG_BUTTON,
      HARDWARE_CONFIG_BUTTON,
      VIEW_LOG_BUTTON,
      MODEL_EWT_STATUS_OBJECT,
      PERFCHECK_STATUS_OBJECT,
      SAILS_STATUS_OBJECT,
      MODEL_DATA_VIEWER_BUTTON,
      LAST_OBJECT};

/*      RDA power source macros         */

enum {UTILITY_POWER, AUXILLIARY_POWER};

/*	RDA alarms device masks		*/

#define ARC_MASK        0x0001  /* Archive II device mask */
#define CTR_MASK        0x0002  /* RDA Control device mask */
#define PED_MASK        0x0004  /* Antenna/Pedestal device mask */
#define RSP_MASK        0x0010  /* Receiver/Signal Processor device mask */
#define USR_MASK        0x0020  /* User link device mask */
#define UTL_MASK        0x0040  /* Tower/Utilities device mask */
#define XMT_MASK        0x0080  /* Transmitter device mask */
#define WID_MASK        0x0100  /* widebank link device mask */
#define COM_MASK        0x0200  /* widebank link device mask */
#define RCV_MASK        0x0400  /* widebank link device mask */
#define SIG_MASK        0x0800  /* widebank link device mask */

#endif
