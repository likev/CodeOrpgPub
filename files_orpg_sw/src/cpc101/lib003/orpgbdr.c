/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2012/06/07 17:43:33 $
 * $Id: orpgbdr.c,v 1.32 2012/06/07 17:43:33 ccalvert Exp $
 * $Revision: 1.32 $
 * $State: Exp $
 */
#include <orpgbdr.h>

#define ORPGBDR_ACCT_READ         1
#define ORPGBDR_ACCT_WRITE        1
#define ORPGBDR_DATA_READ         2
#define ORPGBDR_DATA_WRITE        2
#define ORPGBDR_ADAPT_READ        3
#define ORPGBDR_ADAPT_WRITE       3

/* Static Globals */
static int Read_RDA_RDACNT = 0;
static RDA_rdacnt_t Rda_rdacnt;
static Radial_replay_t Radial_list[MAXN_TYPES];
static int N_types = 0;
static unsigned char Alarm = 0;

static int Last_read_acct_status = 0;
static int Last_read_data_status = 0;
static int Last_read_adapt_status = 0;
static int Last_write_acct_status = 0;
static int Last_write_data_status = 0;

/* Function prototypes. */
static int Read_accounting_data( int data_type, int vol_seq_num,
                                 Replay_volume_t *vol_data );
static int Check_data_type_registered( int data_type );
static int Valid_accounting_data( Replay_volume_t *vol_data,
                                  int data_type, int sub_type, 
                                  int vol_seq_num, int elev_ind );
static int Valid_volume_num( int ind, LB_id_t msg_id, int vol_seq_num );
static void Process_read_status( int read_returned, int what_read );
static void Process_write_status( int write_returned, int what_written );
static int Validate_indices( int acct_ind, int num_radials );
static void Write_informational_msg( int ind, int vol_scan_num );
static int Get_num_elevations( int vcp_num, int vol_scan_num );
static int Get_rpg_elevation_num( int vcp_num, int vol_scan_num, int elev_ind );
static int Get_waveform( int vcp_num, int vol_scan_num,
                         int elev_ind );
static int Read_rda_rdacnt();
static void Notify_callback( int fd, LB_id_t msgid, int msg_info, void *arg );


/********************************************************************

   Description:
      Registration module for radial replay types.  This module 
      must be called before any process can store or access 
      radial replay data.

   Inputs:
      data_type - product ID of data to register. (e.g., BASEDATA,
                  RAWDATA).
      data_lb_id - LB data ID where data is stored.
      acct_data_lb_id - LB data ID where accounting data is stored.

   Outputs:

   Returns:
      0 on success, or a negative error return value.

   Notes: 

********************************************************************/
int ORPGBDR_reg_radial_replay_type( int data_type, int data_lb_id,
                                    int acct_data_lb_id ){

   int class_id = data_type, ret, i;
 
   /* Get the class ID for this data type.  On error, we assume
      the class ID is the same as the data ID. */
   class_id = ORPGPAT_get_class_id( data_type );

   /* Check if data type already registered. */
   for( i = 0; i < N_types; i++ ){

      /* The data type can match or the class ID can match. */
      if( (Radial_list[i].data_id == data_type)
                      ||
          (Radial_list[i].class_id == class_id) )
         return (0);

   }

   /* Check if too many types already registered. */
   if( N_types >= MAXN_TYPES ){

      LE_send_msg( GL_INFO, "More Than %d Radial Types Registered\n",
                   MAXN_TYPES );
      return (ORPGBDR_ERROR);

   }
    
   /* Register the data */
   Radial_list[N_types].data_id = data_type;
   Radial_list[N_types].class_id = class_id;
   Radial_list[N_types].vol_data = NULL;

   Radial_list[N_types].LB_id_data = data_lb_id;
   Radial_list[N_types].LB_id_acct_data = acct_data_lb_id;

   N_types++;

   /* Open ORPGDAT_ADAPTATION for write permission. */
   ORPGDA_write_permission( ORPGDAT_ADAPTATION );

   /* Register for updates. */
   if( (ret = ORPGDA_UN_register( ORPGDAT_ADAPTATION, RDA_RDACNT,
                                  Notify_callback )) < 0 ){
      LE_send_msg(GL_ERROR, 
            "LB Notification Registration Failed for RDA_RDACNT: %d\n", ret );

      return (ORPGBDR_ERROR);

   }

   /* Return success. */
   return (0);

/* End of ORPGBDR_reg_radial_replay_type() */
}

