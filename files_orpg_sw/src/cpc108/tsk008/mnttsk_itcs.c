/*************************************************************************

   Module:  mnttsk_itcs.c


   Description:
      Maintenance Task: RPG Inter-Task Common files (ITCs)

 **************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2009/03/19 19:18:43 $
 * $Id: mnttsk_itcs.c,v 1.36 2009/03/19 19:18:43 steves Exp $
 * $Revision: 1.36 $
 * $State: Exp $
 */

#include <stdio.h>
#include <stdlib.h>            /* EXIT_SUCCESS, EXIT_FAILURE              */
#include <unistd.h>            /* unlink()                                */
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>


#include <orpg.h>

#include <rpg_port.h>
#include <itc.h>
#include <vcp.h>
#include <a309.h>
#include <rdacnt.h>
#include <orpgsite.h>
#include <orpgadpt.h>
 


/*
 * Constant Definitions/Macro Definitions/Type Definitions
 */
#define NAME_LEN    128
#define DEFAULT_VCP  21
#define LASTEL        9

#define STARTUP       1
#define RESTART       2
#define CLEAR         3

/*
 * Static Global Variables
 */


/*
 * Static Function Prototypes
 */
static void Write_cd07_bypassmap(int data_id);
static void Write_cd07_usp(int data_id, int startup_action);
static void Write_a304c2(int data_id);
static void Write_a3cd97(int data_id, int startup_action);
static void Write_model_ewt(int data_id, int startup_action);
static void Write_model_hail(int data_id, int startup_action);
static void Write_ewt_upt(int data_id, int startup_action);
static void Write_sgmts09(int data_id);
static void Write_a315csad(int data_id);
static void Write_a315lock(int data_id);
static void Write_a315trnd(int data_id);
static void Write_a3cd09(int data_id);
static void Write_pvecs09(int data_id);
static void Write_a317ctad(int data_id);
static void Write_a317lock(int data_id);
static void Write_a3cd11(int data_id);
static void Write_a3136c3(int data_id);
static int Get_command_line_args( int argc, char **argv,
                                  int *startup_action );

/**************************************************************************
   Description:
      ITC initialization routines.

   Input:
      argc - the number of command line arguments.
      argv - array of command line argument strings.

   Output:

   Returns:
      On normal execution, exit(0).  On abnormal execution, exit(negative number).

   Notes:
      ITC denotes "Inter-Task Common" or "Inter-Task Communication".  This is
      in support of legacy shared common.

 **************************************************************************/
int main(int argc, char *argv[]){

    int retval;
    int startup_action;

    /* Initialize log error services. */
    (void) ORPGMISC_init(argc, argv, 100, 0, -1, 0) ;

    /* Get command line arguments.  On failure, exit. */
    if( (retval = Get_command_line_args( argc, argv, &startup_action ) ) != 0)
       exit(retval);                                          

    LE_send_msg(GL_INFO,"INITIALIZATION DUTIES:") ;

    /*
     * ITC 100100 ...
     */
    if( startup_action == STARTUP ){

       LE_send_msg(GL_INFO, "Initializing ITC_100100.") ;
       Write_sgmts09(100100) ;
       Write_a315csad(100100) ;
       Write_a315lock(100100) ;
       Write_a315trnd(100100) ;
       Write_a3cd09(100100) ;
       Write_pvecs09(100100) ;
       Write_a317ctad(100100) ;
       Write_a317lock(100100) ;
       Write_a3cd11(100100) ;

    }

    /*
     * ITC 100200 ...  
     *
     * NOTE:  This ITC should be marked "persistent" in the data_tables.  It
     *        is considered state information.
     */
    if( (startup_action == CLEAR) || (startup_action == STARTUP) ){

       LE_send_msg(GL_INFO, "Initializing ITC_100200.") ;
       Write_cd07_usp(100200, startup_action) ;

    }

    /*
     * ITC 100400 ...
     */
    if( (startup_action == CLEAR) || (startup_action == STARTUP) ){

       LE_send_msg(GL_INFO, "Initializing ITC_100400 ... Part 1.") ;
       Write_a3cd97(100400, startup_action) ;

    }

    if( (startup_action == STARTUP) || (startup_action == CLEAR) ){

       LE_send_msg(GL_INFO, "Initializing ITC_100400 ... Part 2.") ;
       Write_model_ewt(100400, startup_action) ;
       Write_model_hail(100400, startup_action) ;
       Write_ewt_upt(100400, startup_action) ;

    }

    /*
     * ITC 100500 ...
     */
    if( startup_action == STARTUP ){

       LE_send_msg(GL_INFO, "Initializing ITC_100500.") ;
       Write_a3136c3(100500) ;

    }

    /*
     * ITC 100700 ...
     */
    if( startup_action == STARTUP ){

       LE_send_msg(GL_INFO, "Initializing ITC_100700.") ;
       Write_a304c2(100700) ;
       Write_cd07_bypassmap(100700) ;

    }

    exit(0) ;

/*END of main()*/
}


