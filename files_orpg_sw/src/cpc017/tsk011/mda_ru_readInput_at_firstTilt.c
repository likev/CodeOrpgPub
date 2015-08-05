/* 
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2010/12/03 20:47:40 $
 * $Id: mda_ru_readInput_at_firstTilt.c,v 1.3 2010/12/03 20:47:40 ccalvert Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

/******************************************************************************
 *	Module:         mda_ru_readInput_at_firstTilt.c			      *
 *	Author:		Yukuan Song					      *
 *   	Created:	Oct. 1, 2003					      *
 *	References:	WDSS MDA Fortran Source code			      *	
 *			AEL						      *
 *									      *
 *      Description:    This program is to read 2D features at the first tilt *
 *      Input:          char* inbuf            			              *
 *      output:         int   num_sf                                          *
 *                      int   feature_2d[num_sf]                              *
 *									      *
 *      Notes:       							      *
 ******************************************************************************/

/*	System include files	*/
#include <stdio.h>
#include <string.h>
#include <mdattnn_params.h>
#include "orpg.h"

/*	local include files	*/
#include "mda_ru.h"

/* declare global parameters */
#define DEBUG	0


void readInput_at_firstTilt(int* num_sf, int* overflow_flg, int elev_time[],
                            feature2d_t feature_2d[], char* inbuf) {

	/* declare the local varaibles */
        feature2d_t one_2d;
        int         one_time;
        int         i;
	char*       inbuf_ptr;

        inbuf_ptr = inbuf;
	
        /* read 2D features from input linear buffer */

	/* read the number of 2D features */
        memcpy(num_sf, inbuf_ptr, sizeof(int));
        inbuf_ptr += sizeof(int);

        /* The next item in the buffer is the overflow flag */
        memcpy(overflow_flg, inbuf_ptr, sizeof(int));
        inbuf_ptr += sizeof(int);        

        /* The next item in the buffer is the array of elevation times. */
	for(i= 0; i< MESO_NUM_ELEVS; i++) {
	 memcpy(&one_time, inbuf_ptr, sizeof(int));
         inbuf_ptr += sizeof(int);
         elev_time[i] = one_time;
        }

        if(DEBUG)
	 fprintf(stderr, "num_sf at the first tilt=%d\n", *num_sf);

	for(i= 0; i< *num_sf; i++) {
	 memcpy(&one_2d, inbuf_ptr, sizeof(feature2d_t)); 
         inbuf_ptr += sizeof(feature2d_t);
         feature_2d[i] = one_2d;
         if(DEBUG)
	  fprintf(stderr, "ca=%f, dia=%f, max_conv=%f\n", 
		feature_2d[i].ca, feature_2d[i].dia, feature_2d[i].max_conv);         
	}

} /* END of the function                                               */
