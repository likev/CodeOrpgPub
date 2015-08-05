/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2010/03/17 21:40:09 $
 * $Id: pbd_super_reso.c,v 1.1 2010/03/17 21:40:09 ccalvert Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

#include <pbd.h>

/* Static Global Variables. */
static Generic_moment_t *Refl = NULL;
static unsigned short Azimuth = 0;
static unsigned short Elevation = 0;


/*******************************************************************

   Description:
      For Super Resolution data, reflectivity data may be carried 
      with Contiguous Doppler W/Ambiguity waveforms.  This data is 
      needed to perform the recombination.  If the reflectivity data
      is available, save the data so that we may latter insert in 
      the RPG radial as additional moment.


   Inputs:
      rda_msg - Pointer to RDA Message 31.
      azi - azimuth angle in BAMS.
      elev - elevation angle in BAMS.

   Returns:
      Returns 0 on success, -1 otherwise.

   Notes:
      Assumes this function is called for VCP_WAVEFORM_CD waveforms.

*******************************************************************/
int SR_save_SuperRes_refl_data( char *rda_msg, unsigned short azi,
                                unsigned short elev ){

   Generic_moment_t *rfl2_block = NULL, *refl_block = NULL, *vel_block = NULL;
   int max_bins;

   refl_block = (Generic_moment_t *) ORPGGDR_get_data_block( rda_msg, ORPGGDR_DREF );
   if( refl_block == NULL )
      return 0;

   max_bins = refl_block->no_of_gates;

   vel_block = (Generic_moment_t *) ORPGGDR_get_data_block( rda_msg, ORPGGDR_DVEL );
   if( vel_block != NULL){

      /* Set the maximum number of bins if the number of reflectivity
         gates exceeds the number of Doppler gates.  We want these the 
         same. */  
      if( vel_block->no_of_gates < refl_block->no_of_gates )
         max_bins = vel_block->no_of_gates;

   }
   else
      return 0;

   /* Is the data in the expected resolution? */
   if( refl_block->data_word_size != BYTE_MOMENT_DATA ){

      LE_send_msg( GL_INFO, "%d-bit Moment Data, %d-bit Data Expected\n",
                   refl_block->data_word_size, BYTE_MOMENT_DATA );
      LE_send_msg( GL_INFO, " ... Skipping Moment\n" );
      return -1;

   }

   /* Allocate sufficient memory to store the data. */
   if( Refl != NULL ){

      free( Refl );
      Refl = NULL;

      Azimuth = 0;
      Elevation = 0;

   }

   Refl = (Generic_moment_t *) calloc( 1, sizeof(Generic_moment_t) + BASEDATA_DOP_SIZE );
   if( Refl == NULL ){

      LE_send_msg( GL_ERROR, "calloc Failed For %d Bytes\n",
                   sizeof( Generic_moment_t) + BASEDATA_DOP_SIZE );
      return -1;

   }

   /* Move the data block header ..... */
   refl_block->no_of_gates = max_bins;
   memcpy( Refl, refl_block, sizeof(Generic_moment_t) );

   /* Move the data ..... */
   memcpy( ((char *) Refl) + sizeof(Generic_moment_t), &refl_block->gate.b[0], 
           max_bins*sizeof(char) ); 

   /* Change the name of the data from "DREF" to "DRF2". */
   rfl2_block = (Generic_moment_t *) Refl;
   memcpy( &rfl2_block->name[0], "DRF2", 4 );   

   /* Save identifying information for this radial. */
   Azimuth = azi;
   Elevation = elev;

   return 0;

/* End of SR_save_SuperRes_refl_data() */
}

/*******************************************************************

   Description:
      Copies the reflectivity data to the RPG radial as an additional
      moment.

   Inputs:
      rpg_msg - pointer to RPG message.
      azm - azimuth angle in BAMS.
      elev - elevation angle in BAMS.

   Returns:
      0 on success and -1 on error.

*******************************************************************/
int SR_restore_SuperRes_refl_data( char *rpg_msg, unsigned short azm, 
                                   unsigned short elev ){

   Base_data_header *rpg_hd = (Base_data_header *) rpg_msg;
   unsigned char *data;
   int offset;
    
   /* Verify match on Azimuth and Elevation. */
   if( (azm != Azimuth) || (elev != Elevation) ){

      LE_send_msg( GL_INFO, 
          "Super Res Mismatch on Elev (%d != %d) or Azimuth (%d != %d)\n",
          elev, Elevation, azm, Azimuth );
      return -1;

   }

   /* Verify the reflectivity data is saved. */
   if( Refl == NULL ){

      LE_send_msg( GL_INFO, "Refl == NULL ... Skip saving Reflectivity\n" );
      return -1;

   }

   offset = rpg_hd->msg_len*sizeof(short);
   offset += (offset % sizeof(int));

   /* Set the offset in the RPG radial header. */
   rpg_hd->offsets[rpg_hd->no_moments] = offset;

   /* Move the data block header ..... */
   memcpy( rpg_msg + offset, Refl, sizeof(Generic_moment_t) );
   offset += sizeof(Generic_moment_t);

   /* Move the data ..... */
   data = (unsigned char *) (rpg_msg + offset);
   memcpy( data, &Refl->gate.b[0], Refl->no_of_gates ); 

   offset += Refl->no_of_gates*sizeof(char);

   /* Ensure the offset is defined on a word boundary. */
   offset += (offset % sizeof(int));

   /* Increase the size of RPG radial to accomodate the additional data. */
   rpg_hd->msg_len = offset/sizeof(short);

   /* Increment the number of additional moments. */
   rpg_hd->no_moments++;

   /* Done with reflectivity data.   Prepare for next radial. */ 
   free( Refl );
   Refl = NULL;

   Azimuth = Elevation = 0;

   return 0;

/* End of SR_restore_SuperRes_refl_data() */
}
