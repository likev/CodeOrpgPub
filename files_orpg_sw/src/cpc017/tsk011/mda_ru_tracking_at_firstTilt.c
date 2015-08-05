/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2007/06/01 21:09:48 $
 * $Id: mda_ru_tracking_at_firstTilt.c,v 1.11 2007/06/01 21:09:48 steves Exp $
 * $Revision: 1.11 $
 * $State: Exp $
 */
 
/*123456789012345678901234567890123456789012345678901234567890123456789012345*/
/******************************************************************************
 *	Module:         mda_ru_tracking_at_firstTilt.c					      *
 *	Author:		Yukuan Song					      *
 *   	Created:	Oct. 1, 2003					      *
 *	References:	WDSS MDA Fortran Source code			      *	
 *			AEL						      *
 *									      *
 *      Description:    This program is to associate the 2D features from     *
 *			the first tilt of the current volume with the 3D      *
 *			features from the previous volume		      *
 *      Input:          char* inbuf            			              *
 *      output:         int   num_sf                                          *
 *                      int   feature_2d[num_sf]                              *
 *									      *
 *      Notes:		This is a modification version of mdattnn_acl()       *
 ******************************************************************************/

/*	System include files	*/
#define DEBUG   0                        /* Controls debug output to log file */
#define STDERR_DB 0                      /* Controls debug to stderr */
#define MIN(x,y) ( (x) < (y) ? (x) : (y) ) /* returns the lesser of x and y */
#define THOUSAND 1000                        /* Controls debug output in this file */

/*      System include files    */
#include <math.h>
#include <stdio.h>
#include <assert.h>

/*      Global include files    */
#include "a309.h"
#include "rpgc.h"
#include "rpgcs.h"
#define FLOAT
#include "rpgcs_coordinates.h"

#include "mda_ru.h"
#include "mda_ru_extrapolate.h"
#include "mdattnn_params.h"

/* define value */
#define fcst_time	300.0 /* time between forecasts (sec)                 */

/*      Externals               */
extern  int    Nbr_old_cplts;
extern  int    Nbr_cplt_trks;
extern  int    Old_time;
extern  int    Old_date;
extern  cplt_t Old_cplt[];

	/*      Prototypes              */
 	void mdattnn_out(FILE* fileptr, cplt_t* cplt, const int n_cplts);
	void mda4d_out(FILE* fileptr_fort34, cplt_t* cplt, const int n_cplts);

        void mda_ru_sort(float n2o_dist[MAX_MDA_FEAT][MAX_MDA_FEAT],
                         int n2o_order[MAX_MDA_FEAT][MAX_MDA_FEAT], int num_features);
        void mda_ru_extrapolate(int* iadd, int addnew[],
                        int n2o_order[MAX_MDA_FEAT][MAX_MDA_FEAT],
			float elevation, int nbr_new_cplts, int first_tilt);
	void getAverageMotion(const cplt_t*  Old_cplt,
                             const int      Nbr_Old_cplts,
                             const float*   avg_mu,
                             const float*   avg_mv);
	int  getTimeDiff(const int new_time,
                        const int new_date,
                        const int old_time,
                        const int old_date);
        void mda_sort(const int flag, cplt_t cplt[MAX_MDA_FEAT], const int n_cplts);

