/* 
 * RCS info 
 * $Author: steves $ 
 * $Locker:  $ 
 * $Date: 2005/10/31 20:55:18 $ 
 * $Id: rpgc_data_access_c.c,v 1.5 2005/10/31 20:55:18 steves Exp $ 
 * $Revision: 1.5 $ 
 * $State: Exp $ 
 */ 
#include <rpgc.h>

#define MAX_PATH_LEN   255
#define MAX_UN_REG      10

static int N_UN = 0;
typedef struct {

   int data_id;
   LB_id_t msg_id;
   void (*function)();

} UN_reg_t;

static UN_reg_t UN_reg[MAX_UN_REG];

static void UN_Handler( int fd, LB_id_t msg_id, int msg_info, void *arg );

/*\///////////////////////////////////////////////////////////////
//
//   Description:  Open "data_id" with permission "flags".
//
//   Notes:  See orpgda man page for details.
//
///////////////////////////////////////////////////////////////\*/
int RPGC_data_access_open( int data_id, int flags ){

   return( ORPGDA_open( data_id, flags ) );

}

/*\///////////////////////////////////////////////////////////////
//
//   Description:  Close "data_id".
//
//   Notes:  See orpgda man page for details.
//
///////////////////////////////////////////////////////////////\*/
int RPGC_data_access_close( int data_id ){

   return( ORPGDA_close( data_id ) );

}

/*\///////////////////////////////////////////////////////////////
//
//   Description:  Get message ID of last message read from
//                 "data_id".
//
//   Notes:  See orpgda man page for details.
//
///////////////////////////////////////////////////////////////\*/
LB_id_t RPGC_data_access_previous_msgid ( int data_id ){

   return( ORPGDA_previous_msgid( data_id ) );

}

/*\///////////////////////////////////////////////////////////////
//
//   Description:  Get message info of last message read from
//                 "data_id".
//
//   Notes:  See orpgda man page for details.
//
///////////////////////////////////////////////////////////////\*/
int RPGC_data_access_msg_info( int data_id, LB_id_t id, LB_info *info ){

   return( ORPGDA_msg_info( data_id, id, info ) );

}

/*\///////////////////////////////////////////////////////////////
//
//   Description:  Clear message(s) from "data_id".
//
//   Notes:  See orpgda man page for details.
//
///////////////////////////////////////////////////////////////\*/
int RPGC_data_access_clear( int data_id, int msgs ){

   return( ORPGDA_clear( data_id, msgs ) );

}

/*\///////////////////////////////////////////////////////////////
//
//   Description:  Get status of "data_id".
//
//   Notes:  See orpgda man page for details.
//
///////////////////////////////////////////////////////////////\*/
int RPGC_data_access_stat( int data_id, LB_status *status ){

   return( ORPGDA_stat( data_id, status ) );

}

/*\///////////////////////////////////////////////////////////////
//
//   Description:  Get information about messages in "data_id".
//
//   Notes:  See orpgda man page for details.
//
///////////////////////////////////////////////////////////////\*/
int RPGC_data_access_list( int data_id, LB_info *list, int nlist ){

   return( ORPGDA_list( data_id, list, nlist ) );

}

/*\///////////////////////////////////////////////////////////////
//
//   Description:  Register for UN Notification for "data_id" 
//                 and "msg_id".
//
//   Notes:  See orpgda man page for details.
//
///////////////////////////////////////////////////////////////\*/
int RPGC_data_access_UN_register( int data_id, LB_id_t msg_id,
                                  void (*callback)() ){

   int i;

   /* Check if this UN registration has already occurred. */
   for( i = 0; i < N_UN; i++ ){

      if( (data_id == UN_reg[i].data_id)
                   &&
          (msg_id == UN_reg[i].msg_id)
                   &&
          (callback == UN_reg[i].function) )
         return 0;

   }

   /* Not enough space to allow registration. */
   if( i >= MAX_UN_REG ){

      LE_send_msg( GL_ERROR, "Unable to Register Notification\n" );
      return(-1);

   }

   /* Do the registration. */
   UN_reg[N_UN].data_id = data_id;
   UN_reg[N_UN].msg_id = msg_id;
   UN_reg[N_UN].function = callback;
   N_UN++;

   return( ORPGDA_UN_register( data_id, msg_id, UN_Handler ) );

}

/*\///////////////////////////////////////////////////////////////
//
//  Description:  This module provides a C/C++ interface to
//                the ORPGDA_seek function.
//
//  Input:        data_id - data id to be associated with group
//                offset - offset relative to msg_id.
//                msg_id - message id associated with the data
//
//  Output:       msg_id - message id associated with seeked 
//                         message
//
//  Return:       The size of the message on success or negative
//                error value otherwise.
//
//  Notes:        Refer to orpgda man page for more details on 
//                ORPGDA_seek.
//
/////////////////////////////////////////////////////////////////\*/      
int RPGC_data_access_seek( int data_id, int offset, LB_id_t *msg_id ){

   LB_info info;
   int ret;

   ret = ORPGDA_seek( data_id, offset, *msg_id, &info );
   if( ret >= 0 ){

      ret = info.size;
      *msg_id = info.id;
      if( (info.id == LB_SEEK_TO_COME) || (info.id == LB_SEEK_EXPIRED) )
         ret = 0;

   }

   return( ret );
         
}

