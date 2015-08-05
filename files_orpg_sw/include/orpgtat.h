/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2012/05/03 20:38:42 $
 * $Id: orpgtat.h,v 1.42 2012/05/03 20:38:42 steves Exp $
 * $Revision: 1.42 $
 * $State: Exp $
 */

/**************************************************************************

      Module: orpgtat.h

 Description: ORPG Task Attribute Table (ORPGTAT_) public header file.

 Assumptions:

 **************************************************************************/


/**@#-*/ /*CcDoc Token Processing OFF*/
#ifndef ORPGTAT_H
#define ORPGTAT_H
/**@#+*/ /*CcDoc Token Processing ON*/

#include <sys/types.h>
#include <cs.h>
#include <lb.h>
#include <orpg_def.h>
#include <orpgtask.h>
#include <mrpg.h>

/*
 * ORPG Task Tables (TT) CS File
 */
#define ORPGTAT_CS_DFLT_TT_FNAME	"task_attr_table"
#define ORPGTAT_CS_TT_COMMENT		'#'
#define ORPGTAT_CS_TT_MAXLINELEN        256


#define ORPGTAT_CS_ATTR_TASK_KEY	"Task"
#define ORPGTAT_CS_NAME_KEY		"filename"
#define ORPGTAT_CS_NAME_TOK		1
#define ORPGTAT_CS_DATA_STREAM_KEY	"data_stream"
#define ORPGTAT_CS_DATA_STREAM_TOK	(1 | (CS_SHORT))
#define ORPGTAT_CS_DESCRIPTION_KEY	"desc"
#define ORPGTAT_CS_DESCRIPTION_TOK	1
#define ORPGTAT_CS_DESCRIPTION_BUFSIZE	(ORPGTAT_CS_TT_MAXLINELEN)

#define ORPGTAT_CS_TYPE_RESPAWN_KEY          "respawn"
#define ORPGTAT_CS_TYPE_ALLOW_DUP_KEY        "allow_duplicate"
#define ORPGTAT_CS_TYPE_RPG_CNTL_KEY         "rpg_control_task"
#define ORPGTAT_CS_TYPE_MON_ONLY_KEY         "monitor_only"
#define ORPGTAT_CS_TYPE_DONT_MON_KEY         "do_not_monitor"
#define ORPGTAT_CS_TYPE_SITE_KEY             "site"
#define ORPGTAT_CS_TYPE_ALLOW_SUPPL_SCANS_KEY    "allow_supplemental_scans"

/*
 * Input/Output Data tokens ...
 */
#define ORPGTAT_CS_INDATA_KEY		"input_data"
#define ORPGTAT_CS_OUTDATA_KEY		"output_data"

/*
 * Maximum Number of Instances Arguments List tokens ...
 */
#define ORPGTAT_CS_ARGS_KEY		"args"
#define ORPGTAT_CS_ARGLIST_INDEX_TOK	(0 | (CS_SHORT))
#define ORPGTAT_CS_ARGLIST_TOK	1
#define ORPGTAT_CS_ARGLIST_BUFSIZE	(ORPGTASK_MAX_TASKARG_SIZ)

/*
 * Some defaults ...
 *
 * arguments - verbose switch
 */
#define ORPGTAT_CS_DFLT_ARGS "-v"

/*
 * Input Data Stream Macros
 */
#define ORPGTAT_UNKNOWN_STREAM      	0
#define ORPGTAT_REALTIME_STREAM     	1
#define ORPGTAT_REPLAY_STREAM       	2

/**
  * TAT arguments entry.
  */
typedef struct {
                               /** Task Instance # (0, 1, 2, ... N) */
    short instance ;
                               /** Task arguments */
    char args[ORPG_PATHNAME_SIZ] ;
} Orpgtat_args_entry_t ;

