/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2011/11/09 19:06:28 $
 * $Id: mdattnn_acl.c,v 1.21 2011/11/09 19:06:28 ccalvert Exp $
 * $Revision: 1.21 $
 * $State: Exp $
 */
 
/******************************************************************************
 *	Module:         mdattnn_acl.c					      *
 *	Author:		Brian Klein					      *
 *                      Yukuan Song (added rapid update functionality         *
 *                                                                            *
 *   	Created:	Jan. 23, 2003					      *
 *                      Oct. 1, 2003 ( Rapid update version)                  *
 *	References:	WDSS MDA Fortran Source code (mdatrck.f dated 121602) *
 *			AEL						      *
 *									      *
 *      Description:    This file contains the main processing for the MDA    *
 *                      Track/Trend/Neural Network algorithm control loop.    *
 *									      *
 *      Notes:       	Original NSSL Fortran code comments have been         *
 *                      retained.                                             *
 ******************************************************************************/

#define DEBUG  	0                          /* Controls debug output to log file*/
#define STDERR_DB 0                        /* Controls debug to stderr      */
#define MIN(x,y) ( (x) < (y) ? (x) : (y) ) /* returns the lesser of x and y */
#define TEN 10.0                           /* constant value 10.0 */

/*	System include files	*/
#include <math.h>
/*#include <stdio.h>*/
#include <assert.h>

/*	Global include files	*/
#include "a309.h"
#include "rpgc.h"
#include "rpgcs.h"
#define FLOAT
#include "rpgcs_coordinates.h"

/*      Local include files     */
#include "mdattnn_params.h"
#include "mdattnn.h"
#include "mda_ru.h"

        int   nbr_new_cplts;  /* # 3Ds found in current volume, from mda3d    */
        int   nbr_new_cplts_non_zero; /* # 3Ds without 0 SR                 */

/*      Externals               */
extern  int    Nbr_old_cplts;
extern  int    Nbr_cplt_trks;
extern  int    Old_time;
extern  int    Old_date;
extern  cplt_t Old_cplt[];
extern  cplt_t new_cplt[];
extern  cplt_t new_cplt_non_zero[];

extern float mda_def_u;      /* Default (right moving) mesocyclone U motion. */
extern float mda_def_v;      /* Default (right moving) mesocyclone V motion. */

extern Prev_newcplt prev_newcplt[MAX_MDA_FEAT];
extern int nbr_prev_newcplt;

extern cplt_t first_elev_newcplt[MAX_MDA_FEAT];
extern int nbr_first_elev_newcplt;

#include "mda_ru.h"
#include "mda_ru_extrapolate.h"

/*      Local Prototypes        */

void mdattnn_out(FILE* fileptr, cplt_t* cplt, const int n_cplts);
void mda4d_out(FILE* fileptr_fort34, cplt_t* cplt, const int n_cplts);
int  getNewMesoID(int start_id);


