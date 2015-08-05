/*
  * RCS info
  * $Author: steves $
  * $Locker:  $
  * $Date: 2014/07/28 14:56:48 $
  * $Id: orpgsails.c,v 1.3 2014/07/28 14:56:48 steves Exp $
  * $Revision: 1.3 $
  * $State: Exp *
  */

/************************************************************************
 *									*
 *	Module: orpgsails.c						*
 *									*
 *	Description:  This module contains a collection of functions	*
 *	dealing with SAILS state and adaptation data.			*
 *									*
 ************************************************************************/

/*	Local include file definitions.					*/

#include <orpgsails.h>

/*	Global variables.						*/
static int SAILS_status_updated  = 0;	/* 0 = LB needs to be read.
				    	   1 = LB does not need to be read. */
static int SAILS_status_registered = 0; /* 0 = LB notify not registered.
					   1 = LB notify registered. */
static int SAILS_request_updated  = 0;	/* 0 = LB needs to be read.
				    	   1 = LB does not need to be read. */
static int SAILS_request_registered = 0; /* 0 = LB notify not registered.
					   1 = LB notify registered. */
static int SAILS_io_status  = 0;	/* Last read/write status for 
                                           SAILS status */
static int SAILS_io_request  = 0;	/* Last read/write status for 
                                           SAILS request */
static SAILS_status_t SAILS_status;	/* Holds the latest SAILS status. */
static SAILS_request_t SAILS_request;	/* Holds the latest SAILS request. */

#define ORPGSAILS_SITE_MAX_DEFAULT 	1
static int Site_max_sails_cuts = -1;     /* Site max allowable SAILS cuts. */

#define SAILS_MAX_SAILS_CUTS		3


/* LB notificaiton callback functions. */
void ORPGSAILS_status_notify( int fd, LB_id_t msg_id, int msg_info, void *arg );
void ORPGSAILS_request_notify( int fd, LB_id_t msg_id, int msg_info, void *arg );

/************************************************************************

   Description: 
      This function sets the SAILS status data LB init flag whenever 
      the SAILS status data is updated.
						
   Input:  
      NONE						

   Output: 
      fd - file descriptor of LB that was updated
      msg_id   - message ID that was updated.	
      msg_info - length of arg data
      *arg     - (unused)

   Return: 
      NONE

***********************************************************************/
void ORPGSAILS_status_notify( int fd, LB_id_t msg_id, int msg_info, 
                                 void *arg ){

   SAILS_status_updated = 0;

/* End of ORPGSAILS_status_notify() */
}

/************************************************************************

   Description: 
      This function sets the SAILS request data LB init flag whenever 
      the SAILS request data is updated.
						
   Input:  
      NONE						

   Output: 
      fd - file descriptor of LB that was updated
      msg_id   - message ID that was updated.	
      msg_info - length of arg data
      *arg     - (unused)

   Return: 
      NONE

***********************************************************************/
void ORPGSAILS_request_notify( int fd, LB_id_t msg_id, int msg_info, 
                               void *arg ){

   SAILS_request_updated = 0;

/* End of ORPGSAILS_request_notify() */
}

/************************************************************************

   Description: 
      This function reads the SAILS status data block.	

   Output: 
      NONE

   Return: 
      Read status (error if negative)

************************************************************************/
int ORPGSAILS_read_status(){

   int status;
   double value = 0.0;
   char *ds_name = NULL;

   /* First check to see if SAILS status LB notification registered. */
   if (!SAILS_status_registered) {

      if( ((status = ORPGDA_write_permission( ORPGDAT_GSM_DATA )) < 0 )
                               ||
          ((status = ORPGDA_UN_register( ORPGDAT_GSM_DATA, SAILS_STATUS_ID,
                                        ORPGSAILS_status_notify )) < 0 ) ){

         LE_send_msg( GL_ERROR, "ORPGDA_UN_register(ORPGDAT_GSM_DATA) Failed (%d)",
                      status );
         return (status);

         /* Set the adaptation data LB name in the DEAU library. */
         ds_name = ORPGDA_lbname( ORPGDAT_ADAPT_DATA );
         if( ds_name != NULL )
            DEAU_LB_name( ds_name );

      }

      /* Set the flag to indicated LB notification is registered. */
      SAILS_status_registered = 1;

   }

   /* Read the SAILS Status data LB.	*/
   SAILS_status_updated = 1;
   SAILS_io_status = ORPGDA_read( ORPGDAT_GSM_DATA,
		                  (char *) &SAILS_status,
		                  (int) sizeof(SAILS_status_t), 
                                  SAILS_STATUS_ID );
			
   if( SAILS_io_status < 0 ){

      SAILS_status_updated = 0;
      LE_send_msg( GL_INFO, "ORPGDAT_GSM_DATA read failed: %d\n",
                   SAILS_io_status );

   }

   /* Get the site maximum number of SAILS cuts in any VCP. */
   Site_max_sails_cuts = DEAU_get_values( "pbd.n_sails_cuts", &value, 1 );
   if( Site_max_sails_cuts < 0 ){

      LE_send_msg( GL_INFO, "DEAU_get_values( \"pbd.n_sails_cuts\" ) Failed: %d\n",
                   Site_max_sails_cuts );
      Site_max_sails_cuts = ORPGSAILS_SITE_MAX_DEFAULT;

   }
   else
      Site_max_sails_cuts = (int) value;
   
   return SAILS_io_status; 

/* End of ORPGSAILS_read_status() */
}

