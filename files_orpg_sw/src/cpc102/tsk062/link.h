/**************************************************************************
   
   Module:  link.h
   
   Description:  
   This file contains the implementation for the link calss to be used
   by the link list.
   
   Assumptions:
   
   **************************************************************************/
/*
 * RCS info
 * $Author: aamirn $
 * $Locker:  $
 * $Date: 2003/08/25 14:30:50 $
 * $Id: link.h,v 1.1 2003/08/25 14:30:50 aamirn Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

#ifndef link_h
#define link_h

/*
 * System Include Files/Local Include Files
 */

/*
 * Constant Definitions/Macro Definitions/Type Definitions
 */

/*
 * Static Globals
 */

/* This enumerated type is in all caps to differentiate it from proposed
 * ANSI C++ standard type bool which is not yet available on all compilers.
 */
enum BOOL {FALSE, TRUE};

/* Link serves as the link for each individual cell in a generalized
 * link list. T can be of any type.
 */
template < class T >
class Link
{
public:
    Link( const T&, Link < T > * );     // constructor for Link
    T _info;      // this variable contains the data for the link.
    Link < T > * _next;     // link to the next cell in the list.
};

template < class T >
Link < T > ::Link( const T& i, Link < T > * n ) :     // constructor for the class Link.
    _info( i ),     // member initializer list for data member.
    _next( n )      // member initializer list for pointer to the next node.
{}

#endif
