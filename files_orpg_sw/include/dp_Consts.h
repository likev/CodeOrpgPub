/*
 * RCS info
 * $Author: dberkowitz $
 * $Locker:  $
 * $Date: 2014/09/02 19:58:16 $
 * $Id: dp_Consts.h,v 1.7 2014/09/02 19:58:16 dberkowitz Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */

#ifndef DP_CONSTS_H
#define DP_CONSTS_H

#include <limits.h> /* SHRT_MIN */

/******************************************************************************
    Filename: dp_Consts.h

    Description:
    ============
       Declare constants shared between Dual Pol tasks.

    Change History
    ==============
    DATE      VERSION  PROGRAMMER   NOTES
    --------  -------  ----------   -----------------------------
    20080508   0000      Ward       Initial version.
    20080709   0001      Ward       Added product version numbers
    20080821   0002      Ward       Added adaptable parameter limits
                                    tied to AEL spreadsheet rows.
    20100310   0003      Ward       Removed RhoHV_min_Kdp_rate which
                                    has been replaced with corr_thresh.
    20100928   0004      Ward       Added Use_PBB1, Max_rate
    20110202   0005      Ward       Made PBB2 default
    20111031   0006      Ward       For CCR NA11-00372:
                                    Removed USE_PBB1
                                    Removed Z_MAX_BEAM_BLK
                                    Added   MIN_BLOCKAGE
                                    Added   KDP_MIN_BEAM_BLK
                                    Removed RHOHV_MIN_RATE
    20140326   0007      Murnan     Added NULL_REASON_6
                                    Added NULL_REASON_7
******************************************************************************/

/* Boolean variables. TRUE/FALSE may be redefined by the RPG */

#ifndef TRUE
   #define TRUE  1
#endif

#ifndef FALSE
   #define FALSE 0
#endif

/* Function error returns */

#define FUNCTION_SUCCEEDED 0
#define FUNCTION_FAILED    1
#define NULL_POINTER       2
#define NEGATIVE_BIAS      3

/* QPE_NODATA indicates bad or missing data. It is set to SHRT_MIN (-32768)
 * instead of INT_MIN (-2147483648) because QPE_NODATA is used by both
 * shorts (hydroclass), ints (accum grids), and floats (rate grids) */

#define QPE_NODATA SHRT_MIN

/* Conversion factors */

#define MM_TO_IN 0.03937008

/* INIT_VALUE is the legacy no data value, defined in a313h.h
 * 20071204 Confirmed by Cham Pham:
 * The "no data" value is INIT_VALUE (0) or IINIT(0) so you can
 * replace it with QPE_NODATA. */

#define INIT_VALUE 0

/* The MIN/MAX constants and the SCALE factors are used to construct
 * outbuf halfword product values. They match what's in the ICD.     */

#define MIN_MAX_ACCUM             0.0  /* inches              */
#define MAX_MAX_ACCUM           100.0  /* inches              */
#define MAX_MAX_ACCUM_DUA       327.6  /* inches              */
#define SCALE_MAX_ACCUM          10.0

#define MIN_MEAN_FIELD_BIAS       0.01 /* unitless            */
#define MAX_MEAN_FIELD_BIAS      99.99 /* unitless            */
#define SCALE_MEAN_FIELD_BIAS   100.0

#define MIN_SAMPLE_SIZE           0.00 /* # gauge/radar pairs */
#define MAX_SAMPLE_SIZE        9999.99 /* # gauge/radar pairs */
#define SCALE_SAMPLE_SIZE       100.0

/* Note: SHORT_MAX = 32767 */

#define MIN_DATE                  1    /* days since 1/1/1970  */
#define MAX_DATE          SHORT_MAX    /* days since 1/1/1970  */

#define MIN_TIME                  0    /* mins since midnight  */
#define MAX_TIME               1439    /* mins since midnight  */

#define MIN_TIME_SPAN            15    /* minutes              */
#define MAX_TIME_SPAN          1440    /* minutes              */

#define MIN_MIN_ACCUM_DIFF     -100.0  /* inches               */
#define MAX_MIN_ACCUM_DIFF      100.0  /* inches               */
#define SCALE_MIN_ACCUM_DIFF     10.0

#define MIN_MAX_ACCUM_DIFF     -100.0  /* inches               */
#define MAX_MAX_ACCUM_DIFF      100.0  /* inches               */
#define SCALE_MAX_ACCUM_DIFF     10.0

/* Note: USHORT_MIN = 0, USHORT_MAX = 65535 */

#define MIN_MAX_INST_PRECIP USHORT_MIN /* inches/hour          */
#define MAX_MAX_INST_PRECIP USHORT_MAX /* inches/hour          */

/* Used by buildDPR_prod */

#define MIN_HYBR_RATE_FILLED      0.01 /* %                    */
#define SCALE_HYBR_RATE_FILLED  100.0
#define MAX_HYBR_RATE_FILLED    100.00 /* %                    */