/********************************************************************

   Description:   
      Initializes SGMTS09 elements.

   Inputs:        
      data_id - Data id of Linear Buffer.

   Outputs:       

   Returns:
      There is no return value defined.

   Notes:
********************************************************************/
static void Write_sgmts09( int data_id )
{
   char *buf = NULL ;
   int buflen ;
   int retval ;
   sgmts09 *segments ;


   buflen = sizeof(sgmts09) ;
   buf = calloc((size_t) 1, (size_t) buflen) ;
   if (buf == NULL) {
      (void) perror("calloc") ;
      return ;
   }

   segments = (sgmts09 *) buf ; 
   segments->segbuf_th[0] = 1.5 ;
   segments->segbuf_th[1] = 3.5 ;
   segments->segbuf_th[2] = 10.0 ;
   segments->segbuf_th[3] = 16.0 ;
   segments->segbuf_th[4] = 21.0 ;
   segments->segbuf_th[5] = 24.0 ;
   segments->segbuf_th[6] = 24.0 ;

   retval = ORPGDA_write( data_id, buf, buflen, LBID_SGMTS09 ) ;
   if (retval != buflen) {
      LE_send_msg( GL_INFO,
                   "Data ID %d: ORPGDA_write(_SGMTS09) failed: %d", 
                   data_id, retval) ;
   }
   else {
      LE_send_msg( GL_INFO,
                   "Data ID %d: wrote %d bytes to msgid %d", 
                   data_id, retval, LBID_SGMTS09) ;
   }

   free( buf );

   return ;

/*END of Write_sgmts09()*/
}

/********************************************************************

   Description:   
      Initializes A315CSAD elements.

   Inputs:        
      data_id - Data id of Linear Buffer.

   Outputs:       

   Returns:
      There is no return value defined.

   Notes:
********************************************************************/
static void Write_a315csad( int data_id )
{
   char *buf = NULL ;
   int buflen ;
   int retval ;


   buflen = sizeof(a315csad) ;
   buf = calloc((size_t) 1, (size_t) buflen) ;
   if (buf == NULL) {
      (void) perror("calloc") ;
      return ;
   }

   retval = ORPGDA_write( data_id, buf, buflen, LBID_A315CSAD ) ;
   if (retval != buflen) {
      LE_send_msg( GL_INFO,
                   "Data ID %d: ORPGDA_write(_A315CSAD) failed: %d", 
                   data_id, retval) ;
   }
   else {
      LE_send_msg( GL_INFO,
                   "Data ID %d: wrote %d bytes to msgid %d", 
                   data_id, retval, LBID_A315CSAD) ;
   }

   free( buf );

   return ;

/*END of Write_a315csad()*/
}

/********************************************************************

   Description:   
      Initializes A315LOCK elements.

   Inputs:        
      data_id - Data id of Linear Buffer.

   Outputs:       

   Returns:
      There is no return value defined.

   Notes:
********************************************************************/
static void Write_a315lock( int data_id )
{
   char *buf = NULL ;
   int buflen ;
   int retval ;


   buflen = sizeof(a315lock) ;
   buf = calloc((size_t) 1, (size_t) buflen) ;
   if (buf == NULL) {
      (void) perror("calloc") ;
      return ;
   }

   retval = ORPGDA_write( data_id, buf, buflen, LBID_A315LOCK ) ;
   if (retval != buflen) {
      LE_send_msg( GL_INFO,
                   "Data ID %d: ORPGDA_write(_A315LOCK) failed: %d", 
                   data_id, retval) ;
   }
   else {
      LE_send_msg( GL_INFO,
                   "Data ID %d: wrote %d bytes to msgid %d", 
                   data_id, retval, LBID_A315LOCK) ;
   }

   free( buf );

   return ;

/*END of Write_a315lock()*/
}

