C
C RCS info
C $Author: ccalvert $
C $Locker:  $
C $Date: 2011/09/19 14:46:26 $
C $Id: orpgdat.inc,v 1.26 2011/09/19 14:46:26 ccalvert Exp $
C $Revision: 1.26 $
C $State: Exp $
C

**ORPGDAT
C
C**************************************************************************
C
C     Module: orpdat.inc
C
C  Description: Global Open Systems Radar Product Generator (ORPG) data
C               include file.
C       Notes: All constants INTEGERd in this file begin with the prefix
C              ORPGDAT_.
C**************************************************************************

C* This must be larger than any defined RPG buffer type numbers
      INTEGER ORPGDAT_BASE
      PARAMETER( ORPGDAT_BASE =	3000 )

C* column number of the LB names in the system configuration text
      INTEGER ORPGSC_LBNAME_COL	
      PARAMETER( ORPGSC_LBNAME_COL = 1 )

C* adaptation data LB
      INTEGER ORPGDAT_ADAPTATION
      PARAMETER ( ORPGDAT_ADAPTATION = 3000 )

C* product request LB 
      INTEGER ORPGDAT_PROD_REQUESTS
      PARAMETER ( ORPGDAT_PROD_REQUESTS = 3001 )

C* product generation message LB 
      INTEGER ORPGDAT_PROD_GEN_MSGS
      PARAMETER ( ORPGDAT_PROD_GEN_MSGS = 3002 )

C* RDA base data LB 
      INTEGER ORPGDAT_SCAN_SUMMARY
      PARAMETER ( ORPGDAT_SCAN_SUMMARY = 3003 )

C* Occultation Adaptation Data File 
      INTEGER ORPGDAT_HYOCCULT
      PARAMETER ( ORPGDAT_HYOCCULT = 3004 )

C* Hybrid Sectors Adaptation Data File
      INTEGER ORPGDAT_HYSECTRS
      PARAMETER ( ORPGDAT_HYSECTRS = 3005 )

C* Volume and elevation accounting data 
      INTEGER ORPGDAT_ACCDATA
      PARAMETER ( ORPGDAT_ACCDATA = 3006 )

C* RDA Status data, volume-based status, and RPG status 
C* data for General Status Message. 
      INTEGER ORPGDAT_RDA_STATUS
      PARAMETER ( ORPGDAT_RDA_STATUS = 3007 )
      INTEGER ORPGDAT_GSM_DATA
      PARAMETER ( ORPGDAT_GSM_DATA = ORPGDAT_RDA_STATUS )

C* Clutter Filter data 
      INTEGER ORPGDAT_CLUTTERMAP    
      PARAMETER ( ORPGDAT_CLUTTERMAP = 3008 )
      INTEGER LBID_BYPASSMAP_LGCY, LBID_BYPASSMAP_ORDA
      INTEGER LBID_EDBYPASSMAP_LGCY, LBID_EDBYPASSMAP_ORDA
      INTEGER LBID_CLUTTERMAP_LGCY, LBID_CLUTTERMAP_ORDA
      INTEGER LBID_CENSOR_ZONES_LGCY, LBID_CENSOR_ZONES_ORDA
      INTEGER LBID_BASELINE_CENSOR_ZONES_LGCY
      INTEGER LBID_BASELINE_CENSOR_ZONES_ORDA
      PARAMETER( LBID_CLUTTERMAP_LGCY = 0 )
      PARAMETER( LBID_BYPASSMAP_LGCY  = 1 )
      PARAMETER( LBID_CENSOR_ZONES_LGCY  = 2 )
      PARAMETER( LBID_EDBYPASSMAP_LGCY  = 3 )
      PARAMETER( LBID_BASELINE_CENSOR_ZONES_LGCY  = 4 )
      PARAMETER( LBID_CLUTTERMAP_ORDA = 5 )
      PARAMETER( LBID_EDBYPASSMAP_ORDA  = 6 )
      PARAMETER( LBID_CENSOR_ZONES_ORDA  = 7 )
      PARAMETER( LBID_BYPASSMAP_ORDA  = 8 )
      PARAMETER( LBID_BASELINE_CENSOR_ZONES_ORDA  = 9 )

