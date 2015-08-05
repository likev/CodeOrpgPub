/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2012/05/03 20:38:41 $
 * $Id: orpgtat.c,v 1.56 2012/05/03 20:38:41 steves Exp $
 * $Revision: 1.56 $
 * $State: Exp $
 */

/**************************************************************************

    Module:  orpgtat.c

    Description:

	This file provides ORPG library Task Attribute Table (TAT) access
        routines.

        Functions that are public are defined in alphabetical order at
        the top of this file and are identified by a prefix of "ORPGTAT_".

        The scope of all other routines defined within this file is
        limited to this file.  The private functions are defined in
        alphabetical order, following the definitions of the API functions.

 **************************************************************************/



#include <stdarg.h>            /* variable-arguments                      */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>            /* strlen(), strncpy(), memset()           */
#include <infr.h>
#include <orpg.h>
#include <debugassert.h>


/*
 * Constant Definitions/Macro Definitions/Type Definitions
 */
#define DATAID_STRTOK_STRING " \t"

/* Static Global Variables */
static int Need_read = 1;
static Orpgtat_dir_entry_t *Dir = NULL;
static int Dir_size = 0;
static int Argc = -1;
static char **Argv = NULL;

/* Function Prototypes */

/* Static Function Prototypes */
static int Allocate_entry_storage(Orpgtat_entry_t **entry_pp,
                                  size_t incr_size,
                                  size_t offset) ;

static int Parse_dataids( char *key, int *num_dataids, int **data_ids, 
                          char **data_names );

static int Compare_task_name( const void *arg1, const void *arg2 );
static int Compare_file_name( const void *arg1, const void *arg2 );

static LB_id_t Binary_search( Orpgtat_dir_entry_t *array, int size, 
                              char *task_name );
static int Parse_args_line (char *line, char *tsk_n, int n_s, 
						char *args, int a_s);
static int Add_entry (Orpgtat_mem_ary_t *mem_ary_p, 
				Orpgtat_entry_t *entry_p, int size);

/*************************************************************************

   Description:
      Sets the command line number of arguments and command line arguments
      for use by ORPGTAT_get_my_task_name().


*************************************************************************/
void ORPGTAT_set_args( int argc, char **argv ){

   int i;

   Argc = argc;

   /* This is necessary if application calls this function
      mutliple times.   Prevents a memory leak. */
   if( Argv != NULL )
      free( Argv );

   Argv = malloc( argc * sizeof( char * ) );

   for( i = 0; i < argc; i++ )
   {
     Argv[ i ] = argv[ i ];
   }

/* End of ORPGTAT_set_args() */
}

/*************************************************************************

   Description:
      Given a TAT entry, searches the input_data and output_data 
      for the task for a match on data_name.  If match found, the
      data ID associated with the name is returned.

   Inputs:
      task_entry - TAT entry
      data_name - data name string

   Returns:
      data ID for the associated data_name, or -1 on error.

*************************************************************************/
int ORPGTAT_get_data_id_from_name( Orpgtat_entry_t *task_entry,
                                   char *data_name ){

   int data_id = -1;

   /* Validate the task entry. */
   if( task_entry == NULL ){
  
      LE_send_msg( GL_INFO, 
              "Invalid Task Entry to ORPGTAT_get_data_id_from_name()\n" );
      return data_id;

   }
   else{

      int num_inputs = task_entry->num_input_dataids;
      int *input_ids = (int *) (((char *) task_entry) + task_entry->input_data);
      char *input_names = NULL;

      int num_outputs = task_entry->num_output_dataids;
      int *output_ids = (int *) (((char *) task_entry) + task_entry->output_data);
      char *output_names = NULL;

      if( task_entry->input_names > 0 )
         input_names = (char *) (((char *) task_entry) + task_entry->input_names);

      /* Find the data ID for data_name.  */
      if( input_names != NULL ){

         int i;

         /* Do For All TAT input entries. */
         for( i = 0; i < num_inputs; i++ ){

            /* Check for match on input data name. */
            if( strcmp( input_names, data_name ) == 0){

               data_id = input_ids[i];
               return data_id;

            }

            if( input_names != NULL )
               input_names += (strlen(input_names) + 1);

         }

      }

      if( task_entry->output_names > 0 )
         output_names = (char *) (((char *) task_entry) + task_entry->output_names);

      /* Find the data ID for data_name. */ 
      if( output_names != NULL ){

         int i;

         /* Do For All TAT output entries. */
         for( i = 0; i < num_outputs; i++ ){

            /* Check for match on input data name. */
            if( strcmp( output_names, data_name ) == 0){

               data_id = output_ids[i];
               return data_id;

            }

            if( output_names != NULL )
               output_names += (strlen(output_names) + 1);

         }

      }

   }

   return data_id;

/* End of ORPGTAT_get_data_id_from_name() */
} 

