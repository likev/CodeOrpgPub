/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2010/03/10 19:26:33 $
 * $Id: clutter.h,v 1.13 2010/03/10 19:26:33 ccalvert Exp $
 * $Revision: 1.13 $
 * $State: Exp $
 */
/*****************************************************************
//
//     Contains the structure defintions for the Clutter Filter
//     Notchwidth Map, Bypass Map (and Edited Bypass Map), 
//     and Clutter Censor Zones.
//
//     Note:  The Clutter Censor Zone data are in RPG internal
//            format.  The Notchwidth Map and Bypass Map are in
//            RDA/RPG ICD format.
//    
//     The Linear Buffer data store ID is ORPGDAT_CLUTTER_DATA
//
****************************************************************/

#ifndef CLUTTER_H
#define CLUTTER_H

#include <rda_notch_width_map.h>
#include <orda_clutter_map.h>
#include <rda_rpg_clutter_map.h>
#include <rpg_clutter_censor_zones.h>

/* Message IDs within ORPGDAT_CLUTTER_DATA. */
enum {
   LBID_CLUTTERMAP_LGCY,
   LBID_BYPASSMAP_LGCY,
   LBID_CENSOR_ZONES_LGCY,
   LBID_EDBYPASSMAP_LGCY,
   LBID_BASELINE_CENSOR_ZONES_LGCY,
   LBID_CLUTTERMAP_ORDA,
   LBID_EDBYPASSMAP_ORDA,
   LBID_CENSOR_ZONES_ORDA,
   LBID_BYPASSMAP_ORDA,
   LBID_BASELINE_CENSOR_ZONES_ORDA
};


/* Constants for legacy "notchwidth" map. */
#define CLNW_RADHW       32
#define CLNW_MAXRAD     256
#define CLNW_MAXSEG       2
#define CLNW_ASEGHW     (CLNW_RADHW * CLNW_MAXRAD * CLNW_MAXSEG)  

/* Constants for ORDA "clutter filter" map. */
#define CLM_RADHW_ORDA   40
#define CLM_MAXRAD_ORDA 360
#define CLM_MAXSEG_ORDA   5
#define CLM_ASEGHW_ORDA (CLM_RADHW_ORDA * CLM_MAXRAD_ORDA * CLM_MAXSEG_ORDA)  

/* Constants for Legacy RDA bypass map. */
#define CLBY_RADHW       32
#define CLBY_MAXRAD     256
#define CLBY_MAXSEG       2
#define CLBY_MAXKM      512
#define CLBY_SEGHW      (CLBY_RADHW*CLBY_MAXRAD)  
#define CLBY_ASEGHW     (CLBY_SEGHW*CLBY_MAXSEG)  

/* Constants for Open RDA bypass map. */
#define CLBY_RADHW_ORDA   32
#define CLBY_MAXRAD_ORDA 360
#define CLBY_MAXSEG_ORDA   5
#define CLBY_MAXKM_ORDA  512
#define CLBY_SEGHW_ORDA  (CLBY_RADHW_ORDA * CLBY_MAXRAD_ORDA)  
#define CLBY_ASEGHW_ORDA (CLBY_SEGHW_ORDA * CLBY_MAXSEG_ORDA)  

#define MAX_LABEL_SIZE   32
#define MAX_CLTR_FILES   20

typedef struct{

   int    time;                   /* Time of last modification (Julian
                                     seconds). */
   char   label [MAX_LABEL_SIZE]; /* Identify label associated with censor
                                     zone file. */

} file_tag_t;

typedef struct{

   file_tag_t             file_id; /* File properties. */
   RPG_clutter_regions_t  regions; /* Legacy RDA Clutter Censor Zones. */

} RPG_clutter_regions_file_t;

typedef struct{

   file_tag_t             file_id; /* File properties. */
   ORPG_clutter_regions_t  regions; /* Open RDA Clutter Censor Zones. */

} ORPG_clutter_regions_file_t;

typedef struct{

   file_tag_t             file_id; /* File properties. */
   B9_ORPG_clutter_regions_t  regions; /* Open RDA Clutter Censor Zones. */

} B9_ORPG_clutter_regions_file_t;


typedef struct{

   int                        last_dwnld_time;       /* */
   int                        last_dwnld_file;       /* */
   RPG_clutter_regions_file_t file [MAX_CLTR_FILES]; /* */

} RPG_clutter_regions_msg_t;

typedef struct{

   int                        last_dwnld_time;       /* */
   int                        last_dwnld_file;       /* */
   ORPG_clutter_regions_file_t file [MAX_CLTR_FILES]; /* */

} ORPG_clutter_regions_msg_t;

typedef struct{

   int                        last_dwnld_time;       /* */
   int                        last_dwnld_file;       /* */
   B9_ORPG_clutter_regions_file_t file [MAX_CLTR_FILES]; /* */

} B9_ORPG_clutter_regions_msg_t;

typedef struct{

   int 	                       last_download_time;  /* Unix Time, last time
						       CCZ downloaded to RDA. */
   int 	                       last_download_file;  /* CCZ file last downloaded 
                                                       to RDA. */
   int                         regions;             /* Number of CCZ regions. */
   ORPG_clutter_region_data_t  data[MAX_NUMBER_CLUTTER_ZONES];                
                                                    /* CCZ data last downloaded. */

} Clutter_region_download_data_t;

#define CCZ_CHANNEL_1		0
#define CCZ_CHANNEL_2		1

typedef struct{

   Clutter_region_download_data_t channel[2];       /* Channel number ... For DOD and NWS single, 
                                                       channel number will always be channel 1 ...
                                                       i.e., data is stored at CCZ_CHANNEL_1.
                                                       For FAA and NWS Redundant, data will be 
                                                       stored at either CZZ_CHANNEL_x depending
                                                       on which channel is connected. */
} Clutter_region_download_info_t;

#endif

