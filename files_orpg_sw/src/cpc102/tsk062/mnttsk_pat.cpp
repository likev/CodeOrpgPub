/*
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2005/07/19 15:50:05 $
 * $Id: mnttsk_pat.cpp,v 1.1 2005/07/19 15:50:05 ryans Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

extern "C"
{
#include <mnttsk_pat_def.h>
}

/* Static Global Variables */
static char Cfg_dir[CFG_NAME_SIZE];     /* config directory pathname */
static char Product_tables_name[CFG_NAME_SIZE];
                                        /* file name for prod tables */
static char Product_tables_basename[CFG_NAME_SIZE];
                                        /* base file name for prod tables extensions */
static char Cfg_extensions_name[CFG_NAME_SIZE];
                                        /* directory name for prod tables extensions */

/* Static Function Prototypes */
static int Init_PAT_main( int startup_action );
static int Get_command_line_options( int argc, char *argv[], int *startup_type );
int Init_attr_tbl( char *file_name, int clear_table );
static void Err_func (char*);
static int Set_config_file_names();
static int Get_next_file_name( char *dir_name, char *basename,
                               char *buf, int buf_size );
static int Get_table_file_name( char *name, char *dir, char *basename,
                                char *tmp, int size );

#ifndef NO_MAIN_FUNCT
/**************************************************************************
    Description:
       Maintenance Task For initializing Product Attribute Table.
  
    Input:
       argc - number of command line arguments
       argv - the command line arguments

    Output:

    Returns:
       Exists with non-zero exit code on error, 0 exit code on success.

    Notes:

**************************************************************************/
int main( int argc, char **argv ){

   int startup_type ;
   int retval ;


   /* Initialize log-error services. */
   (void) ORPGMISC_init(argv[0], 200, 0, -1, 0) ;

   if( (retval = Get_command_line_options(argc, argv, &startup_type )) < 0 )
      exit(1) ;

   /* Perform the initialization based on "startup_type" !!! */
   if( (retval = Init_PAT_main( startup_type )) < 0 ){
 
      LE_send_msg( GL_INFO, "Data ID %d: Init_PAT_main() failed: %d",
                   ORPGDAT_PROD_INFO, retval) ;
      exit(2);

   }

   exit(0) ;

/* End of main() */
}
#endif


