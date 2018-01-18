// -*- Mode: C++; -*-
//                              File      : RDILocksHeld.h
//                              Package   : omniNotify-Library
//                              Created on: 25-Sep-2003
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
//    RDI_LocksHeld struct is passed as first argument to routines
//    that may not need to acquire locks that are already held.
//    One must be careful to only use an element X in this struct
//    only in a context where exactly one X could possibly be involved.
 
#ifndef __RDI_LOCKSHELD_H__
#define __RDI_LOCKSHELD_H__

typedef struct RDI_LocksHeld_s {
  int     server;        // is server lock held?
  int     chanfact;      // is channel factory lock held?
  int     filtfact;      // is filter factory lock held?
  int     channel;       // is channel lock held?
  int     chan_stats;    // is channel stats lock held?
  int     typemap;       // is channel's type map lock held?
  int     cadmin;        // is consumer admin lock held?
  int     sadmin;        // is supplier admin lock held?
  int     cproxy;        // is proxy consumer lock held?
  int     sproxy;        // is proxy supplier lock held?
  int     filter;        // is filter lock held?
  int     map_filter;    // is map filter lock held?
} RDI_LocksHeld;

#endif /*  __RDI_LOCKSHELD_H__  */
