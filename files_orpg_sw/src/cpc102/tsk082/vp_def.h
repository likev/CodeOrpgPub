
/***********************************************************************

    Description: Internal include file for display_status_prod and 
                 standalone_dsp.

***********************************************************************/

/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/07/22 19:05:15 $
 * $Id: vp_def.h,v 1.1 2014/07/22 19:05:15 steves Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */  

#ifndef DSP_DEF_H

#define DSP_DEF_H

#include <time.h>

#define LOCAL_NAME_SIZE 200		/* maximum name size */
#define STR_SIZE 128			/* max size for input strings */

typedef struct {			/* volume file */

    char *path;				/* full path of the file */
    char *name;				/* pointer to the file name */
    int size;				/* file size */
    int datetime[5];			/* Year, month, day, hr, min 
                                           file date/time. */

} Ap_vol_file_t;


#define BAD_FILE_HEADER			-2

int DSPAUX_search_files (char *d_name, Ap_vol_file_t **vol_files);

#endif		/* #ifndef DSP_DEF_H */
