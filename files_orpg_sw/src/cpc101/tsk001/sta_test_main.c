
/******************************************************************

    The main module of sync_to_adapt - Synchronize RPG data structs
    to the adaptation data.
 
******************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2006/03/28 22:59:39 $
 * $Id: sta_test_main.c,v 1.1 2006/03/28 22:59:39 steves Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>

#include <orpg.h>
#include <infr.h>
#include <rpg_port.h>
#include "sta_def.h"

/* Thge legacy color tables. Reflectivity tables are not currently editable. I
   put here for completeness. "VAD 84 Reflectivity Data Levels (Precip/8)" is
   not defined in DEA. So I comment it out. */
static Color_table_t C_tabs[] = {
    {"Vel_data_level_precip_16_97.code",	4,	0,
	"ND -64 -50 -36 -26 -20 -10 -1 0 10 20 26 36 50 64 RF", 1},
    {"Vel_data_level_precip_16_194.code",	6,	0,
	"ND -64 -50 -36 -26 -20 -10 -1 0 10 20 26 36 50 64 RF", 1},
    {"Vel_data_level_precip_8_97.code",		5,	0,
	"ND -10 -5 -1 0 5 10 RF", 				1},
    {"Vel_data_level_precip_8_194.code",	7,	0,
	"ND -10 -5 -1 0 5 10 RF", 				1},
    {"Vel_data_level_clear_16_97.code",		26,	0,
	"ND -64 -50 -36 -26 -20 -10 -1 0 10 20 26 36 50 64 RF", 1},
    {"Vel_data_level_clear_16_194.code",	28,	0,
	"ND -64 -50 -36 -26 -20 -10 -1 0 10 20 26 36 50 64 RF", 1},
    {"Vel_data_level_clear_8_97.code",		27,	0,
	"ND -10 -5 -1 0 5 10 RF", 				1},
    {"Vel_data_level_clear_8_194.code",		29,	0,
	"ND -10 -5 -1 0 5 10 RF", 				1},
    {"STP_data_levels.code",			23,	0,
	"ND 0. .3 .6 1. 1.5 2. 2.5 3. 4. 5. 6. 8. 10. 12. 15.",	1},
    {"OHP/THP_data_levels.code",		22,	0,
	"ND 0. .1 .25 .5 .75 1. 1.25 1.5 1.75 2. 2.5 3. 4. 6. 8.", 1},
    {"RCM_reflectivity_data_levels.code",	20,	0,
	"ND 15 30 40 45 50 55 BLANK",				1},
    {"Reflectivity_data_levels_clear_16.code",	2,	0,
	"ND -28 -24 -20 -16 -12 -8 -4 0 4 8 12 16 20 24 28", 	1},
    {"Reflectivity_data_levels_precip_8.code",	3,	0,
	"ND 5 18 30 41 46 50 57", 				1},
    {"Reflectivity_data_levels_precip_16.code",	1,	0,
	"ND 5 10 15 20 25 30 35 40 45 50 55 60 65 70 75", 	1},
    {"VAD_84_reflectivity_data_levels_precip_8.code",	25,	0,
	"5 5 18 30 41 46 50", 					1}
};
static int N_c_tables = sizeof (C_tabs) / sizeof (Color_table_t);
static int N_upd_tables  = 11;
		/* number of tables need to be updated for the moment */

static color_table_t Ctbl;

static void Print_code (unsigned short *code);
static void Print_color (unsigned short *color);
static int Get_init_thresholds (int tbl_ind, char **thr);

/*********************************************************************


*********************************************************************/