void mdattnn_acl(FILE* output_ttnn, FILE* output_fort34, FILE* output_fort767)
{
        short i, j, k, l;     /* loop indices                                 */
        int   new_time, new_date; /* time and date of current volume          */
        int   time_diff;      /* time between this volume and the last (sec)  */
        static int   elev_time[MESO_NUM_ELEVS]; /* elev start times (secs from midnight)*/
        int   nbr_tvs;        /* TVSes found in current volume, from TDA      */
        static polar_loc_t tvs[TVFEAT_MAX]; /* Locations of TVSes this volume.       */
        static int   n2o_order[MAX_MDA_FEAT][MAX_MDA_FEAT];/* new to old order   */
        static float n2o_dist[MAX_MDA_FEAT][MAX_MDA_FEAT]; /* new to old distance*/
        float fcst_time;      /* time between forecasts (sec)                 */
        float avg_mu, avg_mv; /* avg. motion of detections in last vol.(m/sec)*/
        char* outbuf;         /* points to beginning of output buffer         */
        char* outbuf_ptr;     /* points to current position within output buf.*/
        int   opstatus;       /* status of infrastructure calls               */
        float old_u, old_v;   /* local copies of u & v motion from last vol.  */
        short back_check;
        short duplicate_association;
        int   vol_num;
        Scan_Summary *scan_summary;   /* ptr to scan summary table            */
        int   outbufSize;     /* byte size needed for output buffer           */
        int   overflow_flg;   /* bits set by mda1d, mda2d and mda3d for overflow conditions */
      
        /* The following variables added for rapid update */
	char* inbuf; 	       			/* pointer to the input buffer */
        static feature2d_t feature_2d[MESO_MAX_FEAT] ; /* 2D features */
	int elev_index;   			/* elevation index */
	int last_elev_index;   			/* elevation index */
	int mda_eof;           			/* flag end of the volume */
        int vcp_num;      			/* vcp number */
        int int_elevation; 			/* the elavation angle*10 */
        float elevation;  			/* elevation angle */
	int num_sf;       			/* number of 2D features */
	int iadd;            			/* num of the coasted   */
        static int addnew[MAX_MDA_FEAT];		/* index array   	*/
        int num_now_tracked;                    /* number of currently tracked feats*/
        float u_sum, v_sum;                     /* summation of U and V motions */

	int first_tilt;

	int match_index; /* the index of matched cplt at previous elev */

/***        float pot;***/            /* Probability of tornado (from NN)             */
/***        float posw;***/           /* Probability of severe wind (from NN)         */
        

        if (STDERR_DB) fprintf(stderr,"Entered mdatrack_acl...\n");

	/* Initialize some variables */
	iadd = 0;

/****  NEED TO GET THIS FROM SOMEWHERE, ADAPTATION DATA? ****/
        fcst_time = 300.0;   /* in seconds */

        /* The following block is added for the rapid update */
	inbuf = (char*)RPGC_get_inbuf(MDA3D, &opstatus);

        if (opstatus != NORMAL) {
          RPGC_log_msg(GL_INFO,
              "mdattnn: Could not get mda3d LB!  opstatus = %d\n",opstatus);
          if(STDERR_DB) fprintf(stderr,"Reading mda3d input failed, aborting...\n");
          RPGC_abort();
          return ;           /* go to the next volume */
        }
    
        /* get the elevation index and the angle */

	if( RPGC_is_buffer_from_last_elev( inbuf, &elev_index, &last_elev_index ) < 0 ){
          RPGC_log_msg(GL_ERROR,
              "mdattnn: Could not get elevation index!\n");
          RPGC_abort();
          return ;           /* go to the next volume */
        }

	vcp_num = RPGC_get_buffer_vcp_num(inbuf);
        if ( vcp_num == -1) {
          RPGC_log_msg(GL_ERROR,
              "mdattnn: Could not get vcp number!\n");
          RPGC_abort();
          return ;           /* go to the next volume */
        }

	/* check to see if this is the end of the volume */
	if ( elev_index == last_elev_index)
          mda_eof = 1;
        else
          mda_eof = 0;

        int_elevation = RPGCS_get_target_elev_ang(vcp_num, elev_index);
        if ( int_elevation == RPGCS_ERROR) {
          RPGC_log_msg(GL_ERROR,
              "mdattnn: Could not get target elevation angle!\n");
          RPGC_abort();
          return ;           /* go to the next volume */
        }

	elevation = int_elevation / TEN;
        if (STDERR_DB) {
          fprintf(stderr, "elev_index=%d\n", elev_index);
          fprintf(stderr, "vcp_num=%d\n", vcp_num);
          fprintf(stderr, "elevation=%f\n", elevation);
        }

	/* Obtain the date and time using the volume number returned in       */
	/* The call to RPGC_get_buffer_vol_num(void* bufptr).                 */
	
 	vol_num = RPGC_get_buffer_vol_num(inbuf); 
        scan_summary = RPGC_get_scan_summary(vol_num);

        new_time   = scan_summary->volume_start_time;  /* in seconds */
        new_date   = scan_summary->volume_start_date;
        if (STDERR_DB) {
           fprintf(stderr,"  new_time=%d   ", new_time);
           fprintf(stderr,"  new_date=%d \n", new_date);
           fprintf(stderr,"Nbr_old_cplts = %d   ",Nbr_old_cplts);
           fprintf(stderr,"Nbr_cplt_trks = %d \n",Nbr_cplt_trks);
        }

        if (new_time <  0 || new_time > SECPERDAY || new_date <= 0) {
          RPGC_log_msg(GL_ERROR,
              "mdattnn: Error reading mda3d LB!  Invalid date/time\n");
            if(STDERR_DB) fprintf(stderr,"Invalid date/time, aborting...\n");
            RPGC_abort();
            return;           /* go to the next volume */
        }

	/*Read the SCIT attributes data for the average storm cell speed *
         *It is called only at the end of the volume */

        if ( mda_eof == 1){
          if(STDERR_DB) fprintf(stderr,"reading SCIT data\n");
          readSCITInput(&mda_def_u, &mda_def_v);
        }

        if ( elev_index == 1) {
          readInput_at_firstTilt(&num_sf, &overflow_flg, elev_time, feature_2d, inbuf);

          /*      Release the input buffer now that we've copied it.                 */
          RPGC_rel_inbuf((void*)inbuf);
          inbuf = NULL;

          /* Do updating at the first elevation */
          tracking_at_firstTilt(num_sf, feature_2d, new_time, new_date, elevation,
             mda_def_u, mda_def_v, output_ttnn, output_fort34,
	     &nbr_first_elev_newcplt, first_elev_newcplt, elev_time);

          /* reinitialize the number of new cplt at the previous elevation */
          nbr_prev_newcplt = 0;
          
          /*Read the TDA attributes data for the location of TVSes and ETVSes */
          /* We need to do this at the first elevation even though we know    */
          /* there is no TVS data yet.  Otherwise, the input buffers will get */
          /* out of sync.                                                     */
           
          nbr_tvs = readTdaInput(tvs);
        }
        else
        {
/*        Read the driving input data first                                  */

          nbr_new_cplts = readMda3dInput(new_cplt, elev_time, &vol_num,
                                         &overflow_flg, inbuf);

          /* Check to make sure no feature is downgraded */
          if ( mda_eof != 1) {
	  mda_ru_downgrading_prevention(new_cplt, nbr_new_cplts, 
				first_elev_newcplt, nbr_first_elev_newcplt);
          }

          if (STDERR_DB) fprintf(stderr, "nbr_new_cplts=%d\n", nbr_new_cplts);

          /* Release the input buffer now that we've copied it.                 */
          RPGC_rel_inbuf((void*)inbuf);
          inbuf = NULL;

          /*Read the TDA attributes data for the location of TVSes and ETVSes */
          nbr_tvs = readTdaInput(tvs);


/*        Initialize the neural network by reading in the data files.      */

/***        readNNfiles();***/

/*C*********************************************/
/*C  Sort 3D couplets before time association **/
/*C*********************************************/

          mda_sort(SORT_BY_RANK_THEN_MSI, new_cplt, nbr_new_cplts);

/*C***********************************************************************/
/*C  If there are any old detections, compute new average motion vector **/
/*C***********************************************************************/

          getAverageMotion(Old_cplt, Nbr_old_cplts, &avg_mu, &avg_mv);

          if (STDERR_DB) fprintf(stderr,"avg_mu = %f, avg_mv = %f\n", avg_mu, avg_mv);

/*C*********************************************************************/
/*C  Compute time difference between current and previous volume scan **/
/*C*********************************************************************/

          time_diff = getTimeDiff(new_time, new_date, Old_time, Old_date);

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

          /* Initialize the number of currently tracked features and the  */
          /* U and V summation variables.                                 */

          num_now_tracked = 0;
          u_sum = 0.0;
          v_sum = 0.0;
        
/*C*********************************************/
/*C  If no detections from current scan, quit **/
/*C*********************************************/

          if (nbr_new_cplts == 0) {

	    if(STDERR_DB) fprintf(stderr, "No new cplt is detected upto this tilt!\n");

            nbr_prev_newcplt = 0;     /* added for Ru */
            /*  Nbr_old_cplts = 0; commented out for Ru */

          } else {
            if (STDERR_DB) fprintf(stderr,"Checking # old detections...\n");

/*C***********************************************************************/
/*C  If there are no detections from the previous scan, or if TIME_DIFF **/
/*C  is too great, we can't time associate.  Assign event IDs, fill in  **/
/*C  time-height array, and define other attributes for the new         **/
/*C  detections that were not time associated.                          **/
/*C***********************************************************************/

            if (Nbr_old_cplts == 0 || time_diff == BAD_TIME) {

              for (i = 0; i < nbr_new_cplts; i++) {
                new_cplt[i].meso_event_id = getNewMesoID(Nbr_cplt_trks);
                new_cplt[i].u_motion      = UNDEFINED;
                new_cplt[i].v_motion      = UNDEFINED;
                new_cplt[i].time_code     = NOT_ASSOCIATED;
                new_cplt[i].prop_dir      = UNDEFINED;
                new_cplt[i].prop_spd      = UNDEFINED;
                new_cplt[i].age           = (float)0.0;
                new_cplt[i].num_past_pos  = (int)0;
                new_cplt[i].num_fcst_pos  = (int)0;

/*              This was moved to here from the original NSSL Fortran code */
/*              because we are using zero-relative IDs.                    */
                Nbr_cplt_trks++;
                if (Nbr_cplt_trks > MESO_MAX_EVENT) Nbr_cplt_trks = 0;

/****************************************************************************/
/*        Call the neural network function for determining the probablility */
/*        of a tornado and probability of severe wind for each new couplet. */
/****************************************************************************/

/***                mdaNN(&new_cplt[i], &pot, &posw);***/

/*              Place the probabilities into the new_couplet array            */

/***                new_cplt[i].prob_of_tornado  = pot;***/
/***                new_cplt[i].prob_of_svr_wind = posw;***/
              
                /* Find the closest SCIT identified storm cell and            */
                /* assign the associated storm ID                             */
              
                assignStormId(&(new_cplt[i]));

/****************************************************************************/
/* See if this is a "Tornadic Meso".                                        */
/****************************************************************************/

                /* Initialize the TVS location status   */
       
                new_cplt[i].tvs_near = TVS_UNKNOWN;
                 
                if (makeTVSAssociation(&new_cplt[i], tvs, nbr_tvs))
                   new_cplt[i].tvs_near = TVS_YES;
                else
                   new_cplt[i].tvs_near = TVS_NO;
                 
                if (elev_index == 2)
                  new_cplt[i].tvs_near = TVS_UNKNOWN;

              } /* end for all new couplets */

            } else {

/*C*************************************************************************/
/*C  There are detections from both the previous and current scans so now **/
/*C  can time associate.  Compute MESO_MAX_SPD, the adaptable maximum     **/
/*C  speed threshold (km/min) for mesocyclone movement.                   **/
/*C*************************************************************************/
              if (STDERR_DB) fprintf(stderr,"Detections in two scans...\n");

/*C**************************************************************************/
/*C  Loop through all the old detections.  Set propgation u and v to the   **/
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
              } /* end for all old detections */

/*C****************************************/
/*C  Initialize feature selection arrays **/
/*C****************************************/

              for (i = 0; i < MAX_MDA_FEAT; i++) {
                for (j = 0; j < MAX_MDA_FEAT; j++) {
                  n2o_dist[i][j]  = UNDEFINED;
                  n2o_order[i][j] = NO_ORDER;
                } /*end for */
              } /* end for */


/*C******************************************************************/
/*C  Search for time association between new and old detections    **/
/*C  using an increasing distance threshold.  Do not associate any **/
/*C  detections with a rank=0 or meeting SHALLOW criteria.         **/
/*C******************************************************************/

              for (i = 0; i < nbr_new_cplts; i++) {

/*C************************************************/
/*C  Initialize variables for the new detections **/
/*C************************************************/

                new_cplt[i].u_motion      = UNDEFINED;
                new_cplt[i].v_motion      = UNDEFINED;
                new_cplt[i].prop_dir      = UNDEFINED;
                new_cplt[i].prop_spd      = UNDEFINED;
                new_cplt[i].time_code     = NOT_ASSOCIATED;
                new_cplt[i].age           = (float)0.0;
                new_cplt[i].num_past_pos  = (int)0;

/*****************************************************/
/*   Make the time-associations between this couplet */
/*   and those from the last volume.                 */
/*****************************************************/

                makeMesoAssociations(new_cplt[i], Old_cplt, Nbr_old_cplts,
                                     time_diff, n2o_dist[i], n2o_order[i]);
              } /* end for all new detections */

/*C****************************************************************************/
/*C  Check to see if any old detections are associated with more than one new**/
/*C  detection.  If so, take closest new detection, and make the other new   **/
/*C  detections use their second closest associated old detection.  Repeat   **/
/*C  process until all new detections are associated with best old detection.**/
/*C****************************************************************************/

              i = 0;

              while (i < nbr_new_cplts) {

                if (n2o_order[i][0] == NO_ORDER) {
                  i++; /* increment here because continue skips while increment*/
                  continue; /* continue while loop */
                }

                back_check = NO;
                duplicate_association = NO;

                for ( j = 0; j < nbr_new_cplts; j++) {

                  if ( i == j || n2o_order[j][0] == NO_ORDER) continue;

                  if (n2o_order[i][0] == n2o_order[j][0]) {

                    duplicate_association = YES;
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
               * within MDA_TASSOC_DZ of the feature from the previous volume scan. */

              first_tilt = 0;
              mda_ru_extrapolate(&iadd, addnew, n2o_order, elevation, nbr_new_cplts, first_tilt);
	      
/*C************************************/
/*C  Loop through each new detection **/
/*C************************************/

              for (i = 0; i < nbr_new_cplts; i++) {

	        /* Initialize the number of past and forecast positions. */
	        new_cplt[i].num_past_pos = 0;
	        new_cplt[i].num_fcst_pos = 0;

                j = n2o_order[i][0];
                if (j == NO_ORDER) continue;
                
                /* Count this one as being tracked.                      */
                num_now_tracked++;

	        /* If associated, new_cplt attributes are checked to prevent *
	         * them from downgrading */

	        /* Make sure the feature is not topped AND is not at end of the volume */
	        if ( new_cplt[i].detection_status != 1 && mda_eof != 1) {
	          if (new_cplt[i].strength_rank < Old_cplt[j].strength_rank){
	            new_cplt[i].strength_rank = Old_cplt[j].strength_rank;
                    new_cplt[i].circ_class    = Old_cplt[j].circ_class; 
                  }

                  if (new_cplt[i].msi < Old_cplt[j].msi)
                      new_cplt[i].msi = Old_cplt[j].msi;

                  if (new_cplt[i].nssl_top   < Old_cplt[j].nssl_top &&
                      new_cplt[i].nssl_depth < Old_cplt[j].nssl_depth) {
	             new_cplt[i].nssl_top   = Old_cplt[j].nssl_top;
                     new_cplt[i].nssl_depth = Old_cplt[j].nssl_depth;
                  }

                  if (new_cplt[i].core_top   < Old_cplt[j].core_top &&
		      new_cplt[i].core_depth < Old_cplt[j].core_depth) {
	             new_cplt[i].core_top   = Old_cplt[j].core_top;
		     new_cplt[i].core_depth = Old_cplt[j].core_depth; 
	          }

                  if ( new_cplt[i].max_diam < Old_cplt[j].max_diam )
	               new_cplt[i].max_diam = Old_cplt[j].max_diam;

	          if ( new_cplt[i].max_rot_vel < Old_cplt[j].max_rot_vel)
		       new_cplt[i].max_rot_vel = Old_cplt[j].max_rot_vel;

	          if ( new_cplt[i].max_shear < Old_cplt[j].max_shear)
		       new_cplt[i].max_shear = Old_cplt[j].max_shear;

	          if ( new_cplt[i].max_gtg_vel_dif < Old_cplt[j].max_gtg_vel_dif)
		       new_cplt[i].max_gtg_vel_dif = Old_cplt[j].max_gtg_vel_dif;
	        }

/*C***************************************************************************/
/*C  If association, compute new average motion vector (u,v) and (dir,spd). **/
/*C***************************************************************************/
/*              Units for these motions and speeds are meters per second.      */

                new_cplt[i].u_motion = (new_cplt[i].llx - Old_cplt[j].llx)* KILO /
                                       (float)time_diff;
                new_cplt[i].v_motion = (new_cplt[i].lly - Old_cplt[j].lly)* KILO /
                                       (float)time_diff;
                                       
                if (Old_cplt[j].u_motion != UNDEFINED) {
                  new_cplt[i].u_motion = (new_cplt[i].u_motion +
                                          Old_cplt[j].u_motion) * HALF;
                  new_cplt[i].v_motion = (new_cplt[i].v_motion +
                                          Old_cplt[j].v_motion) * HALF;
                } /* end if */

                new_cplt[i].prop_spd = 
                  sqrt(new_cplt[i].u_motion * new_cplt[i].u_motion +
                       new_cplt[i].v_motion * new_cplt[i].v_motion);

                new_cplt[i].prop_dir =
                  atan2(new_cplt[i].u_motion,new_cplt[i].v_motion) / DTR -
                                                                 HALF_CIRC;
                if (new_cplt[i].prop_dir < 0.0)
                    new_cplt[i].prop_dir = CIRC + new_cplt[i].prop_dir;

                /*  Sum the U and V components for computing the average. */
                u_sum += new_cplt[i].u_motion;
                v_sum += new_cplt[i].v_motion;


/*C***********************************************************/
/*C  Set time association code and compute age of detection **/
/*C***********************************************************/

                new_cplt[i].time_code = ASSOCIATED;
                Old_cplt[j].time_code = ASSOCIATED;
                new_cplt[i].age       = Old_cplt[j].age + 
                                             ((float)time_diff / SECPERMIN);

/*C*************************************/
/*C  Copy event ID into new detection **/
/*C*************************************/

                new_cplt[i].meso_event_id = Old_cplt[j].meso_event_id;

/*C**********************************************************/
/*C  Set codes to classify this as a time-associated MESO. **/
/*C**********************************************************/

                if (new_cplt[i].circ_class == COUPLET_3D ||
                    new_cplt[i].circ_class == LOW_TOPPED_MESO) { 
                  
                  if (new_cplt[i].strength_rank >= MESO_SR_THRESH)
                     new_cplt[i].user_meso = YES;
                 
                } /* end if */

/*C**************************************************************************/
/*C Fill in past centroid array for tracks and time-height cross sections  **/
/*C**************************************************************************/

                new_cplt[i].num_past_pos =
                          MIN((Old_cplt[j].num_past_pos+1),MAX_PAST);

                new_cplt[i].past_x[0] = Old_cplt[j].llx * KILO;
                new_cplt[i].past_y[0] = Old_cplt[j].lly * KILO;

                for (k = 1; k < new_cplt[i].num_past_pos; k++) {
                  new_cplt[i].past_x[k] = Old_cplt[j].past_x[k-1];
                  new_cplt[i].past_y[k] = Old_cplt[j].past_y[k-1];
                } /* end for second past position and beyond */

/*C*************************************/
/*C Fill in forecast array for tracks **/
/*C*************************************/

                new_cplt[i].num_fcst_pos = new_cplt[i].num_past_pos;
                if (new_cplt[i].num_fcst_pos > 4)
                    new_cplt[i].num_fcst_pos = MAX_FCST;

                for (k = 0; k < new_cplt[i].num_fcst_pos; k++) {
                  new_cplt[i].fcst_x[k] = new_cplt[i].llx * KILO +
                                        new_cplt[i].u_motion * fcst_time *
                                        (float)(k+1);
                  new_cplt[i].fcst_y[k] = new_cplt[i].lly * KILO +
                                        new_cplt[i].v_motion * fcst_time *
                                        (float)(k+1);
                } /* end for all forecast positions */
              } /* end for each new detection */

/*            Compute the average U and V of all tracked features  */
/*            in this elevation scan.                              */

              if (num_now_tracked > 0 ) {
                 avg_mu = u_sum / num_now_tracked;
                 avg_mv = v_sum / num_now_tracked;
              }
              else {
                 avg_mu = UNDEFINED;
                 avg_mv = UNDEFINED;
              }

              for (i = 0; i < nbr_new_cplts; i++) {

/****************************************************************************/
/*        Call the neural network function for determining the probablility */
/*        of a tornado and probability of severe wind for each new couplet. */
/****************************************************************************/

/***              mdaNN(&new_cplt[i], &pot, &posw);***/

/*              Place the probabilities into the new_couplet array          */

/***              new_cplt[i].prob_of_tornado  = pot;***/
/***              new_cplt[i].prob_of_svr_wind = posw;***/
              
                /* Find the closest SCIT identified storm cell and          */
                /* assign the associated storm ID                           */
              
                assignStormId(&(new_cplt[i]));
              
/****************************************************************************/
/* See if this is a "Tornadic Meso".                                        */
/****************************************************************************/
                /* Initialize the TVS location status   */
       
                new_cplt[i].tvs_near = TVS_UNKNOWN;
                 
                if (makeTVSAssociation(&new_cplt[i], tvs, nbr_tvs))
                   new_cplt[i].tvs_near = TVS_YES;
                else
                   new_cplt[i].tvs_near = TVS_NO;
                 
                if (elev_index == 2)
                  new_cplt[i].tvs_near = TVS_UNKNOWN;

              } /* end for each new detection */

/*C*********************************************************************/
/*C  Assign event IDs, fill in time-height array, and define other    **/
/*C  attributes for the new detections that were not time associated. **/
/*C*********************************************************************/

              for (i = 0; i < nbr_new_cplts; i++) {

                if (new_cplt[i].time_code == NOT_ASSOCIATED) {
                  match_index = -1;
                  for (j = 0; j < nbr_prev_newcplt; j++) {
                    if ( prev_newcplt[j].ll_azm == new_cplt[i].ll_azm &&
                         prev_newcplt[j].ll_rng == new_cplt[i].ll_rng &&
		         prev_newcplt[j].time_code == new_cplt[i].time_code) {
                     match_index = j;
                     break;
                    }
                  } /*end for */

	          if (match_index == -1) {
                    new_cplt[i].meso_event_id = getNewMesoID(Nbr_cplt_trks);
                    new_cplt[i].u_motion      = UNDEFINED;
                    new_cplt[i].v_motion      = UNDEFINED;
/*                  This increment was moved to here from the original NSSL */
/*                  Fortran code because we are using zero-relative IDs.    */
                    Nbr_cplt_trks++;
                    if (Nbr_cplt_trks > MESO_MAX_EVENT) Nbr_cplt_trks = 0;
                  }
                  else {
	            new_cplt[i].meso_event_id = prev_newcplt[match_index].meso_event_id;
                    new_cplt[i].u_motion      = UNDEFINED;
                    new_cplt[i].v_motion      = UNDEFINED;
	          } /* end if */

                } /* end if */

              } /* end for */

            } /* end if there are detections in the previous and current scan */

          } /* end else there are new detections */

	  /* Remove any low core features that are not associated with a
             SCIT storm cell.  This was found to eliminate numerous false
             "clear air" detections, particularly with noisy velocity data */

          i = 0;
          while (i < nbr_new_cplts) {
/*             if ((new_cplt[i].circ_class == LOW_TOPPED_MESO) && */
             if ((new_cplt[i].storm_id[0] == '?' &&
                  new_cplt[i].storm_id[1] == '?'))  {
                if (i < nbr_new_cplts-1) {
                   /* Move the all remaining couplets down one array index */
                   for (j = i; j < nbr_new_cplts-1; j++)
                     new_cplt[j] = new_cplt[j+1];
                   i--; /* Need to recheck the couplet now at index i */
                }
                nbr_new_cplts--;
             } /* end if */
             i++;
          } /* end while */

          /* Update the prev_newcplt[] array and nbr_newcplt */
	  nbr_prev_newcplt = nbr_new_cplts;
          for ( i = 0; i < nbr_prev_newcplt; i++) 
          {
	    prev_newcplt[i].meso_event_id = new_cplt[i].meso_event_id;
	    prev_newcplt[i].ll_azm = new_cplt[i].ll_azm;
	    prev_newcplt[i].ll_rng = new_cplt[i].ll_rng;
	    prev_newcplt[i].time_code = new_cplt[i].time_code;
          }

	  /* Add coasted elevated features to new_cplt array */
          if ( mda_eof != 1 && iadd > 0) {
            for ( i = 0; i < iadd; i++)
            {
              new_cplt[nbr_new_cplts] = Old_cplt[addnew[i]];
              new_cplt[nbr_new_cplts].detection_status = 2; /* Extrapolated feature */
	    
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
             } /* end for */
          } /* end if */

/*        Sort again so that newly added features are in the correct order. */

          mda_sort(SORT_BY_RANK_THEN_MSI, new_cplt, nbr_new_cplts);

/*C************************************************************************/
/*C  Write information about current detections into the "previous scan" **/
/*C  arrays and counters.                                                **/
/*C************************************************************************/
          /* Write out this only at the end of volume scan */

          if ( mda_eof == 1) {
            Nbr_old_cplts = nbr_new_cplts;
            Old_time      = new_time;
            Old_date      = new_date;

            for (i = 0; i < nbr_new_cplts; i++)
                Old_cplt[i] = new_cplt[i];

          } /* END of if ( mda_eof == 1)  */

          if (DEBUG) mda4d_out(output_fort34, new_cplt, nbr_new_cplts);
          if (DEBUG) mdattnn_out(output_ttnn, new_cplt, nbr_new_cplts);

          /* filter out features with 0 SR and make sure no more than */
          /* MAX_MDA_PROD features are sent to mdaprod.               */
          nbr_new_cplts_non_zero = 0;
	  for (i = 0; i < nbr_new_cplts; i++) {
	    if ((new_cplt[i].strength_rank >= 1 &&
	         nbr_new_cplts_non_zero    <  MAX_MDA_PROD)) {
              new_cplt_non_zero[nbr_new_cplts_non_zero] = new_cplt[i];
              nbr_new_cplts_non_zero++; 
            }
	  } /* end for */

/*        Get an output buffer.                  */

	  outbufSize = sizeof(int) + sizeof(float) + sizeof(float) +
                      (sizeof(int) * MESO_NUM_ELEVS) +
                      (sizeof(cplt_t) * nbr_new_cplts_non_zero);
          outbuf = (char*)RPGC_get_outbuf(MDATTNN, outbufSize, &opstatus);
          outbuf_ptr = outbuf;

          if (opstatus != NORMAL) {
            RPGC_log_msg(GL_INFO,
              "mdattnn: Error getting output LB!  opstatus=%d\n",opstatus);
            if(STDERR_DB) fprintf(stderr,"No output buffer available, aborting...\n");
            RPGC_abort();
            return;           /* go to the next volume */
          }

          /* Output any overflow messages at the end of the volume
             if there are valid 3D features                        */
          if (mda_eof == 1 && nbr_new_cplts_non_zero > 0) {
           if (overflow_flg == 1 || overflow_flg == 3)
              RPGC_log_msg(GL_ERROR,
                          "Too many 1D shear vectors; some MD features not detected");
           if (overflow_flg == 2 || overflow_flg == 3)
              RPGC_log_msg(GL_STATUS|LE_RPG_WARN_STATUS,
                          "Too many 2D features; some MD features not detected");
           if (overflow_flg >= 4)
              RPGC_log_msg(GL_STATUS|LE_RPG_WARN_STATUS,
                          "Too many 3D features; some MD features not detected");
          }

/*        Copy our work into the output buffer.                              */
/*        Start with the number of couplets.                                 */

          memcpy(outbuf_ptr, &nbr_new_cplts_non_zero, sizeof(nbr_new_cplts_non_zero));
          outbuf_ptr += sizeof(nbr_new_cplts_non_zero);

/*        Copy the average U direction motion and average V direction motion */

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

/*        Copy the elevation time array.                                     */          

          memcpy(outbuf_ptr, elev_time, (MESO_NUM_ELEVS*sizeof(int)));
          outbuf_ptr += (MESO_NUM_ELEVS*sizeof(int));

/*        Copy the couplet data itself.                                      */

          if(nbr_new_cplts_non_zero > 0) {
             memcpy(outbuf_ptr, new_cplt_non_zero, sizeof(cplt_t) * nbr_new_cplts_non_zero);
             outbuf_ptr += sizeof(cplt_t) * nbr_new_cplts_non_zero;
          }

/*        Send the product on its way */

          RPGC_rel_outbuf(outbuf,FORWARD|RPGC_EXTEND_ARGS,outbufSize);
          outbuf = outbuf_ptr = NULL;

        /* The following block is raAdded by YKsong for the rapid update */
	} /* END of else of if (elev_index == 1) */

        /***************end of this block (YKsong) ***********************/

/*C*****************************/
/*C  End of program unit body **/
/*C*****************************/

        return;

} /* end mdattnn_acl() */


/*C================================================================*/

int getNewMesoID(int start)
{
        int i, j, new_id;
        int found;

        new_id = start;
        for (j=0; j < MESO_MAX_EVENT; j++) {
           found = 0;
           for (i=0; i < MAX_MDA_FEAT; i++) {
              if (new_id == Old_cplt[i].meso_event_id) {
                 found=1;
                 break;
              }
           }
           if (!found) {
              for (i=0; i < nbr_new_cplts; i++) {
                 if (new_id == new_cplt[i].meso_event_id) {
                    found=1;
                    break;
                 }
              }
           }
           if (!found) {
              for (i=0; i < nbr_prev_newcplt; i++) {
                 if (new_id == prev_newcplt[i].meso_event_id) {
                    found=1;
                    break;
                 }
              }
           }
           if (found) {
              new_id++;
              if (new_id > MESO_MAX_EVENT) new_id = 0;
           }
           else
              return new_id;
        }
        RPGC_log_msg(GL_ERROR,"No available meso ID!");
        LE_send_msg( GL_STATUS | LE_RPG_INFO_MSG,"MDA: No unique Meso IDs available\n");

        return -1;

} /* end of getNewMesoID() */

/*================================================================*/

void mda4d_out(FILE* fileptr, cplt_t* new_cplt, int n_cplts)
{
        int i;

/*C*********************************************/
/*C  If no detections from current scan, quit **/
/*C*********************************************/
                                                       
        if (n_cplts > 0) {        

/*C****************************************************/
/*C  Sort the mesocyclones for the mesocyclone table **/
/*C****************************************************/

/***          mda_sort(SORT_BY_RANK_THEN_MSI);***/

/*C**************************/
/*C  Write to output files **/
/*C**************************/

          fprintf(fileptr,"\n   ID  Azmth Range  Dir  Speed  Age  CC.SR");
          fprintf(fileptr,"MSIa MSIr NSSL TORN LOWT DISP\n");
          fprintf(fileptr,"  ---- ----- ----- ----- ----- ----- ---- ");
          fprintf(fileptr,"---- ---- ---- ---- ---- ----\n");

          for (i = 0; i < n_cplts; i++) {

            fprintf(fileptr,
              "%5d %6.1f%6.1f%6.1f%6.1f%6.1f%5d\n",
                    new_cplt[i].meso_event_id,
                    new_cplt[i].ll_azm,
                    new_cplt[i].ll_rng,
                    new_cplt[i].prop_dir,
                    new_cplt[i].prop_spd,
                    new_cplt[i].age,
                    new_cplt[i].circ_class);

          } /* end for */

        } /* end if */

/*C**************************/
/*C  End program unit body **/
/*C**************************/
    
        return;
} /* end of function mda4d_out() */

/*================================================================*/
       
void mdattnn_out(FILE* fileptr, cplt_t new_cplt[], int n_cplts)
{
        int i, p, f;

/*C*********************************************/
/*C  If no detections from current scan, quit **/
/*C*********************************************/
                                                       
        if (n_cplts > 0) { 

          fprintf(fileptr,"\n  ID  Azmth Range  Dir  Speed  Age LLX LLY class SR meso pot posw stmID df  ds\n");
          fprintf(fileptr," --- ----- ----- ----- ----- ---- ----- ------\n");

          for (i = 0; i < n_cplts; i++) {
            if( new_cplt[i].strength_rank < 1) continue; 
            fprintf(fileptr,
              "%4d %6.1f%6.1f%6.1f%6.1f%4.1f%6.1f%6.1f%3d%3d%3d%5.0f%5.0f  %s  %d  %d %f %f\n",
                    new_cplt[i].meso_event_id,
                    new_cplt[i].ll_azm,
                    new_cplt[i].ll_rng,
                    new_cplt[i].prop_dir,
                    new_cplt[i].prop_spd,
                    new_cplt[i].age,
                    new_cplt[i].llx,
                    new_cplt[i].lly,
                    new_cplt[i].circ_class,
                    new_cplt[i].strength_rank,
                    new_cplt[i].user_meso,
                    new_cplt[i].prob_of_tornado * 100.,
                    new_cplt[i].prob_of_svr_wind * 100.,
                    new_cplt[i].storm_id,
                    new_cplt[i].display_filter,
                    new_cplt[i].detection_status,
		    new_cplt[i].base, new_cplt[i].depth);

            if (new_cplt[i].age > 0.0) {

              fprintf(fileptr,"\n      Past Positions:\n");
              fprintf(fileptr,"      ----------------\n");

              for (p = 0; p < (new_cplt[i].num_past_pos); p++) {
                fprintf(fileptr, "     x = %6.1f  y = %6.1f\n",
                              (new_cplt[i].past_x[p]), new_cplt[i].past_y[p]);
              }

              fprintf(fileptr,"\n      Forecast Positions:\n");
              fprintf(fileptr,"      --------------------\n");

              for (f = 0; f < (new_cplt[i].num_fcst_pos); f++) {
                fprintf(fileptr, "     x = %6.1f  y = %6.1f\n",
                              new_cplt[i].fcst_x[f], new_cplt[i].fcst_y[f]);
              }
 
              fprintf(fileptr,"\n");

            }  /* end if */

          } /* end for */

        } /* end if */

      return;
} /* end of function mdatrack_out() */

