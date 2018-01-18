// -*- Mode: C++; -*-
//                              File      : RDIstrsteam.h
//                              Package   : omniNotify-Library
//                              Created on: 1-July-2001
//                              Authors   : gruber
//
//    Copyright (C) 1998-2003 AT&T Laboratories -- Research
//
//    This file is part of the omniNotify library
//    and is distributed with the omniNotify release.
//
//    The omniNotify library is free software; you can redistribute it and/or
//    modify it under the terms of the GNU Library General Public
//    License as published by the Free Software Foundation; either
//    version 2 of the License, or (at your option) any later version.
//
//    This library is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//    Library General Public License for more details.
//
//    You should have received a copy of the GNU Library General Public
//    License along with this library; if not, write to the Free
//    Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
//    02111-1307, USA
//
//
// Description:
//    Class RDIstrstream: string construction using operator<<
//        for types relevant to the omniNotify implementation

#ifndef __RDI_STRSTREAM_H__
#define __RDI_STRSTREAM_H__

#include "corba_wrappers.h"

// ** NB:  the type of CORBA::Boolean maps to one of the basic types (depends on
// platform).  Need to cast to int to get a "0" or "1" to print
// (otherwise may be treated as a char)

class RDIstrstream {
public:
  RDIstrstream();
  ~RDIstrstream();

  // *** Methods borrowed from omniORB::logger ***
  RDIstrstream& operator<<(char c);
  RDIstrstream& operator<<(unsigned char c) { return (*this) << (char)c; }
  RDIstrstream& operator<<(signed char c) { return (*this) << (char)c; }
  RDIstrstream& operator<<(const char *s);
  RDIstrstream& operator<<(const unsigned char *s) {
    return (*this) << (const char*)s;
  }
  RDIstrstream& operator<<(const signed char *s) {
    return (*this) << (const char*)s;
  }
  RDIstrstream& operator<<(const void *p);
  RDIstrstream& operator<<(int n);
  RDIstrstream& operator<<(unsigned int n);
  RDIstrstream& operator<<(long n);
  RDIstrstream& operator<<(unsigned long n);
  RDIstrstream& operator<<(short n) {return operator<<((int)n);}
  RDIstrstream& operator<<(unsigned short n) {return operator<<((unsigned int)n);}
#ifdef HAS_LongLong
#if SIZEOF_LONG_LONG != SIZEOF_LONG
  RDIstrstream& operator<<(WRAPPED_CORBA_LONGLONG_TYPE n);
  RDIstrstream& operator<<(WRAPPED_CORBA_ULONGLONG_TYPE n);
#endif
#endif
#ifdef HAS_Cplusplus_Bool
  RDIstrstream& operator<<(bool b) { return operator<<((int)b); }
#endif
#ifndef NO_FLOAT
  RDIstrstream& operator<<(double n);
  RDIstrstream& operator<<(float n) { return operator<<((double)n); }
#ifdef HAS_LongDouble
  RDIstrstream& operator<<(WRAPPED_CORBA_LONGDOUBLE_TYPE n);
#endif
#endif

  // *** Methods not in omniORB::logger ***
  // NB setw not guaranteed to work on these methods, just on the methods above
  RDIstrstream& operator<< (const CORBA::Exception &e);
  RDIstrstream& operator<< (const CORBA::TCKind& tckind);
  RDIstrstream& operator<< (const CORBA::TypeCode &tc);
  RDIstrstream& operator<< (const CORBA::Any &a);

  void setw(int n) { _width = _p + n; }
  void clear();
  const char* buf();
  char* retn();
  size_t len() { return (size_t)(_p - _buf); }

protected:
  RDIstrstream(const RDIstrstream&);
  RDIstrstream& operator=(const RDIstrstream&);

  inline void reserve(int n) { if ( _end - _p - 1 < n )  more(n); }
  void more(int n);
  void width_fill();

  char*       _buf;
  char*       _width;  // if _width > _p, setw has been called
  char*       _p;      // assert(*_p == '\0')
  char*       _end;    // assert(_p < _end)
  // note: _width can be >= _end
};

#endif /*  __RDI_STRSTREAM_H__  */
