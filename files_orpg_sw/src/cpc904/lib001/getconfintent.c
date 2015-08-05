static char sccsid[] = "@(#)getconfintent.c	1.2	12 Aug 1998";
/*
 * Copyright (c) 1988-1997 Spider Software Limited
 *
 * This Source Code is furnished under Licence, and may not be
 * copied or distributed without express written agreement.
 *
 * All rights reserved.  Made in Scotland.
 *
 * Authors: Ian Lartey
 *
 * getconfintent.c of sx25 module
 *
 * SpiderX25
 * @(#)$Id: getconfintent.c,v 1.4 2000/07/14 19:45:23 john Exp $
 * 
 * SpiderX25 Release 8
 */

/*
	Modification history:
 
	Chg	Date			Init	Description
   1.    10-AUG-98   mpb   Compile on Windows NT.
*/


/*
 *  GETCONFENT.C Get an subnet or interface entry from the X25CONF file 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <uint.h>
#include <x25_proto.h>
#include <xnetdb.h>
#include <sx25.h>

/*
* #define TRACE
*/

static FILE *xiface = NULL;
static char confline[BUFSIZ+1];
static struct confinterface intent;
static int intstayopen = 0;
static char currconfile[400] = "";

void setconfintent(char *confname, int f)
{
	if (strcmp(currconfile, confname)) {
		if (xiface != NULL) {
			fclose(xiface);
			xiface = NULL;
		}
		strcpy(currconfile, confname);
	}

	if (xiface == NULL)
		xiface = fopen(confname, "r" );
	else
        if(intstayopen != 2 || f == 0)				/*2:C*/
			rewind(xiface);
    intstayopen = f;
}

void endconfintent(void)
{
	if (xiface && !intstayopen) {
		fclose(xiface);
		xiface = NULL;
	}
}

struct confinterface *getconfifacent(char *confname)
{
	char *p;
	register char *cp;

#ifdef TRACE
	if (confname == 0)
	    printf ("getconfifacent\n");
	else
	    printf ("getconfifacent: %s\n", confname);
#endif

	if (strcmp(currconfile, confname)) {
		if (xiface != NULL) {
			fclose(xiface);
			xiface = NULL;
		}
		strcpy(currconfile, confname);
	}

	if (xiface == NULL && (xiface = fopen(currconfile, "r" )) == NULL)
		return (NULL);

	for (;;)
	{
		if ((p = fgets(confline, BUFSIZ, xiface)) == NULL)
			return (NULL);
#ifdef TRACE
		printf (" %s\n", p);
#endif
		/* If the line is a comment then ignore */

		if (*p == '#')
			continue;

		cp = any(p, "#\n");
		if (cp == NULL)
			continue;
		*cp = '\0';

		/* Parse the dev_type field */

		cp = any(p, " \t");
		if (cp == NULL)
			continue;
		*cp++ = '\0';

		if(strcmp(p,IFACE) != 0)
			continue;
		break;
	}
	
	intent.dev_type = p;

	while (*cp == ' ' || *cp == '\t')
		cp++;

	p = cp;

	/* Parse the snid field */

	cp = any(p, " \t");
	if (cp != NULL) 
		*cp++ = '\0';
	intent.snid = snidtox25((unsigned char *)p);

	while (*cp == ' ' || *cp == '\t')
		cp++;
#ifdef TRACE
	printf ("snid field %x\n", intent.snid);
#endif
	/* Parse the llsnid field */

	p = cp;
	cp = any(p, " \t");
	if (cp != NULL) 
		*cp++ = '\0';
	intent.llsnid = snidtox25((unsigned char *)p);

	while (*cp == ' ' || *cp == '\t')
		cp++;

	/* next field unused in generic x25 */

	p = cp;
	cp = any(p, " \t");

	while (*cp == ' ' || *cp == '\t')
		cp++;

	/* Parse the lreg field */

	p = cp;
	cp = any(p, " \t");
	if (cp != NULL) 
		*cp++ = '\0';
	intent.lreg = p;

