/*
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2006/02/09 17:08:13 $
 * $Id: epre_iniwt.c,v 1.2 2006/02/09 17:08:13 ryans Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/*****************************************************************************
  c_init_wt_arrays() Initialize the azimuthal weighting information

    Change History
    ============
    DATE          VERSION    PROGRAMMER         NOTES
    ----------    -------    ---------------    ------------------
    01/15/02      0000       Tim O'Bannon       CCR
    10/26/05      0001       C. Pham            CCR NA05-21401
******************************************************************************/

#include"epreConstants.h"      /* Defines the constants used in the EPRE */

extern double sumWts[MAX_AZM][MAX_RNG];
extern double sumWtdZ[MAX_AZM][MAX_RNG];

void c_init_wt_arrays()
{

 int azm,
     rng;

 for ( azm = 0; azm < MAX_AZM; azm++ )
     for ( rng = 0; rng < MAX_RNG; rng++ ){
         sumWtdZ[azm][rng] = ZERO;
         sumWts[azm][rng] = ZERO;
      }

}