int main (int argc, char **argv) {
    int tbl_ind, fd, ret, i;
    char *fname;

    if (argc < 2 ||
	sscanf (argv[1], "%d", &tbl_ind) != 1 ||
	tbl_ind < 0 || tbl_ind > 34) {
	printf ("Usage: %s table_index (1 - 34 or 0 for all)\n", argv[0]);
	exit (0);
    }
    tbl_ind--;

    fname = "/export/home/jing/data/adapt/Adapt_data.lb";
    fd = LB_open (fname, LB_READ, NULL);
    if (fd < 0) {
	printf ("LB_open %s failed (%d)\n", fname, fd);
	exit (0);
    }
    ret = LB_read (fd, (char *)&Ctbl, sizeof (Ctbl), 10);
    if (ret != sizeof (Ctbl)) {
	printf ("LB_read color table message failed (%d)\n", ret);
	exit (0);
    }

    for (i = 0; i < N_c_tables; i++) {
	unsigned short thb[16], cob[256];
	int ind, n_thrs;
	char *thr;

	ind = C_tabs[i].ind - 1;
	printf ("Test %d: %s\n", i, C_tabs[i].de_id);
	if (tbl_ind >= 0 && ind != tbl_ind)
	    continue;
	memcpy (thb, Ctbl.code[ind], 16 * sizeof (short));
	memcpy (cob, Ctbl.colors[ind], 256 * sizeof (short));
	n_thrs = Get_init_thresholds (ind, &thr);
	ret = STA_update_color_table (ind + 1, thr, n_thrs,
				    Ctbl.colors[ind], Ctbl.code[ind]);
	if (ret < 0)
	    exit (1);
	if (memcmp (thb, Ctbl.code[ind], 16 * sizeof (short)) != 0) {
	    Print_code (thb);
	    Print_code (Ctbl.code[ind]);
	}
	if (memcmp (cob, Ctbl.colors[ind], 256 * sizeof (short)) != 0) {
	    Print_color (cob);
	    Print_color (Ctbl.colors[ind]);
	}
    }
    exit (0);
}

/*********************************************************************


*********************************************************************/

static void Print_code (unsigned short *code) {
    unsigned short c;
    unsigned char b;
    int i;

    for (i = 0; i < 16; i++) {

	c = code[i];

	if (c & 0x800)
	    printf ("> ");
	else if (c & 0x800)
	    printf ("> ");
	else if (c & 0x400)
	    printf ("< ");
	else if (c & 0x200)
	    printf ("+ ");
	else if (c & 0x100)
	    printf ("- ");
	else
	    printf ("  ");


	if (c & 0x8000) {

	    if ((c & 0xff) == 0)
		printf ("BL ");
	    else if ((c & 0xff) == 1)
		printf ("TH ");
	    else if ((c & 0xff) == 2)
		printf ("ND ");
	    else if ((c & 0xff) == 3)
		printf ("RF ");
	    else
		printf ("   ");
	}
	else {
	    b = (unsigned char)c;
	    printf ("%d", b);
	}

	if (c & 0x2000)
	    printf ("* 20 ");
	else if (c & 0x1000)
	    printf ("* 10 ");
	else
	    printf ("     ");
	printf ("\n");
    }
}

/*********************************************************************


*********************************************************************/

static void Print_color (unsigned short *color) {
    unsigned short c;
    int i, sind, eind;

    c = 0xffff;
    sind = eind = 0;
    for (i = 0; i < 256; i++) {

	if (i == 0)
	    c = color[i];
	else if (color[i] != c) {
	    printf ("%d to %d: %d\n", sind, eind, c);
	    sind = i;
	    eind = i;
	    c = color[i];
	}
	eind = i;
    }
    printf ("%d to %d: %d\n", sind, eind, c);
}

/**********************************************************************

    Converts the initial threshold string of the table, whose C_tabs.ind
    matches "tbl_ind" + 1, to the array of null-terminated string form
    and returns it with "thr". Returns the number of strings found.

**********************************************************************/

static int Get_init_thresholds (int tbl_ind, char **thr) {
    static char *vb = NULL;
    int cnt, i;

    vb = STR_reset (vb, 128);
    *thr = vb;
    cnt = 0;
    for (i = 0; i < N_c_tables; i++) {

	if (tbl_ind + 1 == C_tabs[i].ind) {
	    char str[128], *p;

	    p = C_tabs[i].init_thr;
	    while (*p != '\0') {
		if (sscanf (p, "%s", str) <= 0)
		    return (cnt);
		vb = STR_append (vb, str, strlen (str) + 1);
		cnt++;
		while (*p != ' ' && *p != '\0')
		    p++;
		while (*p == ' ')
		    p++;
	    }
	}
    }
    return (cnt);
}

