C
C RCS info
C $Author: steves $
C $Locker:  $
C $Date: 2011/05/05 19:15:23 $
C $Id: rpg_port.inc,v 1.33 2011/05/05 19:15:23 steves Exp $
C $Revision: 1.33 $
C $State: Exp $
C

/***********************************************************************

	This file defines the macros that are needed by the ported RPG
	tasks.

	The contents in this file are derived from rpg_port.h. The macros 
	must be consistent with those defined there. Thus if rpg_port.h is 
	modified, this file has to be updated accordingly.

***********************************************************************/


**rpg_port

C	/* the following is used by FORTRAN programs */

	INTEGER RDACNT_ID, PRODSEL_ID
	INTEGER COLRTBL_ID, ENVIRON_ID

	PARAMETER (RDACNT_ID =  1)
	PARAMETER (PRODSEL_ID =  3)
	PARAMETER (COLRTBL_ID =  7)
	PARAMETER (ENVIRON_ID =  8)

C	/* task types */
	INTEGER TASK_ELEVATION_BASED, TASK_VOLUME_BASED
	INTEGER TASK_TIME_BASED, TASK_RADIAL_BASED, TASK_EVENT_BASED
	PARAMETER (TASK_ELEVATION_BASED = 1)
	PARAMETER (TASK_VOLUME_BASED = 2)
	PARAMETER (TASK_TIME_BASED = 3)
	PARAMETER (TASK_RADIAL_BASED = 4)
	PARAMETER (TASK_EVENT_BASED = 5)

C       /* these are the old macro names */
	INTEGER ELEVATION_BASED, VOLUME_BASED
	INTEGER TIME_BASED, RADIAL_BASED, EVENT_BASED
	PARAMETER (ELEVATION_BASED = TASK_ELEVATION_BASED)
	PARAMETER (VOLUME_BASED = TASK_VOLUME_BASED)
	PARAMETER (TIME_BASED = TASK_TIME_BASED)
	PARAMETER (RADIAL_BASED = TASK_RADIAL_BASED)
	PARAMETER (EVENT_BASED = TASK_EVENT_BASED)

C       /* Product code for intermediate product data */
        INTEGER INT_PROD
        PARAMETER( INT_PROD = 0 )

C	/* adaptation timing values */
	INTEGER ADPT_UPDATE_BOE, ADPT_UPDATE_BOV
        INTEGER ADPT_UPDATE_ON_CHANGE
        INTEGER ADPT_UPDATE_WITH_CALL
        INTEGER ADPT_UPDATE_ON_EVENT
	PARAMETER (ADPT_UPDATE_BOE = 1)
	PARAMETER (ADPT_UPDATE_BOV = 2)
	PARAMETER (ADPT_UPDATE_ON_CHANGE = 3)
        PARAMETER (ADPT_UPDATE_WITH_CALL = 4)
        PARAMETER (ADPT_UPDATE_ON_EVENT = 5)

C       /* these are the old parameter names */
	INTEGER BEGIN_ELEVATION, BEGIN_VOLUME
        INTEGER ON_CHANGE, WITH_CALL, WITH_EVENT
	PARAMETER (BEGIN_ELEVATION = ADPT_UPDATE_BOE)
	PARAMETER (BEGIN_VOLUME = ADPT_UPDATE_BOV)
	PARAMETER (ON_CHANGE = ADPT_UPDATE_ON_CHANGE)
        PARAMETER (WITH_CALL = ADPT_UPDATE_WITH_CALL)
        PARAMETER (WITH_EVENT = ADPT_UPDATE_ON_EVENT)

C	/* ITC timing values */
	INTEGER ITC_BEGIN_ELEVATION, ITC_BEGIN_VOLUME
        INTEGER ITC_ON_EVENT, ITC_ON_CALL, ITC_WITH_EVENT
	PARAMETER (ITC_WITH_EVENT = -5)
	PARAMETER (ITC_ON_EVENT = -4)
	PARAMETER (ITC_BEGIN_ELEVATION = -3)
	PARAMETER (ITC_BEGIN_VOLUME = -2)
	PARAMETER (ITC_ON_CALL = -1)

C       /* ITC operations */
        INTEGER ITC_READ_OPERATION, ITC_WRITE_OPERATION
        PARAMETER( ITC_READ_OPERATION = 0 )
        PARAMETER( ITC_WRITE_OPERATION = 1 )