/********************************************************************

   Description:   
      Initializes A315TRND elements.

   Inputs:        
      data_id - Data id of Linear Buffer.

   Outputs:       

   Returns:
      There is no return value defined.

   Notes:
********************************************************************/
static void Write_a315trnd( int data_id )
{
   char *buf = NULL ;
   int buflen ;
   int retval ;


   buflen = sizeof(a315trnd) ;
   buf = calloc((size_t) 1, (size_t) buflen) ;
   if (buf == NULL) {
      (void) perror("calloc") ;
      return ;
   }

   retval = ORPGDA_write( data_id, buf, buflen, LBID_A315TRND ) ;
   if (retval != buflen) {
      LE_send_msg( GL_INFO,
                   "Data ID %d: ORPGDA_write(_A315TRND) failed: %d", 
                   data_id, retval) ;
   }
   else {
      LE_send_msg( GL_INFO,
                   "Data ID %d: wrote %d bytes to msgid %d", 
                   data_id, retval, LBID_A315TRND) ;
   }

   free( buf );

   return ;

/*END of Write_a315trnd()*/
}

/********************************************************************

   Description:   
      Initializes A3CD09 elements.

   Inputs:        
      data_id - Data id of Linear Buffer.

   Outputs:       

   Returns:
      There is no return value defined.

   Notes:
********************************************************************/
static void Write_a3cd09( int data_id )
{
   char *buf = NULL ;
   int buflen ;
   int retval ;
   int i, j ;

   a3cd09 *storm_cell_data ;


   buflen = sizeof(a3cd09) ;
   buf = calloc((size_t) 1, (size_t) buflen) ;
   if (buf == NULL) {
      (void) perror("calloc") ;
      return ;
   }

   storm_cell_data = (a3cd09 *) buf;

   storm_cell_data->lokid = 0 ;
   storm_cell_data->avgstspd = 0.0 ;
   storm_cell_data->avgstdir = 0.0 ;
   storm_cell_data->timetag = 64696000.0 ;

   for( i = 0; i < NSTR_TOT; i++ ){

      storm_cell_data->strmid[i] = i + 1;

      for( j = 0; j < NSTR_MOV; j++ )
         storm_cell_data->strmove[i][j] = 0.0 ;

   }

   retval = ORPGDA_write( data_id, buf, buflen, LBID_A3CD09 ) ;
   if (retval != buflen) {
      LE_send_msg( GL_INFO,
                   "Data ID %d: ORPGDA_write(_A3CD09) failed: %d", 
                   data_id, retval) ;
   }
   else {
      LE_send_msg( GL_INFO,
                   "Data ID %d: wrote %d bytes to msgid %d", 
                   data_id, retval, LBID_A3CD09) ;
   }

   free( buf );

   return ;

/*END of Write_a3cd09()*/
}

/********************************************************************

   Description:   
      Initializes PVECS09 elements.

   Inputs:        
      data_id - Data id of Linear Buffer.

   Outputs:       

   Returns:
      There is no return value defined.

   Notes:
********************************************************************/
static void Write_pvecs09( int data_id )
{
   char *buf = NULL ;
   int buflen ;
   int retval ;

   buflen = sizeof(pvecs09) ;
   buf = calloc((size_t) 1, (size_t) buflen) ;
   if (buf == NULL) {
      (void) perror("calloc") ;
      return ;
   }

   retval = ORPGDA_write( data_id, buf, buflen, LBID_PVECS09 ) ;
   if (retval != buflen) {
      LE_send_msg( GL_INFO,
                   "Data ID %d: ORPGDA_write(_PVECS09) failed: %d", 
                   data_id, retval) ;
   }
   else {
      LE_send_msg( GL_INFO,
                   "Data ID %d: wrote %d bytes to msgid %d", 
                   data_id, retval, LBID_PVECS09) ;
   }

   free( buf );

   return ;

/*END of Write_pvecs09()*/
}

