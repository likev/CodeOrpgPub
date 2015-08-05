/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2007/02/23 21:25:26 $
 * $Id: vcp_setup.c,v 1.6 2007/02/23 21:25:26 steves Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */

/*  Program to use ORPG adaptation data to get information about MPDA VCPs


R. May      3/03      Modified to use more ORPG structs
R. May      6/02      Changed to use adaptation data from ORPG
W. Zittel   1/02      Original Development

*/

#include <memory.h>
#include <orpg.h>
#include <rdacnt.h>
#include <vcp.h>
#include "mpda_constants.h"
#include "mpda_vcp_info.h"
#include <veldeal.h>

#define ERROR -1
#define ALL_OK 0

int vcp_setup( int cuts_elv[], int cmode[], int lastflg[], int cur_vcp ){

    int i, num_vel, num_cuts, el_num;
    Vcp_struct *vcp_table=NULL;
    Ele_attr *elev_attr;

    /* Copy table of unambiguous ranges from adaptation data to array */
    memcpy( range_table, &RDA_cntl.unambigr, sizeof(RDA_cntl.unambigr) );

    /* Point the vcp_table to the correct VCP in the adaptation data */
    for( i = 0; i < VCPMAX; i++ ){

      vcp_table = (Vcp_struct *) RDA_cntl.rdcvcpta[i];
      if (vcp_table->vcp_num == cur_vcp)
          break;
    }

    /* Check to see that we found a VCP in the adaptation data.  If not, yell
       about it and return an error value (-1) */
    if(i == VCPMAX || cur_vcp == 0){

        LE_send_msg(GL_ERROR,"MPDA: vcp_setup -- Invalid VCP: %d\n", cur_vcp);
        return ERROR;
    }

    /* Set the number of sweeps in the VCP and initialize some variables */
    num_cuts = 1;
    el_num = 0;
    num_vel = 0;

    /* Loop to get data for all sweeps of the VCP */
    for( i = 0; i < vcp_table->n_ele; i++ ){

        elev_attr = (Ele_attr *) vcp_table->vcp_ele[i];

        /* Get the mode of the current sweep */
        vcp.cut_mode[i] = elev_attr->wave_type;
        cmode[i] = vcp.cut_mode[i];

        /* If the current cut's mode is surveillance (reflectivity only), 
           store the PRF number for the reflectivity pulse, otherwise, 
           store the PRF number for the velocity pulse. */
        if (vcp.cut_mode[i] == VCP_WAVEFORM_CS)
            vcp.prf_num[i] = elev_attr->surv_prf_num;
        else{

            vcp.prf_num[i] = elev_attr->dop_prf_num_1;

            /* Since this cut contains velocity data, increment the velocity
               cut counter (for the current elevation) and set the velocity 
               number for this cut to that value */
            vcp.vel_num[i] = ++num_vel;

        }

        /* Check if this is the last allowable cut -- this ensures that we 
           can look ahead to the next cut. */
        if( i < (VCP_MAXN_CUTS - 1)){

            /* Check if the elevation angle for the next cut is different 
               from the elevation angle for the current cut */
            if(elev_attr->ele_angle !=
                     ((Ele_attr *)(vcp_table->vcp_ele[i+1]))->ele_angle){

                /* If the are different, set the last scan flag to true, 
                   save the number of cuts for the current elevation number, 
                   and then reset the counters */
                vcp.last_scan_flag[i] = TRUE;
                lastflg[i] = TRUE;
                vcp.cuts_per_elv[el_num] = num_cuts;
                cuts_elv[el_num] = num_cuts;
                el_num++;
                num_cuts = 1;
                num_vel = 0;

            }
            else{

                /* Otherwise, set the last scan flag to false and increment 
                   the number of cuts for the current elevation number */
                vcp.last_scan_flag[i] = FALSE;
                lastflg[i] = FALSE;
                num_cuts++;

            }

        }
        else{

            /* This is the last allowable cut, so this MUST be the last scan
               -- set the last scan flag to true and save the number of cuts 
               we have for this elevation */
            vcp.last_scan_flag[i] = TRUE;
            lastflg[i] = TRUE;
            vcp.cuts_per_elv[el_num] = num_cuts;
            cuts_elv[el_num] = num_cuts;

        }

    }

    return ALL_OK;

}

