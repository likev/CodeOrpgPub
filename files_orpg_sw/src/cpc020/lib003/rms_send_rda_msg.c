/**************************************************************************

   Module: rms_send_rda_msg.c

   Description:  This module write RDA command to the LB and posts the event.


   Assumptions:

   **************************************************************************/
/*
 * RCS info
 * $Author: dans $
 * $Locker:  $
 * $Date: 2001/04/16 18:33:14 $
 * $Id: rms_send_rda_msg.c,v 1.4 2001/04/16 18:33:14 dans Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */
/*
* System Include Files/Local Include Files
*/
#include <rms_message.h>

/*
* Constant Definitions/Macro Definitions/Type Definitions
*/
#define MAX_NAME_SIZE		256

/*
* Static Globals
*/


/*
* Static Function Prototypes
*/

/**************************************************************************
   Description:  This function receives RDA command information and places
   it into the RDA command buffer and posts the event.

   Input: command - The command to be executed
   	  line_number - Line number
   	  parameter1 - First parameter
   	  parameter2 - Second parameter
   	  parameter3 - Third parameter
      	  message - Text message if any

   Output:  RDA command placed in RDA command buffer


   Returns:

   Notes:

   **************************************************************************/
int rms_send_RDA_command (
int	command,
int	line_number,
int	parameter1,
int	parameter2,
int	parameter3,
char	*message
)
{

	rda_command_t	rda_command;
	int		status, Lb_fd;
	char 		path[MAX_NAME_SIZE];

	/*Define the elemants of the RDA control command structure.*/
	rda_command.command      = command;
	rda_command.line_number  = line_number;
	rda_command.parameter_1  = parameter1;
	rda_command.parameter_2  = parameter2;
	rda_command.parameter_3  = parameter3;



	if (message != NULL) {

	    strcpy (rda_command.message_text, message);

	}



/*	Write the command buffer to the RDA control command linear	*
 *	buffer.								*/


	status = ORPGDA_write (ORPGDAT_RDA_COMMAND,
			(char *) &rda_command,
			sizeof (rda_command_t),
			LB_ANY);


	if (status != sizeof(rda_command_t)) {
		LE_send_msg (RMS_LE_ERROR,
			"RMS: LB_write rms send rda failed (ret %d)", status);
		return (-1);
    		}


/*	Since the write to the LB was successfull, post an event so	*
 *	the RDA comm manager knows to read it.				*/

	status = EN_post (ORPGEVT_RDA_CONTROL_COMMAND,
			  NULL, 0, 0);

	EN_post (ORPGEVT_RDA_STATUS_CHANGE,NULL, 0, 0);
	return (1);

} /* End of rms send RDA command */
