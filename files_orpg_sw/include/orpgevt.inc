XCOMM RCS info 
XCOMM $Author: steves $ 
XCOMM $Locker:  $ 
XCOMM $Date: 2004/03/04 22:08:04 $ 
XCOMM $Id: orpgevt.inc,v 1.16 2004/03/04 22:08:04 steves Exp $ 
XCOMM $Revision: 1.16 $ 
XCOMM $State: Exp $ 
**ORPGEVT
C**************************************************************************
C
C     Module: orpgevt.inc
C
C Description: Global Open Systems Radar Product Generator (ORPG) events
C             include file.
C      Notes: All constants defined in this file begin with the prefix
C             ORPGEVT_.
C**************************************************************************
C*
C* Global ORPG Event Codes
C*
C* ORPG Configuration Change            
      integer  ORPGEVT_CFG_CHANGE
      parameter( ORPGEVT_CFG_CHANGE = 17 )

C* Used by orpg library to indicate no event registered. 
      integer ORPGEVT_NULL_EVENT
      parameter( ORPGEVT_NULL_EVENT = 0 )

C* Event code to be used to indicate that new RDA control command data 
C* was put in the RDA command linear buffer 
      integer ORPGEVT_RDA_CONTROL_COMMAND
      parameter( ORPGEVT_RDA_CONTROL_COMMAND = 100 )

C* Event code to be used to indicate that a CPC4MSG was released which
C* has an associated event 
      integer ORPGEVT_CPC4MSG
      parameter( ORPGEVT_CPC4MSG = 101 )

C* Event code to be used to indicate that RDA status data changed from
C* last message received
      integer ORPGEVT_RDA_STATUS_CHANGE
      parameter( ORPGEVT_RDA_STATUS_CHANGE = 102 )

C* Event code to be used to indicate that a start of volume, start of
C* elevation, end of volume, or end of elevation was detected.
      integer ORPGEVT_SCAN_INFO
      parameter( ORPGEVT_SCAN_INFO = 103 )

C* Event code to be used to indicate that a bypass map was received.
      integer ORPGEVT_BYPASS_MAP_RECEIVED
      parameter( ORPGEVT_BYPASS_MAP_RECEIVED = 105 )

C* Event code to be used to indicate that a clutter filter map was received.
      integer ORPGEVT_CLUTTER_MAP_RECEIVED
      parameter( ORPGEVT_CLUTTER_MAP_RECEIVED = 106 )

C* Event code to be used to indicate start of volume scan.
      integer ORPGEVT_START_OF_VOLUME
      parameter( ORPGEVT_START_OF_VOLUME = 107 )

C* Event code to be used to indicate that RDA Performance/Maintence data
C* was received.
      integer ORPGEVT_PERF_MAIN_RECEIVED
      parameter( ORPGEVT_PERF_MAIN_RECEIVED = 108 )

C* Event code to be used to indicate a Precipitation Category Change 
      integer ORPGEVT_PRECIP_CAT_CHNG
      parameter( ORPGEVT_PRECIP_CAT_CHNG = 112 )

C* Event code to be used to indicate alert message message generated. 
      integer ORPGEVT_WX_ALERT_MESSAGE
      parameter ( ORPGEVT_WX_ALERT_MESSAGE = 115 )


C* Event code to be used to indicate alert paired product generation
C* message. 
      integer ORPGEVT_WX_ALERT_OT_REQ
      parameter ( ORPGEVT_WX_ALERT_OT_REQ = 116 )

C* Event code to be used to indicate user alert message message 
C* generated. 
      integer ORPGEVT_WX_USER_ALERT_MSG
      parameter ( ORPGEVT_WX_USER_ALERT_MSG = 117 )

C* Event code to be used to indicate environmental wind table has
C* changed (either data or VAD update flag).
      integer ORPGEVT_ENVWND_UPDATE
      parameter( ORPGEVT_ENVWND_UPDATE = 600 )

C* Event code to be used to reinitialize hydromet files
      integer ORPGEVT_TERM_PRECIP_ALGS
      parameter( ORPGEVT_TERM_PRECIP_ALGS = 607 )

/*

