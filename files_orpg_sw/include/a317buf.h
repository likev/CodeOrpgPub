/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2009/03/13 14:23:39 $
 * $Id: a317buf.h,v 1.12 2009/03/13 14:23:39 steves Exp $
 * $Revision: 1.12 $
 * $State: Exp $
 */

#ifndef A317BUF_H
#define A317BUF_H

#include <prodsel.h>

/******************************************************************
                                                                    
       THIS FILE CONTAINS OUTPUT BUFFER PARAMETER AND DEFINITION    
                           FILES FOR CPC-17                         
                                                                    
*******************************************************************/

/**************************************************************
                VAD ALERT PARAMETERS               
                    A317AL2P         
**************************************************************/
 
/* Offset parameters

   PABA = Pointer to the beginning azimuth angle processed, deg(INT*4)
   PAEA = Pointer to the ending azimuth angle processed, deg (INT*4)
   PAHT = Pointer to the height of the vad analysis, feet (INT*4)
   PASP = Pointer to the speed at the lowest height, knots (INT*4)
   PADR = Pointer to the wind direction at the lowest height, deg(I*4)
   PAMS = Pointer to the missing data flag, (INT*4)

   Dimensional parameters

   LEN_VAD_ALERT = Length (full words) of the vad alert buffer
*/
#define PARG			0 
#define PABA			1 
#define PAEA			2 
#define PAHT			3 
#define PASP			4 
#define PADR			5 
#define PAMS			6
#define LEN_VAD_ALERT		7

/****************************************************************
                      VAD OUTPUT BUFFER
                      PARAMETER FILE
***************************************************************/
/*
C*    THE FOLLOWING PARAMETERS DEFINE DIMENSIONS AND INDICES TO
C*    VAD_DATA
C*
C*    VAD_DAT    = NUMBER OF DATA ITEMS FOR VAD
C*      VAD_HTG  = HEIGHT VALUES IN METERS FOR EACH ELEVATION
C*      VAD_RMS  = RMS VALUES FOR EACH ELEVATION
C*      VAD_HWD  = WIND DIRECTION IN DEGREES FOR EACH ELEVATION
C*      VAD_SHW  = WIND SPEED IN M/S FOR EACH ELEVATION
C*
C*    THE FOLLOWING DEFINE OFFSETS TO VADTMHGT BUFFER
C*
C*    VPDT = OFFSET TO THE DATE OF THE CURRENT VOLUME SCAN
C*    VPCV = OFFSET TO THE CURREN VOLUME SCAN NUMBER
C*    VPTI = OFFSET TO THE TIMES
C*    VPND = OFFSET TO THE MISSING DATA VALUE
C*    VPDA = OFFSET TO THE TABLE OF HEIGHT, RMS, DIRECTION AND
C*           SPEED VALUES

*/

#define VAD_DAT			13 
#define VAD_NAZIMUTHS  		400
#define VAD_AMBIG_RANGES  	3
#define MAX_VAD_VOLS  		11

/* Index values for VAD_HT_PARAMS. */
#define VAD_HT_PARAMS 		11 
#define VAD_HTG  		0 
#define VAD_RMS  		1 
#define VAD_HWD  		2 
#define VAD_SHW  		3
#define VAD_NRADS		4 
#define VAD_CF1 		5 
#define VAD_CF2 		6 
#define VAD_CF3 		7
#define VAD_ARP 		8 
#define VAD_SLR 		9 
#define VAD_LCN 		10

/* Index values for VAD_AZM_PARAMS. */
#define VAD_AZM_PARAMS 		3 
#define VAD_AZM 		0 
#define VAD_REF 		1 
#define VAD_VEL 		2

/* Index values for VAD_AR_PARAMS. */
#define VAD_AR_PARAMS 		2 
#define VAD_ART 		0
#define VAD_ARL 		1

#define VPDT			0
#define VPCV			1
#define VPTI			2
#define VPND			(VPTI+MAX_VAD_VOLS)
#define VPDA			(VPND+1)
#define VPDAZ 			(VPDA+MAX_VAD_HTS*VAD_HT_PARAMS)
#define VPDAR			(VPDAZ+MAX_VAD_HTS*VAD_NAZIMUTHS*VAD_AZM_PARAMS)

#define VAD_OBUF_SZ 		(VPDA+MAX_VAD_HTS*(VAD_HT_PARAMS+(VAD_AZM_PARAMS*VAD_NAZIMUTHS)+(VAD_AR_PARAMS*VAD_AMBIG_RANGES)))


