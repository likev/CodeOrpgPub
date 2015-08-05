/************************************************************************
 *	Module:   hci_agent.c						*
 *									*
 *	Description: This module contains a set of functions used to	*
 *		     support other HCI applications in order to reduce	*
 *		     the bandwidth load for certain data types.		*
 ***********************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2010/03/19 19:10:25 $
 * $Id: hci_agent.c,v 1.39 2010/03/19 19:10:25 ccalvert Exp $
 * $Revision: 1.39 $
 * $State: Exp $
 */

/*  Local include file definitions.					*/

#include <hci.h>
#include <epre_main.h>
#include <precip_status.h>

/** Verbosity flag */
static int verbose = 0;

/* Static Globals */
static int Rms_changed            = 0; /* 1 = RMS status changed */
static int Rms_state              = 0; /* RMS state data */
static int Process_table_updated  = 0;
static int Process_status_updated = 0;
static int Syslog_updated         = 0;
static int CCZ_RPS_list_updated   = 1;
static int PRF_RPS_list_updated   = 1;
static int RDA_adapt_updated      = 0;
static int Precip_status_updated  = 0;
static int PD_line_info_updated   = 1;
static int PD_prod_user_status_updated   = 0;

static int Num_el_segs            = 0;
static float Ccz_elevs[4]         = {0.0, 0.0, 0.0, 0.0};

static int Prev_sum_area        = PRECIP_AREA_UNKNOWN; 
static int   Prev_time_remaining_to_reset_accum = RESET_ACCUM_UNKNOWN;

#define VERBOSE(s)  (verbose?s:(void)0)

/*  Only 200 characters long because of a bug in lelb_mon
	with log messages greater than 200 characters */
#define LOCAL_NAME_SIZE 200

int Read_options(int argc, char **argv, int* verbose);
void init_hci_task_data ();
int read_hci_task_data (char *buf, int size, int msg_id);
int write_hci_task_data (char *buf, int size, int msg_id);
void Update_ccz_elevs();
void build_rps_list ();
void process_table_updated ();
void process_status_updated ();
void precip_status_updated ();
void prod_info_status_updated ();
void syslog_updated ();

/* Event/Notification Handlers. */
void On_task_status_change(int lb_fd, LB_id_t id, int len, void* data);
void On_hci_data_change(int lb_fd, LB_id_t id, int len, void* data);
void On_rda_adapt_change(int lb_fd, LB_id_t id, int len, void* data);
void On_precip_status_change(int lb_fd, LB_id_t id, int len, void* data);
void On_pd_line_info_change(int lb_fd, LB_id_t id, int len, void* data);
void On_pd_prod_user_status_change(int lb_fd, LB_id_t id, int len, void* data);
void On_syslog_change( int lb_fd, LB_id_t id, int len, void* data );
void On_hci_update_rms( en_t evtcd, void *ptr, size_t msglen );


/*  Termination signal handler. */
int On_terminate( int signal, int flag ){

   HCI_LE_log( "Termination Signal Handler Called" );
   return 0;

/* End of On_terminate(). */
}

/************************************************************************
 *      Description: This function is activated when an RMS status      *
 *                   update event is posted.                            *
 *                                                                      *
 *      Input:  evtcd - event code                                      *
 *              ptr   - pointer to event data                           *
 *              msglen - length (bytes) of event data                   *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/
void On_hci_update_rms( en_t evtcd, void *ptr, size_t msglen ){

   Rms_state = *(int*)ptr;
   Rms_changed = 1;

/* End of On_hci_update_rms(). */
}

/************************************************************************
 *	Description: This function is activated when the HYBRSCAN	*
 *                   LB is updated.		 			*
 *									*
 *	Input:  lb_fd - HYBRSCAN					*
 *		msg_id - ID of updated message				*
 *		msg_len - length of updated message			*
 *		user_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/
void On_precip_status_change ( int lb_fd, LB_id_t msg_id, int msg_len,
                               void *user_data ){

   Precip_status_updated = 1;

/* End of On_precip_status_change(). */
}

/************************************************************************
 *	Description: This function is activated when the		*
 *                   ORPGDAT_PROD_INFO LB is updated. 			*
 *									*
 *	Input:  lb_fd - ORPGDAT_PROD_INFO				*
 *		msg_id - ID of updated message				*
 *		msg_len - length of updated message			*
 *		user_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/
void On_pd_line_info_change ( int lb_fd, LB_id_t msg_id, int msg_len,
                              void *user_data ){

   PD_line_info_updated = 1;

/* End of On_pd_line_info_change(). */
}

/************************************************************************
 *	Description: This function is activated when the		*
 *                   ORPGDAT_PROD_USER_STATUS LB is updated.		*
 *									*
 *	Input:  lb_fd - ORPGDAT_PROD_USER_STATUS			*
 *		msg_id - ID of updated message				*
 *		msg_len - length of updated message			*
 *		user_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/
void On_pd_prod_user_status_change ( int lb_fd, LB_id_t msg_id, int msg_len,
                                     void *user_data ){

   PD_prod_user_status_updated = 1;

/* End of On_pd_prod_user_status_change(). */
}

/************************************************************************
 *	Description: This function is activated when the task status	*
 *                   LB is updated.		 			*
 *									*
 *	Input:  lb_fd - ORPGDAT_RDA_ADAPT_DATA				*
 *		msg_id - ID of updated message				*
 *		msg_len - length of updated message			*
 *		user_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/
void On_rda_adapt_change ( int lb_fd, LB_id_t	msg_id, int msg_len,
                           void *user_data ){

   RDA_adapt_updated = 1;

/* End of On_rda_adapt_change(). */
}

/************************************************************************
 *	Description: This function is activated when the task status	*
 *                   LB is updated.		 			*
 *									*
 *	Input:  lb_fd - ORPGDAT_TASK_STATUS				*
 *		msg_id - ID of updated message				*
 *		msg_len - length of updated message			*
 *		user_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/
void On_task_status_change ( int lb_fd, LB_id_t	msg_id, int msg_len,
                             void *user_data ){

/* The process table message gets updated every time there is 
   a process status change.  However we only need to report 
   the process status of the processes in the process status 
   messsage.  The process status message is filtered so does 
   not include processes that are "monitor only" or special tasks
   such as "active_channel_only".  

   Whenever the process table is updated, we ask mrpg for the process
   status.  When the process status arrives, we report the tasks which
   are failed. */
   if( msg_id == MRPG_PT_MSGID )
      Process_table_updated = 1;

   else if( msg_id == MRPG_PS_MSGID )
      Process_status_updated = 1;

/* End of On_task_status_change(). */
}

/************************************************************************
 *      Description: This function is activated when one of the HCI     *
 *                   data messages are updated.  Special handling is    *
 *                   needed for certain HCI data messages.              *
 *                                                                      *
 *      Input:  lb_fd - ORPGDAT_HCI_DATA                                *
 *              msg_id - ID of updated message                          *
 *              msg_len - length of updated message                     *
 *              user_data - unused                                      *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/
void On_hci_data_change ( int lb_fd, LB_id_t msg_id, int msg_len,
                          void *user_data ){

   switch (msg_id) {

      case HCI_CCZ_TASK_DATA_MSG_ID:    /* Clutter Regions Editor
                                                   task data */
         CCZ_RPS_list_updated = 1;
         break;

      case HCI_PRF_TASK_DATA_MSG_ID:    /* PRF Selection task data */

         PRF_RPS_list_updated = 1;
         break;

   }