/*************************************************************************

   Description:

      Get operational (LB) TAT entry for specified task.
  
      Upon success, a pointer to a valid TAT entry (read from the
      operational TAT) is returned.
 
   Inputs:
      const char *task_name

   Returns:

     pointer to valid TAT entry - success
     NULL pointer -failure

  
     Possible Failures:
  
        Unable to read the TAT.
        Unable to store TAT entry in routine-provided memory.

*****************************************************************************/
Orpgtat_entry_t* ORPGTAT_get_entry( char *task_name ){

    int retval;
    Orpgtat_entry_t *entry;
    LB_id_t msg_id;

    /* Decide if the directory needs to be read. */
    if( Need_read ){

        if( Dir != NULL ){

           free( Dir );
           Dir = NULL;
           Dir_size = 0;

        }

        /* Read the directory. */
        Need_read = 0;
        retval = ORPGDA_read( ORPGDAT_TAT, (char *) &Dir,
                              LB_ALLOC_BUF, ORPGTAT_TAT_DIRECTORY );
        if( retval < 0 ){

           Need_read = 1;
           LE_send_msg( GL_ERROR, 
              "ORPGDA_read( ORPGDAT_TAT, ORPGTAT_TAT_DIRECTORY ) Failed (%d)\n", retval );
           return (NULL);

        }

        Dir_size = retval / ALIGNED_SIZE(sizeof(Orpgtat_dir_entry_t) );

    }

    /* Find entry in directory */
    if( (msg_id = Binary_search( Dir, Dir_size, task_name )) == 0 )
       return (NULL);

    /* Read the entry. */
    if( (retval = ORPGDA_read( ORPGDAT_TAT, (char *) &entry, LB_ALLOC_BUF,
                               msg_id )) < 0 ){

       LE_send_msg( GL_ERROR, "ORPGDA_read( ORPGDAT_TAT, %d ) Failed (%d)\n",
                    msg_id, retval );
       return (NULL) ;

    }

    /* Make sure entry matches what was expected. */
    if( strcmp( task_name, entry->task_name ) != 0 ){

       free( entry );
       LE_send_msg( GL_ERROR, "Unexpected TAT Entry ......\n" );
       return (NULL);

    }

    return (entry);

/* End of ORPGTAT_get_entry() */
}

/*************************************************************************

   Description:

      Get TAT directory.
  
      Upon success, a pointer to a directory is returned.
 
   Inputs:
  
      tat -  pointer to pointer to Orpgtat_dir_entry_t structure.

   Outputs:

      tat - assigned pointer to TAT directory.

   Returns:

     The of entries in the TAT or -1 on failure.  On failure, tat is 
     assigned NULL.

     Possible Failures:
  
        Unable to stat the TAT.
        Unable to allocate storage for the directory.

*****************************************************************************/
int ORPGTAT_get_directory( Orpgtat_dir_entry_t **dir_p ){

    int retval, size;

    /* Decide if the directory needs to be read. */
    if( Need_read ){

        if( Dir != NULL ){

           free( Dir );
           Dir = NULL;
           Dir_size = 0;

        }

        /* Read the directory. */
        Need_read = 0;
        retval = ORPGDA_read( ORPGDAT_TAT, (char *) &Dir,
                              LB_ALLOC_BUF, ORPGTAT_TAT_DIRECTORY );
        if( retval < 0 ){

           Need_read = 1;
           LE_send_msg( GL_ERROR, 
               "ORPGDA_read( ORPGDAT_TAT, ORPGTAT_TAT_DIRECTORY ) Failed (%d)\n", retval );
           return (-1);

        }

        Dir_size = retval / ALIGNED_SIZE(sizeof(Orpgtat_dir_entry_t) );

    }

    size = Dir_size*ALIGNED_SIZE(sizeof(Orpgtat_dir_entry_t));
    *dir_p = (Orpgtat_dir_entry_t *) malloc( size );
    if( dir_p == NULL ){

       LE_send_msg( GL_ERROR, "malloc Failed for %d Bytes\n", size );
       return(-1);

    }

    memcpy( (void *) *dir_p, (void *) Dir, size );

    return( Dir_size );

}

