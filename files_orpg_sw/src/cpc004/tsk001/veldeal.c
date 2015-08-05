/* RCS info */
/* $Author: cmn $ */
/* $Locker:  $ */
/* $Date: 2015/02/19 17:04:19 $ */
/* $Id: veldeal.c,v 1.15 2015/02/19 17:04:19 cmn Exp $ */
/* $Revision: 1.15 $ */
/* $State: Exp $ */

#include "veldeal.h"
#include "vdeal.h"

/* Macro definitions. */
#define VEL_RESO_05MS			2
#define VEL_RESO_1MS			4

/* Static Global Variables. */
static Vdeal_t Vdv;

static char *User_defined_commands = "IV";

static int Monitor_load = 0;

static int Read_vcp = 1;

static Vol_stat_gsm_t Vol_stat;

int Verbose_mode = 0;


/* Function Prototypes. */
static int Check_for_prf_sectors( int rpg_elev_ind );
void Envwnd_update();
int Service_options( int arg, char *optarg );
void Handle_notification( int data_id, LB_id_t msg_id );


/************************************************************************

   Description: 
      This module contains the main function for the velocity dealiasing 
      algorithm. 

***********************************************************************/
int main( int argc, char *argv[] ){

    int i;

    static int vel_reso = VEL_RESO_05MS;
    static int allow_1ms = 0;

    /* Initialize the number of Nyquist intervals to check. */
    Num_intrvl_chk = 4;

    EN_control (EN_SET_SIGNAL, EN_NTF_NO_SIGNAL);

    /* Register custom options. */
    RPGC_reg_custom_options( User_defined_commands, Service_options );

    /* Initialize the log error services. */
    RPGC_init_log_services( argc, argv );

    /* Specify inputs and outputs. */
    RPGC_reg_io( argc, argv );

    /* Register adaptation blocks. */
    RPGC_reg_RDA_control( &RDA_cntl.rdacnt_start, BEGIN_VOLUME );

    /* Register for MPDA and RADAZVD adaptation data. */
    RPGC_reg_ade_callback( mpda_callback_fx, &Mpda.gui_mpda_tover,
	                   MPDA_DEA_NAME, BEGIN_ELEVATION );
    RPGC_reg_ade_callback( radazvd_callback_fx, &Radazvd.num_replace_lookahead_s, 
                           RADAZVD_DEA_NAME, BEGIN_VOLUME );

    /* Register for site info updates. */
    RPGC_reg_site_info( &Siteadp.rda_lat );

    /* Register for scan summary and volume status updates. */
    RPGC_reg_scan_summary();
    RPGC_reg_volume_status( &Vol_stat );

    /* Register for RDA VCP updates. */
    RPGC_data_access_UN_register( ORPGDAT_RDA_VCP_DATA, ORPGDAT_RDA_VCP_MSG_ID,
                                  Handle_notification );

    /* Register ITC outputs */
    RPGC_itc_out( A3CD97, &Ewt.envwndflg, sizeof(A3cd97), ITC_ON_CALL );
    RPGC_itc_out( PCT_OBS, &Pct.vol_time, sizeof(Pct), ITC_ON_CALL );

    /* Register ITC inputs. */
    RPGC_itc_in( A3CD97, &Ewt.envwndflg, sizeof(A3cd97), ITC_ON_CALL );
    RPGC_itc_in( ENVVAD, &Envvad.parameter_from_vad[0], sizeof(Envvad), ITC_ON_CALL );

    /* Register for Environmental Wind Updates from HCI. */
    RPGC_reg_for_external_event( ORPGEVT_ENVWND_UPDATE, Envwnd_update, 
                                 ORPGEVT_ENVWND_UPDATE );

    /* Register for monitoring RPG radial input buffer load.  
       Note:  If this task does not dealias Super Resolution
              data, this function call has no effect. */
    if( Monitor_load )
       RPGC_monitor_input_buffer_load( "RAWDATA" );

    /* Tell system we are radial-based. */
    RPGC_task_init( RADIAL_BASED, argc, argv ); 

    /* Initialize adaptation data to short pulse */
    A304di_vd_local_copy( ORPGVCP_SHORT_PULSE );

    /* Allocate and initialize data arrays to radazvd. 

       Note:  This arrays are allocated instead of statically defined 
              to facilitate debugging using valgrind. */
    Unbias_vel = (short *) calloc( 1, (LOOKUP_SIZE+1)*sizeof(short) );
    Square = (int *) calloc( 1, (LOOKUP_SIZE+1)*sizeof(int) );

    if( (Unbias_vel == NULL) || (Square == NULL) ){

        RPGC_log_msg( GL_ERROR, "calloc Failed for Unbias_vel/Square\n" );
        RPGC_hari_kiri(); 

    }
    
    Square[0] = 0;
    Square[1] = 1;
    Unbias_vel[0] = 0;
    Unbias_vel[1] = 1;
    for( i = 2; i <= LOOKUP_SIZE; i++ ){

        Square[i] = i*i;
        Unbias_vel[i] = i - VEL_BIAS;

        if( Unbias_vel[i] < MIN_VEL )
            Unbias_vel[i] = MIN_VEL;

        if( Unbias_vel[i] > MAX_VEL )  
            Unbias_vel[i] = MAX_VEL; 

    }

    Unbias_vel[LOOKUP_SIZE] = MAX_VEL + 1;

    Deleted_vel = (short *) calloc( 1, MAX_BINS_DELETED*sizeof(short) );
    Deleted_bin = (short *) calloc( 1, MAX_BINS_DELETED*sizeof(short) );
    if( (Deleted_vel == NULL) || (Deleted_bin == NULL) ){

        RPGC_log_msg( GL_ERROR, "calloc Failed for Deleted_vel/Deleted_bin\n" );
        RPGC_hari_kiri(); 

    }

    A304db.vel_prevaz[BAD_PREV_RADIAL] = 
        (short *) calloc( 1, VELDEAL_MAX_SIZE*sizeof(short) );
    A304db.vel_prevaz[GOOD_PREV_RADIAL] = 
        (short *) calloc( 1, VELDEAL_MAX_SIZE*sizeof(short) );
    if( (A304db.vel_prevaz[BAD_PREV_RADIAL] == NULL) 
                         || 
        (A304db.vel_prevaz[GOOD_PREV_RADIAL] == NULL) ){

        RPGC_log_msg( GL_ERROR, "calloc Failed for A304db.vel_prevaz\n" );
        RPGC_hari_kiri(); 

    }

    /* Initialization for the 2D dealiasing algorithm. */
    memset( &Vdv, 0, sizeof(Vdeal_t) );

    /* Waiting for activation. */
    while(1){

        Scan_Summary *summary = NULL;
        int mpda_flg, vol_scan_num;

        RPGC_wait_act( WAIT_DRIVING_INPUT );
        vol_scan_num = RPGC_get_current_vol_num();
        summary = RPGC_get_scan_summary( vol_scan_num ); 

        /* ;** Check if the vcp requires mpda processing */
        mpda_flg = 0;
        if( summary == NULL ){

            RPGC_log_msg( GL_INFO, "RPGC_get_scan_summary( %d) Returned NULL\n",
                          vol_scan_num );
	    A304da_vd_buf_cntrl();

        }
        else{

            check_for_mpda_vcp( summary->vcp_number, &mpda_flg );
            if( mpda_flg ) 
	        mpda_buf_cntrl( summary->vcp_number );

            else{

                int rpg_elev_ind = 0, prf_sectors = 0;

                /* Only need to check for PRF sectors if 2D dealiasing
                   is active. */
                if( Radazvd.use_2D_dealiasing ){

                    /* Get the RPG elevation index.   Use this to determine
                       the RDA elevation index.   For the RDA elevation index,
                       determine if PRF sectors are being used. */
                    rpg_elev_ind = RPGC_get_current_elev_index();
                    RPGC_log_msg( GL_INFO, "Current RPG Elevation Index: %d\n",
                              rpg_elev_ind );

                    prf_sectors = Check_for_prf_sectors( rpg_elev_ind );
                    if( prf_sectors ){

                        RPGC_log_msg( GL_INFO, "PRF sectors defined for this cut.\n" );
                        RPGC_log_msg( GL_INFO, "--->2D Dealiasing Disabled for this cut.\n" );

                    }
              
                    /* Check for 1 m/s velocity increment.  The velocity increment is
                       part of the VCP definition that applies to the entire volume so
                       this only needs to be checked at beginning of volume. */
                    if( rpg_elev_ind == 1 ){ 
 
                       double value = 0;
                       int ret = 0;

                       vel_reso = VEL_RESO_05MS;
                       if( Vol_stat.current_vcp_table.vel_resolution == VEL_RESO_1MS ){

                          RPGC_log_msg( GL_INFO, "Velocity Resolution is 1 m/s\n" );
                          vel_reso = VEL_RESO_1MS;

                          allow_1ms = 0;
                          ret = DEAU_get_values( "alg.vdeal.allow_1ms", &value, 1 );
                          if( ret > 0 ){

                             if( (int) value == 1 ){

                                allow_1ms = 1;
                                RPGC_log_msg( GL_INFO, "--->2D Dealiasing Allowed for this VCP.\n" );

                             }

                          }

                          if( !allow_1ms )
                             RPGC_log_msg( GL_INFO, "--->2D Dealiasing Disabled for this VCP.\n" );

                       }

                    }

                }

                /* Use 2D dealiasing if: 1) enabled, 2) there are not PRF sectors, and
                   3) the velocity increment is not 1 M/S. */
                if( Radazvd.use_2D_dealiasing 
                               && 
                          (!prf_sectors) 
                               &&
                    ((vel_reso != VEL_RESO_1MS) || allow_1ms ) )
                    VD2D_realtime_processing( &Vdv );

                else
	            A304da_vd_buf_cntrl();

            }

        }
     
    }

}

