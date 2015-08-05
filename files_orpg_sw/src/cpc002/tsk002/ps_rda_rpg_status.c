/*
 * RCS info $Author: steves $ $Locker:  $ $Date: 2014/08/12 19:31:01 $ $Id:
 * ps_rda_rpg_status.c,v 1.18 1997/10/16 13:05:45 dodson Exp dodson $
 * $Revision: 1.45 $ $State: Exp $
 */


#include <stdlib.h>     /* EXIT_FAILURE, EXIT_SUCCESS              */
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <gen_stat_msg.h>
#include <infr.h>
#include <prod_user_msg.h>

#define PS_RDA_RPG_STATUS
#include <ps_globals.h>
#undef PS_RDA_RPG_STATUS

#define MAX_N_VOL 10

static time_t       Cr_volume_time = 0;   /* Current volume (data) time UNIX */    
static unsigned int Cr_volume_num = 0;    /* Current volume sequence number */
static unsigned int Cr_init_volnum = 0;   /* Initial volume number */
static short        Cr_init_vol = 1;      /* Initial volume flag */
static short        Cr_wx_mode = PS_DEF_WXMODE_UNKNOWN; 
					  /* Current volume weather mode */
static short        Pr_wx_mode = PS_DEF_WXMODE_UNKNOWN;
                                          /* Previous volume weather mode */
static short        Cr_vcp = 0;           /* Current volume VCP number */
static short        Pr_vcp = 0;           /* Previous volume VCP number */
static short        Cr_n_elev = 0;        /* Current volume VCP number angles */
static short        Cr_elev_angle[VCP_MAXN_CUTS]; /* Current volume VCP angles */
static short        Pr_n_elev = 0;        /* Previous volume VCP number angles */
static short        Pr_elev_angle[VCP_MAXN_CUTS]; /* Previous volume VCP angles */
static time_t       Clock_volume_time;    /* Current volume clock time UNIX */

/**************************************************************************

   Description: 
      This function initializes the Volume Status.
   
   Input:

   Output:

   Returns:
   
**************************************************************************/
void RRS_init_read(void){

   RRS_update_vol_status();

/* END of RRS_init_read () */
}

/**************************************************************************

    Description: 
        This function returns the current weather mode.

    Return: 
        Returns the current weather mode.

**************************************************************************/
int RRS_get_current_weather_mode(void){

   return ((int) Cr_wx_mode);

/* END of RRS_get_current_weather_mode() */
}

/**************************************************************************

    Description: 
        This function returns the previous weather mode.

    Return: 
        Returns the previous weather mode.

**************************************************************************/
int RRS_get_previous_weather_mode(void){

   return ((int) Pr_wx_mode);

/* END of RRS_get_previous_weather_mode() */
}

/**************************************************************************

    Description: 
        This function returns the current VCP number.

    Return: 
        Returns the current VCP number.

**************************************************************************/
int RRS_get_current_vcp_num(void){

   return ((int) Cr_vcp);

/* END of RRS_get_current_vcp_num() */
}

/**************************************************************************

    Description: 
        This function returns the previous VCP number.

    Return: 
        Returns the previous VCP number.

**************************************************************************/
int RRS_get_previous_vcp_num(void){

   return ((int) Pr_vcp);

/* END of RRS_get_previous_vcp_num() */
}

/**************************************************************************

    Description: 
        This function returns the current volume time and the
        clock time when the volume started.

    Output: 
         clock - the clock time when the volume started if not NULL.

    Return: 
         Returns the current volume (data) time.

**************************************************************************/
time_t RRS_get_volume_time(time_t * clock){

   if (clock != NULL)
      *clock = Clock_volume_time;

   return (Cr_volume_time);

/* END of RRS_get_volume_time() */
}

/**************************************************************************

    Description: 
        This function returns the initial volume flag.

    Return: 
         Returns the initial volume flag.

**************************************************************************/
int RRS_initial_volume(){

   return ((int) Cr_init_vol);

/* END of RRS_initial_volume() */
}

