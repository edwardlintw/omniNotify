// -*- Mode: C++; -*-
//                              File      : RDITimeWrappers.h
//                              Package   : omniNotify-Library
//                              Created on: 1-Jan-1998
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
//    wrapper classes for TimeBase::UtcT and TimeBase::TimeT
//
 
#ifndef _RDI_TIME_WRAPPERS_H_
#define _RDI_TIME_WRAPPERS_H_

#include "thread_wrappers.h"
#include "corba_wrappers.h"
#include "RDILimits.h"
#include "timebase.h"
#include "RDIstrstream.h"

#define RDI_1e4  WRAPPED_CORBA_LONGLONG_CONST(10000)
#define RDI_60e7 WRAPPED_CORBA_LONGLONG_CONST(600000000)
#define RDI_1e7  WRAPPED_CORBA_LONGLONG_CONST(10000000)

#define POSIX2COS_CONVERSION WRAPPED_CORBA_LONGLONG_CONST(122192928000000000)
// Used to convert from posix time value to COS time service time value.
// Difference in 100s of nanoseconds between 15th October 1582 and 1st Jan 1970.

#define RDI_ULLGetHi(t) (t >> 32)
#define RDI_ULLGetLo(t) ((t << 32) >> 32)

// change offset in mins to offset in 100's of nanosecs
#define RDI_TDF2ULL(tdf) ((CORBA::ULongLong)tdf * RDI_60e7)
// change from TimeBase representations to unsigned long long rep 
#ifdef TIMEBASE_NOLONGLONG
#  define RDI_TBTimeT2ULL(t) ((CORBA::ULongLong)t.low + ((CORBA::ULongLong)t.high << 32))
#  define RDI_TBUtcT2ULL(u) (RDI_TBTimeT2ULL(u.time) + RDI_TDF2ULL(u.tdf))
#else
#  define RDI_TBTimeT2ULL(t) (t)
#  define RDI_TBUtcT2ULL(u) (u.time + RDI_TDF2ULL(u.tdf))
#endif

////////////////////////////////////////////////////////////////////////////////
// TimeBase comparison
////////////////////////////////////////////////////////////////////////////////

inline CORBA::Boolean RDI_TBTimeT_LT(const TimeBase::TimeT& t1, const TimeBase::TimeT& t2)
{
  return ( RDI_TBTimeT2ULL(t1) < RDI_TBTimeT2ULL(t2) ) ? 1 : 0;
}

inline CORBA::Boolean RDI_TBTimeT_GT(const TimeBase::TimeT& t1, const TimeBase::TimeT& t2)
{
  return ( RDI_TBTimeT2ULL(t1) > RDI_TBTimeT2ULL(t2) ) ? 1 : 0;
}

inline CORBA::Boolean RDI_TBTimeT_EQ(const TimeBase::TimeT& t1, const TimeBase::TimeT& t2)
{
  return ( RDI_TBTimeT2ULL(t1) == RDI_TBTimeT2ULL(t2) ) ? 1 : 0;
}

inline CORBA::Boolean RDI_TBUtcT_LT(const TimeBase::UtcT& t1, const TimeBase::UtcT& t2)
{
  return ( RDI_TBUtcT2ULL(t1) < RDI_TBUtcT2ULL(t2) ) ? 1 : 0;
}

inline CORBA::Boolean RDI_TBUtcT_GT(const TimeBase::UtcT& t1, const TimeBase::UtcT& t2)
{
  return ( RDI_TBUtcT2ULL(t1) > RDI_TBUtcT2ULL(t2) ) ? 1 : 0;
}

inline CORBA::Boolean RDI_TBUtcT_EQ(const TimeBase::UtcT& t1, const TimeBase::UtcT& t2)
{
  return ( RDI_TBUtcT2ULL(t1) == RDI_TBUtcT2ULL(t2) ) ? 1 : 0;
}

////////////////////////////////////////////////////////////////////////////////
// TimeT macrss
////////////////////////////////////////////////////////////////////////////////

// is time t1 < time t2 by more than secs seconds?
#define RDI_TIMET_LT_BY_SECS(t1, t2, secs) (((t1).time + ((secs) * RDI_1e7)) < (t2).time)

// return difference between t1 and t2 as a double, units == seconds
// where t1 is assumed to be < t2
#define RDI_TIMET_DIFF_IN_SECS(t1, t2) ( (CORBA::Long)((t2).time - (t1).time) / (double)RDI_1e7 )

