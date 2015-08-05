/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2004/05/24 16:02:23 $
 * $Id: orpgload.h,v 1.10 2004/05/24 16:02:23 steves Exp $
 * $Revision: 1.10 $
 * $State: Exp $
 */  

/***********************************************************************

	Header defining the data structure and constants used for
	various ORPG load shed items.

***********************************************************************/


#ifndef	ORPG_LOAD_SHED_H
#define	ORPG_LOAD_SHED_H

/**
  * Default ASCII CS Load Shed Table Filename
  */
#define ORPGLOAD_CS_DFLT_TABLE_FNAME "load_shed_table"

#define	ORPGLOAD_DATA_NOT_FOUND		 -1

#define	MIN_LOAD_SHED_VALUE		  0
#define	MAX_LOAD_SHED_VALUE		100

#define	LOAD_SHED_THRESHOLD_MSG_ID	  0 /* Message ID for threshold data */
#define	LOAD_SHED_CURRENT_MSG_ID	  1 /* Message ID for current data   */
#define	LOAD_SHED_THRESHOLD_BASELINE_MSG_ID	  2
					    /* Message ID for baseline 
					       threshold data */

#define	LOAD_SHED_WARNING_ACTIVE	1
#define LOAD_SHED_ALARM_ACTIVE		2

/*	The following enumerated list defines the macros used to	*
 *	identify a load shed category.					*/

enum {LOAD_SHED_CATEGORY_PROD_DIST,
      LOAD_SHED_CATEGORY_PROD_STORAGE,
      LOAD_SHED_CATEGORY_INPUT_BUF,
      LOAD_SHED_CATEGORY_RDA_RADIAL,
      LOAD_SHED_CATEGORY_RPG_RADIAL,
      LOAD_SHED_CATEGORY_WB_USER
};

/*	The following enumerated list defines the macros used to	*
 *	identify a load shed category item.				*/

enum {LOAD_SHED_WARNING_THRESHOLD,
      LOAD_SHED_ALARM_THRESHOLD,
      LOAD_SHED_CURRENT_VALUE
};

/*	The following structure defines the message used to store the	*
 *	ORPG load shed warning and alarm thresholds for the different	*
 *	load shed categories.						*/

typedef	struct {

	unsigned char	prod_dist_warn;	    /* Product Distribution warning  *
					     * threshold (0-100)             */
	unsigned char	prod_dist_alarm;    /* Product Distribution alarm    *
					     * threshold (0-100)             */
	unsigned char	prod_storage_warn;  /* Product Storage warning       *
					     * threshold (0-100)             */
	unsigned char	prod_storage_alarm; /* Product Storage alarm         *
					     * threshold (0-100)             */
	unsigned char	input_buf_warn;     /* Input Buffer warning          *
					     * threshold (0-100)             */
	unsigned char	input_buf_alarm;    /* Input Buffer alarm threshold  *
					     * (0-100)                       */
	unsigned char	rda_radial_warn;    /* RDA Radial warning            *
					     * threshold (0-100)             */
	unsigned char	rda_radial_alarm;   /* RDA Radial alarm threshold    *
					     * (0-100)                       */
	unsigned char	rpg_radial_warn;    /* RPG Radial warning            *
					     * threshold (0-100)             */
	unsigned char	rpg_radial_alarm;   /* RPG Radial alarm threshold    *
					     * (0-100)                       */
	unsigned char	wb_user_warn;        /* Wideband User warning thresh *
					     * (0-100)                       */
	unsigned char	wb_user_alarm;       /* Wideband user alarm thresh   *
					     * (0-100)                       */

} load_shed_threshold_t;

/*	The following structure defines the message used to store the	*
 *	current ORPG load shed utilization for the different load shed	*
 *	categories.  NOTE:  All values are percentages.			*
 *	Flags have been defined for each item in order to keep track	*
 *	of the current warning/alarm state.  We need to store these	*
 *	data in order to make the state information available to all	*
 *	consumers.  Otherwise, each would not know if another user has	*
 *	logged a system log message when the state changes.		*/

typedef	struct {

	unsigned char	prod_dist;      /* Product Distribution current  *
					 *     value (0-100)		 */
	unsigned char	prod_dist_state;/* Prod Dist warning/alarm state */
	unsigned char	prod_storage;   /* Product Storage current       *
					 *     value (0-100)             */
	unsigned char	prod_storage_state;/* Prod Storage warning/alarm state */
	unsigned char	input_buf;      /* Input buffer current value    *
					 *     (0-100)                   */
	unsigned char	input_buf_state;/* Input buf warning/alarm state */
	unsigned char	rda_radial;     /* RDA Radial current value      *
					 *     (0-100)                   */
	unsigned char	rda_radial_state;/* RDA Radial warning/alarm state */
	unsigned char	rpg_radial;     /* RPG Radial current value      *
					 *     (0-100)                   */
	unsigned char	rpg_radial_state;/* RPG Radial warning/alarm state */
	unsigned char	wb_user;        /* Wideband User current value   *
					 *     (0-100)                   */
	unsigned char	wb_user_state;	/* Wideband User warning/alarm state */

} load_shed_current_t;

int	ORPGLOAD_io_status   (int msg_id);
int	ORPGLOAD_update_flag (int msg_id);
void	ORPGLOAD_en_status_callback (int lbfd, LB_id_t msgid, int msglen, void *arg);
int	ORPGLOAD_read  (int msg_id);
int	ORPGLOAD_write (int msg_id);
int	ORPGLOAD_get_data (int category, int type, int *value);
int	ORPGLOAD_set_data (int category, int type, int value);

#endif
