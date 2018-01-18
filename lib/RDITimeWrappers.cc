// -*- Mode: C++; -*-
//                              File      : RDITimeWrappers.cc
//                              Package   : omniNotify-Library
//                              Created on: 1-March-2001
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
//
 
#include "thread_wrappers.h" 
#include "RDITimeWrappers.h"
#include "RDIStringDefs.h"
#include "RDI.h"

#ifdef __WIN32__
#  include <time.h>
#endif

void
RDI_UtcT::set_local_cosbase_secs_nanosecs(unsigned long s, unsigned long n)
{
  // (s,n) is local time from CosTime clock
  // use local info about time zone and clock inaccuracy
  time = 
    ((CORBA::ULongLong)s * RDI_1e7)
    + ((CORBA::ULongLong)n/100);
  RDI_ServerQoS* s_qos = RDI::get_server_qos();
  if (s_qos) {
    inacclo = s_qos->localClockInaccLo;
    inacchi = s_qos->localClockInaccHi;
    tdf     = s_qos->localClockTDF;
  } else {
    inacclo =   0;
    inacchi =   0;
    tdf     = 300;
  }
}

void
RDI_UtcT::set_local_posixbase_secs_nanosecs(unsigned long s, unsigned long n)
{
  // (s,n) is local time from posix clock;
  // use local info about time zone and clock inaccuracy
  time = 
    ((CORBA::ULongLong)s * RDI_1e7)
    + ((CORBA::ULongLong)n/100)
    + POSIX2COS_CONVERSION; 
  RDI_ServerQoS* s_qos = RDI::get_server_qos();
  if (s_qos) {
    inacclo = s_qos->localClockInaccLo;
    inacchi = s_qos->localClockInaccHi;
    tdf     = s_qos->localClockTDF;
  } else {
    inacclo =   0;
    inacchi =   0;
    tdf     = 300;
  }
}

RDIstrstream&
operator<<(RDIstrstream& str, const RDI_TimeT & tm)
{
  unsigned long s, n;
  tm.get_rel_secs_nanosecs(s, n);
  return str << "TimeT(s,n)=(" << s << "," << n << ")";
}

RDIstrstream&
operator<<(RDIstrstream& str, const RDI_UtcT & ut)
{
  unsigned long s, n;
  ut.get_gmt_cosbase_secs_nanosecs(s, n);
  return str << "UtcT[(s,n)=(" << s << "," << n << "), inacclo=" << ut.inacclo <<
    " inacchi=" << ut.inacchi << " tdf=" << ut.tdf << "]";
}

RDIstrstream&
RDI_Watch::log_output(RDIstrstream& str) const
{
  return str << "Stop watch: "<< millisecs() << " msecs"; 
}

RDIstrstream&
RDI_DeltaWatch::log_output(RDIstrstream& str) const
{
  str << "Interval stop watch: \n";
  if (_deltas < 1) { return str << "\tnot enough deltas\n"; }
  for (int interval = 1; interval <= _deltas; interval++) {
    str << "\tInterval " << interval << " : " <<
      millisecs(interval) << " millisecs\n";
  }
  return str << '\n';
}


////////////////////////////////////////
// Pretty-printing

// pretty-print a time value -- we need implementations for different operating systems!

// generic Unix -- need lock because ctime's buffer can change

static TW_Mutex RDI_out_time_lock;

#undef WHATFN
#define WHATFN "RDI_posixbase_out_time"
void
RDI_posixbase_out_time(RDIstrstream& str, CORBA::ULong secs, CORBA::ULong nanosecs)
{
  TW_SCOPE_LOCK(otime_lock, RDI_out_time_lock, "RDI_out_time", WHATFN);
  time_t secs_as_time_t = secs;
  const char* temp = (const char*)ctime(&secs_as_time_t);
  char* cpy = CORBA_STRING_DUP(temp);
  cpy[24] = ' '; // remove newline???
  str << cpy << " [+ " << nanosecs << " nanosecs]";
  CORBA_STRING_FREE(cpy); // already copied into str's buffer
}

///////////////////////////////////////
// RDI_TimeT

static char RDI_TimeT_fmt_local_buf[128*10];
static int RDI_TimeT_fmt_local_buf_idx = 0;

#undef WHATFN
#define WHATFN "RDI_TimeT::fmt_local"
const char *RDI_TimeT::fmt_local() {
  unsigned long ts, tm;
  get_posixbase_secs_msecs(ts, tm);
  time_t secs_as_time_t = ts;
  TW_SCOPE_LOCK(otime_lock, RDI_out_time_lock, "RDI_out_time", WHATFN);
  RDI_TimeT_fmt_local_buf_idx = (RDI_TimeT_fmt_local_buf_idx + 1) % 10;
  char* res = (char*)RDI_TimeT_fmt_local_buf + (128 * RDI_TimeT_fmt_local_buf_idx);
  sprintf(res, "%s%03lu (local time)",
	  (const char*)ctime(&secs_as_time_t),
	  (unsigned long)tm);
  res[24] = '.'; // replace newline with '.'
  return res;
}
