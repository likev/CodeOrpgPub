
/******************************************************************

    vdeal's module containing functions for global processing.

******************************************************************/

/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2009/03/03 23:12:21 $
 * $Id: vdeal_glob.c,v 1.1 2009/03/03 23:12:21 steves Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <infr.h>
#include "vdeal.h"

/********************************************************************

    Partition the image.

*********************************************************************/

int VDG_partition (Vdeal_t *vdv) {
    int y, w, end, cnt;

    w = 60;
    cnt = 0;
    y = vdv->n_parts * w;
    if (vdv->data_type == DT_REALTIME)
	end = vdv->rt_read;
    else
	end = vdv->yz;
    while (y < end) {
	Part_t p;

	p.st_azi = y;
	p.end_azi = y + w - 1;
	if (p.end_azi >= end) {
	    if (vdv->data_type == DT_REALTIME && 
				vdv->radial_status != RS_END_ELE)
		break;
	    p.end_azi = end - 1;
	}
	p.st_range = 0;
	p.end_range = vdv->xz - 1;

	y += w;
	vdv->parts = (Part_t *)STR_append ((char *)vdv->parts, (char *)&p, 
						sizeof (Part_t));
	cnt++;
    }
    vdv->n_parts += cnt;
/*
{
int i;
for (i = 0; i < vdv->n_parts; i++)
    printf ("part %d: st_a %d, end_a %d, st_r %d, end_r %d\n", i, vdv->parts[i].st_azi, vdv->parts[i].end_azi, vdv->parts[i].st_range, vdv->parts[i].end_range);
}
*/
    return (0);
}

