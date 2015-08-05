/*   @(#)tee.h	1.1	07 Jul 1998	*/
/*
 *  Copyright (c) 1993 Spider Software Limited
 *
 *  This Source Code is furnished under Licence, and may not be
 *  copied or distributed without express written agreement.
 *
 *  Authors: George Wilkie
 *
 *  @(#)$Spider: tee.h,v 1.1 1997/05/26 13:55:50 mark Exp $
 */


#ifndef _SYS_SNET_TEE_
#define _SYS_SNET_TEE_

#define TEE_IOCTL	('T' << 8)
#define TEE_REG		(TEE_IOCTL | 'r')

#define TEE_R		1
#define TEE_W		2
#define TEE_RW		3

typedef struct {
	char	tee_index;
	char	tee_rw;
	char	tee_type;
	char	tee_pad;
} TEE_INFO;

#endif /* _SYS_SNET_TEE_ */
