// -*- Mode: C++; -*-
//                              File      : ATTN_sysdep.h
//                              Package   : omniNotify-Library
//                              Created on: 1-Aug-2001
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
//      This file is included by ATTN stub headers.
// 	Define a set of flags in C++ macros to facilitate the use of
//      the ATTN stub library.

#if defined(_ATTN_LIBRARY)

#  undef USE_core_stub_in_nt_dll
#  undef USE_dyn_stub_in_nt_dll

#else

#  ifndef USE_core_stub_in_nt_dll
#    define USE_core_stub_in_nt_dll
#  endif
#  ifndef USE_dyn_stub_in_nt_dll
#    define USE_dyn_stub_in_nt_dll
#  endif

#endif
