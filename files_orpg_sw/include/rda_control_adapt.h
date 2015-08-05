/*
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2004/10/15 14:09:23 $
 * $Id: rda_control_adapt.h,v 1.5 2004/10/15 14:09:23 ryans Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */

/*	This header file defines the structure for control
	RDA adaptation data */


#ifndef RDA_CONTROL_ADAPT_H
#define	RDA_CONTROL_ADAPT_H

#include <orpgctype.h>


/**
	RDA control adaptation data
**/
typedef struct
{
	int	loopback_rate;		/* @name "Loopback Rate"
				           @min 60  @max 300  @units seconds
					*/
	int	loopback_disabled;	/* @name "Loopback Disabled"
					   @enum_values "Enabled=0", "Disabled=1"
					*/
	int	no_of_conn_retries;	/* @name "No of Connection Retries"
					   @min 0  @max 5
					*/
	int 	no_of_conn_timeouts;	/* @name "No of Connection Timeouts"
					   @min 1  @max 30
					*/

} rda_control_adapt_t;


/**   Declare a C++ wrapper class that statically contains a pointer to the meta
      data for this structure.  The C++ wrapper class allows the IOCProperty API
      to be implemented automatically for this structure
**/

#endif
