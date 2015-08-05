/*
  * RCS info
  * $Author: steves $
  * $Locker:  $
  * $Date: 2014/10/03 17:39:45 $
  * $Id: orpgrda_adpt.c,v 1.13 2014/10/03 17:39:45 steves Exp $
  * $Revision: 1.13 $
  * $State: Exp *
  */

/************************************************************************
 *									*
 *	Module: orpgrda_adpt.c						*
 *									*
 *	Description:  This module contains a collection of functions	*
 *	dealing with RDA adaptation data.				*
 *									*
 ************************************************************************/
/* Local and global include file definitions. */
#include <orpg.h>

/* Global variables. */

/* Buffer containing rda adaptation data block. */
static ORDA_adpt_data_t *ORDA_adpt = NULL;	
static ORDA_adpt_data_msg_t *ORDA_adpt_msg = NULL;

static int ORDA_adpt_updated = 0;    /* 0 = ORDA adapt LB needs to be read.
                                        1 = ORDA adapt LB does not need to
                                            be read. */

static int ORDA_adpt_registered = 0; /* 0 = ORDA adapt LB notify not registered.
                                        1 = ORD adapt LB notify registered. */

static void ORPGRDA_ADPT_lb_notify( int fd, LB_id_t msg_id, int msg_info, 
                                    void *arg );

/**********************************************************************
   Description:
     This function sets the ORDA adaptation data LB init flag whenever
     the ORDA adaptation data LB is updated.

   Input:  NONE	

   Output: 
      fd - file descriptor of LB that was updated
      msg_id - message ID that was updated.
      msg_info - length of arg data
      *arg - (unused)

   Return: NONE	

************************************************************************/
void ORPGRDA_ADPT_lb_notify( int fd, LB_id_t msg_id, int msg_info,
                             void *arg ){

   ORDA_adpt_updated = 0;

/* End of ORPGRDA_ADPT_lb_notify() */
}

/************************************************************************
   Description: 
      This function reads the RDA adaptation data block.	

   Return: 
      Pointer to adaptation data structure on success or NULL on error.

************************************************************************/
void* ORPGRDA_ADPT_read( ){

   int status;

   /* First check to see if ORDA adaptation data LB notification 
      registered. */

   if (!ORDA_adpt_registered) {

      status = ORPGDA_UN_register( ORPGDAT_RDA_ADAPT_DATA,
				   ORPGDAT_RDA_ADAPT_MSG_ID, ORPGRDA_ADPT_lb_notify);

      if( status != LB_SUCCESS ){

         LE_send_msg( GL_ERROR,
            "ORPGRDA_ADPT: UN_register(ORPGDAT_RDA_ADAPT_DATA) failed (ret %d)", 
            status);
         return( (void *) NULL );

      }

      ORDA_adpt_registered = 1;

   }

   /* If adaptation data previously read, free buffer now. */
   if( ORDA_adpt_msg != NULL ){

      free( ORDA_adpt_msg );
      ORDA_adpt_msg = NULL;

   }

   /* Read the ORDA adaptation data. */
   status = ORPGDA_read( ORPGDAT_RDA_ADAPT_DATA, (char *) &ORDA_adpt_msg,
			 LB_ALLOC_BUF, ORPGDAT_RDA_ADAPT_MSG_ID);

   if( (status < 0) || (status != ALIGNED_SIZE(sizeof(ORDA_adpt_data_msg_t))) ) {

      LE_send_msg( GL_INFO, "ORPGRDA: ORPGDAT_RDA_ADAPT_DATA read failed: %d\n",
                   status );

      return( (void *) ORDA_adpt );

   }

   /* One time RDA adaptation data buffer allocation. */
   if( ORDA_adpt == NULL ){

      ORDA_adpt = malloc( ALIGNED_SIZE(sizeof(ORDA_adpt_data_t) ) );
      if( ORDA_adpt == NULL ){

         LE_send_msg( GL_ERROR, "malloc Failed for %d Bytes.\n",
                      ALIGNED_SIZE( sizeof(ORDA_adpt_data_t) ) );
         return( (void *) ORDA_adpt );

      }

   }

   memcpy( ORDA_adpt, &ORDA_adpt_msg->rda_adapt, sizeof(ORDA_adpt_data_t) );

   ORDA_adpt_updated = 1;

   return( (void *) ORDA_adpt ); 

/* End of ORPGRDA_ADPT_read() */
}

/************************************************************************
   Description: 
      This function returns a pointer to a ORDA_adapt_data_t structure.
 
   Return: 
      Returns pointer to ORDA_adapt_data_t on success or NULL on error.

************************************************************************/
void* ORPGRDA_ADPT_get_data( ){

   /* If the data has changed in the file then we need to re-read it. */
   if(!ORDA_adpt_updated) 
      return( ORPGRDA_ADPT_read () );


   return( (void *) ORDA_adpt );
}

/************************************************************************
   Description: 
      This function returns the value specified by "item".
 
   Inputs:
      item - Macro definition for data item (see orpgrda_adpt.h)

   Outputs:
      value - contains data value.

   Return: 
      Returns 0 on success, -1 on failure.

************************************************************************/
int ORPGRDA_ADPT_get_data_value( int item, void *value ){

   /* If the data has changed in the file then we need to re-read it. */
   if(!ORDA_adpt_updated)
      ORPGRDA_ADPT_read ();

   if( ORDA_adpt == NULL )
      return(-1);
   
   switch( item ){

      case ORPGRDA_ADPT_DELTA_PRI:
      {

         int *deltaprf = (int *) value;

         /* Validate the value. */
         if( (ORDA_adpt->deltaprf < 0) || (ORDA_adpt->deltaprf > DELPRI_MAX) ){

            LE_send_msg( GL_ERROR, "Bad Delta PRI Value from RDA Adaptation Data\n" );
            return(-1);

         }
          
         *deltaprf = (int) ORDA_adpt->deltaprf;

         return(0);

      }

      case ORPGRDA_ADPT_NBR_EL_SEGMENTS:
      {

         int *nbr_el_segments = (int *) value;

         *nbr_el_segments = (float) ORDA_adpt->nbr_el_segments;
         return(0);

      }

      case ORPGRDA_ADPT_SEG1LIM:
      {

         float *seg1lim = (float *) value;

         *seg1lim = ORDA_adpt->seg1lim;
         return(0);

      }

      case ORPGRDA_ADPT_SEG2LIM:
      {

         float *seg2lim = (float *) value;

         *seg2lim = ORDA_adpt->seg2lim;
         return(0);

      }

      case ORPGRDA_ADPT_SEG3LIM:
      {

         float *seg3lim = (float *) value;

         *seg3lim = ORDA_adpt->seg3lim;
         return(0);

      }

      case ORPGRDA_ADPT_SEG4LIM:
      {

         float *seg4lim = (float *) value;

         *seg4lim = ORDA_adpt->seg4lim;
         return(0);

      }

      case ORPGRDA_ADPT_VELTOVER:
      {

         float *veltover = (float *) value;

         *veltover = ORDA_adpt->vel_data_tover;
         return(0);

      }

      case ORPGRDA_ADPT_SPWTOVER:
      {

         float *spwtover = (float *) value;

         *spwtover = ORDA_adpt->width_data_tover;
         return(0);

      }

      case ORPGRDA_ADPT_REFTOVER:
      {

         float *reftover = (float *) value;

         *reftover = ORDA_adpt->refl_data_tover;
         return(0);

      }
      default:
         return(-1);

   }

/* End of ORPGRDA_ADPT_get_value() */
}