/************************************************************************

   Description: 
      This function reads the SAILS request data block.  

   Output: 
      NONE

   Return: 
      Read status (error if negative)

************************************************************************/
int ORPGSAILS_read_request(){

   int status;

   /* First check to see if SAILS Request LB notification registered. */
   if (!SAILS_request_registered) {

      if( ((status = ORPGDA_write_permission( ORPGDAT_GSM_DATA )) < 0 )
                               ||
          ((status = ORPGDA_UN_register( ORPGDAT_GSM_DATA, SAILS_REQUEST_ID,
                                        ORPGSAILS_request_notify )) < 0 ) ){

         LE_send_msg( GL_ERROR, "ORPGDA_UN_register(ORPGDAT_GSM_DATA) Failed (%d)",
                      status );
         return (status);

      }

      /* Set flag to indicate LB notification has been registered. */
      SAILS_request_registered = 1;

   }

   /* Read the SAILS Request data LB.    */
   SAILS_request_updated = 1;
   SAILS_io_request = ORPGDA_read( ORPGDAT_GSM_DATA,
                                   (char *) &SAILS_request,
                                   (int) sizeof(SAILS_request_t),
                                   SAILS_REQUEST_ID );

   if( SAILS_io_request < 0 ){

      SAILS_request_updated = 0;
      LE_send_msg( GL_INFO, "ORPGDAT_GSM_DATA read failed: %d\n",
                   SAILS_io_request );

   }

   return SAILS_io_request;

/* End of ORPGSAILS_read_request() */
}

/************************************************************************
 
   Description: 
      This function writes the SAILS status data.

   Output: 
      NONE

   Return: 
      Write status (error if negative)	

************************************************************************/
int ORPGSAILS_write_status(){

   SAILS_io_status = ORPGDA_write( ORPGDAT_GSM_DATA, (char *) &SAILS_status,
	                           sizeof(SAILS_status_t), SAILS_STATUS_ID );

   if( SAILS_io_status <= 0 )
      LE_send_msg( GL_INFO, "ORPGDAT_GSM_DATA write failed: %d\n",
		   SAILS_io_status );

   return SAILS_io_status;

/* End of ORPGSAILS_write_status() */
}


/************************************************************************
 
   Description: 
      This function writes the SAILS request data.

   Output: 
      NONE

   Return: 
      Write status (error if negative)	

************************************************************************/
int ORPGSAILS_write_request(){

   SAILS_io_request = ORPGDA_write( ORPGDAT_GSM_DATA, (char *) &SAILS_request,
	                            sizeof(SAILS_request_t), SAILS_REQUEST_ID );

   if( SAILS_io_request <= 0 )
      LE_send_msg( GL_INFO, "ORPGDAT_GSM_DATA write failed: %d\n",
		   SAILS_io_request );

   return SAILS_io_request;

/* End of ORPGSAILS_write_request() */
}

/************************************************************************
   Description: 
      This function returns the current SAILS status (GS_SAILS_DISABLED, 
      GS_SAILS_ACTIVE, GS_SAILS_INACTIVE) or error. 
 
   Input: 
      NONE

   Output: 
      NONE

   Return: 
      SAILS status or -1 on error

************************************************************************/
int ORPGSAILS_get_status(){

   int allowed = 0;
   unsigned char sails = 0;
   int n_cuts = 0;

   /* Is SAILS enabled? */
   sails = ORPGINFO_is_sails_enabled();

   /* Is SAILS allowed? */
   if( (allowed = ORPGSAILS_allowed()) < 0 )
      return -1;
   
   /* Return the SAILS status. */

   /* If SAILS is disabled, return GS_SAILS_DISABLED. */
   if( sails == 0 )
      return GS_SAILS_DISABLED;

   /* If SAILS is not allowed for the current VCP, return
      GS_SAILS_INACTIVE. */
   if( allowed == 0 )
      return GS_SAILS_INACTIVE;   
      
   /* If SAILS is enabled but the number of SAILS cuts is 
      0 return GS_SAILS_INACTIVE. */
   n_cuts = ORPGSAILS_get_num_cuts();
   if( (sails) && (n_cuts == 0) )
      return GS_SAILS_INACTIVE;

   /* If here, SAILS is enabled and allowed for current VCP.
      Return GS_SAILS_ACTIVE. */
   return GS_SAILS_ACTIVE;

/* End of ORPGSAILS_get_status() */
}

