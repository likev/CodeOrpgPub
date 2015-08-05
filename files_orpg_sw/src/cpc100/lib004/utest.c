



#include <stdio.h>

main ()
{
    static int yy [] = {1970, 1971, 1976, 1992, 1996, 1997, 2000, 2001, 2004, 2005, 2008, 2096, 2100, 2101, 0};
    int y, m, d, mon, h, s, i;
    unsigned int tm, tms;
    int ind;

    h = m = s = 0;

    ind = 0;
    while (1) {

	if (yy [ind] == 0)
	    exit (0);
	y = yy [ind];
	ind++;

	mon = 2;
	d = 27;
	tm = 0;
	unix_time (&tm, &y, &mon, &d, &h, &m, &s);

	for (i = 0; i < 6; i++) {

	    tms = tm;

	    unix_time (&tm, &y, &mon, &d, &h, &m, &s);
	    tm = 0;
	    unix_time (&tm, &y, &mon, &d, &h, &m, &s);
	    if (tm != tms) {

		printf ("error: %d %d %d %d %d %d,   %u %u \n", 
			y, mon, d, h, m, s, tm, tms);
		exit (-1);
	    }

	    printf ("%d %d %d %d %d %d,   %u\n", 
			y, mon, d, h, m, s, tm);
	    tm += 86400;
	}
    }

/*
    while (1) {

	y = 1970 + rand () % 100;

	d = 27;

	for (i = 0; i < 6; i++) {

	    tm = 0;
	    unix_time (&tm, &y, &mon, &d, &h, &m, &s);
	    tms = tm;

	    unix_time (&tm, &y, &mon, &d, &h, &m, &s);
	    tm = 0;
	    unix_time (&tm, &y, &mon, &d, &h, &m, &s);
	    if (tm != tms) {

		printf ("error: %d %d %d %d %d %d,   %u %n\n", 
			y, mon, d, h, m, s, tm, tms)
		exit (-1);
	    }

	    if (d == 29) {
		printf (
	    }
	}

    }
*/

}