/*************************************************************************

    Searches the task table to find the task name of this process. The task
    name is returned in buffer "task_name_buf" of size "buf_size". The
    function returns 0 on success or a negative error code.

*****************************************************************************/
int ORPGTAT_get_my_task_name( char *task_name_buf, int buf_size ){

    int retval;
    char *topt, *fname;
    int first, ind_found, st, end, i;

    /* In the event Argc/Argv haven't been initialized, check if these have 
       been initialized within the LE module. */
    if( Argc <= 0 )
       Argc = LE_get_argv( &Argv );

    if (buf_size > 0)
	task_name_buf[0] = '\0';
    if( Argc <= 0 ) {
        LE_send_msg( GL_ERROR, "LE_get_argv Failed - LE_init not yet called\n" );
        return (ORPGTAT_ARGV_NOT_FOUND);
    }
    strncpy (task_name_buf, MISC_basename (Argv[0]), buf_size);
    task_name_buf[buf_size - 1] = '\0';

    /* Decide if the directory needs to be read. */
    if( Need_read ){

        if( Dir != NULL ){

           free( Dir );
           Dir = NULL;
           Dir_size = 0;

        }

        /* Read the directory. */
        Need_read = 0;
        retval = ORPGDA_read( ORPGDAT_TAT, (char *) &Dir,
                              LB_ALLOC_BUF, ORPGTAT_TAT_DIRECTORY );
        if( retval < 0 ){

           Need_read = 1;
           return (ORPGTAT_TASK_TABLE_NOT_FOUND);

        }

        Dir_size = retval / ALIGNED_SIZE(sizeof(Orpgtat_dir_entry_t) );

    }

    topt = "";		/* find -T option */
    for (i = 0; i < Argc; i++) {
	if (strcmp (Argv[i], "-T") == 0) {
	    if (Argv[i][2] != '\0')
		topt = Argv[i] + 2;
	    else if (i < Argc - 1 && Argv[i + 1][0] != '-')
		topt = Argv[i + 1];
	    break;
	}
    }

    /* binary search for the first entry of file name */
    fname = MISC_basename (Argv[0]);
    st = 0;
    end = Dir_size - 1;
    first = -1;
    while (1) {
	int ind = (st + end) >> 1;
	if (ind == st) {
	    if (strcmp (fname, Dir[Dir[st].index].file_name) == 0)
		first = st;
	    else if (strcmp (fname, Dir[Dir[end].index].file_name) == 0)
		first = end;
	    break;
	}
	if (strcmp (fname, Dir[Dir[ind].index].file_name) <= 0)
	    end = ind;
	else
	    st = ind;
    }
    if (first < 0)
	return (ORPGTAT_FILE_NAME_NOT_FOUND);

    ind_found = -1;
    for (i = first; i < Dir_size; i++) {
	int si = Dir[i].index;
	if (strcmp (fname, Dir[si].file_name) != 0)
	    break;
	if (strcmp (Dir[si].task_name, Dir[si].file_name) != 0 &&
	    strcmp (topt, Dir[si].task_name) == 0 ) {
	    ind_found = si;
	    break;
	}
    }
    if (ind_found < 0) {
	for (i = first; i < Dir_size; i++) {
	    int si = Dir[i].index;
	    if (strcmp (fname, Dir[si].file_name) != 0)
		break;
	    if (strcmp (Dir[si].task_name, Dir[si].file_name) == 0) {
		ind_found = si;
		break;
	    }
	}
    }
    if (ind_found < 0)
	return (ORPGTAT_TASK_NAME_NOT_FOUND);

    strncpy (task_name_buf, Dir[ind_found].task_name, buf_size);
    task_name_buf[buf_size - 1] = '\0';
    return (0);
}

#define TMP_BUF_SIZE          128

/**************************************************************************

   Description: 

      Read the entire Task Attribute Table, from the specified
      ASCII configuration (CS) file.

      Place the Task Attribute Table entries in a well-known
              (Orpgtat_mem_ary_t) structure.

   Inputs: 

      pointer to Orpgtat_mem_ary_t structure that is to be initialized.
      pointer to Task Tables filename (may be NULL)

   Output: 

      the Task Attribute Table entries will be placed in the
      array storage provided by this process.

   Returns: 

      0 if successful; -1 otherwise

   Notes:

      If the Orpgtat_mem_ary_p array pointer ("ptr") is not NULL,
      we assume memory has been allocated and should be freed.
 
      CS_entry() guarantees NULL-terminated string

      After re-allocating memory for a given entry, we update the
      entry pointer with respect to the (possibly changed) array
      pointer.

 **************************************************************************/

#define TBUF_SIZE 128

