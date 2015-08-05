/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2012/05/11 21:53:50 $
 * $Id: vcp.h,v 1.24 2012/05/11 21:53:50 steves Exp $
 * $Revision: 1.24 $
 * $State: Exp $
 * $Log: vcp.h,v $
 * Revision 1.24  2012/05/11 21:53:50  steves
 * issues 3-985
 *
 * Revision 1.23  2010/12/01 17:15:11  steves
 * issue 3-803
 *
 * Revision 1.22  2009/05/05 13:59:45  steves
 * issue 3-627
 *
 * Revision 1.21  2009/03/10 14:53:10  ccalvert
 * Reverted from version 1.18
 *
 * Revision 1.18  2009-02-26 13:48:31-06  steves
 * issue 3-580
 *
 * Revision 1.16  2007/02/23 20:32:52  steves
 * issue 3-183
 *
 * Revision 1.15  2006/11/21 23:37:08  steves
 * issue 3-094
 *
 * Revision 1.14  2006/09/14 23:01:03  steves
 * issue 3-046
 *
 * Revision 1.13  2006-09-08 09:23:23-05  steves
 * Support for Super Res (Issue 3-046)
 *
 * Revision 1.12  2006/05/05 21:48:05  steves
 * issue 2-745
 *
 * Revision 1.11  2005/11/08 21:12:16  steves
 * issue 2-744
 *
 * Revision 1.10  2005-11-08 15:07:40-06  steves
 * issue 2-744
 *
 * Revision 1.9  2005/08/15 17:28:29  steves
 * issue 2-811
 *
 * Revision 1.8  2004/10/19 13:36:11  steves
 * issue 2-087
 *
 * Revision 1.7  2003/06/23 14:45:11  steves
 *  issue 2-200
 *
 * Revision 1.6  2003/05/27 18:39:20  steves
 * issue 2-163
 *
 * Revision 1.5  2002/12/11 22:11:37  nolitam
 * Add RCS header information
 *
 * Revision 1.4  2002/11/26 22:28:04  jing
 * Update
 *
 * Revision 1.3  1997/04/02 22:38:35  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.2  1996/09/16 15:51:40  jing
 *
 * Revision 1.1  96/06/25  15:48:49  15:48:49  dodson (Arlis Dodson)
 * Initial revision
 * 
 */
/*****************************************************************

	Header file defining VCP related data structures.

*/



# ifndef VCP_H
# define VCP_H


/* Note: This needs to be kept consistent with rpg_vcp.h, most
         notibly VCP_ICD_msg_t. */
  

/****** part 1 ******/

/* The array RDCWXVCP is used for defining the weather mode for 
   each VCP. The following defines this in C version; We still
   use an short array; Note that the index corresponding to weather_mode
   increments slower if the array is viewed as 1D */

/* The array RDCWXVCP (WXVCPMAX (21), WXMAX (2)) contains 
   the VCP number for each (?, weather_mode).  */
#define VCP_WM_MAXN_VCP 	21
#define VCP_WM_MAXN_WEATHER_MODE 2
#define VCP_WM_SIZE 		(21*2*2)	/* size of the array in bytes */

/* The weather mode can take values either 1 or 2, which mean the clear
   air and convective modes respectively (?) */


/****** part 2 ******/

/* array RDCVCPTA is used to define the VCP data structures in RPG;
   The following defines this in C structures */

#define VCP_MAXN_CUTS 		25	/* maximum number of elevations in a VCP */
#define VCP_MAXN_VCPS 		20	/* max number of VCPs in the VCP table */
#define ELE_ATTR_SIZE	 	23	/* buffer size for each elevation attribute
					   table (in shorts) */
#define VCP_ATTR_SIZE 		11      /* buffer size for the VCP attribute data
                                	   (in shorts) */

#define VCP_MIN_CLEAR_AIR	30
#define VCP_MAX_CLEAR_AIR	39

#define VCP_MIN_MPDA  		100
#define VCP_MAX_MPDA  		199
#define VCP_MIN_SZ2   		200
#define VCP_MAX_SZ2   		299

/* structure defining each cut */
typedef struct {	/* 23 shorts */

    short ele_angle;		/* elevation angle; *180/32768 = degees; 
			   	   The unit used does not seem right; should be 
			   	   in .1 degrees. */

    unsigned char phase; 	/* phase;
					0 - constant phase; 
					1 - random phase;
					2 - SZ2 phase. */

    unsigned char wave_type;	/* waveform type;
					0 - linear channel; 
					1 - log channel. */

    unsigned char super_res; 	/* 1 - 1/2 deg azimuth
                                   2 - 1/4 km reflectivity
                                   4 - 300 km Doppler. 
                                   8 - Dual Pol to 300 km. */

    unsigned char surv_prf_num; /* surveillance PRF number; 1-8. */

    short surv_pulse_cnt; 	/* survallance pulse count; 1-999. */

    short azi_rate;		/* azimuth rate; *22.5/16384 degrees/second.  */

    short surv_thr_parm; 	/* reflectifity threshold, in 1/8 dB; -12.0 - 20.0. */

    short vel_thrsh_parm; 	/* Doppler velocity threshold parameter, in 1/8 dB;
				   -12.0 - 20.0. */

    short spw_thrsh_parm; 	/* spectrum width threshold parameter, in 1/8 dB;
				   -12.0 - 20.0. */ 

    short zdr_thrsh_parm;	/* differental reflectivity threshold parameter, in 1/8 dB;
				   -12.0 - 20.0. */

    short phase_thrsh_parm;	/* differential phase threshold parameter, in 1/8 dB;
				   -12.0 - 20.0. */

    short corr_thrsh_parm;	/* correlation coefficient threshold parameter, in 1/8 dB;
 				   -12.0 - 20.0. */

    /* segment 1 */
    short azi_ang_1;	/* segment 1 azimuth clockwise edge angle 
			   Use the same unit as ele_angle. */

    short dop_prf_num_1;
			/* segment 1 Doppler PRF number; 1-8; PFS1P1NU */

    short pulse_cnt_1;	/* segment 1 pulse count; 1-999. */

    short not_used_1;

    /* segment 2 */
    short azi_ang_2;

    short dop_prf_num_2;

    short pulse_cnt_2;	

    short not_used_2;

    /* segment 3 */
    short azi_ang_3;

    short dop_prf_num_3;

    short pulse_cnt_3;

    short not_used_3;

} Ele_attr;

