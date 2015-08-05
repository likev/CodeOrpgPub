/*   @(#)x32_control.h	1.1	07 Jul 1998	*/

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
 * x32_control.h of snet module
 *
 * SpiderX25
 * @(#)$Id: x32_control.h,v 1.1 2000/02/25 17:15:33 john Exp $
 * 
 * SpiderX25 Release 8
 */




#define MAX_IDENTITY_LEN  32
#define MAX_SIGNATURE_LEN 32

#define PKT 	1
#define LNK	2

/*
 * X.32 State Definition for Packet and Link Levels
 */

#define NO_X32_CONF		0
#define X32_NOT_REG		1
#define X32_REG_FAILED		2
#define X32_ID_SENT		3
#define X32_CMD_CONF_PENDING	4
#define X32_RSP_CONF_PENDING	5
#define X32_REG_SUCCESS		6

#ifndef MAXIOCBSZ
#define MAXIOCBSZ     1024
#endif

#define X32_MAP_SIZE   (MAXIOCBSZ - 8)
#define MAX_X32_ENTS   (X32_MAP_SIZE / sizeof(struct x32conff))

struct x32_facilities {
	unsigned char	identity_len;			/* X.32 identity len */
	unsigned char	signature_len;			/* X.32 sig. len     */
	unsigned char	x32_state;			/* X.32 state        */
	unsigned char	diagnostic;			/* X.32 diagnostic   */
	unsigned char	identity [MAX_IDENTITY_LEN];	/* X.32 identity     */
	unsigned char	signature [MAX_SIGNATURE_LEN];	/* X.32 signature    */
};

struct x32conff {
	uint32                sn_id;   			/* Subnetwork        */
        struct x32_facilities x32_facs;
};

struct x32mapf {
	struct x32conff entries[MAX_X32_ENTS]; /* Data buffer             */
	uint32           first_ent;             /* Where to start search   */
	uint32           num_ent;               /* Number entries returned */
};

/*
 *  Defines for the Multiple Get IOCTL
 *
 *  NOTE: This size of structure "x32mapf" must not
 *	  exceed MAXIOCBSZ byte (as defined in stream.h).
 *	  Thus if other fields are added to "x32mapf"
 *	  then MAX_X32_ENTS must decrease to cope
 *	  with any changes made.
 */
 

 /* 
  * Structure Definitions
  */

struct reg_facilities {
	unsigned char	      has_x32;		/* Has X.32 facs */
	struct x32_facilities x32_facs;
};

struct xid_user_data {
	unsigned char	      has_x32;		/* Has X.32 facs */
	struct x32_facilities x32_facs;
};