int ORPGTAT_read_ascii_tbl( Orpgtat_mem_ary_t *mem_ary_p,
                            const char *tasktbl_fname ){

    Orpgtat_entry_t *entry_p ;
    size_t incr_size ;         /* increment size for malloc/realloc       */
    Orpgtat_args_entry_t *item;
    int retval = 0 ;
    int retval2 ;
    int num_dataids; 
    int *data_ids = NULL; 
    char *data_names = NULL;
    char *save_cfg_name ;      /* CS cfg name upon entering this routine  */
    char *tmp_char_p ;
    char site[TMP_BUF_SIZE];

    /* Error-checking ... */
    if( (mem_ary_p == NULL) || (tasktbl_fname == NULL) ){

        LE_send_msg(GL_ERROR, "one or more args is NULL");
        return(-1);

    }

    /* Ensure pointer is NULL until we succeed in allocating storage 
       for the array of Task Attribute Table entries ... */
    if( mem_ary_p->ptr != NULL ){

        free(mem_ary_p->ptr);
        mem_ary_p->ptr = NULL;
        mem_ary_p->size_bytes = 0;

    }

    /* Save a copy of the current CS configuration name, as we'll need to 
       restore that when we leave this function ... CS_cfg_name() returns 
       a NULL-terminated string ... */
    tmp_char_p = CS_cfg_name( NULL );
    if( strlen( tmp_char_p ) > 0 ){

        save_cfg_name = malloc( strlen(tmp_char_p) + 1 );    
        if( save_cfg_name != NULL ){

            strncpy( save_cfg_name, (const char *) tmp_char_p,
                     strlen(tmp_char_p)+1 );
        }
        else{

            LE_send_msg(GL_MEMORY, "Unable to allocate storage for cfg file name");
            return(-1);

        }

    }
    else{

        save_cfg_name = malloc( strlen("") + 1 );    
        if( save_cfg_name != NULL )
            strncpy( save_cfg_name, (const char *) "", strlen("")+1 );
         
        else{

            LE_send_msg( GL_MEMORY, "Unable to allocate storage for cfg file name" );
            return(-1);

        }

    }

    /* Indicate we're ingesting the Task Table configuration file and that 
       comment lines begin with '#' ... */
    if( strlen(tasktbl_fname) )
        CS_cfg_name( (char *) tasktbl_fname );
     
    else
        CS_cfg_name( (char *) ORPGTAT_CS_DFLT_TT_FNAME );

    CS_control( CS_COMMENT | ORPGTAT_CS_TT_COMMENT );

    /* Proceed to the Task Attribute Table ... and step down into it ... */
    CS_control( CS_TOP_LEVEL );

    mem_ary_p->size_bytes = 0 ;
    entry_p = NULL;
    do {

        char descr[ORPGTAT_CS_DESCRIPTION_BUFSIZE] ;
                               /* temporary storage for prod description  */
        char task_name[ORPG_TASKNAME_SIZ] ;
                               /* temporary storage for task name  */
        int len, ret;
        size_t offset ;
                               /* offset (bytes) from start of entry      */
	char tbuf[TBUF_SIZE];

        offset = 0 ;

        /* Get the local name (task name). */
        if( (retval2 = CS_entry( CS_THIS_LINE, 0, (int) TBUF_SIZE, (void *)tbuf )) > 0 ){

           /* Each entry must start with "ORPGTAT_CS_ATTR_TASK_KEY" */
           if( strcmp( ORPGTAT_CS_ATTR_TASK_KEY, tbuf ) != 0 ){
              LE_send_msg( GL_INFO, "Key %s Not Found\n", ORPGTAT_CS_ATTR_TASK_KEY );
              retval = -1;
              break;
           }

           retval2 = CS_entry( CS_THIS_LINE, 1, (int) ORPG_TASKNAME_SIZ, (void *)task_name );

        }
        else
           break;

        /* Initial memory allocation for this entry ... */
        incr_size = ALIGNED_SIZE( sizeof(Orpgtat_entry_t) );

        if( Allocate_entry_storage( &entry_p, incr_size, offset) < 0 ){
            retval = -1 ;
            break ;
        }

        /* Step down into the Task Attribute Table Entry ... */
        if( CS_level( CS_DOWN_LEVEL ) < 0 ){

            LE_send_msg( GL_INFO, "CS_level( CS_DOWN_LEVEL ) Failed\n" );
            continue;

        }

        offset = ALIGNED_SIZE(sizeof(Orpgtat_entry_t)) ;

        /* Get the process name. */
        if( CS_entry( ORPGTAT_CS_NAME_KEY, ORPGTAT_CS_NAME_TOK,
                      (int) TBUF_SIZE, (void *)tbuf ) <= 0 ){
            LE_send_msg( GL_INFO, "Key %s Not Found\n", ORPGTAT_CS_NAME_KEY );
            retval = -1 ;
            break ;
        }

        /* Store the process name in the task table "name" field. */
	strncpy( entry_p->file_name, MISC_basename( tbuf ), ORPG_TASKNAME_SIZ );
	entry_p->file_name[ ORPG_TASKNAME_SIZ - 1 ] = '\0';


        /* The task name is either explicitly defined or it will be the same as the "name". */
        if( retval2 > 0 ){

	    strncpy( entry_p->task_name, task_name, ORPG_TASKNAME_SIZ );
	    entry_p->task_name[ ORPG_TASKNAME_SIZ - 1 ] = '\0';

        }
        else
	    strcpy( entry_p->task_name, entry_p->file_name ); 

        /* Read the (optional) input stream ... */
        CS_control( CS_KEY_OPTIONAL );

        if( CS_entry( ORPGTAT_CS_DATA_STREAM_KEY, ORPGTAT_CS_DATA_STREAM_TOK,
                      0, (void *) &(entry_p->data_stream)) <= 0 )
            entry_p->data_stream = 0;

        /* Read the (optional) "input data" list. */
        Parse_dataids( ORPGTAT_CS_INDATA_KEY, &num_dataids, &data_ids, &data_names );

        /* Add the data IDs and data names to the task table entry. */
        if( num_dataids > 0 ){

            int *int_p;
            char *char_p;

            incr_size = ALIGNED_SIZE( ((num_dataids) * ALIGNED_SIZE(sizeof(int))) +
                                      STR_size( data_names ));

            if( Allocate_entry_storage( &entry_p, incr_size, offset ) < 0 ){
                retval = -1 ;
                break ;
            }

            entry_p->num_input_dataids = num_dataids;
            entry_p->input_data = offset;
            entry_p->input_names = offset + num_dataids*ALIGNED_SIZE(sizeof(int));

            /* Add the data IDs. */
            int_p = (int *) ((char *) entry_p + entry_p->input_data);
            memcpy( int_p, data_ids, (num_dataids * ALIGNED_SIZE(sizeof(int))) );

            /* Add the data names. */
            char_p = (char *) int_p + (num_dataids * ALIGNED_SIZE(sizeof(int)));
            memcpy( char_p, data_names, STR_size( data_names ) );

            offset += incr_size ;

        }
        else{

            entry_p->num_input_dataids = 0;
            entry_p->input_data = entry_p->input_names = 0;

        }

        /* Read the (optional) "output data" list. */
        Parse_dataids( ORPGTAT_CS_OUTDATA_KEY, &num_dataids, &data_ids, &data_names );

        /* Add the data IDs and data names to the task table entry. */
        if( num_dataids > 0 ){

            int *int_p;
            char *char_p;

            incr_size = ALIGNED_SIZE( ((num_dataids) * ALIGNED_SIZE(sizeof(int))) +
                                      STR_size( data_names ));

            if( Allocate_entry_storage( &entry_p, incr_size, offset ) < 0 ){
                retval = -1 ;
                break ;
            }

            entry_p->num_output_dataids = num_dataids;
            entry_p->output_data = offset;
            entry_p->output_names = offset + num_dataids*ALIGNED_SIZE(sizeof(int));

            /* Add the data IDs. */
            int_p = (int *) ((char *) entry_p + entry_p->output_data);
            memcpy( int_p, data_ids, (num_dataids * ALIGNED_SIZE(sizeof(int))) );

            /* Add the data names. */
            char_p = (char *) int_p + (num_dataids * ALIGNED_SIZE(sizeof(int)));
            memcpy( char_p, data_names, STR_size( data_names ) );

            offset += incr_size ;

        }
        else{

            entry_p->num_output_dataids = 0;
            entry_p->output_data = entry_p->output_names = 0;

        }

        CS_control(CS_KEY_REQUIRED) ;

        /* Read the task description ... note that the description is NOT optional ... 
           so, we must allocate at least one byte for the terminating null ... 
           we must place the correct offset in the "desc" element ... */
        entry_p->desc = offset ;
        if ((len = CS_entry(ORPGTAT_CS_DESCRIPTION_KEY,
                            ORPGTAT_CS_DESCRIPTION_TOK,
                            ORPGTAT_CS_DESCRIPTION_BUFSIZE,
                            (void *) descr)) < 0) {

            LE_send_msg( GL_INFO, "Key %s Not Found\n", ORPGTAT_CS_DESCRIPTION_KEY );
            retval = -1 ;
            break ;
        }

        incr_size = ALIGNED_SIZE(len + 1) ;

        if (Allocate_entry_storage( &entry_p, incr_size, offset) < 0) {
            retval = -1 ;
            break ;
        }

        (void) strncpy((char *) entry_p + offset, descr, len) ;

        offset += incr_size ;

        /* Read some OPTIONAL flags ...
          
           "respawn" Task
           "allow_duplicate" Task
           "monitor_only" Task
           "rpg control_task" Task
           "do_not_monitor" Task
           "site" Task 
           "processing supplement scans allowed" Task
        */
        CS_control(CS_KEY_OPTIONAL) ;

        entry_p->type = 0;
        if( CS_entry( ORPGTAT_CS_TYPE_RESPAWN_KEY, 0, 0, (void *) NULL) == 0 )
            entry_p->type |= ORPGTAT_TYPE_RESPAWN;
         
        if( CS_entry( ORPGTAT_CS_TYPE_ALLOW_DUP_KEY, 0, 0, (void *) NULL ) == 0 )
            entry_p->type |= ORPGTAT_TYPE_ALLOW_DUP;
         
        if( CS_entry( ORPGTAT_CS_TYPE_RPG_CNTL_KEY, 0, 0, (void *) NULL ) == 0 )
            entry_p->type |= ORPGTAT_TYPE_RPG_CNTL;
         
        if( CS_entry( ORPGTAT_CS_TYPE_MON_ONLY_KEY,0,0, (void *) NULL ) == 0 )
            entry_p->type |= ORPGTAT_TYPE_MON_ONLY;
         
        if (CS_entry( ORPGTAT_CS_TYPE_DONT_MON_KEY, 0, 0, (void *) NULL ) == 0 ) 
            entry_p->type |= ORPGTAT_TYPE_DONT_MON;

        if (CS_entry( ORPGTAT_CS_TYPE_ALLOW_SUPPL_SCANS_KEY, 0, 0, (void *) NULL ) == 0 ) 
            entry_p->type |= ORPGTAT_TYPE_ALLOW_SUPPL_SCANS;

        site[0] = '\0';
        if( CS_entry( ORPGTAT_CS_TYPE_SITE_KEY, 0, TMP_BUF_SIZE, site ) > 0 ){

           CS_control( CS_KEY_REQUIRED );
           if( CS_entry ( ORPGTAT_CS_TYPE_SITE_KEY, 1, TMP_BUF_SIZE, site ) <= 0 ){
              LE_send_msg (GL_ERROR, "Missing site task type");
              retval = -1 ;
              break ;
           }

           if (strcmp ( site, "PRODUCT_SERVER") == 0 ){

              entry_p->type |= ORPGTAT_TYPE_PROD_SERVER;
              site[0] = '\0';

           }
           else if (strcmp ( site, "COMM_MANAGER") == 0 ){

              entry_p->type |= ORPGTAT_TYPE_COMM_MANAGER;
              site[0] = '\0';

           }

            CS_control( CS_KEY_OPTIONAL );

        }       

        /* Read the (optional) argument lists. */
        entry_p->maxn_inst = 1;		/* no longer used */
        incr_size = ALIGNED_SIZE(sizeof(Orpgtat_args_entry_t)) ;

        if( Allocate_entry_storage( &entry_p, incr_size, offset ) < 0 ){
            retval = -1;
            break;
        }

        entry_p->args = offset ;
        item = (Orpgtat_args_entry_t *) ((char *) entry_p + entry_p->args) ;
        item->instance = 0;		/* no longer used */
        offset += incr_size;

	ret = CS_entry (ORPGTAT_CS_ARGS_KEY, 0, TBUF_SIZE, tbuf);
	if (ret > 0) {		/* single line of "args" */
	    CS_entry (ORPGTAT_CS_ARGS_KEY, CS_FULL_LINE, TBUF_SIZE, tbuf);
	    if (Parse_args_line (tbuf, entry_p->task_name, ORPG_TASKNAME_SIZ,
					item->args, ORPG_PATHNAME_SIZ) < 0) {
                LE_send_msg (GL_ERROR, "Unexpected args line");
		retval = -1;
		break;
	    }
	    if (Add_entry (mem_ary_p, entry_p, offset) < 0) {
		retval = -1;
		break;
	    }
	}
	else if (ret == CS_KEY_AMBIGUOUS) { /* multiple lines of "args" */
	    char tsk_n[TMP_BUF_SIZE], *ln;

	    ln = CS_THIS_LINE;
	    CS_level (CS_UP_LEVEL);
	    CS_level (CS_DOWN_LEVEL);
	    while (CS_entry (ln, 0, TBUF_SIZE, tbuf) > 0) {
		if (strcmp (tbuf, ORPGTAT_CS_ARGS_KEY) == 0) {
		    CS_entry (CS_THIS_LINE, CS_FULL_LINE, TBUF_SIZE, tbuf);
		    strcpy (tsk_n, entry_p->task_name);
		    if (Parse_args_line (tbuf,
				entry_p->task_name, ORPG_TASKNAME_SIZ,
				item->args, ORPG_PATHNAME_SIZ) < 0) {
                        LE_send_msg (GL_ERROR, "Unexpected args line");
			retval = -1;
			break;
		    }
		    if (Add_entry (mem_ary_p, entry_p, offset) < 0) {
		        retval = -1;
		        break;
	            }
		    strcpy (entry_p->task_name, tsk_n);
		}
		ln = CS_NEXT_LINE;
	    }
	    if (retval < 0)
		break;
	}
	else { 			/* no "args" line */
	    strcpy (item->args, "");
	    if (Add_entry (mem_ary_p, entry_p, offset) < 0) {
		retval = -1;
		break;
	    }
	}

        CS_control( CS_KEY_REQUIRED );
        CS_level( CS_UP_LEVEL );

    } while( CS_entry( CS_NEXT_LINE, 0, 0, (char *) NULL ) >= 0 );

    if (entry_p != NULL)
        free (entry_p);

    /* In the event an error occurred, free memory associated with the
       binary task table. */
    if( (retval != 0) && (mem_ary_p->ptr != NULL) ){

        free( mem_ary_p->ptr );
        mem_ary_p->ptr = NULL;
        mem_ary_p->size_bytes = 0;

    }

    CS_control (CS_CLOSE);
    CS_cfg_name( save_cfg_name );

    /* Cleanup after reading table. */
    free( save_cfg_name );
    if( data_ids != NULL )
       free(data_ids); 

    if( data_names != NULL )
       STR_free(data_names);

    /* Return to caller. */
    return( retval );

/* End of ORPGTAT_read_ascii_tbl() */
}