/* Endo of On_hci_data_change(). */
}

/************************************************************************
 *      Description: This function is activated when the system status  *
 *                   log LB is updated.  New messages are read and      *
 *                   checked for type.  The latest status and alarm     *
 *                   type messages are maintained separately in the HCI *
 *                   data LB.  The major reason for doing this is so    *
 *                   an HCI application running on a remote system will *
 *                   not be required to read the entire system log file *
 *                   to extract the latest status and alarm messages.   *
 *                                                                      *
 *      Input:  lb_fd - ORPGDAT_SYSLOG                                  *
 *              msg_id - ID of updated message                          *
 *              msg_len - length of updated message                     *
 *              user_data - unused                                      *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/
void On_syslog_change( int lb_fd, LB_id_t msg_id, int msg_len, 
                       void* user_data ){

   Syslog_updated = 1;

/* End of On_syslog_change(). */
}


/************************************************************************
 *	Description: This function is activated when the process table	* 
 *		     in the task status LB is updated. 			*
 *									*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/
void process_table_updated(){

   /* Send command to mrpg to send process status. */
   if( ORPGMGR_send_command( MRPG_STATUS ) < 0 )
      HCI_LE_log( "mrpg Status Request Failed" );

}

/************************************************************************
 *	Description: This function builds a list of failed tasks.	* 
 *		     The list is written to ORPGDAT_HCI_DATA.		*
 *									*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/
void process_status_updated (){

   static int old_num_failed = -1;
   int i;
   int update_flag;
   int status;
   int len;
   int num_failed;
   char	*data = NULL, *buf = NULL;
   Mrpg_process_status_t *table;
   static Hci_task_t failed[HCI_MAX_TASK_STATUS_NUM];
   Orpgtat_entry_t *tat;

   HCI_LE_log( "Servicing Process Status Updated" );

   /* Initialize the failed task table the first time through.	*/
   if (old_num_failed == -1) {

      for (i=0;i<HCI_MAX_TASK_STATUS_NUM;i++) {

         failed [i].instance = -1;
         memset( failed[i].name, 1, 0);
	 failed[i].control_task = 0;

      }

   }

   /* Read the process status and add entry to failed task table
      for all failed tasks. */
   len = ORPGDA_read( ORPGDAT_TASK_STATUS, (char *) &data,
                      LB_ALLOC_BUF, MRPG_PS_MSGID );
   if( len <= 0 ){

      HCI_LE_error( "ORPGDA_read(ORPGDAT_TASK_STATUS) Failed (%d)", len );
      return;

   }

   buf = data;
   num_failed  = 0;
   update_flag = 0;

   /* For all task entries, check to see if any are failed.  If so,
      keep track of all non-monitored tasks that are failed. */
   while(1){

      table = (Mrpg_process_status_t *) buf;

      if( (buf - data + sizeof(Mrpg_process_status_t) > len)
              	           ||
          (buf - data + table->size > len)
              	           ||
          (table->name_off == 0) )
         break; 

      if (table->status == MRPG_PS_FAILED) {

         char *cpt = buf + table->name_off;

         /* Make sure we can find the task attribute table entry. */
         if( (tat = ORPGTAT_get_entry ( cpt )) != NULL ){

	    HCI_LE_error( "failed task [%s] type [%x]", tat->task_name, tat->type);

            if( (strcmp( cpt, failed [num_failed].name ) != 0 )
                                 ||
	        (table->instance != failed [num_failed].instance) ){

               update_flag = 1;
	       failed[num_failed].instance = table->instance;

               /* Determine if control task or not control task */
               if( (tat->type & ORPGTAT_TYPE_RPG_CNTL) ) 
                  failed [num_failed].control_task = 1;

               else 
                  failed [num_failed].control_task = 0;

               /* Append the instance number if multiple instance task. */
               if (table->instance >= 0) 
                  sprintf( failed[num_failed].name,"%s.%d", tat->task_name, 
                           table->instance );

               else
	          sprintf (failed[num_failed].name,"%s", tat->task_name);

            }

            /* Increment the number of failed tasks. */
            num_failed++;

            /* Free Task Attribute Table entry. */
            free(tat);

         }

      }

      /* Go to next entry in process status. */
      if( table->size <= 0 ){

         HCI_LE_error( "Process Status Entry Size Error.  Size <= 0 (%d)", 
                      table->size );
         break;

      }

      buf += table->size;

   }

   /* If the failed task list has changed, write it out for other HCI
      applications to share.*/
   if( (update_flag) || (num_failed != old_num_failed) ){

      status = ORPGDA_write( ORPGDAT_HCI_DATA, (char *) failed,
                             sizeof (Hci_task_t)*num_failed,
			     HCI_TASK_INFO_MSG_ID );

      old_num_failed = num_failed;

   }

   if (data != NULL) 
      free (data);

}
/************************************************************************
 *      Description: This function is activated when the HYBRSCAN       *
 *                   LB is updated.                                     *
 *                                                                      *
 *      Input:  NONE		                                        *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/
void precip_status_updated (){

   int year, month, day, hour, minute, second, ret;
   char *buf = NULL;
   EPRE_buf_t      *epre_buf = NULL;
   EPRE_supl_t     *hydro_supl_data = NULL;
   float           *hydro_adpt_data = NULL;
   Precip_status_t *ps = NULL;

   HCI_LE_log( "Servicing Precip Status Updated" );

   /* Read the Hybrid Scan product. */
   ret = ORPGDA_read( HYBRSCAN, (void *) &buf, LB_ALLOC_BUF, LB_NEXT );
   if( ret >= ALIGNED_SIZE((sizeof(EPRE_buf_t)) + ALIGNED_SIZE(sizeof(Prod_header))) ){

      epre_buf = (EPRE_buf_t *) (buf + sizeof(Prod_header));
      hydro_supl_data = (EPRE_supl_t *) epre_buf->HydroSupl;
      hydro_adpt_data = (float *) epre_buf->HydroAdapt;

   }
   else{

      if( ret > 0 )
         HCI_LE_log( "HYBRSCAN buffer smaller (%d) than expected (%d)",
                      ret, ALIGNED_SIZE(sizeof(EPRE_buf_t)) + ALIGNED_SIZE(sizeof(Prod_header)) );
   
      else
         HCI_LE_error( "ORPGDA_read(HYBRSCAN,LB_NEXT) Failed (%d)", ret );

      if( buf != NULL )
         free( buf );

      return;

   }

   /* Read previous precip status msg so previous values can be */
   /* propagated forward if need be.                            */

   ret = ORPGDA_read( ORPGDAT_HCI_DATA, &ps,
                      LB_ALLOC_BUF, HCI_PRECIP_STATUS_MSG_ID);

   if( ret < (int) sizeof(Precip_status_t) )
   {
     HCI_LE_error( "ORPGDA_read() Failed (%d)", ret );
     ps = (Precip_status_t *) calloc( 1, ALIGNED_SIZE(sizeof(Precip_status_t)));
     if( ps == NULL )
     {
       HCI_LE_error( "calloc Failed for %d Bytes",
                    ALIGNED_SIZE(sizeof(Precip_status_t)) );
       if( buf != NULL ){ free( buf ); }
       return;
     }
   }

   /* Extract the information needed to construct the Precipitation Status. */
   /* Thresholds. */
   ps->rain_dbz_thresh_rainz = hydro_adpt_data[RAIN_DBZ_THRESH];
   ps->rain_area_thresh_raina = (int) hydro_adpt_data[RAIN_AREA_THRESH];
   ps->rain_time_thresh_raint = (int) hydro_adpt_data[RAIN_TIME_THRESH];

   /* Rain area. */
   ps->rain_area = (int) hydro_supl_data->sum_area;

   /* Area trend. */
   if( Prev_sum_area == PRECIP_AREA_UNKNOWN )
   {
      ps->rain_area_trend = TREND_UNKNOWN;
      ps->rain_area_diff = PRECIP_AREA_DIFF_UNKNOWN;
   }
   else if( ps->rain_area <  Prev_sum_area )
   {
      ps->rain_area_trend = TREND_DECREASING;
      ps->rain_area_diff = (Prev_sum_area - ps->rain_area);
   }
   else if( ps->rain_area > Prev_sum_area )
   {
      ps->rain_area_trend = TREND_INCREASING;
      ps->rain_area_diff = (ps->rain_area - Prev_sum_area);
   }
   else
   {
      ps->rain_area_trend = TREND_STEADY;
      ps->rain_area_diff = 0;
   }

   Prev_sum_area = ps->rain_area;

   /* Current status. */
   ps->current_precip_status = hydro_supl_data->rain_detec_flg;

   /* Time area threshold last exceeded RAINA. */
   if( ps->current_precip_status == PRECIP_ACCUM )
      ps->time_last_exceeded_raina = (hydro_supl_data->last_date_rain-1)*86400 
                                   + hydro_supl_data->last_time_rain;

   /* Take into account a value of zero for last_date_rain. This could	*/
   /* occur if the system has yet to exceed RAINA, or if something is 	*/
   /* wrong. Either way, assume the value is unknown.			*/
   if( hydro_supl_data->last_date_rain == 0 )
      ps->time_last_exceeded_raina = TIME_LAST_EXC_RAINA_UNKNOWN;     

   /* Time remaining before reset accumulations. */
   if( ps->current_precip_status == PRECIP_ACCUM )
      ps->time_remaining_to_reset_accum = hydro_adpt_data[RAIN_TIME_THRESH];

   else if( Prev_time_remaining_to_reset_accum != RESET_ACCUM_UNKNOWN )
      ps->time_remaining_to_reset_accum = ps->time_last_exceeded_raina 
                                        - Prev_time_remaining_to_reset_accum;

   Prev_time_remaining_to_reset_accum = ps->time_last_exceeded_raina;

   /* Write the Precipitation Status data. */
   ret = ORPGDA_write( ORPGDAT_HCI_DATA, (char *) ps, sizeof(Precip_status_t), 
                       HCI_PRECIP_STATUS_MSG_ID );
   if( ret < 0 )
      HCI_LE_error( "ORPGDA_write(ORPGDAT_HCI_DATA,HCI_PRECIP_STATUS_MSG_ID) Failed (%d)", ret ); 
   
   if( verbose ){

      HCI_LE_log( "Precipitation Status" );
      if( ps->current_precip_status == PRECIP_ACCUM )
         HCI_LE_status( "--->Current Status:       Accumulating (%d)", 
                      ps->current_precip_status );
      else
         HCI_LE_status( "--->Current Status:       Not Accumulating (%d)", 
                      ps->current_precip_status );

      HCI_LE_status( "--->Rain Area:            %8.1f", ps->rain_area ); 

      ret = unix_time( (time_t *) &ps->time_last_exceeded_raina, &year, &month, &day,
                       &hour, &minute, &second );
      if( ret >= 0 ){

         if( year >= 2000 )
            year = year - 2000;
         else
            year = year - 1900;

         HCI_LE_status( "--->Time Area > RAINA:    %02d/%02d/%02d %02d:%02d:%02d",
                      month, day, year, hour, minute, second );

      }

      switch( ps->rain_area_trend ){

         case TREND_UNKNOWN:
         default:
            HCI_LE_status( "--->Area Trend:           Unknown" );
            break;
         case TREND_INCREASING:
            HCI_LE_status( "--->Area Trend:           Increasing" );
            break;
         case TREND_DECREASING:
            HCI_LE_status( "--->Area Trend:           Decreasing" );
            break;
         case TREND_STEADY:
            HCI_LE_status( "--->Area Trend:           Steady" );
            break;

      }

      HCI_LE_log( "Thresholds:" );
      HCI_LE_status( "--->Rain Area:            %6d", ps->rain_area_thresh_raina ); 
      HCI_LE_status( "--->Rain dBZ:             %4.1f", ps->rain_dbz_thresh_rainz ); 
      HCI_LE_status( "--->Rain Time:            %6d", ps->rain_time_thresh_raint ); 

   }

   /* Free all allocated memory used by this module. */
   if( ps != NULL )
      free(ps);

   if( buf != NULL )
      free(buf);

}

