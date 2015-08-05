/*   @(#)echo_control.h	1.1	07 Jul 1998	*/
/*
 *  Copyright (c) 1993 Spider Software Limited
 *
 *  This Source Code is furnished under Licence, and may not be
 *  copied or distributed without express written agreement.
 *
 *  Authors: Ian Heavens, Gavin Shearer, Nick Felisiak
 *
 *  @(#)$Spider: echo_control.h,v 1.2 1997/05/23 19:51:23 mark Exp $
 */

/*
 * ECHO Streams ioctl primitives for TCP/IP
 */

#define ECHO_STID	100		/* for strlog */

/*
 * primitive for ECHO driver registration
 */
#define ECHO_TYPE	'E'
#define TY_ECHO		0x9000