/* Values for wave_type */
#define VCP_WAVEFORM_UNKNOWN 	0	/* unknown wave type */
#define VCP_WAVEFORM_CS      	1	/* contiguous surveillance wave type */
#define VCP_WAVEFORM_CD      	2	/* Continuous Doppler wave type */
#define VCP_WAVEFORM_CDBATCH 	3	/* Continuous Doppler batch wave type */
#define VCP_WAVEFORM_BATCH   	4	/* Batch wave type */
#define VCP_WAVEFORM_STP     	5	/* Staggered Pulse wave type */

/* Values for phase */
#define VCP_PHASE_CONSTANT   	0
#define VCP_PHASE_RANDOM     	1
#define VCP_PHASE_SZ2        	2

/* Values for super resolution type. */
#define VCP_HALFDEG_RAD      	1
#define VCP_QRTKM_SURV	     	2
#define VCP_300KM_DOP        	4
#define VCP_SUPER_RES_MASK	(VCP_HALFDEG_RAD|VCP_QRTKM_SURV|VCP_300KM_DOP)

/* Value for Dual Pol. */
#define VCP_DUAL_POL_ENABLED	8

/* Value to support VCP Translation. */
#define VCP_DO_NOT_TRANSLATE	0x1234

/* define a VCP data structure */
typedef struct {

    short msg_size;		/* number of half words; 23 - 594 depending
			   	   on type; PFNHW=1 */
    short type;			/* pattern type: PFPATTYP=2 
			           Constant elevation cut: 2
			           Horizontal raster scan: 4
			           Vertical raster scan:   8
			           Searchlight:		   16 */
    short vcp_num;		/* pattern number: PFPATNUM=3 
			           Maintenance/Test: > 255
			           Operational: <= 255
			           Constant Elevation types:   1 -  99
			           Horizontal Raster types:  100 - 149
			           Vertical Raster types:    150 - 199
			           Searchlight types:        200 - 249  */

    /* the following fields are good for Constant elevation cut type */

    short n_ele;		/* number of elevations are scanned in this
			           volume (including repeated elev); 1-25. */
    short clutter_map_num; 	/* clutter map group number; 1 - 99. */
    unsigned char vel_resolution;
				/* velocity resolution; 
			   	    0.5 meters/second: 2
			   	    1.0 meters/second: 4   */
    unsigned char pulse_width;	/* pulse width;  
			   	   short:  2
			   	   long:   4     */

/* Note the following items are actually spare fields. */
    short sample_resolution; 	/* sampling range resolution;
			           250 meters: 0
			           50 meters:  1  */
    short spare1; 		/* Note: This is currently used in conjunction with
                                         VCP Translation.   If this value has special
                                         value VCP_DO_NOT_TRANSLATE (0x1234), then VCP
                                         Translation will not translate. */

    short spare2; 

    short spare3; 

    short spare4;

/* Because Ele_attr has a size of 23 shorts, which is not aligned, we use the folowing 
   array to reserve the space for Ele_attr.  When we use this we cast to the struct 
   ele_attr = (Ele_attr *)(vcp.vcp_ele[ele_num])       */
    short vcp_ele [VCP_MAXN_CUTS][ELE_ATTR_SIZE]; /* specify the cuts */

} Vcp_struct;

/* VCP table equivalent to array RDCVCPTA */
typedef struct {

    Vcp_struct vcp_table [VCP_MAXN_VCPS];

} Vcp_table;

#define VCP_VCP_SIZE (sizeof(Vcp_table))	/* size of RDCVCPTA */

/****** part 3 ******/

/* the RDCCON array is a RPG elevation index table of size
    [VCP_MAXN_VCPS][VCP_MAXN_CUTS]; [vcp_ind][elev_num - 1] is
    the correspoding RPG elevation index */

#define VCP_RDCCON_SIZE (VCP_MAXN_VCPS*VCP_MAXN_CUTS*2)	
					/* size of the array in bytes */


/***** part 4 *****/

/* starting offset of each field in the RDACNT adaptation data */
#define VCP_WM_OFFSET	4	/* there is a RDACNT_FIRST in front */
#define VCP_VCP_OFFSET  (VCP_WM_OFFSET+VCP_WM_SIZE)
#define VCP_RDCCON_OFFSET  (VCP_VCP_OFFSET+VCP_VCP_SIZE)



#endif 