/************************************************************************
 *	Description: This function is activated when the		*
 *		ORPGDAT_PROD_INFO or ORPGDAT_PROD_USER_STAUTS LB	*
 *		is updated.						*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/
void prod_info_status_updated (){

  int ret = -1;
  int i = -1;
  int num_nb_lines = -1;
  int nb_line_index = -1;
  int wideband_line_index = -1;
  Pd_line_entry *nb_line_info = NULL;
  Prod_user_status *nb_user_status = NULL;
  Pd_distri_info *Pd_info = NULL;
  Hci_pd_block_t Pd_block;

  /* Read product generation/distribution info LB. */

  ret = ORPGDA_read( ORPGDAT_PROD_INFO,
                     &Pd_info, LB_ALLOC_BUF, PD_LINE_INFO_MSG_ID );

  if( ret < ALIGNED_SIZE( (sizeof(Pd_distri_info) ) ) )
  {
    HCI_LE_error( "ORPGDA_read(ORPGDAT_PROD_INFO) Failed (%d)", ret );
    if( Pd_info != NULL ){ free( Pd_info ); }
    return;
  }
  else if( ret < sizeof( Pd_distri_info ) ||
      Pd_info->line_list < sizeof( Pd_distri_info ) ||
      ret < Pd_info->line_list + Pd_info->n_lines * sizeof( Pd_line_entry ) )
  {
    HCI_LE_error( "ORPGDAT_PROD_INFO buffer smaller (%d) than expected (%d or %d)",
          ret, sizeof( Pd_distri_info ),
          Pd_info->line_list + ( Pd_info->n_lines*sizeof( Pd_line_entry ) ) );
    if( Pd_info != NULL ){ free( Pd_info ); }
    return;
  }

  Pd_block.pd_info = *Pd_info;

  /* Set number of narrowband lines. Loop through each line. */

  num_nb_lines = Pd_info->n_lines;
  wideband_line_index = ORPGCMI_rda_response() - ORPGDAT_CM_RESPONSE;
  nb_line_info = (Pd_line_entry *)((char *)Pd_info + Pd_info->line_list);

  for( i = 0; i < num_nb_lines; i++ )
  {
    Pd_block.pd_line_info[ i ] = nb_line_info[ i ];
    nb_line_index = nb_line_info[ i ].line_ind;

    if( nb_line_index == wideband_line_index || nb_line_index < 0 )
    {
      continue;
    }

    /* Read product user line status LB. If problem reading LB,
       assume line has failed status. */

    if( nb_user_status != NULL ){ free( nb_user_status ); }

    nb_user_status = NULL;

    ret = ORPGDA_read( ORPGDAT_PROD_USER_STATUS, ( char * ) &nb_user_status,
                       LB_ALLOC_BUF, nb_line_index );

    if( ret < ( signed int ) sizeof( Prod_user_status ) )
    {
      if( ret < 0 )
      {
	if (ret == LB_NOT_FOUND)
          HCI_LE_error( "ORPGDA_read(ORPGDAT_PROD_USER_STATUS) Line: %d failed (%d)",
          nb_line_index, ret );
	else
          HCI_LE_error( "ORPGDA_read(ORPGDAT_PROD_USER_STATUS) Line: %d failed (%d)",
          nb_line_index, ret );
      }
      else
      {
          HCI_LE_error( "ORPGDAT_PROD_USER_STATUS buffer smaller (%d) than expected (%d)",
          ret, sizeof( Prod_user_status ) );
      }

      nb_user_status = malloc( sizeof( Prod_user_status ) );
      nb_user_status->enable = HCI_NO_FLAG;
      nb_user_status->line_stat = US_LINE_FAILED;
    }

    Pd_block.pd_user_status[ i ] = *nb_user_status;
  }

   /* Write the Prod info/status data. */

   ret = ORPGDA_write( ORPGDAT_HCI_DATA, (char *) &Pd_block,
                       sizeof(Hci_pd_block_t), 
                       HCI_PROD_INFO_STATUS_MSG_ID );
   if( ret < 0 )
   {
     HCI_LE_error( "ORPGDA_write(ORPGDAT_HCI_DATA,HCI_PRROD_INFO_STATUS_MSG_ID) Failed (%d)", ret ); 
   }

  /* Cleanup. */

  if( Pd_info != NULL ){ free( Pd_info ); }
  if( nb_user_status != NULL ){ free( nb_user_status ); }
}