/********************************************************************

   Description:   
      Initializes A317CTAD elements.

   Inputs:        
      data_id - Data id of Linear Buffer.

   Outputs:       

   Returns:
      There is no return value defined.

   Notes:
********************************************************************/
static void Write_a317ctad( int data_id )
{
   char *buf = NULL ;
   int buflen ;
   int retval ;


   buflen = sizeof(a317ctad) ;
   buf = calloc((size_t) 1, (size_t) buflen) ;
   if (buf == NULL) {
      (void) perror("calloc") ;
      return ;
   }

   retval = ORPGDA_write( data_id, buf, buflen, LBID_A317CTAD ) ;
   if (retval != buflen) {
      LE_send_msg( GL_INFO,
                   "Data ID %d: ORPGDA_write(_A317CTAD) failed: %d", 
                   data_id, retval) ;
   }
   else {
      LE_send_msg( GL_INFO,
                   "Data ID %d: wrote %d bytes to msgid %d", 
                   data_id, retval, LBID_A317CTAD) ;
   }

   free( buf );

   return ;

/*END of Write_a317ctad()*/
}

/********************************************************************

   Description:   
      Initializes A317LOCK elements.

   Inputs:        
      data_id - Data id of Linear Buffer.

   Outputs:       

   Returns:
      There is no return value defined.

   Notes:
********************************************************************/
static void Write_a317lock( int data_id )
{
   char *buf = NULL ;
   int buflen ;
   int retval ;


   buflen = sizeof(a317lock) ;
   buf = calloc((size_t) 1, (size_t) buflen) ;
   if (buf == NULL) {
      (void) perror("calloc") ;
      return ;
   }

   retval = ORPGDA_write( data_id, buf, buflen, LBID_A317LOCK ) ;
   if (retval != buflen) {
      LE_send_msg( GL_INFO,
                   "Data ID %d: ORPGDA_write(_A317LOCK) failed: %d", 
                   data_id, retval) ;
   }
   else {
      LE_send_msg( GL_INFO,
                   "Data ID %d: wrote %d bytes to msgid %d", 
                   data_id, retval, LBID_A317LOCK) ;
   }

   free( buf );

   return ;

/*END of Write_a317lock()*/
}

/********************************************************************

   Description:   
      Initializes A3CD11 elements.

   Inputs:        
      data_id - Data id of Linear Buffer.

   Outputs:       

   Returns:
      There is no return value defined.

   Notes:
********************************************************************/
static void Write_a3cd11( int data_id )
{
   char *buf = NULL ;
   int buflen ;
   int retval ;


   buflen = sizeof(a3cd11) ;
   buf = calloc((size_t) 1, (size_t) buflen) ;
   if (buf == NULL) {
      (void) perror("calloc") ;
      return ;
   }

   retval = ORPGDA_write( data_id, buf, buflen, LBID_A3CD11 ) ;
   if (retval != buflen) {
      LE_send_msg( GL_INFO,
                   "Data ID %d: ORPGDA_write(_A3CD11) failed: %d", 
                   data_id, retval) ;
   }
   else {
      LE_send_msg( GL_INFO,
                   "Data ID %d: wrote %d bytes to msgid %d", 
                   data_id, retval, LBID_A3CD11) ;
   }

   free( buf );

   return ;

/*END of Write_a3cd11()*/
}

/********************************************************************

   Description:   
      Initializes A304C2 elements.  The elements are
      set as follows:

         1)  nw_map_request_pending - 0 (FALSE).
         2)  bypass_map_request_pending - 0 (FALSE).
         3)  unsolicited_nw_received - 0 (FALSE).

   Inputs:        
      data_id - Data ID of Linear Buffer.

   Outputs:       

   Returns:       
      There is no return value defined.

   Notes:
********************************************************************/
static void Write_a304c2( int data_id ){

   int ret;
   A304c2 a304c2_data;

   /*
     Initialize values.
   */
   a304c2_data.nw_map_request_pending = 0;
   a304c2_data.bypass_map_request_pending = 0;
   a304c2_data.unsolicited_nw_received = 0;

   /*
     Write A304C2 data to Linear Buffer.
   */
   ret = ORPGDA_write( data_id, (char *) &a304c2_data, 
                       sizeof(A304c2), LBID_A304C2 ); 
   if (ret != (int) sizeof(A304c2)) {
      LE_send_msg( GL_INFO,
                   "Data ID %d: ORPGDA_write(_A304C2) failed: %d", 
                   data_id, ret) ;
   }
   else {
      LE_send_msg( GL_INFO,
                   "Data ID %d: wrote %d bytes to msgid %d", 
                   data_id, ret, LBID_A304C2) ;
   }


/*END of Write_a304c2()*/
}