/********************************************************************

   Description:
      This function write the radial replay data to its data store
      and also maintains accounting information for the data in the
      data store.

   Inputs:
      data_type - product ID of data to store.
      sub_type - data sub-type.
      ptr - pointer to buffer containint data to store.
      len - length, in bytes, of data to store.

   Outputs:

   Returns:
      Returns 0 on success, or negative error value.

   Notes:

********************************************************************/
int ORPGBDR_write_radial( int data_type, int sub_type, void *ptr, int len ){

   Base_data_header *hdr = (Base_data_header *) ptr;
   int vol_scan_num, radial_vol_scan_num, radial_status;
   int num_elevs, ret, ind, include_cd_cut, i, j;
   LB_id_t msg_id = 0;

   static int num_radials = 0;
   static int acct_ind = -1;
   static int last_elev_num = -1;

   /* Check if data already registered. */
   if( (ind = Check_data_type_registered( data_type )) < 0 )
      return (ind);

   /* Get the radial status of this radial. */
   if( hdr == NULL ){

      LE_send_msg( GL_ERROR, "Invalid buffer pointer in ORPGBDR_write_radial\n" );
      return( ORPGBDR_ERROR );

   }

   radial_status = hdr->status;
   radial_vol_scan_num = hdr->volume_scan_num;
   if( Radial_list[ind].vol_data != NULL )
      vol_scan_num = ORPGMISC_vol_scan_num( Radial_list[ind].vol_data->vol_seq_num );
   else
      vol_scan_num = -1;

   /* Process good beginning of volume or unexpected change in volume. */
   if( ((radial_status == GOODBVOL) && (hdr->elev_num == 1))
                   ||
       ((vol_scan_num >= 0) && (vol_scan_num != radial_vol_scan_num)) ){

      if( !((radial_status == GOODBVOL) && (hdr->elev_num == 1)) )
         LE_send_msg( GL_ERROR, "Accounting Data Written Owing To Unexpected Volume Scan # Change (%d, %d)\n",
                      vol_scan_num, radial_vol_scan_num );
    
      /* If previous volume accounting data not written, write it now. */
      if( Radial_list[ind].vol_data != NULL ){

         /* Unexpected start of volume encountered.  Write volume
            accounting data.  Free memory associated with volume
            data and NULL pointer. */
         ret = ORPGDA_write( Radial_list[ind].LB_id_acct_data, 
                             (char *) Radial_list[ind].vol_data, 
                             sizeof( Replay_volume_t ), 
                             vol_scan_num );

         /* Process write returned status. */
         Process_write_status( ret, ORPGBDR_ACCT_WRITE );

         /* Write out informational message to log file */
         Write_informational_msg( ind, vol_scan_num );

         free( Radial_list[ind].vol_data );
         Radial_list[ind].vol_data = NULL;

         /* Write failed.  Return error. */
         if( ret < 0 ){

            LE_send_msg( GL_ERROR, "Accounting Data Write Failed\n" );
            return( ORPGBDR_WRITE_FAILED );
 
         }

      }

      /* Initialize the accounting elevation index. */
      acct_ind = -1;

      /* Allocate space for new volume scan data and initialize 
         volume scan dependent data. */
      Radial_list[ind].vol_data = 
             (Replay_volume_t *) calloc( 1, sizeof( Replay_volume_t ) );
      if( Radial_list[ind].vol_data == NULL ){

         LE_send_msg( GL_ERROR, "calloc Failed for %d Bytes\n", sizeof( Replay_volume_t ) );
         return( ORPGBDR_ERROR );

      }

      Radial_list[ind].vol_data->vol_start_time = hdr->begin_vol_time / 1000;
      Radial_list[ind].vol_data->vol_start_date = hdr->date;

      Radial_list[ind].vol_data->vol_seq_num = 
         ORPGMISC_vol_seq_num( (int) hdr->vol_num_quotient, 
                               (int) hdr->volume_scan_num );
      Radial_list[ind].vol_data->vol_cov_pat = hdr->vcp_num;

      /* Initialize the elevation message data. */
      for( i = 0; i < MAX_ELEVS; i++ ){

         Radial_list[ind].vol_data->elevs[i].rpg_elev_ind = -1;
         Radial_list[ind].vol_data->elevs[i].sub_type = 0;

         for( j = 0; j < MAX_SR_RADIALS; j++ )
            Radial_list[ind].vol_data->elevs[i].radial_msgid[j] = 0;

      }

      /* Determine the number of elevation cuts for this VCP. */
      num_elevs = Get_num_elevations( (int) hdr->vcp_num,
                                      (int) hdr->volume_scan_num );
      if( num_elevs < 0 )
         Radial_list[ind].vol_data->num_elev_cuts = -1;

      else{

         int waveform, cut_no, count = 0;
         int prev_cut_no, prev_waveform = 0;
         
         i = 0;
         prev_cut_no = Get_rpg_elevation_num( (int) hdr->vcp_num, 
                                              (int) hdr->volume_scan_num, i ); 
         prev_waveform = Get_waveform( (int) hdr->vcp_num, 
                                       (int) hdr->volume_scan_num, i ); 

         if( prev_cut_no >= 0 )
            count++;

         i = 1;
         while( i < num_elevs ){

            /* Find the RPG elevation number of the next cut in the VCP.
               If more than one cut have the same elevation number, count
               the CD cut following a CS cut.  Don't count cuts following
               B cuts.

               ASSUMPTION:  For Normal split cuts, a CS cut is followed
                            by a CD/W cut.   Count these as 2 cuts.  

                            For the currently defined MPDA VCPs, we have 1 
                            of 2 possibilities:

                            1) A CS cut, followed by either multiple CD/W.

                            2) A B cut followed by multiple CD/WO cuts. 

            */
            cut_no = Get_rpg_elevation_num( (int) hdr->vcp_num, 
                                            (int) hdr->volume_scan_num, i ); 
            waveform = Get_waveform( (int) hdr->vcp_num, 
                                     (int) hdr->volume_scan_num, i ); 
            include_cd_cut = 1;

            while( prev_cut_no == cut_no ){

               /* Include the Doppler cut only once. */
               if( include_cd_cut ){

                  /* Handle the lower MPDA and split cuts. */
                  if( (prev_waveform == VCP_WAVEFORM_CS)
                                     &&
                      (waveform == VCP_WAVEFORM_CD) )
                     count++;

                  /* For the upper MPDA cuts which lead with a B cut,
                     only count 1 cut. */

                  include_cd_cut = 0;

               }

               i++;
               cut_no = Get_rpg_elevation_num( (int) hdr->vcp_num, 
                                               (int) hdr->volume_scan_num, i );
               waveform = Get_waveform( (int) hdr->vcp_num, 
                                        (int) hdr->volume_scan_num, i );

            }

            if( prev_cut_no != cut_no ){

               count++;

               /* Increment i (Go to Next Cut) */
               i++;

               prev_cut_no = cut_no;
               cut_no = Get_rpg_elevation_num( (int) hdr->vcp_num, 
                                               (int) hdr->volume_scan_num, i );

               prev_waveform = waveform;
               waveform = Get_waveform( (int) hdr->vcp_num, 
                                        (int) hdr->volume_scan_num, i );

            }

         }

         Radial_list[ind].vol_data->num_elev_cuts = count; 

      }

   }

   /* Check whether buffer for accounting data has been allocated.  If not, 
      the process which writes the data was activated sometime in the middle
      of a volume scan.  In this case, just return.  This buffer will be allocated
      at the start of the next volume scan, in which case we can proceed with
      storing data to support replay. */
   if( Radial_list[ind].vol_data == NULL )
      return(0);

   /* Write the radial to data store. */
   ret = ORPGDA_write( Radial_list[ind].LB_id_data, (char *) ptr, len, LB_ANY );
   msg_id = ORPGDA_get_msg_id();

   /* Process write returned status. */
   Process_write_status( ret, ORPGBDR_DATA_WRITE );
   if( ret < 0 ){

      LE_send_msg( GL_ERROR, "Radial Write Failed\n" );
      return( ORPGBDR_WRITE_FAILED );

   }

   /* Update volume accounting data if necessary. */
   if( (radial_status == GOODBEL) || (radial_status == GOODBVOL) ){

      /* Increment the accounting elevation index on new elevation. */
      if( (hdr->elev_num != last_elev_num) || (radial_status == GOODBVOL) )
         acct_ind++;

      last_elev_num = hdr->elev_num;

      /* Initialize the radial counter to 1 for start of elevation/volume. */
      num_radials = 1;

      /* Validate the Radial_list indices. */
      if( Validate_indices( acct_ind, num_radials ) < 0 )
         return( ORPGBDR_ERROR );

      /* Set the start of elevation message ID. */
      Radial_list[ind].vol_data->elevs[acct_ind].radial_msgid[0] = msg_id;

   }else if( (radial_status == GENDEL) || (radial_status == GENDVOL) ){

      int vol_scan_num;

      /* Increment the radial counter. */
      num_radials++;
     
      /* Validate the Radial_list indices. */
      if( Validate_indices( acct_ind, num_radials ) < 0 )
         return( ORPGBDR_ERROR );

      /* Store all pertinent information about this elevation cut. */
      Radial_list[ind].vol_data->elevs[acct_ind].num_radials = num_radials;
      Radial_list[ind].vol_data->elevs[acct_ind].rpg_elev_ind = hdr->rpg_elev_ind;
      Radial_list[ind].vol_data->elevs[acct_ind].sub_type = sub_type;
      Radial_list[ind].vol_data->elevs[acct_ind].radial_msgid[num_radials-1] = msg_id;
      Radial_list[ind].vol_data->num_completed_cuts++;

      /* Check if this is the last elevation of the volume scan.  For AVSET,
         the actual number of cuts may be less than expected by the VCP definition.  
         If so, reset the number of cuts to the number of completed cuts. */
      if( (hdr->last_ele_flag) && (radial_status == GENDVOL) ){

         if( Radial_list[ind].vol_data->num_completed_cuts != 
                              Radial_list[ind].vol_data->num_elev_cuts ){

            /* Write out informational message. */
            LE_send_msg( GL_INFO, "Last Elev Flag Set ... # Cuts (%d) != # Completed (%d)\n",
                         Radial_list[ind].vol_data->num_elev_cuts,
                         Radial_list[ind].vol_data->num_completed_cuts ); 
            Radial_list[ind].vol_data->num_elev_cuts = 
                             Radial_list[ind].vol_data->num_completed_cuts; 

         }

      }

      /* Write the volume accounting data. */
      vol_scan_num = 
         ORPGMISC_vol_scan_num( Radial_list[ind].vol_data->vol_seq_num );
      ret = ORPGDA_write( Radial_list[ind].LB_id_acct_data, 
                          (char *) Radial_list[ind].vol_data, 
                          sizeof( Replay_volume_t ), 
                          vol_scan_num );

      /* If error on accounting data write, report it and return error. */
      Process_write_status( ret, ORPGBDR_ACCT_WRITE );

      if( radial_status == GENDVOL ){

         /* Write out informational message to log file */
         Write_informational_msg( ind, vol_scan_num );

         free( Radial_list[ind].vol_data );
         Radial_list[ind].vol_data = NULL;

         /* Initialize the accounting elevation index. */
         acct_ind = -1;

      }

      if( ret < 0 ){

         LE_send_msg( GL_ERROR, "Accounting Data Write Failed\n" );
         return( ORPGBDR_WRITE_FAILED );

      }

   }
   else{

      /* Increment the radial counter. */
      num_radials++;

      /* Validate the Radial_list indices. */
      if( Validate_indices( acct_ind, num_radials ) < 0 )
         return( ORPGBDR_ERROR );

      /* Get the intermediate radial message ID. */
      Radial_list[ind].vol_data->elevs[acct_ind].radial_msgid[num_radials-1] = msg_id;

   }

   return (0);

/* End of ORPGBDR_write_radial() */
}