/************************************************************************
 *	Description: This function is called whenever SYslog_updated	*
 *		     flag is set.   It writes the latest System Status	*
 *		     log message or latest Alarm message to datastore	*
 *		     ORPGDAT_SYSLOG_LATEST.				*
 * 									*
 *	Input: NONE							*
 *	Output: NONE							*
 *	Return:	NONE							*
 ***********************************************************************/
void syslog_updated (){

   char	msg [LE_MAX_MSG_LENGTH];
   LE_critical_message	*le_msg;

   int retval;

   HCI_LE_log( "Servicing Syslog Updated" );

   /* Read all unread messages until no more unread messages. */
   while(1){

      /* Read the new system log message. */
      retval = ORPGDA_read (ORPGDAT_SYSLOG, msg,
			    LE_MAX_MSG_LENGTH, LB_NEXT);

      /* If the read was successful, determine the type. */
      if( retval <= 0 ){

         /* If the log message has LB_EXPIRED, go to the first 
            unread message in LB, then try and re-read the LB. */
         if( retval == LB_EXPIRED ){

            ORPGDA_seek( ORPGDAT_SYSLOG, 0, LB_FIRST, NULL );
            continue;

         }
         else 
            return;

      }

      le_msg = (LE_critical_message *) msg;

      if( ( le_msg->code & LE_RPG_ALARM_TYPE_MASK ) ||
          ( le_msg->code & LE_RDA_ALARM_TYPE_MASK ) )
      {
         VERBOSE (HCI_LE_log( "Writing a %d byte alarm message to msg id %d",
                  retval, HCI_SYSLOG_LATEST_ALARM));

         /* Write the message to the alarm message in the HCI data LB. */ 
         retval = ORPGDA_write (ORPGDAT_SYSLOG_LATEST, (void *) msg, retval, HCI_SYSLOG_LATEST_ALARM);

         /* Otherwise, it is considered a regular status type message. */
      }
      else{

         VERBOSE (HCI_LE_log( "Writing a %d byte status message to msg id %d",
                               retval, HCI_SYSLOG_LATEST_STATUS));

         /* Write the message to the status message in the HCI data LB. */
         retval = ORPGDA_write (ORPGDAT_SYSLOG_LATEST, (void *) msg, retval, HCI_SYSLOG_LATEST_STATUS);
      }

      if( retval < 0 )
         HCI_LE_error( "Error %d writing to the %s data store", retval, 
                      ORPGCFG_dataid_to_path (ORPGDAT_SYSLOG_LATEST, NULL));

   } /* End of while loop. */

}

/************************************************************************
 *	Description: This function reads the entire RPG system status	*
 *		     log and extracts the latest status and alarm type	*
 *		     messages and writes them to the HCI data LB.	*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/
void write_latest_msgs()
{
	char	latest_le_alarm_msg [LE_MAX_MSG_LENGTH];
	char	latest_le_status_msg [LE_MAX_MSG_LENGTH];
	char	msg [LE_MAX_MSG_LENGTH];
	LE_critical_message	*le_msg;
        int le_status_msg_length = 0;
	int le_alarm_msg_length  = 0;
	int	status = 0;

	HCI_LE_log( "Write Latest Messages" );

/*	Set the message pointer to the beginning of the file.		*/

	if( ORPGDA_seek (ORPGDAT_SYSLOG, 0, LB_FIRST, NULL) == LB_SUCCESS )
	{
	  status = 1;
	}

/*	Read all of the status log messages.				*/

	while (status > 0) {

	    status = ORPGDA_read (ORPGDAT_SYSLOG,
				  msg,
				  LE_MAX_MSG_LENGTH,
				  LB_NEXT);

	    if (status > 0) {

		le_msg = (LE_critical_message *) msg;

/*		Separate the latest alarm and status type messages.	*/

		if( ( le_msg->code & LE_RPG_ALARM_TYPE_MASK ) ||
		    ( le_msg->code & LE_RDA_ALARM_TYPE_MASK ) ) {

		    memcpy (latest_le_alarm_msg, msg, status);
		    le_alarm_msg_length = status;

		} else {

		    memcpy (latest_le_status_msg, msg, status);
		    le_status_msg_length = status;

		}
	    }
	}
	
/*	If a status type message was found then write it out to the	*
*	HCI data LB.							*/

	if (le_status_msg_length > 0) {

	    status = ORPGDA_write (ORPGDAT_SYSLOG_LATEST,
				  latest_le_status_msg,
				  le_status_msg_length,
				  HCI_SYSLOG_LATEST_STATUS);

	    if (status < 0) {

		HCI_LE_error( "Error %d writing to the %s data store", status, ORPGCFG_dataid_to_path(ORPGDAT_SYSLOG_LATEST, NULL));

	    } else {

 	       VERBOSE ( HCI_LE_log( "Wrote %d bytes to msg id %d successfully", status, HCI_SYSLOG_LATEST_STATUS));

	    }
	}
	
/*	If an alarm type message was found then write it out to the	*
*	HCI data LB.							*/

	if (le_alarm_msg_length > 0) {

	    status = ORPGDA_write (ORPGDAT_SYSLOG_LATEST,
				   latest_le_alarm_msg,
				   le_alarm_msg_length,
				   HCI_SYSLOG_LATEST_ALARM);

	    if (status < 0) {

		HCI_LE_error( "Error %d writing to the %s data store", status, ORPGCFG_dataid_to_path(ORPGDAT_SYSLOG_LATEST, NULL));

	    } else {

 		VERBOSE ( HCI_LE_log( "Wrote %d bytes to msg id %d successfully", status, HCI_SYSLOG_LATEST_ALARM));

	    }
	}
}


				
/************************************************************************
 *	Description: This is the main function for the HCI Agent task.	*
 *									*
 *	Input:  argc - number of commandline arguments			*
 *		argv - pointer to commandline argument data		*
 *	Output: NONE							*
 *	Return: exit code						*

 ************************************************************************/
