/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2012/03/12 13:17:01 $
 * $Id: hca_local.h,v 1.6 2012/03/12 13:17:01 ccalvert Exp $
 * $Revision: 1.6 $
 * $State: Exp $
*/

#ifndef HCA_LOCAL_H
#define HCA_LOCAL_H

#include <hca.h>
/* Membership function definition points        */
#define NUM_X   4
#define X1      0
#define X2      1
#define X3      2
#define X4      3


#define MAXRADIALS	360
#define BLOCK_RNG       230

/* Fuzzy logic input variables */
#define NUM_FL_INPUTS   6
#define SMZ     0       /* smoothed reflectivity (dBZ)                          */
#define ZDR     1       /* differential reflectivity (dB)               */
#define LKDP    2       /* log of specific differential phase (deg/km)  */
#define RHO     3       /* cross-correlation coefficent (unitless)      */
#define SDZ     4       /* smoothed difference (texture) of reflectivity (unitless)     */
#define SDP     5       /* smoothed difference (texture) of total differential phase (unitless) */


/* Two-dimensional membership flag values.  These must match the enumerations in dea/hca.alg */
#define MEMFLAG_NONE    0       /* Not a two-dimensional membership     */
#define MEMFLAG_F1      1       /* Use the f1 equation                  */
#define MEMFLAG_F2      2       /* Use the f2 equation                  */
#define MEMFLAG_F3      3       /* Use the f3 equation                  */
#define MEMFLAG_G1      4       /* Use the g1 equation                  */
#define MEMFLAG_G2      5       /* Use the g2 equation                  */

/* Constants that define the first and last of the hydrometeor classes  */
/* that are determined by fuzzy logic and use the membership functions. */
/* The first two classes defined in hca.h are place holders and the     */
/* last two are special cases.                                          */
#define FIRST_FL_CLASS RA       /* first fuzzy logic class from hca.h   */
#define LAST_FL_CLASS  GR       /* last fuzzy logic class from hca.h    */                        

/* Two dimensional membership function equation look-up tables. */
   float f1[MAX_DATA_VALUE];    /* f1 equation values */
   float f2[MAX_DATA_VALUE];    /* f2 equation values */
   float f3[MAX_DATA_VALUE];    /* f3 equation values */
   float g1[MAX_DATA_VALUE];    /* g1 equation values */
   float g2[MAX_DATA_VALUE];    /* g2 equation values */

/* Three dimensional convenience arrays that map directly to the
   one dimensional arrays named mem** and mem2** in dea data.
   These hold the membership defintions points for each hydro
   class and input variable type.                             */
   float membership[NUM_CLASSES][NUM_FL_INPUTS][NUM_X];
   int   membershipFlag[NUM_CLASSES][NUM_FL_INPUTS][NUM_X];
   float weight[NUM_FL_INPUTS][NUM_CLASSES];

/* Beam Blockage array.  NA11-00387  */
   char  Beam_Blockage[MAXRADIALS*10][BLOCK_RNG];
   int   Min_blockage_thresh;

/*  Weighted Membership Aggregation array */
   float agg[NUM_CLASSES];

/* Note: Melting layer data is in bin number units. A floating point data type is used only*
     *       for compatibility with the Add_moment */
typedef struct {         /* structure for HCA input, output and internal radial data */
    float *hca;          /* Hydrometeor classification */
    float *ml;           /* Melting layer top and bottom */
    float *smz;          /* Smoothed reflectivity */
    float *zdr;          /* Differential reflectivity */
    float *rho;          /* Correlation coefficient */
    float *kdp;          /* Specific differential phase */
    float *lkdp;         /* Log base 10 of specific differential phase (internal to HCA) */
    float *sdz;          /* Texture of reflectivity */
    float *sdp;          /* Texture of differential phase */
    float *phi;          /* Differential phase */
    float *snr;          /* Signal-to-noise ratio */
    float *smv;          /* Smoothed velocity */
    float *qsmz;         /* Quality index for smoothed reflectivity */
    float *qzdr;         /* Quality index for differential reflectivity */
    float *qkdp;         /* Quality index for specific differential phase */
    float *qrho;         /* Quality index for correlation coefficient */
    float *qsdz;         /* Quality index for texture of reflectivity */
    float *qsdp;         /* Quality index for texture of differential phase */
} Hca_data_t;


typedef struct {                /* parameters extracted from input quality index algorithm */
    float   zr0;                /* range of the center of the first sample volume (km) */
    int     n_zgates;   /* number of Z sample volumes in the radial */
    float   zg_size;    /* Z sample volume size (km) */
    float   vr0;             /* Doppler range 0 (km) */
    int     n_vgates;   /* number of V sample volumes in the radial */
    float   vg_size;    /* V sample volume size (km) */
    float   dr0;             /* dual pol range 0 (km) */
    int     n_dgates;   /* number of dual pol sample volumes in the radial */
    float   dg_size;    /* dual pol sample volume size (km) */
} Hca_params_t;


/* Function Prototypes. */
void  MemLookup();
void  DefineMembershipFuncsAndWeights();
void  Hca_beamMLintersection( float elev, int azimuth, float bin_size, float top[MAXRADIALS], 
                              float bottom[MAXRADIALS],ML_bin_t *Melting_layer);
void  Hca_allowedHydroClass(int, float, float, float, float, float, int, float agg[NUM_CLASSES], 
                            ML_bin_t Melting_layer);
float Hca_degreeMembership(float D, float points[NUM_X]);
void  Hca_setMembershipPoints(int    h_class, /* (IN) Hydrometeor class    */
                             int    fl_input, /* (IN) Fuzzy logic input    */
                           /*  int    Z_data,*/   /* (IN) Indexed Reflectivity */
                             float  z_fshield,/* (IN) F-shield adjusted reflectivity */
                             float  points[]  /* (OUT) Membership points   */);
int   Hca_buffer_control(void);
int   Hca_process_radial(char *input, char **output, int *length, float top[MAXRADIALS], float bottom[MAXRADIALS]);
float quick_select(float arr[], int num);
void  read_Blockage(int elev_angle_tenths, char Beam_Blockage[MAXRADIALS*10][BLOCK_RNG]);





#endif /* HCA_LOCAL_H */