/**************************************************************************

    Description: 
        This function returns the initial volume number.

    Return: 
         Returns the initial volume number.

**************************************************************************/
unsigned int RRS_initial_volume_number(){

   return ( Cr_init_volnum );

/* END of RRS_initial_volume_number() */
}

/**************************************************************************

    Description: 
        This function returns the current volume number and the
        clock time when the volume started.

    Output: 
        clock - the clock time when the volume started if not NULL.

    Return: 
        Returns the current volume number.

**************************************************************************/
unsigned int RRS_get_volume_num(time_t * clock){

   if (clock != NULL)
      *clock = Clock_volume_time;
   return (Cr_volume_num);

/* END of RRS_get_volume_num() */
}

/**************************************************************************

   Description:
      Checks the current and previous VCPs for differences.

   Returns:
      RRS_DIFFERENT_VCP - VCPs differ because of different VCP numbers.
      RRS_DIFFERENT_N_ANGLES - VCPS differ because of different number
                               of angles. 
      RRS_DIFFERENT_ANGLES - VCPs differ because of different angles.
      RRS_NO_DIFFERENCES - No differences.
 
**************************************************************************/
int RRS_current_previous_vcps_differ(){

   int i;

   /* If the VCPs numbers are different, return 1. */
   if( Cr_vcp != Pr_vcp ){

      LE_send_msg( GL_INFO, "Current VCP %d different than Previous: %d\n",
                   Cr_vcp, Pr_vcp );
      return RRS_DIFFERENT_VCPS;

   }

   /* Number of elevations differ. */
   if( Cr_n_elev != Pr_n_elev ){

      LE_send_msg( GL_INFO, "Current VCP # Elevs: %d different than Previous: %d\n",
                   Cr_n_elev, Pr_n_elev );
      return RRS_DIFFERENT_N_ANGLES;

   }

   for( i = 0; i < Cr_n_elev; i++ ){

      if( Cr_elev_angle[i] != Pr_elev_angle[i] ){

         LE_send_msg( GL_INFO, "Cr_elev_angle[%d]: %d != Pr_elev_angle[%d]: %d\n",
                      i, Cr_elev_angle[i], i, Pr_elev_angle[i] );
         return RRS_DIFFERENT_ANGLES;
   
      }

   }

   return RRS_NO_DIFFERENCES;

/* END of RRS_current_previous_vcps_differ() */
}

/**************************************************************************

    Description: 
        This function returns the target elevation angle, in .1
        degrees, for the elevation index "elev_ind" in current VCP.

    Input:  
        elev_ind - the elevation index (starting with 1).

    Return: 
        Returns the elevation angle or -10000 if the elevation index
        is not found.

**************************************************************************/
int RRS_get_elevation_angle(int elev_ind){

   /* If elevation index > 0 and less than the number of angles in 
      current VCP, return angle corresponding to index. */
   if (elev_ind > 0 && elev_ind <= Cr_n_elev)
      return (Cr_elev_angle[elev_ind - 1]);
   else
      return (-10000);

/* END of RRS_get_elevation_angle() */
}


/**************************************************************************

    Description: 
        This function returns the elevation index (starting with 1)
        that has nearest elevation to "elev". The weather mode is
        also returned for atomicity.

    Input:  
        elev - the elevation angle in .1 degrees.

    Output: 
        wx_mode - the weather mode.

    Returns: 
        Returns the elevation index on success or 0 on failure.

**************************************************************************/
int RRS_get_elevation_index(int elev, int *wx_mode){

   int i;
   int mind;

   /* Initialize elev angle index of minimum difference to -1 */
   mind = -1;

   /* Cycle through elev angle table for minimum difference */
   for (i = 0; i < Cr_n_elev; i++){

      int diff, min;

      diff = elev - Cr_elev_angle[i];
      if (diff < 0)
         diff = -diff;

      /* If index of minimum difference still initialized or
         difference is the smallest so far, save index and 
         difference */
      if (mind == -1 || diff < min){

         min = diff;
         mind = i;

      }

   }

   /* If wx_mode not NULL, set it to current weather mode. */
   if (wx_mode != NULL)
      *wx_mode = Cr_wx_mode;

   /* If index of minimum difference non-negative, return index.
      Note:  elev_angles stored in zero-indexed array but elevation
      index starts at 1.  Therefore we add 1. */
   if (mind >= 0)
      return (mind + 1);

   else
      return (0);

/* END of RRS_get_elevation_index () */
}


