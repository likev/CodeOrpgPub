/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2009/03/12 17:06:46 $
 * $Id: rpg_itc.c,v 1.13 2009/03/12 17:06:46 steves Exp $
 * $Revision: 1.13 $
 * $State: Exp $
 */
/********************************************************************

	This module contains the functions that support 
	Inter-Task Common block (ITC) based interprocess communication.

********************************************************************/



#include <rpg.h>

#define MAXN_ITCS	32

enum {ITC_READ, ITC_WRITE};	/* values for Itc_t.access */

typedef struct {	/* data structure for ITC common blocks */
    int id;		/* ITC common block id */
    int access;		/* access method: ITC_READ or ITC_WRITE */
    char *name;		/* LB_name for the ITC */
    int data_id;	/* Data ID for LB. */
    LB_id_t msgid;	/* LB message id of the ITC */
    int fd;		/* File descriptor for ITC. */
    char *ptr;		/* pointer to the common block */
    int len;		/* length of the ITC */
    int sync_prd;	/* syncronization product id (or ITC_ON_CALL, ITC_ON_EVENT) */
    en_t event_id;	/* associated event id (for ON_EVENT only) */
    int updated;	/* updated flag (for all timing values except ITC_ON_EVENT) */
    int (*callback) ();	/* callback function */
} Itc_t;

static Itc_t Itc [MAXN_ITCS];
			/* list of all ITC */
static int N_itcs = 0;	/* number of ITCs */


static int Events_registered = 0; 
			/* flag indicating that events have been registered 
			   with ITC updates */

/* local functions */
void Report_itc( int ind );
void ITC_update_callback( int fd, LB_id_t msgid, int msg_info, void *arg );


/***********************************************************************

    Description: 
        This function registers an input ITC common block.

    Inputs:	
        itc_id - id number of the ITC. It is a combination of 
		major an minor id code. The major code is used
		for the LB name and the minor code is for the
		LB message id of the ITC.
	first - address of the first element of the ITC.
	last - address after the last element of the ITC.
	sync_prd - the syncronization product id. ITC_ON_CALL means
		no automatic syncronized update is performed.
                ITC_ON_EVENT means automatic update performed when
                event triggered.  ITC_WITH_EVENT means the event
                is triggered upon reading the ITC (assumes
                explicit read).
        event_id - the syncronization event id. (optional)
                
    Return:	
        Return value is never used.

    Notes:
	When an error due to coding or incorrect run time 
	configuration is detected, this function aborts the task.

	This function can be called multiplically to associate
	an input ITC with multiple input products.

***********************************************************************/
int RPG_itc_in( fint *itc_id, void *first, void *last, fint *sync_prd, ... ){

    int i;
    va_list ap;

    /*
      Do for all registered ITCs.
    */
    for (i = 0; i < N_itcs; i++) {

        /*
          Check for duplicate registration.  Duplicates are not registered.
        */
	if( (*itc_id == Itc [i].id) 
                     && 
            (*sync_prd == Itc [i].sync_prd) 
                     &&
            (Itc [i].access == ITC_READ) )
	    return (0);	

    }

    /*
      Too many ITCs registered.
    */
    if (N_itcs >= MAXN_ITCS){

	/* 
          This will cause a later abort.
        */
	LE_send_msg ( GL_INFO, "Too many ITC specified\n");
	return (0);

    }

    /*
      Register ITC id and product synchronized with ITC read.
    */
    Itc [N_itcs].id = *itc_id;
    Itc [N_itcs].data_id = (*itc_id / ITC_IDRANGE) * ITC_IDRANGE;
    Itc [N_itcs].msgid = *itc_id % ITC_IDRANGE;
    Itc [N_itcs].sync_prd = *sync_prd;

    /*
      If ITC update synchronized with an event.
    */
    if( (*sync_prd == ITC_ON_EVENT) || (*sync_prd == ITC_WITH_EVENT) ){ 

       /*
         Register event id.
       */
       va_start( ap, sync_prd );
       Itc [N_itcs].event_id = (en_t) *(va_arg(ap, int *));
       va_end(ap);
       if( *sync_prd == ITC_ON_EVENT )
          Events_registered = 1;

    }
    else
       Itc [N_itcs].event_id = -1;

    /*
      Register all other ITC information.
    */
    Itc [N_itcs].ptr = (char *) first;
    Itc [N_itcs].access = ITC_READ;
    Itc [N_itcs].name = NULL;
    Itc [N_itcs].len = (char *) last - (char *) first;
    Itc [N_itcs].callback = NULL;
    Itc [N_itcs].updated = -1;
    Itc [N_itcs].fd = ORPGDA_open( Itc [N_itcs].data_id, LB_WRITE );

    /* Tell user about the ITC. */
    Report_itc( N_itcs );

    /*
      Increment the number of registered ITCs.
    */
    N_itcs++;
    return (0);

/* End of RPG_itc_in */
}

