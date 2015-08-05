/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2004/01/06 20:32:31 $
 * $Id: prfbmapc.c,v 1.4 2004/01/06 20:32:31 ccalvert Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */

#include <prfbmap.h>
#include <orpg.h>

#ifdef SUNOS
   #define a3054a__build_bit_map a3054a__build_bit_map_
   #define a30546__end_of_product_processing a30546__end_of_product_processing_
   #define a30547__get_product_size a30547__get_product_size_
#endif

#ifdef LINUX
   #define a3054a__build_bit_map a3054a__build_bit_map__
   #define a30546__end_of_product_processing a30546__end_of_product_processing__
   #define a30547__get_product_size a30547__get_product_size__
#endif

#define BITS_PER_BYTE       8

/* Function Prototypes For Global Functions. */
void a3054a__build_bit_map( int *radcount, int *start_angle, int *delta_ang,
                            short overlaid[][MAX_RANGE], int *start_bin, 
                            int *end_bin, int *num_alwbprfs,
                            int *allowable_prfs, void *outbuf );

void a30546__end_of_product_processing( unsigned int *num_radials, 
                                        unsigned int *vol_time,
                                        unsigned int *num_prfs,
                                        unsigned int *prf_nums,
                                        unsigned int *vcp_num,
                                        unsigned int *vol_num,
                                        unsigned short *vol_date,
                                        unsigned short *elev_ang,
                                        void *outbuf  );
void a30547__get_product_size( unsigned int *bufsiz );


/* Function Prototypes For Local Functions. */
static void Get_bit( unsigned int bin, int *index, int *bit );


/***************************************************************************

   Description:
      This module checks the "overlaid" flag for each allowable prf and
      each range bin along the current radial.  If "overlaid" flag is set,
      the appropriate bit is set in the obscuration bit map output buffer.

   Inputs:
      radcount - current radial number in elevation cut.
      start_angle - starting azimuth angle of current radial.  In degrees
                    * 10.
      delta_ang - azimuth separation, in degrees * 10.
      overlaid - obscuration data for all allowable prfs out to 230 km range.
      start_bin - bin index of first valid data in radial. 
      start_bin - bin index of last valid data in radial. 
      num_alwbprfs - number of prfs to check. 
      allowable_prfs - the prfs to check.

   Outputs:
      outbuf - output buffer containing prf obscuration bit map.

   Returns:
      There are no return values defined for this function.

***************************************************************************/
void a3054a__build_bit_map( int *radcount, int *start_angle, int *delta_ang,
                            short overlaid[][MAX_RANGE], int *start_bin, 
                            int *end_bin, int *num_alwbprfs,
                            int *allowable_prfs, void *outbuf ){

   Prfbmap_prod_t *prf_prod;
   Prfbmap_t *bit_map;
   Radbmap_t *rad_bit_map;
   
   short prf;
   int   index, bit, i, j;

   /* Cast output buffer address to address of type Prfbmap_prod_t */
   prf_prod = (Prfbmap_prod_t *) outbuf;

   /* Do For All allowable PRFs */
   for( i = 0; i < *num_alwbprfs; i++ ){

      /* Extract an allowable PRF. */
      prf = allowable_prfs[i] - 1;
      bit_map = (Prfbmap_t *) &prf_prod->bmap[prf];

      /* Set the start azimuth angle (degrees * 10) and delta azimuth angle 
         (degrees * 10) for this radial. */
      rad_bit_map = &bit_map->prfbmap[*radcount - 1];
      rad_bit_map->azm = *start_angle;
      rad_bit_map->delta_azm = *delta_ang;

      /* Build the bit map for this radial and this PRF. */
      for( j = *start_bin - 1; j < *end_bin - 1; j++ ){

         /* If this this bin is overlaid, then set appropriate bit in 
            bit map. */
         if( overlaid[prf][j] ){

            /* Determine bit to set. */
            Get_bit( (unsigned int) j, &index, &bit ); 

            /* Set the bit. */
            rad_bit_map->radbmap[index] |= (1 << bit);

         }

      }

   }
  
/* End of a3054a__build_bit_map() */
}


/*********************************************************************

   Description:
      Performs end of product processing.  Fills in the product header
      with appropriate data.

   Inputs:
      num_radials - total number of radials in elevation cut.   
      vol_time - volume scan start time, in millisecs past midnight.
      num_prfs - the number of allowable prfs.
      prf_nums - the allowable prfs.
      vcp_num - the volume coverage pattern.
      vol_num - the volume scan sequence number.
      vol_date - the volume scan start date (modified Julian).
      elev_ang - the elevation angle of the product.

   Outputs:
      outbuf - the product output buffer which receives the input
               data.

   Returns:
      There is no return value defined for this function.

*********************************************************************/
void a30546__end_of_product_processing( unsigned int *num_radials, 
                                        unsigned int *vol_time,
                                        unsigned int *num_prfs,
                                        unsigned int *prf_nums,
                                        unsigned int *vcp_num,
                                        unsigned int *vol_num,
                                        unsigned short *vol_date,
                                        unsigned short *elev_ang,
                                        void *outbuf  ){

   Prfbmap_prod_t *prf_prod;
   unsigned int prf;
   int i;

   /* Cast output buffer address to address of type Prfbmap_prod_t */
   prf_prod = (Prfbmap_prod_t *) outbuf;

   /* Set product header items. */
   prf_prod->hdr.num_radials = *num_radials;
   prf_prod->hdr.time = *vol_time;
   prf_prod->hdr.num_prfs = (unsigned short) *num_prfs;

   for( i = 0; i < *num_prfs; i++ ){

      prf = prf_nums[i] - 1;
      prf_prod->hdr.prf_nums |= ( 1 << prf );

   }

   prf_prod->hdr.vcp_num = (unsigned short) *vcp_num;
   prf_prod->hdr.vol_num = (unsigned short) *vol_num;
   prf_prod->hdr.date = *vol_date;
   prf_prod->hdr.elev_angle = *elev_ang;
   
/* End of a30546__end_of_product_processing() */ 
}

/*********************************************************************

   Description:
      Returns the product size, in number of 32-bit words.

   Inputs:

   Outputs:
      bufsiz - location to recieve product size.

   Returns:
      There is no return value define for this function.

*********************************************************************/
void a30547__get_product_size( unsigned int *bufsiz ){


   *bufsiz = ( sizeof( Prfbmap_prod_t ) + 3 ) / sizeof( int );

/* End of a30547__get_product_size() */
}


/* Local Functions. */

/*********************************************************************

   Description:
      Determines the bit map index and bit number given the input bin
      number.

   Inputs:
      bin - bin number.

   Outputs:
      index - receives the bit map index corresponding to bin.
      bit - receives the bit map bit number at index corresponding to
            bin.

   Returns:
      There is no return value define for this function.

   Notes:
      If the index value is greater than MAX_BYTES or less than 0,
      the process terminates.

*********************************************************************/
static void Get_bit( unsigned int bin, int *index, int *bit ){

   /* Determine index into bit map, and bit number at index. */
   *index = bin / BITS_PER_BYTE; 
   *bit = bin % BITS_PER_BYTE;

   /* Validate the bit map index.  Terminate process if invalid. */
   if( *index >= MAX_BYTES || *index < 0 ){

      LE_send_msg( GL_ERROR, "Invalid Bit Map Index %d For Bin %d\n", 
                   *index, bin );
      ORPGTASK_exit( GL_ERROR );

   }

/* End of Get_bit() */
}