/******************************************************************

    Parses a "args" line of "line" in the task attr table and
    returns the task name and args in "task_n" and "args" of sizes
    n_s and a_s respectively. Return 0 on success or -1 on failure.
	
******************************************************************/

static int Parse_args_line (char *line, char *tsk_n, int n_s, 
						char *args, int a_s) {
    char tk[TMP_BUF_SIZE];

    if (MISC_get_token (line, "Q\"", 0, tk, TMP_BUF_SIZE) < 0 ||
	strcmp (tk, "args") != 0 ||
	MISC_get_token (line, "Q\"", 1, tk, TMP_BUF_SIZE) < 0 ||
	MISC_get_token (line, "Q\"", 3, tk, TMP_BUF_SIZE) > 0) {
	return (-1);
    }
    if (MISC_get_token (line, "Q\"", 2, args, a_s) > 0)
	MISC_get_token (line, "Q\"", 1, tsk_n, n_s);
    else {
	MISC_get_token (line, "Q\"", 1, args, a_s);
    }
    return (0);
}

/******************************************************************

    Append of a new tat entry "entry_p" of "size" bytes to array
    "mem_ary_p". Returns 0 on success or -1 on failure.
	
******************************************************************/

static int Add_entry (Orpgtat_mem_ary_t *mem_ary_p, 
				Orpgtat_entry_t *entry_p, int size) {
    char *p;

    entry_p->entry_size = size;

    if (mem_ary_p->ptr == NULL)
        p = malloc(size) ;
    else
        p = realloc(mem_ary_p->ptr, mem_ary_p->size_bytes + size) ;
    if (p == NULL) {
        LE_send_msg(GL_MEMORY, "malloc() FAILED") ;
        return(-1) ;
    }
    memcpy (p + mem_ary_p->size_bytes, entry_p, size);
    mem_ary_p->size_bytes += size;
    mem_ary_p->ptr = (Orpgtat_entry_t *)p;
    return (0);
}