/****************************************************************************

   Description:
      Process command line arguments.

   Inputs:
      argc - number of command line arguments.
      argv - the command line arguments.

   Outputs:
      startup_action - start up action (STARTUP or RESTART)

   Returns:
      exits on error, or returns 0 on success.

*****************************************************************************/
static int Get_command_line_options( int argc, char *argv[], int *startup_action ){

   extern char *optarg;
   extern int optind;
   int c, err, ret;
   char start_up[255];

   /* Initialize startup_action to STARTUP and input_file_path to NULL. */
   *startup_action = RESTART;

   err = 0;
   while ((c = getopt (argc, argv, "ht:")) != EOF) {

      switch (c) {

         case 't':
            if( strlen( optarg ) < 255 ){

               ret = sscanf(optarg, "%s", start_up) ;
               if (ret == EOF) {

                  LE_send_msg( GL_INFO, "sscanf Failed To Read Startup Action\n" ) ;
                  err = 1 ;

               }
               else{

                  if( strstr( start_up, "startup" ) != NULL )
                     *startup_action = STARTUP;

                  else if( strstr( start_up, "restart" ) != NULL )
                     *startup_action = RESTART;

                  else if( strstr( start_up, "clear" ) != NULL )
                     *startup_action = CLEAR;

                  else if( strstr( start_up, "init" ) != NULL )
                     *startup_action = INIT;

                  else
                     *startup_action = STARTUP;

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

   if (err == 1) {              /* Print usage message */
      printf ("Usage: %s [options]\n", MISC_string_basename(argv [0]));
      printf ("\toptions:\n");
      printf ("\t\t-h (print usage msg and exit)\n");
      printf ("\t\t-t Startup Mode/Action (optional - default: startup)\n" );
      exit (1);
   }

   return (0);

/* End of Get_command_line_options() */
}

/********************************************************************
   Description:
      Main routine for initialization of the PAT.

   Inputs:
      startup_action - start up type.

   Outputs:

   Returns:
      Negative value on error, or 0 on success.
      
   Notes:

********************************************************************/
static int Init_PAT_main( int startup_action ){

   int err = 0;

   // Original call:  CS_error ((void (*)(char*))Err_func);
   CS_error ((void (*)(char*))Err_func);

   /* Initial the configuration file names. */
   Set_config_file_names();

   if( (startup_action == CLEAR) 
                    || 
       (startup_action == INIT)
                    || 
       (startup_action == STARTUP) 
                    || 
       (startup_action == RESTART) ){

      LE_send_msg( GL_INFO, "Initializing Product Attribute Table.\n" );

      /* Process the main product attribute table. */
      if( Init_attr_tbl ( Product_tables_name, CLEAR_TABLE ) < 0 ){

         LE_send_msg( GL_ERROR, "Initialize Product Attribute Table Failed\n" );
         err = -1;

      }

      /* If no error, process all the product attribute table extensions. */
      if( err >= 0 ){

         char ext_name[ CFG_NAME_SIZE ], *call_name;

         call_name = Cfg_extensions_name;
         while( Get_next_file_name( call_name, Product_tables_basename,
                                    ext_name, CFG_NAME_SIZE ) == 0 ){

            LE_send_msg( GL_INFO, "---> Reading Product Tables Extension %s\n",
                         ext_name );
            if( Init_attr_tbl( ext_name, NO_CLEAR_TABLE ) < 0 ){

               LE_send_msg( GL_ERROR, "Initialize Product Attribute Table Failed\n" );
               err = -1;
               break;

            }
            call_name = NULL;

         }

      }

      if( err <  0 ){

         LE_send_msg( GL_INFO, "Init Product Attributes Failed\n" );
         return(-1) ;

      }
      else
         LE_send_msg(GL_INFO,"Product Attributes Initialization Complete\n" );


   }

   return 0;

/*END of Init_PAT_main() */
}

/**************************************************************************
   Description: 
      This is the CS error reporting function.

   Input:  
      The error message.

   Output: 
      NONE

   Return: 
      NONE

**************************************************************************/
static void Err_func (char *msg){

   LE_send_msg( GL_INFO,  msg);
   return;

/* End of Err_func() */
}

/**************************************************************************
   Description: 
      This function initializes the product attribute table.

   Input:
      clear_table - flag, if set, causes existing table to be cleared.

   Output: 
      NONE

   Return:
      It returns the number of entries of the table on success
      or -1 on failure.

**************************************************************************/
int Init_attr_tbl( char *file_name, int clear_table ){

    int p_cnt, total_size, err;
    prod_id_t	prod_id;
    prod_id_t   aliased_prod_id;
    prod_id_t   class_id;
    unsigned int class_mask;
    task_id_t	gen_task_id;
    short	wx_modes;
    char	disabled;
    char	n_priority;
    char	n_dep_prods;
    char	n_opt_prods;
    short	prod_code;
    short	type;
    short	elev_index;
    short	alert;
    char        compression;
    char        format_type;
    int         warehoused;
    int         warehouse_id;
    int         warehouse_acct_id;
    int		max_size;
    char	name [PROD_NAME_LEN];
    char	desc [128];
    short	priority;
    short	dep_prod;
    short	opt_prod;
    short	param_index;
    short	param_min  ;
    short	param_max  ;
    short	param_default  ;
    int		param_scale;
    char	param_name [PARAMETER_NAME_LEN];
    char	param_units[PARAMETER_UNITS_LEN];
    int		indx;
    int		ret;

    p_cnt = total_size = err = 0;

    LE_send_msg (GL_INFO, "Initializing product attributes table\n");

/*  If the attributes table already exists, clear it and get ready	*
 *  to create new data.							*/

    if( clear_table )
       ret = ORPGPAT_clear_tbl ();

    CS_cfg_name ( file_name );
    CS_control (CS_COMMENT | '#');
    CS_control (CS_RESET);

    if (CS_entry ("Prod_attr_table", 0, 0, NULL) < 0 ||
	CS_level (CS_DOWN_LEVEL) < 0)
	return (-1);

/*  Repeat for all the product definitions in the product_attributes	*
 *  configuration file.							*/

    do {
	int len;
	int cnt;

	if (CS_level (CS_DOWN_LEVEL) < 0)
	    continue;

	if (CS_entry ("prod_id", 1 | CS_SHORT, 0, (char *)&prod_id) <= 0 ||
	    CS_entry ("prod_code", 1 | CS_SHORT, 0, (char *)&prod_code) <= 0 ||
	    CS_entry ("gen_task", 1 | CS_SHORT, 0, (char *)&gen_task_id) <= 0 ||
	    CS_entry ("wx_modes", 1 | CS_SHORT, 0, (char *)&wx_modes) <= 0 ||
	    CS_entry ("disabled", 1 | CS_BYTE, 0, (char *)&disabled) <= 0 ||
	    CS_entry ("n_priority", 1 | CS_BYTE, 0, (char *)&n_priority) <= 0 ||
	    CS_entry ("n_dep_prods", 1 | CS_BYTE, 0, (char *)&n_dep_prods) <= 0 ||
	    CS_entry ("alert", 1 | CS_SHORT, 0, (char *)&alert) <= 0 ||
	    CS_entry ("warehoused", 1 | CS_INT, 0, (char *)&warehoused) <= 0 ||
	    CS_entry ("type", 1 | CS_SHORT, 0, (char *)&type) <= 0 ||
	    CS_entry ("max_size", 1 | CS_INT, 0, (char *)&max_size) <= 0) {
	    err = 1;
	    break;
	}

/*	A new definition was read in so create a new table entry.	*/

	indx = ORPGPAT_add_prod (prod_id);

	if (indx == ORPGPAT_ERROR) {

	    CS_report ("ERROR: duplicate product found");
	    err = 1;
	    break;

	}

	ret = ORPGPAT_set_alert (prod_id, alert);

	if (ret == ORPGPAT_ERROR) {

	    CS_report ("ERROR: unable to set alert field in PAT");
	    err = 1;
	    break;

	}

	ret = ORPGPAT_set_warehoused (prod_id, warehoused);

	if (ret == ORPGPAT_ERROR) {

	    CS_report ("ERROR: unable to set warehoused field in PAT");
	    err = 1;
	    break;

	}

	ret = ORPGPAT_set_code (prod_id, prod_code);

	if (ret == ORPGPAT_ERROR) {

	    CS_report ("ERROR: unable to set prod_code field in PAT");
	    err = 1;
	    break;

	}

	ret = ORPGPAT_set_type (prod_id, type);

	if (ret == ORPGPAT_ERROR) {

	    CS_report ("ERROR: unable to set type field in PAT");
	    err = 1;
	    break;

	}

	ret = ORPGPAT_set_gen_task_id (prod_id, gen_task_id);

	if (ret == ORPGPAT_ERROR) {

	    CS_report ("ERROR: unable to set gen_task_id field in PAT");
	    err = 1;
	    break;

	}

	ret = ORPGPAT_set_wx_modes (prod_id, wx_modes);

	if (ret == ORPGPAT_ERROR) {

	    CS_report ("ERROR: unable to set wx_modes field in PAT");
	    err = 1;
	    break;

	}

	ret = ORPGPAT_set_disabled (prod_id, disabled);

	if (ret == ORPGPAT_ERROR) {

	    CS_report ("ERROR: unable to set disabled field in PAT");
	    err = 1;
	    break;

	}

	CS_control (CS_KEY_OPTIONAL);
	if (CS_entry ("elev_index", 1 | CS_SHORT, 0, (char *)&elev_index) <= 0)
	    elev_index = -1;

	ret = ORPGPAT_set_elevation_index (prod_id, elev_index);

	if (ret == ORPGPAT_ERROR) {

	    CS_report ("ERROR: unable to set elev_index field in PAT");
	    err = 1;
	    break;

	}

        if (CS_entry ("compression", 1 | CS_BYTE, 0,
                                         (char *)&compression) <  0)
           compression = 0;
  
        ret = ORPGPAT_set_compression_type (prod_id, compression);
  
        if (ret == ORPGPAT_ERROR) {
 
           CS_report ("ERROR: unable to set compression type field in PAT");
           err = 1;
           break;
 
        }

        if (CS_entry ("n_opt_prods", 1 | CS_BYTE, 0,
                                         (char *)&n_opt_prods) <  0)
           n_opt_prods = 0;
  
        if (CS_entry ("format_type", 1 | CS_BYTE, 0,
                                           (char *)&format_type) <  0)
           format_type = 0;

        ret = ORPGPAT_set_format_type (prod_id, format_type);
 
        if (ret == ORPGPAT_ERROR) {
  
           CS_report ("ERROR: unable to set format type field in PAT");
           err = 1;
           break;
  
        }

	CS_control (CS_KEY_REQUIRED);

	if (n_priority > 0) {	/* read the priority list */

	    cnt = 0;
	    while (cnt < n_priority &&
		   CS_entry ("priority_list", (cnt + 1) | CS_SHORT, 0,
					(char *) &priority) > 0) {

		ORPGPAT_add_priority (prod_id, priority);

		cnt++;
	    }

	    if (cnt != n_priority) {
		CS_report ("bad priority list");
		err = 1;
		break;
	    }
	}

	if (n_dep_prods > 0) {	/* read the dep prods list */

	    cnt = 0;
	    while (cnt < n_dep_prods &&
		   CS_entry ("dep_prods_list", (cnt + 1) | CS_SHORT, 0,
					(char *) &dep_prod) > 0) {

		ORPGPAT_add_dep_prod (prod_id, dep_prod);

		cnt++;
	    }

	    if (cnt != n_dep_prods) {
		CS_report ("bad dep prods list");
		err = 1;
		break;
	    }
	}

	if (n_opt_prods > 0) {	/* read the opt prods list */

	    cnt = 0;
	    while (cnt < n_opt_prods &&
		   CS_entry ("opt_prods_list", (cnt + 1) | CS_SHORT, 0,
					(char *) &opt_prod) > 0) {

		ORPGPAT_add_opt_prod (prod_id, opt_prod);

		cnt++;
	    }

	    if (cnt != n_opt_prods) {
		CS_report ("bad opt prods list");
		err = 1;
		break;
	    }
	}

	if ((len = 
	    CS_entry ("desc", 1, 256, (char *)desc)) < 0) {
	    err = 1;
	    break;
	}

	ORPGPAT_set_description (prod_id, desc);

	if ((len = 
	    CS_entry ("prod_id", 2, PROD_NAME_LEN, (char *)name)) < 0) {
	    err = 1;
	    break;
	}

	ORPGPAT_set_name (prod_id, name);

	/* read parameter list */

	CS_control (CS_KEY_OPTIONAL);

	if (CS_entry ("params", 1, 0, (char *)NULL) >= 0) {
	    while (CS_entry (CS_NEXT_LINE, 0, 0, NULL) >= 0) {
		if (CS_entry (CS_THIS_LINE, 0 | CS_SHORT, 0,
				(char *)&param_index) <= 0 ||
		    CS_entry (CS_THIS_LINE, 1 | CS_SHORT, 0,
				(char *)&param_min) <= 0 ||
		    CS_entry (CS_THIS_LINE, 2 | CS_SHORT, 0,
				(char *)&param_max) <= 0 ||
		    CS_entry (CS_THIS_LINE, 3 | CS_SHORT, 0,
				(char *)&param_default) <= 0 ||
		    CS_entry (CS_THIS_LINE, 4 | CS_INT, 0,
				(char *)&param_scale) <= 0 ||
		    CS_entry (CS_THIS_LINE, 5, PARAMETER_NAME_LEN,
				(char *)param_name) <= 0 ||
		    CS_entry (CS_THIS_LINE, 6, PARAMETER_UNITS_LEN,
				(char *)param_units) <= 0)
		    break;

		indx = ORPGPAT_add_parameter (prod_id);

		if (indx < 0) {

		    CS_report ("ERROR adding parameter to PAT");
		    err = 1;
		    break;

		}

		ret = ORPGPAT_set_parameter_index (prod_id, indx, param_index);

		if (ret < 0) {

		    CS_report ("ERROR setting parameter index in PAT");
		    err = 1;
		    break;

		}

		ret = ORPGPAT_set_parameter_min   (prod_id, indx, param_min);

		if (ret < 0) {

		    CS_report ("ERROR setting parameter min in PAT");
		    err = 1;
		    break;

		}

		ret = ORPGPAT_set_parameter_max   (prod_id, indx, param_max);

		if (ret < 0) {

		    CS_report ("ERROR setting parameter max in PAT");
		    err = 1;
		    break;

		}

		ret = ORPGPAT_set_parameter_default (prod_id, indx, param_default);

		if (ret < 0) {

		    CS_report ("ERROR setting parameter default in PAT");
		    err = 1;
		    break;

		}

		ret = ORPGPAT_set_parameter_scale (prod_id, indx, param_scale);

		if (ret < 0) {

		    CS_report ("ERROR setting parameter scale in PAT");
		    err = 1;
		    break;

		}

		ret = ORPGPAT_set_parameter_name (prod_id, indx, param_name);

		if (ret < 0) {

		    CS_report ("ERROR setting parameter name in PAT");
		    err = 1;
		    break;

		}

		ret = ORPGPAT_set_parameter_units (prod_id, indx, param_units);

		if (ret < 0) {

		    CS_report ("ERROR setting parameter units in PAT");
		    err = 1;
		    break;

		}
	    }
	}

	if (CS_entry ("aliased_prod_id", 1 | CS_SHORT, 0, 
					(char *)&aliased_prod_id) <= 0 )
           aliased_prod_id = -1;

	ret = ORPGPAT_set_aliased_prod_id (prod_id, aliased_prod_id);

	if (ret == ORPGPAT_ERROR) {

	    CS_report ("ERROR: unable to set aliased_prod_id field in PAT");
	    err = 1;
	    break;

	}

	if (CS_entry ("class_id", 1 | CS_SHORT, 0, 
					(char *)&class_id) <= 0 )
           class_id = prod_id;

	ret = ORPGPAT_set_class_id (prod_id, class_id);

	if (ret == ORPGPAT_ERROR) {

	    CS_report ("ERROR: unable to set class_id field in PAT");
	    err = 1;
	    break;

	}

	if (CS_entry ("class_mask", 1 | CS_UINT, 0, 
					(char *)&class_mask) <= 0 )
           class_mask = 0;

	ret = ORPGPAT_set_class_mask (prod_id, class_mask);

	if (ret == ORPGPAT_ERROR) {

	    CS_report ("ERROR: unable to set class_mask field in PAT");
	    err = 1;
	    break;

	}

	if (CS_entry ("warehouse_id", 1 | CS_INT, 0, 
					(char *)&warehouse_id) <= 0 )
           warehouse_id = 0;

	ret = ORPGPAT_set_warehouse_id (prod_id, warehouse_id);

	if (ret == ORPGPAT_ERROR) {

	    CS_report ("ERROR: unable to set warehouse ID field in PAT");
	    err = 1;
	    break;

	}

        if( warehouse_id > 0 ){

 	   CS_control (CS_KEY_REQUIRED);

	   if (CS_entry ("warehouse_acct_id", 1 | CS_INT, 0, 
	  				(char *)&warehouse_acct_id) > 0 ){

		ret = ORPGPAT_set_warehouse_acct_id (prod_id, warehouse_acct_id);

		if (ret == ORPGPAT_ERROR) {

	    	    CS_report ("ERROR: unable to set warehouse acct ID field in PAT");
	    	    err = 1;
	    	    break;

		}

	    }

	}
           
 	CS_control (CS_KEY_REQUIRED);

	p_cnt++;

	CS_level (CS_UP_LEVEL);

    } while (CS_entry (CS_NEXT_LINE, 0, 0, NULL) >= 0);

    CS_cfg_name ("");
    
    /* If an error was detected, do nothing and return.*/
    if( err )
	return (-1);

    else{
    
       /* Else, write the table to LB. */
       ORPGPAT_write_tbl ();
       return (p_cnt);

    }

/* End of Init_attr_tbl() */
}

/**************************************************************************
   Description: 
      This function initializes the configuration file names.

    Input:  
       NONE
	   
    Output: 
       NONE

    Return: 
       It returns 0 on success or -1 on failure.

**************************************************************************/
static int Set_config_file_names( ){

    int err = 0;                /* error flag */
    int	len;
    char tmpbuf [CFG_NAME_SIZE];

    /* Check to see if the configuration source (CFG_SRC) environmental
       variable is defined.  If so, make this the directory path to the
       various ASCII configuration files. */
    len = MISC_get_cfg_dir (Cfg_dir, CFG_NAME_SIZE);
    if (len > 0)
       strcat (Cfg_dir, "/");

    strcpy (Product_tables_name, "product_table");
    strcpy (Product_tables_basename, "product_tables");
    strcpy (Cfg_extensions_name, "extensions");

    /* Append the filename to the CFG_DIR.  If CFG_DIR not defined, 
       return error. */
    if( len <= 0 ){

       err = -1;
       LE_send_msg (GL_INFO, "CFG Directory Undefined\n");

     }
     else{

       /* Process the Product Tables file. */
       strcpy (tmpbuf, Cfg_dir);
       strcat (tmpbuf, Product_tables_name);
       strcpy (Product_tables_name, tmpbuf);
       if (Get_table_file_name (Product_tables_name, Cfg_dir, 
			"product_table", tmpbuf, CFG_NAME_SIZE) < 0)
	   return (-1);

       /* Process the configuration extensions. */
       strcpy (tmpbuf, Cfg_dir);
       strcat (tmpbuf, Cfg_extensions_name);
       strcpy (Cfg_extensions_name, tmpbuf);

    }

    return (err);

}

/******************************************************************

   Description:
    Searches in directory "dir" for file
    named "dir/basename" or "dir/basenames". If such a file is found,
    the full path of the file name is returned in "name". If it is
    not found, one of these names is returned. The buffer size
    of "name" are assumed to be big enough. "size" is the 
    buffer size of "tmp" which is assumed to be no less than the size
    of "name".

    Returns 0 on success or -1 if both files are found.
	
******************************************************************/
static int Get_table_file_name (char *name, char *dir, char *basename, 
						char *tmp, int size) {
    int len, cnt;
    char *call_name;

    call_name = dir;
    len = strlen (name);
    cnt = 0;
    while (Get_next_file_name (call_name, basename, 
						tmp, size) == 0) {
	if (strlen (tmp) == len)
	    cnt++;
	else if (strlen (tmp) == len + 1 && tmp[len] == 's') {
	    if (cnt == 0)
		strcat (name, "s");
	    cnt++;
	}
	call_name = NULL;
    }
    if (cnt >= 2) {
	LE_send_msg (GL_ERROR, "dupllicated %s found in %s", basename, dir);
	return (-1);
    }
    return (0);
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

    static DIR *dir = NULL;	/* the current open dir */
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
	if (!(st.st_mode & S_IFREG))	/* not a regular file */
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
