/*
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2004/12/09 21:53:53 $
 * $Id: mnttsk_rda_alarms_tbl_init.c,v 1.7 2004/12/09 21:53:53 jing Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */

#include <stdio.h>
#include <stdlib.h>            /* EXIT_SUCCESS, EXIT_FAILURE              */
#include <unistd.h>            /* unlink()                                */
#include <string.h>            /* strncpy()                               */


#include "mnttsk_rda_alarms_tbl.h"


/* Constant Definitions/Macro Definitions/Type Definitions */

/* Global Variables */
#define CFG_NAME_SIZE                 128
static char Cfg_dir            [CFG_NAME_SIZE];  /* config directory pathname */

extern char RDA_alarms_table_name[NAME_SIZE];
extern char ORDA_alarms_table_name[NAME_SIZE];

/* Static Global Variables */

/* Static Function Prototypes */
static int Read_ascii_rda_alarms_tbl( Orpgrat_mem_ary_t *mem_ary_p, 
                                      char *alarmtbl_fname, int table );
static int Get_alarm_data( Orpgrat_mem_ary_t *mem_ary_p, int table ) ;
static int Allocate_entry_storage( Orpgrat_mem_ary_t *ary_p, RDA_alarm_entry_t **entry_pp,
                                   size_t incr_size, size_t cur_entry_offset,
                                   size_t offset ) ;

