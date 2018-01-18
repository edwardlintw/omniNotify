// -*- Mode: C++; -*-
//                              File      : RDILimits.h
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
//    upper/lower bounds on common CORBA numeric types
//
 
#ifndef __RDI_LIMITS_H__
#define __RDI_LIMITS_H__

#include "corba_wrappers.h"

#define RDI_SHORT_MAX    ((CORBA::Short) 32767)
#define RDI_SHORT_MIN    ((CORBA::Short)-32768)
#define RDI_USHORT_MAX   ((CORBA::UShort)65535)

#define RDI_LONG_MAX    ((CORBA::Long) 2147483647L)
#define RDI_LONG_MIN    ((CORBA::Long)-2147483648L)
#define RDI_ULONG_MAX   ((CORBA::ULong)4294967295UL)

#define RDI_LONGLONG_MAX    (WRAPPED_CORBA_LONGLONG_CONST(  9223372036854775807))
#define RDI_LONGLONG_MIN    (WRAPPED_CORBA_LONGLONG_CONST( -9223372036854775808))
#define RDI_ULONGLONG_MAX   (WRAPPED_CORBA_ULONGLONG_CONST(18446744073709551615))

#endif  /*  __RDI_LIMITS_H__  */
