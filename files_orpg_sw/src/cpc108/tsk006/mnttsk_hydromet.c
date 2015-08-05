/*************************************************************************

   Module:  mnttsk_hydromet.c

   Description:

 **************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2005/12/27 16:41:37 $
 * $Id: mnttsk_hydromet.c,v 1.31 2005/12/27 16:41:37 steves Exp $
 * $Revision: 1.31 $
 * $State: Exp $
 */

#include <stdio.h>
#include <stdlib.h>            /* EXIT_SUCCESS, EXIT_FAILURE              */
#include <unistd.h>            /* unlink()                                */
#include <errno.h>
#include <sys/types.h>         /* open(),lseek()                          */
#include <sys/stat.h>          /* open()                                  */
#include <fcntl.h>             /* open()                                  */
#include <math.h>

#include <orpg.h>
#include <mrpg.h>
#include <prod_gen_msg.h>
#include <gagedata.h>
#include <a3147.h>
#include <string.h>
#include <hydro_files.h>
#include <misc.h>
#include <orpgsite.h>


/*
 * Constant Definitions/Macro Definitions/Type Definitions
 */
#define STARTUP         1
#define RESTART         2
#define CLEAR_HYDRO     3
#define CLEAR_GAGE      4
#define CHECK           5
#define CLEAR_BLOCKAGE  6    

#define LOG_STATUS_MSGS 0

extern int errno;

/* Static Global Variables */
static char Tmp_path[ORPG_PATHNAME_SIZ] ;

typedef void (*byte_swap_func_t)(void* data);

#define FILE_NAME_SIZE              12
static char Acum_fn[ FILE_NAME_SIZE+1 ];
static char Gas_fn[ FILE_NAME_SIZE+1 ];
static char Hyprod_fn[ FILE_NAME_SIZE+1 ];
static char Grp_fn[ FILE_NAME_SIZE+1 ];

typedef struct environ_t{
   int environ_first;
   char occ_fn[FILE_NAME_SIZE];
   char sctr_fn[FILE_NAME_SIZE];
   char acum_fn[FILE_NAME_SIZE];
   char gas_fn[FILE_NAME_SIZE];
   char gdb_fn[FILE_NAME_SIZE];
   char hyprod_fn[FILE_NAME_SIZE];
   char usersel_fn[FILE_NAME_SIZE];
   char grp_fn[FILE_NAME_SIZE];
   int environ_last;
} Environ_t;

int Precipitation_algs_killed = 0;

/*
 * Static Function Prototypes
 */
static int Initialize_from_repository(int data_id, int repository_id,
                                      char *input_file_path, int msgid) ;
static int Initialize_gagedata(void) ;
static int Initialize_hyusrsel(void) ;
static int Check_beam_blockage(void) ;
static int Remove_database_files(void) ;
static int Check_files(void) ;
static int Get_command_line_args( int argc, char **argv, int *startup_action,
                                  char path_name[] );
static int Get_filenames();
static int File_to_dsmsg( const char *filepath, unsigned int offset_bytes,
                           int data_id, LB_id_t msgid, byte_swap_func_t swap_func);

void byte_swap_terrain(void* data);
void Event_handler(en_t event, void *msg, size_t msg_len );

/**************************************************************************
    Description:
       Initialization routine for hydromet files.

    Input:
       argc - the number of command line arguments.
       argv - the command line arguments.

    Output:

    Returns:
       On failure of any sort, exits with exit code > 0.  Normal termination,
       exit code is 0.

    Notes:
 **************************************************************************/
