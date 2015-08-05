/**************************************************************************
   
   Module:  list.h
   
   Description:  
   This file contains the implementation for the list class to be used
  by the link list.
   
   Assumptions:
   
   **************************************************************************/
/*
 * RCS info
 * $Author: aamirn $
 * $Locker:  $
 * $Date: 2003/08/25 14:30:51 $
 * $Id: list.h,v 1.1 2003/08/25 14:30:51 aamirn Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

#ifndef list_h
#define list_h 1

/*
 * System Include Files/Local Include Files
 */

/*
 * Constant Definitions/Macro Definitions/Type Definitions
 */

/*
 * Static Globals
 */

#include <link.h>

/* This is the class interface for the link list class. It
 * uses templates to build a generic link list.
 */
template < class T >
class List
{
public:
    List();  // prototype for default constructor

    List( const List < T > & );  // copy construcor. this constructor 
                                 // takes a list of type T and constructs
                                 // an identical copy.

    const List < T > & operator=( const List < T > & );  // overloaded =. one 
                                                         // list of type T can 
                                                         // be assigned to 
                                                         // another list same
                                                         // type.

    ~List();  // standard destructor.

    void insert_head( T n );  // This method inserts b4 the head of existing 
                              // list.

    void insert_tail( T n );  // This method inserts after the tail of 
                              // existing list

    void insert( T n );  // insert before the current cursor position.

    T remove();  // remove the current element from the list.

    void next();  // move cursor to the next position in the list.

    void reset();  // reset the cursor so it points at the head of the list.

    void empty();  // clear the list and release all memory. a null list.

    //const T& current() const;  // return the current element of the list.
    T& current() const;  // return the current element of the list.

    BOOL at_end() const;  // return TRUE if cursor is at the last member 
                          // of the list retrun FALSE otherwise. useful 
                          // for "for" loops.

    const T& head() const;  // return first element in the list.

    const T& tail() const;  // return last element in the list.

    int length() const;  // returns the total length of the list.

private:

    Link < T > * _head;  // this is the first element in the list.

    Link < T > * _tail;  // this is the last element in the list.

    Link < T > * _pre;   // this is the element right before the current 
                         // element.

    int _len;  // to keep track of the total length of the list.

    void free();  // release all memory to free store.

    void copy( const List < T > & );  // private copy function. takes a link 
                                      // list object and copies it into the 
                                      // present list.

    inline Link < T > * cursor() const;  // gives back a pointer to the current
                                         // element in the link list.
};

template < class T >
List < T > ::List() :  // default constructor.
    _head( 0 ),  // member initializer list to set all pointers to null
    _tail( 0 ),  // length to zero. used in default constructor.
    _pre( 0 ),
    _len( 0 )
{}

template < class T >
List < T > ::List( const List < T > & b )  // copy constructor.
{
  copy( b );  // copy the provided list into current list.
}

 // overloaded "=".
template < class T >
const List < T > & List < T > ::operator=( const List < T > & b ) 
{
  if ( this != &b )  // make sure both list are not pointing to the same
  {                  // dynamic memory.
    free();  // free all the existing memory of the link list.
    copy( b );  // copy the list in argument.
  }
  return *this;  // return a pointer to the newly formed link list.
}

template < class T >
List < T > ::~List()  // destructor.
{
  free();  // release all the memory to free store.
}

template < class T >
void List < T > ::insert_head( T n )  // insert new element before the head
{                                     //and make new element head of the list.
  Link < T > * p = new Link < T > ( n, _head );  // get the memory for 
                                                 // new element.
  _head = p;  // make new element the first element in the list.
  if ( _tail == 0 )  // if this is the first element then head and tail are 
                     // same.
    _tail = _head;
  _len++;  // we just added a new element to the list. increase length by one.
}

template < class T >
void List < T > ::insert_tail( T n )  // insert new element after the tail and 
                                      // make new element tail of the list.
{                              
  Link < T > * p = new Link < T > ( n, 0 );  // get the memory for new element.
  if ( _head == 0 ) _head = _tail = p;  // if head is pointing to null then 
                                        // this is the first element in the 
                                        // list.
  else  // else insert the new element after the tail of the list.
  {
    _tail->_next = p;
    _tail = p;  // make new element the tail of the list.
  }
  _len++;  // we just added a new element to the list. increase length by one.
}

template < class T >
void List < T > ::insert( T n )  // insert new element before the cursor.
{
  Link < T > * p = new Link < T > ( n, cursor() );  // get the memory for new
                                                    // element.
  if ( _pre )  // if the list is not empty and cursor is not pointing at head.
  {
    _pre->_next = p;  // insert new element before the cursor.
    if ( _pre == _tail ) _tail = p;  // If cursor was at the tail of the list
                                     // make new element tail.
  }
  else
  {
    if ( _head == 0 ) _tail = p;  // if list is null then make new element the
    _head = p;                    // first element in the list.
  }
  _pre = p;  // new element is now the element right before cursor.
  _len++;    // we just added a new element to the list. increase length by one.
};

