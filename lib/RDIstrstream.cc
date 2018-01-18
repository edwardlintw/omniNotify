// -*- Mode: C++; -*-
//                              File      : RDIstrstream.cc
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
 
#include "RDIstrstream.h"
#include "RDIStringDefs.h"

// --------------------------------------------------------------------------------
// class RDI::strstream.  Borrows some from omniORB::logger

#ifdef RDI_INIT_BUF_SIZE
#undef RDI_INIT_BUF_SIZE
#endif
#define RDI_INIT_BUF_SIZE  256

RDIstrstream::RDIstrstream()
  : _buf(new char[RDI_INIT_BUF_SIZE])
{
  _p = _buf;
  _width = _p;
  _p[0] = '\0';
  _end = _buf + RDI_INIT_BUF_SIZE;
}

RDIstrstream::~RDIstrstream()
{
  if (_buf) {
    delete [] _buf;
    _buf = 0;
  }
}

void
RDIstrstream::clear()
{
  _p = _buf;
  _width = _p;
  if (_p) {
    _p[0] = '\0';
  }
}

const char* RDIstrstream::buf() {
  return (const char*)_buf;
}

char* RDIstrstream::retn() {
  char* b = _buf;
  if (_buf) {
    delete [] _buf;
    _buf = 0;
  }
  return b;
}

void
RDIstrstream::more(int n)
{
  int wdiff = _width - _p;
  int used = _p - _buf + 1;
  int size = _end - _buf;

  while( size - used < n )  size *= 2;

  char* newbuf = new char[size];
  RDI_STRCPY(newbuf, _buf);
  char* newp = newbuf + (used - 1);
  delete [] _buf;
  _buf = newbuf;
  _p = newp;
  _width = _p;
  if (wdiff > 0) {
    _width += wdiff;
  } 
  _end = _buf + size;
}

void
RDIstrstream::width_fill() {
  int fillsz = _width - _p;
  if (fillsz > 0) {
    reserve(fillsz);
    while (_p < _width) {
      *_p++ = ' ';
    }
    *_p = '\0';
  }
}

RDIstrstream& 
RDIstrstream::operator<<(char c)
{
  if (c == 0) {
    reserve(4);
    RDI_STRCPY(_p, "\\000");
    _p += 4;
  } else if (c == 1) {
    reserve(4);
    RDI_STRCPY(_p, "\\001");
    _p += 4;
  } else { 
    reserve(1);
    *_p++ = c;
    *_p = '\0';
  }
  width_fill();
  return *this;
}

RDIstrstream&
RDIstrstream::operator<<(const char *s)
{
  size_t len = RDI_STRLEN(s);
  reserve(len);
  RDI_STRCPY(_p, s);
  _p += len;
  width_fill();
  return *this;
}

RDIstrstream&
RDIstrstream::operator<<(const void *p)
{
  reserve(30); // guess!
  sprintf(_p, "%p", p);
  _p += RDI_STRLEN(_p);
  width_fill();
  return *this;
}

RDIstrstream&
RDIstrstream::operator<<(int n)
{
  reserve(20);
  sprintf(_p, "%d", n);
  _p += RDI_STRLEN(_p);
  width_fill();
  return *this;
}

RDIstrstream&
RDIstrstream::operator<<(unsigned int n)
{
  reserve(20);
  sprintf(_p, "%u", n);
  _p += RDI_STRLEN(_p);
  width_fill();
  return *this;
}

RDIstrstream&
RDIstrstream::operator<<(long n)
{
  reserve(30);
  sprintf(_p, "%ld", n);
  _p += RDI_STRLEN(_p);
  width_fill();
  return *this;
}

RDIstrstream&
RDIstrstream::operator<<(unsigned long n)
{
  reserve(30);
  sprintf(_p, "%lu", n);
  _p += RDI_STRLEN(_p);
  width_fill();
  return *this;
}

#ifdef HAS_LongLong
#if SIZEOF_LONG_LONG != SIZEOF_LONG
RDIstrstream&
RDIstrstream::operator<<(WRAPPED_CORBA_LONGLONG_TYPE n)
{
  reserve(60);
  sprintf(_p, "%lld", n);
  _p += RDI_STRLEN(_p);
  width_fill();
  return *this;
}

RDIstrstream&
RDIstrstream::operator<<(WRAPPED_CORBA_ULONGLONG_TYPE n)
{
  reserve(60);
  sprintf(_p, "%llu", n);
  _p += RDI_STRLEN(_p);
  width_fill();
  return *this;
}
#endif
#endif

#ifndef NO_FLOAT
RDIstrstream&
RDIstrstream::operator<<(double n)
{
  reserve(30);
  sprintf(_p, "%g", n);
  _p += RDI_STRLEN(_p);
  width_fill();
  return *this;
}

#ifdef HAS_LongDouble
RDIstrstream&
RDIstrstream::operator<<(WRAPPED_CORBA_LONGDOUBLE_TYPE n)
{
  reserve(60);
  sprintf(_p, "%Lg", n);
  _p += RDI_STRLEN(_p);
  width_fill();
  return *this;
}
#endif

