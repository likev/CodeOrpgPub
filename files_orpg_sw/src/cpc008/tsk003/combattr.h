/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2006/12/18 20:25:18 $
 * $Id: combattr.h,v 1.2 2006/12/18 20:25:18 steves Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

#ifndef COMBATTR_H
#define COMBATTR_H

#include <rpgc.h>
#include <rpgcs.h>
#include <a308buf.h>
#include <a315buf.h>
#include <a317buf.h>
#include <a318buf.h>
#include <mda_adapt.h>
#include <alg_adapt.h>

#define NIBUFS		5
#define NOT_AVAIL	-1
#define AVAIL		1

#define MDATTNN_ID	0
#define CENTATTR_ID	1
#define TRFRCATR_ID	2
#define HAILATTR_ID	3
#define TVSATTR_ID	4

#define CAT_OBUF_SIZE	((CNMDA+CAT_NMDA*CAT_MXSTMS)*sizeof(int))

/* Global variables. */
int *Tibuf[NIBUFS];		/* Array of input buffer pointers. */

int Bufmap[NIBUFS];		/* Array of data IDs input to this task. */

int Storm_feats[CATMXSTM][FEAT_TYPES];
				/* Storm features. */

float Basechars[CATMXSTM][NBCHAR];
				/* ???????? */

mda_adapt_t Mda_adapt;

/* Function Prototypes. */
int A30831_buffer_control();

int A30832_comb_att( void *optr );

int A30834_el_az_ran( float range, float height, float *elevation);

int A30835_bld_outbuf( int *outbuf, float hailstats[][NHAL_STS], 
                       float stormain[][NSTM_CHR] );

int A30836_pack_storm( int sidx, int cat_idx, float stormain[][NSTM_CHR],
                       int ntotpred, int stormidtyp[][NSTF_IDT],
                       float stormotion[][NSTF_MOT], 
                       float stormforw[][NSTF_INT][NSTF_FOR],
                       int *forcadap, int *cat_num_storms, int cat_feat[][CAT_NF],
                       float comb_att[][CAT_DAT], int *num_fposits,
                       float forcst_posits[][MAX_FPOSITS][FOR_DAT],
                       int *hailabel, float hailstats[][NHAL_STS] );

int A30837_fill_cat( int num_tvs, int num_etvs, float tvs_main[][TVFEAT_CHR],
                     int *cat_num_rcm, float cat_tvst[][CAT_NTVS] );

int A30838_correl_tvs( float stormotion[][NSTF_MOT], int ntotpred,
                       int num_tvs, int num_etvs, float tvs_main[][TVFEAT_CHR],
                       float *tda_adapt );

int correlate_mda_features( tracking_t *ptrTr, int *ptrMdattnn,
                            float cat_mdat[][CAT_NMDA],
                            int *cat_num_rcm );


# endif