/********************************************************************

   Description:
      Returns the LB message ID of the start of the specified 
      elevation scan radial  for the specified volume scan number.

   Inputs:
      data_type - product ID of desired data.
      sub_type - sub-type of data.
      vol_seq_num - volume scan sequence number.
      rpg_elev_ind - desired elevation scan index

   Outputs:

   Returns:
      Returns LB message ID for start of elevation radial or 
      0 on error.
      
********************************************************************/
LB_id_t ORPGBDR_get_start_of_elevation_msgid( int data_type, 
                                              int sub_type,
                                              int vol_seq_num, 
                                              int rpg_elev_ind ){

   Replay_volume_t vol_data;
   int ind;

   /* If the elevation index is not positive, it is invalid. */
   if( rpg_elev_ind <= 0 )
      return (0);

   /* Validate the accounting data. */
   if( (ind = Valid_accounting_data( &vol_data, data_type, sub_type,
                                     vol_seq_num, rpg_elev_ind )) < 0 )
      return (0);

   /* Return message ID for start of elevation radial. */
   return (vol_data.elevs[ind].radial_msgid[0]);

/* End of ORPGBDR_get_start_of_elevation_msgid() */
}

/********************************************************************

   Description:
      Returns the LB message ID of the end of the specified 
      elevation scan radial  for the specified volume scan number.

   Inputs:
      data_type - product ID of desired data.
      sub_type - data sub-type.
      vol_seq_num - volume scan sequence number.
      rpg_elev_ind - desired elevation scan index

   Outputs:

   Returns:
      Returns LB message ID for end of elevation radial or 
      0 on error.
      
********************************************************************/
LB_id_t ORPGBDR_get_end_of_elevation_msgid( int data_type, 
                                            int sub_type,
                                            int vol_seq_num, 
                                            int rpg_elev_ind ){

   Replay_volume_t vol_data;
   int num_radials, ind;

   /* If the elevation index is not positive, it is invalid. */
   if( rpg_elev_ind <= 0 )
      return (0);

   /* Validate the accounting data. */
   if( (ind = Valid_accounting_data( &vol_data, data_type, sub_type,
                                     vol_seq_num, rpg_elev_ind )) < 0 )
      return (0);

   /* Return message ID for end of elevation radial. */
   num_radials = vol_data.elevs[ind].num_radials;
   return (vol_data.elevs[ind].radial_msgid[num_radials-1]);

/* End of ORPGBDR_get_end_of_elevation_msgid() */
}

/*****************************************************************

   Description:
      Returns the LB message ID of the start of the specified 
      volume scan radial for the specified volume scan number.

   Inputs:
      data_type - product ID of desired data.
      sub_type - data sub-type.
      vol_seq_num - volume scan sequence number.

   Outputs:

   Returns:
      Returns LB message ID for start of volume radial or 0 
      on error.

*****************************************************************/
LB_id_t ORPGBDR_get_start_of_volume_msgid( int data_type, 
                                           int sub_type,
                                           int vol_seq_num ){

   return( ORPGBDR_get_start_of_elevation_msgid( data_type, 
                                                 sub_type,
                                                 vol_seq_num,
                                                 (int) 1 ) );

/* End of ORPGBDR_get_start_of_volume_msgid() */
}

