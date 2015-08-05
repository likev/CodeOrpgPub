/*
 * RCS info
 * $Author: steves $
 * $Date: 2014/05/13 19:57:54 $
 * $Locker:  $
 * $Id: correlate_mda_features.c,v 1.5 2014/05/13 19:57:54 steves Exp $
 * $revision$
 * $state$
 * $Logs$
 */

/*1234567891234567891234567891234567891234567891234567891234567891234567891234*/
/******************************************************************************
 *      Module:         correlate_mda_features.c                              *
 *      Author:         Brian Klein                                           *
 *      Created:        November 4, 2005                                      *
 *                                                                            *
 *      Description:    This module reads the elevation-based output of the   * 
 *                      MDA tracking process (mdattnn) and uses it to         *
 *                      determine if an MDA feature is near a SCIT detected   *
 *                      storm cell.             			      *
 *                                                                            *
 *      Input:          pointer to SCIT storm track structure                 *
 *                      pointer to MDA tracking output buffer (lb 295)        *
 *      Output:         Storm_feats and Basechars arrays updated with         *
 *                      information about the closest MDA feature             *
 *                                                                            *
 *      notes:          Roughly modeled after the legacy Mesocyclone          *
 *                      Algorithm correlation routine A30833.FTN.             *
 *                      This function also performs the work of A30837.FTN    *
 *                      for MDA data only.                                    *
 ******************************************************************************/

/* Global and local include files ------------------------------------------- */
#include <combattr.h>
#include <mdattnn_params.h>

#define LARGEDIST  999999.0
#define debugit    0

