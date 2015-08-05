/* RCS info */
/* $Author: ryans $ */
/* $Locker:  $ */
/* $Date: 2007/06/26 16:10:55 $ */
/* $Id: rcm.h,v 1.1 2007/06/26 16:10:55 ryans Exp $ */
/* $Revision: 1.1 $ */
/* $State: Exp $ */

#include <rpgcs.h>
#include <rpg_port.h>
#include <a309.h>
#include <mda_adapt.h>
#include <a3146.h>
#include <prodsel.h>
#include <siteadp.h>
#include <a308buf.h>
#include <itc.h>
#include <rdacnt.h>
#include <orpgsum.h>
#include <alg_adapt.h>
#include <a313h.h>
#include <a313hparm.h>
#include <coldat.h>
#include <a313buf.h>
#include <a315buf.h>
#include <a317buf.h>
#include <string.h>
#include <gen_stat_msg.h>
#include <orpgda.h>
#include <orpgdat.h>

/* Macro function - find minimum value between two numbers */
#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))

/* Macro defs */
#define RCM_FALSE         0
#define RCM_TRUE          1
#define RCM_SUCCESS       0  /* Generic function return value (success) */
#define RCM_FAILURE       1  /* Generic function return value (failure) */
#define RCM_IN_OPT_WAIT  60  /* Wait time (sec) for optional inputs */
#define RCM_BTH           0  /* Biased refl value, corresponds to < 12 dBZ */
#define RCM_PGBUF         0  /* Identifies Polar Grid input buffer */
#define RCM_CABUF         1  /* Identifies Combined Attributes input buffer */
#define RCM_ETBUF         2  /* Identifies Echo Tops input buffer */
#define RCM_VWBUF         3  /* Identifies VAD Winds input buffer */
#define RCM_HYBUF         4  /* Identifies Hybrscan input buffer */
#define RCM_RADBUF        5  /* Identifies RCM output buffer */
#define RCM_SCRBUF        6  /* Identifies RCM scratch buffer */
#define RCM_NIBUFS        5  /* Number of input buffers */
#define RCM_NIOBUFS       7  /* Number of input/output buffers */
#define RCM_NAROWS       17  /* Num of rows per page available for RCM prod */
#define RCM_MAX_STORM    20  /* Max number of storms */
#define RCM_NACOLS       70  /* Number of cols available for RCM product */
#define RCM_NROWS       100  /* Number of rows in the RCM LFM Grid */
#define RCM_NCOLS       100  /* Number of cols in the RCM LFM Grid */
#define RCM_END          -1  /* Flag indicating that the RCM encoding is
                                being done at either end of a LFM grid row */
#define RCM_MID           1  /* Flag like "RCM_END" but meaning we're 
                                in the middle of a LFM grid row */
#define RCMIGP           20  /* Index into the color level table COLDAT for all
                                the color tables in the RPG: Radar coded
                                message 9 level (int. Graphic product). */
#define RCM_MAXIND    10000  /* Maximum index for A3CM22 routine */
#define RCM_RNG_MAX   230.0  /* Max range (km) for RCM product */
#define RCM_NBINS       460  /* Number of bins */
#define RCM_NC           19  /* RAD.CODED MSG 7 LEVEL NON CL AIR */
#define RCM_RADTODEG 57.2958 /* RADIAN TO DEGREE CONVERSION FACTOR */
#define RCM_ADDAZ     360.0  /* VALUE TO ADD TO NEGATIVE AZIMUTH */
#define RCM_HUNDFT    100.0  /* CONSTANT VALUE 100 */
#define RCM_SBON          1  /* Spot Blanking flag; 1=enabled in product */
#define RCM_NUM4          4  /* Program parameter for the constant 4 */
#define RCM_OFF1          1  /* Constant respresenting offset 1 */
#define RCM_OFF2          2  /* Constant respresenting offset 2 */
#define RCM_OFF3          3  /* Constant respresenting offset 3 */
#define RCM_OFF4          4  /* Constant respresenting offset 4 */
#define RCM_PHBYTES     120  /* Number of bytes in product header */
#define RCM_BYTES_PER_HW  2  /* NUMBER OF BYTES IN I*2 WORD */
#define RCM_NI2WDS_LINE  35  /* NUMBER OF I*2 WORDS IN LINE */
#define RCM_LEN_LETTERS   3  /* Length of 3 grid letters */
#define RCM_NRADS     NRADS  /* Number of radials */
#define RCM_DIVIDER      -1  /* Product Block/Layer divider */
#define RCM_MAXSZ        40  /* Max number of mesos, used in a3082i */



/* Global structures */
typedef struct {
   int count_int; /* Counter for number of intensities */
   int vb[RCM_NIOBUFS]; /* Array for holding "valid buffer status" values */
   int rcmoff;    /* Offset in Part A for RADNE (no reportable reflectivity
                     intensity values) or RADOM (radar down for maintenance) */
   int opmodeoff; /* Offset in Part A for operational mode */
   int rcmidx;    /* Current hw offset into RCM outbuf (not includ hdr) */
   void* tibf[RCM_NIBUFS]; /* Table of input buffer pointers into mem */
   int save_byte; /* Saved byte value for offsets */
   int msg_siz1;  /* Length of output RCM message */
} Rcm_params_t;