int main(int argc, char *argv[]){

    int startup_action ;
    int retval ;
    Mrpg_state_t rpg_state;
    char input_file_path[255];

    /* Initialize log-error services. */
    ORPGMISC_init( argc, argv, 200, 0, -1, LOG_STATUS_MSGS );

    /* Get command line arguments.  On failure, exit. */
    if( (retval = Get_command_line_args( argc, argv, &startup_action, 
                                         input_file_path) ) != 0)
       exit(retval);

    /* If startup_action indicates to check the files, do the following. */
    if( startup_action == CHECK ){

         LE_send_msg(GL_INFO,"CHECK DUTIES:") ;

         LE_send_msg( GL_INFO,
                      "\t1. Check the Accumulation File.") ;
         LE_send_msg( GL_INFO,
                      "\t2. Check the Gage Scan File.") ;
         LE_send_msg( GL_INFO,
                      "\t3. Check the Hydromet Product File.") ;
         LE_send_msg( GL_INFO,
                      "\t4. Check the Gage-Radar Pairs File.") ;

         retval = Get_filenames();
         Check_files();

         exit(0);

    }

    /* If startup_action indicates startup, do the following. */
    if( startup_action == STARTUP ){

         LE_send_msg(GL_INFO,"STARTUP DUTIES:") ;

         LE_send_msg( GL_INFO,
                      "\t1. Initialize GAGEDATA if Data Does Not Exist.") ;
         LE_send_msg( GL_INFO,
                      "\t2. Initialize HYUSRSEL if Data Does Not Exist.") ;
         LE_send_msg( GL_INFO,
                      "\t3. Initialize ORPGDAT_BLOCKAGE from repository.") ;

         /* Initialize the gage data base. */
         LE_send_msg( GL_INFO, "Initializing Gage Database.\n" );
         if( (retval = Initialize_gagedata()) < 0 ){ 

            LE_send_msg( GL_INFO, "Initialize_gagedata() failed (%d)\n", retval) ;
            exit(4) ;

         }

         /* Initialize the hydroment user selectable database. */
         LE_send_msg( GL_INFO, "Initializing User Selectable Database\n" );
         if( (retval = Initialize_hyusrsel() ) < 0 ){

            LE_send_msg( GL_INFO, "Initialize_hyusrsel() failed (%d)\n", retval) ;
            exit(5) ;
         }

         /* Check if Beam Blockage Data already exists.  If does not exist,
            try and create from the terrain data. */   
         if( (retval = Check_beam_blockage()) < 0 ){

            /* Create terrain data linear buffer. */
            LE_send_msg( GL_INFO, "Initializing TERRAIN File.\n" );
            if( (retval = Initialize_from_repository( ORPGDAT_TERRAIN_DAT,
                                                      ORPGDAT_TERRAIN,
                                                      input_file_path, 1) ) < 0 ){

               /* The terrain data is only available at initialization. */
               LE_send_msg( GL_STATUS, "Terrain Data Initialization FAILED\n" ) ;
               LE_send_msg( GL_STATUS, "Beam Blockage Data NOT AVAILABLE\n" );
            
               /* We do not consider this fatal.  Let the user of this data do the
                  error processing. */
               exit(0) ;

            }
  
            /* Run beam blockage algorithm.  */
            if ((retval = Initialize_beam_blockage()) < 0) {

               LE_send_msg(GL_STATUS, "Beam Blockage Data Initialization FAILED\n");
               LE_send_msg( GL_STATUS, "Beam Blockage Data NOT AVAILABLE\n" );
               exit(0);

            }

         }

    }

    /* If startup_action is clear_hydro, then do the following ... */
    else if ( startup_action == CLEAR_HYDRO ){

        LE_send_msg( GL_INFO,"CLEAR HYDROMET DATABASE DUTIES\n" ) ;

        /* re: Legacy RPGCLRDB.CSS script */
        LE_send_msg(GL_INFO,"\t1. Delete HYACCUMS.DAT") ;
        LE_send_msg(GL_INFO,"\t2. Delete HYGAGSCN.DAT") ;
        LE_send_msg(GL_INFO,"\t3. Delete HYPROD.DAT") ;

        /* Check if the RPG is SHUTDOWN or STANDBY. */
        retval = ORPGMGR_get_RPG_states( &rpg_state );

        /* The RPG is either SHUTDOWN or STANDBY. */
        if( rpg_state.state != MRPG_ST_OPERATING ){

            if (Remove_database_files() < 0) {

               LE_send_msg( GL_INFO, "Remove_database_files() failed\n!") ;
               exit(7) ;

            }

        }
        else{

            int ret;
            int count = 0;
            int failed = 0;
            char msg[256];

            /* The RPG is in state OPERATING. */

            /* 0.  Register for event ORPGEVT_TERM_PRECIP_ALGS */
            EN_register( ORPGEVT_TERM_PRECIP_ALGS, (void *) Event_handler );

            /* 1. Tell mrpg to kill the precipitation algorithms. */
            LE_send_msg( GL_INFO, "Command mrpg to STOP epre, prcprtac, prcpadju, hybrprod and prcpprod\n" );
            sprintf( msg, "STOP epre prcprtac prcpadju hybrprod prcpprod ACK-%d",
                     ORPGEVT_TERM_PRECIP_ALGS );
            ret = ORPGMGR_send_msg_command( MRPG_TASK_CONTROL, msg );
            if( ret < 0 ){

               LE_send_msg( GL_STATUS, "Hydromet Initialization FAILED\n" );
               exit(8);

            }

            /* 2. Remove the database files. */
            while( !failed ){

               if (Precipitation_algs_killed ){

                  Precipitation_algs_killed = 0;

                  if( Remove_database_files() < 0){

                     LE_send_msg( GL_INFO, "Remove_database_files() failed\n!") ;
                     failed = 1;

                  }
                  else
                     break;

               }

               /* Increment the pass counter.  If killing takes too long,
                  fail this operation.  NOTE: 60 == 1 minute. */
               count++;
               if( count > 60 ){

                  LE_send_msg( GL_INFO, "Waiting For Event ORPGEVT_TERM_PRECIP_ALGS Timed-out\n" );
                  failed = 1;
               }
 
               /* Wait the Precipitation Algorithms to Die!!!! */
               sleep(1);

            /* End of "while" loop. */
            }

            /* 3. Tell mrpg to restart the precipitation algorithms. */
            LE_send_msg( GL_INFO, "Command mrpg to RESTART epre, prcprtac, prcpadju, hybrprod and prcpprod\n" );
            sprintf( msg, "RESTART epre prcprtac prcpadju hybrprod prcpprod" );
            ret = ORPGMGR_send_msg_command( MRPG_TASK_CONTROL, msg );
            if( ret < 0 ){

               LE_send_msg( GL_STATUS, "The Precip Algs Could Not Be Restarted.\n" );
               failed = 1;

            }

            /* Tell this operator the status of the initialization. */
            if( failed ){

               LE_send_msg( GL_STATUS, "Hydromet Initialization FAILED\n" );
               exit(8);

            }
            else
               LE_send_msg( GL_INFO, "Hydromet Initialization Completed\n" );
            
        }

    }
    /* If startup_action is clear_gage, then do the following ... */
    else if ( startup_action == CLEAR_GAGE ){

        LE_send_msg( GL_INFO,"CLEAR GAGE DATABASE DUTIES\n" ) ;

        /*
         * re: Legacy RPGCLRDB.CSS script
         */
         LE_send_msg(GL_INFO,"\t1. Clear GAGEDATA") ;

        LE_send_msg( GL_INFO, "Clearing GAGEDATA of All Messages ...\n" );
        retval = ORPGDA_clear( GAGEDATA, LB_ALL );
        if( retval < 0 ){

           LE_send_msg( GL_INFO, "ORPGDA_clear(GAGEDATA) failed: %d\n", retval );
           exit(7);

        }

    }

    /* If startup_action is clear_blockage, then... */
    else if ( startup_action == CLEAR_BLOCKAGE ){

	LE_send_msg( GL_INFO,"CLEAR BEAM BLOCKAGE DUTIES\n" ) ;
	LE_send_msg(GL_INFO,"\t1. Clear ORPGDAT_BLOCKAGE") ;

	LE_send_msg( GL_INFO, "Clearing BLOCKAGE data of All Messages...\n" );
	retval = ORPGDA_clear( ORPGDAT_BLOCKAGE, LB_ALL );
	if( retval < 0 ){

	    LE_send_msg( GL_STATUS, "Failed To Clear Beam Blockage Data (%d)\n", 
                         retval );
	    exit(0);

        }

    }

    /* Normal termination. */
    exit(0) ;

/*END of main()*/
}