/***********************************************************************

    Description: 
        This function registers an output ITC common block.

    Inputs:	
        itc_id - id number of the ITC. It is a combination of 
		major an minor id code. The major code is used
		for the LB name and the minor code is for the
		LB message id of the ITC.
	first - address of the first element of the ITC.
	last - address after the last element of the ITC.
	sync_prd - the syncronization product id. ITC_ON_CALL means
		no automatic syncronized update is performed.
                ITC_ON_EVENT means automatic update performed when
                event triggered.  ITC_WITH_EVENT means the event
                is triggered upon writing the ITC (assumes
                explicit write).
        event_id - the syncronization event id. (optional)

    Return:	
        Return value is never used.

    Notes:
	When an error due to coding or incorrect run time 
	configuration is detected, this function aborts the task.

	This function can be called multiplically to associate
	an input ITC with multiple output products.

***********************************************************************/
int RPG_itc_out( fint *itc_id, void *first, void *last, fint *sync_prd, ... ){

    int i;
    va_list ap;

    /*
      Do for all registered ITCs.
    */
    for (i = 0; i < N_itcs; i++) {

        /*
          Check for duplicate ITC registration.  Duplicates are not
          registered.
        */
	if( (*itc_id == Itc [i].id)
                     && 
            (*sync_prd == Itc [i].sync_prd)
                     &&
            (Itc [i].access == ITC_WRITE) )
	    return (0);	

    }

    /*
      Too many ITCs registered.
    */
    if (N_itcs >= MAXN_ITCS) {	

        /* 
          This will cause a later abort.
        */
	LE_send_msg( GL_INFO, "Too many ITC specified\n" );
	return (0);

    }

    /*
      Register the ITC id and product the ITC is synchronized with.
    */
    Itc [N_itcs].id = *itc_id;
    Itc [N_itcs].data_id = (*itc_id / ITC_IDRANGE) * ITC_IDRANGE;
    Itc [N_itcs].msgid = *itc_id % ITC_IDRANGE;
    Itc [N_itcs].sync_prd = *sync_prd;

    /* 
      If ITC update syncronized to event, then ...
    */
    if( *sync_prd == ITC_ON_EVENT || *sync_prd == ITC_WITH_EVENT ){ 

       /*
         Register the event id.
       */
       va_start( ap, sync_prd );
       Itc [N_itcs].event_id = (en_t) *(va_arg(ap, int *));
       va_end(ap);

       if( *sync_prd == ITC_ON_EVENT )
          Events_registered = 1;

    }
    else
       Itc [N_itcs].event_id = -1;

    /*
      Register all other information.
    */
    Itc [N_itcs].ptr = (char *) first;
    Itc [N_itcs].access = ITC_WRITE;
    Itc [N_itcs].name = NULL;
    Itc [N_itcs].len = (char *) last - (char *) first;
    Itc [N_itcs].callback = NULL;
    Itc [N_itcs].updated = -1;

    /* For output ITC, open the ITC with write permission. */
    Itc [N_itcs].fd = ORPGDA_open( Itc [N_itcs].data_id, LB_WRITE );

    /* Tell user about the ITC. */
    Report_itc( N_itcs );

    /*
      Increment the number of ITCs registered.
    */
    N_itcs++;
    return (0);

/* End of RPG_itc_out */
}