/* VADTMHGT Data Structure */
typedef struct {
   int vol_date; /* current vol scan date */
   int vol_num; /* current vol scan number */
   int times[MAX_VAD_VOLS]; /* VAD times - sec after top of hr */
   float missing_data_val;

   /* Hgt based VAD alg data */
   float vad_data_hts[MAX_VAD_HTS][VAD_HT_PARAMS];

   /* Azimuthial based VAD algorithm data */
   float vad_data_azm[MAX_VAD_HTS][VAD_NAZIMUTHS][VAD_AZM_PARAMS];

   /* NYQUIST velocity region data by height */
   float vad_data_ar[MAX_VAD_HTS][VAD_AMBIG_RANGES][VAD_AR_PARAMS];
} Vad_params_t;



/********************* 1DTDA PARAMETER FILE **********************
C
C** Offset Parameters:
C
C*******************************************************************
C  OFFSET     TO       DEFINITION
C  ------  ---------   ---------
C  TBHF   TDAHIFLG    LOGICAL FLAG TO INDICATE WHICH HALF OF THE
C                      TDA1D ATTRIBUTES ARRAY WAS UPDATED MOST RECENT
C  TBPVIP  TDABFPTR    POINTER TO 1ST GOOD PATTERN VECTOR INDEX W/IN
C                      THE TDA1D ATTRIBUTES ARRAY (0 or 3000)
C  TBRIP   TDARIPTR    POINTER TO 1ST GOOD RADIAL INDEX WITHIN THE
C                      TDA1D ATTRIBUTES ARRAY (1 or 401)
C  TBAZP   TDAAZPTR    POINTER TO 1ST GOOD AZIMUTHAL INDEX W/IN THE
C                      TDA1D ATTRIBUTES ARRAY (1 or 401)
C  TBAE    AVG_ELEV    AVG. ELEV ANGLE W/IN THE ELEVATION SCAN (deg)
C  TBPVC   PVCOUNT     # OF PATTERN VECTORS IN THE ELEVATION SCAN
C  TBET    ELV_TIME    BEGINNING TIME OF THE ELEVATION SCAN (msec)
C  TBESF   ELEVSTAT    FLAG OF ELEV. SCAN SEQUENCE W/IN THE VOLUME
C                      SCAN. 1=LAST ELEVATION SCAN, 0=OTHER
C  TBRC    RADCOUNT    # OF RADIALS IN THE ELEVATION SCAN
C  TBBS    DBINSIZE    SIZE OF A DOPPLE BIN (meters)
C
C*******************************************************************/
#define TBHF			0 
#define TBPVIP			1 
#define TBRIP			2 
#define TBAZP			3
#define TBAE			4 
#define TBPVC 			5 
#define TBET 			6 
#define TBESF 			7 
#define TBRC 			8 
#define TBBS 			9 

/* Buffer-size parameter: */
#define SIZ_TDA 		(TBBS+1)
 
/* Shared TDA1D A3CD09 Format Offsets
C
C** Dimensional Parameters:
C
C*******************************************************************
C
C   N1D_ATR   # OF PATTERN VECTOR ATTRIBUTES IN THE TDAMAIN ARRAY
C   N1D_MAX   MAX # OF PATTERN VECTORS IN (HALF OF) THE TDAMAIN ARRAY
C   N1D_NID   # OF HALVES TO THE TDA1D ATTRIBUTE ARRAY
C   NRAD_ELEV MAX # OF RADIALS IN AN ELEVATION SCAN
C
C*******************************************************************/
#define N1D_ATR			4 
#define N1D_MAX			3000
#define N1D_NID			2 
#define NRAD_ELEV		400
 
/* Positional Parameters:
C
C*******************************************************************
C
C   PV_DV   PATTERN VECTOR DELTA VELOCITY (m/s)
C   PV_BIN  PATTERN VECTOR BIN #
C   PV_SHR  PATTERN VECTOR SHEAR (m/s/km)
C   PV_BAZ  PATTERN VECTOR BEGINNING AZIMUTH (deg)
C
C*******************************************************************/
#define PV_DV			0 
#define PV_BIN			1 
#define PV_SHR 			2 
#define PV_BAZ			3

/******************************************************************C
C                                                                  C
C    A317PTAD    -  PARAMETER FILE DEFINING LENGTHS AND OFFSETS    C
C                   FOR TDA ADAPTATION DATA                        C
C     Created on 4/2/97 by R. Lee                                  C
C                                                                  C
C******************************************************************/
/** Total size of TDA Adaptation Data Buffer: */
#define NTDA_AD  		30 
 
