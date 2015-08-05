/*
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2007/03/05 22:45:05 $
 * $Id: mpda_constants.h,v 1.4 2007/03/05 22:45:05 ryans Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */

/*************************************************************************

   mpda_constants.h
   
   PURPOSE:

   This include file contains the constants used in the MPDA software.

   DEFINITIONS:

   ATTEN_CONV_FAC    = factor that converts atmospheric attenuation to dB/km
   DATA_THR          = all int data values must be below this threshold 
   FLOAT_10          = float of 10 for equations
   FULL_CIRC         = full circle of degrees (360 degrees * 100)
   GO_FORWARD        = direction of travel along radial
   GO_BACKWARD       = direction of travel along radial
   INT_10            = int of 10 for equations
   INT_100           = int of 100 for equations
   LOOKUP_TABLE_SIZE = max size of lookup tables expected
   LOOKUP_TABLE_SQR  = square of max lookup table size
   M_TO_KM	     = conversion factor for meters to kilometers
   MAX_GATES         = max gates allowed along a radial
   MAX_INT_VEL       = max dealiased velocity (m/s * 10) to check 
   MAX_LOOKUP_TABLES = max number of lookup tables expected 
   MAX_NUM_UIF       = max number of values possible in Unisys integer format
   MAX_NYQ           = maximum nyquist velocity allowed (m/s * 10)
   MAX_NYQ_INTV      = number of nyquist intervals to search for dealiasing
   MAX_PRFS          = max number of different PRFs available for processing
   MAX_PRFS_SQR      = square of the max number of PRFs
   MAX_RADS          = max rads allowed per tilt 
   MAX_REF_GATES     = max # reflectivity gates allowed along a radial 
   MIN_NYQ           = minimum nyquist velocity allowed (m/s * 10)
   MISSING           = missing int velocity data flag 
   MISSING_BYTE      = missing value as a single byte (repeated gives short val)
   MISSING_F         = missing float data flag 
   MISSING_PWR       = missing power flag 
   PAIRS             = flag for velocity pairs 
   PWR_THR           = all usable power values must be above this threhold 
   RNG_FLD           = int value for range folded data
   TABLE_RES         = resolution of pairs dealiasing lookup table (m/s * 10)
   TH_OVERLAP_SCALE  = scale factor to convert th_overlap/overlap relax to internal value
   TRIPLETS          = flag for velocity triplets
   UIF_MISSING       = UIF value for a missing value
   UIF_RNG_FLD       = UIF value for a range-folded value
   UIF_VEL_RANGE     = largest possible velocity in Unisys integer format
   UIF_VEL_BIAS      = UIF velocity bias for converting from short value to UIF
*****************************************************************************/

#include <rpg_globals.h>	/* Get ORPG Constants */

#define  ATTEN_CONV_FAC      -0.001
#define  DATA_THR             9000
#define  FLOAT_10             10.
#define  FULL_CIRC	      36000
#define  GO_FORWARD           1
#define  GO_BACKWARD         -1
#define  INT_10               10
#define  INT_100              100
#define  LOOKUP_TABLE_SIZE    150
#define  LOOKUP_TABLE_SQR     22500
#define  M_TO_KM	      0.001
#define  MAX_GATES            1200
#define  MAX_INT_VEL          1000
#define  MAX_LOOKUP_TABLES    15
#define  MAX_NUM_UIF          256
#define  MAX_NYQ              375
#define  MAX_NYQ_INTV         3
#define  MAX_PRFS             6
#define  MAX_PRFS_SQR         36
#define  MAX_RADS             BASEDATA_MAX_SR_RADIALS
#define  MAX_REF_GATES        1840
#define  MIN_NYQ              160
#define  MISSING              9766
#define  MISSING_BYTE	      38
#define  MISSING_F            976.6
#define  MISSING_PWR         -990.0
#define  PAIRS                2
#define  PWR_THR             -900.0
#define  RNG_FLD              9997
#define  TABLE_RES            5
#define  TH_OVERLAP_SCALE     20
#define  TRIPLETS             3
#define  UIF_MISSING          0
#define  UIF_RNG_FLD          1
#define  UIF_VEL_RANGE        127.
#define  UIF_VEL_BIAS         (UIF_VEL_RANGE + 2.0)

/* 
  The following constants used throughout the code:

   array elements in "avg" and bits in "ok" correspond to: 
       0 - pre_rad_avg, 1 - for_rad_avg,
       2 - same_rad_avg1, 3 - same_rad_avg2 
*/
#define PRE_RAD_AVG        0
#define FOR_RAD_AVG        1
#define SAME_RAD_AVG1      2
#define SAME_RAD_AVG2      3

/* Bit masks for the flag holding the availabilty of the averages */

#define PRE_RAD_AVG_MASK    0x1
#define FOR_RAD_AVG_MASK    0x2
#define SAME_RAD_AVG1_MASK  0x4
#define SAME_RAD_AVG2_MASK  0x8