/* Global variables */
Rcm_params_t Rcm_params;
a314c1_t Lfm_parms;
Prodsel_t Prod_sel;      /* Product Selectable Parameters Adaptation Data */
Rdacnt RDA_cntl;         /* RDA Control Adaptation Data */
mda_adapt_t Mda_adapt;   /* Mesocyclone Detection Algorithm Adaptation Data */
rcm_prod_params_t Rcm_adapt; /* Radar Coded Message Adaptation Data */
Siteadp_adpt_t Siteadp;  /* Site Adaptation Data */
Scan_Summary *Summary;   /* Pointer to Vol eScan Summary Data (see orpgsum.h) */
combattr_t *Combattr;    /* Combined Attribute structure data */
Echo_top_params_t *Etop_parms; /* Echo top auxiliary parms data structure */
Vad_params_t *Vad_parms; /* VAD parms data structure */
Coldat_t Colrtbl;        /* Product color table data */



/* Prototypes */
int a30821_buffer_control();
int a3082u_get_inbufs();
int a3082l_rcm_driver(void* optr, void* optr_sc, int voln, int cdate, int ctime);
int a30829_cnvtime(int* seconds, int* hmtime);
int a30822_rcm_grid(short* polgrid, short* rcmgrid, short* hybrscan);
int a3082n_header_inter(int cat_num_storms, int* cat_feat, float* comb_att,
   int num_fposits, float* forcst_posits, float* cat_tvst, short* bufout,
   int gendate, int gentime, int volnum, short* rcmgrid);
int a3082m_comm_line(int* irow, int* icol, int* nbytes, short* rcmbuf);
int a30823_rcm_control(short* split_date, int* hmtime, int* irow, int* icol,
   int voln, int* nbytes, short rcmgrid[][RCM_NCOLS], short* rcmbuf);
int a3082c_max_echotop(Echo_top_params_t *etpar, int *irow, int *icol,
   int *nbytes, short *rcmbuf);
int a3082v_buffer_notavail(int func, int nbytes, int irow, int icol, short* rcmbuf);
int a3082d_centroids_parta(int cat_num_storms, int *cat_feat, float *combatt,
   int num_fposits, float *forcst_posits, float *cat_tvst, int *irow, int *icol,
   int *nbytes, short *rcmbuf);
int a3082e_partbc_header(int ipart, short split_date[], int hmtime, int irow,
   int icol, int nbytes, short* rcmbuf);
int a3082f_vad_winds(float vad_data_hts[][VAD_HT_PARAMS], int irow, int icol,
   int nbytes, short* rcmbuf);
int a3082h_tvs(int cat_num_storms, int* cat_feat, float* comb_att,
   int num_fposits, float* forcst_posits, int* cat_num_rcm, float* cat_tvst,
   int irow, int icol, int nbytes, short* rcmbuf);
int a3082i_mesocyclones(int cat_num_storms, int* cat_feat, float* comb_att,
   int num_fposits, float* forcst_posits, int* cat_num_rcm,
   float cat_mdat[][CAT_NMDA], int irow, int icol, int nbytes, short* rcmbuf);
int a3082j_centroids_partc(int cat_num_storms, int *cat_feat, float *comb_att,
   int num_fposits, float *forcst_posits, float *cat_tvst, int irow, int icol,
   int nbytes, short* rcmbuf);
int a30824_header(short* bufout, int gendate, int gentime, int volnum,
   int prod_bytes);
int a3082o_header_layers(int cat_num_storms, int *cat_feat, float *comb_att,
   int num_fposits, float *forcst_posits, float *cat_tvst, int len_lay1,
   short *bufout, short *rcmgrid);
void a3082k_store_line(int* irow, int* icol, int* nbytes, char* rcm_line,
   short* rcmbuf);
int a3082b_parta_header(short* split_date, int* hmtime, int* irow, int* icol,
   int nvol, int* nbytes, short *rcmbuf);
int a30825_rcm_encode(int* ist, int* jst, int* lbl, int* pos, int* run_vip,
   int* reps, int* irow, int* icol, int* nbytes, char* rcmbuf);
int a30826_blank_pad(int* irow, int* icol, int* nbytes, char* rcmbuf);
int a3082g_get_ij(float* azim, float* range, float* elev, int* lfm_i,
   int* lfm_j);
void a30828_grid_letters(int i_loc, int j_loc, char* ret_str);
int a3082q_process_line(int* irow, int* icol, int* nbytes, int* is, int* ic,
   int* increm, int* lend, char* rcm_line, short* rcmbuf);
int a3082t_vad_convert(float* height, float* rconf, float* vdir, float* vspd,
   int* iheight, char* confval, int* idir, int* ispd);
int a3082s_packet_cent(int cat_num_storms, int *cat_feat, float *comb_att,
   int num_fposits, float *forcst_posits, float *cat_tvst, short *bufout);
int a3082a_compact_rcm(int *reps, int *run_vip, int *irow, int *icol,
   int *nbytes, char *rcmbuf);
int a3082p_compact_again(int* run_vip, int* nbytes, int* num, int* rem,
   char *rcmbuf);
void rcm_read_rdastatus_lb( short *rda_status, int *status );
void RCM_strip_off_graphic( char *Rcm, int *status );
