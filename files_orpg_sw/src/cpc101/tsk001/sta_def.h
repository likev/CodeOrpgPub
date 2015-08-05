
/******************************************************************

    Private include file for sync_to_adapt.
 
******************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2006/03/28 22:59:38 $
 * $Id: sta_def.h,v 1.1 2006/03/28 22:59:38 steves Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>

#include <infr.h>


typedef struct {
    char *de_id;		/* DE id for the threshold value array */
    int ind;			/* index in the legacy 2-D table (1, 2, ...) */
    LB_id_t msg_id;		/* LB message ID for the table values */
    char *init_thr;		/* The default threshold table. This is for
				   test purpose. This format needs to be 
				   converted to the format used for 
				   DEAU_get_values output. These are read from
				   the legacy-initialized tables. */
    int updated;		/* this DE has updated */
} Color_table_t;

typedef struct {
	/**  First value in the color table */
	int first;
	/**  Threshold code values */
	unsigned short code[34][16];
	/**  Color Tables */
	unsigned short colors[34][257];

	/**  Last field */
	int last;
} color_table_t;

int STA_update_color_table (int ind, char *thr, int n_thrs,
				unsigned short *ct, unsigned short *tt);