int main (int argc, char **argv){

   int status;

   /* Read commandline options.	*/
   if( !Read_options(argc, argv, &verbose) )
      HCI_task_exit(HCI_EXIT_FAIL);

   /* The following statement is needed so LB notification will work
      in this task for ORPGDAT_HCI_DATA. */
   ORPGDA_write_permission (ORPGDAT_HCI_DATA);

   /* Register Termination Handler. */
   if( ORPGTASK_reg_term_handler( On_terminate ) < 0 ){

      HCI_LE_error("Could not register for termination signals");
      HCI_task_exit(HCI_EXIT_FAIL);

   }

   /* Set up Log Error services. */
   if( (status = ORPGMISC_init(argc, argv, 1000, LB_NORMAL, -1, 0) ) < 0){

      fprintf(stderr, "Error %d occurred while trying to create hci_agent\n", status);
      HCI_task_exit(HCI_EXIT_FAIL);

   }

   /* Initialize HCI task specific data from the configuration file.
      This file should be read in from ASCII, converted to binary,
      and stored in a shared linear buffer if it already doesn't
      exist. */
   HCI_LE_log( "Initializing HCI task data" );
   init_hci_task_data ();

   /* Initialize the latest status and alarm type messages in the HCI data LB. */
   HCI_LE_log( "Initializing status and alarm data");
   write_latest_msgs();

   /* Register for notification services */
   HCI_LE_log("Registering for LB notification for ORPGDAT_SYSLOG");
   status = ORPGDA_UN_register( ORPGDAT_SYSLOG, LB_ANY, On_syslog_change);

   if( status < 0 ){

      fprintf(stderr, 
         "Error %d occurred trying to register for notification of changes to data store %s\n",status,
         ORPGCFG_dataid_to_path(ORPGDAT_SYSLOG, NULL));
         HCI_task_exit(HCI_EXIT_FAIL);

   }

   HCI_LE_log("Registering for LB notification for ORPGDAT_HCI_DATA");
   status = ORPGDA_UN_register(ORPGDAT_HCI_DATA, LB_ANY, On_hci_data_change);

   if( status < 0 ){

      fprintf(stderr, 
         "Error %d occurred trying to register for notification of changes to data store %s\n",status,
         ORPGCFG_dataid_to_path(ORPGDAT_HCI_DATA, NULL));
         HCI_task_exit(HCI_EXIT_FAIL);

   }

   HCI_LE_log("Registering for LB notification for ORPGDAT_TASK_STATUS");
   status = ORPGDA_UN_register(ORPGDAT_TASK_STATUS, MRPG_PT_MSGID, 
                           On_task_status_change);
   if (status < 0){

       fprintf(stderr, 
          "Error %d occurred trying to register for notification of changes to data store %s\n",status,
       ORPGCFG_dataid_to_path(ORPGDAT_TASK_STATUS, NULL));
       HCI_task_exit(HCI_EXIT_FAIL);

   }

   status = ORPGDA_UN_register(ORPGDAT_TASK_STATUS, MRPG_PS_MSGID,
                           On_task_status_change);
   if (status < 0){

      fprintf(stderr, 
         "Error %d occurred trying to register for notification of changes to data store %s\n",status,
      ORPGCFG_dataid_to_path(ORPGDAT_TASK_STATUS, NULL));
      HCI_task_exit(HCI_EXIT_FAIL);

   }

   HCI_LE_log("Registering for LB notification for ORPGDAT_RDA_ADAPT_DATA");
   status = ORPGDA_UN_register(ORPGDAT_RDA_ADAPT_DATA, ORPGDAT_RDA_ADAPT_MSG_ID,
                           On_rda_adapt_change);
   if (status < 0){

      fprintf(stderr, 
         "Error %d occurred trying to register for notification of changes to data store %s\n",status,
      ORPGCFG_dataid_to_path(ORPGDAT_RDA_ADAPT_DATA, NULL));
      HCI_task_exit(HCI_EXIT_FAIL);

   }

   HCI_LE_log("Registering for LB notification for HYBRSCAN");
   status = ORPGDA_UN_register(HYBRSCAN, LB_ANY, On_precip_status_change);
   if (status < 0){

      fprintf(stderr, 
         "Error %d occurred trying to register for notification of changes to data store %s\n",status,
      ORPGCFG_dataid_to_path(HYBRSCAN, NULL));
      HCI_task_exit(HCI_EXIT_FAIL);

   }

   HCI_LE_log("Registering for LB notification for ORPGDAT_PROD_INFO");
   status = ORPGDA_UN_register(ORPGDAT_PROD_INFO, PD_LINE_INFO_MSG_ID, 
                           On_pd_line_info_change);
   if (status < 0){

       fprintf(stderr, 
          "Error %d occurred trying to register for notification of changes to data store %s\n",status,
       ORPGCFG_dataid_to_path(ORPGDAT_PROD_INFO, NULL));
       HCI_task_exit(HCI_EXIT_FAIL);

   }

   HCI_LE_log("Registering for LB notification for ORPGDAT_PROD_USER_STATUS");
   status = ORPGDA_UN_register(ORPGDAT_PROD_USER_STATUS, LB_ANY, 
                           On_pd_prod_user_status_change);
   if (status < 0){

       fprintf(stderr, 
          "Error %d occurred trying to register for notification of changes to data store %s\n",status,
       ORPGCFG_dataid_to_path(ORPGDAT_PROD_USER_STATUS, NULL));
       HCI_task_exit(HCI_EXIT_FAIL);

   }

   status = EN_register (ORPGEVT_RMS_CHANGE, (void *) On_hci_update_rms);

   /* Do initial reads.   If read fails in Update_ccz_elevs because RDA adaptation data 
      is not available, no harm - no foul. */
   process_table_updated ();
   Update_ccz_elevs();

   /* Loop all of the time */
   while (1){

      /* If the RMS status has changed, update the RMS status message in the 
         HCI data LB.  All other message updates are done through LB notification. */ 
      if (Rms_changed) {

	 HCI_LE_log( "RMS Status Update Event Received" );
         Rms_changed = 0;
	 status = ORPGDA_write (ORPGDAT_HCI_DATA, (char *) &Rms_state, sizeof (int),
			        HCI_RMS_STATUS_MSG_ID);

         if (status <= 0) 
            HCI_LE_error( "Error writing RMS status data [%d]", status);

      }
	    
      if (Process_table_updated) {

         HCI_LE_log( "Process Table Updated" );
         Process_table_updated = 0;
         process_table_updated ();

      }

      if (Process_status_updated) {

         HCI_LE_log( "Process Status Updated" );
         Process_status_updated = 0;
         process_status_updated ();

      }

      if (Syslog_updated) {

         HCI_LE_log( "System Status Log Updated" );
         Syslog_updated = 0;
         syslog_updated ();

      }

      if (Precip_status_updated){

         HCI_LE_log( "Precip Status Updated" );
         Precip_status_updated = 0;
         precip_status_updated ();

      }

      if (PD_line_info_updated || PD_prod_user_status_updated){

         if( PD_line_info_updated )
            HCI_LE_log( "PD Line Info Updated" );

         else
            HCI_LE_log( "PD Prod User Status Updated" );

         PD_line_info_updated = 0;
         PD_prod_user_status_updated = 0;
         prod_info_status_updated ();

      }

      if( (CCZ_RPS_list_updated) || (PRF_RPS_list_updated) ) {

         if( CCZ_RPS_list_updated )
	    HCI_LE_log("Data for Clutter Regions Editor task updated; building new RPS list");

         else 
	    HCI_LE_log("Data for PRF Selection task updated; building new RPS list");

	 CCZ_RPS_list_updated = 0;
         PRF_RPS_list_updated = 0;
         build_rps_list ();

      }

      if(RDA_adapt_updated) {

         HCI_LE_log( "RDA Adaptation Data Updated" );
         RDA_adapt_updated = 0;
         Update_ccz_elevs();

      }

      msleep(1000);

   }
	
/*END of main()*/
}

