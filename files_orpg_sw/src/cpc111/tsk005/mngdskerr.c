/*******************
  RCS info
  $Author: steves $
  $Locker:  $
  $Date: 2005/12/27 17:07:43 $
  $Id: mngdskerr.c,v 1.7 2005/12/27 17:07:43 steves Exp $
  $Revision: 1.7 $
  $State: Exp $

KSTAT INFO:
   man kstat
   /usr/include/sys/kstat.h  - for structures and types 


   2/8/2003 Chris Gilbert NA02-33001 Issue 2-091 - Improve Error Reporting in error log.

********************/

#include <kstat.h>
#include <stdio.h>
#include <sys/cpuvar.h>
#include <signal.h>
#include <stdlib.h>
#include "infr.h"
#include "orpgerr.h"
#include "orpgmisc.h"
#include "orpginfo.h"
#include "orpg.h"
 
void update_kstat (kstat_ctl_t *kc);
void get_disk_stats (kstat_ctl_t *kc); 
void get_jaz_stats (kstat_ctl_t *kc); 
extern void sig_handler (int sig);

static char JAZNAME[30]="sd3,err";
static char HDNAME[30]="dad0,error";
/* default polling time in seconds */
static long poll_time = 15;
static int test_flag = 0;

int main (int argc, char **argv) {
   kstat_ctl_t *kc = NULL;
   int ret;
   struct sigaction act, oact;


   /* Register a termination handler. */
   ret = ORPGTASK_reg_term_hdlr(NULL);
   if (ret < 0) {
      LE_send_msg(GL_INFO, "ORPGTASK_reg_term_hdlr failed: (%d)", ret) ;
      LE_send_msg(GL_ERROR, "mngdskerr, Initialization of mngdskerr failed (%d)", ret);
      exit(0) ;
   }


   /* Register a test signal */
   act.sa_handler = sig_handler;
   sigemptyset (&act.sa_mask);
   act.sa_flags = 0;
   act.sa_flags |= SA_RESTART;
   sigaction (SIGUSR1, &act, &oact);

   ret = ORPGMISC_init(argc, argv, 500, 0, -1, 0);

   if (ret < 0) {
      LE_send_msg (GL_ERROR,  "ORPGMISC_init failed (ret %d)", ret);
      exit (1);
   }

   /* Change the default Jaz driver info? */
   if (argc >= 2) {
      strcpy (JAZNAME, argv[1]);
   }
   LE_send_msg (GL_INFO, "   ##########  Start Process ##########\n");
   LE_send_msg (GL_INFO, "JAZNAME = %s \n", JAZNAME);

   /* Change the default hard disk driver info? */
   if (argc >= 3) {
      strcpy (HDNAME, argv[2]);
   }
   LE_send_msg (GL_INFO, "HDNAME = %s \n", HDNAME);

   /* Change the default poll time? */
   if (argc >= 4) {
      poll_time = atol (argv[3]);
   }
   LE_send_msg (GL_INFO, "POLL TIME = %d \n", poll_time);

   do {

     /*
      * Open kstat device. Initialize kernel statistics facility.
      */
      kc = kstat_open();
      if (!kc) {
         LE_send_msg (GL_ERROR, "kstat_open failed \n");
         exit (0);
      } 

      update_kstat (kc);


     /*
      * Gather stats and report any errors.
      */
      get_disk_stats (kc); 
      get_jaz_stats (kc); 

     /*
      * Close kstat. Frees all resources that were associated with kc.
      */ 
      kstat_close (kc);

      sleep ((u_int)poll_time);

   } while (1);

   /* NOTREACHED */

}
/*****************************************************************/
void sig_handler (int sig) {

   LE_send_msg(GL_INFO, "mngdskerr test signal handler  sig=%d",sig) ;
   test_flag = 1;
 
}

/*****************************************************************/
void update_kstat (kstat_ctl_t *kc) {
   kid_t nkcid;

   /*
    *  kstat_chain_update() brings the user's kstat header chain
    *  in sync with the kernel's. The kstat chain is a linked list
    *  of kstat headers (kstat_t's), pointed to by kc->kc_chain,
    *  which is initialized by kstat_open(3K). This chain constitutes
    *  a list of all kstats currently in the system.
    *  During normal operation, the kernel will occasionally create
    *  new kstats and delete old ones, as various device instances
    *  come and go. When this happens, the user's copy of the kstat
    *  chain becomes out of date.
    *  kstat_chain_update() detects this by comparing the kernel's
    *  current kstat chain ID (KCID), which is incremented every time
    *  the kstat chain changes, to the user's KCID, kc->kc_chain_id.
    *  If the KCID's match, kstat_chain_update() does nothing.
    *  Otherwise, it deletes any invalid kstat headers from the
    *  user's kstat chain, and adds any new ones, and sets
    *  kc->kc_chain_id to the new KCID. All other kstat headers in
    *  the user's kstat chain are unmodified.
    */
    do {
       nkcid = kstat_chain_update(kc);
       if (nkcid == -1) {
          LE_send_msg (GL_ERROR, "kstat_chain_update error \n");
          exit (0);
       }
    } while (nkcid != 0 ); 

}