/*****************************************************************

   Description:
      Returns the LB message ID of the end of the specified 
      volume scan radial for the specified volume scan number.

   Inputs:
      data_type - product ID of desired data.
      sub_type - data sub-type.
      vol_seq_num - volume scan sequence number.

   Outputs:

   Returns:
      Returns LB message ID for end of volume radial or 0 on error.

*****************************************************************/
LB_id_t ORPGBDR_get_end_of_volume_msgid( int data_type, 
                                         int sub_type,
                                         int vol_seq_num ){

   int ind;
   Replay_volume_t vol_data;

   /* Read the accounting data. */
   ind = Read_accounting_data( data_type, vol_seq_num, &vol_data );

   if( ind < 0 )
      return (0);

   return( ORPGBDR_get_end_of_elevation_msgid( data_type, sub_type, vol_seq_num,
                           vol_data.elevs[vol_data.num_completed_cuts-1].rpg_elev_ind));

/* End of ORPGBDR_get_end_of_volume_msgid() */
}

/*****************************************************************

   Description:
      Returns the number of completed elevation cuts for the
      specified volume scan.

   Inputs:
      data_type - product ID.
      vol_seq_num - volume sequence number.

   Outputs:

   Returns:
      Returns number of completed cuts for the specified volume
      scan number or negative number on error.

   Notes:

******************************************************************/
int ORPGBDR_get_num_completed_cuts( int data_type, int vol_seq_num ){

   int ind;
   Replay_volume_t vol_data;

   /* Read the accounting data. */
   ind = Read_accounting_data( data_type, vol_seq_num, &vol_data );

   if( ind < 0 )
      return (ORPGBDR_DATA_NOT_FOUND);

   if( vol_seq_num != vol_data.vol_seq_num )
      return (ORPGBDR_DATA_NOT_FOUND);

    return (vol_data.num_completed_cuts);
   
/* End of ORPGBDR_get_num_completed_cuts(). */
} 

/*****************************************************************

   Description:
      Returns the number of elevation cuts for the specified 
      volume scan.

   Inputs:
      data_type - product ID.
      vol_seq_num - volume sequence number.

   Outputs:

   Returns:
      Returns number of elevation cuts for the specified volume
      scan number or negative number on error.

   Notes:

******************************************************************/
int ORPGBDR_get_num_elevation_cuts( int data_type, int vol_seq_num ){

   int ind;
   Replay_volume_t vol_data;

   /* Read the accounting data. */
   ind = Read_accounting_data( data_type, vol_seq_num, &vol_data );
   if( ind < 0 )
      return (ORPGBDR_DATA_NOT_FOUND);

   if( vol_seq_num != vol_data.vol_seq_num )
      return (ORPGBDR_DATA_NOT_FOUND);

   return (vol_data.num_elev_cuts);
   
/* End of ORPGBDR_get_num_elevation_cuts(). */
}
 
/*****************************************************************

   Description:
      Returns the start date and time for the specified volume 
      scan.

   Inputs:
      data_type - product ID
      vol_seq_num - volume sequence number
      start_date - pointer to int to hold volume start date (Modified
                   Julian).
      start_time - pointer to time_t to hold volume start time
                   (secs since midnight).

   Outputs:
      start_date - pointer to int receiving volume start date (Modified
                   Julian).
      start_time - pointer to time_t receiving volume start time
                   (secs since midnight).
      

   Returns:
      Returns 0 on success scan number or negative number on error.

   Notes:

******************************************************************/
int ORPGBDR_get_start_date_and_time( int data_type, int vol_seq_num,
                                     int *start_date, time_t *start_time ){

   int ind;
   Replay_volume_t vol_data;

   /* Read the accounting data. */
   ind = Read_accounting_data( data_type, vol_seq_num, &vol_data );
   if( ind < 0 )
      return (ORPGBDR_DATA_NOT_FOUND);

   if( vol_seq_num != vol_data.vol_seq_num )
      return (ORPGBDR_DATA_NOT_FOUND);

    *start_date = (int) vol_data.vol_start_date;
    *start_time = (time_t) vol_data.vol_start_time;

    return (0);
   
/* End of ORPGBDR_get_start_date_and_time(). */
}

/********************************************************************

   Description:
      Determines whether all accounting data for specfied volume
      is available.

   Inputs:
      data_type - product ID
      sub_type - data sub-type
      vol_seq_num - volume sequence number of volume scan of interest.

   Outputs:

   Returns:
      LB message ID of start of volume on success, or 0 if data
      not found.

   Notes;

********************************************************************/
LB_id_t ORPGBDR_check_complete_volume( int data_type, int sub_type, 
                                       int vol_seq_num ){

   int ind;
   Replay_volume_t vol_data;
   LB_id_t start_msgid, end_msgid;

   /* Read the accounting data. */
   ind = Read_accounting_data( data_type, vol_seq_num, &vol_data );
   if( ind < 0 )
      return (0);

   /* Validate the accounting information read against input parameters.
      The following must match:

         - Volume scan sequence number
         - The number of elevation cuts must match the number of 
           completed elevation cuts.
   */
   if( (vol_seq_num != vol_data.vol_seq_num) 
               ||
       (vol_data.num_completed_cuts != vol_data.num_elev_cuts) )
      return (0);

   /* Get the starting message ID for the desired volume scan. */
   start_msgid = ORPGBDR_get_start_of_volume_msgid( data_type, sub_type, vol_seq_num );
   if( start_msgid == 0 )
      return (0);

   /* Validate the volume scan number. */
   if( !Valid_volume_num( ind, start_msgid, vol_seq_num ) )
      return (0);

   /* Get the ending message ID for the desired volume scan. */
   end_msgid = ORPGBDR_get_end_of_volume_msgid( data_type, sub_type, vol_seq_num );
   if( end_msgid == 0 )
      return (0);

   /* Validate the volume scan number. */
   if( !Valid_volume_num( ind, end_msgid, vol_seq_num ) )
      return (0);

   /* Return the starting message ID. */
   return( start_msgid ); 
   
/* ORPGBDR_check_complete_volume() */
} 

