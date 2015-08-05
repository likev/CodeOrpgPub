/**************************************************************************

   Description:

      This file provides the source for the Initialize Task Information
      routines associated with the Task Attribute Table.

 **************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2012/05/08 14:49:02 $
 * $Id: iti_attr.c,v 1.11 2012/05/08 14:49:02 steves Exp $
 * $Revision: 1.11 $
 * $State: Exp $
 */

#include <stdlib.h>   
#include <orpg.h>

#define ITI_ATTR
#include <iti_globals.h>
#undef ITI_ATTR

/* Constant Definitions/Macro Definitions/Type Defintions */


/* Static Global Variables */


/* Static Function Prototypes */
static LB_id_t ITI_check_for_duplicate( Orpgtat_dir_entry_t *dir, int size, 
                                        Orpgtat_entry_t *entry );


/**************************************************************************
   Description: 
      Initialize the Task Attribute Table

   Input: 
      ttcf_fname - task table configuration file filename

   Output: 

   Returns: 0 upon success; otherwise -1

   Notes:

 **************************************************************************/
int ITI_ATTR_init_table( const char *ttcf_fname, char **tat_dir ){

    size_t byte_cnt ;
    Orpgtat_mem_ary_t mem_ary ;
    Orpgtat_dir_entry_t dir_entry;
    int retval;
    int retval2;

    /* Read Task Attribute Table entries from the Task Table Configuration
       File.  Write the "public" array of Task Attribute Table entries
       into ORPGDAT_RPG_INFO/ORPGINFO_TAT_MSGID. */
    mem_ary.size_bytes = 0;
    mem_ary.ptr = NULL;

    /* Read array of Task Attribute Table entries from the Task
       Table Configuration File. */
    retval = ORPGTAT_read_ascii_tbl( &mem_ary, (const char *) ttcf_fname );

    if( retval != 0 ){

        LE_send_msg( GL_ERROR, "ORPGTAT_read_ascii_tbl() returned %d",
                     retval );
        return(-1);

    }

    /* Write the LB messages ... */
    byte_cnt = 0 ;
    while( byte_cnt < mem_ary.size_bytes ){

        Orpgtat_entry_t *entry_p;
        LB_id_t msg_id;
        int i;

        entry_p = (Orpgtat_entry_t *) ((char *) mem_ary.ptr + byte_cnt) ;

        /* Check for duplicate ..... */
        if( *tat_dir != NULL ){

           int length;

           length = STR_size( *tat_dir ) / ALIGNED_SIZE(sizeof(Orpgtat_dir_entry_t));
           msg_id = ITI_check_for_duplicate( (Orpgtat_dir_entry_t *) *tat_dir, length, entry_p );

        }
        else
           msg_id = LB_NEXT;

        /* The return value of the previous function will either be LB_NEXT
           or the message ID of a duplicate entry. */
        retval2 = ORPGDA_write( ORPGDAT_TAT, (char *) entry_p,
                                entry_p->entry_size, msg_id );
        if( retval2 != (int) entry_p->entry_size ){

            LE_send_msg( GL_ERROR, "ORPGDA_write(ORPGDAT_TAT) failed (%d)", retval2 );
            retval = -1;

        }
        else{

            
            dir_entry.msg_id = ORPGDA_previous_msgid( ORPGDAT_TAT );
            strcpy( (char *) &dir_entry.task_name[0], &entry_p->task_name[0] ); 
            strcpy( (char *) &dir_entry.file_name[0], &entry_p->file_name[0] );
            *tat_dir = STR_append( *tat_dir, (char *) &dir_entry, 
                                   sizeof(Orpgtat_dir_entry_t) );

            if( Verbose ){

                LE_send_msg( GL_INFO, "TAT Entry For Task Name: %s, Process Name: %s",
                             entry_p->task_name, entry_p->file_name );
                LE_send_msg( GL_INFO, "--->type: %d, data_stream: %d\n", 
                             entry_p->type, entry_p->data_stream );
                if( entry_p->type & ORPGTAT_TYPE_ALLOW_SUPPL_SCANS )
                   LE_send_msg( GL_INFO, "------>Allow Supplemental Scans\n" );
                if( (entry_p->num_input_dataids > 0) || (entry_p->num_output_dataids > 0) )
                   LE_send_msg( GL_INFO, "---># inputs: %d, # outputs: %d\n", 
                                entry_p->num_input_dataids, entry_p->num_output_dataids );
                if( entry_p->num_input_dataids > 0 ){

                   int *int_p = (int *) ((char *) entry_p + entry_p->input_data);
                   char *char_p = ((char *) entry_p + entry_p->input_names);

                   for( i = 0; i < entry_p->num_input_dataids; i++ ){

                      LE_send_msg( GL_INFO, "------>INPUT: data ID: %d, data name: %s\n",
                                   *int_p, char_p );
                      int_p++;
                      char_p += (strlen(char_p) + 1);

                   }

                }

                if( entry_p->num_output_dataids > 0 ){

                   int *int_p = (int *) ((char *) entry_p + entry_p->output_data);
                   char *char_p = ((char *) entry_p + entry_p->output_names);

                   for( i = 0; i < entry_p->num_output_dataids; i++ ){

                      LE_send_msg( GL_INFO, "------>OUTPUT: data ID: %d, data name: %s\n",
                                   *int_p, char_p );
                      int_p++;
                      char_p += (strlen(char_p) + 1);

                   }

                }

                LE_send_msg( GL_INFO, "\n" );

            }

        }

        byte_cnt += entry_p->entry_size ;

    } /*endwhile stepping through the TAT memory array*/

    free(mem_ary.ptr) ;
    mem_ary.ptr = NULL ;
    mem_ary.size_bytes = 0 ;

    return(retval) ;

/*END of ITI_ATTR_init_table()*/
}

/*************************************************************************
   Description: 
      Writes the Task Attribute Table directory record

   Input: 

   Output: 

   Returns: 0 upon success; otherwise -1

   Notes:

**************************************************************************/
int ITI_ATTR_write_directory_record( char *tat_dir ){

    int retval;
    int length;

    /* Write out the TAT directory if there are entries in the directory
       to write out. */
    length = STR_size( tat_dir );
    
    if( length > 0 ){

        retval = ORPGTAT_write_directory( tat_dir, length );
        STR_free( tat_dir );

    }
    else 
       return(-1);

    return(retval) ;

/*END of ITI_ATTR_write_directory()*/
}

/*************************************************************************

   Description:
      Checks if "entry" is a duplicate of an entry in "dir".  A duplicate
      has the same task_name. 

**************************************************************************/
static LB_id_t ITI_check_for_duplicate( Orpgtat_dir_entry_t *dir, int size, 
                                        Orpgtat_entry_t *entry ){

   Orpgtat_dir_entry_t *dir_p = dir;
   int i;

   /* Go through all directory entries so far and see if there is a match. */
   for( i = 0; i < size; i++ ){

      if( strcmp( dir_p->task_name, entry->task_name ) == 0 ){

         LE_send_msg( GL_INFO, "Replace Duplicate Entry For %s @ %d.\n", 
                      dir_p->task_name, dir_p->msg_id );
         return( dir_p->msg_id );

      }

      dir_p++;

   }

   /* No duplicate found. */
   return( LB_NEXT );

}
