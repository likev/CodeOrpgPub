/*   @(#)ll_proto.h	1.1	07 Jul 1998	*/

/*
 * Copyright (c) 1988-1997 Spider Software Limited
 *
 * This Source Code is furnished under Licence, and may not be
 * copied or distributed without express written agreement.
 *
 * All rights reserved.  Made in Scotland.
 *
 * ll_proto.h of snet module
 *
 * SpiderX25
 * @(#)$Id: ll_proto.h,v 1.1 2000/02/25 17:14:30 john Exp $
 * 
 * SpiderX25 Release 8
 */


/* Values of 'class' in 'setsnid' */
#define LC_LLC1          15
#define LC_LLC2          16
#define LC_LAPBDTE       17
#define LC_LAPBXDTE      18
#define LC_LAPBDCE       19
#define LC_LAPBXDCE      20
#define LC_LAPDTE        21
#define LC_LAPDCE        22
#define LC_HDLC          27
#define LC_HDLCX         28

#define	LS_FAILED	 0
#define	LS_SUCCESS	 1
#define	LS_EXHAUSTED	 2
#define LS_RESETDONE	 3
#define LS_CONFLICT	 4
#define LS_DISCONNECT	 5
#define LS_RST_FAILED	 6
#define LS_RST_REFUSED	 7


