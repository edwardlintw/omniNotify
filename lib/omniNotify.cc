// -*- Mode: C++; -*-
//                              File      : omniNotify.cc
//                              Package   : omniNotify-Library
//                              Created on: 1-Jan-1998
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
//    Implementation of omniNotify [main library interface]
//

#include "omniNotify.h"
#include "RDI.h"

// Just map the calls onto the real RDI calls

AttNotification::Server_ptr omniNotify::init_server(int& argc, char** argv) {
  return RDI::init_server(argc, argv);
};

AttNotification::Server_ptr omniNotify::get_server() {
  return RDI::get_server();
};

void omniNotify::wait_for_destroy() {
  RDI::wait_for_destroy();
};