////////////////////////////////////////////////////////////////////////////////
// wrappers for TimeBase classes
////////////////////////////////////////////////////////////////////////////////
//
// Both RDI_TimeT and RDI_UtcT keep a ulonglong time value that
// use the CosTime specified base time and units
// (100s of nanoseconds since XXX).
//
// Both support reading/writing 'cosbase' and 'posixbase' values.

// pretty-print a time value that has base posixbase -- no newline
void RDI_posixbase_out_time(RDIstrstream& str, CORBA::ULong secs, CORBA::ULong nanosecs);

// A TimeT can be a relative time, or it can be an absolute local time
// (time zone information is not recorded/accounted for).
class RDI_TimeT {
public:
  CORBA::ULongLong time;
  // constructors
  RDI_TimeT() {time = 0;}
  RDI_TimeT(const RDI_TimeT& v) { time = v.time; }
  RDI_TimeT(const TimeBase::TimeT& v) {    // wrap v
    time = RDI_TBTimeT2ULL(v);
  }
  // -----------------------------------------------------------------
  // Relative time methods (no need for cosbase/posixbase distinction)
  // -----------------------------------------------------------------
  // methods to set self to a specified time
  void set_rel_msecs(CORBA::ULongLong msecs) {
    // time val in milliseconds -- convert to 100's of nanosecs
    time = (msecs * RDI_1e4);
  }
  void set_rel_secs(CORBA::ULongLong secs) {
    // time val in seconds -- convert to 100's of nanosecs
    time = (secs * RDI_1e7);
  }
  void set_rel_mins(CORBA::ULongLong mins) {
    // time val in minutes -- convert to 100's of nanosecs
    time = (mins*60) * RDI_1e7;
  }
  void get_rel_secs_nanosecs(unsigned long& t_s, unsigned long& t_n) const {
    CORBA::ULongLong ts = time / RDI_1e7;
    CORBA::ULongLong tn = (time % RDI_1e7) * 100;
    t_s = (unsigned long)ts;
    t_n = (unsigned long)tn;
  }
  // --------------------------------------------------------------
  // Absolute time methods (need the cosbase/posixbase distinction)
  // --------------------------------------------------------------
  void set_cosbase_secs_nanosecs(unsigned long t_s, unsigned long t_n) {
    // set time based on posix clock value specified in secs/nanosecs
    time =
      ((CORBA::ULongLong)t_s * RDI_1e7)
      + ((CORBA::ULongLong)t_n / 100);
  }
  void set_posixbase_secs_nanosecs(unsigned long t_s, unsigned long t_n) {
    // set time based on posix clock value specified in secs/nanosecs
    time =
      ((CORBA::ULongLong)t_s * RDI_1e7)
      + ((CORBA::ULongLong)t_n / 100)
      + POSIX2COS_CONVERSION;
  }
  // use when this is a relative time
  void get_cosbase_secs_nanosecs(unsigned long& t_s, unsigned long& t_n) const {
    CORBA::ULongLong ts = time / RDI_1e7;
    CORBA::ULongLong tn = (time % RDI_1e7) * 100;
    t_s = (unsigned long)ts;
    t_n = (unsigned long)tn;
  }
  // use when this is an absolute time, convert to posix clock value
  void get_posixbase_secs_nanosecs(unsigned long& t_s, unsigned long& t_n) const {
    CORBA::ULongLong tprime = time - POSIX2COS_CONVERSION;
    CORBA::ULongLong ts = tprime / RDI_1e7;
    CORBA::ULongLong tn = (tprime % RDI_1e7) * 100;
    t_s = (unsigned long)ts;
    t_n = (unsigned long)tn;
  }
  // use when this is an absolute time, convert to posix clock value
  void get_posixbase_secs_msecs(unsigned long& t_s, unsigned long& t_m) const {
    CORBA::ULongLong tprime = time - POSIX2COS_CONVERSION;
    CORBA::ULongLong ts = tprime / RDI_1e7;
    CORBA::ULongLong tm = (tprime % RDI_1e7) / 10000;
    t_s = (unsigned long)ts;
    t_m = (unsigned long)tm;
  }
  // ----------------------------------------------------------
  // Other methods
  // ----------------------------------------------------------
  void set_curtime() {
    // set this to current time
    unsigned long s, n;
    TW_GET_TIME(&s, &n); // gets posix time
    set_posixbase_secs_nanosecs(s, n);
  }
  // TimeBase::TimeT obviously uses cosbase
  void get_TimeBaseTimeT(TimeBase::TimeT& v) const {
#ifdef TIMEBASE_NOLONGLONG
    v.low  = low();
    v.high = high();
#else
    v = time;
#endif
  }
  // copying / conversion methods 
  RDI_TimeT& operator=(const RDI_TimeT& v) { time = v.time; return *this; } 
  RDI_TimeT& operator=(const TimeBase::TimeT& v) {    // wrap v
    time = RDI_TBTimeT2ULL(v);
    return *this;
  }
  // extract lo/hi words of the ulonglong time
  CORBA::ULong low() const {
    CORBA::ULongLong res = RDI_ULLGetLo(time);
    return (CORBA::ULong)res;
  }
  CORBA::ULong high() const {
    CORBA::ULongLong res = RDI_ULLGetHi(time);
    return (CORBA::ULong)res;
  }
  // arithmetic
  void set_delta(const RDI_TimeT& lt, const RDI_TimeT& rt)
  {
    if (lt.time >= rt.time) {
      time = lt.time - rt.time;
    } else {
      time = rt.time - lt.time;
    }
  }