/****************************************************************************

   Description:
      Writes the TAT directory record to LB.

   Inputs:

****************************************************************************/
int ORPGTAT_write_directory( char *tat_dir, int length ){

   int retval, num_entries, i;
   Orpgtat_dir_entry_t *dp = NULL, *tdp = NULL;

   /* Validate the size of the directory .... must be a multiple
      of Orpgtat_dir_entry_t. */
   if( length % ALIGNED_SIZE(sizeof(Orpgtat_dir_entry_t)) != 0 ){

      LE_send_msg( GL_ERROR, "Invalid Length (%d) For TAT Directory\n",
                   length );
      return( -1 );

   }

   /* Sort the directory entries before writing.   Makes it easier when
      we need to locate an entry. */
   num_entries = length / ALIGNED_SIZE(sizeof(Orpgtat_dir_entry_t));
   qsort( (void *) tat_dir, (size_t) num_entries, 
          ALIGNED_SIZE(sizeof(Orpgtat_dir_entry_t)),
          Compare_task_name );

   dp = (Orpgtat_dir_entry_t *) tat_dir;
   for( i = 0; i < num_entries; i++ )
      dp[i].index = i;

   /* Make a copy and sort on filename. */
   tdp = (Orpgtat_dir_entry_t *) malloc( length );
   if( tdp == NULL ){

      LE_send_msg( GL_ERROR, "malloc Failed for %d bytes\n", length );
      return(-1);

   }

   memcpy( (char *) tdp, tat_dir, length );
   qsort( (void *) tdp, (size_t) num_entries, 
          ALIGNED_SIZE(sizeof(Orpgtat_dir_entry_t)),
          Compare_file_name );

   for( i = 0; i < num_entries; i++ )
      dp[i].index = tdp[i].index;

   free( tdp );

   /* Write the directory record. */
   retval = ORPGDA_write( ORPGDAT_TAT, tat_dir, length, ORPGTAT_TAT_DIRECTORY );

   if( retval < 0 )
      LE_send_msg( GL_ERROR, 
         "ORPGDA_write(ORPGDAT_TAT, ORPGTAT_TAT_DIRECTORY) Failed (%d)\n", retval );

   return( retval );

/* End of ORPGTAT_write_tat_directory() */
}

