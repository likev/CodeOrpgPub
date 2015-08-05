/*   @(#) MPSlist.c 00/01/11 Version 1.7   */

/************************************************************************
*                        Copyright (c) 1999 by
*
*
*
*                 ======      =============  =============
*               ===    ===    =============  =============
*              ===      ===        ===            ===
*             ===        ===       ===            ===
*             ===        ===       ===            ===
*             ===        ===       ===            ===
*             ===        ===       ===            ===
*             ===       ===        ===            ===
*             ===      ===         ===            ===
*             === ======           ===            ===
*             ===                  ===            ===
*             ===                  ===            ===
*             ===                  ===            ===
*             ===                  ===            ===
*             ===                  ===            ===
*             ===                  ===       =============
*             ===                  ===       =============
*
*
*             P e r f o r m a n c e   T e c h n o l o g i e s
*                              Incorporated
*
*      This software is furnished under a license and may be used and
*      copied only in accordance with the terms of such license and
*      with the inclusion of the above copyright notice.   This software
*      or any other copies thereof may not be provided or otherwise made
*      available to any other person.   No title to and ownership of the
*      program is hereby transferred.
*
*      The information in this software is subject to change without
*      notice and should not be considered as a commitment by Performance 
*      Technologies Incorporated.
* 
*      Performance Technologies Incorporated
*      San Diego, California
*
*************************************************************************/

/*
Modification history:
 
Chg Date       Init Description
1.  14-MAY-96  mpb  Put pragma's in for 64 to 32 bit conversion (for esca).
2.  23-MAY-96  mpb  Added prototyping of functions if ANSI_C is #ifdef'd.
*/

#include "MPSinclude.h"


/*<-------------------------------------------------------------------------
| 
|			MPS List Routines
V  
-------------------------------------------------------------------------->*/


/*<-------------------------------------------------------------------------
| headFree()
|
| Puts a buffer at the head of a list.
V  
-------------------------------------------------------------------------->*/

#ifdef	ANSI_C
#ifdef	DECUX_32			/* #1 */
#pragma	pointer_size	save
#pragma	pointer_size	long
#endif /* DECUX_32 */

void headFree ( FreeList *p_list, FreeBuf *p_buf )

#ifdef	DECUX_32
#pragma	pointer_size	restore
#endif	/* DECUX_32 */
#else

void headFree(p_list, p_buf)
FreeList *p_list;
FreeBuf *p_buf;

#endif	/* ANSI_C */
{
   if ( p_list->head )
   {
      p_buf->next = p_list->head;
      p_list->head = p_buf;
   }
   else
   {
      p_buf->next = NULL;
      p_list->head = p_buf;
      p_list->tail = p_buf;
   }
} /* end headFree() */

/*<-------------------------------------------------------------------------
| addFree()
|
| Puts a released buffer on the free list.
V  
-------------------------------------------------------------------------->*/

#ifdef	ANSI_C
#ifdef	DECUX_32			/* #1 */
#pragma	pointer_size	save
#pragma	pointer_size	long
#endif /* DECUX_32 */

void addFree ( FreeList *p_list, FreeBuf *p_buf )

#ifdef	DECUX_32
#pragma	pointer_size	restore
#endif	/* DECUX_32 */
#else

void addFree (p_list, p_buf)
FreeList *p_list;
FreeBuf *p_buf;

#endif	/* ANSI_C */
{
   if ( p_buf->this_one != p_buf )
      exit(ERROR);

   p_buf->next = NULL;

   if ( p_list->head )
   {
      p_list->tail->next = p_buf;
      p_list->tail = p_buf;
   }
   else
   {
      p_list->head = p_buf;
      p_list->tail = p_buf;
   }
   return;
} /* end addFree() */


/*<-------------------------------------------------------------------------
| getFree()
|
| Gets a free buffer.
V  
-------------------------------------------------------------------------->*/

#ifdef	ANSI_C
#ifdef	DECUX_32			/* #1 */
#pragma	pointer_size	save
#pragma	pointer_size	long
#endif /* DECUX_32 */

FreeBuf *getFree ( FreeList *p_list )
{	/* need to have "{" here because this function returns a pointer. */

#ifdef	DECUX_32
#pragma	pointer_size	restore
#endif	/* DECUX_32 */
#else

FreeBuf *getFree(p_list)
FreeList *p_list;
{

#endif	/* ANSI_C */

#ifdef	DECUX_32			/* #1 */
#pragma	pointer_size	save
#pragma	pointer_size	long
#endif /* DECUX_32 */
   FreeBuf *p_buf;
#ifdef	DECUX_32
#pragma	pointer_size	restore
#endif	/* DECUX_32 */

   if ( !p_list->head )
   {
      p_list->tail = NULL;
      return(0);
   }

   p_buf = p_list->head;
   p_buf->this_one = p_buf;
   p_list->head = p_list->head->next;

   return(p_buf);
} /* end getFree() */


/*<-------------------------------------------------------------------------
| peekFree()
|
| Peeks at the head of a list to see if anything is there.
V  
-------------------------------------------------------------------------->*/

#ifdef	ANSI_C
#ifdef	DECUX_32			/* #1 */
#pragma	pointer_size	save
#pragma	pointer_size	long
#endif /* DECUX_32 */

FreeBuf *peekFree ( FreeList *p_list )
{	/* need to have "{" here because this function returns a pointer. */

#ifdef	DECUX_32
#pragma	pointer_size	restore
#endif	/* DECUX_32 */
#else

FreeBuf *peekFree(p_list)
FreeList *p_list;
{

#endif	/* ANSI_C */


   if ( !p_list->head )
      return(NULL);

   return((FreeBuf *)p_list->head);
} /* end peekFree() */


/*<-------------------------------------------------------------------------
| drainList()
|
V
-------------------------------------------------------------------------->*/

#ifdef	ANSI_C
#ifdef	DECUX_32			/* #1 */
#pragma	pointer_size	save
#pragma	pointer_size	long
#endif /* DECUX_32 */

void drainList ( FreeList *p_list )

#ifdef	DECUX_32
#pragma	pointer_size	restore
#endif	/* DECUX_32 */
#else

void drainList(p_list)
FreeList *p_list;

#endif	/* ANSI_C */
{
#ifdef	DECUX_32			/* #1 */
#pragma	pointer_size	save
#pragma	pointer_size	long
#endif /* DECUX_32 */
   FreeBuf *p_buf;
#ifdef	DECUX_32
#pragma	pointer_size	restore
#endif	/* DECUX_32 */

   while ( p_list->head )
   {
      p_buf = getFree( p_list );
      free(p_buf);
   }
   p_list->head = p_list->tail = NULL;
} /* end drainList() */

