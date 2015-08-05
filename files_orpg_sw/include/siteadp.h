/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2007/03/08 18:42:25 $
 * $Id: siteadp.h,v 1.20 2007/03/08 18:42:25 steves Exp $
 * $Revision: 1.20 $
 * $State: Exp $
 */

/*	This header file defines the structure which maps to the legacy	*
 *	site adaptation data block (SITEADP).				*/


#ifndef SITEADP_H
#define	SITEADP_H

#include <orpgctype.h>


/**  Macro to access dea values. Make it the empty string, since   */
/*   macros in orpgsite prepend "site_info" but function requires  */
/*   3 arguments. */
#define SITE_INFO_DEA_NAME	"site_info"
#define SITE_INFO_CHAR_LENGTH	8


/************************************************************************
 from a309adapt.inc

C
C   SITE ADAPTATION DATA DEFINITIONS
C
       INTEGER*4 SIRDALAT
       INTEGER*4 SIRDALON
       INTEGER*4 SIRDAHGT
       INTEGER*4 SIRPGID
       INTEGER*4 SIWXSTRT
       INTEGER*4 SIWXAVCP
       INTEGER*4 SIWXBVCP
       INTEGER*4 SIHMLOS   
       INTEGER*4 SIHRMS
       INTEGER*4 SIRTYPE
       INTEGER*4 SICHANNM
       INTEGER*4 SIPCODE

      COMMON /SITEADP/ SIRDALAT, SIRDALON, SIRDAHGT,
     *                 SIRPGID,  SIWXSTRT, SIWXAVCP,
     *                 SIWXBVCP, SIHMOS, SIHRMS, 
     *                 SIRTYPE, SICHANNM, SIPCODE,
*/

#define FAA_REDUNDANT ORPGSITE_FAA_REDUNDANT
#define NO_REDUNDANCY ORPGSITE_NO_REDUNDANCY
#define NWS_REDUNDANT ORPGSITE_NWS_REDUNDANT 

typedef struct {

	fint	rda_lat;		/* RDA Latitude (degrees*1000) */

	fint	rda_lon;		/* RDA Longitude (degrees*1000) */

	fint	rda_elev;		/* Elevation (ft MSL) at RDA*/

	fint	rpg_id;			/* Numeric ID associated with this RPG */

	fint	wx_mode;		/* Default weather mode at the RPG.
					   enum_values "Clear Air=1", "Precipitation=2" */

	fint	def_mode_A_vcp;		/* Default weather mode A VCP */

	fint	def_mode_B_vcp;		/* Default weather mode B VCP */

	int	has_mlos;		/* Does this site have microwave line of site equipment?
					   enum_values "No=0", "Yes=1" */

	int 	has_rms;		/* Does this site have RMS?
					   enum_values "No=0", "Yes=1" */

	int	has_bdds;		/* Does this site have BDDS?
					   enum_values "No=0", "Yes=1" */

	int	has_archive_III;	/* Does this site have Archive III? 
					   enum_values "No=0", "Yes=1" */

	int	is_orda;		/* Does this site have ORDA? 
					   enum_values "No=0", "Yes=1" */

	int	product_code;		/* This is the product code used for the background
					   reflectivity product in the Clutter Regions
					   Editor and PRF Selection task. */

	char 	rpg_name[SITE_INFO_CHAR_LENGTH]; /* 4 character radar acronyms string (ie. KREX) */

} Siteadp_adpt_t;

/**
     Declare a C++ wrapper class that statically contains a pointer to the meta
     data for this structure.  The C++ wrapper class allows the IOCProperty API
     to be implemented automatically for this structure
**/

typedef struct
{
	int	redundant_type; 	 	/* Type of redundancy.  enum_values: "No Redundancy=0", 
						    "FAA Redundant=1", "NWS Redundant" */

	int	channel_number;			/* Channel number of this RPG. */

} Redundant_info_t;

/**
     Declare a C++ wrapper class that statically contains a pointer to the meta
     data for this structure.  The C++ wrapper class allows the IOCProperty API
     to be implemented automatically for this structure
**/

#endif
