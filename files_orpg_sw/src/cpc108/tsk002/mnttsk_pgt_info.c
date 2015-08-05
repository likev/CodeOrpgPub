/*************************************************************************

   Module:  mnttsk_pgt_info.c

   Description:
      Maintenance Task:  Product Generation Tables


 **************************************************************************/
/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2012/12/04 16:35:28 $
 * $Id: mnttsk_pgt_info.c,v 1.17 2012/12/04 16:35:28 steves Exp $
 * $Revision: 1.17 $
 * $State: Exp $
 */

#include <stdio.h>
#include <stdlib.h>            /* EXIT_SUCCESS, EXIT_FAILURE              */
#include <unistd.h>            /* unlink()                                */
#include <string.h>            /* strncpy()                               */
#include <dirent.h>


#include "mnttsk_pgt_def.h"
#include <orpgsite.h>
#include <orpg.h>
#include <a309.h>


/*
 * Constant Definitions/Macro Definitions/Type Definitions
 */
#define SURCHAN_LO             2
#define SURCHAN_HI             4
#define DOPCHAN_LO             3
#define DOPCHAN_HI             5

#define SEGMENT_1              2
#define SEGMENT_2              4
#define SEGMENT_3              8
#define SEGMENT_4             16
#define SEGMENT_5             32

/*
 * Global Variables
 */
extern char Product_gen_tables_name[NAME_SIZE];
extern char Product_gen_tables_basename[NAME_SIZE];
extern char Cfg_extensions_name[CFG_NAME_SIZE];
extern int  Rda_config;

/*
 * Static Global Variables
 */
#define BUF_SIZE       128000
static int Aligned_Buf[BUF_SIZE]; 
static char *Buf = (char *) Aligned_Buf; 

/*
 * Static Function Prototypes
 */
static int Init_pgt_tbl ( int table_id );
static int Read_pgt_tbl ( char *table_name );
static int Read_params (int prod_id, short *params);
static int Process_elevation_parameter( char *tmp, short *param );
static void Process_cfcprod( Pd_prod_entry *item, int table );
static int Get_next_file_name( char *dir_name, char *basename,
                               char *buf, int buf_size );

