/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2005/12/29 22:11:03 $
 * $Id: mnttsk_pat.c,v 1.5 2005/12/29 22:11:03 steves Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */
#include <mnttsk_pat_def.h>

/* Static Global Variables */
static char Cfg_dir[CFG_NAME_SIZE];     /* config directory pathname */
static char Product_tables_name[CFG_NAME_SIZE];
                                        /* file name for prod tables */
static char Product_tables_basename[CFG_NAME_SIZE];
                                        /* base file name for prod tables extensions */
static char Cfg_extensions_name[CFG_NAME_SIZE];
                                        /* directory name for prod tables extensions */
static int Verbose = 0;

/* Static Function Prototypes */
static int Init_PAT_main( int startup_action );
static int Get_command_line_options( int argc, char *argv[], int *startup_type );
int Init_attr_tbl( char *file_name, int clear_table );
static void Err_func ();
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
int main( int argc, char *argv[] ){

   int startup_type ;
   int retval ;


   /* Initialize log-error services. */
   (void) ORPGMISC_init(argc, argv, 200, 0, -1, 0) ;

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

   Verbose = 0;

   err = 0;
   while ((c = getopt (argc, argv, "vht:")) != EOF) {

      switch (c) {

         case 'v':
            Verbose = 1;
            break;

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

   CS_error ((void (*)())Err_func);

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

   if( Verbose ){

      int i, num_prods, prod_id;

      if( ORPGPAT_read_tbl() < 0 )
         LE_send_msg( GL_INFO, "Error Reading PAT!!!!\n" );

      num_prods = ORPGPAT_num_tbl_items();
      LE_send_msg( GL_INFO, "There are %d Products in the PAT\n", num_prods );

      for( i = 0; i < num_prods; i++ ){

         prod_id = ORPGPAT_get_prod_id( i );

         if( prod_id >= 0 ){

            int code;
            char *gen_task;

            code = ORPGPAT_get_code( prod_id );
            gen_task = ORPGPAT_get_gen_task( prod_id );

            LE_send_msg( GL_INFO, "--->Product %d, Code: %d, Gen Task: %s\n", 
                         prod_id, code, gen_task );

         }

      } 

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

    int p_cnt = 0;

    LE_send_msg (GL_INFO, "Initializing product attributes table\n");

    /* If the attributes table already exists, clear it and get ready
       to create new data. */
    if( clear_table )
       ORPGPAT_clear_tbl ();

    /* Create a new PAT.*/
    p_cnt = ORPGPAT_read_ASCII_PAT( file_name );
    LE_send_msg( GL_INFO, "There are %d Products Defined in %s\n", p_cnt, 
                 file_name );

    if( p_cnt < 0 )
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

    strcpy (Product_tables_name, "product_attr_table");
    strcpy (Product_tables_basename, "product_attr_table");
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
			"product_attr_table", tmpbuf, CFG_NAME_SIZE) < 0)
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