/***********************************************************************

    Description: 
        This function registers an callback function for the ITC "itc_id".

    Inputs:	
        itc_id - id number of the ITC. It is a combination of 
		major an minor id code. The major code is used
		for the LB name and the minor code is for the
		LB message id of the ITC.
	func - the callback function for the ITC.

    Return:	
        Return value is never used.

    Notes:
	If "itc_id" is not registered, this function aborts the task.

***********************************************************************/
int RPG_itc_callback( fint *itc_id, fint (*func)() ){

    int i, itc_registered;

    /*
      Initialize the ITC registered flag to "not registered".
    */
    itc_registered = 0;

    /*
      Do for all registered ITCs.
    */
    for (i = 0; i < N_itcs; i++) {

        /*
          If ITC id match, then ...
        */
	if (*itc_id == Itc [i].id){

            /*
              Register the ITC callback function, and set ITC
              registered flag.
            */
	    Itc [i].callback = func;
            itc_registered = 1;

	}

    }

    /*
      Terminate if ITC is not registered.
    */
    if (!itc_registered) {

	PS_task_abort ("unregistered ITC specified in RPG_itc_callback\n");
	return (0);

    }

    return (0);

/* End of RPG_itc_callback */
}

/***********************************************************************

   Description:
      Register read-access ITCs for LB notification.

***********************************************************************/
void ITC_initialize(){

    int ret, i;

    /*
      Do for all registered ITCs ....
    */
    for (i = 0; i < N_itcs; i++) {

       /*
         If not updated by event and access is read ... 
       */
       if( (Itc [i].sync_prd != ITC_ON_EVENT)
                       &&
           (Itc [i].access == ITC_READ) ){

          /*
            Register the ITC for LB Notification.
          */
          if( (ret = ORPGDA_UN_register( Itc [i].data_id, Itc [i].msgid,
                                         ITC_update_callback )) < 0 )
             LE_send_msg( GL_INFO, "ORPGDA_UN_register Failed (%d)\n", ret );
           
          else
             Itc [i].updated = 1;

       }

    }

/* End of ITC_initialize() */
}

/***********************************************************************

    Description: 
        This function reads in all ITC messages associated
	with input product inp_prod.

    Inputs:	
        inp_prod - an input product id (buffer type) that some
	of the ITCs are associated.

    Notes:	
        When an error due to coding or incorrect run time 
	configuration is detected, this function aborts the task.

	This function is called internally when product "inp_prod"
	is read from its buffer.

***********************************************************************/
void ITC_read_all (int inp_prod){

    int i;
    int status;

    /*
      Do for all registered ITCs.
    */
    for (i = 0; i < N_itcs; i++) {

        /*
          If ITC associated with input product and ITC access is read.
        */
	if( (Itc [i].sync_prd == inp_prod) && (Itc [i].access == ITC_READ) ){

            /*
              Read the ITC.
            */
	    RPG_itc_read (&(Itc [i].id), &status);
	    if (status != NORMAL)
		PS_task_abort ("ITC read failed\n");

	}

    }

    return;

/* End of ITC_read_all */
}

/***********************************************************************

    Description: 
        This function writes out all ITC messages associated
	with output product out_prod.

    Inputs:	
        out_prod - an output product id (buffer type) that some
	of the ITCs are associated.

    Notes:
	when an error due to coding or incorrect run time 
	configuration is detected, this function aborts the task.

	This function is called internally when product "out_prod"
	is written to its buffer.

***********************************************************************/
void ITC_write_all (int out_prod){

    int i;
    int status;

    /*
      Do for all ITCs.
    */
    for (i = 0; i < N_itcs; i++) {

        /*
          If ITC associated with output product and ITC access is write.
        */
	if( (Itc [i].sync_prd == out_prod) && (Itc [i].access == ITC_WRITE) ){

            /*
              Write the ITC.
            */
	    RPG_itc_write (&(Itc [i].id), &status);
	    if (status != NORMAL)
		PS_task_abort ("ITC write failed\n");

	}

    }

    return;

/* End of ITC_write_all */
}