/********************************************************************
   Description:
      Initialize product generation tables.

   Inputs:
      startup_action - start up type.
      init_tables - bit map of PGT(s) to initialize.

   Outputs:

   Returns:
      Negative value on error, or 0 on success.
      
   Notes:

********************************************************************/
int MNTTSK_PGT_INFO_maint(int startup_action, int init_tables){

    int retval = 0;
    char *buf;

    /* Open the ORPGDAT_PROD_INFO LB with "write" permission. */
    if( ORPGDA_open( ORPGDAT_PROD_INFO, LB_WRITE ) < 0){

        LE_send_msg( GL_ERROR, "Unable to Open LB(s) With WRITE Permission\n" );
        exit(1);

    }

    /* Clear */
    if (startup_action == CLEAR) {

        LE_send_msg(GL_INFO,
                    "CLEAR: Prod. Gen. Table. Maint. INIT FOR START DUTIES:") ;
        LE_send_msg(GL_INFO,
                    "\t1. Initialize Product Generation Table(s).") ;

        if( init_tables == ALL_TBLS )
           retval = Init_pgt_tbl( ALL_TBLS );

        else if( (init_tables & CURRENT) )
           retval = Init_pgt_tbl( CURRENT );

        else if( (init_tables & DEFAULT_A) )
           retval = Init_pgt_tbl( DEFAULT_A );

        else if( (init_tables & DEFAULT_B) )
           retval = Init_pgt_tbl( DEFAULT_B );

        if (retval != 0){

            LE_send_msg(GL_INFO, "Data IDs %d: Init_pgt_tbl Failed: %d",
                        ORPGDAT_PROD_INFO, retval) ;
            return(-1) ;

        }
        else
            LE_send_msg(GL_INFO,"Product Generation Table(s) Initialized\n" );


    }
    else {

        /* Startup or Restart .... */
        LE_send_msg(GL_INFO,
                    "STARTUP: Prod. Dist. Info. Maint. INIT FOR START DUTIES:") ;
        LE_send_msg(GL_INFO,
                    "\t1. Initialize Product Generation Lists (ORPGDAT_PROD_INFO).") ;

	/* If the product generation tables exist do not initialize	*/
	retval = ORPGDA_read (ORPGDAT_PROD_INFO, &buf, LB_ALLOC_BUF,
			      PD_CURRENT_PROD_MSG_ID);

	if (retval > 0)
	    free (buf);

	else {

	    retval = Init_pgt_tbl( CURRENT );
	    if (retval != 0){

		LE_send_msg(GL_INFO, "Data ID %d: Init_pgt_tbl Failed: %d\n",
                            ORPGDAT_PROD_INFO, retval) ;
                return(-1) ;

	    }
            else 
		LE_send_msg(GL_INFO,"Data ID %d Current Generation List Initialized",
                            ORPGDAT_PROD_INFO) ;

	}

	retval = ORPGDA_read (ORPGDAT_PROD_INFO, &buf, LB_ALLOC_BUF,
			      PD_DEFAULT_A_PROD_MSG_ID);

	if (retval > 0)
	    free (buf);

	else {

	    retval = Init_pgt_tbl( DEFAULT_A );
	    if (retval != 0){

		LE_send_msg(GL_INFO, "Data ID %d: Init_pgt_tbl Failed: %d",
                            ORPGDAT_PROD_INFO, retval) ;
                return(-1) ;

	    }
            else 
		LE_send_msg(GL_INFO,"Data ID %d Default A Generation List Initialized",
                            ORPGDAT_PROD_INFO) ;

	}

	retval = ORPGDA_read (ORPGDAT_PROD_INFO, &buf, LB_ALLOC_BUF,
			      PD_DEFAULT_B_PROD_MSG_ID);

	if (retval > 0)
	    free (buf);

	else {

	    retval = Init_pgt_tbl( DEFAULT_B );
	    if (retval != 0){

		LE_send_msg(GL_INFO, "Data ID %d: Init_pgt_tbl Failed: %d",
                            ORPGDAT_PROD_INFO, retval) ;
                return(-1) ;

	    }
            else 
		LE_send_msg(GL_INFO,"Data ID %d: Default B Generation List Initialized",
                            ORPGDAT_PROD_INFO) ;

	}

    }

    return(0) ;

/*END of MNTTSK_PGT_INFO_maint()*/
}

