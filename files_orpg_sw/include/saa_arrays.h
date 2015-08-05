/*
 * RCS info
 * $Author: cheryls $
 * $Locker:  $
 * $Date: 2007/04/18 21:17:20 $
 * $Id: saa_arrays.h,v 1.3 2007/04/18 21:17:20 cheryls Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */


/************************************************************************
Module:         saa_arrays.h  

Description:    Library include file containing common arrays and structures
		for many modules in the Snow Accumulation Algorithm product
		tasks, saaprods and saausers.         
                
Authors:        Dave Zittel, Meteorologist, ROC/Applications
                    walter.d.zittel@noaa.gov
                Version 1.0, October 2003

Changes:	Outbuf_t increased to 45000 for ORPG Build 9
		04/18/2007  WDZ

*************************************************************************/
#ifndef SAA_ARRAYS_H
#define SAA_ARRAYS_H

#define SAA_VERSION           0 /* Version for SAA intermediate product format */
#define MAX_SAA_RADIALS     360 /* max number of SAA radials possible */
#define MAX_SAA_BINS        230 /* max number of SAA bins per radial  */

typedef struct
{
    short       date;           /* GMT start of elevation */
    int         time;           /* GMT start of elevation in modified julian date format */
    short       version;        /* version number for this product format */
    short       volume_scan_num;/* from 1 to 80 */ 
    short       vcp_num;        /* volume coverage pattern number */
    short	rpg_elev_ind;	/* RPG elevation index (used in final product header) */
    short       flags;          /* high byte= hi_sf_flg, low byte = user RCA flag  */
}SDT_prod_header_t;

typedef struct
{
    float       azimuth;        /* azimuth of this radial */
    short	start_angle;	/* in .1 degrees */
    short	delta_angle;	/* in .1 degrees */
    short       spot_blank_flag;/* See RDA_basedata_header */
    short       spare[3];       /* spare */
}SDT_rad_header_t;

typedef struct
{
      unsigned short halfword[45000];
}  Outbuf_t;

struct saadata_t
{
	int   begin_date;
	int   begin_time;
	int   current_date;
	int   current_time;
        /* Start of Adaptable Parameter List                         */
	float cf_ZS_mult;	    /*Z-S Multiplicative Coefficient */
	float cf_ZS_power;	    /*Z-S Power Coefficient          */
	float s_w_ratio;        /*Snow - Water Ratio*/
	float thr_lo_dBZ;       /*Low reflectivity/isolated bin threshold (dBZ)*/
	float thr_hi_dBZ;       /*High reflectivity/outlier bin threshold (dbZ)*/
	float thr_mn_hgt_corr;  /*Minimum height correction threshold (km)*/
	float cf1_rng_hgt;	    /*Range Height Correction Coefficient #1*/
	float cf2_rng_hgt;	    /*Range Height Correction Coefficient #2*/
	float cf3_rng_hgt;	    /*Range Height Correction Coefficient #3*/
	float thr_time_span;	/*Time Span Threshold (hr)*/
	float thr_mn_pct_time;	/*Minimum percent time threshold*/
	int flag_reset_accum;	/*Accumulation reset flag*/
	int use_RCA_flag;	    /* Select whether to use the RCA correction factors
                            or the default SAA climatological range height
                            correction.  Will be made URC adaptable when the
                            RCA algorithm is implemented. */
	float rhc_base_elev;    /* Base elevation for computing climatological SAA 
                            range height correction */		
	short vcp_num;
	short ohp_flg;
	short swo_data[MAX_SAA_RADIALS][MAX_SAA_BINS];
	short swo_max;
	short sdo_data[MAX_SAA_RADIALS][MAX_SAA_BINS];
	short sdo_max;
	short ohp_start_date;
	short ohp_start_time;
	short ohp_end_date;
	short ohp_end_time;
	short swt_data[MAX_SAA_RADIALS][MAX_SAA_BINS];
	short swt_max;
	short sdt_data[MAX_SAA_RADIALS][MAX_SAA_BINS];
	short sdt_max;
	short stp_start_date;
	short stp_start_time;
	int   stp_flg;
	float stp_missing_period;
	float ohp_missing_period;
	int   vol_sb;
	short spares[1246];
} saa_inp;

#endif