/***********************************************************************

    Description: 
        This function reads in the ITC "itc_id".

    Inputs:	
        itc_id - the id of the ITC to update.
        status - pointer to int to store ITC read status.

    Return:
	Return value is never used.

    Notes:
	When an error due to coding or incorrect run time 
	configuration is detected, this function aborts the task.

***********************************************************************/
int RPG_itc_read( fint *itc_id, fint *status ){

    int len, ret;
    int i;

    /*
      Initialize the status return value to ok (NORMAL).
    */
    *status = NORMAL;

    /*
      Do for all registered ITCs.
    */
    for (i = 0; i < N_itcs; i++) {

        /*
          If ITC matchand access mode is read,  break out of loop.
        */
	if( (Itc [i].id == *itc_id) && (Itc [i].access == ITC_READ) )
	    break;

    }

    /*
      ITC not registered.
    */
    if (i >= N_itcs)
	PS_task_abort ("itc_id (%d) not registered\n", *itc_id);

    /*
      Read the ITC if the ITC has been updated or if the LB_notification
      failed (so we don't know whether or not the LB was updated. 
    */
    if( Itc [i].updated != 0 ){

       /* If the updated flag is less than 0, this indicates the LB_notification
          failed.   In this case, we always want to read the LB. */
       if( Itc [i].updated > 0 )
          Itc [i].updated = 0;

       len = ORPGDA_read (Itc [i].data_id, Itc [i].ptr, Itc [i].len,
                          Itc [i].msgid);

       if (len != Itc [i].len){

           LE_send_msg( GL_INFO, "ITC READ FAILED: Itc[%d].data_id %d, Itc[%d].msgid %d\n", 
                        i, Itc[i].data_id, i, Itc[i].msgid );
           LE_send_msg( GL_INFO, "--->Returned len %d != ITc[%d].len %d\n", len, i, Itc[i].len );
	   *status = NO_DATA;

       }

       /*
         If callback function associated with ITC read, call it now.
       */
       else if (Itc [i].callback != NULL)
 	  Itc [i].callback (&(Itc [i].id), &(Itc [i].access));

    }

    /* 
      If event associated with read, post it now. 
    */
    if( (Itc [i].event_id < 0)
               ||
        (Itc [i].sync_prd == ITC_ON_EVENT) )
       return (0);

    if( ( ret = EN_post( Itc [i].event_id, NULL, 0, 
                         EN_POST_FLAG_DONT_NTFY_SENDER ) ) < 0 )
       LE_send_msg( GL_INFO, "EN_post Of %d Failed\n", Itc [i].event_id );

    else
       LE_send_msg( GL_INFO, "Event %d Posted\n", Itc [i].event_id );

    return (0);

/* End of RPG_itc_read */
}

/***********************************************************************

    Description: 
        This function writes out the ITC "itc_id" to its LB.

    Inputs:	
        itc_id - the id of the ITC to output.
        status - pointer to int where status of write operation 
                 is stored.

    Return:	
        Return value is never used.

    Notes:	
        When an error due to coding or incorrect run time configuration 
        is detected, this function aborts the task.

***********************************************************************/
int RPG_itc_write( fint *itc_id, fint *status ){

    int len, ret;
    int i;

    /*
      Initialize the status return value to ok (NORMAL).
    */
    *status = NORMAL;

    /*
      Do for registered ITCs.
    */
    for (i = 0; i < N_itcs; i++) {

        /*
          If ITC match and access mode is write, break out of
          loop.
        */
	if (Itc [i].id == *itc_id && Itc [i].access == ITC_WRITE)
	    break;
    }

    /*
      ITC not registered.
    */
    if (i >= N_itcs)
	PS_task_abort ("itc_id (%d) Not Registered\n", *itc_id);

    /*
      If callback function registered with ITC, call it now.
    */
    if (Itc [i].callback != NULL)
	Itc [i].callback (&(Itc [i].id), &(Itc [i].access));

    /*
      Write the ITC.
    */
    len = ORPGDA_write (Itc [i].data_id, Itc [i].ptr, Itc [i].len,
                        Itc [i].msgid);

    if (len < 0){

        LE_send_msg( GL_INFO, "ITC WRITE FAILED (%d): Itc[%d].data_id %d, Itc[i].msgid %d\n", 
                     len, i, Itc[i].data_id, i, Itc[i].msgid );
	*status = NO_DATA;

    }

    /* 
      If event associated with write, post it now. 
    */
    if( (Itc [i].event_id < 0)
               ||
        (Itc [i].sync_prd == ITC_ON_EVENT) )
       return (0);

    if( ( ret = EN_post( Itc [i].event_id, NULL, 0, 
                         EN_POST_FLAG_DONT_NTFY_SENDER ) ) < 0 )
       LE_send_msg( GL_INFO, "EN_post Of %d Failed\n", Itc [i].event_id );
    else
       LE_send_msg( GL_INFO, "Event %d Posted\n", Itc [i].event_id );

    return (0);

/* End of RPG_itc_write */
}

