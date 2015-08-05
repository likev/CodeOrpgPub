/*   @(#)debug.h	1.1	07 Jul 1998	*/
/*
 *  General Purpose Debug/Trace Utilities
 *
 *  Copyright (c) 1988-1997 Spider Software Limited
 *
 *  All Rights Reserved.
 *
 *  This Source Code is furnished under Licence, and may not be
 *  copied or distributed without express written agreement.
 *
 *  Written by Mark Valentine.
 *
 *  Made in Scotland.
 *
 *  @(#)$Id: debug.h,v 1.1 2000/02/25 17:14:05 john Exp $
 */

#ifndef _debug_h
#define _debug_h

#ifndef DBGPRINTF
#define DBGPRINTF printf
#endif

#ifdef DECLARE_DBGPRINTF
extern int DBGPRINTF(char *, ...);
#endif

#ifndef DBGLEVEL
#define DBGLEVEL debug
#endif

/*
 * TRACE() is used for unconditional debugging (can be turned off globally).
 * LTRACE() makes the debugging depend on the current "debug level".
 * The X prefix is used to temporarily disable individual debug statements.
 */

#ifdef DEBUG
#define TRACE(x)	(void) (DBGLEVEL > 0 && DBGPRINTF x)
#define LTRACE(lev, x)	(void) (DBGLEVEL >= (lev) && DBGPRINTF x)
#else
#define TRACE(x)	/* nothing */
#define LTRACE(lev, x)	/* nothing */
#endif

#define XTRACE(x)	/* nothing */

/*
 * The debug level (user-settable at start-up, or using a debugger).
 */

#ifdef DEBUG
extern int DBGLEVEL;
#endif

#endif /* _debug_h */
