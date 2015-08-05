/*$(
 *======================================================================= 
 * 
 *   (c) Copyright, 2012 Massachusetts Institute of Technology.
 *       This material may be reproduced by or for the 
 *       U.S. Government pursuant to the copyright license 
 *       under the clause at 252.227-7013 (Jun. 1995).
 * 
 *
 *=======================================================================
 *
 *
 *   FILE:    cip_memLookup.c
 *
 *   AUTHOR:  Robert G Hallowell
 *
 *   CREATED: May 16, 2011; Initial Version
 *
 *   REVISION: 10/12/11		M. Donovan
 *             Modified the CIP membership functions to match the
 *             NCAR step-wise linear functions.
 * 
 *=======================================================================
 *
 *   DESCRIPTION:
 *
 *   This module creates a lookup table of interest values for the
 *   NCAR Current Icing Potential (CIP) Temperature and Relative Humidity 
 *   membership functions.
 *
 *   FUNCTIONS:
 *
 *   float T_lookup( float tmp )
 *   float RH_lookup( float rh )
 *   void create_cipLookup()
 *
 *   NOTES:    
 *
 *
 *=======================================================================
 *$)
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#define Tbegin  -25
#define Tend    0
#define RHbegin 70
#define RHend   100

float TempLookup[26], RhLookup[31];

/******************************************************************
   
   Description:
      Lookup the interest value associated with a model temperature
      value.

   Input:
      tmp - model temperature, in degC

   Return:
      Interest value from temperature lookup table.

******************************************************************/
float T_lookup( float tmp )
{
    if ( tmp < Tbegin || tmp > Tend )
      return( TempLookup[0] );
    else
      return ( TempLookup[(int)(tmp - Tbegin)] );

/* End of T_lookup() */
}

/******************************************************************
   
   Description:
      Lookup the interest value associated with a model relative 
      humidity value.

   Input:
      rh - model relative humidity, in %

   Return:
      Interest value from relative humidity lookup table.

******************************************************************/
float RH_lookup( float rh )
{
    if ( rh < RHbegin ) 
      return( RhLookup[0] );
    else if ( rh > RHend )
      return( RhLookup[RHend-RHbegin] );
    else
      return ( RhLookup[(int)(rh - RHbegin)] ); 

/* End of RH_lookup() */ 
}

/******************************************************************
   
   Description:
      Loop through a range of temperature and relative humidity
      values and build a lookup table that inidicates the 
      likelihood (interest) that icing occurs for that particular
      model value alone.

******************************************************************/
void create_cipLookup()
{

  int i;
  
  float tmp, rh;
  float tmp_int=0.0, rh_int=0.0;

  /* calculate temperature interest */
  for (i=Tbegin; i<=Tend; i++)
  {
    tmp = (float)i;

    if (tmp > Tbegin && tmp < -12.0)
      tmp_int = (tmp - Tbegin) / 13.0;
    else if (tmp >= -12.0 && tmp <= -5.0)
      tmp_int = 1.0;
    else if (tmp > -5.0 && tmp <= Tend)
      tmp_int = (Tend - tmp) / 5.0;
    else if (tmp <= Tbegin || tmp > Tend)
      tmp_int = 0.0;

    TempLookup[i - Tbegin] = tmp_int;
  }

  /* calculate relative humidity interest */
  for (i=RHbegin; i<=RHend; i++)
  {
    rh = (float)i;

    if (rh > RHbegin && rh < RHend)
      rh_int = (rh - RHbegin) / 30.0;
    else if (rh <= RHbegin)
      rh_int = 0.0;
    else if (rh >= RHend)
      rh_int = 1.0;

    RhLookup[i - RHbegin] = rh_int;
  }

/* End of create_cipLookup() */
}
