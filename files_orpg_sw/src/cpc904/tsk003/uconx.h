/*   @(#) uconx.h 99/11/02 Version 1.4   */

/*
Modification history:
 
Chg Date	Init Description
 1.  6-23-98	rjp  Coded.
 2.  7-29-98	rjp  Default service is "mps".
 3.  13-AUG-98	mpb  Take out UconX API prototypes since they are in
                     mpsproto.h.

*/

void	exit_program ();

#define DEFAULT_SERVICE  "mps"

#ifdef WINNT
#define DEFAULT_SERVER  "\\\\.\\PTIXpqcc"
#else
#define DEFAULT_SERVER  "/dev/ucsclone"
#endif /* WINNT */