/****************************************************************************

   Description:
      Checks the RDA VCP definition for PRF sectors for the current 
      elevation.

****************************************************************************/
static int Check_for_prf_sectors( int rpg_elev_ind ){

    int ret, j;

    static char *buf = NULL;
    static short *rdccon = NULL;

    /* Read the VCP definition. */
    if( Read_vcp ){

        /* Free the VCP data. */
        if( buf != NULL ){

           free( buf );
           buf = NULL;

        }

        /* Read the VCP data from datastore. */
        Read_vcp = 0;
        ret = RPGC_data_access_read( ORPGDAT_RDA_VCP_DATA,
                                     &buf, LB_ALLOC_BUF,
                                     ORPGDAT_RDA_VCP_MSG_ID );

        /* If read fails, return 1 (PRF sectors defined). */
        if( ret <= 0 ){

           RPGC_log_msg( GL_INFO, "RPGC_data_access_read( RDA VCP ) Faile: %d\n",
                         ret );
           return 1;

        }

    }

    /* If buf contains VCP information, check if PRF sectors are defined
       for this elevation. */
    if( buf != NULL ){

        Vcp_struct *vcp = (Vcp_struct *) (buf + sizeof(RDA_RPG_message_header_t));
        int ind, n_ele = vcp->n_ele;;
        Ele_attr *ele_attr = NULL;
 
        /* Do some rudimentary validation.   If the number of cuts is not
           within limits, something must be wrong. */
        if( n_ele > VCP_MAXN_CUTS ){

            RPGC_log_msg( GL_INFO, "Invalid Number of Cuts in VCP: %d\n", n_ele );
            return 1;

        }
       
        /* Must be an operational VCP. */
        if( vcp->vcp_num > 255 ){

            RPGC_log_msg( GL_INFO, "Non-operational VCP? %d\n", vcp->vcp_num );
            return 1;

        }

        /* Free the RDA to RPG elevation index mapping table. */
        if( rdccon != NULL ){

           free( rdccon );
           rdccon = NULL;

        }

        /* Get the elevation index table. */
        rdccon = RPGCS_get_elev_index_table( vcp->vcp_num );
        if( rdccon == NULL ){

            RPGC_log_msg( GL_INFO, "Failed to Read RDA/RPG Elevation Mapping Table\n" );
            return 1;

        }
 
        ind = 0;
        while( (ind < n_ele) 
                   &&
               (rdccon[ind] <= rpg_elev_ind) ){
 
            int prf1 = 0, prf2 = 0, prf3 = 0;

            j = rdccon[ind];
            if( rpg_elev_ind == j ){

               ele_attr = (Ele_attr *) &vcp->vcp_ele[ind][0];
               prf1 = ele_attr->dop_prf_num_1;
               prf2 = ele_attr->dop_prf_num_2;
               prf3 = ele_attr->dop_prf_num_3;

               if( (prf1 > 0) && (prf2 > 0) && (prf1 != prf2) )
                   return 1;

               if( (prf1 > 0) && (prf3 > 0) && (prf1 != prf3) )
                   return 1;

            }

            /* Increment the mapping table index. */
            ind++;

        }

    }

    /* Must be all the same PRF. */
    return 0;

} /* End of Check_for_prf_sectors(). */

