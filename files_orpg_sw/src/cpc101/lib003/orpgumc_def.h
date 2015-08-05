/* 
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2006/01/25 22:25:10 $
 * $Id: orpgumc_def.h,v 1.17 2006/01/25 22:25:10 ryans Exp $
 * $Revision: 1.17 $
 * $State: Exp $
 */  
/**************************************************************************

    Module: orpgumc_def.h ( User Message Conversion )

    Description: This is the private header file for the orpgumc.c. 

***************************************************************************/
 

#include <a309.h>

#ifndef ORPGUMC_DEF_H
#define ORPGUMC_DEF_H

#define UMC_MAX_PROD_CODE	LEGACY_MAX_BUFFERNUM

#define ALERT_THRESHOLD_OFF 14
#define ALERT_EXCEEDING_OFF 16
#define ALERT_CELL_ID_OFF   20
#define ALERT_VOL_TIME_OFF  23

/* Message Code Macros */
#define MSG_CODE_ALERT                 9
#define MSG_CODE_STORM_STRUCTURE      62
#define MSG_CODE_USER_ALERT           73
#define MSG_CODE_RCM                  74
#define MSG_CODE_FREE_TEXT            75
#define MSG_CODE_SUPP_PRECIP_DATA     82
#define MSG_CODE_US_LAYER_CR         137

#define MAX_LIST_PROD_CODE    100
#define MAX_LIST_PARAMS		5
/* This table maps the 6 parameters in a product request message (Table II-A) 
   to the 5 parameters in the product list message (Table X) 
   34 and 84, following Ming's function, are different from my Table X */