/********************************************************************
   Description:
      Initialize RDA Alarms Table.

   Inputs:
      startup_action - start up type.

   Outputs:

   Returns:
      Negative value on error, or 0 on success.
      
   Notes:

********************************************************************/
int MNTTSK_RDA_ALARMS_TBL_maint( int startup_action ){

   int retval = 0;
   char *buf;

   /* Clear */
   if (startup_action == CLEAR) {

      LE_send_msg(GL_INFO,
                  "CLEAR: RDA Alarms Table Maintenance Duties:") ;
      LE_send_msg(GL_INFO,
                  "\t1. Initialize RDA Alarms Table(s).") ;

      ORPGRAT_clear_rda_alarms_tbl( ORPGRAT_RDA_TABLE );
      ORPGRAT_clear_rda_alarms_tbl( ORPGRAT_ORDA_TABLE );

   }

   else if( (startup_action == RESTART) || (startup_action == STARTUP) ){

      /* If the RDA Alarms table exists, do not initialize */
      retval = ORPGDA_read( ORPGDAT_RDA_ALARMS_TBL, &buf, LB_ALLOC_BUF,
			    ORPGRAT_ALARMS_TBL_MSG_ID );
      if( retval > 0 ){

         free(buf);
         LE_send_msg( GL_INFO, "RDA Alarms Table Exists ... Do Nothing\n" );

      }
      else{

         Orpgrat_mem_ary_t mem_ary_p;
         char tmpbuf [CFG_NAME_SIZE];
         int len;
  
         /* get the configuration source directory. */
         len = MISC_get_cfg_dir (Cfg_dir, CFG_NAME_SIZE);
         if (len > 0)
            strcat (Cfg_dir, "/");

         strcpy (tmpbuf, Cfg_dir);
         strcat (tmpbuf, RDA_alarms_table_name);
         strcpy (RDA_alarms_table_name, tmpbuf);

         LE_send_msg( GL_INFO, "The RDA Alarms Table Name: %s\n", RDA_alarms_table_name );

         memset( (void *) &mem_ary_p, 0, sizeof(Orpgrat_mem_ary_t) );
         retval = Read_ascii_rda_alarms_tbl( &mem_ary_p, RDA_alarms_table_name, 
                                             ORPGRAT_RDA_TABLE );
         if( retval < 0 ){

            LE_send_msg( GL_ERROR, "Read_ascii_rda_alarms_tbl Failed for RDA Alarms\n" );
            return(-1);

         }
         else
            LE_send_msg( GL_INFO, "Read_ascii_rda_alarms_tbl Successful for RDA Alarms\n" );

         retval = ORPGDA_write( ORPGDAT_RDA_ALARMS_TBL, (char *) mem_ary_p.ptr, 
                                mem_ary_p.size_bytes, ORPGRAT_ALARMS_TBL_MSG_ID );
         if( retval < 0 ){

            LE_send_msg( GL_ERROR, "ORPGDAT_RDA_ALARMS_TBL Write Failed for RDA Alarms (%d)\n",
                         retval );

            return(-1);

         }

         else
            LE_send_msg( GL_INFO, "ORPGDAT_RDA_ALARMS_TBL Write Complete for RDA Alarms\n" );

      }

      /* If the ORDA Alarms table exists, do not initialize */
      retval = ORPGDA_read( ORPGDAT_RDA_ALARMS_TBL, &buf, LB_ALLOC_BUF,
			    ORPGRAT_ORDA_ALARMS_TBL_MSG_ID );
      if( retval > 0 ){

         free(buf);
         LE_send_msg( GL_INFO, "ORDA Alarms Table Exists ... Do Nothing\n" );
         return(0);

      }
      else{

         Orpgrat_mem_ary_t mem_ary_p;
         char tmpbuf [CFG_NAME_SIZE];
         int len;

         len = MISC_get_cfg_dir (Cfg_dir, CFG_NAME_SIZE);
         if (len > 0)
            strcat (Cfg_dir, "/");

         strcpy (tmpbuf, Cfg_dir);
         strcat (tmpbuf, ORDA_alarms_table_name);
         strcpy (ORDA_alarms_table_name, tmpbuf);

         LE_send_msg( GL_INFO, "The ORDA Alarms Table Name: %s\n", ORDA_alarms_table_name );

         memset( (void *) &mem_ary_p, 0, sizeof(Orpgrat_mem_ary_t) );
         retval = Read_ascii_rda_alarms_tbl( &mem_ary_p, ORDA_alarms_table_name, 
                                             ORPGRAT_ORDA_TABLE );
         if( retval < 0 ){

            LE_send_msg( GL_ERROR, "Read_ascii_rda_alarms_tbl Failed for ORDA Alarms\n" );
            return(-1);

         }
         else
            LE_send_msg( GL_INFO, "Read_ascii_rda_alarms_tbl Successful for ORDA Alarms\n" );

         retval = ORPGDA_write( ORPGDAT_RDA_ALARMS_TBL, (char *) mem_ary_p.ptr, 
                                mem_ary_p.size_bytes, ORPGRAT_ORDA_ALARMS_TBL_MSG_ID );
         if( retval < 0 ){

            LE_send_msg( GL_ERROR, "ORPGDAT_RDA_ALARMS_TBL Write Failed for ORDA Alarms (%d)\n",
                         retval );

            return(-1);

         }

         else
            LE_send_msg( GL_INFO, "ORPGDAT_RDA_ALARMS_TBL Write Complete for ORDA alarms\n" );
      }
  
   }

   return(0) ;

/*END of MNTTSK_PGT_INFO_maint()*/
}

/**************************************************************************
   Description: 
      Reads the entire RDA (ORDA) Alarm Table from ASCII configuration file. 

   Input: 
      mem_ary_p - pointer to Orpgrat_mem_ary_t structure that is to 
                  be initialized.
      alarmtbl_fname - pointer to RDA Alarms Table filename (may be NULL)
      table - specifies the table we are processing (RDA or ORDA)

   Output: 
      the RDA Alarms Table entries will be placed in the array storage
      provided by this process

   Returns: 0 if successful; -1 otherwise

   Notes:

      If the Orpgrat_mem_ary_p array pointer ("ptr") is not NULL,
      we assume memory has been allocated and should be freed.
 
      If the RDA Alarms Table filename is NULL, the entries will be
      read from the ORPGDAT_RDA_ALARMS_TBL data store.

      CS_entry() guarantees NULL-terminated string

      After re-allocating memory for a given entry, we update the
      entry pointer with respect to the (possibly changed) array
      pointer.

 **************************************************************************/
