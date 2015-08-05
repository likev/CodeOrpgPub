/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2004/01/07 17:57:42 $
 * $Id: readSCITInput.c,v 1.3 2004/01/07 17:57:42 ccalvert Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

/******************************************************************************
 *	Module:         readSCITInput.c		                      *
 *	Author:		Brian Klein					      *
 *   	Created:	Feb. 19, 2003					      *
 *	References:	WDSS MDA Fortran Source code (mdatrck.f dated 121602) *
 *			AEL						      *
 *									      *
 *      Description:    This file reads the SCIT tracking and forecast        *
 *                      algorithm output.                                     *
 *									      *
 *      Notes:       	Speed received from SCIT is in meters per second      *
 ******************************************************************************/
#include <stdio.h>
#include <math.h>

#include "a309.h"
#include "rpgc.h"
#include "trfrcatr.h"
#include "mdattnn_params.h"

#define FLOAT
#include "rpgcs_coordinates.h"

extern tracking_t scit_tracks;

#define DEBUG 0

void readSCITInput(float* mda_def_u, float* mda_def_v)
{
        int        opstatus;
        char*      inbuf;
        float      avg_SCIT_spd, right_mover_spd;
        float      avg_SCIT_dir, right_mover_dir;

/*      Initialize the outputs to indicate an undefined speed and direction. */

        *mda_def_u = 0.0;
        *mda_def_v = 0.0;
        scit_tracks.hdr.bnt = 0;

/*      Read the data.  This data is declared optional in main.               */

        inbuf = (char*)RPGC_get_inbuf(TRFRCATR, &opstatus);
       
        if (opstatus != NORMAL) return;   /* go on without this data. */

        if (DEBUG) fprintf(stderr,"Obtained SCIT input buffer...\n");

/*      Copy the TRFRCALG data into the structure designed to hold it.        */

        memcpy(&scit_tracks, inbuf, sizeof(scit_tracks));

/*      Get the only two items we need, average speed and direction.          */

        avg_SCIT_spd = scit_tracks.hdr.bvs;
        avg_SCIT_dir = scit_tracks.hdr.bvd;

        if (DEBUG) fprintf(stderr,"SCIT spd/dir = %f/%f\n",
                                  avg_SCIT_spd,
                                  avg_SCIT_dir);


/*      Release the input buffer now that we have what we need.               */

        RPGC_rel_inbuf((void*)inbuf);
        inbuf = NULL;

/*      Compute the default motion of mesocyclones based on "right-moving"    */
/*      logic from the AEL.                                                   */

        if (avg_SCIT_spd < 15.0) {
           right_mover_spd = avg_SCIT_spd * 0.75;
           right_mover_dir = avg_SCIT_dir + 30.0;
        }
        else {
           right_mover_spd = avg_SCIT_spd * 0.80;
           right_mover_dir = avg_SCIT_dir + 20.0;
        }

/*      Convert to U and V components.                                        */

        *mda_def_u = (-right_mover_spd) * sin(right_mover_dir * DTR);
        *mda_def_v = (-right_mover_spd) * cos(right_mover_dir * DTR);

        return;

} /* end of function readSCITInput() */