/********************************************************************

   Description:   
      Initializes CD07_BYPASSMAP elements.  The elements are
      set as follows:

         1)  first_spare - 0.
         2)  bm_gendate - 1 (Day 1 of modified Julian Calendar).
         3)  bm_gentime - 0 (Midnight of Day 1).
         4)  last_spare - 0.

   Inputs:        
      data_id - Data ID of Linear Buffer.

   Outputs:       

   Returns:       
      There is no return value defined.

   Notes:
********************************************************************/
static void Write_cd07_bypassmap( int data_id ){

   cd07_bypassmap bypassmap_data;
   int ret;

   /*
     Initialize values.
   */
   bypassmap_data.first_spare = 0;
   bypassmap_data.bm_gendate = 1;
   bypassmap_data.bm_gentime = 0;
   bypassmap_data.last_spare = 0;

   /*
     Write CD07_BYPASSMAP data to Linear Buffer.
   */
   ret = ORPGDA_write( data_id, (char *) &bypassmap_data, 
                       sizeof(cd07_bypassmap), LBID_CD07_BYPASSMAP ); 
   if (ret != (int) sizeof(cd07_bypassmap)) {
      LE_send_msg( GL_INFO,
                   "Data ID %d: ORPGDA_write(_CD07_BYPASSMAP) failed: %d", 
                   data_id, ret) ;
   }
   else {
      LE_send_msg( GL_INFO,
                   "Data ID %d: wrote %d bytes to msgid %d", 
                   data_id, ret, LBID_CD07_BYPASSMAP) ;
   }


/*END of Write_cd07_bypassmap()*/
}

/********************************************************************

   Description:   
      Initializes CD07_USP elements.  The elements are
      set as follows:

         1)  last_date_hrdb - -2.
         2)  last_time_hrdb - -2 

   Inputs:        
      data_id - Data ID of Linear Buffer.
      startup_action - startup action (STARTUP, RESTART, CLEAR)

   Outputs:       

   Returns:       
      There is no return value defined.

   Notes:
********************************************************************/
static void Write_cd07_usp( int data_id, int startup_action ){

   Hrdb_date_time usp_data;
   int ret;

   if( startup_action != CLEAR ){

      ret = ORPGDA_read( data_id, (char *) &usp_data, 
                         sizeof(Hrdb_date_time), LBID_CD07_USP ); 
      if (ret == (int) sizeof(Hrdb_date_time)) {

         LE_send_msg( GL_INFO, "CD07_USP Available ... Not Re-initialized\n" );

         LE_send_msg( GL_INFO, "--->LAST_DATE_HRDB: %d\n", usp_data.last_date_hrdb );
         LE_send_msg( GL_INFO, "--->LAST_TIME_HRDB: %d\n", usp_data.last_time_hrdb );
         return;

      }
      
   }

   /*
     Initialize values.
   */
   usp_data.last_date_hrdb = -2;
   usp_data.last_time_hrdb = -2;

   /*
     Write CD07_USP data to Linear Buffer.
   */
   ret = ORPGDA_write( data_id, (char *) &usp_data, 
                       sizeof(Hrdb_date_time), LBID_CD07_USP ); 
   if (ret != (int) sizeof(Hrdb_date_time)) {
      LE_send_msg( GL_INFO,
                   "Data ID %d: ORPGDA_write(_CD07_USP) failed: %d", 
                   data_id, ret) ;
   }
   else {

      LE_send_msg( GL_INFO,
                   "Data ID %d: wrote %d bytes to msgid %d", 
                   data_id, ret, LBID_CD07_USP) ;

      LE_send_msg( GL_INFO, "--->LAST_DATE_HRDB: %d\n", usp_data.last_date_hrdb );
      LE_send_msg( GL_INFO, "--->LAST_TIME_HRDB: %d\n", usp_data.last_time_hrdb );

   }


/*END of Write_cd07_usp()*/
}