/****************************************************************************

   Description:
      Takes a file and writes data to LB at location specified by the 
      system configuration.

   Inputs:
      in_data_id - data ID of file.
      data_id - data ID of LB.
      input_file_path - path where file resides (excludes file name).
      msgid - ID of message in LB.

   Returns:
      Returns 0 on success, or -1 on error.

****************************************************************************/
static int Initialize_from_repository( int in_data_id, int data_id,
                                       char *input_file_path, int msgid){

    int retval ;
    char *path_name = NULL;
    byte_swap_func_t byte_swap_func = NULL;	
    
    char siteName[5],
    	 terrainData[9],
    	 *dummy;

    /* Build full path name of legacy file */      
    if( input_file_path[ strlen( input_file_path ) - 1 ] != '/' )
        input_file_path[ strlen( input_file_path ) ] = '/';

    if( in_data_id == ORPGDAT_TERRAIN_DAT ){

	dummy = ORPGSITE_get_string_prop(ORPGSITE_RPG_NAME, siteName, 5);
	strcpy(terrainData, siteName);
	strcat(terrainData, ".trd");
	LE_send_msg(GL_INFO, "Terrain height data file name: %s\n",terrainData);
	
	path_name = malloc( strlen( input_file_path ) + sizeof(terrainData) );
	
	if( path_name != NULL )
	{
	    strcpy( path_name, input_file_path );
	    strcat( path_name, terrainData );
	}
	
	byte_swap_func = byte_swap_terrain;
    }

    LE_send_msg( GL_INFO, "Pathname: %s\n",path_name ); 

    /*  Byte swap if necessary.	*/
    if (byte_swap_func == NULL)
    {
        LE_send_msg(GL_ERROR, "Invalid data type");
	return(-1);
    }

    /* Exit if memory allocation failed. */
    if( path_name == NULL ){

        LE_send_msg( GL_ERROR, "malloc Failed\n" );
        return(-1) ;

    }
        
    /* Convert file to linear buffer. */
    if( (retval = File_to_dsmsg(path_name, 0, data_id, msgid, byte_swap_func) ) <= 0 )
    {
       LE_send_msg( GL_INFO, "Data ID %d: File_to_dsmsg() Failed (%d)\n",
                    data_id, retval ) ;
       return(-1) ;
    }

    if( path_name != NULL )
       free( path_name );

    /* Normal return. */
    return(0);
}

