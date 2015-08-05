/*   @(#)mlp_control.h	1.1	07 Jul 1998	*/

/*
 * Copyright (c) 1988-1997 Spider Software Limited
 *
 * This Source Code is furnished under Licence, and may not be
 * copied or distributed without express written agreement.
 *
 * All rights reserved.  Made in Scotland.
 *
 * Authors: Alan Robertson, Peter Woodhouse, Duncan Walker, Jim Stewart
 *
 * mlp_control.h of snet module
 *
 * SpiderX25
 * @(#)$Id: mlp_control.h,v 1.1 2000/02/25 17:14:36 john Exp $
 * 
 * SpiderX25 Release 8
 */

#define MLP_STID	215

/* IOCTL commands */
#define M_SETSNID    ('M'<<8 | 1) /* Set subnetwork identifier (use ll_snioc) */
#define M_GETSNID    ('M'<<8 | 2) /* Get subnetwork identifier (use ll_snioc) */
#define M_SETTUNE    ('M'<<8 | 3) /* Set tuning parameters (use ll_tnioc)     */
#define M_GETTUNE    ('M'<<8 | 4) /* Get tuning parameters (use ll_tnioc)     */
#define M_GETSTATS   ('M'<<8 | 5) /* Get statistics counts (use ll_stioc)     */
#define M_ZEROSTATS  ('M'<<8 | 6) /* Zero statistics       (use ll_hdioc)     */
#define M_TRACEON    ('M'<<8 | 7) /* Set message tracing on.                  */
#define M_TRACEOFF   ('M'<<8 | 8) /* Set message tracing off.                 */
#define M_GETGSTATS  ('M'<<8 | 10) /* Get global stats counts (use ll_gstioc) */
#define M_ZEROGSTATS ('M'<<8 | 11) /* Zero global statistics		      */


#define LI_MLPTUNE	0x33        /* Indicates 'struct mlp_tnioc'*/


/* MLP tuning structure */
typedef struct mlptune {
    uint16   mw;		/* Size of MLP window			*/
    uint16   mx;		/* Size of MLP guard region		*/
    uint16   mt1;		/* Time to wait for MN(S) = MV(R)	*/
    uint16   mt2;		/* Time to wait for unblock		*/
    uint16   mt3;		/* Time to wait for Reset Confirmation	*/
    uint16   mn1;		/* Number of SLP tx retries		*/
} mlptune_t;


/* Ioctl block for MLP L_SETTUNE and L_GETTUNE commands */
struct mlp_tnioc {
    uint8           lli_type;      /* Table type = LI_MLPTUNE		  */
    uint8           lli_spare[3];  /*   (for alignment)			  */
    uint32          lli_snid;      /* Subnetwork ID character ('*' => 'all') */

    mlptune_t       mlp_tune;     /* Table of tuning values		  */
};

/* Statistics table definitions 	*/
#define	mlpstatmax	12

/* Global statistics			*/
#define	mlpglobstatmax	10

#define MLP_frames_tx	0
#define MLP_frames_rx	1
#define MLP_reset_tx	2
#define MLP_reset_rx	3
#define MLP_confs_tx	4
#define MLP_confs_rx	5
#define MLP_slps	6
#define MLP_num_slps	7
#define MLP_mt1_exp	8
#define MLP_mt2_exp	9
#define MLP_mt3_exp	10
#define MLP_mn1_exp	11
#define MLP_bytes_tx	8
#define MLP_bytes_rx	9


typedef struct mlp_stats {
	uint32	mlpmonarray[mlpstatmax]; /* array of MLP stats */
} mlpstats_t;


struct mlp_stioc {
    uint8           lli_type;   /* Table type = LI_STATS		*/
    uint8	    state;      /* connection state			*/
    uint16          lli_spare;  /*   (for alignment)			*/
    uint32          lli_snid;   /* Subnetwork ID character		*/

    mlpstats_t      lli_stats;  /* Table of stats values		*/
};


struct mlp_gstioc {
    uint8           lli_type;      /* Table type = LI_GSTATS		*/
    uint8           lli_spare[3];  /*   (for alignment)			*/
    uint32	    mlpgstats[mlpglobstatmax];
				   /* global statistics table           */
};

/* Union of ioctl blocks */
typedef union mlp_union {
    struct ll_hdioc    ll_hd;     /* Parameter-less command       	*/
    struct ll_snioc    ll_sn;     /* Set/get subnetwork identifier	*/
    struct mlp_tnioc   mlp_tn;    /* Set/get LAPB tuning          	*/
    struct mlp_stioc   mlp_st;    /* Get lapb per-link statistics   	*/
    struct mlp_gstioc  mlp_gst;   /* Get lapb global statistics     	*/
} mlpiun_t;
