/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2004/02/05 23:04:35 $
 * $Id: mda3d_main.h,v 1.7 2004/02/05 23:04:35 ccalvert Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */

/************************************************************************
Module:         mda3d_main.h

Description:    include file for the task mda3d algorithm
************************************************************************/

#ifndef MDA3D_MAIN_H 
#define MDA3D_MAIN_H 

/* system includes ---------------------------------------------------- */
#include <stdio.h>

/* ORPG   includes ---------------------------------------------------- */
#include <rpg_globals.h>
#include <rpgc.h>
#include <basedata.h>
#include <siteadp.h>
#include <alg_adapt.h>

#include "mda3d_parameter.h"

/* Algorithm specifid constants/definitions --------------------------- */

#define BUFSIZE 300000     /* estimated output size for the ICD product */
                            /* based on 6 degree azimuth and 5 km range */
                            /* and 20 elevations per volume, the maximum*/
                            /* range is 120km   			*/

/* declare the modules */
void mda3d_acl();
void mda3d_set_table();

Siteadp_adpt_t site_adapt;

#endif


