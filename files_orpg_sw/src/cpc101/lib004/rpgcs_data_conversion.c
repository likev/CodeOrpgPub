/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2006/09/11 13:59:15 $
 * $Id: rpgcs_data_conversion.c,v 1.7 2006/09/11 13:59:15 steves Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */

#include <rpgcs_data_conversion.h>
#include <math.h>

/* Static global variables. */
static RESO RPGCS_vel_reso = HIGH_RES;

/* Conversion table for equivalent reflectivity */
static float RPGCS_ref[256] = { -999.,-888.,-32.,-31.5,-31.,-30.5,-30.,
                                -29.5,-29.,-28.5,-28.,-27.5,-27.,-26.5,
                                -26.,-25.5,-25.,-24.5,-24.,-23.5,-23.,
                                -22.5,-22.,-21.5,-21.,-20.5,-20.,-19.5,
                                -19.,-18.5,-18.,-17.5,-17.,-16.5,-16.,
                                -15.5,-15.,-14.5,-14.,-13.5,-13.,-12.5,
                                -12.,-11.5,-11.,-10.5,-10.,-9.5,-9.,-8.5,
                                -8.,-7.5,-7.,-6.5,-6.,-5.5,-5.,-4.5,-4.,
                                -3.5,-3.,-2.5,-2.,-1.5,-1.,-.5,0.,.5,1.,
                                 1.5,2.,2.5,3.,3.5,4.,4.5,5.,5.5,6.,6.5,
                                 7.,7.5,8.,8.5,9.,9.5,10.,10.5,11.,11.5,
                                12.,12.5,13.,13.5,14.,14.5,15.,15.5,16.,
                                16.5,17.,17.5,18.,18.5,19.,19.5,20.,20.5,
                                21.,21.5,22.,22.5,23.,23.5,24.,24.5,25.,
                                25.5,26.,26.5,27.,27.5,28.,28.5,29.,29.5,
                                30.,30.5,31.,31.5,32.,32.5,33.,33.5,34.,
                                34.5,35.,35.5,36.,36.5,37.,37.5,38.,38.5,
                                39.,39.5,40.,40.5,41.,41.5,42.,42.5,43.,
                                43.5,44.,44.5,45.,45.5,46.,46.5,47.,47.5,
                                48.,48.5,49.,49.5,50.,50.5,51.,51.5,52.,
                                52.5,53.,53.5,54.,54.5,55.,55.5,56.,56.5,
                                57.,57.5,58.,58.5,59.,59.5,60.,60.5,61.,
                                61.5,62.,62.5,63.,63.5,64.,64.5,65.,65.5,
                                66.,66.5,67.,67.5,68.,68.5,69.,69.5,70.,
                                70.5,71.,71.5,72.,72.5,73.,73.5,74.,74.5,
                                75.,75.5,76.,76.5,77.,77.5,78.,78.5,79.,
                                79.5,80.,80.5,81.,81.5,82.,82.5,83.,83.5,
                                84.,84.5,85.,85.5,86.,86.5,87.,87.5,88.,
                                88.5,89.,89.5,90.,90.5,91.,91.5,92.,92.5,
                                93.,93.5,94.,94.5 };

/* Conversion table for Doppler velocity .... index 0 is for 
   resolution 1 and index 1 is for resolution 2 */
static float RPGCS_vel[2][256] = { { -999.,-888.,-63.5,-63.,-62.5,-62.,-61.5,-61.,-60.5,-60.,-59.5,-59.,
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
                                     56.,56.5,57.,57.5,58.,58.5,59.,59.5,60.,60.5,61.,61.5,62.,62.5,63.} ,
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
                                    123.,124.,125.,126.} };
  
/* Conversion table for spectrum width */
static float RPGCS_spw[256] = { -999.,-888.,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,
                                 0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,
                                 0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,
                                 0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,
                                 0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,
                                 0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,
                                 0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,
                                 0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,
                                 0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.,.5,1.,1.5,2.,2.5,
                                 3.,3.5,4.,4.5,5.,5.5,6.,6.5,7.,7.5,8.,8.5,9.,9.5,10.0,10.0,
                                 10.0,10.0,10.0,10.0,10.0,10.0,10.0,10.0,10.0,10.0,10.0,10.0,
                                 10.0,10.0,10.0,10.0,10.0,10.0,10.0,10.0,10.0,10.0,10.0,10.0,
                                 10.0,10.0,10.0,10.0,10.0,10.0,10.0,10.0,10.0,10.0,10.0,10.0,
                                 10.0,10.0,10.0,10.0,10.0,10.0,10.0,10.0,10.0,10.0,10.0,10.0,
                                 10.0,10.0,10.0,10.0,10.0,10.0,10.0,10.0,10.0,10.0,10.0,10.0,
                                 10.0,10.0,10.0,10.0,10.0,10.0,10.0,10.0,10.0,10.0,10.0,10.0,
                                 10.0,10.0,10.0,10.0,10.0,10.0,10.0,10.0,10.0,10.0,10.0,10.0,
                                 10.0,10.0,10.0,10.0,10.0,10.0,10.0,10.0,10.0,10.0,10.0,10.0,
                                 10.0,10.0,10.0,10.0,10.0,10.0,10.0,10.0,10.0};

