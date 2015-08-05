#ifndef PRF_BITMAP_H
#define PRF_BITMAP_H

#include <a309.h>
#include <basedata.h>
#include <vcp.h>
#include <rpgc.h>
#include <rpgcs.h>
#include <siteadp.h>
#include <prfbmap.h>

#define MAXBINS				   230
#define MAXB230				   229
#define EPWR_SIZE			   (4*MAXBINS)
#define PREFSIZE			   460
#define FLAG_NO_PWR			   -9999.0f
#define DOP_PRF_BEG			   4
#define DOP_PRF_END			   8
#define MAX_FOLD_BIN			   (MAXBINS-1)
#define DEF_Z_SNR			   2.0f
#define DEF_V_SNR			   3.5f

#ifdef PRFBMAP_MAIN_C
Vol_stat_gsm_t Vol_stat;        	   /* Volume Status. */
#endif

#ifdef PRFBMAP_BUFFER_CONTROL_C
int Endelcut;				   /* End of elevation flag. */
int PS_rpgvcpid;			   /* VCP ID. */
int Allowable_prfs[PRFMAX+1];		   /* Allowable PRFs this VCP */ 
int Num_alwbprfs;		 	   /* Number of allowable PRFS this VCP. */
int Is_SZ2;				   /* Flag if set indicates SZ2 VCP. */
float Z_snr;				   /* Z SNR threshold, from VCP definition. */
float V_snr;				   /* V SNR threshold, from VCP definition. */

extern Vol_stat_gsm_t Vol_stat; 	   /* Volume Status. */
#endif


#ifdef PRFBMAP_PROD_GEN_C
int Radcount;				   /* Radial count this volume. */
int PS_delta_pri;			   /* Delta PRI. */	
float Tover;			 	   /* Overlay Margin, Tover. */	
unsigned short Overlaid[DOP_PRF_END][MAXBINS];

static short Folded_bin1[DOP_PRF_END][MAXBINS];
static short Folded_bin2[DOP_PRF_END][MAXBINS];
static short Folded_bin3[DOP_PRF_END][MAXBINS];
static int Validprf[DOP_PRF_END];

extern int Endelcut;			   /* End of elevation flag. */
extern int Allowable_prfs[PRFMAX+1];	   /* Allowable PRFs this VCP */ 
extern int Num_alwbprfs;		   /* Number of allowable PRFS this VCP. */
extern int Is_SZ2;			   /* Flag if set indicates SZ2 VCP. */
#endif

#ifdef PRFBMAP_SZ2_C
extern int PS_delta_pri;                   /* Delta PRI. */
extern unsigned short Overlaid[DOP_PRF_END][MAXBINS];
extern float Z_snr;			   /* Z SNR threshold, from VCP definition. */
extern float V_snr;			   /* V SNR threshold, from VCP definition. */
#endif



/* Function Prototypes. */
void A30541_buffer_control();
void A30549_product_generation_control( char *bufptr, char *bdataptr,
                                        char *scrptr );
int SZ2_echo_overlay( char *bdataptr, float *epwr, float *pwr_lookup );
int SZ2_read_clutter();



#endif
