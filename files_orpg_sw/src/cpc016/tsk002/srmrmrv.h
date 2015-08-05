/*
 * RCS info
 * $Author: cmn $
 * $Locker:  $
 * $Date: 2008/08/05 19:08:13 $
 * $Id: srmrmrv.h,v 1.2 2008/08/05 19:08:13 cmn Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */
#ifndef SRMRMRV_H
#define SRMRMRV_H

#include <rpgc.h>
#include <rpgcs.h>
#include <a309.h>
#include <coldat.h>
#include <itc.h>
#include <basedata.h>
#include <packet_af1f.h>

/* Structure definitions. */
typedef struct tbufctl {

   int prod_code;		/* Product code. */

   int prod_id;			/* Product ID. */

   int center_azm;		/* Window center azimuth (deg). */

   int center_range;		/* Window center range (km). */

   int storm_speed;		/* Storm speed to remove (m/s*10). */

   int storm_dir;		/* Storm direction to remove (deg*10). */

   char *outbuf;		/* Pointer to product buffer. */

   int gen_date;		/* Product generation date. */

   int gen_time;		/* Product generation time. */

   int max_azm;			/* Window max azimuth angle (deg). */

   int min_azm;			/* Window min azimuth angle (deg). */

   int max_range;		/* Window maximum range (km). */

   int min_range;		/* Window minimum range (km). */
		
   int first_radial;		/* Indicates first radial in window. */ 

   int include_zero_deg;	/* Indicates window includes 0 deg. */

   int color_index;		/* Product color table index. */

   int min_bin;			/* Bin number of minumum slant range. */

   int max_bin;			/* Bin number of maximum slant range. */

   int bin_incr;		/* Ratio of resolution and bin spacing. */

   int maxobin;			/* Maximum bin numbers of the storm motion removed
				   and compacted radials. */

   int maxi2_in_obuf;		/* Number if shorts allocated for each product. */

   int need;			/* Flag indicating the product needs current radial. */

   int status;			/* Product status flag. */

   int num_radials;		/* Number of radials in product. */

   int bufcnt;			/* Number of shorts in the product buffer. */

   int nrleb;			/* Number of bytes RLE in the product buffer. */

   int maxneg;			/* Maxium negative velocity found in window. */

   int maxpos;			/* Maxium posiitive velocity found in window. */

   int storm_info;		/* Flag indicating source of storm motion. */

   int kstorm_speed;		/* Storm speed, in kts*10. */

} Tbufctl_t;

/* Global variables. */
Coldat_t Colrtbl;		/* Color table. */

a3cd09 A3cd09;			/* ITC containing storm motion data. */

int Volnum;			/* Volume scan number. */

int Elev_indx;			/* RPG elevation index. */

int Elev_ang;			/* Elevation angle, in deg*10. */

int Tnumprod;			/* Total number of products. */

int Onumprod;			/* Outstanding number of products. */

int Numwin;			/* Number of window product requests. */

int Numful;			/* Number of full product requests. */

int Srmrvreg_id;		/* Storm Relative Region product ID. */

int Srmrvmap_id;		/* Storm Relative Map product ID. */

int Srmrvreg_code;		/* Storm Relative Region product code. */

int Srmrvmap_code;		/* Storm Relative Map  product code. */

int Need_fcst;			/* Need forecast flag. */

int Dop_reso;			/* Doppler resolution. */

#define MAXSRMRV		20
#define RELEASED		-1
Tbufctl_t Tbufctl[MAXSRMRV];

/* Function prototypes. */
int Srmrmrv_buffer_control();
void Srmrmrv_process( float azm, Base_data_header *hdr, int idelta, int istart );
void Srmrmrv_release_prod( int idbuf, int disposition );

#endif