/************************************************************************
   Description: 
      This function returns the number of SAILS cuts currently being
      used.
 
   Input:
      NONE

   Output: 
      NONE

   Return: 
      Number of SAILS cuts currently defined or -1 on error. 

************************************************************************/
int ORPGSAILS_get_num_cuts(){

   int retval = 0;

   /* If the data has changed in the file then we need to re-read it. */ 
   if( !SAILS_status_updated )
       retval = ORPGSAILS_read_status();
           
   /* On error, return error. */
   if( retval < 0 ){

      LE_send_msg( GL_INFO, "ORPGSAILS_read_status() Failed (%d)\n", retval );
      return -1;

   }
   
   /* Return the SAILS status. */
   return( SAILS_status.n_sails_cuts );

/* End of ORPGSAILS_get_num_cuts() */
}


/************************************************************************
   Description: 
      This function returns the number of SAILS cuts being requested.
 
   Input:
      NONE

   Output: 
      NONE

   Return: 
      Number of SAILS cuts being requested or -1 on error. 

************************************************************************/
int ORPGSAILS_get_req_num_cuts(){

   int retval = 0;

   /* If the data has changed in the file then we need to re-read it. */
   if( !SAILS_request_updated )
       retval = ORPGSAILS_read_request();

   /* On error, return error. */
   if( retval < 0 ){

      LE_send_msg( GL_INFO, "ORPGSAILS_read_request() Failed (%d)\n", retval );
      return -1;

   }

   /* Return the SAILS status. */
   return( SAILS_request.n_req_sails_cuts );

/* End of ORPGSAILS_get_num_cuts() */
}


/************************************************************************

   Description: 
      This function returns the maximum number of SAILS cuts allowed
      for any site.
 
   Input:
      NONE 

   Output: 
      NONE

   Return: 
      Maximum number of SAILS cuts allowed for any site.

   Notes:
      SAILS_MAX_SAILS_CUTS must match the upper limit of pbd.n_sails_cuts
      in adaptation data.
 
************************************************************************/
int ORPGSAILS_get_max_cuts( ){

   /* Return the maximum number of SAILS cuts. */
   return( SAILS_MAX_SAILS_CUTS );

/* End of ORPGSAILS_get_max_cuts() */
}

/************************************************************************

   Description: 
      This function returns the maximum number of SAILS cuts for this
      site.
 
   Input:
      NONE

   Output: 
      NONE

   Return: 
      Maximum number of SAILS cuts allowed for this site.  
 
************************************************************************/
int ORPGSAILS_get_site_max_cuts(){

   double value = 0.0;

   Site_max_sails_cuts = DEAU_get_values( "pbd.n_sails_cuts", &value, 1 );
   if( Site_max_sails_cuts < 0 ){

      LE_send_msg( GL_INFO, "DEAU_get_values( \"pbd.n_sails_cuts\" ) Failed: %d\n",
                   Site_max_sails_cuts );
      Site_max_sails_cuts = ORPGSAILS_SITE_MAX_DEFAULT;

   }
   else
      Site_max_sails_cuts = (int) value;

   /* Return the SAILS status. */
   return( Site_max_sails_cuts );

/* End of ORPGSITE_get_site_max_cuts() */
}

/************************************************************************
   
   Description: 
      This function returns true (1) if the current VCP allows SAILS
      0 otherwise. 
 
   Input:
      NONE

   Output: 
      NONE

   Return: 
      1 if SAILS allowed for current VCP, 0 if not, or -1 on error.

************************************************************************/
int ORPGSAILS_allowed( ){

   int flags = -1, vcp = -1;
   int retval = 0;

   /* Get the current VCP.  On error, return -1*/
   if( (vcp = ORPGVST_get_vcp()) < 0 ){

      LE_send_msg( GL_INFO, "ORPGVST_get_vcp() Failed (%d)\n", vcp );
      return -1;

   }

   /* Get the VCP flags */
   if( (flags = ORPGVCP_get_vcp_flags( vcp )) < 0 ){

      LE_send_msg( GL_INFO, "ORPGVCP_get_vcp_flags(%d) Failed (%d)\n",
                   vcp, flags );
      return -1;

   }
   
   /* Is SAILS allowed for this VCP? */
   if( flags & VCP_FLAGS_ALLOW_SAILS )
      retval = 1;

   return retval;

/* End of ORPGSAILS_allowed() */
}


