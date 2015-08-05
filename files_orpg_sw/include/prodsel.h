/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2009/03/13 15:03:13 $
 * $Id: prodsel.h,v 1.6 2009/03/13 15:03:13 steves Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */  
/**********************************************************************

	Header file mapping to the PRODSEL common block in the legacy
	file "a309adpt.inc (selectable product parameters).

**********************************************************************/


/********************************************************************
**A3CD70C7
*VERSION:2
C*************************************************************
C  PRODUCT SELECTION CONTROL ADAPTATION DATA
C*************************************************************
C
C  Product Selection Control Adaptation Data
       INTEGER MAXVHGTS, MAX_VAD_HTS, MAX_NUM_AMBRNG, MAX_VAD_ELVS
       PARAMETER  (MAXVHGTS=19, MAX_VAD_HTS = 30, MAX_NUM_AMBRNG=3)
       PARAMETER  (MAX_VAD_ELVS = 20)
       INTEGER*4 PSCPNL0D
       INTEGER*4 PSCPNL1D
       INTEGER*4 PSCPNL2D
       INTEGER*4 PSCPNL3D
       INTEGER*4 PSCPNRNG
       REAL*4 XOFF,YOFF,DELTATHE,BOXSIZ
       REAL THRESH_GT230
       INTEGER NCENTROIDS,VAD_RCM_HGTS(MAXVHGTS),VAD_AHTS(MAX_VAD_HTS)
C
      INTEGER PRODSEL_FIRST,PRODSEL_LAST
C
      COMMON /PRODSEL/ PRODSEL_FIRST
C
C     Layer Product Parameters
C
      COMMON /PRODSEL/ PSCPNL0D, PSCPNL1D, PSCPNL2D, PSCPNL3D,
     *                 PSCPNRNG
C
C     RCM Product Parameters
C
      COMMON /PRODSEL/ THRESH_GT230, BOXSIZ, DELTATHE, XOFF, YOFF
C
C     VAD/RCM Height Selection
C
      COMMON /PRODSEL/ VAD_AHTS,VAD_RCM_HGTS
C
      COMMON /PRODSEL/ PRODSEL_LAST
C
************************************************************************/

#ifndef	PRODSEL_H
#define PRODSEL_H

#include <product_parameters.h>
#define MAX_VAD_HTS 	30

typedef struct product_select {

   int		pscpnlod;

   int		pscpnl1d;

   int		pscpnl2d;

   int		pscpnl3d;

   int		pscpnrng;

   float	thresh_gt230;

   float	boxsiz;

   float	deltathe;

   float	xoff;

   float	yoff;

   vad_rcm_heights_t vad_rcm_heights;

} Prodsel_t;

#endif