/************************************************************************
 *	Description: Read the command-line options variables.		*
 *									*
 *	Input:  argc - number of commandline arguments			*
 *		argv - pointer to commandline argument data		*
 *	Output: NONE							*
 *	Return: 1 upon success, 0 otherwise				*
 ************************************************************************/
int Read_options( int argc, char **argv, int* verbose_level ){

   int retval = 1;
   int input;
   *verbose_level = 0;

   while ((input = getopt(argc,argv,"v")) != -1){

      switch(input){

         case 'v':  *verbose_level = 1;
	    break;

         default:
            retval = 0;
            printf ("\n\tUsage:\t%s [options]\n",argv[0]);
            printf ("\n\tDescription:\n");
            printf ("\n\t\tMonitor ORPG data files and write summary information for hci tasks\n");
            printf ("\t\t-v\t\tturn verbosity on\n");			
            break ;	
      }
      if (!retval)
         break;
   }

   return(retval);

/* End of Read_options() */
}

/************************************************************************
 *	Description: This function initializes the HCI task data	*
 *		     messages.						*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
init_hci_task_data ()
{
	int err;
	int cnt;
	int ret;
	char *buf;

	err = 0;

	HCI_LE_log( "HCI Task Data Initialized" );

	buf = (char *) calloc (4,HCI_MAX_ITEMS_IN_LIST+8);

/*	If the message exists for the Clutter Regions Editor task then	*
 *	don't initialize it from configuration.				*/

	ret = ORPGDA_read (ORPGDAT_HCI_DATA,
			   buf,
			   ALIGNED_SIZE(sizeof (Hci_ccz_data_t)),
			   HCI_CCZ_TASK_DATA_MSG_ID);

	HCI_LE_log( "%d bytes read from HCI_CCZ_TASK_DATA_MSG_ID", ret);

	if (ret <= 0) {

/*	    Get configuration data for the Clutter Regions Editor task	*/

	    CS_cfg_name ("");
	    CS_cfg_name ("hci_task_data");
	    CS_control (CS_COMMENT | '#');

	    if ((CS_entry ("hci_ccz",0,0,NULL) < 0) ||
	        (CS_level (CS_DOWN_LEVEL) < 0)) {

		CS_cfg_name ("");
		HCI_LE_log("Unable to read data for HCI task hci_ccz");

	    } else {

		int size;
		Hci_ccz_data_t	*data;

		data = (Hci_ccz_data_t *) buf;
		size = sizeof (Hci_ccz_data_t);
		size = ALIGNED_SIZE (size);

		if (CS_entry ("n_products",   1 | CS_SHORT, 0,
					(void *)&(data->n_products))  <= 0 ||
		    CS_entry ("default_low",  1 | CS_SHORT, 0,
					(void *)&(data->low_product)) <= 0 ||
		    CS_entry ("default_high", 1 | CS_SHORT, 0,
					(void *)&(data->high_product))<= 0 ||
		    CS_entry ("n_cuts",       1 | CS_SHORT, 0,
					(void *)&(data->n_cuts))      <= 0) {

		    err = 1;

		}

		if (data->n_cuts > 0) {

/*		    Don't let the number of cuts in list exceed the maximum	*
 *		    allowed.							*/

		    if (data->n_cuts > HCI_MAX_CUTS_IN_LIST)
		        data->n_cuts = HCI_MAX_CUTS_IN_LIST;

		    cnt = 0;

		    while (cnt < data->n_products &&
			CS_entry ("cuts", (cnt+1) | CS_SHORT, 0,
				(void *) &data->cut_list[cnt]) > 0) {

			cnt++;

		    }

		    if (cnt != data->n_cuts) {

			CS_report ("hci_ccz:: bad cut list");
			err = 1;

		    }
		}

		if (data->n_products > 0) {

/*		    Don't let the number of products in list exceed the maximum	*
 *		    allowed.							*/

		    if (data->n_products > HCI_MAX_ITEMS_IN_LIST)
		        data->n_products = HCI_MAX_ITEMS_IN_LIST;

		    cnt = 0;

		    while (cnt < data->n_products &&
			CS_entry ("products", (cnt+1) | CS_SHORT, 0,
				(void *) &data->product_list[cnt]) > 0) {

			cnt++;

		    }

		    if (cnt != data->n_products) {

			CS_report ("hci_ccz:: bad product list");
			err = 1;

		    }
		}

		CS_cfg_name ("");

		if (!err)
		   ret = write_hci_task_data (buf,
					      size,
					      HCI_CCZ_TASK_DATA_MSG_ID);

	    }

	} else {

	    HCI_LE_log("Clutter Regions Editor (hci_ccz) data already exist");

	}

/*	If the message exists for the PRF Selection task then		*
 *	don't initialize it from configuration.				*/

	err = 0;

	ret = ORPGDA_read (ORPGDAT_HCI_DATA,
			   buf,
			   ALIGNED_SIZE(sizeof (Hci_prf_data_t)),
			   HCI_PRF_TASK_DATA_MSG_ID);

	HCI_LE_log( "%d bytes read from HCI_PRF_TASK_DATA_MSG_ID", ret);

	if (ret <= 0) {

/*	    Get configuration data for the PRF Selection task		*/

	    CS_cfg_name ("");
	    CS_cfg_name ("hci_task_data");
	    CS_control (CS_COMMENT | '#');
	    CS_control (CS_RESET);

	    if ((CS_entry ("hci_prf",0,0,NULL) < 0) ||
		(CS_level (CS_DOWN_LEVEL) < 0)) {

		CS_cfg_name ("");
		HCI_LE_log("Unable to read data for HCI task hci_prf");

	    } else {

		int size;
		Hci_prf_data_t	*data;

		data = (Hci_prf_data_t *) buf;
		size = sizeof (Hci_prf_data_t);
		size = ALIGNED_SIZE (size);

		if (CS_entry ("n_products",   1 | CS_SHORT, 0,
					(void *)&(data->n_products)) <= 0 ||
		    CS_entry ("default_low",  1 | CS_SHORT, 0,
					(void *)&(data->low_product)) <= 0 ||
		    CS_entry ("default_high", 1 | CS_SHORT, 0,
					(void *)&(data->high_product)) <= 0 ||
		    CS_entry ("n_cuts",       1 | CS_SHORT, 0,
					(void *)&(data->n_cuts)) <= 0) {

		    err = 1;

		}

		if (data->n_cuts > 0) {

/*		    Don't let the number of cuts in list exceed the maximum	*
 *		    allowed.							*/

		    if (data->n_cuts > HCI_MAX_CUTS_IN_LIST)
			data->n_cuts = HCI_MAX_CUTS_IN_LIST;

		    cnt = 0;

		    while (cnt < data->n_cuts &&
			CS_entry ("cuts", (cnt+1) | CS_SHORT, 0,
				(void *) &data->cut_list[cnt]) > 0) {

			cnt++;

		    }

		    if (cnt != data->n_cuts) {

			CS_report ("hci_prf:: bad cut list");
			err = 1;

		    }
		}

		if (data->n_products > 0) {

/*		    Don't let the number of products in list exceed the maximum	*
 *		    allowed.							*/

		    if (data->n_products > HCI_MAX_ITEMS_IN_LIST)
			data->n_products = HCI_MAX_ITEMS_IN_LIST;

		    cnt = 0;

		    while (cnt < data->n_products &&
			CS_entry ("products", (cnt+1) | CS_SHORT, 0,
				(void *) &data->product_list[cnt]) > 0) {

			cnt++;

		    }

		    if (cnt != data->n_products) {

			CS_report ("hci_prf:: bad product list");
			err = 1;

		    }
		}

		CS_cfg_name ("");

		if (!err)
		   ret = write_hci_task_data (buf,
					      size,
					      HCI_PRF_TASK_DATA_MSG_ID);

	    }

	} else {

		HCI_LE_log("PRF Selection (hci_prf) data already exist");

	}

	free (buf);
}

