/*   @(#) mps.c 00/01/05 Version 1.4   */


 
/***************************************************************************
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
****************************************************************************/
 
/***************************************************************************
*
* mps.c
*
***************************************************************************/

/*
Modification history:
 
Chg Date       Init Description
6.  19-may-98  rjp  Merge with Spider release 8.1.1
7.  08-oct-99  tdg  Added VxWorks support.
*/

#include        <stdio.h>
#include        <stdlib.h>
#include        <string.h>
#include        <fcntl.h>
#include        <sys/types.h>
#ifndef WINNT
#include        <sys/socket.h>
#else
#include        <winsock.h>    /* #3 */
#endif /* WINNT */

/* #5 */
#if defined ( __hp9000s800 ) || defined ( WINNT ) || defined ( QNX )
#include        "xstypes.h"
#include        "xstra.h"
#else
#ifdef VXWORKS
#include  	<streams/stream.h>
#else
#ifndef LINUX
#include        <sys/stream.h>
#endif
#endif /* VXWORKS */
#endif

#include        "xstopts.h"
#ifndef WINNT
#include        "errno.h"   /* #4 */
#endif /* WINNT */
#include    "debug.h"
#include    "cmu_dnetd_mps.h"

#include    "mpsproto.h"

extern int    MPSerrno, errno;

static int    	nobuild;

static int	xopen();
static int    	xclose();
static int    	xlink();

/*************************************************************************
* doopen 
*
*************************************************************************/
int
doOpen (pUconxDevice)
    UCONX_DEVICE *pUconxDevice;
{
    int           fd;
    OpenRequest   oreq;

    memset (&oreq, 0, sizeof (oreq));
    strcpy (oreq.serverName, pUconxDevice->serverName);
    strcpy (oreq.serviceName,pUconxDevice->serviceName);
    oreq.port       = 0;
    oreq.ctlrNumber = pUconxDevice->ctlrNumber;
    strcpy (oreq.protoName,  pUconxDevice->protoName);
    oreq.dev        = pUconxDevice->device;
    oreq.flags      = O_RDWR;
    oreq.openTimeOut = 15;

    if ( nobuild )
        fd = xopen();
    else
    {
        TRACE (( "Sending open server request.\n"));
	if ( ( fd = MPSopen ( &oreq ) ) == ERROR )
	    MPSperror ( "Unable to open connection to server " );
	else
	    TRACE (( "Open server successful %d\n", fd));
    }
    TRACE ((" => %d\n", fd));
    return(fd);
}

int
doClose(fd)
int fd;
{
    int	result;

    TRACE (( "close ( %d )\n", fd));
    if ( nobuild ) 
           return ( xclose ( fd ) );
    TRACE (("Sending Close Server Request.\n"));

        if ( ( result = MPSclose ( fd ) ) == ERROR )
           printf ( "Unable to Close connection to server: %d %d\n",
                     MPSerrno, errno );
	else
    	    TRACE (("Close Server Successful %d\n", fd));
    return ( result );
}
int
doPush(fd, module)
int    fd;
char    *module;
{
    TRACE (("ioctl(%d, I_PUSH, \"%s\")\n", fd, module));
    if (nobuild)
	return (0);

    if ( MPSioctl ( fd, X_PUSH, module ) == ( -1 ) )
    	printf ("failed to push module \"%s\"", module);

    return(0);
}

int
doLink(ufd, lfd)
int ufd, lfd;
{
    int    muxid;

    TRACE (("ioctl(%d, I_LINK, %d)", ufd, lfd));
    if (nobuild)
        muxid = xlink();
    else
    {
	if ( ( muxid = MPSioctl ( ufd, X_LINK, lfd ) ) == ( -1 ) )
        {
	    printf("\nfailed to link stream %d under driver %d", lfd, ufd);
        }
    }
    TRACE ((" => %d\n", muxid));
    return(muxid);
}

int
doExpmux(ufd, muxid)
int ufd, muxid;
{
    if (muxid )
	TRACE (("muxid(%d) => %d\n", ufd, muxid));
    return 0;    /* Dummy fd value */
}

