/**************************************************************************

      Module:  iti_globals.h

 Description:
	This include file provides definitions and declarations of global
	variables.  The contents of this file may result in compile-time
	allocation of storage.

	Only file-scope globals may be defined in any other Initialize
	Task Information (ITI) source file.

 Assumptions:

 **************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2005/12/27 16:41:31 $
 * $Id: iti_globals.h,v 1.3 2005/12/27 16:41:31 steves Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

#ifndef ITI_GLOBALS_H
#define ITI_GLOBALS_H

/*
 * System Include Files/Local Include Files
 * Truly global header files ...
 * Those header files required to define global variables ...
 */
#include <errno.h>
#include <stdio.h>

#include <infr.h>
#include <orpg.h>
#include <iti_def.h>

/* All routines may use the system error number. */
extern int errno ;

#ifdef ITI_MAIN

extern char *optarg;
int Verbose = 0;

#else
/* All routines may access the following global variables. */
extern int Verbose;
#endif

#ifdef ITI_ATTR
#endif


#endif /*DO NOT DELETE*/
