/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2008/04/08 15:17:59 $
 * $Id: cp13alg.h,v 1.11 2008/04/08 15:17:59 steves Exp $
 * $Revision: 1.11 $
 * $State: Exp $
 */

/*	This header file defines the ORPG structure for the legacy	*
 *	common block CP13ALG.						*/


#ifndef CP13ALG_H
#define	CP13ALG_H
/*********************************************************************
*.
*.           I N C L U D E    F I L E    P R O L O G U E
*.
*.  INCLUDE FILE NAME: A309ADPT.INC
*.
C*************************************************************
C  ALGORITHM ADAPTATION DATA
C*************************************************************
C
C  CPC 13 Precipitation Algorithms
C
C   --VIL/Echo Tops
C
      REAL    EBMWT
      REAL    ENREF
      INTEGER MEVIL
C
C   --Precip Detection Function
C
      REAL    CZM
      REAL    CZP
      INTEGER NUMROWS, NUMCATS
      PARAMETER (NUMROWS=4, NUMCATS=6)
      INTEGER PRECIP_THRESH( NUMROWS,NUMCATS )
C
      INTEGER CP13ALG_FIRST,CP13ALG_LAST
C
      COMMON /CP13ALG/ CP13ALG_FIRST
C
C     VIL/Echo Tops Algorithm
C
      COMMON /CP13ALG/ EBMWT, ENREF, MEVIL
C
C     Precipitation Detection
C
      COMMON /CP13ALG/ CZM, CZP, PRECIP_THRESH
C
C  Hydromet Subseries Algorithms
C
C   --Enhanced Preprocessing
C
C ** Adaptaion data.  Implementer should obtain as define below
      REAL    BEAM_WIDTH
      REAL    BLOCKAGE_THRESHOLD
      INTEGER CLUTTER_THRESHOLD
      REAL    WEIGHT_THRESHOLD
      REAL    FULL_HYS_THRESHOLD
      REAL    LOW_DBZ_THRESHOLD

C ** Rain detection parameters
      REAL    RAIN_DBZ_THRESHOLD
      INTEGER RAIN_AREA_THRESHOLD
      INTEGER RAIN_TIME_THRESHOLD
      INTEGER NUMZONE

C ** Number exclusion zone limits
      REAL    BEG_AZM1, BEG_AZM2, BEG_AZM3, BEG_AZM4, BEG_AZM5
      REAL    BEG_AZM6, BEG_AZM7, BEG_AZM8, BEG_AZM9, BEG_AZM10
      REAL    BEG_AZM11, BEG_AZM12, BEG_AZM13, BEG_AZM14
      REAL    BEG_AZM15, BEG_AZM16, BEG_AZM17, BEG_AZM18
      REAL    BEG_AZM19, BEG_AZM20
      REAL    END_AZM1, END_AZM2, END_AZM3, END_AZM4, END_AZM5
      REAL    END_AZM6, END_AZM7, END_AZM8, END_AZM9, END_AZM10
      REAL    END_AZM11, END_AZM12, END_AZM13, END_AZM14
      REAL    END_AZM15, END_AZM16, END_AZM17, END_AZM18
      REAL    END_AZM19, END_AZM20
      INTEGER BEG_RNG1, BEG_RNG2, BEG_RNG3, BEG_RNG4, BEG_RNG5
      INTEGER BEG_RNG6, BEG_RNG7, BEG_RNG8, BEG_RNG9, BEG_RNG10
      INTEGER BEG_RNG11, BEG_RNG12, BEG_RNG13, BEG_RNG14
      INTEGER BEG_RNG15, BEG_RNG16, BEG_RNG17, BEG_RNG18
      INTEGER BEG_RNG19, BEG_RNG20
      INTEGER END_RNG1, END_RNG2, END_RNG3, END_RNG4, END_RNG5
      INTEGER END_RNG6, END_RNG7, END_RNG8, END_RNG9, END_RNG10
      INTEGER END_RNG11, END_RNG12, END_RNG13, END_RNG14
      INTEGER END_RNG15, END_RNG16, END_RNG17, END_RNG18
      INTEGER END_RNG19, END_RNG20
      REAL    ELEV_AGL1, ELEV_AGL2, ELEV_AGL3, ELEV_AGL4
      REAL    ELEV_AGL5, ELEV_AGL6, ELEV_AGL7, ELEV_AGL8
      REAL    ELEV_AGL9, ELEV_AGL10, ELEV_AGL11, ELEV_AGL12
      REAL    ELEV_AGL13, ELEV_AGL14, ELEV_AGL15, ELEV_AGL16
      REAL    ELEV_AGL17, ELEV_AGL18, ELEV_AGL19, ELEV_AGL20
C
      REAL    MINDBZRFL
      REAL    MAXDBZRFL
C
C   --Rate
C
      INTEGER RNGCUTOFF
      REAL    RNGCOEF1
      REAL    RNGCOEF2
      REAL    RNGCOEF3
      REAL    MINPRATE
      REAL    MAXPRATE
C
C   --Accumulation
C
      INTEGER TIMRESTRT
      INTEGER MAXTIMINT
      INTEGER MINTIMPD
      INTEGER THOURLI
      INTEGER ENTIMGAG
      INTEGER MAXPRDVAL
      INTEGER MAXHLYVAL
C
C   --Adjustment
C
      INTEGER TIMBIEST
      INTEGER MINNPAIRS
      REAL    RESETBI
      INTEGER LONGSTLAG
      LOGICAL BIAS_FLAG
C
     Hydromet Enhanced Preprocessing
C
      COMMON /CP13ALG/ BEAM_WIDTH, BLOCKAGE_THRESHOLD,
     *                 CLUTTER_THRESHOLD,
     *                 WEIGHT_THRESHOLD, FULL_HYS_THRESHOLD,
     *                 LOW_DBZ_THRESHOLD, RAIN_DBZ_THRESHOLD,
     *                 RAIN_AREA_THRESHOLD, RAIN_TIME_THRESHOLD,
     *                 MINDBZRFL, MAXDBZRFL, NUMZONE,
     *    BEG_AZM1, END_AZM1, BEG_RNG1, END_RNG1,ELEV_AGL1,
     *    BEG_AZM2, END_AZM2, BEG_RNG2, END_RNG2,ELEV_AGL2,
     *    BEG_AZM3, END_AZM3, BEG_RNG3, END_RNG3,ELEV_AGL3,
     *    BEG_AZM4, END_AZM4, BEG_RNG4, END_RNG4,ELEV_AGL4,
     *    BEG_AZM5, END_AZM5, BEG_RNG5, END_RNG5,ELEV_AGL5,
     *    BEG_AZM6, END_AZM6, BEG_RNG6, END_RNG6,ELEV_AGL6,
     *    BEG_AZM7, END_AZM7, BEG_RNG7, END_RNG7,ELEV_AGL7,
     *    BEG_AZM8, END_AZM8, BEG_RNG8, END_RNG8,ELEV_AGL8,
     *    BEG_AZM9, END_AZM9, BEG_RNG9, END_RNG9,ELEV_AGL9,
     *    BEG_AZM10, END_AZM10, BEG_RNG10, END_RNG10,ELEV_AGL10,
     *    BEG_AZM11, END_AZM11, BEG_RNG11, END_RNG11,ELEV_AGL11,
     *    BEG_AZM12, END_AZM12, BEG_RNG12, END_RNG12,ELEV_AGL12,
     *    BEG_AZM13, END_AZM13, BEG_RNG13, END_RNG13,ELEV_AGL13,
     *    BEG_AZM14, END_AZM14, BEG_RNG14, END_RNG14,ELEV_AGL14,
     *    BEG_AZM15, END_AZM15, BEG_RNG15, END_RNG15,ELEV_AGL15,
     *    BEG_AZM16, END_AZM16, BEG_RNG16, END_RNG16,ELEV_AGL16,
     *    BEG_AZM17, END_AZM17, BEG_RNG17, END_RNG17,ELEV_AGL17,
     *    BEG_AZM18, END_AZM18, BEG_RNG18, END_RNG18,ELEV_AGL18,
     *    BEG_AZM19, END_AZM19, BEG_RNG19, END_RNG19,ELEV_AGL19,
     *    BEG_AZM20, END_AZM20, BEG_RNG20, END_RNG20,ELEV_AGL20
C
C     Hydromet Rate
C
      COMMON /CP13ALG/ RNGCUTOFF, RNGCOEF1,  RNGCOEF2,  RNGCOEF3,
     *                 MINPRATE,  MAXPRATE
C
C     Hydromet Accumulation
C
      COMMON /CP13ALG/ TIMRESTRT, MAXTIMINT, MINTIMPD,  THOURLI,
     *                 ENTIMGAG,  MAXPRDVAL, MAXHLYVAL
C
C     Hydromet Adjustment
C
      COMMON /CP13ALG/ TIMBIEST, MINNPAIRS, RESETBI, LONGSTLAG,
     *                 BIAS_FLAG
C
C  CPC 14 Layer Composite Reflectivity - AP Removed
C  (This really is CPC 14 but we include it in CPC 13 )
C
      REAL      ADMINDBZ
      INTEGER*4 ADALTLIM1
      INTEGER*4 ADALTLIM2
      INTEGER*4 ADDISLIM1
      INTEGER*4 ADDISLIM2
      INTEGER*4 ADDISLIM3
      REAL      ADELVLIM1
      REAL      ADELVLIM2
      REAL      ADREJVEL
      REAL      ADREJWID
      REAL      ADACPTVEL
      REAL      ADACPTWID
      INTEGER*4 ADRNGGATE
      REAL      ADDBZDIFF
      LOGICAL   ADIFEXTND
      LOGICAL   ADIFMEDIAN
      INTEGER*4 ADRNGMEDIAN
      INTEGER*4 ADCRMEDIAN
      REAL      ADPGDMEDIAN
C
C     Later Reflectivity/AP Removal (layer_reflectivity.h)
C
      COMMON /CP13ALG/ ADMINDBZ,    ADALTLIM1,  ADALTLIM2, ADDISLIM1,
     *                 ADDISLIM2,   ADDISLIM3,  ADELVLIM1, ADELVLIM2,
     *                 ADREJVEL,    ADREJWID,   ADACPTVEL, ADACPTWID,
     *                 ADIFEXTND,   ADRNGGATE,  ADDBZDIFF, ADIFMEDIAN,
     *                 ADRNGMEDIAN, ADCRMEDIAN, ADPGDMEDIAN
C
      COMMON /CP13ALG/ CP13ALG_LAST

**********************************************************************/

#include <orpgctype.h>
#include <vil_echo_tops.h>
#include <precip_detect.h>
#include <hydromet.h>
#include <layer_reflectivity.h>

/*	cp13alg Definition		*/

typedef struct {

	fint		cpc13alg_start;	/*  Beginning of data block	*
					 *  (for legacy compatability)	*/
	vil_echo_tops_t	vil;		/*  VIL/Echo Top Data		*/
	precip_detect_t	precip;		/*  Precip Detection Data	*/
	hydromet_t	hydromet;	/*  Hydromet Data		*/
	layer_dbz_t	layer_dbz;	/*  Layer Reflectivity/AP Rmvl  */
	fint		cpc13alg_last;  /*  End of data block		*
					 *  (for legacy compatability)	*/

} Cp13alg_adpt_t;

#endif