void tracking_at_firstTilt(int num_sf, feature2d_t* feature_2d,
				int new_time, int new_date, float elevation,
				float mda_def_u, float mda_def_v,
				FILE* output_ttnn, FILE* output_fort34,
				int* nbr_first_elev_newcplt,
				cplt_t first_elev_newcplt[],
				int    elev_time[]) {

	/* declare the local varaibles */
 	float avg_mu, avg_mv; /* avg. motion of detections in last vol.(m/sec)*/
	float old_u, old_v;   /* local copies of u & v motion from last vol.  */
	short i, j, k, l;     /* loop indices                                 */
	short back_check;
	int iadd;                               /* num of the coasted   */
	short duplicate_association;
	int   nbr_new_cplts;  /* # 3Ds found in current volume, from mda3d    */
	int   time_diff;      /* time between this volume and the last (sec)  */
	float old_top;
	float x, y; /* position of feature */
	float diff_x, diff_y; /* moving distance of feature */
	float ru_u, ru_v; 
	float prop_spd, prop_dir; /* Propagation speed, direction */
	float spd, dist;
	float u, v;
	float diff_z;
	float base, azm, rng, dia;
	char* outbuf;         /* points to beginning of output buffer         */
        char* outbuf_ptr;     /* points to current position within output buf.*/
        int   outbuf_size;     /* size of data in output buffer, in bytes      */
        int   opstatus;       /* status of infrastructure calls               */
	float t_a_th;       /* time association distance threshold (km)     */
        float max_dist_thr; /* maximum time association distance (km)       */
	int first_tilt;

        static int addnew[MAX_MDA_FEAT];               /* index array          */
	static int   n2o_order[MAX_MDA_FEAT][MAX_MDA_FEAT];/* new to old order   */
        static float n2o_dist[MAX_MDA_FEAT][MAX_MDA_FEAT]; /* new to old distance*/
	static cplt_t new_cplt[MAX_MDA_FEAT]; /* 3D features this volume           */

	/* Initialize some variables */
	nbr_new_cplts = 0;

/*C***********************************************************************/
/*C  Compute new average motion vector **/
/*C***********************************************************************/

        getAverageMotion(Old_cplt, Nbr_old_cplts, &avg_mu, &avg_mv);

        if (STDERR_DB) fprintf(stderr,"avg_mu = %f, avg_mv = %f\n", avg_mu, avg_mv);

/*C*********************************************************************/
/*C  Compute time difference between current and previous volume scan **/
/*C*********************************************************************/

        if (DEBUG) {
          /*C*********************************************/
          /*C  Write output headers to both debug files **/
          /*C*********************************************/

          fprintf(output_fort34, "\n=================================================================\n");
          fprintf(output_fort34, " NEW DATE/TIME: %6.6d %6.6d\n",
                                   new_date, new_time);
        }
        if (DEBUG) {
          fprintf(output_ttnn, "\n=================================================================\n");
          fprintf(output_ttnn, " NEW DATE/TIME: %6.6d %6.6d\n",
                                  new_date, new_time);
        } /* end if DEBUG */
        
	time_diff = getTimeDiff(new_time, new_date, Old_time, Old_date);

/*C*********************************************/
/*C  If no detections from previous volume or current elevation, quit **/
/*C*********************************************/
	/* Make sure that the time difference is not too big */
                                                       
        if (time_diff == BAD_TIME || num_sf == 0 || Nbr_old_cplts == 0) {
        
          /* Either the time difference is too big or    */
          /* there are no features, old or new.          */
          
          Nbr_old_cplts = 0;
          iadd = 0;

        } else {

/*C***********************************************************************/
/*C  If there are detections from both the previous and current scans   **/
/*C  so now can time associate. Compute the distance for the time       **/
/*C   ssociation computation. MESO_MAX_SPD is the adaptable maximum     **/
/*C  speed threshold (km/min) for mesocyclone movement.                 **/
/*C***********************************************************************/
/*C**************************************************************************/
/*C  Loop through all the old detections.  Set propagation u and v to the  **/
/*C  average motion vector of the mesocyclone event.  If no average vector **/
/*C  has been computed, set u and v to the default vector values.          **/
/*C**************************************************************************/

          for (i = 0; i < Nbr_old_cplts; i++) {
 
            Old_cplt[i].time_code = NOT_ASSOCIATED;
            old_u = Old_cplt[i].u_motion;
            old_v = Old_cplt[i].v_motion;

            if (old_u == UNDEFINED) {
               if (avg_mu != UNDEFINED) {
                  old_u = avg_mu;
                  old_v = avg_mv;
               } else {
                  old_u = mda_def_u;
                  old_v = mda_def_v;
               } /* end if */
            } /* end if */

/*C******************************************************************/
/*C  Compute the "first guess" position of the old detection using **/
/*C  the propogation vector.                                       **/
/*C******************************************************************/

            Old_cplt[i].fgx = Old_cplt[i].llx + (old_u * time_diff / KILO);
            Old_cplt[i].fgy = Old_cplt[i].lly + (old_v * time_diff / KILO);
if(STDERR_DB)fprintf(stderr,"old_u[%d]=%f, v=%f\n",i,old_u,old_v);
if(STDERR_DB)fprintf(stderr,"Old_cplt[%d].fgx=%f, fgy=%f\n",i,Old_cplt[i].fgx, Old_cplt[i].fgy);
          } /* end for all old detections */

/*C****************************************/
/*C  Initialize feature selection arrays **/
/*C****************************************/

          for (i = 0; i < num_sf; i++) {
            for (j = 0; j < Nbr_old_cplts; j++) {
               n2o_dist[i][j]  = UNDEFINED;
               n2o_order[i][j] = NO_ORDER;
              } /*end for */
          } /* end for */

/*C******************************************************************/
/*C  Search for time association between new and old detections    **/
/*C  using an increasing distance threshold.  			   **/
/*C******************************************************************/

          for (i = 0; i < num_sf; i++) {
	    
            max_dist_thr = (float)time_diff * MESO_MAX_SPD / SECPERMIN;
	    
	    l = 0;

            t_a_th = 2.0; /* Initial search radius in km */
 

            do {

              for (j = 0; j < Nbr_old_cplts; j++) {

                if (Old_cplt[j].strength_rank == 0            ||
                    Old_cplt[j].circ_class    <= SHALLOW_CIRC ||
                    n2o_dist[i][j]            != UNDEFINED) continue;

/*              Perform a vertical continuity check.  Ensure the difference */
/*              in the features bases is within the threshold distance.     */

                if ((Old_cplt[j].strength_rank >= MESO_SR_THRESH) &&
                    (Old_cplt[j].strength_rank != UNDEFINED) &&
                    (Old_cplt[j].nssl_base     != UNDEFINED))
                  diff_z = (float)(feature_2d[i].ht -
                                   fabs(Old_cplt[j].nssl_base));

                else if (Old_cplt[j].core_base != UNDEFINED)
                  diff_z = (float)(feature_2d[i].ht -
                                   fabs((double)Old_cplt[j].core_base));
                else
                  diff_z = (float)(feature_2d[i].ht -
                                   fabs(Old_cplt[j].base));

/*C**************************************************************************/
/*C  Compute average motion vector (spd) and if greater than MESO_MAX_SPD, **/
/*C  do not use this association as a candidate.                           **/
/*C**************************************************************************/

                u = (feature_2d[i].cx - Old_cplt[j].llx) * KILO / (float)time_diff;
                v = (feature_2d[i].cy - Old_cplt[j].lly) * KILO / (float)time_diff;

                if (Old_cplt[j].u_motion != UNDEFINED) {
                  u = (u + Old_cplt[j].u_motion) * HALF;
                  v = (v + Old_cplt[j].v_motion) * HALF;
                }

                spd = sqrt(u*u + v*v);

                if (spd > MESO_MAX_SPD * KILO / SECPERMIN) continue;

/*C***************************************************************************/
/*C  Compute distance between new detection and old detection's first guess **/
/*C  position.                                                              **/
/*C***************************************************************************/

                dist = sqrt((feature_2d[i].cx - Old_cplt[j].fgx) *
                            (feature_2d[i].cx - Old_cplt[j].fgx) +
                            (feature_2d[i].cy - Old_cplt[j].fgy) *
                            (feature_2d[i].cy - Old_cplt[j].fgy) );

/*C**********************************************************************/
/*C  Time associate by finding the strongest old detection whose first **/
/*C  guess position is within the T_A_TH distance threshold of the     **/
/*C  current new detction.  This would be the first old detection      **/
/*C  encountered in the loop since they have been sorted by strength.  **/
/*C**********************************************************************/

                if (dist <= t_a_th && fabs(diff_z) <= THRESH_VERT_DIST) {
                  n2o_dist[i][j]  = dist;
                  n2o_order[i][l] = j;    /* Note these two lines are          */
                  l++;                 /* reversed in order intentionally.  */
                } /* end if distance within threshold */

              } /* end for all old detections */

              t_a_th += RADIUS_INCREMENT; /* Increment the search radius    */

/*            Because the threshold (t_a_th) is initialized as an integer   */
/*            and incremented with an integer but compared with a float     */
/*            (max_dist_thr), we need to account for the last iteration     */
/*            and set them equal or couplets within 1 km of the             */
/*            radius won't be found.                                        */
/*            To fix this, once our radius is beyond the max value but      */
/*            within the increment value, just set it to the max value.     */

            } while (t_a_th <= floor(max_dist_thr+0.5)); /* end do while */

if(STDERR_DB)fprintf(stderr,"n2o_order[%d][0]=%d, [%d][1]=%d, [%d][2]=%d\n"
                ,i,n2o_order[i][0],i,n2o_order[i][1],i,n2o_order[i][2]);
if(STDERR_DB)fprintf(stderr,"n2o_dist[%d][order0]=%f, [%d][order1]=%f, [%d][order2]=%f\n\n"
       ,i,n2o_dist[i][n2o_order[i][0]],i,n2o_dist[i][n2o_order[i][1]],i,n2o_dist[i][n2o_order[i][2]]);                
if(STDERR_DB)fprintf(stderr,"rank[%d]=%d\n",i,(int)(feature_2d[i].rank + 0.5));
          } /* end for all new detections */

          /* If there is more than one time association candidate, make sure that
           * they are listed within n2o_order from closest to furthest. */
/**   Removing sort.  It was not in NSSL version and seems to defeat the **/
/**   increasing search radius logic.                                    **/
/**          mda_ru_sort(n2o_dist, n2o_order, num_sf);                   **/

/*C****************************************************************************/
/*C  Check to see if any old detections are associated with more than one new**/
/*C  detection.  If so, take closest new detection, and make the other new   **/
/*C  detections use their second closest associated old detection.  Repeat   **/
/*C  process until all new detections are associated with best old detection.**/
/*C****************************************************************************/

          i = 0;

          while (i < num_sf) {

            if (n2o_order[i][0] == NO_ORDER) {
              i++; /* increment here because continue skips while increment*/
              continue; /* continue while loop */
            }

            back_check = NO;
            duplicate_association = NO;

            for ( j = 0; j < num_sf; j++) {

              if ( i == j || n2o_order[j][0] == NO_ORDER) continue;

              if (n2o_order[i][0] == n2o_order[j][0]) {

                duplicate_association = YES;
   if(STDERR_DB) fprintf(stderr,"duplicate! i=%d j=%d\n",i,j);
   if(STDERR_DB) fprintf(stderr,"  n2o_dist[i]=%f, [j]=%f\n", n2o_dist[i][n2o_order[i][0]],n2o_dist[j][n2o_order[j][0]]);

                if (n2o_dist[i][n2o_order[i][0]] <=
                    n2o_dist[j][n2o_order[j][0]]) {
                  k = j;
                  if ( i > j ) back_check = YES;
                } else {
                  k = i;
                } /*end if i distances is less than or equal to j distance */

                n2o_dist[k][n2o_order[k][0]] = UNDEFINED;

                for ( l = 0; l < MAX_MDA_FEAT - 1; l++)
                            n2o_order[k][l] = n2o_order[k][l+1];

                n2o_order[k][MAX_MDA_FEAT-1] = NO_ORDER;

                if (back_check) i = j;

                break; /* break from loop */

              } /* end if i order equals j order */

            } /* end for all new couplets */

            /* Emulate the GOTO 100 in NSSL code */

            if (!duplicate_association) i++; 

          } /* end while */

          /* Input elevated features into new_cplt array that should 
           * be coasted. Features are checked to make sure they are vertically
           * within MDA_TASSOC_DZ of the feature from the previous volume scan.*/
          
          first_tilt = 1;
	  mda_ru_extrapolate(&iadd, addnew, n2o_order, elevation, num_sf, first_tilt);

          if(STDERR_DB)
            fprintf(stderr, "iadd=%d, nbr_old_cplt=%d\n", iadd, Nbr_old_cplts);

          if (STDERR_DB) {
            for (i=0; i < iadd; i++) {
             fprintf(stderr, "addnew[]=%d\n", addnew[i]);
            }
          }

/*C************************************/
/*C  Loop through each new detection **/
/*C************************************/

          for (i = 0; i < num_sf; i++) {

            /* Initialize the number of past and forecast positions. */

	    new_cplt[nbr_new_cplts].num_past_pos = 0;
            new_cplt[nbr_new_cplts].num_fcst_pos = 0;

            j = n2o_order[i][0];
            if (j == NO_ORDER) continue;

/*C***************************************************************************/
/*C  If association, compute new average motion vector (u,v) and (dir,spd). **/
/*C***************************************************************************/
/*          Units for these motions and speeds are meters per second.        */

            x = feature_2d[i].cx;
            y = feature_2d[i].cy;
            diff_x = x - Old_cplt[j].llx;
            diff_y = y - Old_cplt[j].lly;
            ru_u = diff_x / (float)time_diff;
	    ru_v = diff_y / (float)time_diff;

            /* Convert them to m/s */
            ru_u = ru_u * KILO;
            ru_v = ru_v * KILO;

            if (Old_cplt[j].u_motion != UNDEFINED) {
              ru_u = (ru_u + Old_cplt[j].u_motion) * HALF;
              ru_v = (ru_v + Old_cplt[j].v_motion) * HALF;
            } /* end if */

            prop_spd = sqrt(ru_u*ru_u + ru_v*ru_v); 
            prop_dir = atan2(ru_u, ru_v) / DTR - HALF_CIRC;
            if (prop_dir < 0.0)
                prop_dir = CIRC + prop_dir;

            azm = atan2(x,y) / DTR;
            rng = sqrt(x*x + y*y);

            if (azm < 0.0)
                azm += 360;

            /* set the base negative because it is the first elevation */
       
            base = -feature_2d[i].ht;
            dia  =  feature_2d[i].dia;

            /* Copy information into new_cplt array */
            new_cplt[nbr_new_cplts] = Old_cplt[j];
	      
            /* Update the U and V motions */
            new_cplt[nbr_new_cplts].u_motion = ru_u;
            new_cplt[nbr_new_cplts].v_motion = ru_v;

            /* Update the age */
            new_cplt[nbr_new_cplts].age = new_cplt[nbr_new_cplts].age +
                                          ((float)time_diff / SECPERMIN);

/*C**************************************************************************/
/*C Fill in past centroid array for tracks and time-height cross sections  **/
/*C**************************************************************************/

            new_cplt[nbr_new_cplts].num_past_pos =
                          MIN((Old_cplt[j].num_past_pos+1),MAX_PAST);

            new_cplt[nbr_new_cplts].past_x[0] = Old_cplt[j].llx * KILO;
            new_cplt[nbr_new_cplts].past_y[0] = Old_cplt[j].lly * KILO;

            for (k = 1; k < new_cplt[nbr_new_cplts].num_past_pos; k++) {
              new_cplt[nbr_new_cplts].past_x[k] = Old_cplt[j].past_x[k-1];
              new_cplt[nbr_new_cplts].past_y[k] = Old_cplt[j].past_y[k-1];
            } /* end for second past position and beyond */

/*C*************************************/
/*C Fill in forecast array for tracks **/
/*C*************************************/

            new_cplt[nbr_new_cplts].num_fcst_pos = new_cplt[nbr_new_cplts].num_past_pos;
            if (new_cplt[nbr_new_cplts].num_fcst_pos > 4)
                new_cplt[nbr_new_cplts].num_fcst_pos = MAX_FCST;

            for (k = 0; k < new_cplt[nbr_new_cplts].num_fcst_pos; k++) {
              new_cplt[nbr_new_cplts].fcst_x[k] = new_cplt[nbr_new_cplts].llx * KILO +
                                 new_cplt[nbr_new_cplts].u_motion * fcst_time *
                                 (float)(k+1);
              new_cplt[nbr_new_cplts].fcst_y[k] = new_cplt[nbr_new_cplts].lly * KILO +
                                 new_cplt[nbr_new_cplts].v_motion * fcst_time *
                                 (float)(k+1);
            } /* end for all forecast positions */


            /* Modify new_cplt attributes that can be updated with information
             * from the lowest elevation scan */

            old_top = fabs(new_cplt[nbr_new_cplts].base) + new_cplt[nbr_new_cplts].depth;
            new_cplt[nbr_new_cplts].base = base;

            /* The implementation here is different with NSSL's */
            new_cplt[nbr_new_cplts].depth = old_top - fabs(base);

            if ((new_cplt[nbr_new_cplts].strength_rank >= MESO_SR_THRESH) &&
                (new_cplt[nbr_new_cplts].strength_rank != UNDEFINED) &&
                (new_cplt[nbr_new_cplts].nssl_base     != UNDEFINED)) {
              new_cplt[nbr_new_cplts].nssl_base = base;
	      new_cplt[nbr_new_cplts].nssl_depth = new_cplt[nbr_new_cplts].nssl_top -
						   fabs(new_cplt[nbr_new_cplts].nssl_base);
	    }
	    else if (fabs(new_cplt[nbr_new_cplts].core_base) != UNDEFINED) {
	      new_cplt[nbr_new_cplts].core_base = base;
              new_cplt[nbr_new_cplts].core_depth = new_cplt[nbr_new_cplts].core_top -
						     fabs(new_cplt[nbr_new_cplts].core_base);
	    } 

            new_cplt[nbr_new_cplts].ll_diam = dia;
            if( new_cplt[nbr_new_cplts].ll_diam > new_cplt[nbr_new_cplts].max_diam)
              new_cplt[nbr_new_cplts].max_diam = new_cplt[nbr_new_cplts].ll_diam;

            new_cplt[nbr_new_cplts].ll_rot_vel = feature_2d[i].rot_vel;

            if ( new_cplt[nbr_new_cplts].ll_rot_vel >new_cplt[nbr_new_cplts].max_rot_vel)
              new_cplt[nbr_new_cplts].max_rot_vel = new_cplt[nbr_new_cplts].ll_rot_vel;

            new_cplt[nbr_new_cplts].ll_shear = feature_2d[i].shr;

            if ( new_cplt[nbr_new_cplts].ll_shear > new_cplt[nbr_new_cplts].max_shear)
              new_cplt[nbr_new_cplts].max_shear = new_cplt[nbr_new_cplts].ll_shear;

            new_cplt[nbr_new_cplts].ll_gtg_vel_dif = feature_2d[i].gtgmax;
      
            if ( new_cplt[nbr_new_cplts].ll_gtg_vel_dif > new_cplt[nbr_new_cplts].max_gtg_vel_dif)
              new_cplt[nbr_new_cplts].max_gtg_vel_dif = new_cplt[nbr_new_cplts].ll_gtg_vel_dif;

            new_cplt[nbr_new_cplts].prop_dir = prop_dir;
            new_cplt[nbr_new_cplts].prop_spd = prop_spd;
            new_cplt[nbr_new_cplts].ll_azm = azm;
            new_cplt[nbr_new_cplts].ll_rng = rng;
            new_cplt[nbr_new_cplts].llx = x;
            new_cplt[nbr_new_cplts].lly = y;
            new_cplt[nbr_new_cplts].tvs_near = TVS_UNKNOWN;

            /* update time-height information */
            new_cplt[nbr_new_cplts].num2D = 1;
            new_cplt[nbr_new_cplts].mda_th_xs[0].tilt_num = 0; 
            new_cplt[nbr_new_cplts].mda_th_xs[0].height = (int)(feature_2d[i].ht*THOUSAND + 0.5); 
            new_cplt[nbr_new_cplts].mda_th_xs[0].diam = (int)(feature_2d[i].dia + 0.5); 
            new_cplt[nbr_new_cplts].mda_th_xs[0].rot_vel = (int)(feature_2d[i].rot_vel + 0.5); 
            new_cplt[nbr_new_cplts].mda_th_xs[0].shear = (int)(feature_2d[i].shr + 0.5); 
            new_cplt[nbr_new_cplts].mda_th_xs[0].gtgmax = (int)(feature_2d[i].gtgmax + 0.5); 
            new_cplt[nbr_new_cplts].mda_th_xs[0].rank = (int)(feature_2d[i].rank + 0.5); 
            new_cplt[nbr_new_cplts].mda_th_xs[0].ca = feature_2d[i].ca;
            new_cplt[nbr_new_cplts].mda_th_xs[0].cr = feature_2d[i].cr;
            
            /* set the detection_status to be 0; neither topped or extrapolated */
            new_cplt[nbr_new_cplts].detection_status = 0;

            nbr_new_cplts++;
          } /* end for each new detection */

        } /* END of else of if (BAD_TIME || num_sf == 0 || Nbr_old_cplts == 0) */ 
	 
        /* copy the info into first_elev_newcplt[] */
          
        *nbr_first_elev_newcplt = nbr_new_cplts;
        for (i = 0; i < nbr_new_cplts; i++)
        {
          first_elev_newcplt[i] = new_cplt[i];
        }
         
	/* Input the ones that should be coasted into the new_cplt array */

        if (nbr_new_cplts < MAX_MDA_FEAT)
        {
          for(i = 0; i < iadd; i++)
          {
            new_cplt[nbr_new_cplts] = Old_cplt[addnew[i]]; 
            new_cplt[nbr_new_cplts].detection_status = 2;/* Extrapolated */ 

            new_cplt[nbr_new_cplts].llx = new_cplt[nbr_new_cplts].fgx;
            new_cplt[nbr_new_cplts].lly = new_cplt[nbr_new_cplts].fgy;
            new_cplt[nbr_new_cplts].ll_azm = atan2(new_cplt[nbr_new_cplts].llx,
                                           new_cplt[nbr_new_cplts].lly) / DTR;
            if ( new_cplt[nbr_new_cplts].ll_azm < 0.0)
                 new_cplt[nbr_new_cplts].ll_azm +=360;

            new_cplt[nbr_new_cplts].ll_rng = sqrt(new_cplt[nbr_new_cplts].llx *
                                                  new_cplt[nbr_new_cplts].llx +
                                                  new_cplt[nbr_new_cplts].lly *
                                                  new_cplt[nbr_new_cplts].lly);

            nbr_new_cplts++;
            if (STDERR_DB) fprintf(stderr, "addnew[]=%d\n", addnew[i]);
          }/* end for */
        }/* end if */

/*      Sort so that newly added features are in the correct order. */

        mda_sort(SORT_BY_RANK_THEN_MSI, new_cplt, nbr_new_cplts);

        if (DEBUG) mda4d_out(output_fort34, new_cplt, nbr_new_cplts);
        if (DEBUG) mdattnn_out(output_ttnn, new_cplt, nbr_new_cplts);

/*      Get an output buffer.                  */

        outbuf = (char*)RPGC_get_outbuf(MDATTNN, sizeof(int) +
                                  sizeof(float) + sizeof(float) + 
                                  MESO_NUM_ELEVS * sizeof(int) +
                                 (sizeof(cplt_t) * nbr_new_cplts), &opstatus);
        outbuf_ptr = outbuf;

        if (opstatus != NORMAL) {
          RPGC_log_msg(GL_ERROR,
              "mdattnn: Error getting output LB!  opstatus=%d\n",opstatus);
          if(STDERR_DB) fprintf(stderr,"No output buffer available, aborting...\n");
          RPGC_abort();
          return;           /* go to the next volume */
        }

/*      Copy our work into the output buffer.                              */
/*      Start with the number of couplets.                                 */

        memcpy(outbuf_ptr, &nbr_new_cplts, sizeof(nbr_new_cplts));
        outbuf_ptr += sizeof(nbr_new_cplts);

/*      Copy the average U direction motion and average V direction motion */

        if (avg_mu != UNDEFINED) {
          memcpy(outbuf_ptr, &avg_mu, sizeof(avg_mu));
          outbuf_ptr += sizeof(avg_mu);
          memcpy(outbuf_ptr, &avg_mv, sizeof(avg_mv));
          outbuf_ptr += sizeof(avg_mv);
        }
        else {
          memcpy(outbuf_ptr, &mda_def_u, sizeof(mda_def_u));
          outbuf_ptr += sizeof(mda_def_u);
          memcpy(outbuf_ptr, &mda_def_v, sizeof(mda_def_v));
          outbuf_ptr += sizeof(mda_def_v);
        }

/*      Copy the elevation time array.                                     */          

        memcpy(outbuf_ptr, elev_time, (MESO_NUM_ELEVS*sizeof(int)));
        outbuf_ptr += (MESO_NUM_ELEVS*sizeof(int));

/*      Copy the couplet data itself.                                      */

        if(nbr_new_cplts > 0){
          memcpy(outbuf_ptr, new_cplt, sizeof(cplt_t) * nbr_new_cplts);
          outbuf_ptr += sizeof(cplt_t) * nbr_new_cplts;
        }

        outbuf_size = (int) (outbuf_ptr - outbuf);

/*      Send the product on its way */

        RPGC_rel_outbuf(outbuf,FORWARD|EXTENDED_ARGS_MASK, outbuf_size);
        outbuf = outbuf_ptr = NULL;

/*C*****************************/
/*C  End of program unit body **/
/*C*****************************/

        return;


} /* END of the function                                               */
