// -*- Mode: C++; -*-
//                              File      : RDIInteractive.h
//                              Package   : omniNotify-Library
//                              Created on: 10-Sep-2001
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
//    Class to help implement AttN::Interactive
 
#ifndef __RDI_INTERACTIVE_H__
#define __RDI_INTERACTIVE_H__

#include "CosNotifyShorthands.h"
#include "RDIstrstream.h"

class RDIInteractive {
 public:
  static void cleanup_channels(RDIstrstream& str, AttN::Interactive_ptr chanfact,
			       CORBA::Boolean admins, CORBA::Boolean proxies);
  static void cleanup_channel(RDIstrstream& str, AttN::Interactive_ptr chan,
			      CORBA::Boolean admins, CORBA::Boolean proxies);
  static CORBA::Boolean cleanup_admin(RDIstrstream& str, AttN::Interactive_ptr admin,
				      CORBA::Boolean admins, CORBA::Boolean proxies);
};

#endif /*  __RDI_INTERACTIVE_H__  */