/********************************************************************

   Description:   
      Initializes A3CD97 elements.  The following initial values
      are set:

      1)  envwndflg - 1 (Auto VAD update is TRUE).
      2)  ewtab[NPARMS][LEN_EWTAB] - All elements set to MTTABLE
                                     (Undefined).
      3)  nelems - 0.
      4)  basehgt - 0.
      5)  newndtab[LEN_EWTAB][NEPARAMS] - All elements set to
                                          MTTABLE_INT (Undefined).
      6)  sound_time - 0.

   Inputs:        
      data_id - Data ID of Linear Buffer.
      startup_action - either CLEAR, STARTUP or RESTART

   Outputs:       

   Returns:       
      There is no return value defined.

   Notes:
********************************************************************/
static void Write_a3cd97( int data_id, int startup_action ){

   int ret, i, j;
   A3cd97 env_wind_table_data;

   /* Do not initialize unless startup action is CLEAR or the
      message does not exist. */
   if( startup_action != CLEAR ){

      ret = ORPGDA_read( data_id, (char *) &env_wind_table_data, 
                         sizeof(A3cd97), LBID_A3CD97 ); 
      if (ret == (int) sizeof(A3cd97)){

         /*
           Table exists.  Do Nothing. 
         */
         LE_send_msg( GL_INFO, "EWT Data Available ... Bypass Initialization\n" );
         return;

      }

   }
     
   /*
     Initialize elements.
   */
   env_wind_table_data.envwndflg = 1;
   env_wind_table_data.sound_time = 0;

   /*
     Get the default weather mode for startup.
   */
   env_wind_table_data.basehgt = ORPGSITE_get_int_prop( ORPGSITE_RDA_ELEVATION );
   if( ORPGSITE_error_occurred() ){  

      env_wind_table_data.basehgt = 0;
      LE_send_msg( 0, "ORPGSITE_get_int_prop Of ORPGSITE_RDA_ELEVATION Failed\n" );
      LE_send_msg( 0, "Setting Default Base Height To 0\n" );
   
   }

   /*
     Initialize speed/direction array.
   */
   for( j = 0; j < NPARMS; j++ ){

      for( i = 0; i < LEN_EWTAB; i++ )
         env_wind_table_data.ewtab[j][i] = MTTABLE;

   }

   /*
     Initialize component array.
   */
   for( j = 0; j < LEN_EWTAB; j++ ){

      for( i = 0; i < NEPARAMS; i++ )
         env_wind_table_data.newndtab[j][i] = MTTABLE_INT;

   }

   /*
     Write a3cd97 data to Linear Buffer.
   */
   ret = ORPGDA_write( data_id, (char *) &env_wind_table_data, 
                       sizeof(A3cd97), LBID_A3CD97 ); 
   if (ret != (int) sizeof(A3cd97)) {
      LE_send_msg( GL_INFO,
                   "Data ID %d: ORPGDA_write(_A3CD97) failed: %d", 
                   data_id, ret) ;
   }
   else {
      LE_send_msg( GL_INFO,
                   "Data ID %d: wrote %d bytes to msgid %d", 
                   data_id, ret, LBID_A3CD97) ;
   }


/*END of Write_a3cd97()*/
}

/********************************************************************

   Description:   
      Initializes MODEL_EWT elements.  The following initial values
      are set:

      1)  envwndflg - 1 (Auto VAD update is TRUE).
      2)  ewtab[NPARMS][LEN_EWTAB] - All elements set to MTTABLE
                                     (Undefined).
      3)  nelems - 0.
      4)  basehgt - 0.
      5)  newndtab[LEN_EWTAB][NEPARAMS] - All elements set to
                                          MTTABLE_INT (Undefined).
      6)  sound_time - 0.

   Inputs:        
      data_id - Data ID of Linear Buffer.
      startup_action - Startup action.

   Outputs:       

   Returns:       
      There is no return value defined.

   Notes:
********************************************************************/
static void Write_model_ewt( int data_id, int startup_action ){

   int ret, i, j;
   A3cd97 env_wind_table_data;

   /* Do not initialize unless startup action is CLEAR or the
      message does not exist. */
   if( startup_action != CLEAR ){

      ret = ORPGDA_read( data_id, (char *) &env_wind_table_data, 
                         sizeof(A3cd97), LBID_MODEL_EWT ); 
      if (ret == (int) sizeof(A3cd97)) {

         LE_send_msg( GL_INFO, "Model EWT Data Available ... Bypass Initialization\n" );
         return;

      }

   }

   /*
     Initialize elements.
   */
   env_wind_table_data.envwndflg = 1;
   env_wind_table_data.sound_time = 0;

   /*
     Get the default weather mode for startup.
   */
   env_wind_table_data.basehgt = ORPGSITE_get_int_prop( ORPGSITE_RDA_ELEVATION );
   if( ORPGSITE_error_occurred() ){  

      env_wind_table_data.basehgt = 0;
      LE_send_msg( 0, "ORPGSITE_get_int_prop Of ORPGSITE_RDA_ELEVATION Failed\n" );
      LE_send_msg( 0, "Setting Default Base Height To 0\n" );
   
   }

   /*
     Initialize speed/direction array.
   */
   for( j = 0; j < NPARMS; j++ ){

      for( i = 0; i < LEN_EWTAB; i++ )
         env_wind_table_data.ewtab[j][i] = MTTABLE;

   }

   /*
     Initialize component array.
   */
   for( j = 0; j < LEN_EWTAB; j++ ){

      for( i = 0; i < NEPARAMS; i++ )
         env_wind_table_data.newndtab[j][i] = MTTABLE_INT;

   }

   /*
     Write a3cd97 data to Linear Buffer.
   */
   ret = ORPGDA_write( data_id, (char *) &env_wind_table_data, 
                       sizeof(A3cd97), LBID_MODEL_EWT ); 
   if (ret != (int) sizeof(A3cd97)) {
      LE_send_msg( GL_INFO,
                   "Data ID %d: ORPGDA_write(_A3CD97) failed: %d", 
                   data_id, ret) ;
   }
   else {
      LE_send_msg( GL_INFO,
                   "Data ID %d: wrote %d bytes to msgid %d", 
                   data_id, ret, LBID_MODEL_EWT) ;
   }


/*END of Write_model_ewt()*/
}

