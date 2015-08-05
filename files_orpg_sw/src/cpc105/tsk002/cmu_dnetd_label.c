/*   @(#) label.c 00/01/05 Version 1.5   */

/*
 *  STREAMS Network Daemon
 *
 *  Copyright (c) 1988-1997 Spider Software Limited
 *
 *  This Source Code is furnished under Licence, and may not be
 *  copied or distributed without express written agreement.
 *
 *  Written by Mark Valentine.
 *
 *  Made in Scotland.
 *
 *  @(#)$Id: cmu_dnetd_label.c,v 1.2 2000/10/20 15:24:49 jing Exp $
 */

/*
Modification history.
Chg Date       Init Description
 1. 15-Jul-98 	rjp Fixed typo.
 2. 17-AUG-98 	mpb 'lower' is a char*, so check to see if it is NULL, not
                    if the first character of it is NULL.
 3. 08-oct-99   tdg Added VxWorks support.
 4. 28-oct-99   tdg Added netd_delete_all_labels() for VxWorks cleanup
                    and had netd_delete_label reset all fields
*/	


/*
* #define DEBUG
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <debug.h>
#include <xstopen.h>
#include "cmu_dnetd_mps.h"
#include "cmu_dnetd_label.h"

static int netd_get_label_index(char *);
static struct label *netd_get_label(char *);
static void netd_dec_lref(struct label *);

/*
 * Label table.
 */

#define MAXLABELS 1000

static struct label labels[MAXLABELS];

char *
netd_generate_label(void)
{
    static int n = 0;
    char *label;

    if ((label = malloc(8)) == NULL) {
        TRACE(("can't allocate 8 bytes for generated label!\n"));
        return NULL;
    }

    sprintf(label, "#%06d", ++n);

    TRACE(("Generated temp. label: %s\n", label));

    return label;
}

static int
netd_get_label_index(char *label)
{
    int i;

    for (i = 0; i < MAXLABELS; i++)
    {
        if (labels[i].name == NULL)
            continue;

        if (strcmp(labels[i].name, label) == 0)
            return i;
    }
    return -1;
}


static struct label *
netd_get_label(char *label)
{
    int i;

    for (i = 0; i < MAXLABELS; i++)
    {
        if (labels[i].name == NULL)
            continue;

        if (strcmp(labels[i].name, label) == 0)
            return(&labels[i]);
    }
    return((struct label *)0);
}