static signed char List_params_index [MAX_LIST_PROD_CODE][MAX_LIST_PARAMS] = {
    {	-1,	-1,	-1,	-1,	-1},		/* pcode 0 */
    {	-1,	-1,	-1,	-1,	-1},		/* pcode  */
    {	-1,	-1,	-1,	-1,	-1},		/* pcode  */
    {	-1,	-1,	-1,	-1,	-1},		/* pcode  */
    {	-1,	-1,	-1,	-1,	-1},		/* pcode  */
    {	-1,	-1,	-1,	-1,	-1},		/* pcode 5 */
    {	-1,	-1,	-1,	-1,	-1},		/* pcode  */
    {	-1,	-1,	-1,	-1,	-1},		/* pcode  */
    {	-1,	-1,	-1,	-1,	-1},		/* pcode  */
    {	-1,	-1,	-1,	-1,	-1},		/* pcode  */
    {	-1,	-1,	-1,	-1,	-1},		/* pcode 10 */
    {	-1,	-1,	-1,	-1,	-1},		/* pcode  */
    {	-1,	-1,	-1,	-1,	-1},		/* pcode  */
    {	-1,	-1,	-1,	-1,	-1},		/* pcode  */
    {	-1,	-1,	-1,	-1,	-1},		/* pcode  */
    {	-1,	-1,	-1,	-1,	-1},		/* pcode  */
    {	2,	-1,	-1,	-1,	-1},		/* pcode 16 */
    {	2,	-1,	-1,	-1,	-1},		/* pcode 17 */
    {	2,	-1,	-1,	-1,	-1},		/* pcode 18 */
    {	2,	-1,	-1,	-1,	-1},		/* pcode 19 */
    {	2,	-1,	-1,	-1,	-1},		/* pcode 20 */
    {	2,	-1,	-1,	-1,	-1},		/* pcode 21 */
    {	2,	-1,	-1,	-1,	-1},		/* pcode 22 */
    {	2,	-1,	-1,	-1,	-1},		/* pcode 23 */
    {	2,	-1,	-1,	-1,	-1},		/* pcode 24 */
    {	2,	-1,	-1,	-1,	-1},		/* pcode 25 */
    {	2,	-1,	-1,	-1,	-1},		/* pcode 26 */
    {	2,	-1,	-1,	-1,	-1},		/* pcode 27 */
    {	2,	-1,	-1,	-1,	-1},		/* pcode 28 */
    {	2,	-1,	-1,	-1,	-1},		/* pcode 29 */
    {	2,	-1,	-1,	-1,	-1},		/* pcode 30 */
    {	-1,	0,	1,	-1,	-1},		/* pcode 31 */
    {	-1,	-1,	-1,	-1,	-1},		/* pcode  */
    {	-1,	-1,	-1,	-1,	-1},		/* pcode  */
    {	-1,	0,	-1,	-1,	-1},		/* pcode 34 */
    {	-1,	-1,	-1,	-1,	-1},		/* pcode  */
    {	-1,	-1,	-1,	-1,	-1},		/* pcode  */
    {	-1,	-1,	-1,	-1,	-1},		/* pcode  */
    {	-1,	-1,	-1,	-1,	-1},		/* pcode  */
    {	-1,	-1,	-1,	-1,	-1},		/* pcode  */
    {	-1,	-1,	-1,	-1,	-1},		/* pcode 40 */
    {	-1,	-1,	-1,	-1,	-1},		/* pcode 41 */
    {	-1,	5,	-1,	-1,	-1},		/* pcode 42 */
    {	2,	0,	1,	-1,	-1},		/* pcode 43 */
    {	2,	0,	1,	-1,	-1},		/* pcode 44 */
    {	2,	0,	1,	-1,	-1},		/* pcode 45 */
    {	2,	0,	1,	-1,	-1},		/* pcode 46 */
    {	-1,	-1,	-1,	-1,	-1},		/* pcode  */
    {	-1,	-1,	-1,	-1,	-1},		/* pcode  */
    {	2,	0,	1,	-1,	-1},		/* pcode 49 */
    {	-1,	0,	1,	2,	3},		/* pcode 50 */
    {	-1,	0,	1,	2,	3},		/* pcode 51 */
    {	-1,	0,	1,	2,	3},		/* pcode 52 */
    {	-1,	0,	1,	3,	4},		/* pcode 53 */
    {	-1,	-1,	-1,	-1,	-1},		/* pcode  */
    {	2,	0,	1,	3,	4},		/* pcode 55 */
    {	2,	-1,	-1,	3,	4},		/* pcode 56 */
    {	-1,	-1,	-1,	-1,	-1},		/* pcode  */
    {	-1,	-1,	-1,	-1,	-1},		/* pcode  */
    {	-1,	-1,	-1,	-1,	-1},		/* pcode  */
    {	-1,	-1,	-1,	-1,	-1},		/* pcode 60 */
    {	-1,	-1,	-1,	-1,	-1},		/* pcode  */
    {	-1,	-1,	-1,	-1,	-1},		/* pcode  */
    {	-1,	-1,	-1,	-1,	-1},		/* pcode  */
    {	-1,	-1,	-1,	-1,	-1},		/* pcode  */
    {	-1,	-1,	-1,	-1,	-1},		/* pcode  */
    {	-1,	-1,	-1,	-1,	-1},		/* pcode  */
    {	-1,	-1,	-1,	-1,	-1},		/* pcode  */
    {	-1,	-1,	-1,	-1,	-1},		/* pcode  */
    {	-1,	-1,	-1,	-1,	-1},		/* pcode  */
    {	-1,	-1,	-1,	-1,	-1},		/* pcode 70 */
    {	-1,	-1,	-1,	-1,	-1},		/* pcode  */
    {	-1,	-1,	-1,	-1,	-1},		/* pcode  */
    {	-1,	-1,	-1,	-1,	-1},		/* pcode  */
    {	-1,	-1,	-1,	-1,	-1},		/* pcode  */
    {	-1,	-1,	-1,	-1,	-1},		/* pcode  */
    {	-1,	-1,	-1,	-1,	-1},		/* pcode  */
    {	-1,	-1,	-1,	-1,	-1},		/* pcode  */
    {	-1,	-1,	-1,	-1,	-1},		/* pcode  */
    {	-1,	-1,	-1,	-1,	-1},		/* pcode  */
    {	-1,	-1,	-1,	-1,	-1},		/* pcode 80 */
    {	-1,	-1,	-1,	-1,	-1},		/* pcode  */
    {	-1,	-1,	-1,	-1,	-1},		/* pcode  */
    {	-1,	-1,	-1,	-1,	-1},		/* pcode  */
    {	-1,	-1,	-1,	2,	-1},		/* pcode 84 */
    {	-1,	0,	1,	2,	3},		/* pcode 85 */
    {	-1,	0,	1,	2,	3},		/* pcode 86 */
    {	-1,	-1,	-1,	-1,	-1},		/* pcode */
    {	-1,	-1,	-1,	-1,	-1},		/* pcode */
    {	-1,	-1,	-1,	-1,	-1},		/* pcode */
    {	-1,	-1,	-1,	-1,	-1},		/* pcode */
    {	-1,	-1,	-1,	-1,	-1},		/* pcode */
    {	-1,	-1,	-1,	-1,	-1},		/* pcode */
    {	 2,	-1,	-1,	-1,	-1},		/* pcode 93 */
    {	 2,	-1,	-1,	-1,	-1},		/* pcode 94 */
    {	-1,	-1,	-1,	-1,	-1},		/* pcode 95 */
    {	-1,	-1,	-1,	-1,	-1},		/* pcode 96 */
    {	-1,	-1,	-1,	-1,	-1},		/* pcode 97 */
    {	-1,	-1,	-1,	-1,	-1},		/* pcode 98 */
    {	 2,	-1,	-1,	-1,	-1},		/* pcode 99 */
};


