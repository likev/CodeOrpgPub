/* 
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2005/02/09 22:55:43 $
 * $Id: alert_threshold.h,v 1.7 2005/02/09 22:55:43 ryans Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */  
/**********************************************************************

	Header defining the data structures and constants used for
	Alert Threshold Adaptation data table.	

**********************************************************************/


#ifndef	ADAPTATION_ALERT_THRESHOLD_H
#define ADAPTATION_ALERT_THRESHOLD_H

#define ALERTING_DEA_NAME "Alert"

#define	ALERT_THRESHOLD_CATEGORIES	41
#define	ALERT_THRESHOLD_GROUPS		 3

#define	ALERT_THRESHOLD_NAME_LEN	16
#define	ALERT_THRESHOLD_UNIT_LEN	 8

/*	Definition for alert threshold table entry (legacy)		*/

typedef struct {

	short	group;		/* Positive integer denotes valid	*
				 * group. 0 = Unused entry.		*/
	short	category;	/* Alert category (1=41).		*/
	short	num_thresh;	/* Number of thresholds defined		*/
	short	thresh_1;	/* Threshold 1				*/
	short	thresh_2;	/* Threshold 2				*/
	short	thresh_3;	/* Threshold 3				*/
	short	thresh_4;	/* Threshold 4				*/
	short	thresh_5;	/* Threshold 5				*/
	short	thresh_6;	/* Threshold 6				*/
	short	prod_code;	/* Product identifier			*/

} alert_threshold_data_t;

/*	Definition for alert threshold table entry (ORPG)		*/

typedef struct {

	short	group;		/* Positive integer denotes valid	*
				 * group. 0 = Unused entry.		*/
	short	category;	/* Alert category (1=41).		*/
	short	num_thresh;	/* Number of thresholds defined		*/
	short	thresh_1;	/* Threshold 1				*/
	short	thresh_2;	/* Threshold 2				*/
	short	thresh_3;	/* Threshold 3				*/
	short	thresh_4;	/* Threshold 4				*/
	short	thresh_5;	/* Threshold 5				*/
	short	thresh_6;	/* Threshold 6				*/
	short	prod_code;	/* Product identifier			*/
	short	type;		/* Type of allowed paired product.	*
				 * bit 0 = Volume			*
				 * bit 1 = Elevation			*
				 * bit 2 = Hydromet			*/
	short	min;		/* Minimum threshold value.		*/
	short	max;		/* Maximum threshold value.		*/
	short	loca;		/* Level of change authority (LOCA)	*
				 * required to edit this category:	*
				 * (Bitmap data: inclusive)		*
				 *	0 = None			*
				 *	1 = URC				*
				 *	2 = Agency			*
				 *	4 = OSF				*/
	char	name [ALERT_THRESHOLD_NAME_LEN];
				/* Description of the alert entry.	*/
	char	unit [ALERT_THRESHOLD_UNIT_LEN];
				/* Units of alert entry.		*/

} orpg_alert_threshold_data_t;

/*	Definition or alert threshold table.				*/

typedef struct {

	alert_threshold_data_t	data [ALERT_THRESHOLD_CATEGORIES];

} alert_threshold_t;

/*	The following typedef defines the structure for defining	*
 *	alert threshold groups in the ORPG.				*/

typedef	struct {

	short	id;		/*  Group id (>0).			*/
	short	spare;
	char	name [ALERT_THRESHOLD_NAME_LEN];

} orpg_alert_threshold_group_t;

/*	The following structure defines the header for the alert	*
 *	threshold data.  This header is used to specify the number	*
 *	of group (alert_threshold_group_data_t) and category		*
 *	(alert_threshold_data_t) structure which immediately follow	*
 *	it.								*/

typedef struct {

	short	categories;
	short	groups;

} orpg_alert_threshold_hdr_t;

#endif