/********************************************************************

   Description:
      Determines whether all accounting data for specfied volume
      and elevation is available.

   Inputs:
      data_type - product ID
      sub_type - data sub-type
      vol_seq_num - volume sequence number of volume scan of interest.
      elev_ind - rpg elevation index of interest.

   Outputs:

   Returns:
      LB message ID of start of elevation index on success, 
      or 0 is data not found.

   Notes;

********************************************************************/
LB_id_t ORPGBDR_check_complete_elevation( int data_type, 
                                          int sub_type,
                                          int vol_seq_num, 
                                          int elev_ind ){

   int ind;
   Replay_volume_t vol_data;
   LB_id_t start_msgid, end_msgid;

   /* Read the accounting data. */
   ind = Read_accounting_data( data_type, vol_seq_num, &vol_data );
   if( ind < 0 )
      return (0);

   /* Validate the accounting information read against input parameters.
      The following must match:

         - Volume scan sequence number
         - The number of completed elevation cuts must be greater
           than or equal to the elev_ind input.
   */
   if( (vol_seq_num != vol_data.vol_seq_num) 
               ||
       (vol_data.elevs[vol_data.num_completed_cuts-1].rpg_elev_ind < elev_ind) )
      return (0);

   /* Get the starting message ID for the desired volume scan. */
   start_msgid = ORPGBDR_get_start_of_elevation_msgid( data_type, 
                                                       sub_type,
                                                       vol_seq_num,
                                                       elev_ind );
   if( start_msgid == 0 )
      return (0);

   /* Validate the volume scan number. */
   if( !Valid_volume_num( ind, start_msgid, vol_seq_num ) )
      return (0);

   /* Get the starting message ID for the desired volume scan. */
   end_msgid = ORPGBDR_get_end_of_elevation_msgid( data_type, 
                                                   sub_type,
                                                   vol_seq_num,
                                                   elev_ind );
   if( end_msgid == 0 )
      return (0);

   /* Validate the volume scan sequence number. */
   if( !Valid_volume_num( ind, end_msgid, vol_seq_num ) )
      return (0);

   /* Return the starting message ID. */
   return( start_msgid ); 

/* End of ORPGBDR_check_complete_elevation. */   
}
 
/*****************************************************************
   
   Description:
      This function sets the LB read pointer to "msg_id" for the
      data LB identified by "data_type"

   Inputs:
      data_type - data ID for which the read pointer is to be set.
      msg_id - message ID to position the LB (i.e., read pointer).
      offset - offset value relative to msg_id.

   Outputs:

   Returns:
      ORPGBDR_INVALID_DATATYPE if "data_type" was not previously
      registered, ORPGBDR_DATA_NOT_FOUND if the ORPGDA_seek fails.
      Otherwise, 0 is returned on success.

   Notes:

*****************************************************************/
int ORPGBDR_set_read_pointer( int data_type, LB_id_t msg_id, 
                              int offset ){

   int ret, ind;

   /* Locate the data type in the list of registered data types.
      Return error if not registered. */
   if( (ind = Check_data_type_registered( data_type )) < 0 )
      return (ind);

   /* Position the LB read pointer to "msg_id". */
   ret = ORPGDA_seek( Radial_list[ind].LB_id_data, offset, msg_id,
                      NULL );

   /* Did seek fail? */
   if( ret < 0 )
      return (ORPGBDR_DATA_NOT_FOUND );

   /* Seek successful. */
   return (0);

/* End of ORPGBDR_set_read_pointer() */
}

/*****************************************************************
   Description:
      Interface routine for ORPGDA_read.

   Inputs:
      data_type - data ID of data to read.
      buf - pointer to buffer to receive data read.
      buflen - length of data, in bytes, to read.
      msg_id - LB message ID of data to read.

   Outputs:
      buf - on success, receives data read.

   Returns:
      Negative value on error, or number of bytes read on success.

   Notes:
      
*****************************************************************/
int ORPGBDR_read_radial( int data_type, void *buf, int buflen,
                         LB_id_t msg_id ){

   int ind, ret;

   /* Locate the data type in the list of registered data types.
      Return error if not registered. */
   if( (ind = Check_data_type_registered( data_type )) < 0 )
      return (ind);

   ret = ORPGDA_read( Radial_list[ind].LB_id_data, buf,
                      buflen, msg_id );

   /* Process read returned value. */
   Process_read_status( ret, ORPGBDR_DATA_READ );
   return(ret);

/* End of ORPGBDR_read_radial() */
}

/*****************************************************************
   Description:
      Interface routine for ORPGDA_direct.

   Inputs:
      data_type - data ID of data to read.
      buf - pointer to buffer to receive data read.
      msg_id - LB message ID of data to read.

   Outputs:
      buf - on success, receives data read.

   Returns:
      Negative value on error, or number of bytes read on success.

   Notes:
      
*****************************************************************/
int ORPGBDR_read_radial_direct( int data_type, void *buf, 
                                LB_id_t msg_id ){

   int ind, ret;

   /* Locate the data type in the list of registered data types.
      Return error if not registered. */
   if( (ind = Check_data_type_registered( data_type )) < 0 )
      return (ind);

   ret = ORPGDA_direct( Radial_list[ind].LB_id_data, buf, msg_id );

   /* Process read returned value. */
   if( ret != LB_BAD_ACCESS && ret != RSS_NOT_SUPPORTED ) 
      Process_read_status( ret, ORPGBDR_DATA_READ );

   return(ret);

/* End of ORPGBDR_read_radial_direct() */
}

/*****************************************************************

   Description:
      Reads radial replay accounting data for specified volume
      scan.

   Inputs:
      data_type - product ID.
      vol_seq_num - volume scan sequence number for which 
                    accounting data is desired.
      vol_data - pointer to structure to receive accounting
                 data.

   Outputs:
      vol_data - pointer to structure containing accounting
                 data.

   Returns:
      The index into Radial_list on success or negative value 
      on error.

   Notes:
      On error, no accounting data is provided.

*****************************************************************/
static int Read_accounting_data( int data_type, int vol_seq_num,
                                 Replay_volume_t *vol_data ){

   int ret, ind;
   int vol_scan_num;

   /* Validate the volume scan number.   This number must be
      positive. */
   if( vol_seq_num <= 0 )
      return (ORPGBDR_DATA_NOT_FOUND);

   /* Locate the data type in the list of registered data types.
      Return error if not registered. */
   for( ind = 0; ind < N_types; ind++ ){

      if( Radial_list[ind].data_id == data_type )
         break;

   }

   if( ind >= N_types ){

      LE_send_msg( GL_ERROR, "Attempting To Read Unknown Accounting Type\n" );
      return (ORPGBDR_INVALID_DATATYPE);

   }

   /* The volume scan number can either be volume scan number in the
      range 1-80 or can be the volume scan sequence number. */
   vol_scan_num = ORPGMISC_vol_scan_num( vol_seq_num );
   ret = ORPGDA_read( Radial_list[ind].LB_id_acct_data, (char *) vol_data,
                      sizeof( Replay_volume_t ), vol_scan_num );

   /* Check if read succeeded.  Return failure on error. */
   if( ret < 0 )
      return (ORPGBDR_DATA_NOT_FOUND);

   return (ind);

/* End of Read_accounting_data() */
}