/* Static function prototypes. */
static int Nearest_integer( float value );

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
RESO RPGCS_get_velocity_reso( int reso ){

   switch( reso ){

      case 1:
         return HIGH_RES;

      case 2:
         return LOW_RES;

      default:
         return HIGH_RES;

   }

/* End of RPGCS_get_velocity_reso() */
}

/******************************************************************

   Description:
      Given the velocity resolution from radial header, set the 
      table index for referencing this data.

   Inputs:
      reso - velocity increment (resolution) from radial header.

   Outputs:

   Returns:
      The resolution value (table index) for retrieving m/s values.

   Notes:

******************************************************************/
RESO RPGCS_set_velocity_reso( int reso ){

   switch( reso ){

      case 1:
         RPGCS_vel_reso = HIGH_RES;
         return HIGH_RES;

      case 2:
         RPGCS_vel_reso = LOW_RES;
         return LOW_RES;

      default:
         RPGCS_vel_reso = HIGH_RES;
         return HIGH_RES;

   }

/* End of RPGCS_set_velocity_reso() */
}

/******************************************************************

   Description:
      Given the base data reflectivity value (as defined in 
      RDA/RPG ICD), return the corresponding dBZ value.  

   Inputs:
      value - the data value in scaled and biased units (i.e.,
              ICD format).

   Outputs:

   Returns:
      The corresponding dBZ value or INVALID_DATA if value out
      of range.

   Notes:

******************************************************************/
float RPGCS_reflectivity_to_dBZ( int value ){

   if( (value < MIN_VALUE) || (value > MAX_DATA_VALUE) )
      return(INVALID_DATA);

   return( RPGCS_ref[value] );

/* End of RPGCS_reflectivity_to_dBZ() */
}

/******************************************************************

   Description:
      Converts dBZ to scaled and biased units.  

   Inputs:
      value - data value in dBZ.

   Outputs:

   Returns:
      If value out of range, returns DATA_OUT_OF_RANGE.  Otherwise
      returns reflectivity in scaled and biased units.

   Notes:

******************************************************************/
int RPGCS_dBZ_to_reflectivity( float value ){

   int iref = 0;

   /* Verify the reflectivity value. */
   if( (value < Z_MIN_VALUE) || (value > Z_MAX_VALUE) )
      return(DATA_OUT_OF_RANGE);

   /* Scale the reflectivity. */
   value = (value + value) + 64.0;

   /* Take nearest integer. */
   iref = Nearest_integer( value );

   /* Bias the reflectivity. */
   iref += 2;

   return( iref );

/* End of RPGCS_dBZ_to_reflectivity() */
}

/******************************************************************

   Description:
      Given the base data velocity value (as defined in RDA/RPG ICD)
      and velocity resolution, return the corresponding m/s value.  

   Inputs:
      value - the data value in scaled and biased units (i.e.,
              ICD format).

   Outputs:

   Returns:
      velocity value in m/s or DATA_OUT_OF_RANGE if the input
      value is out of range.

   Notes:

******************************************************************/
float RPGCS_velocity_to_ms( int value ){

   if( (value < MIN_VALUE) || (value > MAX_DATA_VALUE) )
      return(INVALID_DATA);

   return( RPGCS_vel[RPGCS_vel_reso][value] );

/* End of RPGCS_velocity_to_ms() */
}

/******************************************************************

   Description:
      Converts m/s to scaled and biased units.  

   Inputs:
      reso - the velocity resolution value.
      value - the data value in m/s.

   Outputs:

   Returns:
      If value out of range, returns DATA_OUT_OF_RANGE.  Otherwise
      returns velocity in scaled and biased units.

   Notes:

******************************************************************/
int RPGCS_ms_to_velocity( RESO reso, float value ){

   int ivel = 0;

   /* Verify the resolution. */
   if( (reso < HIGH_RES) || (reso > LOW_RES) )
      return(INVALID_DATA);

   /* Verify the velocity value. */
   if( (value < RPGCS_vel[reso][MIN_DATA_VALUE]) 
                      || 
       (value > RPGCS_vel[reso][MAX_DATA_VALUE]) )
      return(DATA_OUT_OF_RANGE);

   /* Scale the velocity. */
   if( reso == HIGH_RES )
      value = (value + value) + 127.0;
  
   else
      value += 127.0;

   /* Take nearest integer. */
   ivel = Nearest_integer( value );

   /* Bias the velocity. */
   ivel += 2;

   return( ivel );

/* End of RPGCS_ms_to_velocity() */
}

