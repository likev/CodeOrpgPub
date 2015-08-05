/*
 * RCS info
 * $Author: nolitam $
 * $Locker:  $
 * $Date: 2002/12/11 22:10:09 $
 * $Id: gagedata.h,v 1.4 2002/12/11 22:10:09 nolitam Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 * $Log: gagedata.h,v $
 * Revision 1.4  2002/12/11 22:10:09  nolitam
 * Add RCS header information
 *
 * Revision 1.3  1996/10/17 17:19:47  cm
 * Using Fortran/C types
 *
 * Revision 1.2  96/10/16  16:24:36  16:24:36  cm (OS RPG Configuration Management Account)
 * Add start/end variables
 * 
 * Revision 1.1  96/10/16  08:17:37  08:17:37  dodson (Arlis Dodson)
 * Initial revision
 * 
 */
/*************************************************************************

      Module: gagedata.h

 Description:

	Header file for the ORPG implementation of the legacy RPG Rain
	Gage Database file (a.k.a. gagedb.dat).

 Assumptions:

**************************************************************************/




#ifndef GAGEDATA_H
#define GAGEDATA_H

/*
 * System/Local Include Files
 */
#include <a309.h>
#include <rpg_port.h>
#include <orpg.h>

/*
 * Constant Definitions/Macro Definitions/Type Definitions
 *
 * Legacy RPG Rain Gage Database file sectors
 * Sector  0 - Precipitation Status Information
 * Sector  1 - Gage Identification Information
 * Sector 32 - Gage Reports
 */
#define GAGE_STAT	0
#define GAGE_IDS	1
#define GAGE_RPTS	32

/*
 * Precipitation Status Information
 * Legacy RPG file sector 0
 *
 * Date/time precipitation detection algorithm last executed (units???)
 * Date/time of last precipitation (units???)
 * Current precipitation category
 * Last precipitation category
 * Number of rain gages
 * Date/time gage data base last updated (units???)
 * Gage database update pending flag
 *
 * Note that we must retain the start_stat and end_stat elements since
 * the legacy RPG code offsets reflect this common block members ...
 */
typedef struct {
   fint start_stat ;
   fint date_detect_ran ;
   fint time_detect_ran ;
   fint date_last_precip ;
   fint time_last_precip ;
   fint cur_precip_cat ;
   fint last_precip_cat ;
   fint num_gages ;
   fint date_gdb_update ;
   fint time_gdb_update ;
   fint gdb_update_pending ;
   fint end_stat ;
} gage_precip_status_info ;

/*
 * Gage Identification Information
 * Legacy RPG file sector 1
 *
 *   gage_pass ???
 *   gage_passflg ???
 *   Permanent Gage List
 *      gage 8-character identifier
 *      latitude/longitude of the gage
 *      azimuth/range from the radar
 *      pointer to the start of the reports
 *   Gage Order Table
 *      list of pointers into the Permanent Gage Identifier Table  
 *   Gage Distance Matrix
 *      matrix of distances between all gages
 *   Pending Gage List
 *      gage 8-character identifier
 *      latitude/longitude of the gage
 *      azimuth/range from the radar
 *      pointer to the start of the reports
 *
 * Note that we must retain the start_gid and end_gid elements since
 * the legacy RPG code offsets reflect this common block members ...
 */
typedef struct {
   fint start_gid ;
   fint gage_pass[PASSITMS] ;
   fint gage_passflg ;
   fint gage_id[MAXGAGES][GAGE_ITEMS] ;
   fint2 gage_order[MAXGAGES] ;
   fint2 gage_rel_dist[MAXGAGES][MAXGAGES] ;
   fint wgage_id[MAXGAGES][GAGE_ITEMS] ;
   fint end_gid ;
} gage_id_info ;

/*
 * Gage Reports
 * Legacy RPG file sector 32
 *
 * Three-dimensional table indexed by report parameter, gage number, and
 * report number.
 *
 * Report Parameters
 *   date of the report (units???)
 *   time of the report most-significant two bytes (units???)
 *   time of the report least-significant two bytes (units???)
 *   type of report (incremental or accumulative)
 *   duration of report (time interval if incremental)
 *   amount of precipitation in inches scaled 10
 *
 * Note that we must retain the start_reports and end_reports elements since
 * the legacy RPG code offsets reflect this common block members ...
 */
typedef struct {
   fint start_reports ;
   fint2 gage_reports[NGAGES_USER][NREP_GAGE][NREP_PARMS] ;
   fint end_reports ;
} gage_reports ;

/*
 * The complete Rain Gage Database
 */
typedef struct {
   /*
    * Legacy RPG File Sector 0
    * Legacy RPG File Sector 1
    * Legacy RPG File Sector 32
    */
   gage_precip_status_info precip_status_info ;
   gage_id_info id_info ;
   gage_reports reports ;
} gagedata ;


#endif /* DO NOT REMOVE */
