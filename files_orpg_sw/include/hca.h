/*
 */

#ifndef _HCA_H
#define _HCA_H


#define MLDA	        328

/* Hydrometeor classes. */
#define NUM_CLASSES	   14 /* Total number of classes used external to HCA */

#define U0     0    /* unused by HCA         */
#define U1     1    /* unused by HCA         */
#define RA	2	/* light or moderate rain*/
#define HR	3	/* heavy rain			*/
#define RH	4	/* rain and hail		*/
#define BD	5	/* big drops			*/
#define BI	6	/* biological			*/
#define GC	7	/* ground clutter		*/
#define DS	8	/* dry snow			*/
#define WS	9	/* wet snow			*/
#define IC	10	/* ice crystals		*/
#define GR	11	/* graupel			*/
#define UK	12	/* unknown			*/
#define NE	13	/* no echo			*/

/* Miscelaneous Definitions */
#define INVALID_CLASS	-1.0	/* Used to prevent an unrealistic class assignment */
#define HCA_NO_DATA      -1.e5f
#define HCA_RF_DATA      -2.e5f
#define MAX_DATA_VALUE    255
#define MAX_ML_AZ         400
#define C0                0.f
#define Cp5               0.5f
#define HCA_SCALE         1.f
#define HCA_OFFSET        0.f
#define RF_FLAG           1
#define ZDR_SCALE        16.f
#define ZDR_OFFSET      128.f
#define RHO_SCALE       300.f
#define RHO_OFFSET      -60.f
#define KDP_SCALE        20.f
#define KDP_OFFSET       43.f
#define PHI_SCALE         2.8361f
#define PHI_OFFSET        2.f
#define ML_SCALE          1.f
#define ML_OFFSET         0.f
#define MLTT              0      /* Index into data.ml for melting layer top    */
#define MLT               1      /* Index into data.ml for melting layer top    */
#define MLB               2      /* Index into data.ml for melting layer top    */
#define MLBB              3      /* Index into data.ml for melting layer bottom */


/*  Melting Layer */
typedef struct {
    float r_bb;
    float r_b;
    float r_tt;
    float r_t;
} ML_r_t;

typedef struct {
    int   bin_bb;
    int   bin_b;
    int   bin_tt;
    int   bin_t;
} ML_bin_t;

	

#endif