/************************************************************************
   Description:
      Checks if the specified data type has been previously registered.

   Inputs:
      data_type - data ID of data to check.

   Outputs:

   Returns:
      ORPGBDR_INVALID_DATATYPE if data not registered, otherwise 
      index of data in Radial_list.

   Notes:

************************************************************************/
static int Check_data_type_registered( int data_type ){

   int ind;

   /* Locate the data type in the list of registered data types.
      Return error if not registered. */
   for( ind = 0; ind < N_types; ind++ ){

      /* Check for match on data type. */
      if( Radial_list[ind].data_id == data_type )
         return (ind);

   }

   /* If here, data was not previously registered. */
   if( ind >= N_types ){

      LE_send_msg( GL_ERROR, "Datatype (%d) Not Previously Registered\n",
                   data_type );
      return( ORPGBDR_INVALID_DATATYPE );

   }

   return( ORPGBDR_INVALID_DATATYPE );

/* End of Check_data_type_registered() */
}

/**********************************************************************

   Description:
      Reads accounting data for specified volume number.  If read is
      successful, checks to make sure accounting data is valid.  Valid
      accounting data ensures the volume scan number matches what was
      requested, and the number of elevations and completed elevations
      is greater than or equal to the requested elevation index.

   Inputs:
      vol_data - pointer to Replay_vol_t structure to receive the
                 accounting data.
      data_type - product ID.
      sub_type - data sub-type.
      vol_seq_num  - volume sequence number for which accounting data 
                     is desired.
      rpg_elev_ind - elevation index for which accounting data is 
                     desired.

   Outputs:
      vol_data - contains the accounting data for the specified
                 volume sequence number.

   Returns:
      Returns negative number on error, or elevation index.

   Notes:

**********************************************************************/
static int Valid_accounting_data( Replay_volume_t *vol_data, int data_type, 
                                  int sub_type, int vol_seq_num, 
                                  int rpg_elev_ind ){

   int ind;
   
   /* Read the accounting data. */
   ind = Read_accounting_data( data_type, vol_seq_num, vol_data );
   if( ind < 0 )
      return (-1);

   /* Validate the accounting information read against input parameters.
      The following must match:

         - Volume scan sequence number
         - Elevation index must be less than or equal to number of 
           elevation cuts for this volume scan.
         - Elevation index must be less than or equal to the number
           of cuts for which there is accounting data.
   */
   if( (vol_seq_num != vol_data->vol_seq_num) 
               ||
       (rpg_elev_ind > vol_data->num_elev_cuts) 
               || 
       (rpg_elev_ind > vol_data->num_completed_cuts) )
      return (-1);

   /* Check for match on RPG elevation index and sub-type. */
   for( ind = 0; ind < vol_data->num_completed_cuts; ind++ ){

      if( (vol_data->elevs[ind].rpg_elev_ind == rpg_elev_ind)
                            &&
          (vol_data->elevs[ind].sub_type & sub_type) )
         return(ind);
   }

   /* Return invalid elevation index. */
   return (-1);

/* End of Valid_accounting_data() */
}

/********************************************************************
   Description:
      Reads the radial at "msg_id" within datastore 
      "Radial_list[ind].LB_id_data" and validates the volume scan
      number in the radial against "vol_seq_num".

   Inputs:
      ind - index into Radial_list for this data type.
      msg_id - LB message ID to read.
      vol_seq_num - volume sequence number to compare with radial value.

   Outputs:

   Returns:
      0 is volume scan numbers do not match, or 1 if they match.

   Notes:

********************************************************************/
static int Valid_volume_num( int ind, LB_id_t msg_id, int vol_seq_num ){

   char *buf;
   int ret;

   /* Read the radial. */
   ret = ORPGDA_read( Radial_list[ind].LB_id_data, &buf, 
                      LB_ALLOC_BUF, msg_id );
   if( ret > 0 ){

      Base_data_header *hdr = (Base_data_header *) buf;
      int volume_number = 
         ORPGMISC_vol_seq_num( (int) hdr->vol_num_quotient, 
                               (int) hdr->volume_scan_num );

      free( buf );

      /* Validate the volume sequence number in the radial header. */
      if( volume_number != vol_seq_num ) 
         return (0);

      /* Volume sequence number match! */
      return (1);

   }
   else{

      LE_send_msg( GL_ERROR, "ORPGDA_read Failed (%d)\n", ret );
      return (0);

   }

/* End of Valid_volume_num() */
}

/********************************************************************
   Description:
      Process the base data read status.  Sets/Clears Base Data Disk
      Failure depending on read status.

   Input:
      read_returned - value returned from ORPGDA_read.
      what_read - the LB which was read.

   Outputs:

   Notes:

*********************************************************************/
static void Process_read_status( int read_returned, int what_read ){

   /* Get status of the Base Data Disk Failure Alarm. */
   if( ORPGINFO_statefl_rpg_alarm( ORPGINFO_STATEFL_RPGALRM_DBFL,
                                   ORPGINFO_STATEFL_GET, &Alarm ) ){

      LE_send_msg( GL_ERROR, "Unable To GET Data Base Failure State\n" );
      return;

   }

   /* On normal read, clear Base Data Disk Failure alarm if active,
      then return. */
   if( read_returned > 0 ){

      if( Alarm ){

         int clear_alarm = 0;

         if( what_read == ORPGBDR_ACCT_READ ){

            if( Last_read_data_status >= 0 )
               clear_alarm = 1;

         }
         else if( what_read == ORPGBDR_DATA_READ ){

            if( Last_read_acct_status >= 0 )
               clear_alarm = 1;

         }

         if( clear_alarm ){

            if( ORPGINFO_statefl_rpg_alarm( ORPGINFO_STATEFL_RPGALRM_DBFL,
                                            ORPGINFO_STATEFL_CLR, &Alarm ) )
               LE_send_msg( GL_INFO, "Unable To CLEAR Data Disk Failure\n" );
      
         }

      }
  
   }

   /* On read failure, process depending on failure. */
   else{

      if( (read_returned != LB_EXPIRED) && (read_returned != LB_BUF_TOO_SMALL) ){

         if( !Alarm ){

            /* Must set the Base Data Disk Failure Alarm. */  
            if( ORPGINFO_statefl_rpg_alarm( ORPGINFO_STATEFL_RPGALRM_DBFL,
                                            ORPGINFO_STATEFL_SET, &Alarm ) )
               LE_send_msg( GL_INFO, "Unable To SET Base Data Disk Failure\n" );

         }

      }

   }

   if( what_read == ORPGBDR_ACCT_READ )
      Last_read_acct_status = read_returned;
   else if( what_read == ORPGBDR_DATA_READ )
      Last_read_data_status = read_returned;
   else
      Last_read_adapt_status = read_returned;

/* End of Process_read_status() */
}

