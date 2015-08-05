/*   @(#)sdlpi.h	1.1	07 Jul 1998	*/

/*
 * Copyright (c) 1988-1997 Spider Software Limited
 *
 * This Source Code is furnished under Licence, and may not be
 * copied or distributed without express written agreement.
 *
 * All rights reserved.  Made in Scotland.
 *
 * sdlpi.h of snet module
 *
 * SpiderX25
 * @(#)$Id: sdlpi.h,v 1.1 2000/02/25 17:14:55 john Exp $
 * 
 * SpiderX25 Release 8
 */

/*
Modification history:

Chg Date	Init Descpription
1.   6-Jun-98	rjp  Added support for x25llmon utility.

*/



#ifndef _SDLPI_H_
#define _SDLPI_H_
#include "uint.h"

/*
	Below is the proprietory addressing structure which should be used
	in *all* Spider DLPI interfaces.
*/
#define	LSAPMAXSIZE	9

struct lsapformat {
	uint8	lsap_len;		/* Address length in semi-octets */
	uint8	lsap_add[LSAPMAXSIZE];	/* Address in hexadecimal */
	uint8	pad[2];			/* For alignment purposes */
};


/*
    Structure definition for the state table of a Provider DLPI interface.
*/
typedef struct dlpi_prim
{
    uint32 p_state;               /* Acceptable starting state */
    int	  (*p_funcp)();          /* function() to call        */
} DLPI_prov_prim_t;


/*
    Structure definition for the state table of a User DLPI interface.
*/
typedef struct dlpi_state_mach
{
	void	(*func)();
} DLPI_user_prim_t;


/*
    Used for checking DLPI primitive sizes.
*/
typedef struct dlpi_prim_sizes
{
    int     size;
} DLPI_sz_t;


/*
    For dlpi_prim_tab DL_SPECIAL indicates that primitive is valid
    in several states.
*/
#define DL_SPECIAL    (DL_SUBS_UNBIND_PND+1)


/*
    Maximum valid DLPI primitive
*/
#if 0 /* #1 */
#define	DL_MAXPRIM	DL_GET_STATISTICS_ACK
#else
#define	DL_MAXPRIM	DL_MONITOR_LINK_LAYER
#endif

/*
    Number of DLPI states.
*/
#define DLPI_NUM_STATES	(DL_SUBS_UNBIND_PND+1)

#ifdef KERNEL
extern int format_bind(mblk_t **, uint32, uint32, ushort, ushort, uint32);
extern int format_disc_req(mblk_t **, uint32, uint32);
extern int format_connect_res(mblk_t **, uint32, uint32, bufp_t, uint32, uint32);
extern int format_token_req(mblk_t **);
extern int format_unbind(mblk_t **);
extern int format_xid_res(mblk_t **, uint32, bufp_t, uint32);
extern int format_test_res(mblk_t **, uint32, caddr_t, uint32);
extern int format_connect_req(mblk_t **, bufp_t, uint32, bufp_t, uint32, uint32);
extern int format_reset_res(mblk_t **);
extern int format_merror(mblk_t **, int);
extern int dlpi_primok(mblk_t *);
extern int format_info_ack(mblk_t **);
extern int format_ok_ack(mblk_t **, uint32);
extern int format_bind_ack(mblk_t **, uint32, unchar *, int, int, uint32);
extern int format_connect_ind(mblk_t **, uint32, unchar *, int, unchar *,
			      int, unchar *, int);
extern int format_connect_con(mblk_t **, unchar *, int, unchar *, int);
extern int format_discon_ind(mblk_t **, uint32, uint32, uint32);
extern int format_reset_ind(mblk_t **, uint32, uint32);
extern int format_reset_con(mblk_t **);
#endif /* KERNEL */
#endif /* _SDLPI_H_ */
