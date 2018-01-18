// -*- Mode: C++; -*-
//                              File      : RDICatchMacros.h
//                              Package   : omniNotify-Library
//                              Created on: 19-Sep-2001
//                              Authors   : gruber&panagos
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
//    proprietary interface
//
 
#ifndef _RDI_CATCHMACROS_H_
#define _RDI_CATCHMACROS_H_

//-------------------------------------------------------------------------
// This macro sets a boolean b to 1

#define CATCH_INVOKE_PROBLEM(b) \
  catch ( CORBA::INV_OBJREF& e ) { b = 1; } \
  catch ( CORBA::OBJECT_NOT_EXIST& e ) { b = 1; } \
  catch ( CORBA::COMM_FAILURE& e ) { b = 1; }

#define CATCH_INVOKE_PROBLEM_NOOP \
  catch ( CORBA::INV_OBJREF& ) { } \
  catch ( CORBA::OBJECT_NOT_EXIST& ) { } \
  catch ( CORBA::COMM_FAILURE& ) { }

#endif  /*  _RDI_CATCHMACROS_H_  */
