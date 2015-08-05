
/*******************************************************************

    Module: rms.h

    Description: Public header file for the Record Management 
		Service (RSIS) module.

*******************************************************************/

/*
 * RCS info 
 * $Author: jing $
 * $Locker:  $
 * $Date: 2005/09/14 14:37:46 $
 * $Id: misc_rsis.h,v 1.11 2005/09/14 14:37:46 jing Exp $
 * $Revision: 1.11 $
 * $State: Exp $
 * $Log: misc_rsis.h,v $
 * Revision 1.11  2005/09/14 14:37:46  jing
 * Update
 *
 * Revision 1.10  2000/08/21 20:42:40  jing
 * @
 *
 * Revision 1.9  2000/03/13 15:22:53  jing
 * @
 *
 * Revision 1.8  1999/02/16 17:02:06  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.7  1999/01/12 19:56:45  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.4  1999/01/11 16:58:23  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.2  1999/01/08 22:13:58  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.1  1999/01/08 19:11:38  jing
 * Initial revision
 *
 * Revision 1.1  1999/01/08 19:04:20  jing
 * Initial revision
 *
*/

#ifndef MISC_RSIS_H
#define MISC_RSIS_H

enum {RSIS_LESS, RSIS_EQUAL, RSIS_GREATER};
enum {RSIS_LEFT, RSIS_RIGHT};

#define RSIS_NOT_FOUND   -100
#define RSIS_TOO_MANY_RECORDS   -101
#define RSIS_INVALID_INDEX -102
#define RSIS_INVALID_ARGS -103

#ifdef __cplusplus
extern "C"
{
#endif

int RSIS_size (int maxn, int n_keys, int rec_size);
char *RSIS_init (int maxn, int n_keys, int rec_size, 
		char *buf, char *lbuf, int (*compare)(int, void *, void *));
int RSIS_insert (char *rsid, void *new_rec);
int RSIS_delete (char *rsid, int ind);
int RSIS_find (char *rsid, int which_key, void *key, void *record);
int RSIS_traverse (char *rsid, int which_key, int dir, int ind, void *record);
int RSIS_left (char *rsid, void *record);
int RSIS_right (char *rsid, void *record);
char *RSIS_localize (char *buf, char *lbuf, 
			int (*compare)(int, void *, void *));
int RSIS_is_corrupted (char *rsid);
int RSIS_get_next_ind (char *rsid, int ind);
char *RSIS_get_record_address (char *rsid);
int RSIS_local_buf_size (int n_keys);


#ifdef __cplusplus
}
#endif

#endif			/* #ifndef MISC_RSIS_H */