/************************************************************************
 *	Description: This function writes the specified HCI task data	*
 *		     message.						*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

int
write_hci_task_data (
char	*buf,
int	size,
int	msg_id
)
{

	int	status;

	HCI_LE_log( "Write HCI Task Data" );

	status = ORPGDA_write (ORPGDAT_HCI_DATA,
			       buf,
			       size,
			       msg_id);

	if (status <= 0) {

	    HCI_LE_error( "Unable to write message %d to ORPGDAT_HCI_DATA: %d",
		msg_id, status);

	} else {

	    HCI_LE_log( "message %d of ORPGDAT_HCI_DATA updated: %d",
		msg_id, status);

	}

	return (status);
}

/************************************************************************
 *	Description: This function reads the specified HCI task data	*
 *		     message.						*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/
int read_hci_task_data( char *buf, int size, int msg_id ){

	int	status;

	HCI_LE_log( "Read HCI Task Data: size: %d, msg_id: %d", size, msg_id );

	status = ORPGDA_read (ORPGDAT_HCI_DATA, buf, size, msg_id);

	if (status <= 0) {

	    HCI_LE_error( "Unable to read message %d of ORPGDAT_HCI_DATA: %d",
		msg_id, status);

	} else {

	    HCI_LE_log( "message %d of ORPGDAT_HCI_DATA read: %d",
		msg_id, status);

	}

	return (status);

}

/************************************************************************
 *	Description: This function sets the ccz elevation angles in 	*
 *		     Hci_ccz_data_t structure based on the values	*
 *		     provided in RDA adaptation data.               	*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/
void Update_ccz_elevs(){

   int ind, ret;
   char *buf = NULL;
   Hci_ccz_data_t *data = NULL;

   float seg1lim, seg2lim, seg3lim, seg4lim;
   int   nbr_el_segments;

   HCI_LE_log( "Servicing Update CCZ Elevations" );

   /* Determine which elevation segments to generate products for CCZ editor. */
   if( ORPGRDA_ADPT_get_data_value( ORPGRDA_ADPT_NBR_EL_SEGMENTS, &nbr_el_segments ) < 0 ){

      HCI_LE_error( "Unable to get ORPGRDA_ADAPT_NBR_EL_SEGMENTS value" );
      return;

   }

   /* Validate the number of elevation segments. */
   if( (nbr_el_segments <= 0) 
               || 
       (nbr_el_segments  > MAX_ELEVATION_SEGS_ORDA) ){

      HCI_LE_error( "Invalid Number Of Elevation Segments: %d", nbr_el_segments);
      return;

   }

   Num_el_segs = nbr_el_segments;

   if( (ORPGRDA_ADPT_get_data_value( ORPGRDA_ADPT_SEG1LIM, &seg1lim ) < 0) ||
       (ORPGRDA_ADPT_get_data_value( ORPGRDA_ADPT_SEG2LIM, &seg2lim ) < 0) ||
       (ORPGRDA_ADPT_get_data_value( ORPGRDA_ADPT_SEG3LIM, &seg3lim ) < 0) ||
       (ORPGRDA_ADPT_get_data_value( ORPGRDA_ADPT_SEG4LIM, &seg4lim ) < 0) ){

      HCI_LE_error( "Unable to get ORPGRDA_ADPT_SEGxLIM values" );
      return;

   }

   Ccz_elevs[0] = seg1lim;
   Ccz_elevs[1] = seg2lim;
   Ccz_elevs[2] = seg3lim;
   Ccz_elevs[3] = seg4lim;

   buf = (char *) calloc (1, ALIGNED_SIZE(sizeof(Hci_ccz_data_t)));
   if( buf == NULL ){

      HCI_LE_error( "calloc Failed for %d Bytes", 
                   ALIGNED_SIZE(sizeof(Hci_ccz_data_t)) );
      return;

   }

   /* Read the existing HCI CCZ task data */
   ret = ORPGDA_read( ORPGDAT_HCI_DATA, buf, ALIGNED_SIZE(sizeof (Hci_ccz_data_t)),
                       HCI_CCZ_TASK_DATA_MSG_ID );

   if( ret <= 0 ){

      HCI_LE_error( "ORPGDA_read( HCI_CCZ_TASK_DATA_MSG_ID ) Failed (%d)", ret );
      return;

   }

   HCI_LE_log( "%d bytes read from HCI_CCZ_TASK_DATA_MSG_ID", ret);

   /* Transfer the Clutter map elevation angles to Hci_ccz_data_t structure. */
   data = (Hci_ccz_data_t *) buf;
   if( Num_el_segs > 0 ){

      data->n_cuts = Num_el_segs;
      for( ind = 0; ind < data->n_cuts; ind++ ){

         /* For the lowest segment, the target elevation is 1/2 the segment
            limit angle. */
         if( ind == 0 )
            data->cut_list[ind] = (int) (Ccz_elevs[ind]*10.0/2.0);

         /* For the highest segment, the target elevation is 1 degree above the 
            segment limit angle. */
         else if(  ind == (data->n_cuts - 1) )
            data->cut_list[ind] = (int) (Ccz_elevs[ind-1]*10.0 + 10.0);

         /* For intermediate segments, the target elevation is the midpoint
            between segments. */
         else
            data->cut_list[ind] = (int) ((Ccz_elevs[ind] + Ccz_elevs[ind-1])*10.0/2.0);


      }  

   }

   /* Write the data out.  This will cause the RPS_list_updated flag to 
      be set and a new RPS list generated. */
   ret = write_hci_task_data( buf, ALIGNED_SIZE(sizeof(Hci_ccz_data_t)), 
                              HCI_CCZ_TASK_DATA_MSG_ID );

   HCI_LE_log("Clutter Regions Editor (hci_ccz) data Updated");

   /* Free allocated memory.                                    */
   if( buf != NULL )
      free(buf);

}