/*************************************************************************
* netd_store_label
*
*************************************************************************/
int
netd_store_label(char *pLabel, int value, UCONX_DEVICE *pDevice,
	char *lower, int type, char *ass_dev)
{
    int i, n = -1;

#ifdef DEBUG
    if (pDevice != 0)
 	printf ("netd_store_label: %s, %x, %s, %s, %s, %d %s\n", pLabel, value,
  		pDevice->serverName, pDevice->protoName, lower, type, ass_dev);
    else
         printf ("netd_store_label: %s %x %s %d %s\n",  pLabel, value, lower,
         	type, ass_dev);
#endif

    if ((i = netd_get_label_index(pLabel)) == -1)
    {
        for (i = 0; i < MAXLABELS; i++)
        {
            if (labels[i].name == NULL) {
                if (n == -1)
                    n = i;
                continue;
            }

            if (strcmp(labels[i].name, pLabel) == 0) {
                TRACE(("label \"%s\" redefined\n", pLabel));
                return -1;
            }
        }
        if (n == -1) {
            TRACE(("%s: no free label slots\n", pLabel));
            return -1;
        }

#ifdef VXWORKS
	if ((labels[n].name = malloc(strlen(pLabel)+1)) == NULL) {
#else
        if ((labels[n].name = strdup(pLabel)) == NULL) {
#endif 
            TRACE(("%s: insufficient memory to store label\n", pLabel));
            return -1;
        }
#ifdef VXWORKS
	else
	    strcpy(labels[n].name, pLabel);
#endif	
    }
    else
    {
        n = i;

        if (labels[n].pDevice)
        {
            free(labels[n].pDevice);
            labels[n].pDevice = NULL;
        }

        if (labels[n].lower)
            labels[n].lower = NULL;
    }

    if (pDevice != NULL)
    {
        labels[n].pDevice = malloc (sizeof (*(labels [n].pDevice)));
        if (labels[n].pDevice == NULL)
	{				/* #1				*/
            TRACE(("%s: insufficient memory to store label\n", pDevice)); 
            return -1;
        }
	*labels [n].pDevice = *pDevice;	/* Duplicate pDevice		*/
    }

    /* #2 */
    if ( *lower != ( char ) NULL )
    {
        if ((labels[n].lower = netd_get_label(lower)) == NULL) {
            TRACE(("%s: insufficient memory to store label\n",
                pDevice));		/* #1				*/
            return -1;
        }
        labels[n].lower->refs++;
    }

    labels[n].value = value;
    labels[n].type = type;

    if (type == NETD_MUXID_TYPE)
    {
        if (ass_dev == NULL)
        {
            TRACE(("%s: no associated device with muxid\n", pDevice));
            return -1;
        }
        for (i = 0; i < MAXLABELS; i++)
        {
            if (labels[i].name == NULL)
                continue;

            if (strcmp(labels[i].name, ass_dev) == 0)
            {
                labels[n].ass_label = &labels[i];
                break;
            }
        }
        if (i == MAXLABELS)
        {
            TRACE(("%s: invalid associated device\n", pDevice));
            return -1;
        }
    }

    TRACE(("[stored \"%s\" = %d]\n", pDevice, value));

    return 0;
}

int
netd_lookup_label(char *label, int type)
{
    int i;

    for (i = 0; i < MAXLABELS; i++)
    {
        if (labels[i].name == NULL)
            continue;

        if (strcmp(labels[i].name, label) == 0)
        {
            TRACE(("[found \"%s\" = %d]\n", label, labels[i].value));

            if (labels[i].type != type)
            {
                /*
                * Let's see if we can play our get out of jail
                * card now !
                *
                * If its a muxid, the associated label which has
                * the fd of the stream down which the I_LINK
                * command was sent, is saved in the ass_label
                * field.
                */
                if (labels[i].type == NETD_MUXID_TYPE)
                {
    		    TRACE(("lookup on mux dev label '%s'\n",
			    labels[i].ass_label->name));
                    return labels[i].ass_label->value;
                }
                TRACE(("%s: label of wrong type\n", label));
                return -1;
            }

            return labels[i].value;
        }
    }

    TRACE(("%s: label not found\n", label));

    return -1;
}

int
netd_rename_label(char *old, char *new)
{
    int i;

    for (i = 0; i < MAXLABELS; i++)
    {
        if (labels[i].name == NULL)
            continue;

        if (strcmp(labels[i].name, old) == 0)
        {
            TRACE(("[found \"%s\" = %d]\n", new, labels[i].value));
            netd_store_label(new, labels[i].value, NULL,
                     (labels[i].lower)?
                        labels[i].lower->name: 0,
                     labels[i].type, NULL);
            netd_delete_label(old);
            return 0;
        }
    }

    TRACE(("%s: couldn't find label to rename it\n", old));

    return -1;
}

int
netd_copy_label(char *existing, char *label)
{
    int i;

    for (i = 0; i < MAXLABELS; i++)
    {
        if (labels[i].name == NULL)
            continue;

        if (strcmp(labels[i].name, existing) == 0)
        {
            TRACE(("[found \"%s\" = %d]\n", existing, labels[i].value));
            return netd_store_label(label, labels[i].value, NULL,
                        (labels[i].lower)?
                            labels[i].lower->name: 0,
                        labels[i].type, NULL);
        }
    }

    TRACE(("%s: couldn't find label to copy it\n", existing));

    return -1;
}

int
netd_delete_label(char *label)
{
    int i;

    for (i = 0; i < MAXLABELS; i++)
    {
        if (labels[i].name && strcmp(labels[i].name, label) == 0)
        {
            TRACE(("[free \"%s\" = %d]\n", label, labels[i].value));
            free(labels[i].name);
            labels[i].name = NULL;

            if (labels[i].pDevice)
            {
                free(labels[i].pDevice);
                labels[i].pDevice = NULL;
            }

            if (labels[i].lower)
                labels[i].lower = NULL;
#ifdef VXWORKS
	    /* we need to do a little more cleanup as other fields
	       will remain set in the VxWorks global world */

	    if (labels[i].ass_label)
		labels[i].ass_label = NULL;
	
	    if (labels[i].type)
		labels[i].type = 0;

	    if (labels[i].value)
		labels[i].value = 0;

	    if (labels[i].control)
		labels[i].control = 0;

	    if (labels[i].refs)
		labels[i].refs = 0;
#endif /* VXWORKS */

            return 0;
        }
    }

    TRACE(("%s: couldn't find label to delete it\n", label));

    return -1;
}

void
netd_delete_all_labels(void)
{
    int i;
   
    for (i = 0; i < MAXLABELS; i++)
    {
        if (labels[i].name != NULL)
	    netd_delete_label(labels[i].name);
    }
}

void
netd_print_labels(void)
{
#ifdef DEBUG
    int i;

    TRACE(("==================================================\n"));
    TRACE((" %-32s  %6s  (%s)\n", "Label", "Value", "Type"));
    TRACE(("--------------------------------------------------\n"));

    for (i = 0; i < MAXLABELS; i++) {
        if (labels[i].name != NULL)
            TRACE((" %-32s  %6d   (%s)\n",
                labels[i].name,
                labels[i].value,
                labels[i].type == NETD_FDESC_TYPE? "F" : "M"));
    }

    TRACE(("==================================================\n"));
#endif /* DEBUG */
}

/*
* If an I_LINK command is passed to a file descriptor which is being tracked
* with a label, then mark that label as being a "control STREAM". This means
* that any subsequent I_LINK commands which are passed and use this label as
* the lower file descriptor will not actually cause this STREAM to be used.
* We *must* keep the control STREAM open if we are to be able to support
* I_UNLINK commands as they must come down the same STREAM as the I_LINK.
*/
void
netd_mark_control_label(char *label)
{
    int i;

    for (i = 0; i < MAXLABELS; i++)
    {
        if (labels[i].name == NULL)
            continue;

        if (strcmp(labels[i].name, label) == 0)
	{
            labels[i].control = 1;
	}
    }
}

/*
* Check to see if a label is associated with a control STREAM.
*/
UCONX_DEVICE *
netd_control_label(char *label)
{
    int i;

    for (i = 0; i < MAXLABELS; i++)
    {
        if (labels[i].name == NULL)
            continue;

        if ((strcmp(labels[i].name, label) == 0) && labels[i].control)
            return(labels[i].pDevice);
    }

    return (UCONX_DEVICE *) 0;
}

/*
* Return the lower device label for closing after an UNLINK if the label is
* not a control label.
*
* This assumes that the lower device is a non-CLONEABLE device which should
* be freed after closing.
*/
char *
get_lower_label(char *label)
{
    int i;

    for (i = 0; i < MAXLABELS; i++)
    {
        if (labels[i].name == NULL)
            continue;

        /*
        * If it is the relevant label and not a control STREAM.
        */
        if ((strcmp(labels[i].name, label) == 0) && labels[i].control)
            return((labels[i].lower)? labels[i].lower->name:
                          (char *)0);
    }
    return NULL;
}


void delete_lower_ref(char *label)
{
    int i;

    for (i = 0; i < MAXLABELS; i++)
    {
        if (labels[i].name == NULL)
            continue;

        /*
        * If it is the relevant label
        */
        if (strcmp(labels[i].name, label) == 0)
        {
            if (labels[i].lower)
            {
                netd_dec_lref(labels[i].lower);
                labels[i].lower = NULL;
            }
        }
    }
}

/*
    Decrement the number of references to a label. This is to keep track
    of the number of times a label/STREAM has been linked under a
    multiplexor.
*/
static void netd_dec_lref(struct label *lptr)
{
    lptr->refs--;

    if ( ! lptr->refs)
        netd_delete_label(lptr->name);
}