static int Read_ascii_rda_alarms_tbl( Orpgrat_mem_ary_t *mem_ary_p, 
                                      char *alarmtbl_fname, int table ){

   RDA_alarm_entry_t *entry_p ;
   size_t incr_size ;
   int retval = 0 ;
   char *save_cfg_name ;      /* CS cfg name upon entering this routine  */
   char *tmp_char_p ;

   /* Error checking. */
   if( mem_ary_p == NULL ){

      LE_send_msg( GL_INFO,"Read_ascii_rda_alarms_tbl: mem_ary_p NULL") ;
      return(-1) ;

   }

   /* Check that the RDA (ORDA) alarm table configuration file name is defined. */
   if( alarmtbl_fname == NULL ){

      retval = Get_alarm_data( mem_ary_p, table ) ;
      LE_send_msg(GL_INFO,
                  "Read_ascii_rda_alarms_tbl: Get_alarm_data for table %d returned %d",
                   table, retval) ;
      return(retval);

   }

   /* Save a copy of the current CS configuration name, as we'll need to 
      restore that when we leave this function ... 

      CS_cfg_name() returns a NULL-terminated string ... */
   tmp_char_p = CS_cfg_name(NULL) ;
   if( strlen(tmp_char_p) > 0 ){

       save_cfg_name = malloc(strlen(tmp_char_p) + 1) ;    
       if( save_cfg_name != NULL )
          (void) strncpy( save_cfg_name, (const char *) tmp_char_p,
                          strlen(tmp_char_p)+1) ;
        else{

           LE_send_msg(GL_MEMORY,"malloc(save_cfg_name) failed!") ;
           return(-1) ;

        }
   }
   else{

      save_cfg_name = malloc(strlen("") + 1) ;    
      if( save_cfg_name != NULL ) 
         (void) strncpy(save_cfg_name, (const char *) "", strlen("")+1) ;

      else{

         LE_send_msg(GL_MEMORY,"malloc(empty save_cfg_name) failed!") ;
         return(-1) ;

      }

   }

   /* Indicate we're not ingesting the (default) RDA alarms configuration
      file and that comment lines begin with '#' ... */
   if( strlen(alarmtbl_fname) )
      (void) CS_cfg_name((char *) alarmtbl_fname) ;

   else{

      if( table == ORPGRAT_RDA_TABLE )
         (void) CS_cfg_name((char *) ORPGRAT_CS_DFLT_ALARM_FNAME) ;

      else 
         (void) CS_cfg_name((char *) ORPGRAT_CS_DFLT_ORDA_ALARM_FNAME) ;

   }

   CS_control(CS_COMMENT | ORPGRAT_CS_ALARM_COMMENT) ;

   /* Proceed to the RDA Alarm Table ... and step down into it ... */
   CS_control(CS_TOP_LEVEL) ;
   if( (CS_entry(ORPGRAT_CS_ALRMTBL_KEY, 0, 0, (char *) NULL) < 0)
                           ||
       (CS_level(CS_DOWN_LEVEL) < 0 ) ){

      (void) CS_cfg_name(save_cfg_name) ;
       free(save_cfg_name) ;
       LE_send_msg(GL_INFO,"Read_ascii_rda_alarms_tbl: Unable To Step Into RAT!") ;
       return(-1) ;

   }

   do{

      /* Automatic variables ...  */
      char alarm_text[ORPGRAT_CS_TEXT_BUFSIZE] ;
                               /* temporary storage for alarm text  */
      size_t cur_entry_offset ;
                               /* offset (bytes) from start of array      */
      int len ;
      size_t offset ;
                               /* offset (bytes) from start of entry      */

      offset = 0 ;
      cur_entry_offset = mem_ary_p->size_bytes ;

      /* Step down into the RDA Alarms Table Entry ... */
      if( CS_level(CS_DOWN_LEVEL < 0) )
          continue ;

      /* Initial memory allocation for this entry ... */
      incr_size = ALIGNED_SIZE(sizeof(RDA_alarm_entry_t)) ;
      if( Allocate_entry_storage( mem_ary_p, &entry_p, incr_size,
                                 cur_entry_offset, offset ) < 0 ){

         LE_send_msg( GL_MEMORY, "Allocate_entry_storage(inital) failed!" ) ;
         retval = -1 ;
         break ;

      }

      offset = ALIGNED_SIZE(sizeof(RDA_alarm_entry_t)) ;
      if( CS_entry( ORPGRAT_CS_CODE_KEY, ORPGRAT_CS_CODE_TOK,
                     0, (void *) &(entry_p->code) ) <= 0 ){

         LE_send_msg(GL_INFO,"CS_entry(default fields) failed!") ;
         retval = -1 ;
         break ;

      }

      CS_control(CS_KEY_OPTIONAL) ;
      if( CS_entry( ORPGRAT_CS_STATE_KEY, ORPGRAT_CS_STATE_TOK,
                    0, (void *) &(entry_p->state) ) <= 0 )
         entry_p->state = ORPGRAT_NOT_APPLICABLE ;

      if( CS_entry( ORPGRAT_CS_TYPE_KEY, ORPGRAT_CS_TYPE_TOK,
                    0, (void *) &(entry_p->type) ) <= 0 )
         entry_p->type = ORPGRAT_NOT_APPLICABLE ;

      if( CS_entry( ORPGRAT_CS_DEVICE_KEY, ORPGRAT_CS_DEVICE_TOK,
                    0, (void *) &(entry_p->device) ) < 0 )
         entry_p->device = ORPGRAT_NOT_APPLICABLE ;

      if( CS_entry( ORPGRAT_CS_SAMPLE_KEY, ORPGRAT_CS_SAMPLE_TOK,
                    0, (void *) &(entry_p->sample) ) < 0 )
         entry_p->sample = ORPGRAT_NOT_APPLICABLE ;

      CS_control(CS_KEY_REQUIRED) ;

      /* Read the RDA alarm text ... note that the text is NOT optional ... 
         so, we must allocate at least one byte for the terminating null ... */
      if( (len = CS_entry( ORPGRAT_CS_TEXT_KEY, ORPGRAT_CS_TEXT_TOK,
                           ORPGRAT_CS_TEXT_BUFSIZE, (void *) alarm_text ) ) < 0 ){

            LE_send_msg( GL_CS(len),"CS_entry(alarm_text) failed! (%d)", 
                         entry_p->code ) ;
            retval = -1 ;
            break ;
      }

      (void) strncpy((char *) entry_p->alarm_text, alarm_text, len) ;

      (void) CS_level(CS_UP_LEVEL) ;

    } while (CS_entry(CS_NEXT_LINE, 0, 0, (char *) NULL) >= 0);

    CS_control(CS_DELETE) ;

    if ((retval != 0) && (mem_ary_p->ptr != NULL)) {
        free(mem_ary_p->ptr) ;
        mem_ary_p->ptr = NULL ;
        mem_ary_p->size_bytes = 0 ;
    }

    (void) CS_cfg_name(save_cfg_name) ;
    free(save_cfg_name) ;

    return(retval) ;

/*END of ORPGRAT_read_ascii_rda_alarms_tbl()*/
}