template < class T >
T List < T > ::remove()  // remove the current element from the list.
{
  Link < T > * cur = cursor();  // assign to cur the elemented pointed to by 
                                // cursor.
  static T defaultval;  // create a default value of type T. This can create
                        // problem if T doesn't have default constructor.
  if ( !cur ) return defaultval;  // If list is empty then return the default.
  if ( _tail == cur ) _tail = _pre;  // if element to be removed is the last
                                     // then reassign the tail to previous
                                     // element.
  if ( _pre )  // If element to be removed is not the head
    _pre->_next = cur->_next;  // reassign the next pointer of the previous
                               // element to the element pointed to by
                               // next pointer of the current element.
  else  // element ot be removed is the head.
    _head = cur->_next;  // assign the next element in the list to be head.
  T r = cur->_info;  // get the data part of the element to be removed.
  delete cur;  // release the memory to free store.
  _len--;  // we just removed an element from the list. decrease length by one.
  return r;  // return the element removed.
}

template < class T >
void List < T > ::next()  // move the cursor to the next element.
{
  if ( !_pre ) _pre = _head;  // check to see if we are at the head of the ist.
  else if ( _pre->_next ) _pre = _pre->_next;  // do not advance past the tail
                                               // of the list
}

template < class T >
void List < T > ::reset()  // reset the cursor to point to the head of the list.
{
  _pre = 0;  // there is no previous element.
}

template < class T >
void List < T > ::empty()  // clear the list and release all memory.
{
  free();  // release all memory back to free store.
}

template < class T >
T& List < T > ::current() const  // return the current element of the 
//const T& List < T > ::current() const  // return the current element of the 
                                       // list.
{
  Link < T > * cur = cursor();  // create a new element of type link and point
                                // it to the element pointed to by cursor.
  return cur ? cur->_info : *( ( T * ) 0 );  // retrun the current element if it
                                             // is not null, otherwise return
                                             // an element of type T pointing
                                             // to null.
}

template < class T >
BOOL List < T > ::at_end() const  // return TRUE if cursor is at the last member
                                  // of the list return FALSE otherwise. useful
                                  // for "for" loops.
{
  if ( cursor() == 0 )
    return TRUE;
  else
    return FALSE;
}

template < class T >
const T& List < T > ::head() const
{
  return _head ? _head->_info : *( ( T * ) 0 );  // retrun the first element 
                                                 // if it is not null, 
                                                 // otherwise return an element
                                                 // of type T pointing to null.
}

template < class T >
const T& List < T > ::tail() const
{
  return _tail ? _tail->_info : *( ( T * ) 0 );  // retrun the first element if
                                                 // it is not null, otherwise 
                                                 // return an element of type 
                                                 // T pointing to null.
}

template < class T >
int List < T > ::length() const  // returns the total length of the list.
{
  return _len;  // returns an integer value indicating length.
}

template < class T >
void List < T > ::free()  // release memory to free store.
{
  Link < T > * p = _head;  // create a temporary pointer p to head.
  while ( p != 0 )  // iterate the list.
  {
    Link < T > * q = p;  // point temporary pointer q to p.
    p = p->_next;  // point p to the next element.
    delete q;  // delete q thus freeing the memory.
  }
  _head = _tail = _pre = ( Link < T > * ) 0;  // head, tail and pre all null
                                              // pointers now.
  _len = 0;  // list no more :-)
}

template < class T >
void List < T > ::copy( const List < T > & b )  // copy the given list to the
                                                // present list.
{
  _len = b._len;  // make the length of both lists to be equal.
  _pre = _head = 0;  // make the present list empty.
  Link < T > * pa = 0;  // create the link pointer.
  for ( Link < T > * pb = b._head; pb != 0; pb = pb->_next )  // iterate the 
                                                              // length of the
                                                              // list to be 
                                                              // copied from.
  {
    Link < T > * pnew = new Link < T > ( pb->_info, 0 );  // assign new memory
                                                          //for each element.
    if ( pa )  // if pa is not the first element.
      pa = pa->_next = pnew;  // new element is added.
    else   // if pa is the first element
      pa = _head = pnew;  // make it the head of the list.
    if ( b._pre == pb ) _pre = pa;  // if at the tail of the list
  }
  _tail = pa;  // last element in the list.
}

template < class T >
inline Link < T > * List < T > ::cursor() const  // current element of the list.
{
  return _pre ? _pre->_next : _head ;  // return the next element to _pre
                                       // _pre is not null, otherwise we
                                       // are at the head of the list.
}

#endif
