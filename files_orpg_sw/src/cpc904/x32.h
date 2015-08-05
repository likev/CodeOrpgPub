/*   @(#)x32.h	1.1	07 Jul 1998	*/

/*
 * Copyright (c) 1988-1997 Spider Software Limited
 *
 * This Source Code is furnished under Licence, and may not be
 * copied or distributed without express written agreement.
 *
 * All rights reserved.  Made in Scotland.
 *
 * Authors: Alan Robertson, Peter Woodhouse, Duncan Walker, Jim Stewart
 *
 * x32.h of snet module
 *
 * SpiderX25
 * @(#)$Id: x32.h,v 1.1 2000/02/25 17:15:31 john Exp $
 * 
 * SpiderX25 Release 8
 */

/*
 * X.32 - Codes for elements type identifiers
 */
#define IDENTITY_ETI		0xCC
#define SIGNATURE_ETI		0xCD
#define DIAG_ETI		0x07

/*
 * X.32 - Defines which elements should be in packet/frame
 */
#define NO_ELEMENT		0x00
#define IDENTITY_ELEMENT	0x01
#define DIAG_ELEMENT		0x02
#define ERROR_DIAG_ELEMENT	0x04

