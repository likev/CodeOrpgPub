/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/03/03 15:06:54 $
 * $Id: dp_elev_Consts.h,v 1.2 2009/03/03 15:06:54 ccalvert Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

#ifndef DP_ELEV_CONSTS_H
#define DP_ELEV_CONSTS_H

/******************************************************************************
    Filename: dp_elev_Consts.h

    Description:
    ============
    Declare constants for the Dual Pol Elevation algorithm.

    Change History
    ==============
    DATE        VERSION    PROGRAMMER         NOTES
    ----        -------    ----------         ---------------
    09/24/2007  0001       James Ward         Initial version
******************************************************************************/

#include <basedata.h>  /* Base_data_header */
#include <hca.h>       /* NUM_CLASSES      */
#include <dp_Consts.h> /* MAX_AZM          */

/* dpprep outputs 9 generic moments: KDP PHI RHO SDP SDZ SMV SMZ SNR ZDR.  *
 * (grep for Add_moment in ~/src/cpc004/tsk011/dpp_format.c)               *
 * (Spectrum width is in the base data header, and not in generic format.) *
 * qia adds the 6 q vectors: QKD, QRO, QSZ, QTP, QTZ, QZD.                 *
 * hca adds 2 more moments: HCA and ML, drops the 6 q vectors,             *
 * and outputs 11 generic moments.                                         *
 * (grep for Add_moment in ~/src/cpc017/tsk012/hca_process_radial.c)       *
 * Of these 11 hca output, qperate only uses 6. Unused by qperate are:     *
 * PHI (differential phase), SDP (differential phase texture),             *
 * SDZ (reflectivity texture), SMV (smoothed velocity) and                 *
 * SNR (signal to noise ratio).                                            */

#define NUM_MOMENTS 11

/* GM_SIZE is the size of the generic moment header. */

#define GM_SIZE sizeof(Generic_moment_t)

/* Define the number of bins for each moment. Even though qperate      *
 * uses only 920 bins, another DP_BASEDATA_ELEV users might use        *
 * all of them.                                                        *
 *                                                                     *
 * To see the max gates, look at Add_moment() in:                      *
 *                                                                     *
 * moment  type  max             look at                               *
 * ------  ----  -----------  ---------------------------------------- *
 *  DHCA    'h'  n_dgates     ~/src/cpc017/tsk012/hca_process_radial.c *
 *  DKDP    'd'  n_dgates     ~/src/cpc004/tsk011/dpp_format.c         *
 *  DML     'm'  4            ~/src/cpc017/tsk012/hca_process_radial.c *
 *  DPHI    'd'  n_dgates     ~/src/cpc004/tsk011/dpp_format.c         *
 *  DRHO    'd'  n_dgates     ~/src/cpc004/tsk011/dpp_format.c         *
 *  DSDP    'd'  n_dgates     ~/src/cpc004/tsk011/dpp_format.c         *
 *  DSDZ    'z'  n_surv_bins  ~/src/cpc004/tsk011/dpp_format.c         *
 *  DSMV    'v'  n_dop_bins   ~/src/cpc004/tsk011/dpp_format.c         *
 *  DSMZ    'z'  n_surv_bins  ~/src/cpc004/tsk011/dpp_format.c         *
 *  DSNR    'z'  n_surv_bins  ~/src/cpc004/tsk011/dpp_format.c         *
 *  DZDR    'd'  n_dgates     ~/src/cpc004/tsk011/dpp_format.c         *
 *                                                                     *
 * n_dgates and n_dop_bins should be BASEDATA_RHO_SIZE (1200).         *
 * n_surv_bins should be MAX_BASEDATA_REF_SIZE (1840).                 *
 * These are defined in basedata.h                                     *
 *                                                                     *
 * 20071101 Brian Klein suggests using EHC (Elevation HydroClass)      *
 * instead of HCA as the moment name because the hydroclasses have     *
 * been filtered. All the other moment names stay their name.          *
 *                                                                     *
 * 20090107 Note: In qia_process.c, QIA is not applied beyond n_dgates *
 * Brian Klein says: "There is only one field that QIA cares           *
 * about that could be different than n_dgates and that is the         *
 * smoothed reflectivity which is n_zgates. Since the HCA can't do     *
 * anything for a bin that doesn't have all the input fields HCA       *
 * requires, there's no sense in going beyond n_dgates."               */

#define NUM_EHC BASEDATA_RHO_SIZE
#define NUM_KDP BASEDATA_RHO_SIZE
#define NUM_ML  4
#define NUM_PHI BASEDATA_RHO_SIZE
#define NUM_RHO BASEDATA_RHO_SIZE
#define NUM_SDP BASEDATA_RHO_SIZE
#define NUM_SDZ MAX_BASEDATA_REF_SIZE
#define NUM_SMV BASEDATA_RHO_SIZE
#define NUM_SMZ MAX_BASEDATA_REF_SIZE
#define NUM_SNR MAX_BASEDATA_REF_SIZE
#define NUM_ZDR BASEDATA_RHO_SIZE

/* Define the size of each moment, in bytes. The short moments require a
 * higher data resolution for accuracy. */

#define EHC_SIZE ALIGNED_SIZE(GM_SIZE + (NUM_EHC * sizeof(char )))
#define KDP_SIZE ALIGNED_SIZE(GM_SIZE + (NUM_KDP * sizeof(short)))
#define  ML_SIZE ALIGNED_SIZE(GM_SIZE + (NUM_ML  * sizeof(short)))
#define PHI_SIZE ALIGNED_SIZE(GM_SIZE + (NUM_PHI * sizeof(short)))
#define RHO_SIZE ALIGNED_SIZE(GM_SIZE + (NUM_RHO * sizeof(short)))
#define SDP_SIZE ALIGNED_SIZE(GM_SIZE + (NUM_SDP * sizeof(char )))
#define SDZ_SIZE ALIGNED_SIZE(GM_SIZE + (NUM_SDZ * sizeof(char )))
#define SMV_SIZE ALIGNED_SIZE(GM_SIZE + (NUM_SMV * sizeof(char )))
#define SMZ_SIZE ALIGNED_SIZE(GM_SIZE + (NUM_SMZ * sizeof(char )))
#define SNR_SIZE ALIGNED_SIZE(GM_SIZE + (NUM_SNR * sizeof(char )))
#define ZDR_SIZE ALIGNED_SIZE(GM_SIZE + (NUM_ZDR * sizeof(char )))

/* NUM_MODE_CLASSES is the number of hydrometeor classes
 * to filter over.                                       */

#define NUM_MODE_CLASSES NUM_CLASSES

#define DP_ELEV_PROD_DEBUG FALSE

#endif /* DP_ELEV_CONSTS_H */
