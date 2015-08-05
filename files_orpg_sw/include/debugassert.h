/*
 * RCS info
 * $Author eforren$
 * $Locker:  $
 * $Date: 1998/07/07 17:56:20 $
 * $Id: debugassert.h,v 1.1 1998/07/07 17:56:20 eforren Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

#ifndef DEBUGASSERT_H
#define DEBUGASSERT_H

/*  This file implements assertions based on the presence of a DEBUG flag instead of
	the lack of an NDEBUG flag */

#ifdef DEBUG
#ifdef NDEBUG
#undef NDEBUG
#endif
#else
#ifndef NDEBUG
#define NDEBUG
#endif
#endif

#include <assert.h>

#endif  
