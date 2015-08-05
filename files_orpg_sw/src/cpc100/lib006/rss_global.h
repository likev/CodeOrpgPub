
/****************************************************************
		
    Module: rss_global.h	
				
    Description: This file declares all global variables that are 
	required to build the server. 
	This file is not needed for application programs that use
	RSS library routines.

****************************************************************/

/*
 * RCS info 
 * $Author: cm $
 * $Locker:  $
 * $Date: 1996/06/04 16:44:27 $
 * $Id: rss_global.h,v 1.1 1996/06/04 16:44:27 cm Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 * $Log: rss_global.h,v $
 * Revision 1.1  1996/06/04 16:44:27  cm
 * Initial revision
 *
 * 
*/

#ifndef RSS_GLOBAL_H
#define RSS_GLOBAL_H


#ifdef GLOBAL_DEFINED
#define EXTERN
#else
#define EXTERN extern
#endif

#include <signal.h>

/* a shared buffer for messages. This must be static space. We let all
   user modules on the server side share this same buffer space. This 
   will save memory. */
EXTERN char RSS_buffer[RMT_PREFERRED_SIZE + 256];


#endif		/* #ifndef RSS_GLOBAL_H */
