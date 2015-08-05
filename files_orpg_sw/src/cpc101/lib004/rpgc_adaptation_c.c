/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2006/09/15 21:58:49 $
 * $Id: rpgc_adaptation_c.c,v 1.3 2006/09/15 21:58:49 steves Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

/********************************************************************

	This module contains adaptation supporting functions for the 
	old RPG product generaton tasks.

********************************************************************/



#define RPG_ADAPTATION

#include <rpgc.h>
#include <orpgadpt.h>
#include <siteadp.h>
#include <prodsel.h>
#include <rdacnt.h>
#include <product_parameters.h>


#define NAME_LEN	128	/* max LB name length */
#define MAXN_ADAPT	32	/* max number of adaptation blocks */

typedef struct {

   int data_store_id;		/* Data store ID where subblock
                                   resides */
   char *block_name;		/* Object name */

   unsigned int block_offset;	/* byte offset of subbblock within
                                   block */
   unsigned int length;		/* Size of this structure */

} Legacy_adapt_t;

typedef struct {		/* adaptation block registration */

    LB_id_t id;			/* LB message ID. */
    char *pt;			/* receiving buffer for data. */
    int timing;			/* update timing. */
    en_t event_id;		/* event id if timing is ON_EVENT */
    int changed;		/* TRUE/FALSE recording the change for 
				   scheduled future update */
    Legacy_adapt_t *block;	/* Block address */
    int num_blocks;		/* Number of subblocks in block */

} Adapt_block;

static int N_adapt = 0;		/* number of adapt blocks registered */
static Adapt_block Adapt [MAXN_ADAPT];
				/* adapt blocks registered */

static int Events_registered = 0; /* flag indicating adaptation data 
             			     updates are tied to events */


/* These are the known blocks for which an algorithm can register.
   This is intended only to support legacy.  For new algorithms,
   it is assumed that librpgc will be used, and the adapation
   data registration interface is different. */
#define RDACNT_BLOCKS         1
static Legacy_adapt_t rdacnt_block[ RDACNT_BLOCKS ] = {

                      { ORPGDAT_ADAPTATION		/* data store id */,
                        "RDA Control" 			/* block name */,
                        sizeof( int )			/* offset */,
			0 }				/* length is not used */
		      };

#define COLRTBL_BLOCKS        1
static Legacy_adapt_t colrtbl_block[COLRTBL_BLOCKS] = {

                      { ORPGDAT_ADAPTATION		/* data store id */,
                        "Color Tables"			/* block name */,
                        0				/* offset */,
			0 				/* length */
		      }
		      };

/* local functions */
static void Init_read ();
static void Read_block ( int ind );
static void Report_adaptation( int ind );
static int Read_subblock( Legacy_adapt_t *block, char *start_addr,
                          int id );
void Adapt_lb_notify_callback( int fd, LB_id_t msgid, int msg_info,
                               void *arg );
void Register_for_updates( int ind, int subblock_ind );

/********************************************************************

    Description:
       This function adds a new adaptation block to the registered
       adaptation block list.

    Inputs:
       id - adaptation data block id.
       buf - adaptation data block pointer.
       timing - update timing (ADPT_UPDATE_BOE, ADPT_UPDATE_BOV,
		ADPT_UPDATE_ON_CHANGE, ADPT_UPDT_WITH_CALL, or
                ADPT_UPODATE_ON_EVENT).
       event_id - event id if timing is ADPT_UPDTAE_ON_EVENT (optional)

    Return:	
       The return value is never used. On failure this function 
       calls PS_task_abort to terminate the task.

    Notes:	

********************************************************************/
int RPGC_reg_adpt( int id, char *buf, int timing, ...){

    int status;
    va_list ap;

    /* Check if adaptation block alread registered. */
    if( RPGC_is_adapt_block_registered( id, &status ) >= 0 )
       return (0);

    if( N_adapt >= MAXN_ADAPT ){	/* This should never happen */

	N_adapt++;			/* This will cause later abort */
	return (0);

    }

    Adapt [N_adapt].id = id;
    Adapt [N_adapt].pt = buf;

    if( (timing != ADPT_UPDATE_BOE)
                && 
        (timing != ADPT_UPDATE_BOV)
                &&
        (timing != ADPT_UPDATE_ON_CHANGE)
                && 
        (timing != ADPT_UPDATE_WITH_CALL)
                && 
        (timing != ADPT_UPDATE_ON_EVENT) )
       PS_task_abort( "Invalid Timing In Adaptation Data Registration (%d)\n",
                      timing );
    Adapt [N_adapt].timing = timing;
    
    if( timing == ADPT_UPDATE_ON_EVENT ){

       va_start(ap, timing);		/* Start the variable argument
					   list */
       Adapt [N_adapt].event_id =
          (en_t) *(va_arg(ap, int *));	/* Get the event_id argument */ 

       va_end(ap);			/* End the variable argument list */

       Events_registered = 1;		/* Set flag indicating this adaptation
					   data update is tied to an event */

    }
    else
       Adapt [N_adapt].event_id = -1;

    Adapt [N_adapt].changed = FALSE;

    /* Tell user about registered adaptation data. */
    Report_adaptation( N_adapt );
    N_adapt++;

    return (0);

/* End of RPGC_reg_adpt() */
}

