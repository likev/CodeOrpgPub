/* 
 * RCS info 
 * $Author: ccalvert $ 
 * $Locker:  $ 
 * $Date: 2004/02/05 23:21:18 $ 
 * $Id: rpg_data_access.c,v 1.4 2004/02/05 23:21:18 ccalvert Exp $ 
 * $Revision: 1.4 $ 
 * $State: Exp $ 
 */ 
#include <rpg.h>

/*\///////////////////////////////////////////////////////////////
//
//  Description:  This module provides a FORTRAN interface to
//                the ORPGDA_group function.
//
//  Input:        group - group number associated with group
//                data_id - data id to be associated with group
//                msg_id - message id associated with the data
//                msg - pointer to buffer where data to to be 
//                      placed when read
//                msg_size - size, in bytes, of msg buffer 
//
//  Return:       There are no return values defined.  The return
//                value from ORPGDA_group is saved in the status
//                argument.
//
//  Notes:        Refer to orpgda man page for more details on
//                ORPGDA_group.
//
/////////////////////////////////////////////////////////////////\*/      
void RPG_data_access_group( fint *group, fint *data_id, fint *msg_id,
                            void *msg, fint *msg_size, fint *status ){

   *status = ORPGDA_group( *group, *data_id, (LB_id_t ) *msg_id, 
                           (char *) msg, *msg_size ); 

   /*
     If status is negative, error. 
   */
   if( *status < 0 )
      LE_send_msg( GL_INFO, "ORPGDA_group Return Bad Status: Ret = %d\n", 
                   *status );

}

/*\///////////////////////////////////////////////////////////////
//
//  Description:  This module provides a FORTRAN interface to
//                the ORPGDA_update function.
//
//  Input:        group - group number associated with group
//
//  Return:       There are no return values defined.  The return
//                value from ORPGDA_update is saved in the status
//                argument.
//
//  Notes:        Refer to orpgda man page for more details on 
//                ORPGDA_update.
//
/////////////////////////////////////////////////////////////////\*/      
void RPG_data_access_update( fint *group, fint *status ){

   *status = ORPGDA_update( *group );

   /*
     If status is negative, report error. 
   */
   if( *status < 0 )
      LE_send_msg( GL_INFO, "ORPGDA_update Return Bad Status: Ret = %d\n", 
                   *status );

}

/*\///////////////////////////////////////////////////////////////
//
//  Description:  This module provides a FORTRAN interface to
//                the ORPGDA_read function.
//
//  Input:        data_id - data id to be associated with group
//                buf - pointer to buffer where data to to be 
//                      placed when read
//                buflen - size, in bytes, of message buffer 
//                msg_id - message id associated with the data
//
//  Return:       There are no return values defined.  The return
//                value from ORPGDA_read is saved in the status
//                argument.
//
//  Notes:        Refer to orpgda man page for more details on 
//                ORPGDA_read.
//
/////////////////////////////////////////////////////////////////\*/      
void RPG_data_access_read( fint *data_id, void *buf, fint *buflen,
                           fint *msg_id, fint *status ){

   *status = ORPGDA_read( *data_id, (char *) buf, *buflen, 
                          (LB_id_t) *msg_id );

   /*
     If status is negative, report error. 
   */
   if( *status  < 0 )
      LE_send_msg( GL_INFO, "ORPGDA_read Return Bad Status: Ret = %d\n", 
                   *status );
}

/*\///////////////////////////////////////////////////////////////
//
//  Description:  This module provides a FORTRAN interface to
//                the ORPGDA_write function.
//
//  Input:        data_id - data id to be associated with group
//                msg - pointer to buffer where data to to be 
//                      written resides
//                length - size, in bytes, of data to be written
//                msg_id - message id associated with the data
//
//  Return:       There are no return values defined.  The return
//                value from ORPGDA_write is saved in the status
//                argument.
//
//  Notes:        Refer to orpgda man page for more details on 
//                ORPGDA_write.
//
/////////////////////////////////////////////////////////////////\*/      
void RPG_data_access_write( fint *data_id, void *msg, fint *length,
                            fint *msg_id, fint *status ){

   *status = ORPGDA_write( *data_id, (char *) msg, *length, 
                           (LB_id_t) *msg_id );

   /*
     If status is negative, report error. 
   */
   if( *status < 0 )
      LE_send_msg( GL_INFO, "ORPGDA_write Return Bad Status: Ret = %d\n", 
                   *status );

}
