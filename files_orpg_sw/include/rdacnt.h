/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/02/11 17:07:26 $
 * $Id: rdacnt.h,v 1.18 2014/02/11 17:07:26 steves Exp $
 * $Revision: 1.18 $
 * $State: Exp $
 */
#ifndef RDACNT_H
#define RDACNT_H

#include <time.h>

/* Message ID for VCP data received from RDA. */
#define RDA_RDACNT	      2

/*
  Define VCP data constants and size information.
*/
#define WXVCPMAX	     21
#define WXMAX                 2
#define COMPMAX               2
#define VCPMAX               20
#define FILES_MAX            VCPMAX
#define ECUTMAX              25
#define ECUT_UNIQ_MAX        20
#define VOL_ATTR             11
#define EL_ATTR              23
#define VCPSIZ               ((ECUTMAX*EL_ATTR)+VOL_ATTR)

/* 
  Where-defined values. 
*/
#define VCP_RDA_DEFINED       0
#define VCP_RPG_DEFINED       1
#define VCP_EXPERIMENTAL      2

/*
  VCP flag bits. 
*/
#define VCP_FLAGS_ALLOW_SAILS		0x01
#define VCP_MAX_SAILS_CUTS_MASK         0x0e
#define VCP_MAX_SAILS_CUTS_SHIFT	1

/*
  Define PRF data constants and sizes.
*/
#define PRFMAX		8
#define NAPRFVCP	(PRFMAX+2)
#define NAPRFELV	(PRFMAX+1)
#define MXALWPRF	(NAPRFVCP+(ECUTMAX*NAPRFELV))
#define DELPRI_MAX	5

/*
  RDACNT Adaptation Data Structure
*/
typedef struct {

   int   rdacnt_start;			/* Structure start identifier.	*/

   short rdcwxvcp[WXMAX][WXVCPMAX]; 	/* Weather mode table.        	*/

   short rdc_where_defined[COMPMAX][VCPMAX]; 	/* VCP defined table.  	*/

   short rdcvcpta[VCPMAX][VCPSIZ];	/* Table of VCPs.		*/

   short rdccon[VCPMAX][ECUTMAX];	/* Table of RPG Elev Indices.	*/

   short alwblprf[VCPMAX][MXALWPRF];	/* Table of Allowable PRFs 	*/
  
   float prfvalue[PRFMAX];		/* Table of PRF's, in Hertz. 	*/

   short vcp_times[VCPMAX];	   	/* Table of VCP times, in seconds. */

   short vcp_flags[VCPMAX];		/* Table of VCP flags. */

   int   unambigr[DELPRI_MAX][PRFMAX];	/* Table of Unambiguous Ranges. */

   int   unambigr_SPRT[DELPRI_MAX][PRFMAX];	/* Table of Unambiguous Ranges
						   (SPRT only). */

   int   delta_pri;			/* Index into PRF table.        */

   int   rdacnt_last;			/* Stucture end identifier.	*/

} Rdacnt;

/*  Define a structure for the allowable prf table	*/

typedef struct {

    short	vcp_num;
			/* VCP number					*/
    short	num_alwbl_prf;
			/* Number of allowable prfs for this VCP	*/
    short	prf_num [PRFMAX];
			/* Allowable prf numbers for this VCP		*/
    short	pulse_cnt [ECUTMAX][NAPRFELV];
			/* Pulse counts for each allowable prf and	*
			 * elevation in this VCP.  Element NAPRFELV	*
			 * is the defalut prf number.			*/

} Vcp_alwblprf_t;

/* Offsets into the pulse_cnt array of Vcp_alwblprf_t structure. */

#define NUM1PULS       0         /* Pulse count for PRF # = 1 */
#define NUM2PULS       1         /* Pulse count for PRF # = 2 */
#define NUM3PULS       2         /* Pulse count for PRF # = 3 */
#define NUM4PULS       3         /* Pulse count for PRF # = 4 */
#define NUM5PULS       4         /* Pulse count for PRF # = 5 */
#define NUM6PULS       5         /* Pulse count for PRF # = 6 */
#define NUM7PULS       6         /* Pulse count for PRF # = 7 */
#define NUM8PULS       7         /* Pulse count for PRF # = 8 */
#define DEFLTPRF       8         /* Default PRF number */

/* Flags defined for the Supplemental Data. */
#define RDACNT_IS_CS          0x0001          /* Contg Surv cut. */
#define RDACNT_IS_CD          0x0002          /* Contig Doppler W Amb Reso. */
#define RDACNT_IS_CDBATCH     0x0004          /* Contig Doppler W/O Amb Reso. */
#define RDACNT_IS_BATCH       0x0008          /* Batch cut. */
#define RDACNT_IS_SPRT        0x0010          /* Staggered PRT cut. */
#define RDACNT_IS_SR          0x0020          /* SR cut. */
#define RDACNT_IS_SZ2         0x0040          /* SZ2 cut. */
#define RDACNT_IS_SPARE_TYPE  0x0080
#define RDACNT_SUPPL_SCAN     0x0100          /* Supplemental cut. */

/* Defines the VCP information:  Pattern definition, RDA to RPG 
   elevation mapping, and supplemental data. */
typedef struct RDA_vcp_info {

   int volume_scan_number;      /* Volume scan number: 1-80   */

   short rdcvcpta[VCPSIZ];	/* Table of VCPs.	      */

   short rdccon[ECUTMAX];	/* Table of RPG Elev Indices. */

   unsigned short suppl[ECUTMAX]; /* Supplemental Scan flags. */
  

} RDA_vcp_info_t;

#define MAX_ENTRIES		2

/* Stores informaton about the VCP received from the RDA. */
typedef struct RDA_rdacnt {

   int last_entry;		/* Index for last entry updated. 
                                   Either 0 or 1. */

   time_t last_entry_time;	/* Unix time when entry updated. */

   time_t last_message_time;	/* RDA VCP message data time 
                                   (Unix time) */

   RDA_vcp_info_t data[MAX_ENTRIES]; 

} RDA_rdacnt_t;

#endif
