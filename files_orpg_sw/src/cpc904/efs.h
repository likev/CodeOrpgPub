/*   @(#)efs.h	1.1	07 Jul 1998	*/

/*
 * Copyright (c) 1988-1997 Spider Software Limited
 *
 * This Source Code is furnished under Licence, and may not be
 * copied or distributed without express written agreement.
 *
 * All rights reserved.  Made in Scotland.
 *
 * Authors: Peter Woodhouse 
 *
 * efs.h of include module
 *
 * SpiderX25
 * @(#)$Id: efs.h,v 1.1 2000/02/25 17:14:17 john Exp $
 * 
 * SpiderX25 Release 8
 */

#define F_FILE FILE
#define F_EOF EOF

/*
 * Declarations for the file handling functions
 */

extern int   f_initialise();
extern F_FILE *f_fopen();
extern int   f_fclose();
extern char *f_fgets();
extern int   f_fgetc();
extern int   f_ungetc();