/********************************************************************

   Description:   
      Initializes EWT_UPT elements.  The following initial values
      are set:

      1)  flag - 1 (Allow Model Updates to A3CD97)

   Inputs:        
      data_id - Data ID of Linear Buffer.
      startup_action - Startup action.

   Outputs:       

   Returns:       
      There is no return value defined.

   Notes:
********************************************************************/
static void Write_model_hail( int data_id, int startup_action ){

   int ret;
   Hail_temps_t hail;
   double value;
   char buf[64];

   if( startup_action != CLEAR ){

      ret = ORPGDA_read( data_id, (char *) &hail, 
                         sizeof(Hail_temps_t), LBID_MODEL_HAIL ); 
      if (ret == (int) sizeof(Hail_temps_t)) {

         LE_send_msg( GL_INFO, "Model Hail Temperature Data Available ... Bypass Initialization\n" );
         return;

      }

   }

   /*
     Initialize elements.
   */
   sprintf( buf, "alg.hail.height_0" );
   if( DEAU_get_values( buf, &value, 1 ) > 0 )
      hail.height_0 = (float) value;

   sprintf( buf, "alg.hail.height_minus_20" );
   if( DEAU_get_values( buf, &value, 1 )  > 0 )
      hail.height_minus_20 = (float) value;

   sprintf( buf, "alg.hail.hail_date_yy" );
   if( DEAU_get_values( buf, &value, 1 )  > 0 )
      hail.hail_date_yy = (int) value;

   sprintf( buf, "alg.hail.hail_date_mm" );
   if( DEAU_get_values( buf, &value, 1 )  > 0 )
      hail.hail_date_mm = (int) value;

   sprintf( buf, "alg.hail.hail_date_dd" );
   if( DEAU_get_values( buf, &value, 1 )  > 0 )
      hail.hail_date_dd = (int) value;

   sprintf( buf, "alg.hail.hail_time_hr" );
   if( DEAU_get_values( buf, &value, 1 )  > 0 )
      hail.hail_time_hr = (int) value;

   sprintf( buf, "alg.hail.hail_time_min" );
   if( DEAU_get_values( buf, &value, 1 )  > 0 )
      hail.hail_time_min = (int) value;

   sprintf( buf, "alg.hail.hail_time_sec" );
   if( DEAU_get_values( buf, &value, 1 )  > 0 )
      hail.hail_time_sec = (int) value;

   /*
     Write Hail_temps_t data to Linear Buffer.
   */
   ret = ORPGDA_write( data_id, (char *) &hail, sizeof(Hail_temps_t), 
                       LBID_MODEL_HAIL ); 
   if (ret != (int) sizeof(Hail_temps_t)) {
      LE_send_msg( GL_INFO,
                   "Data ID %d: ORPGDA_write(MODEL_HAIL) failed: %d", 
                   data_id, ret) ;
   }
   else {
      LE_send_msg( GL_INFO,
                   "Data ID %d: wrote %d bytes to msgid %d", 
                   data_id, ret, LBID_MODEL_HAIL) ;
   }


/*END of Write_model_hail()*/
}

