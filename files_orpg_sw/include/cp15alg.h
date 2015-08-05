/*
 * RCS info
 * $Author: nolitam $
 * $Locker:  $
 * $Date: 2002/12/11 22:10:07 $
 * $Id: cp15alg.h,v 1.2 2002/12/11 22:10:07 nolitam Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/*	This header file defines the ORPG structure for the legacy	*
 *	common block CP15ALG.						*/


#ifndef CP15ALG_H
#define	CP15ALG_H
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
C  CPC 15 Storm Cell Identification and Tracking Algorithms
C
C --Storm Cell Segments
C
      INTEGER*4 MAX_REF_THRESH
      PARAMETER (MAX_REF_THRESH = 7)
C
C      --Integer Adaptation Data
C
         INTEGER*4 REFLECTH(MAX_REF_THRESH)
         INTEGER*4 DRREFDFF
         INTEGER*4 NDROPBIN
         INTEGER*4 NUMSEGMX
         INTEGER*4 NUMAVGBN
         INTEGER*4 RADSEGMX
         INTEGER*4 SEGRNGMX
         INTEGER*4 NREFLEVL
C
C      --Real Adaptation Data
C
         REAL      SEGLENTH(MAX_REF_THRESH)
         REAL      MWGTFCTR
         REAL      MULTFCTR
         REAL      MCOEFCTR
C
C --Storm Cell Components/Centroids
C
      INTEGER*4 MAX_RADIUS_THRESH
      PARAMETER (MAX_RADIUS_THRESH = 3)
C
C      --Integer Adaptation Data
C
         INTEGER*4 OVLAPADJ
         INTEGER*4 MXPOTCMP
         INTEGER*4 NUMCMPMX
         INTEGER*4 MXDETSTM
         INTEGER*4 NBRSEGMN
         INTEGER*4 NUMSTMMX
         INTEGER*4 STMVILMX
C
C      --Real Adaptation Data
C
         REAL      CMPARETH(MAX_REF_THRESH)
         REAL      RADIUSTH(MAX_RADIUS_THRESH)
         REAL      DEPTHDEL
         REAL      HORIZDEL
         REAL      ELVMERGE
         REAL      HGTMERGE
         REAL      HRZMERGE
         REAL      AZMDLTHR
C
C --Storm Cell Tracking/Position Forecast
C
C
C      --Integer Adaptation Data
C
         INTEGER*4 NPASTVOL
         INTEGER*4 NUMFRCST
         INTEGER*4 FRCINTVL
         INTEGER*4 ALLOWERR
         INTEGER*4 ERRINTVL
         INTEGER*4 DEFDIREC
         INTEGER*4 MAXVTIME
C
C      --Real Adaptation Data
C
         REAL      DEFSPEED
         REAL      CORSPEED
         REAL      SPEEDMIN
C
C --CPC 15 Hail Detection Algorithm
C
C
C      --Integer Adaptation Data
C
C
         INTEGER*4 REF_W_LL
         INTEGER*4 REF_W_UL
         INTEGER*4 MNPOHREF
         INTEGER*4 MXHALRNG
         INTEGER*4 POSHOFST
         INTEGER*4 RCMPRBHL
         INTEGER*4 RCMPOSHL
         INTEGER*4 HADATE(3)
         INTEGER*4 HATIME(3)
         INTEGER*4 MAX_STI_ALPHA_CELLS
         INTEGER*4 MAX_SS_ALPHA_CELLS
         INTEGER*4 MAX_HI_ALPHA_CELLS
         INTEGER*4 MAX_STI_ATTR_CELLS
         INTEGER*4 MAX_CAT_ATTR_CELLS
         INTEGER*4 MAX_HI_ATTR_CELLS
C
C      --Real Adaptation Data
C
         REAL     HT0
         REAL     HT20
         REAL     HKECOEF1
         REAL     HKECOEF2
         REAL     HKECOEF3
         REAL     POSHCOEF
         REAL     HAILSZCF
         REAL     HAILSZEX
         REAL     WT_SM_CF
         REAL     WT_SM_OF
         REAL     POHTDIF1
         REAL     POHTDIF2
         REAL     POHTDIF3
         REAL     POHTDIF4
         REAL     POHTDIF5
         REAL     POHTDIF6
         REAL     POHTDIF7
         REAL     POHTDIF8
         REAL     POHTDIF9
         REAL     POHTDIF0
C
C   Global Common Definition
C
      INTEGER*4 CP15ALG_FIRST,CP15ALG_LAST
C
      COMMON /CP15ALG/ CP15ALG_FIRST
C
C     Storm Cell Segments
C
      COMMON /CP15ALG/ REFLECTH, SEGLENGTH, DRREFDFF, NDROPBIN,
     *                 NREFLEVL, SEGRNGMX, RADSEGMX, NUMSEGMX,
     *                 NUMAVGBN, MWGTFCTR, MULTFCTR, MCOEFCTR
C
C     Storm Cell Components/Centroids
C
      COMMON /CP15ALG/ OVLAPADJ, MXPOTCMP, NUMCMPMX, MXDETSTM,
     *                 NBRSEGMN, NUMSTMMX, STMVILMX, CMPARETH,
     *                 RADIUSTH, DEPTHDEL, HORIZDEL, ELVMERGE,
     *                 HGTMERGE, HRZMERGE, AZMDLTHR
C
C     Storm Cell Tracking/Position Forecast Data
C
      COMMON /CP15ALG/ NPASTVOL, NUMFRCST, FRCINTVL, ALLOWERR,
     *                 ERRINTVL, DEFDIREC, MAXVTIME, DEFSPEED,
     *                 CORSPEED, SPEEDMIN
C
C     Hail Detection
C
      COMMON /CP15ALG/ REF_W_LL, REF_W_UL, MNPOHREF, HKECOEF1,
     *                 HKECOEF2, HKECOEF3, POSHCOEF, POSHOFST,
     *                 MXHALRNG, HAILSZCF, HAILSZEX, WT_SM_CF,
     *                 WT_SM_OF, POHTDIF1, POHTDIF2, POHTDIF3,
     *                 POHTDIF4, POHTDIF5, POHTDIF6, POHTDIF7,
     *                 POHTDIF8, POHTDIF9, POHTDIF0, RCMPRBHL,
     *                 RCMPOSHL
C
C     Hail environment
C
      COMMON /CP15ALG/ HT0, HT20, HADATE, HATIME
C
C     Cell Product Parameters
C
      COMMON /CP15ALG/ MAX_STI_ALPHA_CELLS, MAX_SS_ALPHA_CELLS,
     *                 MAX_HI_ALPHA_CELLS,  MAX_STI_ATTR_CELLS,
     *                 MAX_CAT_ATTR_CELLS, MAX_HI_ATTR_CELLS
C
      COMMON /CP15ALG/ CP15ALG_LAST
**********************************************************************/

#include <orpgctype.h>

#include <storm_cell.h>
#include <hail_detection.h>
#include <product_parameters.h>


/*	cp15alg Definition		*/

typedef struct {

	fint		cpc15alg_start;	/*  Beginning of data block	*
					 *  (for legacy compatability)	*/
	storm_cell_t		storm;
					/* Storm Cell Segments/Components/ *
					 * Tracking (used by storm cell ID *
					 * and tracking algorithms.        */
	hail_t			hail;
					/* Hail detection algorithm	*/
	cell_prod_params_t	cell;
					/* Cell selectable product	*
					 * parameters.			*/
	fint		cpc15alg_last;  /*  End of data block		*
					 *  (for legacy compatability)	*/

} Cp15alg_adpt_t;

#endif
