// -*- Mode: C++; -*-
//                              File      : RDIThreadPriority.h
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
//    proprietary interface
//
 
#ifndef _RDI_THREAD_PRIORITY_H_
#define _RDI_THREAD_PRIORITY_H_

#include "thread_wrappers.h"

// For the moment, only NORMAL is used
#define RDI_NOTIFY_WORKER_PRIORITY  TW_PRIORITY_NORMAL
#define RDI_PULL_WORKER_PRIORITY    TW_PRIORITY_NORMAL
#define RDI_CHANNEL_WORKER_PRIORITY TW_PRIORITY_NORMAL
#define RDI_EVENT_QUEUE_PRIORITY    TW_PRIORITY_NORMAL

#endif  /* _RDI_THREAD_PRIORITY_H_ */

