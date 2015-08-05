/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2005/11/03 21:55:27 $
 * $Id: write_gagedb.c,v 1.18 2005/11/03 21:55:27 steves Exp $
 * $Revision: 1.18 $
 * $State: Exp $
 */
#define WRITE_GAGEDB_C
#include <pcipdalg.h>

#include <gagedata.h>
#include <assert.h>
#include <prod_gen_msg.h>
#include <orpgsite.h>

#define OCDATE      0
#define OCTIME      1
#define OCTGRY      2
#define OLDATE      3
#define OLTIME      4

/* Global Variables. */
int Previous_wxstatus;

/*\/////////////////////////////////////////////////////////////////////////

   Description:
      Write the Precipitation Status Message to the Gage Database.

   Inputs:
      precip_status - pointer to Precipitation Status Message.

   Note:
      This function was removed from rgagedaq and made part of pcipdalg.  

/////////////////////////////////////////////////////////////////////////\*/
void Write_gagedb( int *precip_status ){

   gagedata *gagedb;
   static int last_avail = 1;
   int num_items, buflen, len, hdr_offset;
   int year, month, day, hour, minute, second;
   int *buf;
   time_t utime;
   
   LB_id_t data_time;
   LB_info list;
   
   /* Get some information about this Linear Buffer. */
   num_items = RPGC_data_access_list( GAGEDATA, &list, last_avail );
   if( num_items <= 0 ){

      RPGC_log_msg( GL_ERROR, "There are no GAGEDATA entries. num_items = %d\n",
                    num_items );
      exit( -1 );

   }

   /* Get the Linear Buffer ID. */
   data_time = list.id;

   /* Calculate the size of the ORPG product header. */
   hdr_offset = sizeof( Prod_header );

   /* Calculate the size of the product data with header. */
   buflen = sizeof( gagedata ) + hdr_offset;

   /* Allocate space to accommodate this buffer. */
   buf = (int *) calloc( (size_t) 1, (size_t) buflen );
   assert( buf != NULL );

   /* Read the Gage Database Linear Buffer. */
   len = RPGC_data_access_read( GAGEDATA, (char *) buf, buflen, data_time );

   if( len <= 0 ){

      RPGC_log_msg( GL_ERROR, "Bad len from GAGEDATA read. len = %d\n", len );
      exit(-1);

   }

   /* Update precip_status_info. */
   gagedb = (gagedata *) (buf + hdr_offset / sizeof( int ) );
   gagedb->precip_status_info.date_detect_ran = precip_status[ OCDATE ];
   gagedb->precip_status_info.time_detect_ran = precip_status[ OCTIME ];
   gagedb->precip_status_info.date_last_precip = precip_status[ OLDATE ];
   gagedb->precip_status_info.time_last_precip = precip_status[ OLTIME ];
   gagedb->precip_status_info.last_precip_cat = gagedb->precip_status_info.cur_precip_cat; 
   gagedb->precip_status_info.cur_precip_cat = precip_status[ OCTGRY ];

   RPGC_log_msg( GL_INFO, "Precipitation Status Message:\n" );

   utime = (gagedb->precip_status_info.date_detect_ran - 1)*SEC_IN_DAY + 
            gagedb->precip_status_info.time_detect_ran;
   if( utime > 0 ){

      unix_time( &utime, &year, &month, &day, &hour, &minute, &second );
      RPGC_log_msg( GL_INFO, "--->Date/Time Mode Selection Ran: %02d/%02d/%02d %02d:%02d:%02d\n",
                    month, day, year, hour, minute, second );

   }
 
   utime = (gagedb->precip_status_info.date_last_precip - 1)*SEC_IN_DAY + 
            gagedb->precip_status_info.time_last_precip;
   if( utime > 0 ){

      unix_time( &utime, &year, &month, &day, &hour, &minute, &second );
      RPGC_log_msg( GL_INFO, "--->Date/Time Precip Mode Criteria Last Met: %02d/%02d/%02d %02d:%02d:%02d\n",
                    month, day, year, hour, minute, second );

   }

   RPGC_log_msg( GL_INFO, "--->Last Category Detected:  %d\n", gagedb->precip_status_info.last_precip_cat );
   RPGC_log_msg( GL_INFO, "--->Current Category Detected:  %d\n", gagedb->precip_status_info.cur_precip_cat );

   /* Write Linear Buffer. */
   buflen = RPGC_data_access_write( GAGEDATA, (char *) buf, len, data_time );

   /* Free memory regardless of outcome of write. */
   free( buf );

   if( buflen <= 0 ){

      RPGC_log_msg( GL_ERROR, "Error encounter on GAGEDATA write. buflen = %d\n",
                    buflen );
      exit( -1 );

   }
   
   return;

} /* End of Write_gagedb(). */


