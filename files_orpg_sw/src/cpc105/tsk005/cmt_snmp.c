
/******************************************************************

        file: cmt_snmp.c

        This module has snmp related functions.

******************************************************************/

/*
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2006/03/02 21:13:37 $
 * $Id: cmt_snmp.c,v 1.17 2006/03/02 21:13:37 jing Exp $
 * $Revision: 1.17 $
 * $State: Exp $ 
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <infr.h>

#include "cmt_def.h"

/* If we want to set time out, we can add "-t seconds" in the following. */
#define SNMP_CMD_OPTIONS "-v1 -Os"
#define CMD_BUF_SIZE 256
#define TMP_BUF_SIZE 128

extern int MAXIF;

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
    int ret;
    char cmd[CMD_BUF_SIZE], obuf[TMP_BUF_SIZE];

    sprintf (cmd, "snmpset %s -m ALL -c %s %s %s %s %s", SNMP_CMD_OPTIONS, 
		community_name, host_name, object_id, datatype, value);

    obuf[0] = '\0';
    ret = MISC_system_to_buffer (cmd, obuf, TMP_BUF_SIZE, NULL);
    if (ret < 0) {
	LE_send_msg (GL_ERROR, 
		"MISC_system_to_buffer (%s) failed (%d)", cmd, ret);
	LE_send_msg (GL_INFO, "    Cmd out: %s\n", obuf);
	return (-1);
    }
    LE_send_msg (LE_VL2, "SNMP_set cmd (ret %d): %s", ret, cmd);
    LE_send_msg (LE_VL2, "    Cmd out: %s\n", obuf);
    return (1);
}

/**************************************************************************

    Description: SNMP_get -This function gets settings from a snmp host. 

    Inputs:     host_name - IP address or name of host
                community_name - snmp community name on host
                object_id - object that is going to be changed

    Output:     none 

    Return:     It returns a pointer to the value on success or NULL on
		failure. 

**************************************************************************/