C* RDA Alarms data 
      INTEGER ORPGDAT_RDA_ALARMS
      PARAMETER ( ORPGDAT_RDA_ALARMS = 3010 )

C* RDA Command data 
      INTEGER ORPGDAT_RDA_COMMAND
      PARAMETER ( ORPGDAT_RDA_COMMAND = 3011 )

C* RDA Performance/Maintenance data 
      INTEGER ORPGDAT_RDA_PERF_MAIN
      PARAMETER ( ORPGDAT_RDA_PERF_MAIN = 3012 )

C* Weather Alert Request Message
      INTEGER ORPGDAT_WX_ALERT_REQ_MSG
      PARAMETER ( ORPGDAT_WX_ALERT_REQ_MSG = 3016 )

C* Hydromet Terrain Data
      INTEGER ORPGDAT_TERRAIN
      PARAMETER ( ORPGDAT_TERRAIN = 3017 )

C* Hydromet Blockage Data
      INTEGER ORPGDAT_BLOCKAGE
      PARAMETER ( ORPGDAT_BLOCKAGE = 3018 )

C* Node Name and Software Version Data
      INTEGER ORPGDAT_NODE_VERSION
      PARAMETER ( ORPGDAT_NODE_VERSION = 3054 )

C* Information regarding Archive II
      INTEGER ORPGDAT_ARCHIVE_II_INFO
      PARAMETER ( ORPGDAT_ARCHIVE_II_INFO = 3055 )

C* HCI data 
      INTEGER ORPGDAT_HCI_DATA
      PARAMETER ( ORPGDAT_HCI_DATA = 3099 )

C* product generation and distribution info (adaptation) LB
      INTEGER ORPGDAT_PROD_INFO
      PARAMETER ( ORPGDAT_PROD_INFO = 4000 )

C* product status LB updated by ps_routine 
      INTEGER ORPGDAT_PROD_STATUS 
      PARAMETER ( ORPGDAT_PROD_STATUS = 4001 )

C* comm_manager request data store. We need to reserve 100 number
      INTEGER ORPGDAT_CM_REQUEST
      PARAMETER ( ORPGDAT_CM_REQUEST = 4010 )

C* comm_manager response data store. We need to reserve 100 number 
      INTEGER ORPGDAT_CM_RESPONSE
      PARAMETER ( ORPGDAT_CM_RESPONSE = 5010 )

C* one-time product scheduler response data store. We need to 
C* reserve 100 number 
      INTEGER ORPGDAT_OT_RESPONSE
      PARAMETER ( ORPGDAT_OT_RESPONSE = 6011 )

C* routine product scheduler response data store. We need to 
C* reserve 100 number/
      INTEGER ORPGDAT_RT_RESPONSE
      PARAMETER ( ORPGDAT_RT_RESPONSE = 7011 )

C* data store for messages from p_server to ps_onetime 
      INTEGER ORPGDAT_OT_REQUEST
      PARAMETER ( ORPGDAT_OT_REQUEST = 8011 )

C* data store for messages from p_server to ps_routine 
      INTEGER ORPGDAT_RT_REQUEST
      PARAMETER ( ORPGDAT_RT_REQUEST = 8012 )

C* data store for environmental data
      INTEGER ORPGDAT_ENVIRON_DATA_MSG
      PARAMETER ( ORPGDAT_ENVIRON_DATA_MSG = 10002 )

C* data store for Gage Database data
      INTEGER ORPGDAT_GAGEDATA, GAGEDATA
      PARAMETER ( ORPGDAT_GAGEDATA = 10003 )
      PARAMETER ( GAGEDATA = ORPGDAT_GAGEDATA )

C* data store for User Selectable Precipitation Database data
      INTEGER ORPGDAT_HYUSRSEL, HYUSRSEL
      PARAMETER ( ORPGDAT_HYUSRSEL = 10004 )
      PARAMETER ( HYUSRSEL = ORPGDAT_HYUSRSEL )

C* data store for Snow Accumulation Algorithm User Selectable product
      INTEGER SAAUSERSEL
      PARAMETER( SAAUSERSEL = 300000 )
/*
