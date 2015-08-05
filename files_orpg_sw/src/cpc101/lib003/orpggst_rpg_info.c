/*
 * RCS info
 * $Author: nolitam $
 * $Locker:  $
 * $Date: 2002/12/11 21:12:54 $
 * $Id: orpggst_rpg_info.c,v 1.5 2002/12/11 21:12:54 nolitam Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */
/**************************************************************************
   
   Module: orpggst_rpg_info.c 
   
   Description:  This is the module for retrieving rpg status.
   

   Assumptions:  
      
   **************************************************************************/

/*
* System Include Files/Local Include Files
*/

#include <orpggst.h>

/*
* Constant Definitions/Macro Definitions/Type Definitions
*/

/*
* Static Globals
*/


/*
* Static Function Prototypes
*/
int ORPGGST_get_rpg_stats(Orpginfo_statefl_t *rpg_stats_p);

/**************************************************************************
   Description:  This function retrieves The orpg information.
      
   Input: 
      
   Output:  ORPG status message.
   
   Returns: 
   
   Notes:  

   **************************************************************************/
int ORPGGST_get_rpg_stats(Orpginfo_statefl_t *rpg_stats_p ){  

	int ret;
	
	ret = ORPGDA_read(ORPGDAT_RPG_INFO,&rpg_stats_p,ORPGINFO_STATEFL_SIZE,ORPGINFO_STATEFL_MSGID);    
        
        if (ret < 0) {
            LE_send_msg(GL_INFO,
                        "State File RPG Status GET failed: ret %d", ret) ;
                        return (-1);
        	}    
      	
      	
       	return (1);	
       		
}/* End ORPGGST get rpg stats*/

/**************************************************************************
   Description:  This function retrieves rpg operational status.
      
   Input: 
      
   Output:  
   
   Returns: 
   
   Notes:  

   **************************************************************************/

unsigned int ORPGGST_get_rpg_status(int request){  

	Orpginfo_statefl_t rpg_op_stats_p ;
	int ret;	
	
	ret = ORPGGST_get_rpg_stats(&rpg_op_stats_p);
	
	if( ret < 0){
		LE_send_msg(GL_INFO,
                        "State File RPG Operational Status GET failed") ;
                        return (0);   
        		}
        switch (request){
        
        	case RPG_OPERATIONAL_STATUS:		
       			return (rpg_op_stats_p.rpg_op_status);
       			break;
       			
       		case RPG_STATE:		
       			return (rpg_op_stats_p.rpg_status);
       			break;
       			
       		case RPG_ALARMS:		
       			return (rpg_op_stats_p.rpg_alarms);
       			break;
       		}
       		
       		return (0);
}/* End ORPGGST get rpg status*/