/***********************************************************************

    Description: 
        This function updates ITC that are attached to the data time.

    Inputs:	
        new_vol - flag, if set, indicates beginning of a volume scan.

    Return:
        There is no return value defined for this function.

    Notes:	
        On fatal error this function aborts the task.

***********************************************************************/
void ITC_update (int new_vol){

    int i;
    int status;

    /*
      Do for all ITCs ...
    */
    for (i = 0; i < N_itcs; i++) {

        /*
          If ITC registered for write access, then ...
        */
	if (Itc [i].access == ITC_WRITE) {

            /*
              If new volume scan and ITC update synchronized to beginning
              of volume, or if ITC update synchronized to beginning of elevation,
              then ....
            */
	    if ((Itc [i].sync_prd == ITC_BEGIN_VOLUME && new_vol) ||
		Itc [i].sync_prd == ITC_BEGIN_ELEVATION) {

                /*
                  Write the ITC.
                */
		RPG_itc_write (&(Itc [i].id), &status);
		if (status != NORMAL ){

                   if( !AP_abort_flag( UNKNOWN ) )
		      PS_task_abort ("ITC write failed in ITC_update. Status = %d\n",
                                      status);

                   else
		      LE_send_msg ( GL_INFO, "ITC write failed in ITC_update. Status = %d\n",
                                   status);

               }

	    }

	}

        /*
          If ITC registered for read access, then ...
        */
	if (Itc [i].access == ITC_READ) {

            /*
              If new volume scan and ITC update synchronized to beginning
              of volume, or if ITC update synchronized to beginning of elevation,
              then ....
            */
	    if ((Itc [i].sync_prd == ITC_BEGIN_VOLUME && new_vol) ||
		Itc [i].sync_prd == ITC_BEGIN_ELEVATION) {

                /*
                  Read ITC.
                */
		RPG_itc_read (&(Itc [i].id), &status);
		if (status != NORMAL){

                   if(!AP_abort_flag( UNKNOWN ) )
		      PS_task_abort ("ITC read failed in ITC_update. Status = %d\n",
                                      status);
                   else
		      LE_send_msg (GL_INFO, "ITC read failed in ITC_update. Status = %d\n",
                                   status);

               }

	    }

	}

    }

    return;

/* End of ITC_update */
}

/***********************************************************************

    Description: 
        This function updates ITC that are associatged with an event.

    Inputs:
	event_id - event id which triggers ITC update.

    Return:
        There is no return value defined for this function.

    Notes:	
        On fatal error this function aborts the task.

***********************************************************************/
void ITC_update_by_event ( en_t event_id ){

    int i;
    int status;

    /*
      If no events registered, no point in continuing.
    */
    if( !Events_registered )
       return;

    /*
      Do for all registered ITCs.
    */
    for (i = 0; i < N_itcs; i++) {

        /*
          If ITC access is write, then ...
        */
	if (Itc [i].access == ITC_WRITE) {

            /*
              If ITC update associated with event and event_id matches 
              ITC event id, then ....
            */
	    if ((Itc [i].sync_prd == ITC_ON_EVENT) 
                          && 
                (Itc [i].event_id == event_id)){

               /*
                 Write ITC.
               */
	       RPG_itc_write (&(Itc [i].id), &status);
	       if (status != NORMAL)
		    PS_task_abort ("ITC Write Failed In ITC_update_by_event.  Status = %d\n",
                                    status);

	    }

	}

        /*
          If ITC access is write, then ...
        */
	if (Itc [i].access == ITC_READ) {

            /*
              If ITC update associated with event and event_id matches 
              ITC event id, then ....
            */
	    if ((Itc [i].sync_prd == ITC_ON_EVENT) 
                          && 
                (Itc [i].event_id == event_id)){ 

                /*
                  Read the ITC.
                */
		RPG_itc_read (&(Itc [i].id), &status);
		if (status != NORMAL)
		    PS_task_abort ("ITC Read Failed in ITC_update_by_event.  Status = %d\n",
                                    status);

	    }

	}

    }

    return;

/* End of ITC_update_by_event */
}

