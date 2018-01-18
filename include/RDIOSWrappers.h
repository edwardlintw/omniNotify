// -*- Mode: C++; -*-
//                              File      : RDIOSWrappers.h
//                              Package   : omniNotify-Library
//                              Created on: 27-June-2001
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
//    wrap OS functionality
//    that might differ for different operating systems
//
 
#ifndef __RDI_OS_WRAPERS_H__
#define __RDI_OS_WRAPERS_H__

////////////////////////////////////////////////////////////////////////////////////
// Macros
// ------
// RDI_OS_MAX_OPEN_FILE_DESCRIPTORS
//  => set resource limit on max open file descriptors to the max possible value
//
////////////////////////////////////////////////////////////////////////////////////

#ifdef __WIN32__

#include <stdio.h>

#define RDI_OS_MAX_OPEN_FILE_DESCRIPTORS do { \
  _setmaxstdio(2048); \
} while (0)

#else

#include <sys/resource.h>

// Increase the limit of open file descriptor to the maximum
#define RDI_OS_MAX_OPEN_FILE_DESCRIPTORS do { \
  struct rlimit rlp; \
  if ( getrlimit(RLIMIT_NOFILE, &rlp) == 0 ) { \
    rlp.rlim_cur = rlp.rlim_max; \
    (void) setrlimit(RLIMIT_NOFILE, &rlp); \
  } \
} while (0)

#endif

#endif  /* __RDI_OS_WRAPPERS_H__ */