/*****************************************************************************

   Description:
      Initializes the GAGEDATA file.

   Inputs:

   Outputs:

   Returns:
      Negative value on error, or 0 on success.

*****************************************************************************/
static int Initialize_gagedata(void){

    char *buf ;
    size_t buflen ;
    size_t hdr_offset ;
    gagedata gagedb ;
    Prod_header *prodhdr ;
    int retval ;

    LB_status status;
    LB_attr attr;
    LB_id_t msgid ;


    /*
     * Get information about the LB.
     */
    status.n_check = 0;
    status.attr = &attr;
    retval = ORPGDA_stat( GAGEDATA, &status );
    if( retval < 0 ){

        LE_send_msg( GL_INFO, "ORPGDA_stat(GAGEDATA) Failed: %d\n",
                     retval );
        return(-1);

    }

    /*
     * If there are messages already in this LB, then do nothing. 
     */
    if( status.n_msgs != 0 ){ 

        LE_send_msg( GL_INFO, 
                     "GAGEDATA has %d messages.  Bypass Initialization\n",
                     status.n_msgs );
        return(0) ;

    }

    /*
     * Do initialization.  Format and write the seminal GAGEDATA message ...
     */
    LE_send_msg( GL_INFO, "Perform GAGEDATA Initialization\n" );
    (void) memset(&gagedb, 0, sizeof(gagedb)) ;

    hdr_offset = sizeof(Prod_header) ;
    buflen = sizeof(gagedb) + hdr_offset ;

    buf = malloc(buflen) ;
    if (buf == NULL) {

        LE_send_msg(GL_INFO,"GAGEDATA malloc(%d) failed!", (int) buflen);
        return(-1);

    }

    (void) memset(buf, 0, buflen) ;

    prodhdr = (Prod_header *) buf ;
    prodhdr->g.prod_id = GAGEDATA ;
    prodhdr->g.gen_t = time((time_t *) NULL) ;
    msgid = (LB_id_t) prodhdr->g.gen_t ;

    (void) memcpy(buf + hdr_offset, &gagedb, sizeof(gagedb)) ;

    retval = ORPGDA_write(GAGEDATA, buf, (int) buflen, msgid) ;
    if (retval != (int) buflen) {

        LE_send_msg(GL_INFO, "ORPGDA_write(GAGEDATA msgid %d) failed: %d != %d",
                    (int) msgid, retval, (int) buflen) ;
        free(buf) ;
        return(-1) ;

    }
    else 
        LE_send_msg( GL_INFO,
                    "Data ID %d: (GAGEDATA) msgid %d has been initialized ...",
                    GAGEDATA, (int) msgid) ;

    free(buf) ;

    return(0) ;

/*END of Initialize_gagedata()*/
}


/******************************************************************************

   Description:
      Initializes the user selectable database file.

   Inputs:

   Outputs:

   Returns:
      Negative value on error, or 0 on success.

*******************************************************************************/
static int Initialize_hyusrsel(void){

    int retval, i ;

    LB_id_t lb_ids[MAX_USDB_RECS] ;
    LB_status status;
    LB_attr attr;

    a3147c9_common_s *hdr_p = NULL ;

    /*
     * First check if there are any messages in the LB.  If there are,
     * do nothing. 
     */
     status.n_check = 0;
     status.attr = &attr;
     retval = ORPGDA_stat( HYUSRSEL, &status );
     if( retval < 0 ){

        LE_send_msg( GL_INFO, "ORPGDA_stat(HYUSRSEL) Failed: %d\n",
                     retval ); 
        return (-1);

     }

     if( status.n_msgs != 0 ){

        LE_send_msg( GL_INFO, 
                     "HYUSRSEL has %d messages.  Bypass Initialization\n",
                     status.n_msgs );
        return (0);

     }

    LE_send_msg( GL_INFO, "Perform HYUSRSEL Initialization\n" );
  
    /*
     * Initialize the array of valid HYUSRSEL Linear Buffer message ids ...
     * These correspond to the constant USDB Sector Offsets values ...
     * This code mimics the USDB_SCTR_OFFS() initialization code found in
     * A31472__INIT_USDB.
     */
    lb_ids[0] = (LB_id_t) USDB_HDR_SCTR ;
    lb_ids[1] = lb_ids[0] + (LB_id_t) NUM_HDR_SCTRS ;

    for (i=2; i < (MAX_USDB_RECS - 1); ++i)
        lb_ids[i] = lb_ids[i-1] + (LB_id_t) NUM_POLAR_SCTRS ;

    lb_ids[MAX_USDB_RECS - 1] = (LB_id_t) DFLT_24H_SCTR ;


    /*
     * Allocate memory for the HYUSRSEL header ...
     */
    hdr_p = calloc((size_t) 1, (size_t) NUM_HDR_BYTES) ;
    if (hdr_p == NULL) 
        return(-1) ;

   /*
    * Initialize hourly scan disk file indices ...
    * Record 1 is for the HDR_REC, the last record is for the DFLT_24H_REC ...
    */
    hdr_p->usdb_sctr_offs[0] = USDB_HDR_SCTR ;
    hdr_p->usdb_sctr_offs[1] = hdr_p->usdb_sctr_offs[0] + NUM_HDR_SCTRS ;

    for (i=2; i < (MAX_USDB_RECS - 1); ++i)
        hdr_p->usdb_sctr_offs[i] = hdr_p->usdb_sctr_offs[i-1] + NUM_POLAR_SCTRS ;

    hdr_p->usdb_sctr_offs[MAX_USDB_RECS - 1] = DFLT_24H_SCTR ;


    /*
     * Initialize the other things in the directory record ...
     */
    for (i=0; i < MAX_USDB_HRS; ++i) {
        /*
         * Initialize the control pointers of the accumulations ...
         */
        hdr_p->usdb_hrs_old[i] = MAX_USDB_HRS - i ;
        hdr_p->usdb_hrly_recno[i] = USDB_HDR_RECNO + i + 1 ;
        hdr_p->usdb_adju_recno[i] = hdr_p->usdb_hrly_recno[i] + MAX_USDB_HRS ;

        /*
         * Initialize times for each of the clock hour accumulations ...
         */
        hdr_p->usdb_hrly_edate[i] = USDB_NO_DATA ;
        hdr_p->usdb_hrly_etime[i] = USDB_NO_DATA ;
  
        /*
         * Set flags for each of the clock hourly accumulation scans ...
         */
        hdr_p->usdb_flg_zero_hrly[i] = 1 ;
        hdr_p->usdb_flg_no_hrly[i] = 1 ;
        hdr_p->usdb_hrly_scan_type[i] = 0 ;

        hdr_p->usdb_sb_status_hrly[i] = 0 ;

        /*
         * Initialize hourly bias values ...
         */
        hdr_p->usdb_cur_bias[i] = 0 ;
        hdr_p->usdb_cur_grpsiz[i] = 0 ;
        hdr_p->usdb_cur_mspan[i] = 0;
        hdr_p->usdb_flg_adjust[i] = 0 ;
    }

    hdr_p->usdb_last_date = USDB_NO_DATA ;
    hdr_p->usdb_last_time = USDB_NO_DATA ;

    hdr_p->usdb_last_data_recno = USDB_NO_DATA ;

    hdr_p->date_gcprod = USDB_NO_DATA ;
    hdr_p->time_gcprod = USDB_NO_DATA ;
    hdr_p->time_span_gcprod = USDB_NO_DATA ;
    hdr_p->flag_no_gcprod = 0 ;

    retval = ORPGDA_write(HYUSRSEL, (char *) hdr_p, NUM_HDR_BYTES, lb_ids[0]) ;
    if (retval <= 0){

        LE_send_msg(GL_INFO,"ORPGDA_write(HYUSRSEL) failed: %d",
                    retval) ;
        free(hdr_p);
        return(-1) ;

    }

    free(hdr_p);

    LE_send_msg( GL_INFO,
                "Data ID %d: (HYUSRSEL) msgid %d has been initialized ...",
                HYUSRSEL, (int) lb_ids[0]) ;
    return(0) ;

/*END of Initialize_hyusrsel()*/
}