/********************************************************************

   Description:   
      Initializes EWT_UPT elements.  The following initial values
      are set:

      1)  flag - 1 (Allow Model Updates to A3CD97)

   Inputs:        
      data_id - Data ID of Linear Buffer.
      startup_action - Startup action.

   Outputs:       

   Returns:       
      There is no return value defined.

   Notes:
********************************************************************/
static void Write_ewt_upt( int data_id, int startup_action ){

   int ret;
   EWT_update_t update;

   if( startup_action != CLEAR ){

      ret = ORPGDA_read( data_id, (char *) &update, 
                         sizeof(EWT_update_t), LBID_EWT_UPT ); 
      if (ret == (int) sizeof(EWT_update_t)) {

         LE_send_msg( GL_INFO, "A3CD97 Update Flag Available ... Bypass Initialization\n" );
         return;

      }

   }

   /*
     Initialize elements.
   */
   update.flag = 1;

   /*
     Write EWT_update_t data to Linear Buffer.
   */
   ret = ORPGDA_write( data_id, (char *) &update, sizeof(EWT_update_t), 
                       LBID_EWT_UPT ); 
   if (ret != (int) sizeof(EWT_update_t)) {
      LE_send_msg( GL_INFO,
                   "Data ID %d: ORPGDA_write(EWT_UPT) failed: %d", 
                   data_id, ret) ;
   }
   else {
      LE_send_msg( GL_INFO,
                   "Data ID %d: wrote %d bytes to msgid %d", 
                   data_id, ret, LBID_EWT_UPT) ;
   }


/*END of Write_ewt_upt()*/
}

/********************************************************************

   Description:   
      Initializes A3136C3 elements.  All items are initialize to 0.

   Inputs:        
      data_id - Data ID of Linear Buffer.

   Outputs:       

   Returns:       
      There is no return value defined.

   Notes:
********************************************************************/
static void Write_a3136c3( int data_id ){

   int ret;
   A3136C3_t *a3136c3 = NULL;

   /*
     Initialize elements.
   */
   a3136c3 = calloc( (size_t) 1, sizeof( A3136C3_t) );
   if( a3136c3 == NULL ){

      LE_send_msg( 0, "ITC Init Of LBID_A3136C3 Failed\n" ); 
      return;

   }

   /*
     Write a3136C3 data to Linear Buffer.
   */
   ret = ORPGDA_write( data_id, (char *) a3136c3,
                       sizeof(A3136C3_t), LBID_A3136C3 ); 
   if (ret != (int) sizeof(A3136C3_t)) {
      LE_send_msg( GL_INFO,
                   "Data ID %d: ORPGDA_write(_A3136C3) failed: %d", 
                   data_id, ret) ;
   }
   else {
      LE_send_msg( GL_INFO,
                   "Data ID %d: wrote %d bytes to msgid %d", 
                   data_id, ret, LBID_A3136C3) ;
   }


   free( a3136c3 );

   return;

/*END of Write_a3136c3()*/
}

/****************************************************************************

   Description:
      Process command line arguments.

   Inputs:
      Argc - number of command line arguments.
      Argv - the command line arguments.

   Outputs:
      startup_action - start up action (STARTUP, CLEAR, or RESTART)

   Returns:
      exits on error, or returns 0 on success.

*****************************************************************************/
static int Get_command_line_args( int Argc, char **Argv, int *startup_action ){

   extern char *optarg;
   extern int optind;
   int c, err, ret;
   char start_up[255];

   /* Initialize startup_action to RESTART and input_file_path to NULL. */
   *startup_action = RESTART;

   err = 0;
   while ((c = getopt (Argc, Argv, "ht:")) != EOF) {

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

                  else if( strstr( start_up, "clear" ) != NULL )
                     *startup_action = CLEAR;

                  else if( strstr( start_up, "restart" ) != NULL )
                     *startup_action = RESTART;

                  else
                     *startup_action = RESTART;

               }
            }
            else
               err = 1;

            break;
 
         case 'h':
         case '?':
         default:
            err = 1;
            break;
      }

   }

   if (err == 1 ){

      printf ("Usage: %s [options]\n", MISC_string_basename(Argv [0]));
      printf ("\toptions:\n");
      printf ("\t\t-h (print usage msg and exit)\n");
      printf ("\t\t-t Startup Action (optional - default: RESTART)\n" );
      exit (1);

   }
  
   return (0);

}
