/*   @(#)bmalloc.h	1.1	07 Jul 1998	*/
/*
 * Copyright (c) 1988-1997 Spider Software Limited
 *
 * This Source Code is furnished under Licence, and may not be
 * copied or distributed without express written agreement.
 *
 * All rights reserved.  Made in Scotland.
 *
 * Authors: Gavin Shearer
 *
 * bmalloc.h of snet module
 *
 * SpiderX25
 * @(#)$Id: bmalloc.h,v 1.1 2000/02/25 17:14:04 john Exp $
 * 
 * SpiderX25 Release 8
 */

char *bmalloc(unsigned long size);
void bfree(char *ptr);
unsigned long bmalloc_bufcall(unsigned long size, void (*func)(), long arg);
void bmalloc_unbufcall(unsigned long id);

#ifdef BMALLOC_DEBUG
void traverse_free(void);
#ifdef BMALLOC_TEST
void bmalloc_test1(void);
void bmalloc_test2(void);
#endif
#endif