#define DB_FILES_NAMELEN 12
#define DB_FILES_NAMESIZ ((DB_FILES_NAMELEN) + 1)
#define CFG_DIR_NAME_SIZE 128

/*****************************************************************************
*****************************************************************************/
static int Check_beam_blockage(void){

    int retval ;

    char *lbname, *p, *path, *cfg_lbname;
 
    LB_status status;
    LB_attr attr;

    status.n_check = 0;
    status.attr = &attr;

    /*
     * Get information about the LB.
     */
    if ((retval = ORPGDA_stat(ORPGDAT_BLOCKAGE, &status)) < 0 ){
        LE_send_msg( GL_INFO, "ORPGDA_stat(ORPGDAT_BLOCKAGE) Failed: %d\n",
                     retval );
        return(-1);
    }

    /*
     * If there are messages already in this LB, then do nothing. 
     */
    if( status.n_msgs != 0 ){ 

        LE_send_msg( GL_INFO, 
                     "ORPGDAT_BLOCKAGE Exists and has %d Messages.\n",
                     status.n_msgs );
        return(0) ;
    }

    /*
     * Get the location of the cfg/bin directory.  
     */
    cfg_lbname = (char *) malloc( CFG_DIR_NAME_SIZE + 1 );
    if( cfg_lbname == NULL ){

       LE_send_msg( GL_ERROR, "malloc Failed for %d Bytes\n", CFG_DIR_NAME_SIZE + 1 );
       return(-1);

    }

    if( (retval = MISC_get_cfg_dir( cfg_lbname, CFG_DIR_NAME_SIZE )) < 0 ){

       LE_send_msg( GL_ERROR, "MISC_get_cfg_dir Returned Error %d\n",
                     retval );
       return(-1);

     }
     
     /* 
      * Construct path of block data in the cfg/bin directory and check if the 
      * file exists.  If not, then return error. 
      */
     if( (lbname = ORPGDA_lbname( ORPGDAT_BLOCKAGE )) == NULL ){
         
        LE_send_msg( GL_ERROR, "ORPGDA_lbname( ORPGDAT_BLOCKAGE ) Failed\n" ); 
        return(-1);

     }
      
 
     p = path = lbname;
     
     while( *p != '\0' ){

        if( *p == '/' )
           path = p + 1;

         p++;

     }

     cfg_lbname = strcat( cfg_lbname, "/" );
     cfg_lbname = strcat( cfg_lbname, "bin/" );
     cfg_lbname = strcat( cfg_lbname, path );

     LE_send_msg( GL_INFO, "Copy File %s to %s\n", cfg_lbname, lbname );
     retval = RSS_copy( cfg_lbname, lbname );

     free( cfg_lbname );
    
     if( retval < 0 ){

        LE_send_msg( GL_ERROR, "RSS_copy Failed (%d)\n", retval );
        return(-1);

     }

     return(0);

}