#define MIN_HIGHEST_ELEV_USED     0.5  /* degrees              */
#define SCALE_HIGHEST_ELEV_USED  10.0
#define MAX_HIGHEST_ELEV_USED    19.5  /* degrees              */

/* Note: SHORT_MAX = 32767 */

#define MIN_SCALE_FACTOR          1    /* unitless             */
#define MAX_SCALE_FACTOR  SHORT_MAX    /* unitless             */

/* MAX_PACKET1 = the maximum length of a packet 1 string.
 *
 * MAX_PACKET1 should be 80 characters,
 * as CVG line wraps after 80 characters */

#define MAX_PACKET1 80

/* DP_LIB002_DEBUG is put here because lib002 doesn't
 * have its own constants file. */

#define DP_LIB002_DEBUG FALSE

/* NULL product indicator codes */

#define NULL_REASON_1 1  /* no accum available - s2s is null       */
#define NULL_REASON_2 2  /* no precip in time span - dua/TOH       */
#define NULL_REASON_3 3  /* no accum in db for time span - dua/TOH */
#define NULL_REASON_4 4  /* storm is not active                    */
#define NULL_REASON_5 5  /* no hourly accum                        */
#define NULL_REASON_6 6  /* no Top_of_Hour accumulation - some     *
                          * problem encountered with the SQL query *
                          * resulted in an error                   */
#define NULL_REASON_7 7  /* no Top_of_Hour accumulation because of *
                          * excessive missing time encountered     */

/* Time related constants */

#define SECS_PER_MIN     60  /* seconds in one minute */
#define SECS_PER_HOUR  3600  /* seconds in one hour   */
#define SECS_PER_DAY  86400  /* seconds in one day    */

#define MINS_PER_HOUR    60  /* minutes in one hour   */

/* MAX_MODE_BINS is the maximum number of bins
 * that the mode filter should ever see. */

#define MAX_MODE_BINS  2000  /* max bins/radial should be
                              * MAX_BASEDATA_REF_SIZE (1840) */

/* MX_AZM and MX_RNG are for legacy */

#define MX_AZM 360 /* Number of whole degree in legacy resolution       */
#define MX_RNG 230 /* Maximum number of range bins in legacy resolution */

/* MAX_2KM_RESOLUTION is the maximum legacy resolution (2 km)
 * when it gets to the difference product. It is halved from 230. */

#define MAX_2KM_RESOLUTION 115 /* 115 = 920/8 */

/* MAX_AZM and MAX_BINS are for Dual Pol. basedata_elev.h defines
 * MAX_RADIALS_ELEV as 400, but we are only going to store 360.
 * Erin fill is 592 bins. */

#define MAX_AZM  360 /* 360 = 1 degree            */
#define MAX_BINS 920 /* 920 = 0.250 km resolution */

#define MAX_AZM_BINS (MAX_AZM * MAX_BINS)

#define INT_AZM_BINS (sizeof(int) * MAX_AZM_BINS)

/* Linear buffer read/write return values */

#define READ_OK       0
#define READ_FAILED  -1

#define WRITE_OK      0
#define WRITE_FAILED -1

/* The 8 bit product output range is a constant,
 * 1 to 255 = UCHAR_MAX, as a float.
 *
 * See: http://en.wikipedia.org/wiki/Limits.h */

#define NODATA_VALUE_4BIT    0
#define NODATA_VALUE_8BIT    0
#define FACTOR_8BIT       2540.0

/* Product version numbers for halfword 54. 2008109 Mike Istok says:
 *
 * Initial version should be zero. It is incremented when something
 * "significant" changes so that downstream software can handle either case
 * (e.g., PPS when the products changed from Cartesian data to polar;
 * Composite reflectivity when old meso algorithm data was replace with
 * the new MDA data).  We need to be very careful about incrementing it.
 * For example, sometime in the past 5 years, OHD increment the PPS version,
 * but suddenly the FAAs WARP system could not display the product because
 * their code was not expecting that that version number. In this case, ROC
 * changed the version back to what it was previously.  If it is zero,
 * downstream software should not actually be using the version number. */

#define OHA_VERSION 0 /* product 169 */
#define DAA_VERSION 0 /* product 170 */
#define STA_VERSION 0 /* product 171 */
#define DSA_VERSION 0 /* product 172 */
#define DUA_VERSION 0 /* product 173 */
#define DOD_VERSION 0 /* product 174 */
#define DSD_VERSION 0 /* product 175 */
#define DPR_VERSION 0 /* product 176 */


/***** Exclusion zones *****/

#define EXZONE_FIELDS                        5

/***** Number of Exclusion zones ****/

/* The following variable is duplicated as a different variable in the
 * DEA file (dp_precip.alg in /cfg/dea).  The variable there is called
 * "Num_zones".  There are multiple references in the source code using
 * the MAX_NUM_ZONES to see array parameters. */

#define MAX_NUM_ZONES                       20  

#endif /* DP_CONSTS_H */