/*************************************************************************

   Description: 
      This function initializes the default product generation table.

   Inputs:
      table_id - the ID of the table to initialize.  The IDs are defined
      in header file orpgpgt.h.

   Outputs:

   Returns:	
       Returns the number of entries in the table on success or -1 
       on failure.

*************************************************************************/
static int Init_pgt_tbl ( int table_id ){

    int	err;
    int def_wx_mode;

    char ext_name[ CFG_NAME_SIZE ], *call_name;

/*  We need to initialize the memory for the product attributes		*
 *  table first since we are using ORPGPGT and ORPGPAT library		*
 *  functions to build the generation tables.  ORPGPAT needs to		*
 *  issue an ORPGDA_info call to determine the amount of memory		*
 *  to allocate for the attributes table.  If we don't do this		*
 *  now, then CS will have a problem since only one configuration	*
 *  file can be open at a time.						*/


    switch (table_id){

       case ALL_TBLS:
          LE_send_msg (GL_INFO, "Initializing ALL Product Generation Tables\n");
          ORPGPGT_clear_tbl (ORPGPGT_CURRENT_TABLE);
          ORPGPGT_clear_tbl (ORPGPGT_DEFAULT_A_TABLE);
          ORPGPGT_clear_tbl (ORPGPGT_DEFAULT_B_TABLE);
          break;

       case CURRENT:
          LE_send_msg (GL_INFO, "Initializing CURRENT Product Generation Table\n");
          ORPGPGT_clear_tbl (ORPGPGT_CURRENT_TABLE);
          break;

       case DEFAULT_A:
          LE_send_msg (GL_INFO, "Initializing DEFAULT A Product Generation Table\n");
          ORPGPGT_clear_tbl (ORPGPGT_DEFAULT_A_TABLE);
          break;

       case DEFAULT_B:
          LE_send_msg (GL_INFO, "Initializing DEFAULT B Product Generation Table\n");
          ORPGPGT_clear_tbl (ORPGPGT_DEFAULT_B_TABLE);
          break;

       default:
          LE_send_msg (GL_INFO, "Initializing ALL Product Generation Tables\n");
          ORPGPGT_clear_tbl (ORPGPGT_CURRENT_TABLE);
          ORPGPGT_clear_tbl (ORPGPGT_DEFAULT_A_TABLE);
          ORPGPGT_clear_tbl (ORPGPGT_DEFAULT_B_TABLE);
    }

    /*  We need to have the product attributes table initialized.  */
    err = ORPGPAT_read_tbl ();
    if (err < 0) {

	LE_send_msg (GL_INFO, "Need to initialize PAT first\n");
        exit(1);

    }
    else
       LE_send_msg(GL_INFO, "PAT Read Successfully\n" );

    /* Read the Default Product Generation Tables. */
    if( Read_pgt_tbl( Product_gen_tables_name ) < 0 ){

       LE_send_msg( GL_INFO, "Unable To Read PGT From Product Tables\n" );
       exit(1);

    }
    else
       LE_send_msg( GL_INFO, "PGT Read Successfully\n" );

    /* If no error, process all the product generation table extensions. */
    call_name = Cfg_extensions_name;
    while( Get_next_file_name( call_name, Product_gen_tables_basename,
                               ext_name, CFG_NAME_SIZE ) == 0 ){

       LE_send_msg( GL_INFO, "---> Reading Product Tables Extension %s\n",
                    ext_name );
       if( Read_pgt_tbl( ext_name ) < 0 ){

          LE_send_msg( GL_ERROR, "Unable To Read PGT Extension\n" );
          break;

       }
       call_name = NULL;

    }

    /*  Write the different tables to the product LB. */
    switch( table_id ){

       case DEFAULT_A:
          LE_send_msg( GL_INFO, "Writing Default A Table\n" );
          ORPGPGT_write_tbl (ORPGPGT_DEFAULT_A_TABLE);
          break;

       case DEFAULT_B:
          LE_send_msg( GL_INFO, "Writing Default B Table\n" );
          ORPGPGT_write_tbl (ORPGPGT_DEFAULT_B_TABLE);
          break;

       case ALL_TBLS:
	
          LE_send_msg( GL_INFO, "Writing Default A, B, M Tables\n" );
          ORPGPGT_write_tbl (ORPGPGT_DEFAULT_A_TABLE);
          ORPGPGT_write_tbl (ORPGPGT_DEFAULT_B_TABLE);
          break;

       default:
          break;

    }

    if( table_id == CURRENT ){
 
 /*    Read the site adaptation data to determine the default weather	*
  *    mode, and copy the default table into current.  If the default	*
  *    weather mode cannot be determined, assume mode A.		*/

       def_wx_mode = ORPGSITE_get_int_prop( ORPGSITE_WX_MODE );
       if( ORPGSITE_error_occurred() )        
          ORPGPGT_replace_tbl  (ORPGPGT_DEFAULT_A_TABLE, ORPGPGT_CURRENT_TABLE);

       else{

          switch( def_wx_mode ){

             case PRECIPITATION_MODE:
	        ORPGPGT_replace_tbl  (ORPGPGT_DEFAULT_A_TABLE, ORPGPGT_CURRENT_TABLE);
                break;

             case CLEAR_AIR_MODE:
	        ORPGPGT_replace_tbl  (ORPGPGT_DEFAULT_B_TABLE, ORPGPGT_CURRENT_TABLE);
                break;

             default:
	        ORPGPGT_replace_tbl  (ORPGPGT_DEFAULT_A_TABLE, ORPGPGT_CURRENT_TABLE);

          }
		
       }

    }

    return (0);

}

