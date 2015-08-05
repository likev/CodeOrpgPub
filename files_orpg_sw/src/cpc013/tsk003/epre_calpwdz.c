/*
 * RCS info
 * $Author: aamirn $
 * $Locker:  $
 * $Date: 2008/01/04 17:09:30 $
 * $Id: epre_calpwdz.c,v 1.3 2008/01/04 17:09:30 aamirn Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

#include <math.h>
#include<epreConstants.h>

extern double lowZLimit_val;

/*****************************************************************************
 Converts the Level II "biased" reflectivity data to power (in mm^6/m^3) and
 corrects for partial radar beam blockage

******************************************************************************/

double biasedDBZ2power (int biasedDBZ, double blockage, int bin)
{

 double  dBZ, eqvPower;

 if ( biasedDBZ <= 1 )
     eqvPower = lowZLimit_val;
 else {
      dBZ = (double)(biasedDBZ - 66) / 2.0;
      eqvPower = pow(10., (dBZ / 10.)) / (1.0 - blockage);
      }
 return(eqvPower);
}

/*****************************************************************************
 Converts reflectivity from power (in mm^6/m^3) to Level II "biased"
 reflectivity

 Change History
 ==============
  DATE          VERSION    PROGRAMMER         NOTES
  ----------    -------    ---------------    ------------------
  01/15/02      0000       Tim O'Bannon       CCR
  10/26/05      0001       C. Pham            CCR NA05-21401
*****************************************************************************/

int power2biasedDBZ (double eqvPower)
{

 int biasedDBZ;
 double dBZ, fdBZ,
 
 roundErr = lowZLimit_val/ 10.;

 if ( eqvPower <= lowZLimit_val + roundErr )
    biasedDBZ = 0;

 else {
      dBZ = ((double)10.) * log10((double)eqvPower);
/*      // Minimizing round off error by adding 0.05 to round the first */
/*      // decimal place and then  truncate.*/
      fdBZ = (double)(int)(10. * (dBZ+0.05))/10.;
      biasedDBZ = 2 * fdBZ + 66;
      }

 return(biasedDBZ);
}