/********************************************************************
   Description:
      Process the base data write status.  Sets/Clears Base Data Disk
      Failure depending on write status.

   Input:
      write_returned - value returned from ORPGDA_write.
      what_written - the LB which was written to.

   Outputs:

   Notes:

*********************************************************************/
static void Process_write_status( int write_returned, int what_written ){

   /* Get Base Data Disk Alarm status. */
   if( ORPGINFO_statefl_rpg_alarm( ORPGINFO_STATEFL_RPGALRM_DBFL,
                                   ORPGINFO_STATEFL_GET, &Alarm ) ){

      LE_send_msg( GL_INFO, "Unable To GET Base Data Disk Failure State\n" );
      return;

   }

   /* On normal write, clear Base Data Disk Failure alarm if active,
      then return. */
   if( write_returned > 0 ){

      if( Alarm ){

         int clear_alarm = 0;

         if( what_written == ORPGBDR_ACCT_WRITE ){

            if( Last_write_data_status >= 0 )
               clear_alarm = 1;

         }
         else if( what_written == ORPGBDR_DATA_WRITE ){

            if( Last_write_acct_status >= 0 )
               clear_alarm = 1;

         }

         if( clear_alarm ){

            if( ORPGINFO_statefl_rpg_alarm( ORPGINFO_STATEFL_RPGALRM_DBFL,
                                            ORPGINFO_STATEFL_CLR, &Alarm ) )
               LE_send_msg( GL_INFO, "Unable To CLEAR Base Data Disk Failure\n" );
      
         }

      }

   }

   /* On write failure, activate alarm. */
   else{

      if( !Alarm ){

         /* Must set the Base Data Disk Failure Alarm. */  
         if( ORPGINFO_statefl_rpg_alarm( ORPGINFO_STATEFL_RPGALRM_DBFL,
                                         ORPGINFO_STATEFL_SET, &Alarm ) )
            LE_send_msg( GL_INFO, "Unable To SET Base Data Disk Failure\n" );
   
      }

   }

   if( what_written == ORPGBDR_ACCT_WRITE )
      Last_write_acct_status = write_returned;
   else if( what_written == ORPGBDR_DATA_WRITE )
      Last_write_data_status = write_returned;

/* End of Process_write_status() */
}

/***************************************************************************

   Description:
      Writes informational message to task log file about volume just
      completed.

   Inputs:
      ind - index into Radial_list for this volume scan.
      vol_scan_num - volume scan number for this volume scan.

   Outputs:

   Returns:

   Notes:

**************************************************************************/
static void Write_informational_msg( int ind, int vol_scan_num ){

   int ret = 0, num_radials, i;
   int year = 0, month = 0, day = 0;
   int hours = 0, minutes = 0, seconds = 0;
   time_t time_value = ((Radial_list[ind].vol_data->vol_start_date-1)*86400)
                       + Radial_list[ind].vol_data->vol_start_time;

   LE_send_msg( GL_INFO, "Radial Replay Accounting Data For Vol # %2d (Vol Seq # %d)\n",
                vol_scan_num, Radial_list[ind].vol_data->vol_seq_num );

   ret = unix_time( &time_value, &year, &month, &day, &hours, &minutes,
                             &seconds );
   if( ret >= 0 ){

      if ( year >= 2000 )
         year -= 2000;
      else
         year -= 1900;

   }

   /* Convert date/time to mm/dd/yy hh:mm:ss format. */
   LE_send_msg( GL_INFO, 
                "---> Vol Date/Time (%d %d): %02d/%02d/%02d %02d:%02d:%02d\n",
                Radial_list[ind].vol_data->vol_start_date,
                Radial_list[ind].vol_data->vol_start_time,
                month, day, year, hours, minutes, seconds );

   LE_send_msg( GL_INFO, "---> VCP: %4d, # Elevs: %2d, # Cmpltd Elevs: %2d\n",
                Radial_list[ind].vol_data->vol_cov_pat,
                Radial_list[ind].vol_data->num_elev_cuts,
                Radial_list[ind].vol_data->num_completed_cuts );

   for( i = 0; i < Radial_list[ind].vol_data->num_completed_cuts; i++ ){

      num_radials = Radial_list[ind].vol_data->elevs[i].num_radials;
      LE_send_msg( GL_INFO, "------> Cut #: %2d, Start ID: %d, End ID: %d\n",
                   i+1, Radial_list[ind].vol_data->elevs[i].radial_msgid[0],
                   Radial_list[ind].vol_data->elevs[i].radial_msgid[num_radials-1] );

   }

/* End of Write_informational_msg() */
}

/***************************************************************************

   Description:
      Validates the index values used to access the accounting data.

   Inputs:
      acct_ind - index into Radial_list for elevation cut number.
      num_radials - index into Radial_list for radial number.

   Outputs:

   Returns:
      ORPGBDR_ERROR on error or 0 otherwise.

   Notes:

**************************************************************************/
static int Validate_indices( int acct_ind, int num_radials ){

   /* Validate the elevation cut number index. */
   if( (acct_ind < 0) || (acct_ind >= MAX_ELEVS) ){

      LE_send_msg( GL_ERROR, "acct_ind (%d) >= MAX_ELEVS (%d).\n", acct_ind, MAX_ELEVS );
      return( ORPGBDR_ERROR );

   }

   /* Validate the number of radials index. */
   if( (num_radials < 0) || (num_radials >= MAX_SR_RADIALS) ){

      LE_send_msg( GL_ERROR, "num_radials (%d) >= MAX_SR_RADIALS (%d)\n", 
                   num_radials, MAX_SR_RADIALS );
      return( ORPGBDR_ERROR );

   }

   return 0;

/* End of Validate_indices( ) */
}


