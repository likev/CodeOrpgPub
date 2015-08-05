/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2003/02/20 17:47:10 $
 * $Id: orpggst_rda_info.c,v 1.5 2003/02/20 17:47:10 steves Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */

/**************************************************************************
   
   Module: ordagst_rda_info.c 
   
   Description:  This is the module for retrieving rda status.
   

   Assumptions:  
      
   **************************************************************************/

/*
* System Include Files/Local Include Files
*/ 

#include <orpggst.h>
#include <rda_alarm_table.h>

/*
* Constant Definitions/Macro Definitions/Type Definitions
*/
#define	MAJOR	27
#define MINOR	25
#define FUSE	26
#define REMOTE	28
/*
* Static Globals
*/
static	ALIGNED_t rda_stats[ALIGNED_T_SIZE (sizeof (RDA_status_t))];
static  int 	first_time;
static  int	wb_alarm_1;
static	int	wb_alarm_2;
/*
* Static Function Prototypes
*/

/**************************************************************************
   Description:  This function retrieves The orda information.
      
   Input: 
      
   Output:  ORPG status message.
   
   Returns: 
   
   Notes:  

   **************************************************************************/
RDA_status_t  *ORPGGST_get_rda_stats(){  

	RDA_status_t *rda_stats_p ;
		
	int ret;
	
	/* Retrieve RDA status */
	 ret = ORPGDA_read (ORPGDAT_GSM_DATA,
			(char *) &rda_stats,
			sizeof (RDA_status_t),
			RDA_STATUS_ID);
			
	if (ret != sizeof (RDA_status_t)) {
		LE_send_msg (GL_ORPGDA (ret), 
	  "ORPGDA_read failed (RDA_STATUS) in status message (ret %d)",ret);
	  return (NULL);
          }		
  	
  	rda_stats_p = (RDA_status_t*)&rda_stats;
  			
       	return (rda_stats_p);	
       		
}/* End ORPGGST get rda stats*/

/**************************************************************************
   Description:  This function retrieves The wb alarm information.
      
   Input: 
      
   Output:  ORPG status message.
   
   Returns: 
   
   Notes:  

   **************************************************************************/
 int ORPGGST_get_wb_alarm(int wb_num){  

	RDA_alarm_t *wb_alarm_stat ;
	ALIGNED_t data[ALIGNED_T_SIZE (sizeof (RDA_alarm_t))];
			
	int ret = 1;
	int lbfd;
	
	/* If this is the first time through the data base then set the pointer
	   to the beginning of the LB to read all the messages. */
	   
	if(first_time == 0){
	
		/*Get the current file descriptor for the RDA Alarm LB*/

	    	lbfd = ORPGDA_lbfd (ORPGDAT_RDA_ALARMS);

		/*Position the cursor to the beginning of the file so we can	
 		 read all o the stored messages.*/

	   	 LB_seek (lbfd,
			 0,
			 LB_FIRST,
		 	NULL);
		 	
		 first_time = 1;
		 
		}
		
	/*Loop until no more unread messages are found or if an error
 	occurred.*/
	
 	/* Wideband #1 alarm check */
	if(wb_num == 1){
	
				
		while (ret > 0) {

	    		ret = ORPGDA_read (ORPGDAT_RDA_ALARMS,
				(char *) &data,
				sizeof (RDA_alarm_t),
				LB_NEXT);
				
			wb_alarm_stat = (RDA_alarm_t*)data;
			
			switch (wb_alarm_stat->alarm){
		
				case MINOR:
					if(wb_alarm_stat->code == 1 )
						wb_alarm_1 = 1;
						
					if(wb_alarm_stat->code == 0 && wb_alarm_1 == 1)
						wb_alarm_1 = 0;
						
					break;
				case FUSE:
					if(wb_alarm_stat->code == 1 )
						wb_alarm_1 = 4;
						
					if(wb_alarm_stat->code == 0 && wb_alarm_1 == 4 )
						wb_alarm_1 = 0;	
					
					break;		
  				case MAJOR:
					if(wb_alarm_stat->code == 1 )
						wb_alarm_1 = 2;
						
					if(wb_alarm_stat->code == 0 && wb_alarm_1 == 2)
						wb_alarm_1 = 0;	
					
					break;
  				case REMOTE:
					if(wb_alarm_stat->code == 1 )
						wb_alarm_1 = 3;
						
					if(wb_alarm_stat->code == 0 && wb_alarm_1 == 3)
						wb_alarm_1 = 0;	
					
					break;
  				}/* End switch*/
  			
  			}/* End loop */
  			
  			return (wb_alarm_1);
  			
  		}/* End if */
  		
  	/* Wideband #2 alarm check - to be coded when ORPG redundant is designed */	
  	else{
  		wb_alarm_2 = 0;
  		
  		return (wb_alarm_2);
  		}
  		
  	
  		
       	       		
}/* End ORPGGST get rda stats*/

