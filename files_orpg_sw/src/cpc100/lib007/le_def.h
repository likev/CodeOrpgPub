
/*******************************************************************

    Module: le_def.h

    Description: The private header file for the LE module.

*******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2012/06/14 18:57:55 $
 * $Id: le_def.h,v 1.11 2012/06/14 18:57:55 jing Exp $
 * $Revision: 1.11 $ 
 * $State: Exp $
 */  

#ifndef LE_DEF_H
#define LE_DEF_H

#define DEFAULT_LB_SIZE 300
#define DEFAULT_LB_TYPE 0

#define UNDEFINED_LB_SIZE -1
#define UNDEFINED_LB_TYPE -1

/* private functions */
void LE_save_message (char *msg);
char *LE_get_saved_msg ();
int LE_get_instance ();
char *LE_get_label ();
int LE_check_and_open_lb (char *arg0, int *use_plain_filep);
int LE_set_disable (int yes);
void LE_set_print_src_mask (unsigned int mask);
void LE_set_output_fd (void *fl);
FILE *LE_get_plain_file_fh ();
void LE_file_lock (int lock);
char *LE_get_le_name ();


#endif 		/* #ifndef LE_DEF_H */
