/************************************************************************
 *									* 
 *	Module:  hci_get_rms_status.c					*
 *									* 
 *	Description: This module sets up the RPG Control/Status window	*
 *		     to include or remove the rms button and rms console*
 *		     button.						*
 *									*
 ************************************************************************/
/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/02/27 22:26:08 $
 * $Id: hci_get_rms_status.c,v 1.5 2009/02/27 22:26:08 ccalvert Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */

/*	Local include file definitions.					*/

#include <hci_control_panel.h>

/*
* Static Globals
*/

static int rms_down;

/*
* Static Function Prototypes
*/

/************************************************************************
 *	Description:  This function sets the rms down flag.		*
 *									* 
 *	Input:  rms_state - RMS down flag				*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void hci_set_rms_down_flag (int rms_state){
	
	rms_down = rms_state;

}

/************************************************************************
 *	Description:  This function gets the rms down flag.		*
 *									* 
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: RMS down flag						*
 ************************************************************************/

int hci_get_rms_down_flag (){

	return (rms_down);

}

