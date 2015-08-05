/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2004/01/07 17:57:44 $
 * $Id: readTdaInput.c,v 1.3 2004/01/07 17:57:44 ccalvert Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

/******************************************************************************
 *	Module:         readTdaInput.c		                      *
 *	Author:		Brian Klein					      *
 *   	Created:	Feb. 19, 2003					      *
 *	References:	WDSS MDA Fortran Source code (mdatrck.f dated 121602) *
 *			AEL						      *
 *									      *
 *      Description:    This file contains logic taken from the               *
 *                      NSSL MDA Tracking algorithm.                          *
 *									      *
 *      Notes:       	                                                      *
 ******************************************************************************/
#include <stdio.h>

#include "a309.h"
#include "rpgc.h"
#include "tvsattr.h"
#include "mdattnn_params.h"

#define DEBUG 0
#define TVSATTR_RU 290

int readTdaInput(polar_loc_t* tvs)
{
        int        i, num_features, num_tvs, opstatus, nt;
        char*      inbuf;
        tvsattr_t  in_tvs;         /* Input data from tda2d3d process.       */


/*      Read the data.  This data is declared optional in main.              */

        inbuf = (char*)RPGC_get_inbuf(TVSATTR_RU, &opstatus);
       
        if (opstatus != NORMAL) return (int)0;   /* go on without this data. */

        if (DEBUG) fprintf(stderr,"Obtained TDA input buffer...\n");

/*      Copy the TDAATTR data into the structure designed to hold it.        */

        memcpy(&in_tvs, inbuf, sizeof(in_tvs));

/*      Get the total number of features and just the TVSes.                 */
/*      The legacy code sets these values to negative if the maximum number  */
/*      if TVA/ETVS is exceeded.  It is a flag to indicate that there may    */
/*      be more of these features but processing stopped at the max.         */

        num_features = abs(in_tvs.num_tvs) + abs(in_tvs.num_etvs);
        num_tvs      = abs(in_tvs.num_tvs);

        if (DEBUG) fprintf(stderr,"Total TVS/ETVS features= %d\n",num_features);

        if (num_features > TVFEAT_MAX) {
            RPGC_log_msg(GL_ERROR,
              "mdatrack: Too many TVS/ETVS features! %d\n", num_features);
            RPGC_rel_inbuf((void*)inbuf);
            inbuf = NULL;
            return (int)0;
        }

        nt = 0;
        for (i = 0; i < num_features; i++) {

/*          Copy the TVS and ETVS locations (in polar format)           */
/*          Per the ROC and the AEL, ignore ETVSes.                     */

            if (in_tvs.tvsattr[i].type == ETVS_T) continue;
            tvs[nt].azimuth = in_tvs.tvsattr[i].azimuth;
            tvs[nt].range   = in_tvs.tvsattr[i].range;
            nt++;
        } /* end for */

        if (DEBUG) fprintf(stderr,"Completed reading TVS input buffer...\n");

        if (DEBUG) {

           fprintf(stderr,"v_time=%d, v_date=%d, num_tvs=%d, num_etvs=%d nt=%d\n",
                            in_tvs.v_time,  in_tvs.v_date,
                            in_tvs.num_tvs, in_tvs.num_etvs, nt);
           for (i = 0; i < num_features; i++) {

             fprintf(stderr, "    i=%d, az=%f, rg=%f\n\n",i, tvs[i].azimuth,
                                                             tvs[i].range);
           }
        }

/*      Release the input buffer now that we've copied it.                 */

        RPGC_rel_inbuf((void*)inbuf);
        inbuf = NULL;

        return num_tvs;

} /* end of function readTdaInput() */

