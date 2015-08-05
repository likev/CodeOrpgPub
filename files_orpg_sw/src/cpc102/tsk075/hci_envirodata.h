/*	hci_envirodata.h - This header file defines		*
 *	functions used to display environmental model data.	*/

/*
 * RCCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/03/11 18:26:04 $
 * $Id: hci_envirodata.h,v 1.2 2014/03/11 18:26:04 steves Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

#ifndef HCI_ENVIRODATA_DEF
#define	HCI_ENVIRODATA_DEF

/*	Include files needed.						*/

#include <hci.h>
#include <basedata.h>

#define	TEMPERATURE			0
#define	REL_HUMIDITY			1
#define	SFC_PRESSURE			2

#define	SIZEOF_ENVIRODATA	(4000*sizeof(float))

#define	TEMPERATURE_MIN			 -60.0  /* Lowest displayable temperature */
#define	TEMPERATURE_MAX			  50.0  /* Highest displayable temperature */ 
#define	REL_HUMIDITY_MIN		   0.0  /* Lowest displayable relative humidity */
#define	REL_HUMIDITY_MAX		 100.0  /* Highest displayable relative humidity */
#define	SFC_PRESSURE_MIN		 650.0  /* Lowest displayable surface pressure */
#define	SFC_PRESSURE_MAX	        1050.0  /* Highest displayable surface pressure */

#define	MAX_RANGE			460.0

void	hci_envirodata( Widget, XtPointer, XtPointer );

#endif