	while (*cp == ' ' || *cp == '\t')
		cp++;

	/* Parse the dl_template field */

	p = cp;
	cp = any(p, " \t");
	if (cp != NULL) 
		*cp++ = '\0';
	intent.dl_template = p;

	while (*cp == ' ' || *cp == '\t')
		cp++;

	/* Parse the wan_template field */

	p = cp;
	cp = any(p, " \t");
	if (cp != NULL) 
		*cp++ = '\0';
	intent.wan_template = p;

	while (*cp == ' ' || *cp == '\t')
		cp++;

	/* Parse the wanmap_file field */

	p = cp;
	cp = any(p, " \t");
	if (cp != NULL) 
		*cp++ = '\0';
	intent.wanmap_file = p;

	while (*cp == ' ' || *cp == '\t')
		cp++;


	/* Parse the priority field */

	p = cp;
	cp = any(p, " \t");
	if (cp != NULL) 
		*cp++ = '\0';
	intent.priority = atoi(p);

	while (*cp == ' ' || *cp == '\t')
		cp++;

	/* Parse the board_number field */

	p = cp;
	cp = any(p, " \t");
	if (cp != NULL) 
		*cp++ = '\0';
	intent.board_number = atoi(p);

	while (*cp == ' ' || *cp == '\t')
		cp++;

	/* Parse the line_number field */

	p = cp;
	cp = any(p, " \t");
	if (cp != NULL) 
		*cp++ = '\0';
	intent.line_number = atoi(p);
	return(&intent);
}

struct confinterface *confifacentdup(struct confinterface * src_p)
{
    struct confinterface * new_p;

    int dev_type_len;
    int lreg_len;
    int dl_template_len;
    int wan_template_len = 0;
    int wanmap_file_len = 0;
    int total_len;

    char * c_ptr;

    dev_type_len    = strlen(src_p->dev_type) + 1;
    lreg_len        = strlen(src_p->lreg) + 1;
    dl_template_len = strlen(src_p->dl_template) + 1;
    if (src_p->wan_template != NULL) {
	wan_template_len = strlen(src_p->wan_template) + 1;
    }
    if (src_p->wanmap_file != NULL) {
	wanmap_file_len = strlen(src_p->wanmap_file) + 1;
    }

    /* start with the size of 'confinterface' struct */
    total_len = sizeof(struct confinterface);

    /* and add the length of all the strings */
    total_len += (dev_type_len + lreg_len +
		  dl_template_len + wan_template_len + wanmap_file_len);

    if ((new_p = (struct confinterface *) malloc(total_len)) != NULL)
    {
	c_ptr = (char *) new_p + sizeof(struct confinterface);

	strcpy(c_ptr, src_p->dev_type);
	new_p->dev_type = c_ptr;
	c_ptr += dev_type_len;

	strcpy(c_ptr, src_p->wan_template);
	new_p->wan_template = c_ptr;
	c_ptr += wan_template_len;

	strcpy(c_ptr, src_p->wanmap_file);
	new_p->wanmap_file = c_ptr;
	c_ptr += wanmap_file_len;

	strcpy(c_ptr, src_p->lreg);
	new_p->lreg = c_ptr;
	c_ptr += lreg_len;

	strcpy(c_ptr, src_p->dl_template);
	new_p->dl_template = c_ptr;
	c_ptr += dl_template_len;

	if (wan_template_len) {
	    strcpy(c_ptr, src_p->wan_template);
	    new_p->wan_template = c_ptr;
	    c_ptr += wan_template_len;
	}
	else {
	    new_p->wan_template = NULL;
	}

	if (wanmap_file_len) {
	    new_p->wanmap_file = c_ptr;
	    strcpy(c_ptr, src_p->wanmap_file);
	}
	else {
	    new_p->wanmap_file = NULL;
	}

	new_p->snid         = src_p->snid;
	new_p->llsnid       = src_p->llsnid;
	new_p->priority     = src_p->priority;
	new_p->board_number = src_p->board_number;
	new_p->line_number  = src_p->line_number;
    }

    return(new_p);
}

