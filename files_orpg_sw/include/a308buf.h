/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/11/07 21:48:44 $
 * $Id: a308buf.h,v 1.11 2014/11/07 21:48:44 steves Exp $
 * $Revision: 1.11 $
 * $State: Exp $
 */

/*
 * Combined Attributes Table Header File
 *
 *      The contents in this file are derived from a308buf.inc. The macros
 *      must be consistent with those defined there. Thus, if a308buf.inc is
 *      modified, this file has to be updated accordingly.
 *
 * References: Par. 3.5.1.1.18.1 f. of the C5-Specification
 */


#ifndef A308BUF_H
#define A308BUF_H

#include <orpgctype.h>

/*
 * Combined Attributes Parameters A308P3
 */
#define CAT_MXSTMS 100	/* max # storm cells allowed in buffer */
#define CAT_NF       8	/* number of features per storm cell */
#define CAT_SID	     0	/* ndx for storm id */
#define CAT_TVS      1	/* ndex for TVS flag */
#define CAT_POH      2	/* ndx for probability of hail */
#define CAT_POSH     3	/* ndx for prob. of severe hail */
#define CAT_MEHS     4	/* ndx for max expected hail size */
#define CAT_HAIL     5	/* ndx for hail potential label */
#define CAT_TYPE     6	/* ndx for storm cell type (new/cont) */
#define CAT_MDA      7  /* ndx for MDA flag */

#define CAT_DAT   19	/* number of attributes per storm cell */
#define CAT_AZ	   0	/* ndx for azimuth of centroid                (deg)*/
#define CAT_RNG	   1	/* ndx for slant range of centroid             (km)*/
#define CAT_HCN    2	/* ndx for height (ARL) of centroid            (km)*/
#define CAT_ELCN   3	/* ndx for elevation of centroid             (deg?)*/ 
#define CAT_SBS    4	/* ndx for height (ARL) of storm cell base     (km)*/
#define CAT_MXZ    5	/* ndx for max reflectivity of storm cell     (dBZ)*/
#define CAT_HMXZ   6	/* ndx for height of max reflectivity          (km)*/
#define CAT_ELVXZ  7	/* ndx for elevation of max reflectivity     (deg?)*/
#define CAT_VIL    8	/* ndx for cell based VIL                 (kg/m**2)*/
#define CAT_STP    9	/* ndx for storm top height (ARL)              (km)*/
#define CAT_FDIR  10	/* ndx for forecast storm cell direction     (deg) */
#define CAT_FSPD  11	/* ndx for forecast storm cell speed          (mps)*/
#define CAT_AZTVS 12	/* ndx for azimuth of TVS base                (deg)*/
#define CAT_RNTVS 13	/* ndx for range of TVS base                   (km)*/
#define CAT_ELTVS 14	/* ndx for elevation of TVS base             (deg?)*/
#define CAT_AZMDA 15    /* ndx for azimuth of MDA feature base        (deg)*/
#define CAT_RNMDA 16    /* ndx for range of MDA feature base           (km)*/
#define CAT_ELMDA 17    /* ndx for elevation of MDA feature base      (deg)*/
#define CAT_SRMDA 18    /* ndx for strength rank of MDA feature       (n/a)*/

/*
 * For FORCST_POSITS
 */
#define MAX_FPOSITS 4	/* max # forecast positions per storm cell */
#define FOR_DAT     2	/* # of descriptors per forecast position */
#define CAT_FX      0	/* X ndx of the forecast position */
#define CAT_FY      1	/* Y ndx of the forecast position */

/*
 * The following define the Radar Coded Message (RCM) parameters for
 * mesocyclones and TVS's.
 */
#define CAT_RCM  2	/* # total data fields for RCM */
#define RCM_TVS  0	/* ndx for total # of TVS */
#define RCM_MDA  1      /* ndx for total # of MDA features */

#define CAT_NTVS  3	/* number of attributes per TVS */
#define CAT_TVSAZ 0	/* ndx for azimuth of TVS base                (deg)*/
#define CAT_TVSRN 1	/* ndx for range of TVS base                   (km)*/
#define CAT_TVSEL 2	/* ndx for elevation of TVS base             (deg?)*/

#define CAT_NMDA  4     /* number of attributes per MDA feature */
#define CAT_MDAAZ 0     /* ndx for azimuth of MDA feature base        (deg)*/
#define CAT_MDARN 1     /* ndx for range of MDA feature base           (km)*/
#define CAT_MDAEL 2     /* ndx for elevation of MDA feature base      (deg)*/
#define CAT_MDASR 3     /* ndx for strength rank of MDA feature       (n/a)*/

typedef struct {
   fint cat_num_storms ;		/* # storms found in current volscn */
   fint cat_feat[CAT_MXSTMS][CAT_NF] ;	/* 2D array of storm ids and severe */
					/*    wx-ctgry absent/present flags */
   freal comb_att[CAT_MXSTMS][CAT_DAT] ;/* 2D array of storm attributes     */
					/*   combined from the Storm Series,*/
					/*   TVS and MDA algorithms         */
   fint num_fposits ;			/* # positions for which forecasts  */
					/*    made for the storms           */
   freal forcst_posits[CAT_MXSTMS][MAX_FPOSITS][FOR_DAT] ;
   fint cat_num_rcm[CAT_RCM] ;		/* 1D array of # of severe wx phenom*/
					/*    detected per volscan, reported*/
					/*    to RCM                        */
   freal cat_tvst[CAT_MXSTMS][CAT_NTVS] ;
					/* 2D array of TVS attributes       */
   freal cat_mdat[CAT_MXSTMS][CAT_NMDA] ;
                                        /* 2D array of MDA attributes       */
} combattr_t;

/*
 * The following is a flag indicating No Data in a field:
 */
#ifndef NODATA
#define NODATA -999.99f
#endif

/*
 * Combined Attributes Parameters A308Q3
 */
#define FEAT_TYPES     2
#define CATMXSTM       100
#define CAT_TVS_TYPE   0
#define CAT_MDA_TYPE   1

#define NBCHAR     7
#define TVSB_AZ    0
#define TVSB_RN    1
#define TVSB_EL    2
#define MDAB_AZ    3
#define MDAB_RN    4
#define MDAB_EL    5
#define MDA_SR     6

#define CNS		0
#define CFEA		1
#define CATT            (CFEA+(CAT_NF*CAT_MXSTMS))
#define CNFP            (CATT+(CAT_DAT*CAT_MXSTMS))
#define CNFST           (CNFP+1)
#define CNRCM           (CNFST+(FOR_DAT*MAX_FPOSITS*CAT_MXSTMS))
#define CNTVS           (CNRCM+CAT_RCM)
#define CNMDA           (CNTVS+(CAT_NTVS*CAT_MXSTMS))

/*
 * A308P4
 *
 * Alert Processing: Lowest Elevation Velocity Output Parameters.
 *
 */

#define	NACOL		58	/* The number of columns in the alert grid. */
#define NAROW		58	/* The number of rows in the alert grid. */
#define ABOXHGHT	16	/* The height (y-dir) of an Alert box, in km. */
#define ABOXWDTH	16	/* The width (x-dr) of an Alert box, in km. */

#define OMAX		0	/* The offset to the maximum velocity value. */
#define OMAXI		1	/* The offset to the maximum velocity I position. */
#define OMAXJ		2	/* The offset to the maximum velocity J position. */
#define OMODE		3	/* The Doppler velocity weather mode offset. */
#define OGRID		4	/* The offset to the lowest el. velocity Alert grid. */

#endif /* DO NOT REMOVE! */