/*\/////////////////////////////////////////////////////////////////////////

   Description:
      Write the Wx Status Message to the General Status LB.

   Inputs:
      area - precipitation area, in km^2.
      wxstatus_deselect - flag to indicate whether the weather mode is 
                          being forced to change.


/////////////////////////////////////////////////////////////////////////\*/
void Write_wx_status( float area, int wxstatus_deselect ){

   int year, month, day, hour, minute, second;
   int buflen;
   Wx_status_t *wxstatus = NULL;

   /* Allocate space for Wx Status message. */
   wxstatus = (Wx_status_t *) calloc( 1, sizeof(Wx_status_t) );

   if( wxstatus == NULL ){

      RPGC_log_msg( GL_ERROR, "malloc Failed for %d bytes\n", sizeof(Wx_status_t) );
      return;

   }

   /* Update Wx Status message. */
   wxstatus->current_wxstatus = Mode_select_status.current_wxstatus;
   wxstatus->current_vcp = Mode_select_status.current_vcp;
   wxstatus->recommended_wxstatus = Mode_select_status.recommended_wxstatus;
   wxstatus->recommended_wxstatus_start_time = 
                         Mode_select_status.recommended_wxstatus_start_time;

   if( wxstatus->recommended_wxstatus == PFWXCONV )
      wxstatus->recommended_wxstatus_default_vcp = 
                           ORPGSITE_get_int_prop( ORPGSITE_DEF_MODE_A_VCP );
   
   else
      wxstatus->recommended_wxstatus_default_vcp = 
                            ORPGSITE_get_int_prop( ORPGSITE_DEF_MODE_B_VCP );

   wxstatus->conflict_start_time = Mode_select_status.conflict_start_time;
   wxstatus->current_wxstatus_time = Mode_select_status.current_wxstatus_time;
   wxstatus->precip_area = area;
   wxstatus->wxstatus_deselect = wxstatus_deselect;
   wxstatus->mode_select_adapt = Newtbl;
   wxstatus->a3052t = Mode_select_status.a3052t;

   /* Tell the operator what the values are. */
   RPGC_log_msg( GL_INFO, "Weather Status Message:\n" );
   if( wxstatus->current_wxstatus == PFWXCONV )
      RPGC_log_msg( GL_INFO, "--->Current Wx Mode:  Precipitation\n" );

   else  if( wxstatus->current_wxstatus == PFWXCLA )
      RPGC_log_msg( GL_INFO, "--->Current Wx Mode:  Clear Air\n" );

   RPGC_log_msg( GL_INFO, "--->Current VCP:  %d\n", wxstatus->current_vcp );
   
   if( wxstatus->recommended_wxstatus == PFWXCONV )
      RPGC_log_msg( GL_INFO, "--->Recommended Wx Mode:  Precipitation\n" );

   else if( wxstatus->recommended_wxstatus == PFWXCLA )
      RPGC_log_msg( GL_INFO, "--->Recommended Wx Mode:  Clear Air\n" );

   if( wxstatus->wxstatus_deselect )
      RPGC_log_msg( GL_INFO, "--->Wx Status Deselect: True\n" );

   if( wxstatus->recommended_wxstatus_start_time > 0 ){

      unix_time( &wxstatus->recommended_wxstatus_start_time, &year, &month, &day, &hour, &minute, &second );
      RPGC_log_msg( GL_INFO, "--->Recommended Wx Mode Start Date/Time: %02d/%02d/%02d %02d:%02d:%02d\n",
                    month, day, year, hour, minute, second );

   }

   RPGC_log_msg( GL_INFO, "--->Recommended Wx Mode Default VCP:  %d\n", 
                 wxstatus->recommended_wxstatus_default_vcp );

   if( wxstatus->conflict_start_time > 0 ){

      unix_time( &wxstatus->conflict_start_time, &year, &month, &day, &hour, &minute, &second );
      RPGC_log_msg( GL_INFO, "--->Conflict Start Date/Time: %02d/%02d/%02d %02d:%02d:%02d\n",
                    month, day, year, hour, minute, second );

   }

   if( wxstatus->current_wxstatus_time > 0 ){

      unix_time( &(wxstatus->current_wxstatus_time), &year, &month, &day, &hour, &minute, &second );
      RPGC_log_msg( GL_INFO, "--->Current Wx Mode Begin Date/Time: %02d/%02d/%02d %02d:%02d:%02d\n",
                    month, day, year, hour, minute, second );

   }

   RPGC_log_msg( GL_INFO, "--->Precipitation Area:  %f km^2\n", wxstatus->precip_area );
   RPGC_log_msg( GL_INFO, "--->Mode Selection Adaptation Data:\n" );
   RPGC_log_msg( GL_INFO, "------>dBZ Threshold: %6.1f  Area Threshold: %6d\n",
                 wxstatus->mode_select_adapt.precip_mode_zthresh, 
                 wxstatus->mode_select_adapt.precip_mode_area_thresh );
   RPGC_log_msg( GL_INFO, "------>Auto Mode A: %2d  Auto Mode B: %2d\n",
                 wxstatus->mode_select_adapt.auto_mode_A, wxstatus->mode_select_adapt.auto_mode_B );
   RPGC_log_msg( GL_INFO, "------>Ignore Mode Conflict?: %6d  Mode Conflict Duration: %6d\n",
                 wxstatus->mode_select_adapt.ignore_mode_conflict,
                 wxstatus->mode_select_adapt.mode_conflict_duration );
   RPGC_log_msg( GL_INFO, "------>Mode B Selection Time: %6d\n",
                 wxstatus->mode_select_adapt.mode_B_selection_time );

   RPGC_log_msg( GL_INFO, "--->Mode Selection A3052t Data:\n" );
   if( wxstatus->a3052t.curr_time > 0 ){

      unix_time( &wxstatus->a3052t.curr_time, &year, &month, &day, &hour, &minute, &second );
      RPGC_log_msg( GL_INFO, "------>Date/Time Mode Selection Ran: %02d/%02d/%02d %02d:%02d:%02d\n",
                    month, day, year, hour, minute, second );

   }
 
   if( wxstatus->a3052t.last_time > 0 ){

      unix_time( &wxstatus->a3052t.last_time, &year, &month, &day, &hour, &minute, &second );
      RPGC_log_msg( GL_INFO, "------>Date/Time Precip Mode Criteria Last Met: %02d/%02d/%02d %02d:%02d:%02d\n",
                    month, day, year, hour, minute, second );

   }

   if( wxstatus->a3052t.time_to_cla > 0 ){

      unix_time( &wxstatus->a3052t.time_to_cla, &year, &month, &day, &hour, &minute, &second );
      RPGC_log_msg( GL_INFO, "------>Date/Time Until Clear Mode Allowed: %02d/%02d/%02d %02d:%02d:%02d\n",
                    month, day, year, hour, minute, second );

   }

   RPGC_log_msg( GL_INFO, "------>Current Category Detected:  %d\n", wxstatus->a3052t.pcpctgry );
   RPGC_log_msg( GL_INFO, "------>Previous Category Detected:  %d\n", wxstatus->a3052t.prectgry );

   /* Write Linear Buffer. */
   buflen = RPGC_data_access_write( ORPGDAT_GSM_DATA, (char *) wxstatus, sizeof(Wx_status_t),
                          WX_STATUS_ID );

   if( buflen < 0 )
      RPGC_log_msg( GL_ERROR, "RPGC_data_access_write( ORPGDAT_GSM_DATA, WX_STASTUS_ID ) Failed (%d)\n",
                    buflen );

   /* Free the buffer. */
   free( wxstatus );

   return;

} /* End of Write_wx_status(). */