/* Maximum number of Delta Velocity Levels: */
#define MAX_DV_LEVELS 		6 
 
/* Offset to beginning of primary field of local Adaptation Data: */
#define OFF_TDA  		0 
 
/* Offsets within primary field of local Adaptation Data: */
#define TDA_DV1 		0 
#define TDA_DV2 		1 
#define TDA_DV3 		2 
#define TDA_DV4 		3 
#define TDA_DV5 		4 
#define TDA_DV6 		5 
#define TDA_MDV 		6 
#define TDA_MFAR		7 
#define TDA_MPVR		8 
#define TDA_CR1 		9 
#define TDA_CR2 		10 
#define TDA_CRR 		11 
#define TDA_MTBH 		12 
#define TDA_MTBE 		13 
#define TDA_MPVH 		14 
#define TDA_MPVF 		15 
#define TDA_MTED 		16 
#define TDA_MLDV 		17 
#define TDA_MTDV 		18 
#define TDA_MVRD 		19 
#define TDA_MVAD 		20 
#define TDA_MRV 		21 
#define TDA_SAD 		22 
#define TDA_MFPD 		23 
#define TDA_MN2D 		24 
#define TDA_MN3D 		25 
#define TDA_MNPV 		26 
#define TDA_MNET 		27 
#define TDA_MNT 		28 
#define TDA_MADH 		29 

/*************** TDA1D BUFFER LOCK SIZING DEFINITIONS **************/
 
/* Array & Variable Definitions: */
#ifndef LOW
#define LOW 			0 
#endif

#ifndef HIGH
#define HIGH  			1
#endif

#ifndef UNAVAILABLE
#define UNAVAILABLE		1
#endif

#ifndef AVAILABLE
#define AVAILABLE		0
#endif

/*
C***************** TDA ALGORITHMS ADAPTATION DATA ******************
C*************************** (A317CTAD) ****************************
C
C** Array Definitions (equivalent arrays):
C
C     INTEGER*4  TDAADAP( 0:NTDA_AD-1 )
C     REAL     TADADAP( 0:NTDA_AD-1 )
      INTEGER*4  TDAADAP( NTDA_AD )
      REAL     TADADAP( NTDA_AD )
C
C** TDA 1D Variable Definitions (for equivalencing to arrays):
C
      INTEGER*4  ADPMDV,ADPMPVR, ADPMRV, ADPMNPV
C
C** TDA 2D Variable Definitions (for equivalencing to arrays):
C
      INTEGER*4 ADPDV1, ADPDV2, ADPDV3, ADPDV4, ADPDV5, ADPDV6
      INTEGER*4 ADPCRR, ADPMPVF, ADPMFPD, ADPMN2D
C
      REAL  ADPMFAR, ADPMVRD, ADPMVAD, ADPCR1, ADPCR2, ADPMPVH
C
C** TDA 3D Variable Definitions (for equivalencing to arrays):
C
      INTEGER*4  ADPMLDV, ADPMTDV
      INTEGER*4  ADPMN3D, ADPMNET, ADPMNT
C
      REAL    ADPMTED, ADPMTBH, ADPMTBE, ADPSAD, ADPMADH
C
C** Equivalences:
C
C** --Integer & Real arrays...
C
      EQUIVALENCE ( TDAADAP(1), TADADAP(1) )
C
C** --Individual elements & Integer array...
C
      EQUIVALENCE ( TDAADAP(TDA_MDV),ADPMDV )
      EQUIVALENCE ( TDAADAP(TDA_MPVR),ADPMPVR )
      EQUIVALENCE ( TDAADAP(TDA_MRV),ADPMRV )
      EQUIVALENCE ( TDAADAP(TDA_MNPV),ADPMNPV )
      EQUIVALENCE ( TDAADAP(TDA_DV1),ADPDV1 )
      EQUIVALENCE ( TDAADAP(TDA_DV2),ADPDV2 )
      EQUIVALENCE ( TDAADAP(TDA_DV3),ADPDV3 )
      EQUIVALENCE ( TDAADAP(TDA_DV4),ADPDV4 )
      EQUIVALENCE ( TDAADAP(TDA_DV5),ADPDV5 )
      EQUIVALENCE ( TDAADAP(TDA_DV6),ADPDV6 )
      EQUIVALENCE ( TDAADAP(TDA_CRR),ADPCRR )
      EQUIVALENCE ( TDAADAP(TDA_MN2D),ADPMN2D )
      EQUIVALENCE ( TDAADAP(TDA_MPVF),ADPMPVF )
      EQUIVALENCE ( TDAADAP(TDA_MFPD),ADPMFPD )
      EQUIVALENCE ( TDAADAP(TDA_MN3D),ADPMN3D )
      EQUIVALENCE ( TDAADAP(TDA_MLDV),ADPMLDV )
      EQUIVALENCE ( TDAADAP(TDA_MNET),ADPMNET )
      EQUIVALENCE ( TDAADAP(TDA_MNT),ADPMNT )
      EQUIVALENCE ( TDAADAP(TDA_MTDV),ADPMTDV )
C
C** --Individual elements & Real array...
C
      EQUIVALENCE ( TADADAP(TDA_MPVH),ADPMPVH )
      EQUIVALENCE ( TADADAP(TDA_MFAR),ADPMFAR )
      EQUIVALENCE ( TADADAP(TDA_MVRD),ADPMVRD )
      EQUIVALENCE ( TADADAP(TDA_MVAD),ADPMVAD )
      EQUIVALENCE ( TADADAP(TDA_CR1),ADPCR1 )
      EQUIVALENCE ( TADADAP(TDA_CR2),ADPCR2 )
      EQUIVALENCE ( TADADAP(TDA_MTED),ADPMTED )
      EQUIVALENCE ( TADADAP(TDA_MTBH),ADPMTBH )
      EQUIVALENCE ( TADADAP(TDA_MTBE),ADPMTBE )
      EQUIVALENCE ( TADADAP(TDA_SAD),ADPSAD )
      EQUIVALENCE ( TADADAP(TDA_MADH),ADPMADH )
C
C** Named Common:
C
      COMMON /A317CTAD/  TDAADAP
C
*/