/****************************************************************************

   Description:
      Remove database files on "clear".

   Inputs:

   Outputs:

   Returns:
      Negative value on error, or 0 on success.

****************************************************************************/
static int Remove_database_files(void){

    char db_files[][DB_FILES_NAMESIZ] = {"HYACCUMS.DAT",
                                         "HYGAGSCN.DAT",
                                         "HYPROD.DAT"} ;
    register int i ;
    int num_db_files ;
    int retval ;
    char workpath[ORPG_PATHNAME_SIZ] ;

    (void) memset(workpath, 0, sizeof(workpath)) ;
    retval = MISC_get_work_dir(workpath, sizeof(workpath)) ;
    if (retval < 0) {

        LE_send_msg(GL_INFO, "MISC_get_work_dir() failed: %d", retval) ;
        return(-1) ;

    }

    /*
     * Blindly delete each of the database files ... since we execute on
     * each node in the RPG system, we expect the unlink() to fail on those
     * nodes on which the HYDROMET tasks have not been executing ...
     */
    num_db_files = sizeof(db_files) / DB_FILES_NAMESIZ ; 

    for (i=0; i < num_db_files; ++i) {

        (void) memset(Tmp_path, 0, sizeof(Tmp_path)) ;
        if ((strlen(workpath) + strlen("/") +
                                strlen(db_files[i])) < sizeof(Tmp_path)) {
            (void) sprintf(Tmp_path, "%s/%s", workpath, db_files[i]) ;
            LE_send_msg(GL_INFO,"Deleting %s ...", db_files[i]) ;
            (void) unlink((const char *) Tmp_path) ;
        }
        else {
            LE_send_msg(GL_INFO,"%s pathname exceeds %d bytes!",
                        db_files[i], sizeof(Tmp_path) - 1) ;
        }

    }

    return(0) ;

/*END of Remove_database_files()*/
}

/****************************************************************************

   Description:
      Process command line arguments.

   Inputs:
      Argc - number of command line arguments.
      Argv - the command line arguments.

   Outputs:
      startup_action - start up action (CLEAN, CLEAR, or RESTART)
      input_file_path - path of terrain data file (excludes file name) 

   Returns:
      Exits with non-zero exit code on error, or returns 0 on success.

*****************************************************************************/
static int Get_command_line_args( int Argc, char **Argv, int *startup_action,
                                  char input_file_path[] ){

   extern char *optarg;
   extern int optind;
   int c, err, ret;
   char start_up[255];

   /* Initialize startup_action to RESTART and input_file_path to NULL. */
   *startup_action = RESTART;
   input_file_path[0] = 0;

   err = 0;
   while ((c = getopt (Argc, Argv, "ht:p:")) != EOF) {

      switch (c) {
  
         case 't':
            if( strlen( optarg ) < 255 ){    

               ret = sscanf(optarg, "%s", start_up) ;
               if (ret == EOF) {

                   LE_send_msg(GL_INFO, "sscanf failed to read startup action\n") ;
                   err = 1 ;

               }
               else{

                  if( strstr( start_up, "startup" ) != NULL )
                     *startup_action = STARTUP;

                  else if( strstr( start_up, "clear_hydro" ) != NULL )
                     *startup_action = CLEAR_HYDRO;

                  else if( strstr( start_up, "clear_gage" ) != NULL )
                     *startup_action = CLEAR_GAGE;

                  else if( strstr( start_up, "clear_blockage" ) != NULL )
                     *startup_action = CLEAR_BLOCKAGE;

                  else if( strstr( start_up, "restart" ) != NULL )
                     *startup_action = RESTART;

                  else if( strstr( start_up, "check" ) != NULL )
                     *startup_action = CHECK;

                  else
                     *startup_action = RESTART;

               }
            }
            else
               err = 1;

            break;
         case 'p':
            if( strlen( optarg ) < 255 ){    

               ret = sscanf(optarg, "%s", input_file_path) ;
               if (ret == EOF) {

                   LE_send_msg(GL_INFO, "sscanf failed to read LB pathname\n") ;
                   err = 1 ;

               }
               else
                  LE_send_msg( GL_INFO, "input_file_path = %s\n", input_file_path );

            }
            else{

               LE_send_msg( GL_INFO, "Input File Path Too Long\n" );
               err = 1;

            }
            break;
 
         case 'h':
         case '?':
         default:
            err = 1;
            break;
      }

   }

   if (err == 1 || ((input_file_path[0] == 0) && 
                    ((*startup_action == STARTUP) || (*startup_action == RESTART))) ){

      printf ("Usage: %s [options]\n", MISC_string_basename(Argv [0]));
      printf ("\toptions:\n");
      printf ("\t\t-h (print usage msg and exit)\n");
      printf ("\t\t-t Startup Action (optional - default: restart)\n" );
      printf ("\t\t-p Input File Path Name (required)\n" );
      exit (1);

   }
  
   return (0);

}

