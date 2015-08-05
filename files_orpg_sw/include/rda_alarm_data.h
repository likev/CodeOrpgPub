/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2003/02/20 17:47:51 $
 * $Id: rda_alarm_data.h,v 1.7 2003/02/20 17:47:51 steves Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */

/*************************************************************************

	Header defining the various data elements for RDA alarms.

**************************************************************************/

#ifndef RDA_STATUS_DATA_H
#define RDA_STATUS_DATA_H

#include <rda_status.h>

typedef struct {

	short	state;		/* Alarm state: Secondary, Maintenance     *
				 * Required, Maintenance Mandatory, or	   *
				 * Inoperable, Not Applicable.		   */
	short	type;		/* Alarm type: N/A, Edge Detected, 	   *
				 * Filtered, Occurrence.		   */
	short	device;		/* Alarm Device Category: Transmitter,     *
				 * Utility, Receiver/Signal Processor,     *
				 * Control, Pedestal, Archive, User, RPG,  *
				 * Wideband.				   */
	short	sample;		/* Alarm reporting count (Edge Detected).  */
	const char *message;	/* String containing Text message from	   *
				 * RDA/RPG ICD.				   */

} RDA_alarm_data_t;

/*	NOTE: Refer to the header file orpgrda.h for macro definitions	   *
 *	corresponding to state, type, and device fields.		   */

#endif