/*************************************************************************

   Description: 
      This function initializes the default product generation table 
      from the ASCII default product generation table found in configuration
      file "product_tables".

   Inputs:

   Outputs:

   Returns:	 
      It returns the number of entries in the table on success or -1 
      on failure.

*************************************************************************/
static int Read_pgt_tbl ( char *table_name ){

    Pd_prod_entry *item;
    int	cnt, err = 0 ;
    int	i, ndx;
    int	cnt_A, cnt_B, cnt_M;
    int ret1 = 0, ret2 = 0;

    CS_cfg_name ("");
    CS_cfg_name (table_name);
    CS_control (CS_COMMENT | '#');
    CS_control (CS_RESET);

    LE_send_msg( GL_INFO, "Adding Products for Table: %s\n", table_name );

    if ( (ret1 = CS_entry ("Default_prod_gen", 0, 0, NULL)) < 0 ||
	 (ret2 = CS_level (CS_DOWN_LEVEL) < 0)){

        if( ret1 < 0 )
           LE_send_msg( GL_INFO, "CS_entry returned %d\n", ret1 );
        else
           LE_send_msg( GL_INFO, "CS_level returned %d\n", ret2 );

        LE_send_msg( GL_INFO, "Error \"Default_prod_gen\" \n" );

	return (-1);

    }

    cnt = cnt_A = cnt_B = cnt_M = err = 0;
    item = (Pd_prod_entry *)(Buf);

    do {

	if (CS_entry (CS_THIS_LINE, CS_SHORT, 0,
				(void *)&(item[cnt].prod_id)) > 0 &&
	    CS_entry (CS_THIS_LINE, 1 | CS_BYTE, 0,
				(void *)&(item[cnt].wx_modes)) > 0 &&
	    CS_entry (CS_THIS_LINE, 2 | CS_BYTE, 0,
				(void *)&(item[cnt].gen_pr)) > 0 &&
	    CS_entry (CS_THIS_LINE, 3 | CS_INT, 0,
				(void *)&(item[cnt].stor_retention)) > 0 &&
	    Read_params ((int) item[cnt].prod_id, item[cnt].params) == 0) {

	    item[cnt].req_num = 0;

/*	    Use the wx_modes field to determine which default tables	*
 *	    will include this entry.  If an entry is to me added, use	*
 *	    the ORPGPGT_add_entry () function to first create a new	*
 *	    entry and the fill in the fields with the appropriate	*
 *	    ORPGPGT put functions.  Memory allocation is handled	*
 *	    internally by the ORPGPGT library.				*/

	    if (item [cnt].wx_modes & 0x02) {	/*  Wx Mode A		*/

/*		We need to treat the CFC Product specially.  If ALL	*
 *		is set, then we need to create multiple entries in tbl.	*/

		if ((item[cnt].prod_id == CFCPROD) &&
		    (item[cnt].params[0] == PARAM_ALL_VALUES)) {

		    Process_cfcprod( &item[cnt], ORPGPGT_DEFAULT_A_TABLE );

		} else {

		    ndx = ORPGPGT_add_entry (ORPGPGT_DEFAULT_A_TABLE);

		    ORPGPGT_set_prod_id             (ORPGPGT_DEFAULT_A_TABLE,
					   ndx, (int) item [cnt].prod_id);
		    ORPGPGT_set_generation_interval (ORPGPGT_DEFAULT_A_TABLE,
					   ndx, (int) item [cnt].gen_pr);
	    	    ORPGPGT_set_retention_period    (ORPGPGT_DEFAULT_A_TABLE,
					   ndx, (int) item [cnt].stor_retention);

		    for (i=0;i<NUM_PROD_DEPENDENT_PARAMS;i++) {

		        ORPGPGT_set_parameter (ORPGPGT_DEFAULT_A_TABLE,
					   ndx, i, item [cnt].params [i]);
		    }

		}

		cnt_A++;

	    }

	    if (item [cnt].wx_modes & 0x04) {	/*  Wx Mode B		*/

/*		We need to treat the CFC Product specially.  If ALL	*
 *		is set, then we need to create multiple entries in tbl.	*/

		if ((item[cnt].prod_id == CFCPROD) &&
		    (item[cnt].params[0] == PARAM_ALL_VALUES)) {

		    Process_cfcprod( &item[cnt], ORPGPGT_DEFAULT_B_TABLE );

		} else {

		    ndx = ORPGPGT_add_entry (ORPGPGT_DEFAULT_B_TABLE);

		    ORPGPGT_set_prod_id             (ORPGPGT_DEFAULT_B_TABLE,
					   ndx, (int) item [cnt].prod_id);
		    ORPGPGT_set_generation_interval (ORPGPGT_DEFAULT_B_TABLE,
					   ndx, (int) item [cnt].gen_pr);
	    	    ORPGPGT_set_retention_period    (ORPGPGT_DEFAULT_B_TABLE,
					   ndx, (int) item [cnt].stor_retention);

		    for (i=0;i<NUM_PROD_DEPENDENT_PARAMS;i++) {

		        ORPGPGT_set_parameter (ORPGPGT_DEFAULT_B_TABLE,
					   ndx, i, item [cnt].params [i]);
		    }

		}

		cnt_B++;

	    }

	    cnt++;
	}
	else {
	    err = 1;
	    break;
	}
    } while (CS_entry (CS_NEXT_LINE, 0, 0, NULL) >= 0);

    CS_cfg_name ("");

    if (err)
	return (-1);

    else
	return (cnt);

}