/****************************************************************************

   Description:
      Event service routine.

****************************************************************************/
void Envwnd_update(){

    LE_send_msg( GL_INFO, "New Evironmental Wind Data Event Received\n" );
    Env_wnd_tbl_updt = 1;

} /* End of Envwnd_update(). */

/***************************************************************************

   Description:
      Routine for servicing command line options.

   Inputs:
      arg - the command line argument.
      optarg - pointer to the options arguments.

   Returns:
      0 on Success, -1 on error.

**************************************************************************/
int Service_options( int arg, char *optarg ){

    switch( arg ){

        case 'I':
            RPGC_log_msg( GL_INFO, "Setting Monitor Radial Load\n" );
            Monitor_load = 1;
            break;

        case 'V':
            RPGC_log_msg( GL_INFO, "Setting Verbose Mode\n" );
            Verbose_mode = 1;
            break;

        default:
            break;

    }

    return 0;

/* End of Service_options() */
}

/************************************************************************

   Description:
      Notification handler called when data_id and msg_id have been 
      updated.  When called, Read_vcp flag is set.

   Inputs:
      data_id - Data ID of LB 
      msg_id - Message ID within LB.

   
*************************************************************************/
void Handle_notification( int data_id, LB_id_t msg_id ){

    /* Verify the data ID .... */
    if( (data_id == ORPGDAT_RDA_VCP_DATA) 
                 && 
        (msg_id == ORPGDAT_RDA_VCP_MSG_ID) ){

        RPGC_log_msg( GL_INFO, "!!! ORPGDAT_RDA_VCP_DATA Updated !!!\n" );
        Read_vcp = 1;

    }

} /* End of Handle_notification(). */