/* Bitflags for TAT Task Type values ... */
#define ORPGTAT_TYPE_RESPAWN         0x0001
#define ORPGTAT_TYPE_ALLOW_DUP       0x0002
#define ORPGTAT_TYPE_RPG_CNTL        0x0004
#define ORPGTAT_TYPE_MON_ONLY        0x0008
#define ORPGTAT_TYPE_DONT_MON        0x0010

/* Bitflags for "site" tasks. */
#define ORPGTAT_TYPE_PROD_SERVER     0x0020
#define ORPGTAT_TYPE_COMM_MANAGER    0x0040

/* Bitflags for Controlling Input. */
#define ORPGTAT_TYPE_ALLOW_SUPPL_SCANS   0x0100

typedef struct {

    char task_name[ORPG_TASKNAME_SIZ] ;
                               /* logical (task) name.                    */
    char file_name[ORPG_TASKNAME_SIZ] ;
                               /* executable (process) name.              */
    size_t entry_size ;        /* Size of task table entry, in bytes.     */
    unsigned int type ;        /* bitflags; refer to ORPGTAT_TYPE_ macros */
    short data_stream ;        /* Data stream identifier .... a number
                                  between 0 and 255.                      */
    short desc ;               /* offset (bytes from start of this struct)*/
                               /* to null-terminated task description     */
    short maxn_inst ;          /* max number of instances permitted       */
    short args ;               /* offset (bytes from start of this struct)*/
                               /* to task arguments list                  */
    short num_input_dataids ;
    short input_data ;         /* offset (bytes from start of this struct)*/
                               /* to list of input Data IDs ... access as */
                               /* an array of integers ...                */
    short input_names ;        /* offset (bytes from start of this struct)*/
                               /* to list of input Data Names ... access  */
                               /* as an array of strings ...              */
    short num_output_dataids ;
    short output_data ;        /* offset (bytes from start of this struct)*/
                               /* to list of output Data IDs ... access as*/
                               /* an array of integers ...                */
    short output_names ;       /* offset (bytes from start of this struct)*/
                               /* to list of output Data Names ... access */
                               /* as an array of strings ...              */
} Orpgtat_entry_t ;

/*
 * Use this typedef when storing TAT in memory ...
 */
typedef struct {

    size_t size_bytes ;        /* size of memory array of TAT entries     */
    Orpgtat_entry_t *ptr ;     /* pointer to array of TAT entries         */

} Orpgtat_mem_ary_t ;


/* The task table directory entries are sorted according to task_name.  The
   index field is the order of this entry based on file_name sort. The entry
   having file_name first in sorted order has index 0. */
typedef struct {

   LB_id_t msg_id;
   char task_name[ ORPG_TASKNAME_SIZ ];
   char file_name[ ORPG_TASKNAME_SIZ ];
   int  index;
  
} Orpgtat_dir_entry_t;          

/* Reserved message ID for TAT directory. */
#define ORPGTAT_TAT_DIRECTORY           0

/* return error codes */
#define ORPGTAT_ARGV_NOT_FOUND        -10001
#define ORPGTAT_TASK_TABLE_NOT_FOUND  -10002
#define ORPGTAT_FILE_NAME_NOT_FOUND   -10003
#define ORPGTAT_TASK_NAME_NOT_FOUND   -10004

/* Function Prototypes */
void ORPGTAT_set_args( int argc, char **argv );
int ORPGTAT_get_data_id_from_name( Orpgtat_entry_t *task_entry, char *data_name ); 
Orpgtat_entry_t *ORPGTAT_get_entry( char *task_name );
int ORPGTAT_read_ascii_tbl( Orpgtat_mem_ary_t *mem_ary_p, const char *tasktbl_fname );
int ORPGTAT_get_directory( Orpgtat_dir_entry_t **dir_p );
int ORPGTAT_write_directory( char *dir_p, int length );
int ORPGTAT_get_my_task_name( char *task_name_buf, int buf_size );

#endif /* #ifndef ORPGTAT_H */