/**************************************************************************

   Description: 
      This function reads and parses the six product parameters.

   Inputs:
      params - array of short to hold the six product dependent parameters.

   Output:

   Returns:	
      It returns 0 on success or -1 on failure.

**************************************************************************/

#define TBUF_SIZE 16

static int Read_params (int prod_id, short *params){

    char tmp[TBUF_SIZE];
    int elev_ind, i;

    /* If this product is elevation-based, save the parameter index. */
    elev_ind = ORPGPAT_elevation_based( prod_id );

    /* Initialize the params array. */
    memset( params, 0, sizeof(short)*6 );

    for (i = 0; i < 6; i++) {
	if (CS_entry (CS_THIS_LINE, i + 4, TBUF_SIZE, (void *)tmp) > 0) {

	    int v;

	    if (strcmp (tmp, "UNU") == 0)
		params[i] = PARAM_UNUSED;
	    else if (strcmp (tmp, "ANY") == 0)
		params[i] = PARAM_ANY_VALUE;
	    else if (strcmp (tmp, "ALG") == 0)
		params[i] = PARAM_ALG_SET;
	    else if (strcmp (tmp, "ALL") == 0)
		params[i] = PARAM_ALL_VALUES;
	    else if (strcmp (tmp, "EXS") == 0)
		params[i] = PARAM_ALL_EXISTING;
	    else{

                /* If this product is elevation-based, then the elevation parameter
                   has special format.  If product not elevtion-based, elev_ind
                   is negative value. */
		if( elev_ind == i ){

                   if( Process_elevation_parameter( tmp, &params[i] ) < 0 )
                      return (-1); 

                   LE_send_msg( GL_INFO, "--->Elevation Parameter For Product %d (%s): %x\n",
                                prod_id, tmp, params[i] );

		}
		else{ 
           
                   if (sscanf (tmp, "%d", &v) == 1)
	  	      params[i] = v;
                   else{

                      LE_send_msg( GL_ERROR, "!!!!!!sscanf Failed\n" );
		      return (-1);

                   }

                }

	    }
	}
	else{

            LE_send_msg( GL_ERROR, "!!!!!!CS_entry Failed\n" );
	    return (-1);

        }

    }

    return (0);
}