/**************************************************************************
    Description:

       Reallocate storage for a given table entry

    Input:

        pointer-to- memory TAT array structure
        pointer-to-pointer to table entry
        required storage increase increment (bytes)
        offset of current entry from start of array of table entries
        offset from start of entry to storage increment

   Output:

   Returns: 

      0 if successful; -1 otherwise

   Notes:

 **************************************************************************/
static int Allocate_entry_storage(Orpgtat_entry_t **entry_pp,
                       size_t incr_size,
                       size_t incr_offset){

    if (*entry_pp == NULL) {
        /*
         * insure++ indicates that although Standard C supports
         * passing null pointer to realloc(), doing so may result
         * in portability problems ...
         */
        *entry_pp = malloc(incr_size) ;
        if (*entry_pp == NULL) {
            LE_send_msg(GL_MEMORY,
                        "User Task List malloc() FAILED") ;
            return(-1) ;
        }
        (*entry_pp)->entry_size = incr_size ;
    }
    else if ((*entry_pp)->entry_size < incr_offset + incr_size) {
        char *p = realloc(*entry_pp,
                                 incr_offset + incr_size) ;
        if (p == NULL) {
            LE_send_msg(GL_MEMORY,
                        "User Task List realloc() FAILED") ;
            return(-1) ;
        }
        *entry_pp = (Orpgtat_entry_t *)p;
        (*entry_pp)->entry_size = incr_offset + incr_size ;
    }


    /*
     * Since the value of the array pointer may have changed, update
     *     the current entry pointer.
     * Zero-out the storage increment.
     * Increment the entry and array sizes.
     */
    (void) memset((char *) *entry_pp + incr_offset, 0, incr_size) ;

    return(0) ;

/* End of Allocate_entry_storage() */
}