  // pretty-printing of absolute local time -- no newline
  void out_local(RDIstrstream& str) {
    unsigned long ts, tn;
    get_posixbase_secs_nanosecs(ts, tn);
    RDI_posixbase_out_time(str, ts, tn);
    str << " (local time)";
  }

  const char *fmt_local();

  // static helpers for producing TimeBase::TimeT min and max
  static TimeBase::TimeT MinTimeBaseTimeT() {
    TimeBase::TimeT res;
#ifdef TIMEBASE_NOLONGLONG
    res.low = res.high = 0;
#else
    res = 0;
#endif
    return res;
  }
  static TimeBase::TimeT MaxTimeBaseTimeT() {
    TimeBase::TimeT res;
#ifdef TIMEBASE_NOLONGLONG
    res.low = res.high = RDI_ULONG_MAX;
#else
    res = RDI_ULONGLONG_MAX;
#endif
    return res;
  }
};

class RDI_UtcT {
public:
  CORBA::ULongLong time;
  CORBA::ULong     inacclo;
  CORBA::UShort    inacchi;
  CORBA::Short     tdf; // minutes west of greenwich meridian
  // constructors
  RDI_UtcT() : time(0), inacclo(0), inacchi(0), tdf(0) {;}
  RDI_UtcT(const CORBA::ULongLong greenwich_time)
    : time(greenwich_time), inacclo(0), inacchi(0), tdf(0) {;}
  RDI_UtcT(const RDI_UtcT& v) { time = v.time; inacclo = v.inacclo; inacchi = v.inacchi; tdf = v.tdf; }
  RDI_UtcT(const TimeBase::UtcT& v) {    // wrap v
    time = RDI_TBTimeT2ULL(v.time);
    inacclo = v.inacclo;
    inacchi = v.inacchi;
    tdf = v.tdf;
  }
  // ----------------------------------------------------------
  // Absolute time methods (need the utct/posix distinction; + 
  //  need local/gmt distinction since we have time zone info)
  // ----------------------------------------------------------
  void get_local_cosbase_secs_nanosecs(unsigned long& t_s, unsigned long& t_n) const {
    CORBA::ULongLong ts = time / RDI_1e7;
    CORBA::ULongLong tn = (time % RDI_1e7) * 100;
    t_s = (unsigned long)ts;
    t_n = (unsigned long)tn;
  }
  void get_gmt_cosbase_secs_nanosecs(unsigned long& t_s, unsigned long& t_n) const {
    CORBA::ULongLong gtime = greenwich_time();
    CORBA::ULongLong ts = gtime / RDI_1e7;
    CORBA::ULongLong tn = (gtime % RDI_1e7) * 100;
    t_s = (unsigned long)ts;
    t_n = (unsigned long)tn;
  }
  void get_local_posixbase_secs_nanosecs(unsigned long& t_s, unsigned long& t_n) const {
    CORBA::ULongLong tprime = time - POSIX2COS_CONVERSION;
    CORBA::ULongLong ts = tprime / RDI_1e7;
    CORBA::ULongLong tn = (tprime % RDI_1e7) * 100;
    t_s = (unsigned long)ts;
    t_n = (unsigned long)tn;
  }
  // get posix secs/nanosecs; *do* consider tdf
  void get_gmt_posixbase_secs_nanosecs(unsigned long& t_s, unsigned long& t_n) const {
    CORBA::ULongLong tprime = greenwich_time() - POSIX2COS_CONVERSION;
    CORBA::ULongLong ts = tprime / RDI_1e7;
    CORBA::ULongLong tn = (tprime % RDI_1e7) * 100;
    t_s = (unsigned long)ts;
    t_n = (unsigned long)tn;
  }
  void set_local_posixbase_secs_nanosecs(unsigned long s, unsigned long n);
    // (s,n) is local time from posix clock;
    // use local info about time zone and clock inaccuracy
  void set_gmt_posixbase_secs_nanosecs(unsigned long s, unsigned long n) {
    // (s,n) is gmt time from a posix clock;
    // nothing is known about clock inaccuracy
    time = 
      ((CORBA::ULongLong)s * RDI_1e7)
      + ((CORBA::ULongLong)n/100)
      + POSIX2COS_CONVERSION; 
    inacclo = 0;
    inacchi = 0;
    tdf     = 0;
  }
  void set_local_cosbase_secs_nanosecs(unsigned long s, unsigned long n);
    // (s,n) is local time from CosTime clock
    // use local info about time zone and clock inaccuracy
  void set_gmt_cosbase_secs_nanosecs(unsigned long s, unsigned long n) {
    // (s,n) is gmt time from a CosTime clock;
    // nothing is known about clock inaccuracy
    time = 
      ((CORBA::ULongLong)s * RDI_1e7)
      + ((CORBA::ULongLong)n/100);
    inacclo = 0;
    inacchi = 0;
    tdf     = 0;
  }
  // ----------------------------------------------------------
  // Other methods
  // ----------------------------------------------------------
  void set_curtime() {
    // set this to current time
    unsigned long s, n;
    TW_GET_TIME(&s, &n);
    set_local_posixbase_secs_nanosecs(s, n);
  }
  // extract lo/hi words of the ulonglong time
  CORBA::ULong low() const {
    CORBA::ULongLong res = RDI_ULLGetLo(time);
    return (CORBA::ULong)res;
  }
  CORBA::ULong high() const {
    CORBA::ULongLong res = RDI_ULLGetHi(time);
    return (CORBA::ULong)res;
  }
  CORBA::ULongLong greenwich_time() const {
    return time + RDI_TDF2ULL(tdf);
  }
  void get_TimeBaseUtcT(TimeBase::UtcT& v) const {
#ifdef TIMEBASE_NOLONGLONG
    v.time.low     = low();
    v.time.high    = high();
#else
    v.time         = time;
#endif
    v.inacclo      = inacclo;
    v.inacchi      = inacchi;
    v.tdf          = tdf;
  }
  // copying / conversion methods 
  RDI_UtcT& operator=(const RDI_UtcT& v) {
    time = v.time; inacclo = v.inacclo; inacchi = v.inacchi; tdf = v.tdf;
    return *this;
  }
  RDI_UtcT& operator=(const TimeBase::UtcT& v) {    // wrap v
    time = RDI_TBTimeT2ULL(v.time);
    inacclo = v.inacclo;
    inacchi = v.inacchi;
    tdf = v.tdf;
    return *this;
  }
  // arithmetic
  void add(const RDI_TimeT& rel_time) {
    // add rel_time to this
    time += rel_time.time;
  }
  void sub(const RDI_TimeT& rel_time) {
    // sub rel_time from this
    time -= rel_time.time;
  }
  // pretty-printing of absolute univ time -- no newline
  void out_gmt(RDIstrstream& str) {
    unsigned long ts, tn;
    get_gmt_posixbase_secs_nanosecs(ts, tn);
    RDI_posixbase_out_time(str, ts, tn);
    str << " (greenwich mean time)";
  }
  // static helpers for producing TimeBase::UtcT min and max
  static TimeBase::UtcT MinTimeBaseUtcT() {
    TimeBase::UtcT res;
#ifdef TIMEBASE_NOLONGLONG
    res.time.low = res.time.high = 0;
#else
    res.time = 0;
#endif
    res.inacclo = res.inacchi = res.tdf = 0;
    return res;
  }
  static TimeBase::UtcT MaxTimeBaseUtcT() {
    TimeBase::UtcT res;
#ifdef TIMEBASE_NOLONGLONG
    res.time.low = res.time.high = RDI_ULONG_MAX;
#else
    res.time = RDI_ULONGLONG_MAX;
#endif
    res.inacclo = res.inacchi = res.tdf = 0;
    return res;
  }
};

