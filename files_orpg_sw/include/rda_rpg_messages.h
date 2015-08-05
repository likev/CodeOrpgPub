/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2008/03/24 22:28:15 $
 * $Id: rda_rpg_messages.h,v 1.5 2008/03/24 22:28:15 steves Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */  
/************************************************************************

	Header file defining the various data message types as defined
	in the Interface Control Document (ICD) for the RDA/RPG.

*************************************************************************/


#ifndef RDA_RPG_MESSAGES_H
#define RDA_RPG_MESSAGES_H


/*			RAD/RPG Message Header				*
 *									*
 *	rda_rpg_message_header.h defines the RDA/RPG Message Header	*
 *	data.								*/

#include <rda_rpg_message_header.h>


/*			Message Type 1					*
 *									*
 *	basedata.h defines the Digital Radar Data message type		*/

#define	MESSAGE_TYPE_DIGITAL_RADAR_DATA		1
#include <basedata.h>


/*			Message Type 2					*
 *									*
 *	rda_status.h defines the RDA Status Data message type		*/

#define	MESSAGE_TYPE_RDA_STATUS			2
#include <rda_status.h>


/*			Message Type 3					*
 *									*
 *	rda_performance_maintenance.h defines the Performance		*
 *	Maintenance Data message type					*/

#define	MESSAGE_TYPE_PERFORMANCE_MAINTENANCE	3
#include <rda_performance_maintenance.h>


/*			Message Types 4	& 10				*
 *									*
 *	rda_rpg_console_message.h defines the Console Message (RDA to	*
 *	RPG and RPG to RDA) message types.				*/

#define	MESSAGE_TYPE_CONSOLE_RDA_TO_RPG		4
#define	MESSAGE_TYPE_CONSOLE_RPG_TO_RDA		10

#include <rda_rpg_console_message.h>

/*			Message Type 5					*
 *									*
 *	rpg_vcp.h defines the Volume Coverage Pattern message type	*/

#define	MESSAGE_TYPE_RDA_VCP			5


/*			Message Type 6					*
 *									*
 *	rda_control.h defines the RDA Control Commands message type	*/

#define	MESSAGE_TYPE_RDA_CONTROL		6
#include <rda_control.h>


/*			Message Type 7					*
 *									*
 *	rpg_vcp.h defines the Volume Control Pattern message type	*/

#define	MESSAGE_TYPE_VCP			7
#include <rpg_vcp.h>


/*			Message Type 8					*
 *									*
 *	rpg_clutter_censor_zones.h defines the Clutter Sensor Zones	*
 *	message type							*/

#define	MESSAGE_TYPE_CLUTTER_CENSOR_ZONES	8
#include <rpg_clutter_censor_zones.h>


/*			Message Type 9					*
 *									*
 *	rpg_request_data.h defines the Request for Data message type	*/

#define	MESSAGE_TYPE_REQUEST_DATA		9
#include <rpg_request_data.h>


/*			Message Types 11 & 12				*
 *									*
 *	rda_loop_back.h defines the Loop Back Test (RDA to RPG and	*
 *	message type						*/

#define MESSAGE_TYPE_LOOP_BACK_RDA_TO_RPG	11
#define	MESSAGE_TYPE_LOOP_BACK_RPG_TO_RDA	12

#include <rda_rpg_loop_back.h>


/*			Message Types 13 & 14				*
 *									*
 *	rda_rpg_clutter_map.h defines the Clutter Filter Bypass		*
 *	Map (RDA to RPG) and Edited Clutter Filter Bypass Map message	*
 *	types.								*/

#define	MESSAGE_TYPE_CLUTTER_FILTER_RDA_TO_RPG	13
#define	MESSAGE_TYPE_CLUTTER_FILTER_RPG_TO_RDA	14

#include <rda_rpg_clutter_map.h>


/*			Message Type 15					*
 *									*
 *	rda_notch_width_map.h defines the Clutter filter Notch Width	*
 *	Map message type.						*/
 
#define	MESSAGE_TYPE_NOTCH_WIDTH_MAP	15 
#include <rda_notch_width_map.h>

/*			Message Type 31					*
 *									*
 *	generic_basedata.h defines the Generic Digital Radar Data	*
 *	message type.							*/

#define	MESSAGE_TYPE_GENERIC_DIGITAL_RADAR_DATA		31
#include <generic_basedata.h>

#endif