#include <prod_user_msg.h>

#define UNUSED PARAM_UNUSED
#define ANY    PARAM_ANY_VALUE
#define ALG    PARAM_ALG_SET
#define ALL    PARAM_ALL_VALUES
#define EXS    PARAM_ALL_EXISTING

#define TOALG	10		/* turn this into "ALG" if this parameter is -1. 
				   Otherwise copy it over. */
#define PTOALG  11		/* turn this into "ALG" if the previous parameter 
				   is "ALG". Otherwise copy it over. */
#define COPY	0		/* copy the parameter over */

static short Prod_req_param_conv [PI_MAX_BUFFERNUM + 1][NUM_PROD_DEPENDENT_PARAMS] = {
     /*   0   0 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*   1   0 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*   2  19 */    { UNUSED, UNUSED,   COPY, UNUSED, UNUSED, UNUSED} ,
     /*   3  16 */    { UNUSED, UNUSED,   COPY, UNUSED, UNUSED, UNUSED} ,
     /*   4  20 */    { UNUSED, UNUSED,   COPY, UNUSED, UNUSED, UNUSED} ,
     /*   5  17 */    { UNUSED, UNUSED,   COPY, UNUSED, UNUSED, UNUSED} ,
     /*   6  21 */    { UNUSED, UNUSED,   COPY, UNUSED, UNUSED, UNUSED} ,
     /*   7  18 */    { UNUSED, UNUSED,   COPY, UNUSED, UNUSED, UNUSED} ,
     /*   8  28 */    { UNUSED, UNUSED,   COPY, UNUSED, UNUSED, UNUSED} ,
     /*   9  29 */    { UNUSED, UNUSED,   COPY, UNUSED, UNUSED, UNUSED} ,
     /*  10  30 */    { UNUSED, UNUSED,   COPY, UNUSED, UNUSED, UNUSED} ,
     /*  11  25 */    { UNUSED, UNUSED,   COPY, UNUSED, UNUSED, UNUSED} ,
     /*  12  22 */    { UNUSED, UNUSED,   COPY, UNUSED, UNUSED, UNUSED} ,
     /*  13  26 */    { UNUSED, UNUSED,   COPY, UNUSED, UNUSED, UNUSED} ,
     /*  14  23 */    { UNUSED, UNUSED,   COPY, UNUSED, UNUSED, UNUSED} ,
     /*  15  27 */    { UNUSED, UNUSED,   COPY, UNUSED, UNUSED, UNUSED} ,
     /*  16  24 */    { UNUSED, UNUSED,   COPY, UNUSED, UNUSED, UNUSED} ,
     /*  17  49 */    {   COPY,   COPY,   COPY, UNUSED, UNUSED, UNUSED} ,
     /*  18  39 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  19  40 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  20   0 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  21   0 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  22   0 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  23  37 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  24  35 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  25  38 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  26  36 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  27   0 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  28  42 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED,   COPY} ,
     /*  29  41 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  30   0 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  31   0 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  32   0 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  33  59 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  34   0 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  35  48 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  36  63 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  37  64 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  38  65 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  39  66 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  40   0 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  41   0 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  42  60 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  43   0 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  44  67 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  45  68 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  46  69 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  47   0 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  48   0 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  49  62 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  50   0 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  51  58 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  52   0 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  53   0 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  54  83 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  55   0 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  56  31 */    {   COPY,   COPY, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  57   0 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  58   0 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  59  46 */    {   COPY,   COPY,   COPY, UNUSED, UNUSED, UNUSED} ,
     /*  60  43 */    {   COPY,   COPY,   COPY, UNUSED, UNUSED, UNUSED} ,
     /*  61  45 */    {   COPY,   COPY,   COPY, UNUSED, UNUSED, UNUSED} ,
     /*  62  44 */    {   COPY,   COPY,   COPY, UNUSED, UNUSED, UNUSED} ,
     /*  63  47 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  64   0 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  65  70 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  66  71 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  67  72 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  68  56 */    { UNUSED, UNUSED,   COPY,  TOALG, PTOALG, UNUSED} ,
     /*  69  55 */    {   COPY,   COPY,   COPY,  TOALG, PTOALG, UNUSED} ,
     /*  70   0 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  71   0 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  72   0 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  73   0 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  74   0 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  75   0 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  76   0 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  77   0 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  78   0 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  79   0 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  80  61 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  81   0 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  82  52 */    {   COPY,   COPY,   COPY,   COPY, UNUSED, UNUSED} ,
     /*  83  51 */    {   COPY,   COPY,   COPY,   COPY, UNUSED, UNUSED} ,
     /*  84  50 */    {   COPY,   COPY,   COPY,   COPY, UNUSED, UNUSED} ,
     /*  85  57 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  86   0 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  87  93 */    { UNUSED, UNUSED,   COPY, UNUSED, UNUSED, UNUSED} ,
     /*  88  53 */    {   COPY,   COPY, UNUSED,   COPY,   COPY, UNUSED} ,
     /*  89   0 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  90   0 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  91   0 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  92   0 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  93   0 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  94  94 */    { UNUSED, UNUSED,   COPY, UNUSED, UNUSED, UNUSED} ,
     /*  95   0 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  96   0 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  97   0 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  98   0 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /*  99  99 */    { UNUSED, UNUSED,   COPY, UNUSED, UNUSED, UNUSED} ,
     /* 100   0 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /* 101   0 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /* 102   0 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /* 103   0 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /* 104   0 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /* 105  78 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /* 106  79 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /* 107  80 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /* 108  81 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /* 109  82 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /* 110  84 */    { UNUSED, UNUSED,   COPY, UNUSED, UNUSED, UNUSED} ,
     /* 111  85 */    {   COPY,   COPY,   COPY,   COPY, UNUSED, UNUSED} ,
     /* 112  86 */    {   COPY,   COPY,   COPY,   COPY, UNUSED, UNUSED} ,
     /* 113   0 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /* 114  87 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /* 115  88 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /* 116  89 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /* 117  90 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /* 118   0 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /* 119  34 */    {   COPY, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /* 120   0 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /* 121   0 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /* 122   0 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /* 123  97 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /* 124  95 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /* 125  98 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /* 126  96 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /* 127  ?? */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /* 128  ?? */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /* 129  74 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED} ,
     /* 130  83 */    { UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED}
};



#endif			/* #ifndef ORPGUMC_DEF_H */
