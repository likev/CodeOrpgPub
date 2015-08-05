/**********************************************************************

	Header file defining the product requests.

**********************************************************************/

/*
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2003/03/04 20:09:08 $
 * $Id: prod_request.h,v 1.9 2003/03/04 20:09:08 ryans Exp $
 * $Revision: 1.9 $
 * $State: Exp $
 * $Log: prod_request.h,v $
 * Revision 1.9  2003/03/04 20:09:08  ryans
 * Change outdated macro naming convention
 *
 * Revision 1.8  2002/12/15 17:52:04  steves
 * issues 2-085
 *
 * Revision 1.6  2000/01/06 23:14:58  steves
 * fix
 *
 * Revision 1.5  1999/07/06 21:05:39  steves
 * NO COMMENT SUPPLIED
 *
 * Revision 1.4  1998/08/12 22:43:21  steves
 * modefy
 *
 * Revision 1.3  1997/06/09 15:02:10  steves
 * modefy
 *
 * Revision 1.2  1996/06/25 14:42:18  dodson
 * Build9/PORT update
 *
 */

#ifndef PROD_REQUEST_H
#define PROD_REQUEST_H

/* The following define the product request format used by librpg and librpgc
   for scheduling products.  For elevation-based products, a positive
   elevation index refers to the elevation, whereas -1 means all elevations,
   and -2 means not schedule.  For volume-based products, -1 means scheduled 
   and -2 means not scheduled.

   For the product dependent parameters, refer to the RPG/APUP ICD. */

/* Macro definitions for Prod_request "type" field values. */
#define ALERT_OT_REQUEST      0
#define USER_OT_REQUEST       1

#define REQ_ALL_ELEVS        -1
#define REQ_NOT_SCHEDLD      -2

typedef struct {

    short pid;		/* product id (Buffer number) */
    short param_1;	/* Product dependent parameter 1 */
    short param_2;	/* Product dependent parameter 2 */
    short param_3;	/* Product dependent parameter 3 */
    short param_4;	/* Product dependent parameter 4 */
    short param_5;	/* Product dependent parameter 5 */
    short param_6;	/* Product dependent parameter 6 */
    short elev_ind;	/* elevation index; -1 = all elevations, -2 = not scheduled */
    short req_num;	/* a unique request sequence number */
    short type;		/* Product request type ... Either ALERT_OT_REQUEST or
			   USER_OT_REQUEST */
    unsigned int vol_seq_num;	/* Volume scan sequence number */
    
} Prod_request;

/* requests for each product are stored in the product request LB as a 
   replaceable message with LB id = pid. The request list in the message
   is considered terminated if Prod_request.pid != LB message id.
   Since the message size can not change for a replaceable LB, the message 
   size allocated for a product must have its maximum size, which varies 
   for products and is set at the system configuration time.
*/

/* size of the product request structure */
#define PREQ_SIZE	10

/* field offsets in the structure */
#define PREQ_PID	0
#define PREQ_WIN_AZI	1
#define PREQ_END_TIME   PREQ_WIN_AZI
#define PREQ_WIN_RANGE	2
#define PREQ_DURATION   PREQ_WIN_RANGE
#define PREQ_ELAZ	3
#define PREQ_STORM_SPEED	4
#define PREQ_STORM_DIR	5
#define PREQ_SPARE	6
#define PREQ_ELEV_IND	7
#define PREQ_REQ_NUM	8
#define PREQ_RESERVED	9

#endif

