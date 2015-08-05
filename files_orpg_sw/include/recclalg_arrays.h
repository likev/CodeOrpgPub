/* 
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2007/03/07 14:29:41 $
 * $Id: recclalg_arrays.h,v 1.5 2007/03/07 14:29:41 ryans Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */

#ifndef RECCLALG_ARRAYS_H
#define RECCLALG_ARRAYS_H

#include <basedata.h>

#define RECCLALG_VERSION      1 /* Version for REC intermediate product format */
#define MAX_1KMBINS         230 /* maximum number of one kilomter size bins */
#define MAX_DOP_BIN           4 /* number of Doppler bins per 1KM reflectivity bin */
#define MAX_QTRKMBINS       (MAX_1KMBINS*MAX_DOP_BIN)

/*  Includes reflectivity and Doppler output arrays  */

typedef struct
{
    int         time;           /* GMT start of elevation */
    short       date;           /* GMT start of elevation in modified julian date format */
    short       version;        /* version number for this product format */
    short       volume_scan_num;/* from 1 to 80 */ 
    short       vcp_num;        /* volume coverage pattern number */
    short       elev_num;       /* serial numbering of elevation cut within volume, from RDA */
    short	rpg_elev_ind;	/* RPG elevation index (used in final product header) */
    short       target_elev;    /* in .1 degrees, from VCP tables */
    short       bin_size;       /* in meters */ 
    short       n_bins;         /* number of bins in each radial of this product */
    short	cos_ele_short;	/* float value cos_ele multiplied by 1000 and truncated */
    short       last_ele_flag;  /* set to 1 for last elevation cut*/ 
    short       num_radials;	/* number of radials that contain valid data (<= MAX_RADIALS) */
    int		adapt_offset;	/* byte offset from beginning of product to start of adaptation data */
    short       spare[7];       /* spare */ 
}Rec_prod_header_t;


typedef struct
{
    float       azimuth;        /* azimuth of this radial */
    short	start_angle;	/* in .1 degrees */
    short	delta_angle;	/* in .1 degrees */
    short       spot_blank_flag;/* See RDA_basedata_header */
    short       spare[3];       /* spare */
}Rec_rad_header_t;


struct Refl_array
{
    Rec_prod_header_t   pHeader;                    /* output array for reflectivity intermediate product header */
    Rec_rad_header_t    rHeader[MAX_RADIALS];       /* output array for reflectivity radial header */
    short       Z_data[MAX_RADIALS][MAX_1KMBINS];   /* working array for reflectivity base data */
    short       Z_clut[MAX_RADIALS][MAX_1KMBINS];   /* output array for reflectivity clutter likelihood */
} Z_array;


struct Dopp_array
{
    Rec_prod_header_t   pHeader;                    /* output array for Doppler intermediate product header */
    Rec_rad_header_t    rHeader[MAX_RADIALS];       /* output array for Doppler radial header */
    short       V_data[MAX_RADIALS][MAX_1KMBINS][MAX_DOP_BIN];/* working array for Doppler velocity base data */
    short       W_data[MAX_RADIALS][MAX_1KMBINS][MAX_DOP_BIN];/* working array for Doppler spectrum width base data */
    short       D_clut[MAX_RADIALS][MAX_1KMBINS][MAX_DOP_BIN];/* output array for Doppler clutter likelihood */
} D_array;


#endif