char *SNMP_get (char *host_name, char *community_name, char *object_id ) {
    char *value;
    int ret, quoted;
    char cmd[CMD_BUF_SIZE], obuf[TMP_BUF_SIZE], *p;

    sprintf (cmd, "snmpget %s -m ALL -c %s %s %s", SNMP_CMD_OPTIONS, 
		community_name, host_name, object_id);

    obuf[0] = '\0';
    ret = MISC_system_to_buffer (cmd, obuf, TMP_BUF_SIZE, NULL);
    if (ret < 0) {
	LE_send_msg (GL_ERROR, 
		"MISC_system_to_buffer (%s) failed (%d)", cmd, ret);
	LE_send_msg (GL_INFO, "    Output: %s", obuf);
	return (NULL);
    }

    p = obuf;
    while (*p != '\0' && *p != '=')
	p++;
    if (*p != '=') {
	LE_send_msg (GL_INFO, "No value (=) found with cmd: %s", cmd);
	LE_send_msg (GL_INFO, "    Cmd out: %s\n", obuf);
	return (NULL);
    }
    p++;

    while (*p != '\0' && *p != ':')
	p++;
    if (*p != ':') {
	LE_send_msg (GL_INFO, "No value (:) found with cmd: %s", cmd);
	LE_send_msg (GL_INFO, "    Cmd out: %s\n", obuf);
	return (NULL);
    }
    p++;

    while (*p == ' ' || *p == '\t')
	p++;
    quoted = 0;
    if (*p == '\"') {
	p++;
	quoted = 1;
    }
    value = malloc (strlen (p) + 1);
    if (value == NULL) {
	LE_send_msg (GL_ERROR, "malloc failed");
	return (NULL);
    }
    strcpy (value, p);
    p = value + strlen (value) - 1;
    while (p >= value && (*p == ' ' || *p == '\n')) {
	*p = '\0';
	p--;
    }
    if (quoted) {
	if (*p != '\"')
	    LE_send_msg (LE_VL2, 
		"Missing ending \", \"%s\" got from cmd: %s", value, cmd);
	else
	    *p = '\0';
    }
    LE_send_msg (LE_VL2, "Value \"%s\" got from cmd: %s", value, cmd);
    return (value);
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


int SNMP_find_index (char *interface, char *snmp_host, char *community, int curval) {

   int ifnum, num;
   char *vars;
   int i = 0;
   char buf[50];
   int found =0;

   if ( curval > 0 && curval <= MAXIF) {
      /* Does the current index value match? */
      /*sprintf (buf, "ifDescr.%d", curval);*/
      sprintf (buf, ".1.3.6.1.2.1.2.2.1.2.%d", curval);
      vars = SNMP_get (snmp_host, community, buf);
      if (vars != NULL) {
         if ( strcmp (vars, interface) == 0) {
            /* the current value still matches */
            free (vars);
            return (curval);
         }
         free (vars);
      }
   }
   
   /* Current Value didn't match. So, lets find it */

   /* Ask about the number of interfaces */

   /*vars = SNMP_get (snmp_host, community, "ifNumber.0");*/
   vars = SNMP_get (snmp_host, community, ".1.3.6.1.2.1.2.1.0");

   if (vars == NULL || sscanf (vars, "%d", &num) != 1) {
      LE_send_msg (GL_ERROR, "Error in snmp get\n");
      free (vars);
      return (-1);
   }

   ifnum = num;
   free (vars);

   LE_send_msg (LE_VL3, "Num of snmp Interfaces %d \n", ifnum );


   /* Go through the interface descriptions looking for the index */
   /* The SNMP index of a device is dynamic. Therefore, we should */
   /* always search for the index and never assume it is static.  */

   /* List the name of the interfaces */
   for (i = 1; i< MAXIF; i++) {

      /*sprintf (buf, "ifDescr.%d", i);*/
      sprintf (buf, ".1.3.6.1.2.1.2.2.1.2.%d", i);

      vars = SNMP_get (snmp_host, community, buf);
      if (vars == NULL) {
         /*LE_send_msg (GL_ERROR, "Error in snmp get index %d\n",i);*/
         continue;
      }
   
      found++; 

      if ( strcmp (vars, interface) == 0) {
         /* found the interface */
         LE_send_msg (LE_VL3, "Snmp: ifDescr.%d = %s \n", i, vars);
         free (vars);
         return (i);
      }
      free (vars);
      if (found >= ifnum) {
         break; /* Went through all the interfaces */
      }
   }

   return (-1);
}




/**************************************************************************

    Description: SNMP_disable -This function sends a 'disable' command to
                               a hardware device or hardware interface. 

    Inputs:     link - the link structure list.

    Output:     none 

    Return:     -1 on Failure or 1 for success

**************************************************************************/
int SNMP_disable (Link_struct *link) {

   char command[100];
   int  retval;

   if (link->network != CMT_PPP ) {
      return (-1);
   }

   /* Make sure the values are defined */
   if ( link->snmp_str.snmp_interface == NULL ) {
      LE_send_msg (GL_ERROR, "snmp_interface not defined\n");
      return (-1);
   }
   
   if ( link->snmp_str.snmp_host == NULL ) {
      LE_send_msg (GL_ERROR, "snmp_host not defined\n");
      return (-1);
   }
   
   if ( link->snmp_str.snmp_community == NULL ) {
      LE_send_msg (GL_ERROR, "snmp_community not defined\n");
      return (-1);
   }

   if ( link->snmp_str.disable.oid_cmd == NULL ) {
      LE_send_msg (GL_ERROR, "snmp disable oid cmd not defined\n");
      return (-1);
   }

   if ( link->snmp_str.disable.oid_type == NULL ) {
      LE_send_msg (GL_ERROR, "snmp disable oid type not defined\n");
      return (-1);
   }

   if ( link->snmp_str.disable.oid_value == NULL ) {
      LE_send_msg (GL_ERROR, "snmp disable oid value not defined\n");
      return (-1);
   }
   
   if ( link->snmp_str.drop_dtr.oid_cmd == NULL ) {
      LE_send_msg (GL_ERROR, "snmp drop_dtr oid cmd not defined\n");
      return (-1);
   }

   if ( link->snmp_str.drop_dtr.oid_type == NULL ) {
      LE_send_msg (GL_ERROR, "snmp drop_dtr oid type not defined\n");
      return (-1);
   }

   if ( link->snmp_str.drop_dtr.oid_value == NULL ) {
      LE_send_msg (GL_ERROR, "snmp drop_dtr oid value not defined\n");
      return (-1);
   }

   if (link->dynamic_dial) {
      if ( link->snmp_str.modem_bsy_set.oid_cmd == NULL ) {
         LE_send_msg (GL_ERROR, "snmp modem_bsy_set oid cmd not defined\n");
         return (-1);
      }
      if ( link->snmp_str.modem_bsy_set.oid_type == NULL ) {
         LE_send_msg (GL_ERROR, "snmp modem_bsy_set oid type not defined\n");
         return (-1);
      }
      if ( link->snmp_str.modem_bsy_set.oid_value == NULL ) {
         LE_send_msg (GL_ERROR, "snmp modem_bsy_set oid value not defined\n");
         return (-1);
      }
   }
   
   
   link->snmp_str.snmp_index = SNMP_find_index(link->snmp_str.snmp_interface,
                           link->snmp_str.snmp_host,
                           link->snmp_str.snmp_community, link->snmp_str.snmp_index);

   if (link->snmp_str.snmp_index < 1) {
      LE_send_msg (GL_ERROR, "snmp could not find index for %s\n",
                             link->snmp_str.snmp_interface);
      return (-1);
   }

   if (link->dynamic_dial) {
      /* First send drop DTR command to hang up modem */
      sprintf (command, link->snmp_str.drop_dtr.oid_cmd, link->snmp_str.snmp_index);

      LE_send_msg (LE_VL3, "snmp drop dtr command %s \n", command);

      /* send the snmp drop dtr command */
      retval = SNMP_set (link->snmp_str.snmp_host,
                         link->snmp_str.snmp_community, command,
                         link->snmp_str.drop_dtr.oid_type,
                         link->snmp_str.drop_dtr.oid_value);

      if (retval != 1) {
         LE_send_msg (GL_ERROR, "snmp could not set host:%s community:%s cmnd:%s\n",
                                link->snmp_str.snmp_host,
                                link->snmp_str.snmp_community, command);
         return (-1);
      }

      /* Now, busy out the modem */
      LE_send_msg (LE_VL3, "snmp modem_bsy_set command %s \n",
                         link->snmp_str.modem_bsy_set.oid_cmd);

      /* send the snmp drop dtr command */
      retval = SNMP_set (link->snmp_str.snmp_host,
                         link->snmp_str.snmp_community,
                         link->snmp_str.modem_bsy_set.oid_cmd,
                         link->snmp_str.modem_bsy_set.oid_type,
                         link->snmp_str.modem_bsy_set.oid_value);

      if (retval != 1) {
         LE_send_msg (GL_ERROR, "snmp could not set host:%s community:%s cmnd:%s\n",
                                link->snmp_str.snmp_host,
                                link->snmp_str.snmp_community,
                                link->snmp_str.modem_bsy_set.oid_cmd);
         return (-1);
      }
       
   } /* end dynamic_dial */

   /* Send the disable command */
   sprintf (command, link->snmp_str.disable.oid_cmd, link->snmp_str.snmp_index);

   LE_send_msg (LE_VL3, "snmp disable command %s \n", command);

   /* send the snmp disable command */
   retval = SNMP_set (link->snmp_str.snmp_host,
                      link->snmp_str.snmp_community, command,
                      link->snmp_str.disable.oid_type,
                      link->snmp_str.disable.oid_value);

   if (retval != 1) {
      LE_send_msg (GL_ERROR, "snmp could not set host:%s community:%s command:%s\n",
                             link->snmp_str.snmp_host,
                             link->snmp_str.snmp_community, command);
      return (-1);
   }


   /* Now send drop DTR command */
   sprintf (command, link->snmp_str.drop_dtr.oid_cmd, link->snmp_str.snmp_index);

   LE_send_msg (LE_VL3, "snmp drop dtr command %s \n", command);

   /* send the snmp drop dtr command */
   retval = SNMP_set (link->snmp_str.snmp_host,
                      link->snmp_str.snmp_community, command,
                      link->snmp_str.drop_dtr.oid_type,
                      link->snmp_str.drop_dtr.oid_value);

   if (retval != 1) {
      LE_send_msg (GL_ERROR, "snmp could not set host:%s community:%s command:%s\n",
                             link->snmp_str.snmp_host,
                             link->snmp_str.snmp_community, command);
      return (-1);
   }

   return (retval);

}


/**************************************************************************

    Description: SNMP_enable -This function sends an 'enable' command to
                               a hardware device or hardware interface. 

    Inputs:     link - the link structure list.

    Output:     none 

    Return:     -1 on Failure or 1 for success

**************************************************************************/
int SNMP_enable (Link_struct *link) {

   char command[100];
   int retval;

   if (link->network != CMT_PPP ) {
      return (-1);
   }

   /* Make sure the values are defined */
   if ( link->snmp_str.snmp_interface == NULL ) {
      LE_send_msg (GL_ERROR, "snmp_interface not defined\n");
      return (-1);
   }
   
   if ( link->snmp_str.snmp_host == NULL ) {
      LE_send_msg (GL_ERROR, "snmp_host not defined\n");
      return (-1);
   }
   
   if ( link->snmp_str.snmp_community == NULL ) {
      LE_send_msg (GL_ERROR, "snmp_community not defined\n");
      return (-1);
   }

   if ( link->snmp_str.enable.oid_cmd == NULL ) {
      LE_send_msg (GL_ERROR, "snmp disable oid cmd not defined\n");
      return (-1);
   }

   if ( link->snmp_str.enable.oid_type == NULL ) {
      LE_send_msg (GL_ERROR, "snmp disable oid type not defined\n");
      return (-1);
   }

   if ( link->snmp_str.enable.oid_value == NULL ) {
      LE_send_msg (GL_ERROR, "snmp disable oid value not defined\n");
      return (-1);
   }

   if (link->dynamic_dial) {
      if ( link->snmp_str.modem_bsy_clr.oid_cmd == NULL ) {
         LE_send_msg (GL_ERROR, "snmp modem_bsy_clr oid cmd not defined\n");
         return (-1);
      }
      if ( link->snmp_str.modem_bsy_clr.oid_type == NULL ) {
         LE_send_msg (GL_ERROR, "snmp modem_bsy_clr oid type not defined\n");
         return (-1);
      }
      if ( link->snmp_str.modem_bsy_clr.oid_value == NULL ) {
         LE_send_msg (GL_ERROR, "snmp modem_bsy_clr oid value not defined\n");
         return (-1);
      }
   }
 
   
   link->snmp_str.snmp_index = SNMP_find_index(link->snmp_str.snmp_interface,
                                  link->snmp_str.snmp_host,
                                  link->snmp_str.snmp_community, link->snmp_str.snmp_index);

   if (link->snmp_str.snmp_index < 1) {
      LE_send_msg (GL_ERROR, "snmp could not find index for %s\n",
                             link->snmp_str.snmp_interface);
      return (-1);
   }

   /* Send the enable command */
   sprintf (command, link->snmp_str.enable.oid_cmd, link->snmp_str.snmp_index);

   LE_send_msg (LE_VL3, "snmp enable command %s \n", command);

   /* send the snmp enable command */
   retval = SNMP_set (link->snmp_str.snmp_host,
                      link->snmp_str.snmp_community, command,
                      link->snmp_str.enable.oid_type,
                      link->snmp_str.enable.oid_value);

   if (retval != 1) {
      LE_send_msg (GL_ERROR, "snmp could not set host:%s community:%s command:%s\n",
                             link->snmp_str.snmp_host,
                             link->snmp_str.snmp_community, command);
      return (-1);
   }

   if (link->dynamic_dial) {

      /* Now, clear busy out on the modem */
      LE_send_msg (LE_VL3, "snmp modem_bsy_clr command %s \n",
                         link->snmp_str.modem_bsy_clr.oid_cmd);

      /* send the snmp modem_bsy_clr  command */
      retval = SNMP_set (link->snmp_str.snmp_host,
                         link->snmp_str.snmp_community,
                         link->snmp_str.modem_bsy_clr.oid_cmd,
                         link->snmp_str.modem_bsy_clr.oid_type,
                         link->snmp_str.modem_bsy_clr.oid_value);

      if (retval != 1) {
         LE_send_msg (GL_ERROR, "snmp could not set host:%s community:%s cmnd:%s\n",
                                link->snmp_str.snmp_host,
                                link->snmp_str.snmp_community,
                                link->snmp_str.modem_bsy_clr.oid_cmd);
         return (-1);
      }
       
   } /* end dynamic_dial */

   return (retval);

}


