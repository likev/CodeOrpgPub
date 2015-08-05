/********************************************************************************
 
            file:  mngred_comms_relay.c

     Description:  This file contains the routines that manage the comms
                   relay

 ********************************************************************************/
/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2013/02/12 16:21:52 $
 * $Id: mngred_comms_relay.c,v 1.5 2013/02/12 16:21:52 steves Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */


#include <fcntl.h>
#include <stropts.h>
#include <unistd.h>

#include <mngred_globals.h>


   /* define the digital I/O card device driver macros */

#define   DIO_IOC                   ('D' << 8)
#define   DIO_READ_RELAY_STATE      (DIO_IOC | 1)   /* cmd to read the relay state */
#define   DIO_SET_RELAY             (DIO_IOC | 2)   /* cmd to set the relay state  */
#define   DIO_SWITCH_RELAY_STATE    (DIO_IOC | 3)   /* cmd to switch relay states  */

#define   DIO_DEVICE                "/dev/dio"      /* the DIO device driver       */

#define   ACQUIRE_RELAY             1               /* Unit/Integration test flag  */
#define   READ_RELAY                0               /* Unit/Integration test flag  */


/* file scope variables */

int Dio_fd;   /* file descriptor for the DIO card */


   /* Test function */
static Comms_relay_state_t Uit_comms_relay (int control_function);



/********************************************************************************

    Description: This routine acquires the comms relay

          Input:

         Output:

         Return: relay_state --  state of the comms relay

        Globals:

          Notes:

 ********************************************************************************/

Comms_relay_state_t CR_acquire_comms_relay (void)
{
   Comms_relay_state_t relay_state;              /* current state of the comms relay */
   static int acquired_alarm_set = MNGRED_FALSE; /* acquired alarm flag used to 
                                                    write a system alarm msg */

   /* if the unit/integration test flag is set, then return the test 
      condition state */

#ifdef MNGRED_UIT
   int ret_val;
   ret_val = Uit_comms_relay(ACQUIRE_RELAY);
   return (ret_val);
#endif

      /* check the relay state before commanding the relay to switch */

   relay_state = CR_read_comms_relay_state ();

      /* switch the relay if not assigned */

   if (relay_state == ORPGRED_COMMS_RELAY_UNASSIGNED)
   {
      ioctl (Dio_fd, DIO_SWITCH_RELAY_STATE, NULL);
      relay_state = CR_read_comms_relay_state ();
   }

      /* if the command to acquire the comms relay failed, then write 
         a status alarm msg */

   if (relay_state != ORPGRED_COMMS_RELAY_ASSIGNED)
   {
      if (acquired_alarm_set == MNGRED_FALSE)
      {
         LE_send_msg (GL_STATUS | LE_RPG_WARN_STATUS, 
                      "%s Failed to acquire the NB/WB Comms Relay",
                      MNGRED_WARN_ACTIVE);
         acquired_alarm_set = MNGRED_TRUE;
      }
   }
      /* clear the acquired alarm msg and local flag it it was previously set */
   else if (acquired_alarm_set == MNGRED_TRUE)
   {
      LE_send_msg (GL_STATUS, "%s Failed to acquire the NB/WB Comms Relay",
                   MNGRED_WARN_CLEAR);
      acquired_alarm_set = MNGRED_FALSE;
   }

   return (relay_state);
}


/********************************************************************************

    Description: This routine opens the DIO device

          Input:

         Output:

         Return: 0 if DIO card successfully opened or system error code on error

        Globals: Dio_fd - see file scope global section

          Notes:

 ********************************************************************************/

int CR_open_dio_card (void)
{

   /* if the unit/integration test flag is set, then return the test 
      condition state */

#ifdef MNGRED_UIT
   LE_send_msg (MNGRED_OP_VL,
                "CR_open_dio_card: UIT test code executed...open the  DIO device");
   return (0);
#endif

   Dio_fd = open (DIO_DEVICE, O_RDWR, 0);

   if (Dio_fd < 0)
      return (Dio_fd);
   else
      return (0);
}


/********************************************************************************

    Description: This routine reads the state of the comms relay

          Input:

         Output:

         Return: current_state - the current state of the comms relay

        Globals:

          Notes:

 ********************************************************************************/

Comms_relay_state_t CR_read_comms_relay_state (void)
{
   unsigned int register1;                       /* DIO card's register 1 value */
   int return_value;                         /* return value from ioctl */
   Comms_relay_state_t current_state;        /* current state of the comms relay */
   static int read_alarm_set = MNGRED_FALSE; /* flag specifying that a relay 
                                                read cmd returned "unknown" */

   /* if the unit/integration test flag is set, then return the test 
      condition state */

#ifdef MNGRED_UIT
   int ret_val;
   ret_val = Uit_comms_relay(READ_RELAY);
   return (ret_val);
#endif

   return_value = ioctl (Dio_fd, DIO_READ_RELAY_STATE, &register1);

      /* if a negative # is passed back from ioctl, then the relay state
         is unknown */

   if (return_value < 0)
   {
      current_state = ORPGRED_COMMS_RELAY_UNKNOWN;

         /* write a read relay status alarm msg if one has not been 
            previously written */

      if (read_alarm_set == MNGRED_FALSE)
      {
         LE_send_msg (GL_STATUS | LE_RPG_WARN_STATUS, 
            "%s Failure reading NB/WB comms relay (relay state is \"Unknown\")",
            MNGRED_WARN_ACTIVE);
         read_alarm_set = MNGRED_TRUE;
      }
   }
   else
   {
          /* truncate register to 16 bits */
      
      register1 = register1 & 0xffff;
      
          /* if bit 0 is set, then the relay is not in control */

      if (register1 & 0x1)
          current_state = ORPGRED_COMMS_RELAY_UNASSIGNED;
      else
          current_state = ORPGRED_COMMS_RELAY_ASSIGNED;

         /* if a system read relay alarm msg has been previously 
            written, then clear it */

      if (read_alarm_set == MNGRED_TRUE)
      {
         LE_send_msg (GL_STATUS, 
            "%s Failure reading NB/WB comms relay (relay state is \"Unknown\")",
            MNGRED_WARN_CLEAR);
         read_alarm_set = MNGRED_FALSE;
      }
   }

   return (current_state);
}


/********************************************************************************

    Description: This function is used for Unit/Integration Testing only.

          Input:

         Output:

         Return: "ASSIGNED" if the RDA is controlling or "UNASSINGED" if the
                 RDA is non-controlling

        Globals: CHAnnel_status - see mngred_globals.h & orpgred.h

          Notes:

 ********************************************************************************/

static Comms_relay_state_t Uit_comms_relay (int control_function)
{
   switch (control_function)
   {
      case ACQUIRE_RELAY:
            /* return "ASSIGNED" if the rda is controlling */
         if (CHAnnel_status.rda_control_state == ORPGRED_RDA_CONTROLLING)  
            return (ORPGRED_COMMS_RELAY_ASSIGNED);
         else    
            return (ORPGRED_COMMS_RELAY_UNASSIGNED);
      break;

      case READ_RELAY:
            /* return "ASSIGNED" if the rda is controlling */
         if (CHAnnel_status.rda_control_state == ORPGRED_RDA_CONTROLLING)
            return (ORPGRED_COMMS_RELAY_ASSIGNED);
         else
            return (ORPGRED_COMMS_RELAY_UNASSIGNED);
      break;
   }
   return (ORPGRED_COMMS_RELAY_UNKNOWN);
}
