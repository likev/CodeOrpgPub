/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2008/01/07 23:19:39 $
 * $Id: cp17alg.h,v 1.4 2008/01/07 23:19:39 steves Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */

/*	This header file defines the ORPG structure for the legacy	*
 *	common block CP17ALG.						*/


#ifndef CP17ALG_H
#define	CP17ALG_H

/*********************************************************************
*.
*.           I N C L U D E    F I L E    P R O L O G U E
*.
*.  INCLUDE FILE NAME: A309ADPT.INC
*.
             C P C - 1 7   A D A P T A T I O N   V A L U E S
**************************************************************************
 
  VAD Adaptatable Parameters
 ---------------------------
 
    VARIABLE DESCRIPTIONS:
 
        DESCRIPTION        NAME       FORMAT     RANGE
        -----------        ----       ------     -----
 
        Analysis Range     EVR        REAL*4     km
 
        Beginning azimuth  EBA        REAL*4     deg
 
        Ending Azimuth     EEA        REAL*4     deg
 
        Threshold Velocity EVEL       REAL*4     m/s
 
        Threshold Symmetry ESYM       REAL*4     m/s
 
        # of Fit Tests     ENFT       INT*4       #
 
        Min # Sample Pts   NPTS_IN_SAMPLE INT*4   #
 
 
  Mesocyclone Adaptable Parameters
  -------------------------------------
 
  *******************************************************************
                                                                    *
                        Variable Descriptions                       *
  ----------------------------------------------------------------- *
  *                                                                 *
  *    EMNF       I*4   Maximum number of features in volume scan.  *
  *    EMNM       I*4   Maximum number of mesocyclones in volume.   *
  *    EPVN       I*4   Minimum number of pattern vectors for       *
  *                     feature.                                    *
  *    EFH        R*4   Maximum allowable height for feature.       *
  *    EHM        R*4   High momentum threshold (m**2/sec).         *
  *    ELM        R*4   Low momentum threshold (m**2/sec).          *
  *    EHS        R*4   High shear threshold (1/sec).               *
  *    ELS        R*4   Low shear threshold (1/sec).                *
  *    EMXR       R*4   Maximum diameter ratio threshold.           *
  *    EFMXR      R*4   Far maximum diameter ratio threshold.       *
  *    EMNR       R*4   Minimum diameter ratio threshold.           *
  *    EFMNR      R*4   Far minimum diameter ratio threshold.       *
  *    ERNG       R*4   Range to far minimum and maximum ratios(km).*
  *    ERD        R*4   Mimimum radial distance for feature (km).   *
  *    EPVA       R*4   Pattern Vector overlap threshold (deg).     *
  *                                                                 *
  *******************************************************************

  TDA Adadptable Parameters
 -------------------------------------

 *****************************************************************
                       Variable Descriptions
 -----------------------------------------------------------------
 
  *   MINREFL   I * 4   Minimum Reflectivity Threshold
  *   MINPVDV   I * 4   Vector Velocity Difference
  *   MAXPVRNG  I * 4   Maximum Pattern Vector Range
  *   MAXPVHT   R * 4   Maximum Pattern Vector Height
  *   MAXNUMPV  I * 4   Maximum # Pattern Vectors
  *   H2DDV1    I * 4   Differential Velocity #1
  *   TH2DDV2   I * 4   Differential Velocity #2
  *   TH2DDV3   I * 4   Differential Velocity #3
  *   TH2DDV4   I * 4   Differential Velocity #4
  *   TH2DDV5   I * 4   Differential Velocity #5
  *   TH2DDV6   I * 4   Differential Velocity #6
  *   MIN1DP2D  I * 4   Minimum # Vectors Per 2D Feature
  *   MAXPVRD   R * 4   2D Vector Radial Distance
  *   MAXPVAD   R * 4   2D Vector Azimuthal Distance
  *   MAX2DAR   R * 4   Maximum 2D Feature Aspect Ratio
  *   THCR1     R * 4   Circulation Radius #1
  *   THCR2     R * 4   Circulation Radius #2
  *   THCRR     I * 4   Circulation Radius Range
  *   MAXNUM2D  I * 4   Maximum # 2D Features
  *   MIN2DP3D  I * 4   Minimum # 2D Features Per 3D Feature
  *   MINTVSD   R * 4   Minimum 3D Feature Depth
  *   MINLLDV   I * 4   Minimum 3D Feature Low Level Delta
  *                     Velocity
  *   MINMTDV   I * 4   Minimum TVS Delta Velocity
  *   MAXNUM3D  I * 4   Maximum # 3D Features
  *   MAXNUMTV  I * 4   Maximum # TVS
  *   MAXNUMET  I * 4   Maximum # ETVS
  *   MINTVSBH  R * 4   Minimum TVS Base Height
  *   MINTVSBE  R * 4   Minimum TVS Base Elevation
  *   MINADVHT  R * 4   Average Delta Velocity Height
  *   MAXTSTMD  R * 4   Maximum Storm Association Distance

C
C  CPC 17 Kinematic Algorithms
C
C   --VAD
C
      REAL    EVEL
      INTEGER ENFT
      INTEGER NPTS_IN_SAMPLE
      REAL    EVR
      REAL    EBA
      REAL    EEA
      REAL    ESYM
C
C   --Mesocyclone Detection
C
      INTEGER EMNF
      INTEGER EMNM
      REAL    EFH
      REAL    ERD
      REAL    ELM
      REAL    EHM
      REAL    ELS
      REAL    EHS
      REAL    EPVA
      REAL    EPVN
      REAL    EMXR
      REAL    EMNR
      REAL    EFMXR
      REAL    EFMNR
      REAL    ERNG
C
C   --TVS Detection Algorithm
C
      INTEGER  MINREFL
      INTEGER  MINPVDV
      INTEGER  MAXPVRNG
      REAL  MAXPVHT
      INTEGER  MAXNUMPV
      INTEGER  TH2DDV1
      INTEGER  TH2DDV2
      INTEGER  TH2DDV3
      INTEGER  TH2DDV4
      INTEGER  TH2DDV5
      INTEGER  TH2DDV6
      INTEGER  MIN1DP2D
      REAL  MAXPVRD
      REAL  MAXPVAD
      REAL  MAX2DAR
      REAL  THCR1
      REAL  THCR2
      INTEGER  THCRR
      INTEGER  MAXNUM2D
      INTEGER  MIN2DP3D
      REAL  MINTVSD
      INTEGER  MINLLDV
      INTEGER  MINMTDV
      INTEGER  MAXNUM3D
      INTEGER  MAXNUMTV
      INTEGER  MAXNUMET
      REAL  MINTVSBH
      REAL  MINTVSBE
      REAL  MINADVHT
      REAL  MAXTSTMD
C
      INTEGER CP17ALG_FIRST,CP17ALG_LAST
C
      COMMON /CP17ALG/ CP17ALG_FIRST
C
C
C     VAD Algorithm Data
C
      COMMON /CP17ALG/ EVEL, ENFT, NPTS_IN_SAMPLE, EVR,
     *                 EBA,  EEA,  ESYM
C
C     Mesocyclone Algorithm Data
C
      COMMON /CP17ALG/ EMNF,  EMNM,  EFH,  ERD,  ELM,  EHM,
     *                 ELS,   EHS,   EPVA, EPVN, EMXR, EMNR,
     *                 EFMXR, EFMNR, ERNG
C
C     Tornader Detection Algorithm (TDA) Data
C
      COMMON /CP17ALG/ MINREFL,  MINPVDV,  MAXPVRNG, MAXPVHT,  MAXNUMPV,
     *                 TH2DDV1,  TH2DDV2,  TH2DDV3,  TH2DDV4,  TH2DDV5,
     *                 TH2DDV6,  MIN1DP2D, MAXPVRD,  MAXPVAD,  MAX2DAR,
     *                 THCR1,    THCR2,    THCRR,    MAXNUM2D, MIN2DP3D,
     *                 MINTVSD,  MINLLDV,  MINMTDV,  MAXNUM3D, MAXNUMTV,
     *                 MAXNUMET, MINTVSBH, MINTVSBE, MINADVHT, MAXTSTMD
C
      COMMON /CP17ALG/ CP17ALG_LAST

***************************************************************************/

#include <orpgctype.h>
#include <vad.h>
#include <mesocyclone.h>
#include <tda.h>

/* Structure definition for CPC 17 Algorithm Adaptation Data. */

typedef struct {

      fint		cp17alg_first;
      vad_t		vad;
      mesocyclone_t	meso;
      tda_t		tda;
      fint		cp17alg_last;

} Cp17alg_adpt_t;

#endif
