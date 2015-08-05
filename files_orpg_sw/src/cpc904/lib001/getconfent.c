static char sccsid[] = "@(#)getconfent.c	1.2	12 Aug 1998";
/*
 * Copyright (c) 1988-1997 Spider Software Limited
 *
 * This Source Code is furnished under Licence, and may not be
 * copied or distributed without express written agreement.
 *
 * All rights reserved.  Made in Scotland.
 *
 * Modified by Dougal Featherstone
 *
 * getconfent.c of sx25 module
 *
 * SpiderX25
 * @(#)$Id: getconfent.c,v 1.4 2000/07/14 19:45:22 john Exp $
 * 
 * SpiderX25 Release 8
 */

/*
	Modification history:
 
	Chg	Date			Init	Description
   1.    10-AUG-98   mpb   Compile on Windows NT.
*/


/* GETCONFENT.C - Get an subnet or interface entry from the X25ACT file */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <uint.h>
#include <x25_proto.h>
#include <xnetdb.h>
#include <sx25.h>


static FILE *xconf = NULL;
static char line[BUFSIZ+1];
static struct confsubnet subent;
static int stayopen = 0;
static char currconfile[400] = "";

void setconfent(char *confname, int f)
{
	if (strcmp(currconfile, confname)) {
		if (xconf != NULL) {
			fclose(xconf);
			xconf = NULL;
		}
		strcpy(currconfile, confname);
	}

	if (xconf == NULL)
		xconf = fopen(confname, "r" );
	else
	    if(stayopen != 2 || f == 0)
		rewind(xconf);
	stayopen = f;
}

void endconfent(char *confname)
{
	if ((strcmp(currconfile, confname) == 0) && 
		xconf && !stayopen) {
			fclose(xconf);
			xconf = NULL;
		}
}

struct confsubnet *getconfsubent(char *confname)
{
	char *p;
	register char *cp;
	int no_desc = 0;

	if (strcmp(currconfile, confname)) {
		if (xconf != NULL) {
			fclose(xconf);
			xconf = NULL;
		}
		strcpy(currconfile, confname);
	}

	if (xconf == NULL && (xconf = fopen(currconfile, "r" )) == NULL)
		return (NULL);
again:
	if ((p = fgets(line, BUFSIZ, xconf)) == NULL)
		return (NULL);

	/* If the line is a comment of the line is an interface entry - ignore 
 	 * the line and read the next line.
	 */

	if (*p == '#')
		goto again;

	/* The fields must be terminated by nulls instead of spaces for string
	 * functions to operate properly.
	 */

	cp = any(p, "#\n");
	if (cp == NULL)
		goto again;

	memset((char *)&subent, 0, sizeof(struct confsubnet));

	/* Parse the dev_type field */

	cp = any(p, " \t\n");
	if (cp == NULL || *cp == '\n')
		goto again;
	*cp++ = '\0';

	if(strcmp(p,IFACE) == 0)
		goto again;
	
	subent.dev_type = p;

	while (*cp == ' ' || *cp == '\t')
		cp++;

	/* Parse the friendly_name field */
	p = cp;
	cp = any(p, " \t\n");
	if (cp == NULL || *cp == '\n')
		goto again;
	*cp++ = '\0';

	subent.friendly_name = p;

	while (*cp == ' ' || *cp == '\t')
		cp++;

        /* Parse the x25reg field */

        p = cp;
        cp = any(p, " \t\n");
        if (cp == NULL || *cp == '\n')
                goto again;
        *cp++ = '\0';

        subent.x25reg = p;

        while (*cp == ' ' || *cp == '\t')
                cp++;

        /* Parse the snid field */

	p = cp;
        cp = any(p, " \t\n");
        if (cp == NULL || *cp == '\n')
                goto again;
        *cp++ = '\0';
        subent.snid = snidtox25( (unsigned char *)p );

        while (*cp == ' ' || *cp == '\t')
                cp++;

	/* Parse the net_type field */

	p = cp;
	cp = any(p, " \t\n");
	if (cp == NULL || *cp == '\n')
		goto again;
	*cp++ = '\0';

	subent.net_type = p;

	while (*cp == ' ' || *cp == '\t')
		cp++;

	/* Parse the x25_template field */

	p = cp;
	cp = any(p, " \t\n");
	if (cp == NULL || *cp == '\n')
		goto again;
	*cp++ = '\0';

	subent.x25_template = p;

	while (*cp == ' ' || *cp == '\t')
		cp++;

	/* Parse the mlp_template field */