/** STOP AND DELTA WATCH
  *
  * The following classes offer simple and delta timers.
  */

#define RDI_NS2US(x) ((x) / 1000)
#define RDI_NS2MS(x) ((x) / 1000000)

#define RDI_SC2MS(x) ((x) * 1000)
#define RDI_SC2US(x) ((x) * 1000000)
#define RDI_SC2NS(x) ((x) * 1000000000)

#define RDI_TIMEDIFF_MSECS(start_tval_s,start_tval_n,end_tval_s,end_tval_n)\
  ( (end_tval_n > start_tval_n) ? \
    (RDI_SC2MS(end_tval_s - start_tval_s) + \
     RDI_NS2MS(end_tval_n - start_tval_n)) : \
    (RDI_SC2MS(end_tval_s - start_tval_s - 1) + \
     RDI_NS2MS((RDI_SC2NS(1) + end_tval_n) - start_tval_n)))

#define RDI_TIMEDIFF_USECS(start_tval_s,start_tval_n,end_tval_s,end_tval_n)\
  ( (end_tval_n > start_tval_n) ? \
    (RDI_SC2US(end_tval_s - start_tval_s) + \
     RDI_NS2US(end_tval_n - start_tval_n)) : \
    (RDI_SC2US(end_tval_s - start_tval_s - 1) + \
     RDI_NS2US((RDI_SC2NS(1) + end_tval_n) - start_tval_n)))