/**************************************************************************

   Description: 
       This function updates the volume status. It is called
       when a volume scan starts.

   Inputs:

   Outputs:

   Returns:
      If the volume number in the volume status message is the same
      as the current volume scan number (i.e., for some reason this
      function gets called more than once per volume scan), 
      PS_DEF_DUP_VOL_NUM is returned.  Otherwise PS_DEF_SUCCESS is returned.

   Notes:

**************************************************************************/
int RRS_update_vol_status(void){

   Vol_stat_gsm_t vol;
   int n_elev, ind;
   int ret, i;

   /* Read Volume Status from GSM LB. */
   ret = ORPGDA_read(ORPGDAT_GSM_DATA, (char *) &vol,
                     sizeof(Vol_stat_gsm_t), VOL_STAT_GSM_ID);

   /* If not enough bytes read, return error. */
   if (ret != sizeof(Vol_stat_gsm_t)){

      LE_send_msg(GL_ORPGDA(ret),
             "ORPGDA_read of ORPGDAT_GSM_DATA (VOL_STAT_GSM_ID) Failed (%d)\n",
             ret);
      ORPGTASK_exit(GL_EXIT_FAILURE);

   }

   /* Save volume sequence number, initial volume flag, and initial volume
      number. */
   if( Cr_volume_num == vol.volume_number 
                     &&
       Cr_volume_num != Cr_init_volnum ){

      LE_send_msg( GL_INFO, 
         "Current Volume Number/GSM Volume Number Match!\n");
      return( PS_DEF_DUP_VOL_NUM );

   }
      
   Cr_volume_num = vol.volume_number;
   Cr_init_vol = vol.initial_vol;
   if( Cr_init_vol )
      Cr_init_volnum = Cr_volume_num;

   /* Set volume (data) time in UTC */
   Cr_volume_time = 
               UNIX_SECONDS_FROM_RPG_DATE_TIME(vol.cv_julian_date, vol.cv_time);

   /* Save current clock time UTC */
   Clock_volume_time = MISC_systime(NULL);

   /* Save current weather mode and previous weather mode. */
   Pr_wx_mode = Cr_wx_mode;
   Cr_wx_mode = vol.mode_operation;

   /* Save current VCP number */
   Pr_vcp = Cr_vcp;
   Cr_vcp = vol.vol_cov_patt;

   /* Copy current elevs to prev elevs. */
   Pr_n_elev = Cr_n_elev;
   for( i = 0; i < Cr_n_elev; i++ )
      Pr_elev_angle[i] = Cr_elev_angle[i];

   for( i = Cr_n_elev; i < VCP_MAXN_CUTS; i++ )
      Pr_elev_angle[i] = 0;

   /* Save VCP elevation angles for current volume. */
   ind = -1;
   n_elev = 0;
   for (i = 0; i < vol.num_elev_cuts; i++){

      if (ind == vol.elev_index[i])
         continue;

      if (n_elev >= VCP_MAXN_CUTS){

         LE_send_msg(GL_ERROR,
                     "Too many elevations (%d) found in VOL_STAT_GSM_ID");

         break;

      }

      ind = vol.elev_index[i];
      Cr_elev_angle[n_elev] = vol.elevations[i];
      n_elev++;

   }

   /* Save number of elevations. */
   Cr_n_elev = n_elev;

   /* Fill remainder of elevation angle table with zeros */
   for (i = n_elev; i < VCP_MAXN_CUTS; i++)
      Cr_elev_angle[i] = 0;

   return( PS_DEF_SUCCESS );

/* END of RRS_update_vol_status () */
}