/**************************************************************************

   Description: 
      This function processes the elevation parameter.

   Inputs:
      tmp - elevation parameter in request.

   Output:
      param - pointer to short to hold the elevation product dependent 
              parameter.

   Returns:	
      It returns 0 on success or -1 on failure.

**************************************************************************/
static int Process_elevation_parameter( char *elev_str, short *param ){

    int j, value;
    char *bits;

    /* Check if elevation parameter is in special format. */
    j = 0;
    while( elev_str[j] != '-' ){

        j++;
        if( j >= TBUF_SIZE )
            break;
    } 
                   
    /* The elevation parameter is defined in special format. */
    if( j < TBUF_SIZE ){

        char tmp[TBUF_SIZE];

        /* Copy "elev_str" to temporary buffer. */
        strcpy( tmp, elev_str );

        /* Validate the elevation/cut field.  If not valid, return error. */
        if( (j >= (TBUF_SIZE-2)) || (sscanf(&tmp[j+1], "%d", &value) != 1)
                   ||
            (*param < 0) ){

            LE_send_msg( GL_ERROR, "!!!!!!sscanf of Elevation Parameter Failed\n" );
	    return (-1);

        }

        *param = (short) value;

        /* Test the "bit" values ... set parameter accordingly. */
        tmp[j] = '\0';
        if( (bits = strstr( tmp, "00" )) != NULL )
            return 0;

        else if( (bits = strstr( tmp, "01" )) != NULL ){

            *param |= ORPGPRQ_LOWER_ELEVATIONS;
            return 0;

        } 
        else if( (bits = strstr( tmp, "10" )) != NULL ){

            *param |= ORPGPRQ_ALL_ELEVATIONS;
            return 0;

        } 
        else if( (bits = strstr( tmp, "11" )) != NULL ){

            *param |= ORPGPRQ_LOWER_CUTS;
            return 0;

        } 
        else{

           LE_send_msg( GL_ERROR, "!!!!!!Unknown Bits Set For Elevation Parameter\n" );
           return (-1);

        }

    }
    else{

        if (sscanf (elev_str, "%d", &value) == 1){

            *param = (short) value;
	    return 0;

        }
        else{

            LE_send_msg( GL_ERROR, "!!!!!!sscanf for Elevation Parameter Failed\n" ); 
	    return (-1);

        }

    }

    return 0;

}

