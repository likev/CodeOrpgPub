/********************************************************************************

    file: rdasim_config.c

    Description:  This module reads the link config file during 
                  initialization for the RDA simulator.
 ********************************************************************************/

/* 
 * RCS info
 * $Author: garyg $
 * $Locker:  $
 * $Date: 1999/04/15 15:54:05 $
 * $Id: rdasim_config.c,v 1.1 1999/04/15 15:54:05 garyg Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */  


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <infr.h>
#include <comm_manager.h>
#include <rdasim_simulator.h>

static char *Conf_name;        /* name of the link config file */


    /* local functions */

static int Send_err_and_ret (char *text);
static void Print_cs_error (char *msg);


/********************************************************************************

    Description: This function reads the link configuration file.

    Inputs:    cm_index - comm manager index;
               link_conf_name - link config file name;
               link - the link structure;

    Output:    device_number - device number;

    Return:    It returns the number of links on success or -1 on failure.

 ********************************************************************************/

#define BUF_SIZE 256

int CO_read_link_config (int cm_index, char *link_conf_name, 
            Link_t *link, int *device_number)
{
   int tlinks;    /* total number of links configured */
   int n_links;   /* number of links managed by this process */
   int i;
   char buf [BUF_SIZE];
   char *key;
   int device;

   Conf_name = link_conf_name;
   CS_cfg_name (Conf_name);
   CS_error (Print_cs_error);

   key = "number_links";
   if (CS_entry (key, 1, BUF_SIZE, buf) < 0)
       return (-1);

   if (sscanf (buf, "%d", &tlinks) != 1 || tlinks <= 0)
       return (Send_err_and_ret (buf));

   n_links = 0;
   device = -1;

    /* Scan through the configuration file until the entry for the wideband
       link is found, then load the link configuration table. If more than one entry
       is found(e.g. if more than one link has been configured for this comm
       manager), return an error because the simulator can only be configured
       for one wideband link. */

   for (i = 0; i < tlinks; i++) 
   {
      int type,   /* line type(dial-in, dedicated, etc.) */ 
          dn,     /* physical device number */
          pn,     /* physical port number */
          cm,     /* comm manager number */
          rate,   /* line/baud rate */
          mps,    /* max packet size */
          npvc,   /* number of permanent virtual circuits */
          den;    /* incoming event notification */

          /* match the comm manager number and the subsystem */

      if (CS_entry ((char *)i, CS_INT_KEY | 2, BUF_SIZE, buf) < 0 ||
          sscanf (buf, "%d", &cm) != 1 ||
          CS_entry ((char *)i, CS_INT_KEY | 7, BUF_SIZE, buf) < 0) 
               return (Send_err_and_ret (buf));

      if (cm != cm_index || strcmp (buf, "SIMPAC") != 0)
          continue;

          /* check for properly assigned line type */

      if (CS_entry ((char *)i, CS_INT_KEY | 5, BUF_SIZE, buf) < 0)
          return (Send_err_and_ret (buf));

      if (strcmp (buf, "Dedic") == 0)
          type = DEDICATED;
      else 
      {
         fprintf (stderr, "config file error, unexpected link type\n");
         return (Send_err_and_ret (buf));
      }

         /* get remaining config settings */

      if (CS_entry ((char *)i, CS_INT_KEY | CS_FULL_LINE, 
          BUF_SIZE, buf) < 0 || sscanf (buf, 
           "%*s %*d %*d %d %d %*s %d %*s %d %d %*d %d",
           &dn, &pn, &rate, &mps, &npvc, &den) != 6) 
                 return (Send_err_and_ret (buf));

           /* perform general checks to ensure proper config settings */

      if (mps < 32 || npvc < 0 || npvc > MAX_N_STATIONS) 
      {
         fprintf (stderr, "unexpected number specified\n");
         return (Send_err_and_ret (buf));
      }        

           /* ensure only one wideband link was detected in the configuration
              file */

      if (device < 0)
          device = dn;
      else if (device != dn) 
      {
         fprintf (stderr, "rda simulator comm mgr only processes a single device\n");
         return (Send_err_and_ret (buf));
      }

         /* check for more than one link configured */

      if (n_links > 0) 
      {
         fprintf (stderr, "too many (%d) links specified for this comm_manager\n", 
                      n_links + 1);
         return (-1);
      }

         /* update link configuration table */

      link->link_ind = i;
      link->device = dn;
      link->port = pn;
      link->link_type = type;
      link->line_rate = rate;
      link->packet_size = mps;
      link->data_en = den;
      n_links++;

   } /* end for tlinks loop */

   *device_number = device;

        /* close this config file and return to the generic system config text */

   CS_cfg_name (""); 

   return (n_links);
}


/**************************************************************************

    Description: This function sends an error message.

    Input:    text - text in which the error is found.

    Return:    It returns -1.

**************************************************************************/

static int Send_err_and_ret (char *text)
{

   fprintf (stderr, "error found in \"%s\" (file %s)\n",
                text, Conf_name);
   return (-1);
}


/**************************************************************************

    Description: This is the CS error call back function.

    Input:    msg - the error msg.

**************************************************************************/

static void Print_cs_error (char *msg)
{

   fprintf (stderr, "%s", msg);
   return;
}