/************************************************************************
 *	Description: This function creates an RPS list based on the	*
 *		     products defined in the HCI task data config file	*
 *		     and writes it to the response LB for ps_routine	*
 *		     to read and add to its generation list.		*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/
void build_rps_list (){

	char	*inbuf;
	char	*outbuf;
	time_t	tm;
	int	ret;
	int	i;
	int	size;
	Pd_msg_header	*hdr;
	Pd_request_products	*req;

	HCI_LE_log( "Servicing Build RPS List" );

	inbuf  = (char *) calloc (ALIGNED_SIZE ( sizeof (Hci_ccz_data_t)),1);
	outbuf = (char *) calloc (1024,1); /* Size of output buffer enough
					      to handle and RPS list of 62
					      products. */

/*	Build header for RPS list */

	tm = time (NULL);

	hdr = (Pd_msg_header *) outbuf;
	req = (Pd_request_products *)(outbuf +
			ALIGNED_SIZE (sizeof (Pd_msg_header)));

	hdr->date     = RPG_JULIAN_DATE (tm);
	hdr->time     = RPG_TIME_IN_SECONDS (tm);
	hdr->length   = ALIGNED_SIZE (sizeof (Pd_msg_header));
	hdr->src_id   = 0;
	hdr->dest_id  = 0;
	hdr->n_blocks = 1;
	hdr->line_ind = 0;

/*	Add the background product for the Clutter Regions Editor task.	*/

	size = ALIGNED_SIZE (sizeof (Hci_ccz_data_t));

	ret = read_hci_task_data ((char *)inbuf, size, HCI_CCZ_TASK_DATA_MSG_ID);
	
	if (ret <= 0) {

	    HCI_LE_log( "Unable to read clutter regions editor task data: %d", ret);

	} else {

	    Hci_ccz_data_t	*data;
	    int			prod_code;

	    data = (Hci_ccz_data_t *) inbuf;

	    for( i = 0; i < data->n_cuts; i++ ){

/*		Add entry for both low and high bandwidth cases.	*/

		prod_code = ORPGPAT_get_prod_id_from_code (data->high_product);

		req->prod_id = prod_code;
		if (req->prod_id <= 0) {

		    HCI_LE_error( "Invalid product code detected: %d",
			req->prod_id);
		    continue;

		}

		req->divider = -1;
		req->length  = 32;
		req->flag_bits = 0x8000;
		req->seq_number = 0;
		req->num_products = -1;
		req->req_interval = 1;
		req->VS_date      = 0;
		req->VS_start_time = 0;
		req->params[0] = PARAM_UNUSED;
		req->params[1] = PARAM_UNUSED;
		req->params[2] = data->cut_list[i];
		req->params[3] = PARAM_UNUSED;
		req->params[4] = PARAM_UNUSED;
		req->params[5] = PARAM_UNUSED;

	        HCI_LE_log( "CCZ Product Request: ID: %d, Elv: %d", 
                             req->prod_id, req->params[2] );

		req++;
		hdr->length+=sizeof(Pd_request_products);
		hdr->n_blocks++;

/*		Only generate the low bandwidth product if product ID	*
 *		different from high bandwidth product.			*/

		if (data->high_product != data->low_product) {

		    prod_code = ORPGPAT_get_prod_id_from_code (data->low_product);

		    req->prod_id = prod_code;
		    if (req->prod_id <= 0) {

			HCI_LE_error( "Invalid product code detected: %d",
			req->prod_id);

		        continue;

		    }

		    req->divider = -1;
		    req->length  = 32;
		    req->flag_bits = 0x8000;
		    req->seq_number = 0;
		    req->num_products = -1;
		    req->req_interval = 1;
		    req->VS_date      = 0;
		    req->VS_start_time = 0;
		    req->params[0] = PARAM_UNUSED;
		    req->params[1] = PARAM_UNUSED;
		    req->params[2] = data->cut_list[i];
		    req->params[3] = PARAM_UNUSED;
		    req->params[4] = PARAM_UNUSED;
		    req->params[5] = PARAM_UNUSED;

	            HCI_LE_log( "Product Request: ID: %d, Elv: %d", 
                                 req->prod_id, req->params[2] );
		    req++;
		    hdr->length+=sizeof(Pd_request_products);
		    hdr->n_blocks++;

		}
	    }
	}

/*	Add the background product for the PRF Selection task.	*/

	size = ALIGNED_SIZE (sizeof (Hci_prf_data_t));

	ret = read_hci_task_data ((char *)inbuf, size, HCI_PRF_TASK_DATA_MSG_ID);
	
	if (ret <= 0) {

	    HCI_LE_error( "Unable to read PRF selection task data: %d", ret);

	} else {

	    int	prod_code;
	    Hci_prf_data_t	*data;

	    data = (Hci_prf_data_t *) inbuf;

	    for( i = 0;i < data->n_cuts; i++ ){

/*		Add entry for both low and high bandwidth cases.	*/

		prod_code = ORPGPAT_get_prod_id_from_code (data->high_product);

		req->prod_id = prod_code;
		if (req->prod_id <= 0) {

		    HCI_LE_error( "Invalid product code detected: %d",
			req->prod_id);

		    continue;

		}

		req->divider = -1;
		req->length  = 32;
		req->flag_bits = 0x8000;
		req->seq_number = 0;
		req->num_products = -1;
		req->req_interval = 1;
		req->VS_date      = 0;
		req->VS_start_time = 0;
		req->params[0] = PARAM_UNUSED;
		req->params[1] = PARAM_UNUSED;
		req->params[2] = data->cut_list[i];
		req->params[3] = PARAM_UNUSED;
		req->params[4] = PARAM_UNUSED;
		req->params[5] = PARAM_UNUSED;

	        HCI_LE_log( "PRF Product Request: ID: %d, Elv: %d", 
                             req->prod_id, req->params[2] );

		req++;
		hdr->length+=sizeof(Pd_request_products);
		hdr->n_blocks++;

/*		Only generate the low bandwidth product if product ID	*
 *		different from high bandwidth product.			*/

		if (data->high_product != data->low_product) {

		    prod_code = ORPGPAT_get_prod_id_from_code (data->low_product);

		    req->prod_id = prod_code;
		    if (req->prod_id <= 0) {

			HCI_LE_error( "Invalid product code detected: %d",
			req->prod_id);

		        continue;

		    }

		    req->divider = -1;
		    req->length  = 32;
		    req->flag_bits = 0x8000;
		    req->seq_number = 0;
		    req->num_products = -1;
		    req->req_interval = 1;
		    req->VS_date      = 0;
		    req->VS_start_time = 0;
		    req->params[0] = PARAM_UNUSED;
		    req->params[1] = PARAM_UNUSED;
		    req->params[2] = data->cut_list[i];
		    req->params[3] = PARAM_UNUSED;
		    req->params[4] = PARAM_UNUSED;
		    req->params[5] = PARAM_UNUSED;

	            HCI_LE_log( "PRF Product Request: ID: %d, Elv: %d", 
                                 req->prod_id, req->params[2] );

		    req++;
		    hdr->length+=sizeof(Pd_request_products);
		    hdr->n_blocks++;

		}
	    }
	}

/*	Now that we have built the RPS list we need to write it to the	*
 *	response LB.							*/

	ret = ORPGDA_write (ORPGDAT_RT_REQUEST, (char *) outbuf, hdr->length, LB_ANY);

	if (ret <= 0)
	    HCI_LE_error( "Error sending product request: %d", ret);

	else {

	    HCI_LE_log( "Updated product request sent: %d", ret);
	    EN_post (ORPGEVT_RT_REQUEST, NULL, 0, 0);

	}

/*      Free allocated memory.                                          */
        if( inbuf != NULL )
            free(inbuf);

        if( outbuf != NULL )
            free(outbuf);

}