/******************************************************************************

   Description:
       Given the ITC id, returns pointer to start of the ITC data buffer.

   Inputs:
       itc_id - id of ITC    

   Returns:
       Pointer to start of ITC data buffer, or NULL if ITC not defined.

******************************************************************************/
void* ITC_get_data( int itc_id ){

   int i;

   for( i = 0; i < N_itcs; i++ ){

      if( Itc[i].id == itc_id )
         return ( (void *) Itc[i].ptr);

   } 

   return (NULL);

/* End of ITC_get_data */
}

/*****************************************************************************
   Description:
      Sends to the log error file, information about the registered ITC.

   Inputs:
      ind - index of the registered ITC.

   Outputs:

   Returns:

   Notes:

*****************************************************************************/
void Report_itc( int ind ){

   int sync_prod = Itc[ind].sync_prd;

   switch( sync_prod ){

      case ITC_BEGIN_VOLUME:
      {
         if( Itc[ind].access == ITC_READ )
            LE_send_msg( GL_INFO, "Itc %d Read At Beginning Of Volume\n",
                         Itc[ind].id );
         if( Itc[ind].access == ITC_WRITE )
            LE_send_msg( GL_INFO, "Itc %d Written At Beginning Of Volume\n",
                         Itc[ind].id );

         break;
      }
      case ITC_BEGIN_ELEVATION:
      {
         if( Itc[ind].access == ITC_READ )
            LE_send_msg( GL_INFO, "Itc %d Read At Beginning Of Elevation\n",
                         Itc[ind].id );
         if( Itc[ind].access == ITC_WRITE )
            LE_send_msg( GL_INFO, "Itc %d Written At Beginning Of Elevation\n",
                         Itc[ind].id );

         break;
      }
      case ITC_ON_EVENT:
      {
         if( Itc[ind].access == ITC_READ )
            LE_send_msg( GL_INFO, "Itc %d Read On Event %d\n",
                         Itc[ind].id, Itc[ind].event_id );
         if( Itc[ind].access == ITC_WRITE )
            LE_send_msg( GL_INFO, "Itc %d Written On Event %d\n",
                         Itc[ind].id, Itc[ind].event_id );

         break;
      }
      case ITC_WITH_EVENT:
      {
         if( Itc[ind].access == ITC_READ )
            LE_send_msg( GL_INFO, "Itc %d Read With Event %d\n",
                         Itc[ind].id, Itc[ind].event_id );
         if( Itc[ind].access == ITC_WRITE )
            LE_send_msg( GL_INFO, "Itc %d Written With Event %d\n",
                         Itc[ind].id, Itc[ind].event_id );

         break;
      }
      case ITC_ON_CALL:
      {
         if( Itc[ind].access == ITC_READ )
            LE_send_msg( GL_INFO, "Itc %d Read With Call\n",
                         Itc[ind].id );
         if( Itc[ind].access == ITC_WRITE )
            LE_send_msg( GL_INFO, "Itc %d Written With Call\n",
                         Itc[ind].id );

         break;
      }
      default:
      {
         if( Itc[ind].access == ITC_READ )
            LE_send_msg( GL_INFO, "Itc %d Read With Receipt Of Product %d\n",
                         Itc[ind].id, sync_prod );
         if( Itc[ind].access == ITC_WRITE )
            LE_send_msg( GL_INFO, "Itc %d Written With Release Of Product %d\n",
                         Itc[ind].id, sync_prod );

         break;

      }

   /* End of "switch" */

}

/* End of Report_itc() */
}

/*****************************************************************************
   Description:
      LB Notification for ITCs not updated by Event.

   Inputs:
      See orpgda man page for details.

   Outputs:

   Returns:

   Notes:

*****************************************************************************/
void ITC_update_callback( int fd, LB_id_t msgid, int msg_info, void *arg ){

   int i;

   for( i = 0; i < N_itcs; i++ ){

      if( (Itc[i].fd == fd) 
                 && 
          (Itc[i].msgid == msgid) 
                 &&
          (Itc[i].access == ITC_READ) ){

         Itc[i].updated = 1;

      }

   }

/* End of ITC_update_callback() */
}
