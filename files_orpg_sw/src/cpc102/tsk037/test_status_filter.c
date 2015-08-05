/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2005/12/27 19:49:52 $
 * $Id: test_status_filter.c,v 1.3 2005/12/27 19:49:52 steves Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */  

#include <orpg.h>
/*\******************************************************************
******************************************************************\*/
int main( int argc, char *argv[] ){

    /* Initialize Log-Error services. */
    if( ORPGMISC_init( argc, argv, 100, 0, -1, 0 ) < 0 ){

       LE_send_msg( GL_INFO, "ORPGMISC_init Failed\n" );
       ORPGTASK_exit(GL_EXIT_FAILURE);

    }

    /* Write the various types of messages. */
    LE_send_msg( GL_STATUS | LE_RPG_INFO_MSG, 
                 "This is an RPG Informational Message\n" );

    sleep(4);

    LE_send_msg( GL_STATUS | LE_RPG_WARN_STATUS, 
                 "This is an RPG Warning Status Message\n" );

    sleep(4);

    LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS, 
                 "This is an RPG General Status Message\n" );

    sleep(4);

    LE_send_msg( GL_STATUS | LE_RPG_COMMS, 
                 "This is an RPG Narrowband Communications Message\n" );

    sleep(4);

    LE_send_msg( GL_STATUS | LE_RPG_AL_LS, 
                 "This is an RPG Alarm (Load Shed) Message\n" );

    sleep(4);

    LE_send_msg( GL_STATUS | LE_RPG_AL_LS | LE_RPG_AL_CLEARED, 
                 "This is an RPG Alarm Cleared (Load Shed) Message\n" );

    sleep(4);

    LE_send_msg( GL_STATUS | LE_RPG_AL_MAR, 
                 "This is an RPG Alarm (Maintenance Required) Message\n" );

    sleep(4);

    LE_send_msg( GL_STATUS | LE_RPG_AL_MAR | LE_RPG_AL_CLEARED, 
                 "This is an RPG Alarm Cleared (Maintenance Required) Message\n" );

    sleep(4);

    LE_send_msg( GL_STATUS | LE_RPG_AL_MAM, 
                 "This is an RPG Alarm (Maintenance Mandatory) Message\n" );

    sleep(4);

    LE_send_msg( GL_STATUS | LE_RPG_AL_MAM | LE_RPG_AL_CLEARED, 
                 "This is an RPG Alarm Cleared (Maintenance Mandatory) Message\n" );

    sleep(4);

    LE_send_msg( GL_STATUS | LE_RDA_AL_SEC, 
                 "This is an RDA Alarm (Secondary) Message\n" );

    sleep(4);

    LE_send_msg( GL_STATUS | LE_RDA_AL_SEC | LE_RDA_AL_CLEARED, 
                 "This is an RDA Alarm Cleared (Secondary) Message\n" );

    sleep(4);

    LE_send_msg( GL_STATUS | LE_RDA_AL_MAR, 
                 "This is an RDA_Alarm (Maintenance Required) Message\n" );

    sleep(4);

    LE_send_msg( GL_STATUS | LE_RDA_AL_MAR | LE_RDA_AL_CLEARED, 
                 "This is an RDA Alarm Cleared (Maintenance Required) Message\n" );

    sleep(4);

    LE_send_msg( GL_STATUS | LE_RDA_AL_MAM, 
                 "This is an RDA Alarm (Maintenance Mandatory) Message\n" );

    sleep(4);

    LE_send_msg( GL_STATUS | LE_RDA_AL_MAM | LE_RDA_AL_CLEARED, 
                 "This is an RDA Alarm Cleared (Maintenance Mandatory) Message\n" );

    sleep(4);

    LE_send_msg( GL_STATUS | LE_RDA_AL_INOP, 
                 "This is an RDA Alarm (Inoperable) Message\n" );

    sleep(4);

    LE_send_msg( GL_STATUS | LE_RDA_AL_INOP | LE_RDA_AL_CLEARED, 
                 "This is an RDA Alarm Cleared (Inoperable) Message\n" );

   return 0;

}