/*************************************************************************
   Description:
      This function checks if adaptation data block "block_id"
      has been registered.

   Inputs:
      block_id - pointer to int containing block ID
      status - pointer to int to receive status of operation.  
               This is used to provide a FORTRAN interface.

   Outputs:
      status - receives the status of the operation.  If
               already registered, status is set to 1; otherwise
               status is set to 0.

   Returns:
      Adaptation data index if block already registered,
      or -1 if it is not.

   Notes:

*************************************************************************/
int RPGC_is_adapt_block_registered( int block_id, int *status ){

   int ind;

   /* Initialize status to not registered. */
   *status = 0;

   /* Check all previously registered adaptation blocks. */
   for( ind = 0; ind < N_adapt; ind++ ){

      /* Check for match on block IDs. */
      if( block_id == Adapt[ind].id ){

         /* Block is registered. */
         *status = 1;
         return (ind);

      }

   /* End of "for" loop. */
   }

   /* Block is not registered. */
   return (-1);

/* End of RPGC_is_adapt_block_registered() */
}

/*******************************************************************

   Description:
      Read the adaptation data block specified by "block_id".

   Inputs:
      block_id - pointer to int containing block ID.
      status - pointer to int to receive status of operation.

   Outputs:
      status - contains status of operation:  0 if unsuccessful 
               or 1 if successful.

   Returns:
     Returns 0 on failure (block not registered) or 1 on 
     successful read.

   Notes:

********************************************************************/
int RPGC_read_adapt_block( int block_id, int *status ){

   int success = 0;
   int ind;

   /* Initialize the status to unsuccessful. */
   *status = 0;

   /* Is the block already registered? */
   if( (ind = RPGC_is_adapt_block_registered( block_id, &success )) < 0 )
      return 0;

   /* Read the adaptation data block. */
   Read_block( ind );   

   /* Normal return. */
   *status = 1;
   return (1);

/* End of RPGC_read_adapt_block() */
}

/********************************************************************

    Description: 
       This function updates the registered adaptation blocks by 
       reading adaptation data.  Only adaptation data which has 
       been updated since previous read is re-read.

    Return:	
       There is no return value for this function. (See Notes)

    Notes:	

********************************************************************/
void RPGC_update_adaptation (){

   int ind;

   /* Do for all registered blocks. */
   for( ind = 0; ind < N_adapt; ind++ ){

      /* If adaptation data message indicates no change, continue */
      if( Adapt[ind].changed == FALSE )
         continue;

      /* Read adaptation data block. */
      Adapt[ind].changed = FALSE;
      Read_block( ind );

   /* End of "for" loop. */
   }

   return;

/* End of RPGC_update_adaptation() */
}

/********************************************************************

   Description: 
      This function opens the adaptation LB(s) and initially reads in 
      all registered adaptation blocks.

   Inputs:

   Outputs:

   Returns:

   Notes:

********************************************************************/
void ADP_initialize(){

   if (N_adapt > MAXN_ADAPT)		
      PS_task_abort ("Too Many Adaptation Blocks Specified\n");

   /* Do an initial read. */
   Init_read ();

/* End of ADP_initialize() */
}

