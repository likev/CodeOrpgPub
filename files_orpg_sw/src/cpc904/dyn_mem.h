/*   @(#)dyn_mem.h	1.1	07 Jul 1998	*/
 
 
/*******************************************************************************
*                            Copyright (c) 1998 by
*
*    ===     ===     ===         ===         ===
*    ===     ===   =======     =======     =======
*    ===     ===  ===   ===   ===   ===   ===   ===
*    ===     === ===     === ===     === ===     ===
*    ===     === ===     === ===     === ===     ===   ===            ===
*    ===     === ===         ===     === ===     ===  =====         ======
*    ===     === ===         ===     === ===     === ==  ===      =======
*    ===     === ===         ===     === ===     ===      ===    ===   =
*    ===     === ===         ===     === ===     ===       ===  ==
*    ===     === ===         ===     === ===     ===        =====
*    ===========================================================
*    ===     === ===         ===     === ===     ===        =====
*    ===     === ===         ===     === ===     ===       ==  ===
*    ===     === ===     === ===     === ===     ===      ==    ===
*    ===     === ===     === ===     === ===     ====   ===      ===
*     ===   ===   ===   ===   ===   ===  ===     =========        ===  ==
*      =======     =======     =======   ===     ========          =====
*        ===         ===         ===     ===     ======             ===
*
*      U   c   o   n   X      C   o   r   p   o   r   a   t   i   o   n
*
*      This software is furnished  under  a  license and may be used and
*      copied only  in  accordance  with  the  terms of such license and
*      with the inclusion of the above copyright notice.   This software
*      or any other copies thereof may not be provided or otherwise made
*      available to any other person.   No title to and ownership of the
*      program is hereby transferred.
*
*      The information  in  this  software  is subject to change without
*      notice  and  should  not be considered as a commitment by UconX
*      Corporation.
*
*******************************************************************************/
 
/*
Modification history:
 
Chg Date       Init Description
1.  13-May-98  rjp  Define MEM_ALLOC and MEM_FREE to be malloc and free
		    respectively.

*/
 
 

/*
 * Copyright (c) 1988-1997 Spider Software Limited
 *
 * This Source Code is furnished under Licence, and may not be
 * copied or distributed without express written agreement.
 *
 * All rights reserved.  Made in Scotland.
 *
 * Written by Gavin Shearer.
 *
 * dyn_mem.h of snet module
 *
 * SpiderX25
 * @(#)$Id: dyn_mem.h,v 1.1 2000/02/25 17:14:15 john Exp $
 * 
 * SpiderX25 Release 8
 */


#define TYPE_PAGE_MEM           1
#define TYPE_UNPAGED_MEM        2


#define MEM_NULL		NULL


#ifdef SVR4
#define MEM_ALLOC(sz, type)	kmem_alloc(sz, KM_NOSLEEP)
#define MEM_FREE(ptr, sz)	kmem_free(ptr, sz)
#else
#define MEM_ALLOC(sz, type)	malloc(sz)
#define MEM_FREE(ptr, sz)	free(ptr)
extern	char *			malloc();
extern	void			free();
#endif	/* SVR4 */



