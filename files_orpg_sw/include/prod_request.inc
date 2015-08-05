C
C RCS info
C $Author: ryans $
C $Locker:  $
C $Date: 2003/03/04 20:10:42 $
C $Id: prod_request.inc,v 1.4 2003/03/04 20:10:42 ryans Exp $
C $Revision: 1.4 $
C $State: Exp $
C

/**********************************************************************

	Header file defining the product requests.

**********************************************************************/


**product_request

C	size of the product request structure
	integer PREQ_SIZE
	parameter (PREQ_SIZE = 10)

C	field offsets in the structure
	integer PREQ_PID
	parameter (PREQ_PID = 1)
	integer PREQ_WIN_AZI
	parameter (PREQ_WIN_AZI = 2)
	integer PREQ_WIN_RANGE
	parameter (PREQ_WIN_RANGE = 3)
	integer PREQ_ELAZ
	parameter (PREQ_ELAZ = 4)
	integer PREQ_STORM_SPEED
	parameter (PREQ_STORM_SPEED = 5)
	integer PREQ_STORM_DIR
	parameter (PREQ_STORM_DIR = 6)
	integer PREQ_SPARE
	parameter (PREQ_SPARE = 7)
	integer PREQ_ELEV_IND
	parameter (PREQ_ELEV_IND = 8)
	integer PREQ_REQ_NUM
	parameter (PREQ_REQ_NUM = 9)
	integer PREQ_RESERVED
	parameter (PREQ_RESERVED = 10)


/* */
