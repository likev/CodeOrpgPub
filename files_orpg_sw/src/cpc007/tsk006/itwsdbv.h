/* RCS info */
/* $Author: steves $ */
/* $Locker:  $ */
/* $Date: 2007/02/22 16:11:54 $ */
/* $Id: itwsdbv.h,v 1.2 2007/02/22 16:11:54 steves Exp $ */
/* $Revision: 1.2 $ */
/* $State: Exp $ */

#ifndef ITWSDBV_H
#define ITWSDBV_H

#include <rpgc.h>
#include <rpgcs.h>
#include <coldat.h>
#include <a309.h>
#include <packet_16.h>
#include <rpgp.h>

#define NUMPRODS                1
#define SCALED_ZERO	        129
#define NEG_VEL_CAP		123
#define POS_VEL_CAP		122
#define EST_PER_RAD     	34
#define HGTMX           	5.4865f
#define PCODE           	93
#define MAXBINS         	115
#define RADSTEP         	4
#define LASTBIN         	460


/* These are buffer sizes and must be defined in terms of # of bytes. */
#define BSIZ87                  50000

/* Global variables. */
Coldat_t color_data;

Siteadp_adpt_t Siteadp;		/* Site adaptation data. */
int ITWS_prod_id;
int Endelcut;
int Proc_rad;
short Radcount;
short Mxneg;
short Mxpos;
short Vc;
short Elmeas;

/* Function Prototypes */
int A30761_buffer_control();

#endif