/************************************************************************

   Description: 
      This function sets the number of SAILS cuts requested to be 
      inserted when SAILS is active. 
 
   Input:
      num_cuts- Number of SAILS cuts. 
 
   Output: 
      NONE 
                   
   Return:
      0 on success, -1 on error  
************************************************************************/
int ORPGSAILS_set_req_num_cuts( int num_cuts ){

   int retval = 0;

   /* If the value is 0, disable SAILS. */
   if( num_cuts == 0 )
      ORPGINFO_clear_sails_enabled();

   else {

      /* Get the site maximum number of cuts allowed. */
      ORPGSAILS_get_site_max_cuts();

      /* Check against the maximum number allowed for this site. */
      if( num_cuts > Site_max_sails_cuts ){
   
         LE_send_msg( GL_INFO, "Number SAILS cuts %d Exceeds Max Allowed: %d\n",
                      num_cuts, Site_max_sails_cuts );
         return -1;

      }

      /* Enable SAILS. */
      ORPGINFO_set_sails_enabled();

   }

   /* Set the number of SAILS cuts. */
   SAILS_request.n_req_sails_cuts = num_cuts;

   /* Write the SAILS status. */
   if( (retval = ORPGSAILS_write_request()) < 0 ){

      LE_send_msg( GL_INFO, "ORPGSAILS_write_request() Failed: %d\n", retval ); 
      return -1;

   }

   return 0;

/* End of ORPGSAILS_set_req_num_cuts() */
}


/************************************************************************

   Description: 
      This function sets the number of SAILS cuts inserted into the
      SAILS enabled VCP. 
 
   Input:
      num_cuts- Number of SAILS cuts. 
 
   Output: 
      NONE 
                   
   Return:
      0 on success, -1 on error  
************************************************************************/
int ORPGSAILS_set_num_cuts( int num_cuts ){

   /* Get the site maximum number of cuts. */
   ORPGSAILS_get_site_max_cuts();

   /* Check against the maximum number allowed for this site. */
   if( num_cuts > Site_max_sails_cuts ){

      LE_send_msg( GL_INFO, "Number SAILS cuts %d Exceeds Max Allowed: %d\n",
                   num_cuts, Site_max_sails_cuts );
      return -1;

   }

   /* Set the number of SAILS cuts. */
   SAILS_status.n_sails_cuts = num_cuts;

   /* Write the SAILS status. */
   if( (SAILS_io_status = ORPGSAILS_write_status()) < 0 ){

      LE_send_msg( GL_INFO, "ORPGSAILS_write_status() Failed: %d\n", 
                   SAILS_io_status );
      return -1;

   }

   return 0;

/* End of ORPGSAILS_set_num_cuts() */
}


/************************************************************************

   Description: 
      This function initializes the SAILS status/request data if it 
      does not exist. 
 								
   Input:  
      NONE

   Output: 
      NONE

   Return: 
      NONE

************************************************************************/
void ORPGSAILS_init(){

   unsigned char sails = 0;

   /* Is SAILS enabled? */
   sails = ORPGINFO_is_sails_enabled();

   /* Does the SAILS Status message exist? */
   int retval = ORPGSAILS_read_status();

   /* Do we need to initialize the data? */
   if( retval != sizeof(SAILS_status_t) ){

      /* Let's initialize. */
      LE_send_msg( GL_INFO, "Initializing SAILS Status data.\n" );
   
      /* Initialize the read of the fields. */
      SAILS_status.n_sails_cuts = 1;
      if( sails == 0 )
         SAILS_status.n_sails_cuts = 0;

      /* Write the data back to LB. */
      ORPGSAILS_write_status();

   }

   /* Does the SAILS Request message exist? */
   retval = ORPGSAILS_read_request();

   /* Do we need to initialize the data? */
   if( retval != sizeof(SAILS_request_t) ){

      /* Let's initialize. */
      LE_send_msg( GL_INFO, "Initializing SAILS request data.\n" );
   
      /* Initialize the requested number of cuts based on
         whether SAILS is enabled or disabled. */
      SAILS_request.n_req_sails_cuts = 0;
      if( sails )
         SAILS_request.n_req_sails_cuts = 1;

      /* Write the data back to LB. */
      ORPGSAILS_write_request();

   }
 
/* End of ORPGSAILS_init() */
}

