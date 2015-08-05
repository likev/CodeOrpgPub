/*
 * RCS info
 * $Author: christie $
 * $Locker:  $
 * $Date: 2003/07/11 19:17:36 $
 * $Id: mda1d_data_conversion.c,v 1.2 2003/07/11 19:17:36 christie Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/* This code is from Steve Smith in ROC */
#include "mda1d_data_conversion.h"


#define INVALID_DATA        -777.0f

#define DATA_OUT_OF_RANGE        -1
#define MIN_VALUE                 0
#define MIN_DATA_VALUE            2
#define MAX_DATA_VALUE          255
#define MAX_VALUE               256

/* Conversion table for Doppler velocity .... index 0 is for 
   resolution 1 and index 1 is for resolution 2 */
static double VEL[2][257] = { { -999.,-888.,-63.5,-63.,-62.5,-62.,-61.5,-61.,-60.5,-60.,-59.5,-59.,
                               -58.5,-58.,-57.5,-57.,-56.5,-56.,-55.5,-55.,-54.5,-54.,-53.5,-53.,
                               -52.5,-52.,-51.5,-51.,-50.5,-50.,-49.5,-49.,-48.5,-48.,-47.5,-47.,
                               -46.5,-46.,-45.5,-45.,-44.5,-44.,-43.5,-43.,-42.5,-42.,-41.5,-41.,
                               -40.5,-40.,-39.5,-39.,-38.5,-38.,-37.5,-37.,-36.5,-36.,-35.5,-35.,
                               -34.5,-34.,-33.5,-33.,-32.5,-32.,-31.5,-31.,-30.5,-30.,-29.5,-29.,
                               -28.5,-28.,-27.5,-27.,-26.5,-26.,-25.5,-25.,-24.5,-24.,-23.5,
                               -23.,-22.5,-22.,-21.5,-21.,-20.5,-20.,-19.5,-19.,-18.5,-18.,-17.5,
                               -17.,-16.5,-16.,-15.5,-15.,-14.5,-14.,-13.5,-13.,-12.5,-12.,-11.5,
                               -11.,-10.5,-10.,-9.5,-9.,-8.5,-8.,-7.5,-7.,-6.5,-6.,-5.5,-5.,-4.5,
                               -4.,-3.5,-3.,-2.5,-2.,-1.5,-1.,-.5,0.,.5,1.,1.5,2.,2.5,3.,3.5,4.,
                                4.5,5.,5.5,6.,6.5,7.,7.5,8.,8.5,9.,9.5,10.,10.5,11.,11.5,12.,12.5,
                               13.,13.5,14.,14.5,15.,15.5,16.,16.5,17.,17.5,18.,18.5,19.,19.5,
                               20.,20.5,21.,21.5,22.,22.5,23.,23.5,24.,24.5,25.,25.5,26.,26.5,27.,27.5,
                               28.,28.5,29.,29.5,30.,30.5,31.,31.5,32.,32.5,33.,33.5,34.,34.5,
                               35.,35.5,36.,36.5,37.,37.5,38.,38.5,39.,39.5,40.,40.5,41.,41.5,
                               42.,42.5,43.,43.5,44.,44.5,45.,45.5,46.,46.5,47.,47.5,48.,48.5,
                               49.,49.5,50.,50.5,51.,51.5,52.,52.5,53.,53.5,54.,54.5,55.,55.5,
                               56.,56.5,57.,57.5,58.,58.5,59.,59.5,60.,60.5,61.,61.5,62.,62.5,63.,-777.} ,
                             { -999.,-888.,-127.,-126.,-125.,-124., -123.,-122.,-121.,-120.,-119.,
                               -118.,-117.,-116.,-115.,-114.,-113., -112.,-111.,-110.,-109.,-108.,
                               -107.,-106.,-105.,-104.,-103.,-102.,-101.,-100.,-99.,-98.,-97.,-96.,
                               -95.,-94.,-93.,-92.,-91.,-90.,-89.,-88.,-87.,-86.,-85.,-84.,-83.,
                               -82.,-81.,-80.,-79.,-78.,-77.,-76.,-75.,-74.,-73.,-72.,-71.,-70.,
                               -69.,-68.,-67.,-66.,-65.,-64.,-63.,-62.,-61.,-60.,-59.,-58.,-57.,
                               -56.,-55.,-54.,-53.,-52.,-51.,-50.,-49.,-48.,-47.,-46.,-45.,-44.,
                               -43.,-42.,-41.,-40.,-39.,-38.,-37.,-36.,-35.,-34.,-33.,-32.,-31.,
                               -30.,-29.,-28.,-27.,-26.,-25.,-24.,-23.,-22.,-21.,-20.,-19.,-18.,
                               -17.,-16.,-15.,-14.,-13.,-12.,-11.,-10.,-9.,-8.,-7.,-6.,-5.,-4.,
                               -3.,-2.,-1.,0.,1.,2.,3.,4.,5.,6.,7.,8.,9.,10.,11.,12.,13.,14.,15.,
                               16.,17.,18.,19.,20.,21.,22.,23.,24.,25.,26.,27.,28.,29.,30.,31.,
                               32.,33.,34.,35.,36.,37.,38.,39.,40.,41.,42.,43.,44.,45.,46.,47.,
                               48.,49.,50.,51.,52.,53.,54.,55.,56.,57.,58.,59.,60.,61.,62.,63.,
                               64.,65.,66.,67.,68.,69.,70.,71.,72.,73.,74.,75.,76.,77.,78.,79.,
                               80.,81.,82.,83.,84.,85.,86.,87.,88.,89.,90.,91.,92.,93.,94.,95.,
                               96.,97.,98.,99.,100.,101.,102.,103.,104.,105.,106.,107.,108.,109.,
                              110.,111.,112.,113.,114.,115.,116.,117.,118.,119.,120.,121.,122.,
                              123.,124.,125.,126.,-777.} };
   

/******************************************************************

   Description:
      Given the velocity resolution from radial header, returns the 
      table index for referencing this data.

   Inputs:
      reso - velocity increment (resolution) from radial header.

   Outputs:

   Returns:
      The resolution value (table index) for retrieving m/s values.

   Notes:

******************************************************************/
RESO RPGCSX_get_velocity_reso( int reso ){

   switch( reso ){

      case 1:
         return LOW_RES;

      case 2:
         return HIGH_RES;

      default:
         return LOW_RES;

   }

}

/******************************************************************

   Description:
      Given the base data velocity value (as defined in RDA/RPG ICD)
      and velocity resolution, return the corresponding m/s value.  

   Inputs:
      reso - the velocity resolution value.
      value - the data value in scaled and biased units (i.e.,
              ICD format).

   Outputs:

   Returns:
      velocity value in m/s or DATA_OUT_OF_RANGE if the input
      value is out of range.

   Notes:

******************************************************************/
double RPGCSX_velocity_to_ms( RESO reso, int value ){

   if( (value < MIN_VALUE) || (value > MAX_DATA_VALUE) )
      return(INVALID_DATA);

   if( (reso < LOW_RES) || (reso > HIGH_RES) )
      return(INVALID_DATA);

   return( VEL[reso][value] );

}