int correlate_mda_features( tracking_t *ptrTr, int *ptrMdattnn,
                            float cat_mdat[][CAT_NMDA], int *cat_num_rcm ){

    cplt_t  *ptrCplt  = NULL; /* pointer to struct output by mdattnn */
    int   num_cplts;
    int   num_storms;
    int   allow_suppl_scans;
    int   m, s, i, temp, vcpnum;
    float distance_x, distance_y, distance;
    float mda_x, mda_y, min;
    float storm_x, storm_y;

    /* Get the number of SCIT storm cells detected */
     
    num_storms = ptrTr->hdr.bnt;
 
    if (0 & debugit) {
       
       fprintf(stderr,"num_storms = %d\n",num_storms);
       for (i = 0; i < num_storms; i++)
       {
         fprintf(stderr,"ptrTr->bsm[%d].x0=%f\n",i,ptrTr->bsm[i].x0);
         fprintf(stderr,"ptrTr->bsm[%d].y0=%f\n\n",i,ptrTr->bsm[i].y0);
       }
    }
    
    /* Get the number of MDA features (couplets).  Its the first 4 bytes.  */
       
    num_cplts = ((mda_ttnn_hdr_t*)ptrMdattnn)->num_cplts;
    if (debugit) fprintf(stderr, "\n\ncombattr, num_cplts=%d\n",num_cplts);

    /* Anything to correlate? */   
    if (num_cplts == 0 || num_storms == 0){

        cat_num_rcm[RCM_MDA] = 0;
        return 0;

    }
       
    /* Skip over the number of couplets, average U and V, and the */
    /* elevation time integers.                                   */
       
    ptrCplt   = (cplt_t*)(ptrMdattnn+3+MESO_NUM_ELEVS);

    /* Loop for all features in the input MDA data.                     */

    for (m = 0; m < num_cplts; m++) {
       if (0 & debugit) fprintf(stderr,"ptrCplt[%d].strength_rank=%d\n",m,ptrCplt[m].strength_rank);

      /* Get the location of this MDA feature in cartesian coordinates */
      
      mda_x = ptrCplt[m].llx;
      mda_y = ptrCplt[m].lly;
      
      /* Correlate this MDA feature to the nearest storm. */
      
      min  = LARGEDIST;
      temp = -1;
      
      for (s = 0; s < num_storms; s++) {

         /* Get the location of this storm cell in cartesian coordinates */
         
         storm_x = ptrTr->bsm[s].x0;
         storm_y = ptrTr->bsm[s].y0;
         
         distance_x = storm_x - mda_x;
         distance_y = storm_y - mda_y;
         
         /* Don't need to take square root, results will be the same */
         
         distance   = (distance_x * distance_x) + (distance_y * distance_y);
         if (0 & debugit) fprintf(stderr,"m=%d s=%d dist(km**2)=%f\n",m,s,distance);
         
         if (distance < min && distance < 400.0) {
            min = distance;
            temp = s;
         }
      } /* end for each SCIT storm cell */
      
      /* If the storm is not yet associated to an MDA feature associate it */
      /* Because the MDA buffer is sorted by strength rank, the stronger   */
      /* feature will be associated first.                                 */

      if (min == LARGEDIST || Storm_feats[temp][CAT_MDA_TYPE] != 0) continue;
      if (0 & debugit) fprintf(stderr,"  Storm_feats[%d][%d]=%d\n",
                                 temp,CAT_MDA_TYPE,Storm_feats[temp][CAT_MDA_TYPE]);
      
      Storm_feats[temp][CAT_MDA_TYPE] = ptrCplt[m].strength_rank;
      
      /* Store the attributes of the MDA feature base.             */
      /* We need to convert the elevation index to an angle        */
      /* Note that RPGCS_get_target_elev_ang assumes the elevation */
      /* index starts at 1 so we have to increment it.             */
      
      vcpnum = RPGC_get_buffer_vcp_num((void*)ptrMdattnn);
      if( (allow_suppl_scans = RPGC_allow_suppl_scans()) == 0 )
         vcpnum = -vcpnum;

      Basechars[temp][MDAB_AZ] = ptrCplt[m].ll_azm;
      Basechars[temp][MDAB_RN] = ptrCplt[m].ll_rng;
      Basechars[temp][MDAB_EL] = 
         RPGCS_get_target_elev_ang(vcpnum,(ptrCplt[m].mda_th_xs[0].tilt_num+1));
      Basechars[temp][MDA_SR]  = ptrCplt[m].strength_rank;
      
      if (debugit) {
         fprintf(stderr,"Associating MDA feature %d to storm %d, dist(km**2)=%f\n",
                                           m,temp,min);
         fprintf(stderr,"   rank=%f ll_azm=%f, ll_rng=%f, el=%f, tilt=%d\n",
                              Basechars[temp][MDA_SR],
                              Basechars[temp][MDAB_AZ],
                              Basechars[temp][MDAB_RN],
                              Basechars[temp][MDAB_EL],
                              ptrCplt[m].mda_th_xs[0].tilt_num);
      }
   } /* end for each MDA feature */

   /* Now do the work of a30837.c for the MDA portion of the    */
   /* combined attributes table.                                */
   
   /* The cat_mdat array maps to the legacy fortran array so reverse */
   /* the indicies.                                                  */
    
   for (m = 0; m < CAT_NMDA; m++)
     for (s = 0; s < CAT_MXSTMS; s++)
        cat_mdat[s][m] = 0;

   /* Loop through the number of MDA features: Store the azimuth,  */
   /* range, elevation angle and strength rank.                    */
   /* Note that RPGCS_get_target_elev_ang assumes the elevation    */
   /* index starts at 1 so we have to increment it.                */
   
   vcpnum = RPGC_get_buffer_vcp_num((void*)ptrMdattnn);
   if( (allow_suppl_scans = RPGC_allow_suppl_scans()) == 0 )
      vcpnum = -vcpnum;

   for (m = 0; m < num_cplts; m++) {
      cat_mdat[m][CAT_MDAAZ] = ptrCplt[m].ll_azm;
      cat_mdat[m][CAT_MDARN] = ptrCplt[m].ll_rng;
      cat_mdat[m][CAT_MDAEL] = 
         RPGCS_get_target_elev_ang(vcpnum,(ptrCplt[m].mda_th_xs[0].tilt_num+1));
      cat_mdat[m][CAT_MDASR] = ptrCplt[m].strength_rank;
   }
 
   /* Set the number of MDA features for the RCM product. */
   
   cat_num_rcm[RCM_MDA] = num_cplts;
  
   return 0;      
}