/******************************************************************************

   Description:
      Checks for empty hydromet files.  If the file is empty, it is removed.
      The files which are cheked are:

         HYGAGSCN.DAT
         HYPROD.DAT
         HYGRPAIR.DAT

   Inputs:

   Outputs:

   Returns:
      -1 on error, 0 on success.

   Notes:

******************************************************************************/
static int Check_files(){

   int ret, path_size;
   char *path_str = NULL, workpath[ORPG_PATHNAME_SIZ];
   struct stat stats;
   
   (void) memset(workpath, 0, sizeof(workpath)) ;
   ret = MISC_get_work_dir(workpath, sizeof(workpath)) ;
   if( ret < 0 ){

      LE_send_msg(GL_INFO, "MISC_get_work_dir() Failed (%d)", ret ) ;
      return(-1) ;

   }

   path_size = strlen( workpath ) + FILE_NAME_SIZE + 2; 
   path_str = (char *) calloc( (size_t) 1, (size_t) path_size );
   if( path_str == NULL ){

      LE_send_msg( GL_INFO, "malloc Failed For %d Bytes\n", path_size );
      return (-1);

   }

   /* Construct the path for the Gage Scan File. */
   memset( path_str, 0, path_size );
   strcat( path_str, workpath );
   path_str[ strlen(workpath) ] = '/';
   strcat( path_str, Gas_fn );

   LE_send_msg( GL_INFO, "The Gage Scan File Path:  %s\n", path_str );

   /* Check if the file has any bytes in it. */
   if( (ret = stat( path_str, &stats )) == -1 )
      LE_send_msg( GL_INFO, "Unable to stat %s (%d)\n", path_str, errno ) ;

   else if( stats.st_size == 0 ){

      /* File is empty.... Delete the file. */
      ret = remove( path_str );
      if( ret < 0 )
         LE_send_msg( GL_INFO, "Unable to Delete Empty File %s\n", path_str );

      else
         LE_send_msg( GL_INFO, "Empty File %s Deleted\n", path_str );

   }
   else
      LE_send_msg( GL_INFO, "File %s Has %d Bytes\n", path_str, stats.st_size );
   
   /* Construct the path for the Hydromet Product File. */
   memset( path_str, 0, path_size );
   strcat( path_str, workpath );
   path_str[ strlen(workpath) ] = '/';
   strcat( path_str, Hyprod_fn );

   LE_send_msg( GL_INFO, "The Hydromet Product File Path:  %s\n", path_str );

   /* Check if the file has any bytes in it. */
   if( (ret = stat( path_str, &stats )) == -1 )
      LE_send_msg( GL_INFO, "Unable to stat %s (%d)\n", path_str, errno ) ;

   else if( stats.st_size == 0 ){

      /* File is empty.... Delete the file. */
      ret = remove( path_str );
      if( ret < 0 )
         LE_send_msg( GL_INFO, "Unable to Delete Empty File %s\n", path_str );

      else
         LE_send_msg( GL_INFO, "Empty File %s Deleted\n", path_str );

   }
   else
      LE_send_msg( GL_INFO, "File %s Has %d Bytes\n", path_str, stats.st_size );

   /* Construct the path for the Gage Radar Pair File. */
   memset( path_str, 0, path_size );
   strcat( path_str, workpath );
   path_str[ strlen(workpath) ] = '/';
   strcat( path_str, Grp_fn );

   LE_send_msg( GL_INFO, "The Gage Radar Pair File Path:  %s\n", path_str );

   /* Check if the file has any bytes in it. */
   if( (ret = stat( path_str, &stats )) == -1 )
      LE_send_msg( GL_INFO, "Unable to stat %s (%d)\n", path_str, errno ) ;

   else if( stats.st_size == 0 ){

      /* File is empty.... Delete the file. */
      ret = remove( path_str );
      if( ret < 0 )
         LE_send_msg( GL_INFO, "Unable to Delete Empty File %s\n", path_str );

      else
         LE_send_msg( GL_INFO, "Empty File %s Deleted\n", path_str );

   }
   else
      LE_send_msg( GL_INFO, "File %s Has %d Bytes\n", path_str, stats.st_size );

   return (0);

}

/******************************************************************************

   Description:
      Reads adaptation data to get the names of the hydromet files.

   Inputs:

   Outputs:

   Returns:
      -1 on error, 0 on success.

   Notes:

*******************************************************************************/
static int Get_filenames(){

   int ret;
   char *buf = NULL, *str = NULL;
   Environ_t *environ;
   
   /* Read Adaptation data where all the file names reside. */
   ret = ORPGDA_read( ORPGDAT_ADAPTATION, &buf, LB_ALLOC_BUF,
                      ENVIRON ); 
   if( ret <= 0 ){

      LE_send_msg( GL_INFO, 
                   "ORPGDA_read of ORPGDAT_ADAPTATION (ENVIRON) Failed (%d)\n",
                   ret );
      return(-1);

   }

   /* Construct the file names. */
   environ = (Environ_t *) buf;

   /* Accumulation file name. */
   memcpy( Acum_fn, environ->acum_fn, FILE_NAME_SIZE );
   str = strchr( Acum_fn, (int) ' ' );
   if( str != NULL )
      *str = 0;
   else
      Acum_fn[FILE_NAME_SIZE] = 0;

   /* Gage Scan file name. */
   memcpy( Gas_fn, environ->gas_fn, FILE_NAME_SIZE );
   str = strchr( Gas_fn, (int) ' ' );
   if( str != NULL )
      *str = 0;
   else
      Gas_fn[FILE_NAME_SIZE] = 0;

   /* Hydromet Product file name. */
   memcpy( Hyprod_fn, environ->hyprod_fn, FILE_NAME_SIZE );
   str = strchr( Hyprod_fn, (int) ' ' );
   if( str != NULL )
      *str = 0;
   else
      Hyprod_fn[FILE_NAME_SIZE] = 0;

   /* Hydromet Product file name. */
   memcpy( Grp_fn, environ->grp_fn, FILE_NAME_SIZE );
   str = strchr( Grp_fn, (int) ' ' );
   if( str != NULL )
      *str = 0;
   else
      Grp_fn[FILE_NAME_SIZE] = 0;

   return (0);

} 