/**************************************************************************

   Description:

      Parse the "data_ids" (and optionally "data_names") associated with "key".

   Inputs:
      key - should be either ORPGTAT_CS_INDATA_KEY or ORPGTAT_CS_OUTDATA_KEY

   Output:
      num_data_id - number of data IDs found
      data_ids - list of data IDs
      data_names - list of data names

   Returns:

      0 if successful; -1 otherwise


**************************************************************************/
static int Parse_dataids( char *key, int *num_data_ids, int **data_ids,
                          char **data_names ){

    int retval = 0;
    int retval2;
    short cnt;

    *num_data_ids = 0;

    /* Free any memory associated with data IDs and data names. */
    if( *data_ids != NULL ){

        free( *data_ids );
        *data_ids = NULL;

    }

    if( *data_names != NULL ){

        STR_free( *data_names );
        *data_names = NULL;

    } 

    /* Read the (optional) "key" list ... our preference is to determine
       the number of input Data IDs so that we need only one memory re-allocation. */
    CS_control( CS_KEY_OPTIONAL );
    cnt = 0;
    retval2 = CS_entry( key, 0, 0, (void *) NULL );
    if( retval2 == 0 ){

        char listbuf[ORPGTAT_CS_TT_MAXLINELEN+1];
        char *tok_p;

        /* Count the number of input Data IDs by actually reading them ... an 
           "empty" list is fine ... note that CS_FULL_LINE places the key at 
           the beginning of the buffer. */
        (void) memset(listbuf, 0, sizeof(listbuf));

        if( CS_entry( CS_THIS_LINE, CS_FULL_LINE, sizeof(listbuf), 
                      (void *) listbuf) > 1 ){

            /* listbuf is short-lived ... allow strtok() to "chew" on it ...
               toss-out the "key" ... */
            tok_p = strtok( listbuf, DATAID_STRTOK_STRING );
            while( (tok_p = strtok(NULL, DATAID_STRTOK_STRING)) != NULL )
                ++cnt;

        }

        *num_data_ids = cnt ;

        /* If there are input data IDs listed, parse them. */
        if( *num_data_ids > 0 ){

            int *data_ids_p, i;

            CS_control(CS_KEY_REQUIRED) ;

            /* we had BETTER find the list again! */
            retval2 = CS_entry( key, 0, 0, (void *) NULL );
            if( retval2 == 0 ){

                char buf[ PROD_NAME_LEN ];

                /* malloc temporary storage for data IDs. */
                *data_ids = (int *) malloc( (*num_data_ids)*ALIGNED_SIZE(sizeof(int)) );
                if( *data_ids == NULL ){

                    LE_send_msg( GL_ERROR, "malloc Failed for %d bytes\n", 
                                 (*num_data_ids)*ALIGNED_SIZE(sizeof(int)) );
                    return( -1 );

                }
    
                cnt = 0;
                data_ids_p = *data_ids;
                for( i = 1; i <= *num_data_ids ; ++i ){

                    if( CS_entry( CS_THIS_LINE, i, PROD_NAME_LEN, (void *) buf ) > 0 ){

                        char *start, *end, c;
                        int  num_conv;

                        /* Read the data ID using sscanf.  The number of returned conversions
                           determines the format. */
                        num_conv = sscanf( buf, "%d%c", data_ids_p, &c ); 
                        if( num_conv == 0 ){

                            /* Check for starting ( and closing ). */
                            if( (start = strstr( buf, "(" )) != NULL ){

                                if( (end = strstr( buf, ")" )) == NULL ){

                                    LE_send_msg( GL_ERROR, "Expecting Closing ) for Data ID\n" );
                                    retval = -1;
                                    break;

                                }
                                else
                                    *end = '\0'; 

                                *data_ids_p = atoi( start+1 );
                                *start = '\0';

                                *data_names = STR_append( *data_names, buf, strlen(buf)+1 );
                                
                            }
                            else{

                                LE_send_msg( GL_ERROR, "Expecting Opening ( for Data ID\n" );
                                retval = -1;
                                break;

                            }

                        }
                        else if( num_conv == 1 ){

                            /* The data name is not specified. */
                            *data_names = STR_append( *data_names, "", strlen("")+1 );

                        }
                        else if( num_conv == 2 ){

                            LE_send_msg( GL_ERROR, 
                                 "Invalid Token .... Token Can't Start With Number: %s\n", buf );
                            return( -1 );

                        }

                        ++cnt;
                        ++data_ids_p;

                    }

                } /* End for each of the input Data IDs*/

            }

            /* Sanity check .... the number of tokens must match the number of
               parsed data IDs. */
            if (cnt != *num_data_ids) {

                LE_send_msg( GL_ERROR,"Read %d Data IDs ... Expected %d",
                             cnt, *num_data_ids );
                return( -1 );

            }

       }

    }

    return 0;

/* End of Parse_dataids() */
}

/*********************************************************************

   Description:
      Comparison function for the qsort.   Sorting is based on 
      task_name. 

**********************************************************************/
static int Compare_task_name( const void *arg1, const void *arg2 ){

   Orpgtat_dir_entry_t *entry1 = (Orpgtat_dir_entry_t *) arg1;
   Orpgtat_dir_entry_t *entry2 = (Orpgtat_dir_entry_t *) arg2;

   return( strcmp( entry1->task_name, entry2->task_name ) );

/* End of Compare_task_name() */
}

/*********************************************************************

   Description:
      Comparison function for the qsort.   Sorting is based on
      task_name.

**********************************************************************/
static int Compare_file_name( const void *arg1, const void *arg2 ){

   Orpgtat_dir_entry_t *entry1 = (Orpgtat_dir_entry_t *) arg1;
   Orpgtat_dir_entry_t *entry2 = (Orpgtat_dir_entry_t *) arg2;

   return( strcmp( entry1->file_name, entry2->file_name ) );

/* End of Compare_file_name() */
}

/*******************************************************************
   Description:
      This function performs a binary search of an array of
      Orpgtat_dir_entry_t entries based in task name.  If a match 
      found, returns message ID.

   Inputs:
      array - pointer to array to search.
      size - size of array to search.
      task_name - the task name to find.

   Returns:
      Returns LB ID to entry, or 0 if entry not found.
      (0 is the directory entry LB ID so can never be
       a valid ID.)

******************************************************************/
static LB_id_t Binary_search( Orpgtat_dir_entry_t *array, int size, 
                              char *task_name ){

   int top, middle, bottom;

   /* Set the list top and bottom bounds. */
   top = size - 1;
   if( top == -1 )
      return (-1);

   bottom = 0;

   /* Do Until task_name found or task_name not in list. */
   while( top > bottom ){

      middle = (top + bottom)/2;
      if( strcmp( task_name, (array+middle)->task_name ) > 0 )
         bottom = middle + 1;

      else if( strcmp( task_name, (array+middle)->task_name ) == 0 )
         return ( (array+middle)->msg_id );

      else
         top = middle;

   /* End of "while" loop. */
   }

   /* Modify the task status. */
   if( strcmp( (array+top)->task_name, task_name ) == 0 )
      return ( (array+top)->msg_id );

   else
      return (0);

/* End of Binary_search( ) */
}

