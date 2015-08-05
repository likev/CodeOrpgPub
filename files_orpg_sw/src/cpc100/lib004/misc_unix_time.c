/*
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2004/05/27 16:46:43 $
 * $Id: misc_unix_time.c,v 1.3 2004/05/27 16:46:43 jing Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

#include <config.h>
#include <time.h>
#include <misc.h>


/*************************************************************************

    Description: Conversion between the UNIX time and the (year, month, 
		day, hour, minute, second) representation. If "time" is
		non-zero, this function returns the year, month, 
		day, hour, minute and second as converted from "time". 
		Otherwise it assumes that "y", "mon", "d", "h", "m" and 
		"s" are inputs, converts them to UNIX time and stores the 
		result in "time".

    Inputs/Outputs:
		time - the UNIX time;
		y - year (e.g. 1995; >= 1970)
		mon - month (1 through 12)
		d - day (1, 2, 3, ...)
		h - hour (0 through 23)
		m - minute (0 through 59)
		s - second (0 through 59)

    Returns:	The function returns 0 on success or -1 on failure (an 
		argument is out of range).       

    Notes:	This function uses unsigned int internally. It works up 
		to at least the end of year 2104 on 32 bit machines.

		This function is supposed to be efficient.

		Constants used in the functions:

		671 = 1461 - 790, where 790 = 365 * 2 + 31 + 29 is the 
			number of days to go to the end of the first Feb 
			29th since 1970.01.01.00.00.00;

		1461 = 365 * 4 + 1: number of days in a 4 year period; 

		17 = 25 - 8 (92 = 100 - 8); where 8 comes from the fact 
			that the first 100(400)-year-correction happens 
			at the 8-th leap year since 1970.01.01.00.00.00;

		25 (100): The number of 4-year period in 100(400)-year 
			period

		To compute number of extra days due to leap years given 
		total days requires solving an equation of integer 
		computations. I use an iterative algorithm to solve this 
		equation. The iteration terminates rapidly (1 to 3 
		iterations). I have a proof of the convergence of this 
		algorithm.

*************************************************************************/

int 
unix_time (time_t *time, int *y, int *mon, int *d, int *h, int *m, int *s)
{
    /* number of days of the year before any given month */
    static unsigned short mon_tbl[12] =
    {0, 31, 59, 90, 120, 151, 181, 212,
     243, 273, 304, 334};	

    if (*time == 0) {			/* find UNIX time */
	unsigned int tm, td;
	int n_extra;

        if (*mon < 1 || *mon > 12 || *y < 1970)
            return (-1);
        td = 365 * (*y - 1970) + mon_tbl[*mon - 1] + *d;
					/* total days without the extra days 
					   due to leap years */

	/* number of extra days due to the leap years */
        n_extra = (*y - 1968) / 4 - (*y - 1900) / 100 + (*y - 1600) / 400;
        if (*mon <= 2 && *y % 4 == 0 && (*y % 100 != 0 || *y % 400 == 0))
            n_extra -= 1;

        tm = (((td + n_extra - 1) * 24 + *h) * 60 + *m) * 60 + *s; 

	*time = tm;

        return (0);
    }
    else {				/* find year, month, ... */
	unsigned int tm, td, bd;
	int dy, dm, yy, mm, n_extra, n_cor;

        tm = *time;
        *s = tm % 60;			/* seconds */
        tm = tm / 60;
        *m = tm % 60;			/* minutes */
        tm = tm / 60;
        *h = tm % 24;			/* hours */
        tm = tm / 24;
        td = tm;			/* total number of days since 
					   1970.01.01.00.00.00 */

	/* compute how many extra days due to leap years */
	n_cor = 0;			/* the 100(400)-year-correction */
	while (1) {
	    int tmp_cor;

	    n_extra = (td + 671 + n_cor) / 1461;
					/* extra days ignoring the 
					   100(400)-year-correction */
	    tmp_cor = (n_extra + 17) / 25 - (n_extra + 92) / 100;
	    if (n_cor == tmp_cor)
		break;
	    else
		n_cor = tmp_cor;
	}

	bd = td - n_extra + n_cor;	/* total number of days excluding 
					   Feb29s */
        yy = 1970 + bd / 365;		/* year */
        dy = bd % 365;			/* day in the year */
        for (mm = 0; mm < 12; mm++)	/* month */
            if (dy < (int)mon_tbl[mm])
                break;
        dm = dy - mon_tbl[mm - 1] + 1;	/* day in the month */

        if ((yy % 100 != 0 || yy % 400 == 0) && 
			((td + 671 + n_cor) % 1461) == 1460) {
					/* Feb 29th of the leap year */
            mm = 2;
            dm = 29;
        }
        *y = yy;
        *mon = mm;
        *d = dm;

        return (0);
    }
}