/**************************************************************************
   Description: 
      Reallocate storage for a given table entry

   Input: 
pointer-to- memory RAT array structure
              pointer-to-pointer to table entry
              required storage increase increment (bytes)
              offset of current entry from start of array of table entries
              offset from start of entry to storage increment
      Output:
     Returns: 0 if successful; -1 otherwise
       Notes:
 **************************************************************************/
static int Allocate_entry_storage( Orpgrat_mem_ary_t *mem_ary_p,
                                   RDA_alarm_entry_t **entry_pp, size_t incr_size,
                                   size_t cur_entry_offset, size_t incr_offset){

    void *save_p = mem_ary_p->ptr ;

    if( save_p == NULL ){

       /* insure++ indicates that although Standard C supports passing 
          null pointer to realloc(), doing so may result in portability 
          problems ... */

       mem_ary_p->ptr = malloc(incr_size) ;
       if( mem_ary_p->ptr == NULL ){

          LE_send_msg( GL_MEMORY,
                       "malloc() Failed For %d Bytes", incr_size ) ;
          return(-1) ;

      }

   }
   else{

      mem_ary_p->ptr = realloc( mem_ary_p->ptr,
                                mem_ary_p->size_bytes + incr_size) ;
      if( mem_ary_p->ptr == NULL ){

         LE_send_msg( GL_MEMORY,
                      "realloc() Failed for %d Bytes", mem_ary_p->size_bytes + incr_size ) ;
         free(save_p) ;
         return(-1) ;

      }

   }

   /* Since the value of the array pointer may have changed, update
      the current entry pointer.  Zero-out the storage increment.
      Increment the entry and array sizes. */
   *entry_pp = (RDA_alarm_entry_t *)
               ((char *) mem_ary_p->ptr + cur_entry_offset) ;
   (void) memset((char *) *entry_pp + incr_offset, 0, incr_size) ;
   mem_ary_p->size_bytes += incr_size ;

   return(0) ;

/* END of Allocate_entry_storage() */
}