/**************************************************************************

   Description: 
      This function processes the CFC product parameters based on RDA
      configuration.

   Inputs:
      item - Pd_prod_entry structure.
      table - Product generation table.

   Output:

   Returns:	

**************************************************************************/
static void Process_cfcprod( Pd_prod_entry *item, int table ){


   int num = 0;
   int ndx, i;

   /* Process CFC product based on the configuration. */
   if( Rda_config == ORPGRDA_LEGACY_CONFIG ){

      LE_send_msg( GL_INFO, 
             "Table %d: Processing CFC product for Legacy RDA configuration\n",
             table );
      
      for( i = 0; i < 4; i++ ){

         ndx = ORPGPGT_add_entry ( table );

         ORPGPGT_set_prod_id( table, ndx, (int) item->prod_id );
         ORPGPGT_set_generation_interval( table, ndx, (int) item->gen_pr );
         ORPGPGT_set_retention_period( table, ndx, (int) item->stor_retention );

         switch (i) {

            case 0 :

	       num = SURCHAN_LO;
	       break;

            case 1 :

	       num = SURCHAN_HI;
	       break;

            case 2 :

	       num = DOPCHAN_LO;
	       break;

            case 3 :

               num = DOPCHAN_HI;
	       break;

         }

         ORPGPGT_set_parameter( table, ndx, 0, num );

      }

   }
   else{


      LE_send_msg( GL_INFO, 
             "Table %d: Processing CFC product for ORDA configuration\n",
             table );
      
      /* Don't know how many elevation segments so schedule all possible. */
      for( i = 0; i < 5; i++ ){

         ndx = ORPGPGT_add_entry ( table );

         ORPGPGT_set_prod_id( table, ndx, (int) item->prod_id );
         ORPGPGT_set_generation_interval( table, ndx, (int) item->gen_pr );
         ORPGPGT_set_retention_period( table, ndx, (int) item->stor_retention );

         switch (i) {

            case 0 :

	       num = SEGMENT_1;
	       break;

            case 1 :

	       num = SEGMENT_2;
	       break;

            case 2 :

	       num = SEGMENT_3;
	       break;

            case 3 :

               num = SEGMENT_4;
	       break;

            case 4 :

               num = SEGMENT_5;
	       break;

         }

         ORPGPGT_set_parameter( table, ndx, 0, num );

      }

   }

}

/*******************************************************************

   Description:
      Returns the name of the first (dir_name != NULL) or the next 
      (dir_name = NULL) file in directory "dir_name" whose name matches 
      "basename".*. The caller provides the buffer "buf" of size 
      "buf_size" for returning the file name. 

   Inputs:  
      dir_name - directory name or NULL
      basename - product table base name
      buf - receiving buffer for next file name
      buf_size - size of receiving buffer

   Outputs:
      buf - contains next file name

   Returns:
      It returns 0 on success or -1 on failure.

*******************************************************************/
static int Get_next_file_name( char *dir_name, char *basename,
                               char *buf, int buf_size ){

    static DIR *dir = NULL;     /* the current open dir */
    static char saved_dirname[CFG_NAME_SIZE] = "";
    struct dirent *dp;

    /* If directory is not NULL, open directory. */
    if( dir_name != NULL ){

        int len;

        len = strlen (dir_name);
        if (len + 1 >= CFG_NAME_SIZE) {
            LE_send_msg (GL_ERROR,
                "dir name (%s) does not fit in tmp buffer\n", dir_name);
            return (-1);
        }
        strcpy (saved_dirname, dir_name);
        if (len == 0 || saved_dirname[len - 1] != '/')
            strcat (saved_dirname, "/");
        if (dir != NULL)
            closedir (dir);
        dir = opendir (dir_name);
        if (dir == NULL)
            return (-1);
    }

    if (dir == NULL)
        return (-1);

    /* Read the directory. */
    while ((dp = readdir (dir)) != NULL) {

        struct stat st;
        char fullpath[2 * CFG_NAME_SIZE];

        if (strncmp (basename, dp->d_name, strlen (basename)) != 0)
            continue;

        if (strlen (dp->d_name) >= CFG_NAME_SIZE) {
            LE_send_msg (GL_ERROR,
                "file name (%s) does not fit in tmp buffer\n", dp->d_name);
            continue;
        }
        strcpy (fullpath, saved_dirname);
        strcat (fullpath, dp->d_name);
        if (stat (fullpath, &st) < 0) {
            LE_send_msg (GL_ERROR,
                "stat (%s) failed, errno %d\n", fullpath, errno);
            continue;
        }
        if (!(st.st_mode & S_IFREG))    /* not a regular file */
            continue;

        if (strlen (fullpath) >= buf_size) {
            LE_send_msg (GL_ERROR,
                "caller's buffer is too small (for %s)\n", fullpath);
            continue;
        }
        strcpy (buf, fullpath);
        return (0);
    }

    return (-1);

/* End of Get_next_file_name() */
}

