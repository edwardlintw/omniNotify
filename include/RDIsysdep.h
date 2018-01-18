// -*- Mode: C++; -*-
//                              File      : RDIsysdep.h
//                              Package   : omniNotify-Library
//                              Created on: 2003/04/22
//                              Authors   : Duncan Grisby
//
//    Copyright (C) 2003 Apasphere Ltd.
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
// Description:
//    Macro definition to deal with importing symbols from DLLs on Windows.

#ifndef _RDIsysdep_h_
#define _RDIsysdep_h_

#if defined(_OMNINOTIFY_LIBRARY)
#  define _nfy_attr
#else
#  define _nfy_attr _OMNIORB_NTDLL_IMPORT
#endif

#endif // _RDIsysdep_h_
