/*
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2005/04/15 14:39:54 $
 * $Id: cmt_snmp_dummy.c,v 1.3 2005/04/15 14:39:54 jing Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 *
 * 12MAR2002 Chris Gilbert - NA01-34801 Issue 1-886 - Add support for TCP Dial-out.
 *
 * 20MAR2002 Chris Gilbert - NA01-34801 Issue 1-886 - Fix some minor problems
 *                           found in unit testing. 
 *
 */
#include <stdlib.h>
#include <comm_manager.h>
#include <cmt_def.h>

/******************************************************************

        file: cmt_snmp.c

        This module has snmp stub functions for the comm 
	manager version that does not support dial out

******************************************************************/


/**************************************************************************

    Description: SNMP_set -This function sends a SNMP PDU to host_name in 
                 order to change a value.

    Inputs:     host_name - IP address or name of host
                community_name - snmp community name on host
                object_id - object that is going to be changed
                datatype - type of object
                value - new value of object

    Output:     none 

    Return:     It returns a -1 on failure or a 1 for success.

**************************************************************************/


int SNMP_set (char *host_name, char *community_name, char *object_id, 
              char *datatype, char *value) {

   return (-1);

}


/**************************************************************************

    Description: SNMP_get -This function gets settings from a snmp host. 

    Inputs:     host_name - IP address or name of host
                community_name - snmp community name on host
                object_id - object that is going to be changed

    Output:     none 

    Return:     It returns a pointer to variable_list struct that contains the
                requested data.

**************************************************************************/
struct variable_list *SNMP_get (char *host_name, char *community_name, char *object_id ) 
{
   return (NULL);
}


/**************************************************************************

    Description: SNMP_find_index -This function gets the snmp 
                                  index for an interface. 

    Inputs:     interface - Interface name
                snmp_host - IP address or name of host
                community - snmp community name on host
                curval    - current value of the index (if any)

    Output:     none 

    Return:     It returns the SNMP index to the interface

**************************************************************************/
int SNMP_find_index (char *interface, char *snmp_host, char *community, int curval) 
{
   return (-1);

}



#include "cmt_def.h"

/**************************************************************************

    Description: SNMP_disable -This function sends a 'disable' command to
                               a hardware device or hardware interface. 

    Inputs:     link - the link structure list.

    Output:     none 

    Return:     -1 on Failure or 1 for success

**************************************************************************/
int SNMP_disable (Link_struct *link) 
{
   return (-1);
}


/**************************************************************************

    Description: SNMP_enable -This function sends an 'enable' command to
                               a hardware device or hardware interface. 

    Inputs:     link - the link structure list.

    Output:     none 

    Return:     -1 on Failure or 1 for success

**************************************************************************/
int SNMP_enable (Link_struct *link) 
{
     return(-1);
}