C	/* input/output data types .... "type" field in PAT. */
	INTEGER ELEVATION_DATA, VOLUME_DATA, RADIAL_DATA
	INTEGER UNDEFINED_DATA, DEMAND_DATA
        INTEGER REQUEST_DATA, EXTERNAL_DATA, TIME_DATA
	PARAMETER (UNDEFINED_DATA = -1)
	PARAMETER (VOLUME_DATA = 0)
	PARAMETER (ELEVATION_DATA = 1)
        PARAMETER (TIME_DATA = 2)
	PARAMETER (DEMAND_DATA = 3)
        PARAMETER (REQUEST_DATA = 4)
	PARAMETER (RADIAL_DATA = 5)
	PARAMETER (EXTERNAL_DATA = 6)

C	/* argument for RPG_wait_act */
	INTEGER WAIT_DRIVING_INPUT, WAIT_ANY_INPUT, WAIT_ALL
	PARAMETER (WAIT_DRIVING_INPUT = 0)
	PARAMETER (WAIT_ANY_INPUT = 1)
	PARAMETER (WAIT_ALL = WAIT_DRIVING_INPUT)

C	/* range for the msg id of the ITC */
	INTEGER ITC_IDRANGE, ITC_MIN
	PARAMETER (ITC_IDRANGE = 100)
C	/* minimum LB id of the ITC */
	PARAMETER (ITC_MIN = 1000)

C       /* types of events. */
        INTEGER EVT_ANY_INPUT_AVAILABLE
        PARAMETER (EVT_ANY_INPUT_AVAILABLE = -1)
        INTEGER EVT_CFCPROD_REPLAY_PRODUCT_REQUEST
        PARAMETER (EVT_CFCPROD_REPLAY_PRODUCT_REQUEST = -2)
        INTEGER EVT_USER_NOTIFICATION
        PARAMETER (EVT_USER_NOTIFICATION = -3)
        INTEGER EVT_WAIT_FOR_EVENT_TIMEOUT
        PARAMETER (EVT_WAIT_FOR_EVENT_TIMEOUT = -4)

C       /* parameter definition for "user_array" */
        INTEGER UA_NUM_FIELDS, UA_NUM_PARMS
        INTEGER UA_PROD_CODE
        INTEGER UA_DEP_PARM_1
        INTEGER UA_DEP_PARM_2
        INTEGER UA_DEP_PARM_3
        INTEGER UA_DEP_PARM_4
        INTEGER UA_DEP_PARM_5
        INTEGER UA_DEP_PARM_6
        INTEGER UA_ELEV_INDEX
        INTEGER UA_REQ_NUMBER
        INTEGER UA_SPARE

        PARAMETER( UA_NUM_FIELDS = 10, UA_NUM_PARMS = 6 )
        PARAMETER( UA_PROD_CODE = 1 )
        PARAMETER( UA_DEP_PARM_1 = 2 )
        PARAMETER( UA_DEP_PARM_2 = 3 )
        PARAMETER( UA_DEP_PARM_3 = 4 )
        PARAMETER( UA_DEP_PARM_4 = 5 )
        PARAMETER( UA_DEP_PARM_5 = 6 )
        PARAMETER( UA_DEP_PARM_6 = 7 )
        PARAMETER( UA_ELEV_INDEX = 8 )
        PARAMETER( UA_REQ_NUMBER = 9 )
        PARAMETER( UA_SPARE = 10 )

C       /* parameter definitions for dependent parameter special flags. */
        INTEGER PARAM_UNUSED
        INTEGER PARAM_ANY_VALUE
        INTEGER PARAM_ALG_SET
        INTEGER PARAM_ALL_VALUES
        INTEGER PARAM_ALL_EXISTING

        PARAMETER( PARAM_UNUSED = -32768 )
        PARAMETER( PARAM_ANY_VALUE = -32767 )
        PARAMETER( PARAM_ALG_SET = -32766 )
        PARAMETER( PARAM_ALL_VALUES = -32765 )
        PARAMETER( PARAM_ALL_EXISTING = -32764 )

C       /* support for product compression. */
        INTEGER BZIP2_COMPRESSION
        PARAMETER( BZIP2_COMPRESSION = 1 )

C       /* support for rel_outbuf */
        INTEGER RPG_BUF_NOT_FOUND
        PARAMETER( RPG_BUF_NOT_FOUND = -1)

C       /* support for what_moments */
        INTEGER UNSPECIFIED_MOMENTS
        INTEGER REF_MOMENT
        INTEGER VEL_MOMENT
        INTEGER WID_MOMENT 

        PARAMETER( UNSPECIFIED_MOMENTS = 0 )
        PARAMETER( REF_MOMENT = 1 )
        PARAMETER( VEL_MOMENT = 2 )
        PARAMETER( WID_MOMENT = 4 )

        INTEGER RPG_END_TIME, RPG_START_TIME
        PARAMETER( RPG_START_TIME = 1 )
        PARAMETER( RPG_END_TIME = 2 )

/* */

