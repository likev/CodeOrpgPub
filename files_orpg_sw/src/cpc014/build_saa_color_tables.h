/*
 * RCS info
 * $Author: dzittel $
 * $Locker:  $
 * $Date: 2005/02/17 16:03:35 $
 * $Id: build_saa_color_tables.h,v 1.2 2005/02/17 16:03:35 dzittel Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */


/************************************************************************
Module:         build_saa_color_tables.h

Description:    include file for SAA library function build_saa_color_tables.c
		that builds color lookup tables for the six snow accumulation
		algorithm products, 144 - OSW, 145 - OSD, 146 - SSW, 147 -
		SSD, 150 - USW, and 151 USD.        
                
Authors:        Dave Zittel, Meteorologist, ROC/Applications
                    walter.d.zittel@noaa.gov
                Version 1.0, October 2003

History:	11/04/2004	SW CCR NA04-30810	Build8 Changes

*************************************************************************/
/* include file for build_saa_color_tables  */

#define MAX_DPTH 1024			/* increased from 512 for Build8  */
#define MAX_WEQV 1024			/* increased from 512 for Build8  */
#define LWEQV_THRES  400
#define LDPTH_THRES  500
#define HWEQV_THRES  250
#define HDPTH_THRES  300
#define AREA_THRES 10  /*  square kilometers  */

short shdpth_clr[MAX_DPTH];				/* new Build8  */
short hdpth_clr[MAX_DPTH];
short ldpth_clr[MAX_DPTH];
short shweqv_clr[MAX_WEQV];				/* new Build8  */
short hweqv_clr[MAX_WEQV];
short lweqv_clr[MAX_WEQV];

extern int hi_sf_flg;					/* new Build8  */

/* end of include file  */