/***********************************************************************

    Description: 
       All required adaptation data blocks are initially read.

    Inputs:

    Outputs:

    Returns:

    Notes:
		
***********************************************************************/
static void Init_read (){

    int ind, i;
 
    /* Do For All adaptation data blocks. */
    for (ind = 0; ind < N_adapt; ind++){

       /* Do For All adaptation data subblocks. */
       for( i = 0; i < Adapt[ind].num_blocks; i++ ){

          /* Register for LB updates. */
          Register_for_updates( ind, i );

       /* End of "for" loop. */
       }

       /* Do an initial read of message. */
       Read_block( ind );
       Adapt[ind].changed = FALSE;

    /* End of "for" loop. */
    }

    return;

/* End of Init_read() */
}

/********************************************************************

   Description: 
      This function updates the registered adaptation blocks by 
      reading adaptation data from the adaptation LB file.

   Inputs:
      new_vol - flag indicating whether a new volume scan has
                occurred.

   Outputs:

   Returns:

   Notes:

********************************************************************/
void ADP_update_adaptation (int new_vol){

   int ind;

   /* Do for all registered blocks. */
   for (ind = 0; ind < N_adapt; ind++) {

      /* If adaptation data block indicates no change, continue */
      if( Adapt[ind].changed == FALSE )
         continue;

      /* If not new volume scan and update timing is beginning of
         volume, continue.  A read of the data occurs when the new 
         volume scan occurs. */
      if( !new_vol && Adapt[ind].timing == ADPT_UPDATE_BOV )
         continue;

      /* Adaptation data has changed so read block.  Reset "changed"
         flag before reading. */
      Adapt[ind].changed = FALSE;
      Read_block( ind );

    /* End of "for" loop. */
    }

    return;

/* End of ADP_update_adaptation() */
}


/********************************************************************

    Description: 
       This function updates the registered adaptation blocks by 
       reading adaptation data from the adaptation LB file.  Updating 
       is triggered by an event.

    Input:	
       event_id - event_id which triggers adaptation data update
 
    Return:	
       There is no return value for this function. 

    Notes:	
       We don't check ORPGDA_stat return value because any problem will
       be detected by Read_block.

********************************************************************/
void ADP_update_adaptation_by_event ( en_t event_id ){

   int ind;

   if( !Events_registered ){

      /* If no events registered, no need to go any further */
      return;

   }

   /* Do for all registered blocks. */
   for( ind = 0; ind < N_adapt; ind++ ){

      /* If adaptation data message not associated with this event,
         continue */
      if(Adapt[ind].event_id != event_id)
         continue;

      /* If adaptation data message indicates no change, continue */
      if( Adapt[ind].changed == FALSE )
         continue;

      /* Read adaptation data block */
      Adapt[ind].changed = FALSE;
      Read_block( ind );

   /* End of "for" loop. */
   }

   return;

/* End of ADP_update_adaptation_by_event() */
}

/***************************************************************

   Description:
      Reads adaptation data block whose registered index is "ind".

   Inputs:
      ind - index into Adapt for this block.

   Outputs:

   Returns:
      There is no return value defined for this function.

   Notes:
      The task terminates if the read fails.

***************************************************************/
void Read_block( int ind ){

   int error = 0;
   int i;

   /* Do For All adaptation data subblocks. */
   for( i = 0; i < Adapt[ind].num_blocks; i++ ){

      /* Read the subblock. */
      if( (error = Read_subblock( &Adapt[ind].block[i], 
                   (char *) Adapt[ind].pt, Adapt[ind].id ) ) != 0 )
         break;

   /* End of "for" loop. */
   }

   if ( error )
      PS_task_abort( "Reading Adaptation Block (%d) Failed\n", 
                     Adapt[ind].id );

/* End of Read_block() */
}