/*
 *  Emulate open/close/I_LINK for debugging.
 */

static long    xfds = 0L;

static int
xopen()
{
    int    fd;

    for (fd=0; fd<sizeof(long)*8; fd++)
    {
        if ((xfds&(01<<fd)) == 0L)
        {
            xfds |= (01<<fd);
            return fd;
        }
    }

    return -1;
}

static int
xclose(fd)
int fd;
{
    if (fd < 0 || sizeof(long)*8 <= fd)
        return -1;

    if (xfds&(01<<fd))
    {
        xfds &= ~(01<<fd);
        return 0;
    }

    return -1;
}

static int    xmuxid = 100;

static int
xlink()
{
    return xmuxid++;
}

/*************************************************************************
* uconx_device_create
*
* Init device structure with caller's parameters.
*
*************************************************************************/
int
uconx_device_create (pUconx_device, pServerName, pServiceName,
	pCtlrNumber, pProtoName, pDevice)
    UCONX_DEVICE	*pUconx_device;
    char		*pServerName;
    char		*pServiceName;
    char		*pCtlrNumber;
    char		*pProtoName;
    char		*pDevice;
{
    /*
    * Check server name.
    */
    if (strlen (pServerName) >= MAXSERVERLEN)
    {
        TRACE(("[server name %s too long. ]\n", pServerName));
        return -1;
    }
    strcpy (pUconx_device->serverName, pServerName);
    /*
    * Check service name.
    */
    if (strlen (pServiceName) >= MAXSERVNAMELEN)
    {
        TRACE(("[service name %s too long.]\n", pServiceName));
        return -1;
    }
    strcpy (pUconx_device->serviceName, pServiceName);
    /*
    * Check controller number.
    */
    pUconx_device->ctlrNumber = strtol (pCtlrNumber, (char **)NULL, 10);
    if (pUconx_device->ctlrNumber < 0)
    {
        TRACE(("[Controller number invalid %s.]\n", pCtlrNumber));
        return -1;
    }
    /*
    * Check protocol name.
    */
    if (strlen (pProtoName) >= MAXPROTOLEN)
    {
        TRACE(("[protocol name %s too long.]\n", pProtoName));
        return -1;
    }
    strcpy (pUconx_device->protoName, pProtoName);
    /*
    * Check device number.
    */
    pUconx_device->device = strtol (pDevice, (char **)NULL, 10);
    if (pUconx_device->device< 0)
    {
        TRACE(("[Controller number invalid %s. ]\n", pDevice));
        return -1;
    }
    return 1;
}

/*
* uconx_device_validate
*
*/
int
uconx_device_validate (pServerName, pServiceName, pCtlrNumber, pProtoName,
	pDevice)
    char	*pServerName;
    char	*pServiceName;
    char	*pCtlrNumber;
    char	*pProtoName;
    char	*pDevice;
{
    int		number;

    /*
    * Check server name.
    */
    if (strlen (pServerName) >= MAXSERVERLEN)
    {
        TRACE(("[server name %s too long.]\n", pServerName));
        return -1;
    }
    /*
    * Check service name.
    */
    if (strlen (pServiceName) >= MAXSERVNAMELEN)
    {
        TRACE(("[service name %s too long.]\n", pServiceName));
        return -1;
    }
    /*
    * Check controller number.
    */
    number = strtol (pCtlrNumber, (char **)NULL, 10);
    if (number < 0)
    {
        TRACE(("[Controller %s invalid.]\n", pCtlrNumber));
        return -1;
    }
    /*
    * Check protocol name.
    */
    if (strlen (pProtoName) >= MAXPROTOLEN)
    {
        TRACE(("[protocol name %s too long.]\n", pProtoName));
        return -1;
    }
    /*
    * Check device number.
    */
    number = strtol (pDevice, (char **)NULL, 10);
    if (number < 0)
    {
        TRACE(("[Device %s invalid.]\n", pDevice));
        return -1;
    }
    return 1;
}