	p = cp;
	cp = any(p, " \t\n");
	if (cp == NULL || *cp == '\n')
		goto again;
	*cp++ = '\0';

	if(*p == '-')
	    subent.mlp_template = NULL;
	else
	    subent.mlp_template = p;

	while (*cp == ' ' || *cp == '\t')
		cp++;

	/* Parse the x25addr field */

	p = cp;
	cp = any(p, " \t\n");
	if (cp == NULL || *cp == '\n')
		goto again;
	*cp++ = '\0';
	subent.x25addr = p;

	while (*cp == ' ' || *cp == '\t')
		cp++;

	/* Parse the nsap field */

	p = cp;
	cp = any(p, " \t\n");
	if (cp == NULL )
		goto again;


	if ( *cp == '\n' )
	{
		no_desc = 1;
		*cp = '\0';
	}
	else
		*cp++ = '\0';

	if(*p == '-')
	    subent.nsap = NULL;
	else
	    subent.nsap = p;

	if ( no_desc == 1 )
	{
		return(&subent);
	}

	while (*cp == ' ' || *cp == '\t')
        	cp++;
	

	/* Parse the description field */

	p = cp;
	cp = any(p, "\n#");
	subent.descrip = p;
	*cp = '\0';

	return (&subent);
}

struct confsubnet *confsubentdup(struct confsubnet * src_p)
{
    struct confsubnet * new_p;

    int dev_type_len;
    int friendly_name_len;
    int net_type_len;
    int x25reg_len;
    int x25_template_len;
    int mlp_template_len = 0;
    int x25addr_len;
    int nsap_len = 0;
    int descrip_len = 0;
    int total_len;

    char * c_ptr;

    dev_type_len      = strlen(src_p->dev_type) + 1;
    friendly_name_len = strlen(src_p->friendly_name) + 1;
    net_type_len      = strlen(src_p->net_type) + 1;
    x25reg_len        = strlen(src_p->x25reg) + 1;
    x25_template_len  = strlen(src_p->x25_template) + 1;
    if (src_p->mlp_template != NULL) {
	mlp_template_len  = strlen(src_p->mlp_template) + 1;
    }
    x25addr_len       = strlen(src_p->x25addr) + 1;
    if (src_p->nsap != NULL) {
	nsap_len = strlen(src_p->nsap) + 1;
    }
    if (src_p->descrip != NULL) {
	descrip_len = strlen(src_p->descrip) + 1;
    }

    /* start with the size of 'confsubnet' struct */
    total_len = sizeof(struct confsubnet);

    /* and add the length of all the strings */
    total_len += (dev_type_len + friendly_name_len + net_type_len +
		  x25reg_len + x25_template_len + mlp_template_len +
		  x25addr_len + nsap_len + descrip_len);

    if ((new_p = (struct confsubnet *) malloc(total_len)) != NULL)
    {
	c_ptr = (char *) new_p + sizeof(struct confsubnet);

	strcpy(c_ptr, src_p->dev_type);
	new_p->dev_type = c_ptr;
	c_ptr += dev_type_len;

	strcpy(c_ptr, src_p->friendly_name);
	new_p->friendly_name = c_ptr;
	c_ptr += friendly_name_len;

	strcpy(c_ptr, src_p->net_type);
	new_p->net_type = c_ptr;
	c_ptr += net_type_len;

	strcpy(c_ptr, src_p->x25reg);
	new_p->x25reg = c_ptr;
	c_ptr += x25reg_len;

	strcpy(c_ptr, src_p->x25_template);
	new_p->x25_template = c_ptr;
	c_ptr += x25_template_len;

	if (mlp_template_len) {
	    strcpy(c_ptr, src_p->mlp_template);
	    new_p->mlp_template = c_ptr;
	    c_ptr += mlp_template_len;
	}
	else {
	    new_p->mlp_template = NULL;
	}

	strcpy(c_ptr, src_p->x25addr);
	new_p->x25addr = c_ptr;
	c_ptr += x25addr_len;

	if (nsap_len) {
	    strcpy(c_ptr, src_p->nsap);
	    new_p->nsap = c_ptr;
	    c_ptr += nsap_len;
	}
	else {
	    new_p->nsap = NULL;
	}

	if (descrip_len) {
	    strcpy(c_ptr, src_p->descrip);
	    new_p->descrip = c_ptr;
	}
	else {
	    new_p->descrip = NULL;
	}

	new_p->snid = src_p->snid;
    }

    return(new_p);
}