/*\///////////////////////////////////////////////////////////////
//
//  Description:  This module provides a C/C++ interface to
//                the ORPGDA_read function.
//
//  Input:        data_id - data id to be associated with group
//                buf - pointer to buffer where data to to be 
//                      placed when read
//                buflen - size, in bytes, of message buffer 
//                msg_id - message id associated with the data
//
//  Return:       The return value from ORPGDA_read.
//
//  Notes:        Refer to orpgda man page for more details on 
//                ORPGDA_read.
//
/////////////////////////////////////////////////////////////////\*/      
int RPGC_data_access_read( int data_id, void *buf, int buflen,
                           LB_id_t msg_id ){

   return( ORPGDA_read( data_id, (char *) buf, buflen, msg_id ) );

}

/*\///////////////////////////////////////////////////////////////
//
//  Description:  This module provides a C/C++ interface to
//                the ORPGDA_write function.
//
//  Input:        data_id - data id to be associated with group
//                buf - pointer to buffer where data to to be 
//                      written resides
//                length - size, in bytes, of data to be written
//                msg_id - message id associated with the data
//
//  Return:       The return value from ORPGDA_write. 
//
//  Notes:        Refer to orpgda man page for more details on 
//                ORPGDA_write.
//
/////////////////////////////////////////////////////////////////\*/      
int RPGC_data_access_write( int data_id, void *buf, int length,
                            LB_id_t msg_id ){

   /* Send the sender's PID in the event the reader of the data 
      cares. */
   pid_t pid = getpid();
   EN_control( EN_SET_SENDER_ID, (unsigned short) pid );

   return( ORPGDA_write( data_id, (char *) buf, length, msg_id ) );

}

/*\///////////////////////////////////////////////////////////////
//
//  Description:  This module returns the path name of the 
//                current working directory (i.e., output of
//                MISC_get_work_dir).
//
//  Output:       The path name of the current working directory.
//
//  Return:       The size of the path name, in bytes.  Does not
//                include trailing '/0'. 
//
//  Notes:        Refer to misc man page for more details on 
//                MISC_get_work_dir.
//
//                The caller is responsible for releasing the
//                memory allocated for "path_name".
//
/////////////////////////////////////////////////////////////////\*/      
int RPGC_get_working_dir( char **path_name ){

   static char working_dir[MAX_PATH_LEN];
   int len, ret;

   /* Clear the path */
   memset( working_dir, 0, MAX_PATH_LEN );
   *path_name = NULL;

   /* Gets the path of the "work" or "tmp" directory.  If succeeds, 
      the path is NULL-terminated without trailing "/". */
   ret = MISC_get_work_dir( working_dir, MAX_PATH_LEN );
   if( ret <= 0 )
      return ret;

   /* Append the trailing "/" */
   strcat( working_dir, "/" );

   len = strlen( working_dir );

   if( len > 0 ){

      *path_name = (char *) calloc( 1, len+1 );
      if( *path_name == NULL )
         return -1;

   }
 
   strcat( *path_name, working_dir );

   return len;

}

/*\///////////////////////////////////////////////////////////////
//
//  Description:  Given a file name string, this function returns
//                the fully qualified name of the file.  All
//                files using this interface are directory to
//                the directory returned from MISC_get_work_dir().
//
//  Input:        file_name - pointer to file name string.
//
//  Output:       path_name - on success, a buffer is malloc'd
//                which contains the fully qualified file name.
//
//  Return:       The length of the string "*path_name".
//
//  Notes:        Refer to misc man page for more details on 
//                MISC_get_work_dir
//
//                The caller is responsible for releasing the
//                memory allocated for "path_name".
//
/////////////////////////////////////////////////////////////////\*/      
int RPGC_construct_file_name( char *file_name, char **path_name ){

   int len;

   len = RPGC_get_working_dir( path_name );

   if( (len > 0) && (*path_name != NULL) && (file_name != NULL)){

      len += strlen( file_name );
      *path_name = realloc( *path_name, len+1 ); 
      strcat( *path_name, file_name );
      len = strlen( *path_name );

   }

   return len;

}

/*\///////////////////////////////////////////////////////////////
//
//  Description:  UN Notifcation handler.   Calls user defined
//                handler with data ID and Message ID as 
//                arguments.
//
//  Notes:        Refer to orpgda man page for more details. 
//
//
/////////////////////////////////////////////////////////////////\*/      
void UN_Handler( int fd, LB_id_t msg_id, int msg_info, void *arg ){

   int i;

   /* Check for match on fd and msg_id.  Call callback with data ID
      and msg_id */
   for( i = 0; i < N_UN; i++ ){

      if( (ORPGDA_lbfd( UN_reg[i].data_id ) == fd) 
                         &&
          ((UN_reg[i].msg_id == LB_ANY) || (msg_id == UN_reg[i].msg_id)) ){

         UN_reg[i].function( UN_reg[i].data_id, msg_id );
         break;

      }

   }

}