/**************************************************************************
   Description: 
      Read the RDA Alarms Table from ORPGDAT_RDA_ALARMS_TBL/
      ORPGRAT_RDA_ALARMS_TBL_MSG_ID.

   Input: 
      mem_ary_p -  pointer to memory RAT structure
      table - which table to read

   Output: the entries will be placed in the array storage
           the size of the memory RAT array structure will be stored

   Returns: 
      0 if successful; -1 otherwise

   Notes:
**************************************************************************/
static int Get_alarm_data( Orpgrat_mem_ary_t *mem_ary_p, int table ){

   LB_info lb_info ;
   int retval = 0 ;

   /* Assume the worst ... */
   mem_ary_p->ptr = NULL ;
   mem_ary_p->size_bytes = 0 ;

   /* Determine the size of the ORPGRAT_ALARMS_TBL_MSG_ID message. */
   if( table == ORPGRAT_RDA_TABLE )
      retval = ORPGDA_info( ORPGDAT_RDA_ALARMS_TBL, 
                            (LB_id_t) ORPGRAT_ALARMS_TBL_MSG_ID,
                            &lb_info ) ;

   else
      retval = ORPGDA_info( ORPGDAT_RDA_ALARMS_TBL, 
                            (LB_id_t) ORPGRAT_ORDA_ALARMS_TBL_MSG_ID,
                            &lb_info ) ;

   if( retval != LB_SUCCESS ){

      LE_send_msg( GL_ORPGDA(retval),
                   "ORPGDA_INFO ORPGDAT_RDA_ALARMS_TBL Failed (%d)",
                   retval) ;
        return(-1) ;

   }

   if( lb_info.size <= 0 ){

      LE_send_msg( GL_ERROR, "ORPGRAT_ALARMS_TBL_MSG_ID Size %d <= 0",
                   lb_info.size) ;
        return(-1) ;

   }

   mem_ary_p->ptr = calloc((size_t) 1, (size_t) lb_info.size) ;
   if( mem_ary_p->ptr == NULL ){

      LE_send_msg( GL_MEMORY,
                   "Failed to calloc() %d bytes for RDA Alarms Table",
                    lb_info.size) ;
      return(-1) ;

   }

   if( table == ORPGRAT_RDA_TABLE )
      retval = ORPGDA_read( ORPGDAT_RDA_ALARMS_TBL,
                            (char *) mem_ary_p->ptr,
                            lb_info.size,
                            (LB_id_t) ORPGRAT_ALARMS_TBL_MSG_ID) ;

   else
      retval = ORPGDA_read( ORPGDAT_RDA_ALARMS_TBL,
                            (char *) mem_ary_p->ptr,
                            lb_info.size,
                            (LB_id_t) ORPGRAT_ORDA_ALARMS_TBL_MSG_ID) ;

   if( retval != lb_info.size ){

      LE_send_msg( GL_ORPGDA(retval),
                   "ORPGDA_read ORPGDAT_RDA_ALARMS_TBL Failed (%d)",
                    retval) ;
      return(-1) ;

   }

   mem_ary_p->size_bytes = lb_info.size ;

   return(0) ;

/*END of Get_alarm_data()*/
}