#endif

// XXX TBD
RDIstrstream&
RDIstrstream::operator<< (const CORBA::Exception &e) {
  return (*this) << "CORBA::Exception [better description TBD]";
}

RDIstrstream&
RDIstrstream::operator<< (const CORBA::TCKind& tckind) {
  switch (tckind) {
  case CORBA::tk_null:           return (*this) << "null";
  case CORBA::tk_void:           return (*this) << "void";
  case CORBA::tk_short:          return (*this) << "short";
  case CORBA::tk_long:           return (*this) << "long";
  case CORBA::tk_ushort:         return (*this) << "ushort";
  case CORBA::tk_ulong:          return (*this) << "ulong";
  case CORBA::tk_float:          return (*this) << "float";
  case CORBA::tk_double:         return (*this) << "double";
  case CORBA::tk_boolean:        return (*this) << "bool";
  case CORBA::tk_char:           return (*this) << "char";
  case CORBA::tk_octet:          return (*this) << "octet";
  case CORBA::tk_any:            return (*this) << "any";
  case CORBA::tk_TypeCode:       return (*this) << "TypeCode";
  case CORBA::tk_Principal:      return (*this) << "Principle";
  case CORBA::tk_objref:         return (*this) << "objref";
  case CORBA::tk_struct:         return (*this) << "struct";
  case CORBA::tk_union:          return (*this) << "union";
  case CORBA::tk_enum:           return (*this) << "enum";
  case CORBA::tk_string:         return (*this) << "string";
  case CORBA::tk_sequence:       return (*this) << "sequence";
  case CORBA::tk_array:          return (*this) << "array";
  case CORBA::tk_alias:          return (*this) << "alias";
  case CORBA::tk_except:         return (*this) << "except";
  case CORBA::tk_longlong:       return (*this) << "longlong";
  case CORBA::tk_ulonglong:      return (*this) << "ulonglong";
#ifdef HAS_LongDouble
  case CORBA::tk_longdouble:     return (*this) << "longdouble";
#endif
  }
  return (*this) << "(XXX unexpected CORBA::TCKind)" << (unsigned long)tckind;
}

// XXX we could expand the complex typecodes
RDIstrstream&
RDIstrstream::operator<< (const CORBA::TypeCode &tc) {
  return (*this) << tc.kind();
}

RDIstrstream&
RDIstrstream::operator<< (const CORBA::Any &a) {
  CORBA::TypeCode_var tmp_tcp = a.type();
  (*this) << "Any[type: " << *(tmp_tcp) << ", val: ";

  while (1) {
    if (CORBA::_tc_null->equivalent(tmp_tcp))      { break; }
    if (CORBA::_tc_void->equivalent(tmp_tcp))      { break; }
    if (CORBA::_tc_ushort->equivalent(tmp_tcp))    { CORBA::UShort c;    a >>= c; (*this) << c; break; }
    if (CORBA::_tc_short->equivalent(tmp_tcp))     { CORBA::Short c;     a >>= c; (*this) << c; break; }
    if (CORBA::_tc_ulong->equivalent(tmp_tcp))     { CORBA::ULong c;     a >>= c; (*this) << c; break; }
    if (CORBA::_tc_long->equivalent(tmp_tcp))      { CORBA::Long c;      a >>= c; (*this) << c; break; }
#ifndef TIMEBASE_NOLONGLONG
    if (CORBA::_tc_ulonglong->equivalent(tmp_tcp)) { CORBA::ULongLong c; a >>= c; (*this) << c; break; }
    if (CORBA::_tc_longlong->equivalent(tmp_tcp))  { CORBA::LongLong c;  a >>= c; (*this) << c; break; }
#endif
    if (CORBA::_tc_float->equivalent(tmp_tcp))     { CORBA::Float c;     a >>= c; (*this) << c; break; }
    if (CORBA::_tc_double->equivalent(tmp_tcp))    { CORBA::Double c;    a >>= c; (*this) << c; break; }
    if (CORBA::_tc_boolean->equivalent(tmp_tcp))
      { CORBA::Boolean c;   CORBA::Any::to_boolean to_c(c); a >>= to_c; (*this) << c; break; }
    if (CORBA::_tc_char->equivalent(tmp_tcp))
      { CORBA::Char c;      CORBA::Any::to_char to_c(c);    a >>= to_c; (*this) << c; break; }
    if (CORBA::_tc_octet->equivalent(tmp_tcp))
      { CORBA::Octet c;     CORBA::Any::to_octet to_c(c);   a >>= to_c; (*this) << c; break; }

    if (CORBA::_tc_string->equivalent(tmp_tcp))    {  const char* c = 0; a >>= c; (*this) << c; break; }
    if (CORBA::_tc_TypeCode->equivalent(tmp_tcp))  { CORBA::TypeCode_ptr c; a >>= c; (*this) << *c; break; }
    // other cases -- some would require constructing DynAny and extracting parts
    (*this) << "(TODO)";
    break;
  }
  (*this) << "]" << '\n';
  return *this;
}

