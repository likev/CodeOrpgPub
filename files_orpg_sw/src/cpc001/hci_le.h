/******************************************************************
 *	hci_le.h is the header file for using the log-error	  *
 *	messaging services within the hci.			  *
 ******************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2011/10/03 21:47:47 $
 * $Id: hci_le.h,v 1.4 2011/10/03 21:47:47 ccalvert Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */

#ifndef HCI_LE_H
#define HCI_LE_H

#include <orpgerr.h>

/*	System include file definitions					*/

#include <infr.h>

#define	HCI_LE_ERROR_BIT	GL_ERROR_BIT
#define	HCI_LE_LOG		GL_INFO /* Go to task log */
#define	HCI_LE_STATUS		GL_INFO /* Go to task log */
#define	HCI_LE_ERROR		GL_INFO /* Go to task log */
#define	HCI_LE_STATUS_LOG	GL_STATUS /* Go to RPG status log */
#define	HCI_LE_ERROR_LOG	GL_ERROR /* Go to RPG error log */

#define	HCI_LE_MSG_MAX_LENGTH	LE_MAX_MSG_LENGTH

/*	RPG Alarms							*/
#define	HCI_LE_RPG_ALARM_MASK	LE_RPG_ALARM_TYPE_MASK
#define	HCI_LE_RPG_ALARM_LS	LE_RPG_AL_LS
#define	HCI_LE_RPG_ALARM_MAR	LE_RPG_AL_MAR
#define	HCI_LE_RPG_ALARM_MAM	LE_RPG_AL_MAM
#define	HCI_LE_RPG_ALARM_CLEAR	LE_RPG_AL_CLEARED
/*	RPG Status							*/
#define	HCI_LE_RPG_STATUS_MASK	LE_RPG_STATUS_TYPE_MASK
#define	HCI_LE_RPG_STATUS_WARN	LE_RPG_WARN_STATUS
#define	HCI_LE_RPG_STATUS_GEN	LE_RPG_GEN_STATUS
#define	HCI_LE_RPG_STATUS_INFO	LE_RPG_INFO_MSG
#define	HCI_LE_RPG_STATUS_COMMS	LE_RPG_COMMS
/*	RDA Alarms							*/
#define	HCI_LE_RDA_ALARM_MASK	LE_RDA_ALARM_TYPE_MASK
#define	HCI_LE_RDA_ALARM_NA	LE_RDA_AL_NOT_APP
#define	HCI_LE_RDA_ALARM_SEC	LE_RDA_AL_SEC
#define	HCI_LE_RDA_ALARM_MAR	LE_RDA_AL_MAR
#define	HCI_LE_RDA_ALARM_MAM	LE_RDA_AL_MAM
#define	HCI_LE_RDA_ALARM_INOP	LE_RDA_AL_INOP
#define	HCI_LE_RDA_ALARM_CLEAR	LE_RDA_AL_CLEARED

#endif
