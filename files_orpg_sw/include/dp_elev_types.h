/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/03/03 15:06:54 $
 * $Id: dp_elev_types.h,v 1.2 2009/03/03 15:06:54 ccalvert Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

#ifndef DP_ELEV_TYPES_H
#define DP_ELEV_TYPES_H

/*****************************************************************************
    Filename: dp_elev_types.h

    Description:
    ============
    Declare structures for the DualPol Elevation algorithm.

    Note: Compact_dp_basedata_elev is the structure for the output
          DP_BASEDATA_ELEV linear buffer. If you change it, also change
          its max_size in ~/cfg/extensions/product_attr_table.dp_precip

    Change History
    ==============
    DATE      VERSION    PROGRAMMER        NOTES
    --------  -------    ----------        ---------------------------------
    20070924   0001      James Ward        Initial version
    20080718   0002      James Ward        Split out moments for ease of use
*****************************************************************************/

#include <dp_elev_Consts.h> /* EHC_SIZE */

/* Split out the generic moments for ease of use. We could take the next
 * step and convert them to float arrays, but that would consume more
 * storage. */

typedef struct {
   Base_data_header bdh;               /* radial metadata             */
   unsigned char ehc_moment[EHC_SIZE]; /* elevation hydroclass        */
   unsigned char kdp_moment[KDP_SIZE]; /* specific differential phase */
   unsigned char  ml_moment[ML_SIZE];  /* melting layer               */
   unsigned char phi_moment[PHI_SIZE]; /* differential phase          */
   unsigned char rho_moment[RHO_SIZE]; /* correlation coefficient     */
   unsigned char sdp_moment[SDP_SIZE]; /* differential phase texture  */
   unsigned char sdz_moment[SDZ_SIZE]; /* reflectivity texture        */
   unsigned char smv_moment[SMV_SIZE]; /* smoothed velocity           */
   unsigned char smz_moment[SMZ_SIZE]; /* smoothed reflectivity       */
   unsigned char snr_moment[SNR_SIZE]; /* signal to noise ratio       */
   unsigned char zdr_moment[ZDR_SIZE]; /* differential reflectivity   */
} Compact_dp_radial;

/* Define the elevation product structure. */

typedef struct {
   short type;                        /* PREPROCESSED_DUALPOL_TYPE   */
   short vol_ind;                     /* volume index                */
   short elev_ind;                    /* elevation index             */
   short mode_filter_length;          /* number used to filter       */
   int   num_radials;                 /* number of radials processed */
   int   radial_is_good[MAX_AZM];     /* TRUE if good, FALSE if bad  */
   Compact_dp_radial radial[MAX_AZM]; /* the radial data             */
} Compact_dp_basedata_elev;

#endif /* DP_ELEV_TYPES_H */