/*****************************************************************/
void get_disk_stats (kstat_ctl_t *kc) {
   kstat_t *ks;
   kstat_named_t *kn;
   static int last_hderr=0;
   static int last_trans_err=0;
   static int ld, lm, li = 0; 
   int hard = 0;
   int soft = 0;
   int trans = 0;
   int err_found = 0;
   char flag;

  /*
   * Get the primary hard drive stats
   */
   ks = kstat_lookup(kc, NULL, -1, HDNAME);

   if (ks == NULL) {
      LE_send_msg (GL_ERROR, "kstat_lookup %s not found \n", HDNAME);
      exit (0);
   } 

   if (kstat_read(kc, ks, 0) == -1) {
     LE_send_msg (GL_ERROR, "kstat_read error \n");
     exit (0);
   }


   kn = kstat_data_lookup(ks, "Hard Errors");
   if (kn || test_flag) {
      hard = kn->value.ui32;
      if ( (kn->value.ui32 > last_hderr) || test_flag ) {
         LE_send_msg (GL_STATUS | GL_ERROR, "Primary Hard Disk Error \n"); /** STATUS MSG ERR **/
         last_hderr = kn->value.ui32;
         err_found = 1;
      }
   }

   kn = kstat_data_lookup(ks, "Soft Errors");
   if (kn) {
      soft = kn->value.ui32;
   }

   kn = kstat_data_lookup(ks, "Transport Errors");
   if (kn || test_flag) {
      trans = kn->value.ui32;
      if ( (kn->value.ui32 > last_trans_err) || test_flag ) {
         LE_send_msg (GL_STATUS | GL_ERROR, "Primary Hard Disk Transport Error \n"); /** STATUS INFO MSG **/
         last_hderr = kn->value.ui32;
         err_found = 1;
      }
   }

   LE_send_msg (GL_INFO, "%s: Hard %d, Soft %d, Transport %d \n", HDNAME, hard, soft, trans);

   if (err_found) {

      kn = kstat_data_lookup(ks, "Illegal Request");
      if (kn || test_flag) {
         if ( (kn->value.ui32 > li) || test_flag ) {
            LE_send_msg (GL_STATUS | GL_INFO,
                          "Primary Hard Disk: Illegal Request = %d \n", kn->value.ui32); /**STATUS INFO MSG**/
            li = kn->value.ui32;
         }
      }

      kn = kstat_data_lookup(ks, "Device Not Ready");
      if (kn || test_flag) {
         if ( (kn->value.ui32 > ld) || test_flag ) {
            LE_send_msg (GL_STATUS | GL_INFO, 
                          "Primary Hard Disk: Device Not Ready = %d \n", kn->value.ui32); /**STATUS INFO MSG**/
            ld = kn->value.ui32;
         }
      }

      kn = kstat_data_lookup(ks, "Media Error");
      if (kn || test_flag) {
         if ( (kn->value.ui32 > lm) || test_flag ) {
            LE_send_msg (GL_STATUS | GL_INFO, 
                          "Primary Hard Disk: Media Error = %d \n", kn->value.ui32); /**STATUS INFO MSG**/
            lm = kn->value.ui32; 
         }
      }

      /* Set the MM Media Error Alarm. Something is wrong with the disk */
      ORPGINFO_statefl_rpg_alarm (ORPGINFO_STATEFL_RPGALRM_MEDIAFL, ORPGINFO_STATEFL_SET, &flag);

   } /* end hard errors */

   /* Clear test condition */
   if (test_flag) {
      test_flag = 0;
   }    


} /* end get_disk_stats */