/*******************************************************************

   Description:
      This is used to read adaptation data.

   Inputs:
      block - pointer to structure defining adaptation data block.
      start_addr - pointer to receiving buffer where read data is
                   transferred.
      id - message id

   Outputs:

   Returns:
      Returns -1 on error, or 0 otherwise.

   Notes:

*******************************************************************/
static int Read_subblock( Legacy_adapt_t *block, char *start_addr,
                          int id ){

   int ret;

   /* If data store id == ORPGDAT_ADAPTATION, use ORPGDA_read */
   if( block->data_store_id == ORPGDAT_ADAPTATION ){

      ret = ORPGDA_read( ORPGDAT_ADAPTATION, start_addr, 1000000, id );
      if( ret < 0 ){

         LE_send_msg( GL_INFO, "ORPGDA_read of ORPGDAT_ADAPTATION Failed (%d)\n", ret );
         return (-1);

      }

      return (0);

   }
   else{

      LE_send_msg( GL_INFO, "Unknown/Unsupported Data Store ID:\n",
                      block->data_store_id );
      return (-1);

   }

   /* Normal Return. */
   return (0);

}

/****************************************************************

   Description:
      Reports through log error services about registered 
      adaptation blocks.

   Inputs:
      ind - index into the regristration table.

   Outputs:

   Returns:

   Notes:

****************************************************************/
static void Report_adaptation( int ind ){

   switch( Adapt [ind].id ){

       case RDACNT:
          LE_send_msg( GL_INFO, "Registered For RDACNT Adaptation Data\n" );
          Adapt[ind].block = rdacnt_block;
          Adapt[ind].num_blocks = RDACNT_BLOCKS;
          break;

       case COLRTBL:
          LE_send_msg( GL_INFO, "Registered For COLRTBL Adaptation Data\n" );
          Adapt[ind].block = colrtbl_block;
          Adapt[ind].num_blocks = COLRTBL_BLOCKS;
          break;

       default:
          PS_task_abort( "Registered For Unknown Adaptation Data (%d)\n",
                       Adapt[ind].id );
          break;

   /* End of "switch" */
   } 

/* End of Report_adaptation() */
}

/**********************************************************************

   Description:
      Callback function used for LB notification.

   Inputs:
      fd - adaptation data file LB fd.
      msgid - message id withing LB which was updated.
      msg_info - length of the message (not used).
      arg - pointer to Adapt_block structure for the adaptation block
            which was updated.

   Outputs:
      None.

   Notes:

**********************************************************************/
void Adapt_lb_notify_callback( int fd, LB_id_t msgid, int msg_info, 
                               void *arg ){

   /* Set the changed flag for this adaptation data block. */
   Adapt_block *ptr = (Adapt_block *) arg;
   ptr->changed = TRUE;

/* End of Adapt_lb_notify_callback() */
}

/***************************************************************

   Description:
      Registers the adaptation block specified by "ind" for
      changes.

   Input:
      ind - "Adapt" index.
      subblock_ind - subblock index.

   Outputs:

   Returns:

   Notes:

***************************************************************/
void Register_for_updates( int ind, int subblock_ind ){

   int ret;

   /* If data store is ORPGDAT_ADAPTATION, use LB notification. */
   if( Adapt[ind].block[subblock_ind].data_store_id == ORPGDAT_ADAPTATION ){

      /* Open with write permission so we don't lose UN Registration. */
      ORPGDA_write_permission( ORPGDAT_ADAPTATION );

      LB_NTF_control( LB_NTF_PUSH_ARG, (void *) &Adapt[ind] );
      if( (ret = ORPGDA_UN_register( ORPGDAT_ADAPTATION, Adapt[ind].id, 
                                     Adapt_lb_notify_callback )) < 0 )
         PS_task_abort( 
               "LB Notification Registration Failed For Adapt File %d (%d)",
               Adapt[ind].block[subblock_ind].data_store_id, ret );
      else
         LE_send_msg( GL_INFO, "Adaptation Data Block %s Registered for Updates\n",
                      Adapt[ind].block[subblock_ind].block_name );

   }
   else
      PS_task_abort( "Unsupported Adaptation Data Data Store ID: %d\n",
                     Adapt[ind].block[subblock_ind].data_store_id );

/* End of Register_for_updates() */
}