/*\/////////////////////////////////////////////////////////////////////////

   Description:
      Read the Wx Status Message from the General Status LB.  This function
      is called only at startup.

/////////////////////////////////////////////////////////////////////////\*/
void Read_wx_status( ){

   int add_time, buflen;
   time_t current_time = time(NULL);
   char *buf = NULL;
   Wx_status_t *wxstatus = NULL;


   /* Read Linear Buffer. */
   buflen = RPGC_data_access_read( ORPGDAT_GSM_DATA, (char *) &buf, LB_ALLOC_BUF, WX_STATUS_ID );

   if( buflen < 0 ){

      RPGC_log_msg( GL_ERROR, "RPGC_data_access_read( ORPGDAT_GSM_DATA, WX_STASTUS_ID ) Failed (%d)\n",
                    buflen );
      return;

   }

   wxstatus = (Wx_status_t *) buf;

   /* Verify time did not go backwards.  If it did, initialize the Wx Status. */
   if( (wxstatus->conflict_start_time != WX_STATUS_UNDEFINED)
                                &&
       (current_time < wxstatus->conflict_start_time) )
      memset( &Mode_select_status, 0, sizeof(Mode_select_status_t) );
    
   else{

      /* Update Wx Status message. */
      Mode_select_status.current_wxstatus = wxstatus->current_wxstatus;
      Mode_select_status.current_vcp = wxstatus->current_vcp;
      Mode_select_status.recommended_wxstatus = wxstatus->recommended_wxstatus;
      Mode_select_status.recommended_wxstatus_start_time = 
                         wxstatus->recommended_wxstatus_start_time;
      Mode_select_status.conflict_start_time = wxstatus->conflict_start_time;
      Mode_select_status.current_wxstatus_time = wxstatus->current_wxstatus_time;
      Mode_select_status.a3052t = wxstatus->a3052t;

   }

   if( Mode_select_status.conflict_start_time == WX_STATUS_UNDEFINED )
      Mode_select_status.conflict_start_time = 0;

   Previous_wxstatus = Mode_select_status.current_wxstatus;

   /* Free the buffer. */
   free( buf );

   /* Check if category at time of shutdown was 1 (Significant Precipitation). */
   if( Mode_select_status.a3052t.pcpctgry == CTGRY1 ){

      /* Set Time & Date of expiration of Precip Category #1 to time precip. 
         was last detected plus deselection interval of time.  If Auto Mode B 
         is set, then set the interval of time to be 0. */
      if( Mode_select.auto_mode_B ==  1 )
         add_time = 0;

      else
         add_time = SEC_IN_MIN * Mode_select.mode_B_selection_time;

      Mode_select_status.a3052t.time_to_cla = Mode_select_status.a3052t.last_time + add_time;

   }
   else{

      /* Initialize category 1 end time to the current time. */
      Mode_select_status.a3052t.time_to_cla = Mode_select_status.a3052t.curr_time;

   }

   Mode_B_selection_time = Mode_select.mode_B_selection_time;   

   RPGC_log_msg( GL_INFO, "Precipitation Status\n" );
   RPGC_log_msg( GL_INFO, "--->Current Category: %d, Previous Category: %d\n",
                 Mode_select_status.a3052t.pcpctgry, Mode_select_status.a3052t.prectgry );

   {
      int month, day, year, hour, minute, second;

      if( Mode_select_status.a3052t.last_time > 0 ){

         unix_time( &Mode_select_status.a3052t.last_time, &year, &month, &day, &hour, &minute, &second );
         RPGC_log_msg( GL_INFO, "--->Last Date/Time Precipitation Detected: %02d/%02d/%4d - %02d:%02d:%02d\n",
                       month, day, year, hour, minute, second );

      }

      if( Mode_select_status.a3052t.time_to_cla > 0 ){

         unix_time( &Mode_select_status.a3052t.time_to_cla, &year, &month, &day, &hour, &minute, &second );
         RPGC_log_msg( GL_INFO, "--->Last Date/Time Precipitation Category 1 Detected: %02d/%02d/%4d - %02d:%02d:%02d\n",
                       month, day, year, hour, minute, second );

      }

   }

   return;

} /* End of Read_wx_status(). */