/*****************************************************************/
void get_jaz_stats (kstat_ctl_t *kc) {
   kstat_t *ks;
   kstat_named_t *kn;
   static int li, ld, lm, lp, ln, lt = 0;
   static int startup = 1; /* ignore any initial Jaz errors of startup */
   int hard = 0;
   int soft = 0;
   int trans = 0;
   int err_found = 0;
   int critical_err = 0;
   int eject_err = 0;
   static int err_cnt = 1; /* Print out error approx every hour */


  /*
   * Get the Jaz drive stats.
   * A field site will have sd19 as the Jaz disk.
   */
   ks = kstat_lookup(kc, NULL, -1, JAZNAME);

   if (ks == NULL) {
      LE_send_msg (GL_INFO, "kstat_lookup %s not found. No Jaz Installed? \n", JAZNAME);
      if ( err_cnt == 1 ) {
        LE_send_msg (GL_ERROR, "kstat_lookup %s not found. No Jaz Installed? \n", JAZNAME);
        err_cnt++;
        if (err_cnt > 240) /* approx every hour */
           err_cnt = 1;
      }
      startup = 0;
      return;
   } 

   if (kstat_read(kc, ks, 0) == -1) {
      LE_send_msg (GL_ERROR, "kstat_read error \n");
      exit (0);
   }


   kn = kstat_data_lookup(ks, "Hard Errors");
   if (kn) {
      hard = kn->value.ui32;
      err_found = 1;
   }
   kn = kstat_data_lookup(ks, "Soft Errors");
   if (kn) {
      soft = kn->value.ui32;
      err_found = 1;
   }

   kn = kstat_data_lookup(ks, "Transport Errors");
   if (kn) {
      trans = kn->value.ui32;
      if (trans > lt) {
         if (!startup) {
            LE_send_msg (GL_STATUS | GL_ERROR,
                        "JAZ: Transport Error (%d)\n", kn->value.ui32); /** STATUS ERR MSG **/
            critical_err = 1; 
         }
         lt = trans;
      }
      err_found = 1;
   }

   LE_send_msg (GL_INFO, "%s: Hard %d, Soft %d, Transport %d \n", JAZNAME, hard, soft, trans);

   if (err_found) {

      kn = kstat_data_lookup(ks, "Illegal Request");
      if (kn) {
         if (kn->value.ui32 > li) {
            if (!startup) {
               LE_send_msg (GL_STATUS | GL_ERROR,
                            "JAZ EJECT: Illegal Request (%d)\n", kn->value.ui32); /** STATUS ERR MSG **/
               eject_err = 1;
            }
            li = kn->value.ui32;
         }
      }

      kn = kstat_data_lookup(ks, "Device Not Ready");
      if (kn) {
         if (kn->value.ui32 > ld) {
            if (!startup) {
               LE_send_msg (GL_INFO,
                             "JAZ: Device Not Ready (%d) \n", kn->value.ui32);
            }
            ld = kn->value.ui32;
         }
      }

      kn = kstat_data_lookup(ks, "Media Error");
      if (kn) {
         if (kn->value.ui32 > lm) {
            if (!startup) {
               LE_send_msg (GL_STATUS | GL_ERROR,
                            "JAZ Media Error = %d \n", kn->value.ui32); /** STATUS ERR MSG **/
               eject_err = 1;
               critical_err = 1; 
            }
            lm = kn->value.ui32;
 
         }
      }

      kn = kstat_data_lookup(ks, "Predictive Failure Analysis");
      if (kn) {
         if (kn->value.ui32 > lp) {
            if (!startup) {
               LE_send_msg (GL_STATUS | GL_ERROR,
                            "JAZ Predictive Failure Error = %d \n", kn->value.ui32); /** STATUS ERR MSG **/
               critical_err = 1; 
            } 
            lp = kn->value.ui32;
 
         }
      }

      kn = kstat_data_lookup(ks, "No Device");
      if (kn) {
         if (kn->value.ui32 > ln) {
            if (!startup) {
               LE_send_msg (GL_STATUS | GL_ERROR,
                             "JAZ No Device Error = %d \n", kn->value.ui32); /** STATUS ERR MSG **/
               critical_err = 1; 
            }
            ln = kn->value.ui32;
         }
      }

   } /* end errors found */

   if (eject_err && !startup) {
      system ("eject_ar3");
      LE_send_msg (GL_STATUS | GL_INFO, "JAZ ERROR: Bad Disk or Drive");
      LE_send_msg (GL_INFO, "Eject Error: Called eject_ar3 \n");
   }


   if (critical_err && !startup) {
#ifdef SND_ARIII_SIG
      /* Send archiveIII process a signal to halt */
      /* A task failure will generate a Maintenace Required Alarm */
      system ("/usr/bin/pkill -15 archiveIII");
      LE_send_msg (GL_INFO, "Sent archiveIII process a terminate signal. \n");
#endif
      system ("eject_ar3");
      LE_send_msg (GL_INFO, "Critical Error: Called eject_ar3 \n");
      LE_send_msg (GL_STATUS | GL_INFO, "JAZ ERROR: Bad Disk or Drive");
   }

   startup = 0;
   critical_err = 0;
   eject_err = 0;

} /* end get_jaz_stats */