/***********************************************************************

   Description:
      Read data from an input file and use those data to write a given
      datastore message.

   Inputs:
      filepath  - Pointer to nul-terminated string representation of
                  the input file pathname.
   
      offset_bytes  - Offset (bytes) from start of input file at which
                      data transfer is to begin.
   
      data_id  - RPG Data ID of the datastore that is to be updated.
   
      msgid  - ID of the datastore message that is to be written to.

      byte_swap_func - A pointer to a byte swap function

   Returns:
      0 - number of bytes transferred from input data file
     -1 - failure

***********************************************************************/
static int File_to_dsmsg( const char *filepath, unsigned int offset_bytes,
                           int data_id, LB_id_t msgid, byte_swap_func_t swap_func){

    char *buf ;
    off_t f_size ;
    int fd ;
    size_t read_size ;
    int retval ;

    LE_send_msg(GL_INFO,
    "Data ID %d: use data file (offset %d bytes) to write to msgid %d ...",
                data_id, (int) offset_bytes, msgid) ;

    /*
     * Read the input file data ... write the data into the corresponding
     * datastore ...
     */
    errno = 0 ;
    fd = open(filepath, O_RDONLY, 0) ;
    if (fd < 0) {
        LE_send_msg(GL_ERROR,
                    "Data ID %d: Unable to open input data file: %d (errno %d)",
                    data_id, fd, errno) ;
        return(-1) ;
    }
    else {
        LE_send_msg(GL_ERROR,
                    "Data ID %d: Opened input data file ...", data_id) ;
    }

    errno = 0 ;
    f_size = lseek(fd, 0, SEEK_END) ;
    if (f_size == (off_t) -1) {
        LE_send_msg(GL_ERROR,
        "Data ID %d: Unable to determine size of input data file; lseek(): %d (errno %d)",
                    data_id, f_size, errno) ;
        (void) close(fd) ;
        return(-1) ;
    }
    else {
        LE_send_msg(GL_INFO,
                    "Data ID %d: input data file holds %d bytes of data",
                    data_id, (int) f_size) ;
    }

    retval = (int) lseek(fd, offset_bytes, SEEK_SET) ;
    if (retval != (int) offset_bytes) {
        LE_send_msg(GL_ERROR,
        "Data ID %d: Unable to seek to offset of %d bytes in input data file",
                    data_id, (int) offset_bytes) ;
        (void) close(fd) ;
        return(-1) ;
    }
    else {
        LE_send_msg(GL_INFO,
        "Data ID %d: seeked to offset of %d bytes in input data file",
                    data_id, (int) offset_bytes) ;
    }


    read_size = (size_t) (f_size - offset_bytes) ;
    buf = malloc(read_size) ;
    if (buf == NULL) {
        LE_send_msg(GL_ERROR,"Data ID %d: malloc(%d) failed!",
                    data_id, (int) read_size) ;
        (void) close(fd) ;
        return(-1) ;
    }

    /*
     * We ignore the fact that read() can be interrupted ...
     */
    retval = read(fd, buf, (unsigned int) read_size) ;
    if (retval != (int) read_size) {
        LE_send_msg(GL_ERROR,
                    "Data ID %d: read(%d bytes) from input data file failed: %d",
                    data_id, (int) read_size, retval) ;
        free(buf) ;
        (void) close(fd) ;
        return(-1) ;
    }
    else {
        LE_send_msg(GL_INFO,
                    "Data ID %d: read(%d bytes) from input data ...",
                    data_id, (int) read_size) ;
    }

    (void) close(fd) ;

#ifdef LITTLE_ENDIAN_MACHINE
    (*swap_func)(buf);
#endif

    /* We must take into account the possibility of data compression. */
    retval = ORPGDA_write(data_id, buf, read_size, msgid) ;
    if (retval < 0) {
        LE_send_msg(GL_ERROR,
                    "Data ID %d: ORPGDA_write(%d bytes to msgid %d) failed: %d",
                    data_id, (int) read_size, msgid, retval) ;
        free(buf) ;
        return(-1) ;
    }
    else {
        LE_send_msg(GL_INFO,
                    "Data ID %d: ORPGDA_write(%d bytes to msgid %d)",
                    data_id, (int) read_size, msgid) ;
    }

    free(buf) ;
    return((int) read_size) ;

/*END of File_to_dsmsg()*/
}

/*************************************************************************************

   Description:  Byte swap Hydromet Terrain data on a little endian machine
   Input/Output data - A pointer to Hydromet Terrain data

**************************************************************************************/
void byte_swap_terrain(void* data)
{
   int i;
   terrain_data_t *terrain = (terrain_data_t *) data;

   LE_send_msg(GL_INFO, "Swapping Terrain Data");

   for( i = 0; i < MAX_AZIMS; i++ )
   {
            MISC_swap_shorts(MAX_RANGE, &terrain->height[i][0] );
   }
}

/*****************************************************************************

   Description:
        Event handler to service mrpg event.

   Inputs:
      event - event code.
      msg - (optional) pointer to associated message.
      msg_len - length of optional message.

*****************************************************************************/   
void Event_handler(en_t event, void *msg, size_t msg_len ){


   Precipitation_algs_killed = 1;
   LE_send_msg( GL_INFO, "ORPGEVT_TERM_PRECIP_ALGS Event Received\n" );

}