class RDI_Watch {
public:
  RDI_Watch(void)        {;}

  void start(void)      { TW_GET_TIME(&_start_s, &_start_n); }
  void stop(void)       { TW_GET_TIME(&_end_s, &_end_n); }

  unsigned int millisecs() const 
                { return RDI_TIMEDIFF_MSECS(_start_s,_start_n,_end_s,_end_n); }
  unsigned int microsecs() const 
                { return RDI_TIMEDIFF_USECS(_start_s,_start_n,_end_s,_end_n); }

  RDIstrstream& log_output(RDIstrstream& str) const;
private:
  unsigned long _start_s;
  unsigned long _start_n;
  unsigned long _end_s;
  unsigned long _end_n;
};

#define RDI_MAX_WATCH_DELTAS 128

class RDI_DeltaWatch {
public:
  RDI_DeltaWatch(void) : _deltas(-1) {;}

  void delta(void) {
    if ( (_deltas+1) < RDI_MAX_WATCH_DELTAS ) {
      _deltas++;
      TW_GET_TIME(&(_tms_s[_deltas]), &(_tms_n[_deltas]));
    }
  }
  int  deltas(void)     { return _deltas; }

  unsigned long millisecs(int interval) const {
    return ((interval < 1) || (_deltas < interval)) ? 0 :
      RDI_TIMEDIFF_MSECS(_tms_s[interval-1], _tms_n[interval-1],
                         _tms_s[interval], _tms_n[interval]);
  }
  unsigned long microsecs(int interval) const {
    return ((interval < 1) || (_deltas < interval)) ? 0 :
      RDI_TIMEDIFF_USECS(_tms_s[interval-1], _tms_n[interval-1],
                         _tms_s[interval], _tms_n[interval]);
  }

  RDIstrstream& log_output(RDIstrstream& str) const;
private:
  int _deltas;
  unsigned long _tms_s[RDI_MAX_WATCH_DELTAS];
  unsigned long _tms_n[RDI_MAX_WATCH_DELTAS];
};

////////////////////////////////////////
// Logging

RDIstrstream& operator<< (RDIstrstream& str, const RDI_TimeT & tm);
RDIstrstream& operator<< (RDIstrstream& str, const RDI_UtcT & ut);

inline RDIstrstream& operator<< (RDIstrstream& str, const RDI_Watch& w) { return w.log_output(str); }
inline RDIstrstream& operator<< (RDIstrstream& str, const RDI_DeltaWatch& w) { return w.log_output(str); }

#endif  //_RDI_TIME_WRAPPERS_H_