/***************************************************************
   Description:
      Returns the number of elevations in the VCP, given the 
      VCP number. 

   Inputs:
      vcp_num - volume coverage pattern (VCP) number.
      vol_scan_num - volume scan number.

   Returns: 
      Number of elevations in VCP on success.

***************************************************************/
static int Get_num_elevations( int vcp_num, int vol_scan_num ){

   int vs_num = 0, ret = sizeof(RDA_rdacnt_t);

   /* Read RDA_RDACNT from ORPGDAT_ADAPTATION.   This holds the 
      RDA provided VCP definition.  If available and the vcp 
      number matches, use this.  Otherwise use the information
      provide by RDACNT. */
   if( Read_RDA_RDACNT )
      ret = Read_rda_rdacnt();

   if( ret >= sizeof(RDA_rdacnt_t) ){

      VCP_ICD_msg_t *rdavcp = NULL;

      /* Determine which index to access. */
      if( (vol_scan_num <= 0) || (vol_scan_num >= MAX_VSCANS) )
         vs_num = Rda_rdacnt.last_entry;

      else
         vs_num = vol_scan_num % 2;

      rdavcp = (VCP_ICD_msg_t *) &Rda_rdacnt.data[vs_num].rdcvcpta[0];
      if( rdavcp->vcp_msg_hdr.pattern_number == vcp_num ){

         /* Return number of elevations. */
         return( (int) rdavcp->vcp_elev_data.number_cuts );

      }

   }

   /* If it falls through to here, then use the data provided 
      in RDACNT (via ORPGVCP functions). */
   return( ORPGVCP_get_num_elevations( vcp_num ) );

/* End of Get_num_elevations() */
}


/***************************************************************
   Description:
      Returns the RPG elevation number given the VCP number and 
      RDA elevation index.

   Inputs:
      vcp_num - volume coverage pattern (VCP) number.
      vol_scan_num - volume scan number [1-80]
      elev_ind - RDA elevation index within VCP (zero indexed).

   Returns: 
      -1 on error or RPG elevation number on success.

***************************************************************/
static int Get_rpg_elevation_num( int vcp_num, 
                                  int vol_scan_num, 
                                  int elev_ind ){

   int vs_num = 0, ret = sizeof(RDA_rdacnt_t);

   /* Read RDA_RDACNT from ORPGDAT_ADAPTATION.   This holds the 
      RDA provided VCP definition.  If available and the vcp 
      number matches, use this.  Otherwise use the information
      provide by RDACNT. */
   if( Read_RDA_RDACNT )
      ret = Read_rda_rdacnt();

   if( ret >= sizeof(RDA_rdacnt_t) ){

      VCP_ICD_msg_t *rdavcp = NULL;

      /* Determine which index to access. */
      if( (vol_scan_num <= 0) || (vol_scan_num >= MAX_VSCANS) )
         vs_num = Rda_rdacnt.last_entry;

      else
         vs_num = vol_scan_num % 2;

      rdavcp = (VCP_ICD_msg_t *) &Rda_rdacnt.data[vs_num].rdcvcpta[0];

      if( rdavcp->vcp_msg_hdr.pattern_number == vcp_num ){
 
         int num_elevs = Get_num_elevations( vcp_num, vol_scan_num );

         /* Verify we aren't trying to get an RPG elevation index for 
            a more than the number of cuts in the VCP. */
         if( (num_elevs < 0) || (elev_ind >= num_elevs) )
            return -1;

         /* Return RPG elevation number. */
         return( (int) Rda_rdacnt.data[vs_num].rdccon[elev_ind] );

      }

   }

   /* If it falls through to here, then use the data provided 
      in RDACNT (via ORPGVCP functions). */
   return( ORPGVCP_get_rpg_elevation_num( vcp_num, elev_ind ) );

/* End of Get_rpg_elevation_num() */
}


/***************************************************************
   Description:
      Returns the waveform given the VCP number and RDA 
      elevation index.

   Inputs:
      vcp_num - volume coverage pattern (VCP) number.
      vol_scan_num - volume scan number [1-80]
      elev_ind - RDA elevation index within VCP (zero indexed).

   Returns: 
      -1 on error or waveform on success.

***************************************************************/
static int Get_waveform( int vcp_num, int vol_scan_num,
                         int elev_ind ){

   int vs_num = 0, ret = sizeof(RDA_rdacnt_t);

   /* Read RDA_RDACNT from ORPGDAT_ADAPTATION.   This holds the 
      RDA provided VCP definition.  If available and the vcp 
      number matches, use this.  Otherwise use the information
      provide by RDACNT. */
   if( Read_RDA_RDACNT )
      ret = Read_rda_rdacnt();

   if( ret >= sizeof(RDA_rdacnt_t) ){

      VCP_ICD_msg_t *rdavcp = NULL;

      /* Determine which index to access. */
      if( (vol_scan_num <= 0) || (vol_scan_num >= MAX_VSCANS) )
         vs_num = Rda_rdacnt.last_entry;

      else
         vs_num = vol_scan_num % 2;

      rdavcp = (VCP_ICD_msg_t *) &Rda_rdacnt.data[vs_num].rdcvcpta[0];

      if( rdavcp->vcp_msg_hdr.pattern_number == vcp_num ){

         int num_elevs = Get_num_elevations( vcp_num, vol_scan_num );

         /* Verify we aren't trying to get an RPG elevation index for 
            a more than the number of cuts in the VCP. */
         if( (num_elevs < 0) || (elev_ind >= num_elevs) )
            return -1;

         /* Return RPG elevation number. */
         return( (int) rdavcp->vcp_elev_data.data[elev_ind].waveform );

      }

   }

   /* If it falls through to here, then use the data provided 
      in RDACNT (via ORPGVCP functions). */
   return( ORPGVCP_get_waveform( vcp_num, elev_ind ) );

/* End of Get_waveform() */
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
void Notify_callback( int fd, LB_id_t msgid, int msg_info, void *arg ){

   /* Set flag to read RDA_RDACNT. */
   Read_RDA_RDACNT = 1;

/* End of Notify_callback() */
}

/*********************************************************************

   Description:
      Reads the RDA_RDACNT message from ORPGDAT_ADAPTATION.

   Returns:
      The return value from ORPGDA_read().

*********************************************************************/
static int Read_rda_rdacnt(){

   /* Reset update flag and read LB. */
   Read_RDA_RDACNT = 0;
   return( ORPGDA_read( ORPGDAT_ADAPTATION, (void *) &Rda_rdacnt,
                        sizeof(RDA_rdacnt_t), RDA_RDACNT ) );

/* End of Read_rda_rdacnt(). */
}