/******************************************************************

   Description:
      Given the base data spectrum width value (as defined in 
      RDA/RPG ICD), return the corresponding m/s value.  

   Inputs:
      value - the data value in scaled and biased units (i.e.,
              ICD format).

   Outputs:

   Returns:
      spectrum width value in m/s or DATA_OUT_OF_RANGE if the input
      value is out of range.

   Notes:

******************************************************************/
float RPGCS_spectrum_width_to_ms( int value ){

   if( (value < MIN_VALUE) || (value > MAX_DATA_VALUE) )
      return(INVALID_DATA);

   return( RPGCS_spw[value] );

/* End of RPGCS_spectrum_width_to_ms() */
}

/******************************************************************

   Description:
      Converts m/s to scaled and biased units.  

   Inputs:
      value - the data value in m/s.

   Outputs:

   Returns:
      If value out of range, returns DATA_OUT_OF_RANGE.  Otherwise
      returns spectrum width in scaled and biased units.

   Notes:

******************************************************************/
int RPGCS_ms_to_spectrum_width( float value ){

   int ispw = 0;

   /* Verify the velocity value. */
   if( (value < W_MIN_VALUE) || (value > W_MAX_VALUE) )
      return(DATA_OUT_OF_RANGE);

   /* Scale the spectrum width. */
   value = (value + value) + 127.0;
  
   /* Take nearest integer. */
   ispw = Nearest_integer( value );

   /* Bias the spectrum width. */
   ispw += 2;

   return( ispw );

/* End of RPGCS_ms_to_spectrum_width() */
}

/******************************************************************

   Description:
      Performs data conversion.   Conversion information is 
      provided in data structure "data_block". 

   Inputs:
      data - pointer to data to be converted.
      data_block - Generic_moment_t pointer.   Data structure
                   contains conversion information.
      below_threshold - value to assign to below threshold data.
      range_folded - value to assign to range folded data.

   Returns:
      -1 on error, 0 if the original array of data does not
      require conversion, or 1 on success.

   Notes:
      The array pointed to by "*converted_data" must be freed 
      by caller.

******************************************************************/
int RPGCS_radar_data_conversion( void* data, Generic_moment_t *data_block,
                                 float below_threshold, float range_folded,
                                 float **converted_data ){

   int i;

   /* Check to make sure data_block is really a data block and data
      is a valid pointer. */
   if( (data == NULL ) || (data_block == NULL ) 
                       ||
       (data_block->name[0] != 'D') )
      return -1;

   /* Check to see if the data needs converting. */
   if( data_block->scale == 0.0 )
      return 0;

   /* Perform some validation. 
  
      Is the number of gates valid? */
   if( (data_block->no_of_gates <= 0)
                  ||
       (data_block->no_of_gates > (int) MAX_BASEDATA_REF_SIZE) )
      return -1;

   /* Is the data resolution something other than expected? */
   if( (data_block->data_word_size != BYTE_MOMENT_DATA) 
                       &&
       (data_block->data_word_size != SHORT_MOMENT_DATA) )
      return -1;

   /* So for, so good.   Allocate buffer to hold the converted data. */
   *converted_data = (float *) malloc( data_block->no_of_gates*sizeof(float) );
   if( *converted_data == NULL )
      return -1;

   /* Convert the data and store the converted data in "converted_data". */
   if( data_block->data_word_size == BYTE_MOMENT_DATA ){

      unsigned char *gate = (unsigned char *) data;

      for( i = 0; i < data_block->no_of_gates; i++ ){

         switch( (int) gate[i] ){

            case 0:
               (*converted_data)[i] = below_threshold;
               break;

            case 1: 
               (*converted_data)[i] = range_folded;
               break;

            default: 
               (*converted_data)[i] = 
                      ((float) gate[i] - data_block->offset)/data_block->scale;
               break;

         } /* End of "switch" statement. */

      } /* End of "for" loop. */

   }
   else if( data_block->data_word_size == SHORT_MOMENT_DATA ){

      unsigned short *gate = (unsigned short *) data;

      for( i = 0; i < data_block->no_of_gates; i++ ){

         switch( (int) gate[i] ){

            case 0:
               (*converted_data)[i] = below_threshold;
               break;

            case 1: 
               (*converted_data)[i] = range_folded;
               break;

            default: 
               (*converted_data)[i] = 
                      ((float) gate[i] - data_block->offset)/data_block->scale;
               break;

         } /* End of "switch" statement. */

      } /* End of "for" loop. */

   }

   return 1;

/* End of RPGCS_radar_data_conversion() */
}

/******************************************************************

   Description:
      Implements the nearest integer function.

   Inputs:
      value - value to be rounded to nearest integer.

   Returns:
      the nearest integer to "value"

******************************************************************/
static int Nearest_integer( float value ){

#ifdef LINUX

   float rounded = roundf( value );
   return( (int) rounded );

#else

   /* Take nearest integer. */
   if( value < 0.0 )
      return( (int) (value - 0.5) );

   else
      return( (int) (value + 0.5) );

#endif
   
/* End of Nearest_integer() */
}