/********************* TDA 2D&3D PARAMETER FILE *********************
C**************************** (A317TP9) *****************************
C
C** TVSATTR Output Buffer Dimensional Parameters
C
C*******************************************************************
C
C   TVFEAT_CHR  # OF TVS FEATURE ATTRIBUTES IN TVS_MAIN
C   TVFEAT_MAX  MAXIMUM # OF TVS FEATURES (TVSs + ETVSs) IN TVS_MAIN
C   NTDA_ADP    # OF TDA ADAPTABLE PARAMETERS IN THE TVSATTR BUFFER
C
C*******************************************************************/
#define TVFEAT_CHR		13 
#define TVFEAT_MAX		50 
#define NTDA_ADP		30
 
/* POINTERS FOR TVSATTR_MAIN ARRAY AND TVS3D ARRAY */
 
/*  PARAMETER DEFINITIONS FOR TVS FEATURES
C
C*******************************************************************
C
C  TV_TYP  TVS FEATURE TYPE: TVS=1, ETVS=2
C  TV_AZM  AZIMUTH OF FEATURE BASE (deg)
C  TV_RAN  SLANT RANGE OF FEATURE BASE (km)
C  TV_LDV  LOW-LEVEL DELTA VELOCITY (m/s)
C  TV_ADV  AVERAGE DELTA VELOCITY (m/s)
C  TV_MDV  MAXIMUM DELTA VELOCITY (m/s)
C  TV_MVH  HEIGHT OF MAXIMUM DELTA VELOCITY (km)
C  TV_DEP  DEPTH OF FEATURE, NEGATIVE MEANS TOP OR BASE IS ON HIGHEST
C          OR LOWEST ELEVATION SCAN, RESPECTIVELY (km)
C  TV_BAS  BASE HEIGHT (km)
C  TV_TOP  TOP HEIGHT (km)
C  TV_SHR  MAXIMUM SHEAR (m/s/km)
C  TV_SRH  HEIGHT OF MAXIMUM SHEAR (km)
C
C*******************************************************************/
#define TV_TYP 			0 
#define TV_AZM 			1 
#define TV_RAN 			2 
#define TV_LDV 			3
#define TV_ADV 			4 
#define TV_MDV 			5 
#define TV_MVH 			6 
#define TV_DEP  		7
#define TV_BAS 			8 
#define TV_BEL 			9 
#define TV_TOP 			10 
#define TV_SHR 			11
#define TV_SRH 			12

/* Offset Parameters: */
#define TVT			0 
#define TVD			1 
#define TNT			2 
#define TNE 			3 
#define TAM			4
#define TAD 			(TAM+TVFEAT_CHR*TVFEAT_MAX)
 
/* Buffer-size parameter:
   Must add ONE to size since Buffer Offsets start at ZERO. */
#define SIZ_TVS 		(TAD+NTDA_ADP+1)

#endif
