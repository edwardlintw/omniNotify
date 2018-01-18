// -*- Mode: C++; -*-
//                              File      : thread_wrappers_impl.h
//                              Package   : omniNotify-Library
//                              Created on: 26-Aug-2003
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
//    Implement thread debugging.
//
 
// This should be included by exactly one .cc file, e.g.,
// by the .cc file containing main().
//
// It is also included in the CosNotify library, thus if you link with
// that library you do not need to include it in one of your .cc files.

#ifndef __THREAD_WRAPPERS_IMPL_H__
#define __THREAD_WRAPPERS_IMPL_H__

#include <stdio.h>
#include <stdlib.h>
#include "thread_wrappers.h"

#ifdef TW_DEBUG

// --------------------------------------------------------------------------------

FILE*             TW::_dbg_log = stderr;

FILE* TW::DbgLog(const char* pathnm)
{
  return DbgLog(pathnm, "w");
}

FILE* TW::DbgLog(const char* pathnm, const char *mode)
{
  if (::strcmp(pathnm, "stdout") == 0) {
    _dbg_log  = stdout;
  } else if (::strcmp(pathnm, "stderr") == 0) {
    _dbg_log  = stderr;
  } else {
    if (!mode) mode = "w";
    if ( ! (_dbg_log = fopen(pathnm, mode)) ) {
      fprintf(stderr,
	      "\nTW::DbgLog: file open failed for %s in mode %s\n"
	      "              thread debug logging reverts to stderr\n",
	      pathnm, mode);
      _dbg_log   = stderr;
    }
  }
  return _dbg_log;
}

// --------------------------------------------------------------------------------

#define TW_INIT_THREAD_DBG_BUF_SIZE  256
TW_strstream::TW_strstream()
  : _buf(new char[TW_INIT_THREAD_DBG_BUF_SIZE])
{
  _p = _buf;
  _width = _p;
  _p[0] = '\0';
  _end = _buf + TW_INIT_THREAD_DBG_BUF_SIZE;
}

TW_strstream::~TW_strstream()
{
  if (_buf) {
    delete [] _buf;
    _buf = 0;
  }
}

void
TW_strstream::clear()
{
  _p = _buf;
  _width = _p;
  if (_p) {
    _p[0] = '\0';
  }
}

char*
TW_strstream::retn() {
  char* b = _buf;
  if (_buf) {
    delete [] _buf;
    _buf = 0;
  }
  return b;
}

void
TW_strstream::more(int n)
{
  int wdiff = _width - _p;
  int used = _p - _buf + 1;
  int size = _end - _buf;

  while( size - used < n )  size *= 2;

  char* newbuf = new char[size];
  ::strcpy(newbuf, _buf);
  char* newp = newbuf + (used - 1);
  delete [] _buf;
  _buf = newbuf;
  _p = newp;
  _width = _p;
  if (wdiff > 0) {
    _width += wdiff;
  } 
  _end = _buf + size;
}

void
TW_strstream::width_fill() {
  int fillsz = _width - _p;
  if (fillsz > 0) {
    reserve(fillsz);
    while (_p < _width) {
      *_p++ = ' ';
    }
    *_p = '\0';
  }
}

TW_strstream& 
TW_strstream::operator<<(char c)
{
  if (c == 0) {
    reserve(4);
    ::strcpy(_p, "\\000");
    _p += 4;
  } else if (c == 1) {
    reserve(4);
    ::strcpy(_p, "\\001");
    _p += 4;
  } else { 
    reserve(1);
    *_p++ = c;
    *_p = '\0';
  }
  width_fill();
  return *this;
}

TW_strstream&
TW_strstream::operator<<(const char *s)
{
  size_t len = ::strlen(s);
  reserve(len);
  ::strcpy(_p, s);
  _p += len;
  width_fill();
  return *this;
}

TW_strstream&
TW_strstream::operator<<(const void *p)
{
  reserve(30); // guess!
  sprintf(_p, "%p", p);
  _p += ::strlen(_p);
  width_fill();
  return *this;
}

TW_strstream&
TW_strstream::operator<<(int n)
{
  reserve(20);
  sprintf(_p, "%d", n);
  _p += ::strlen(_p);
  width_fill();
  return *this;
}

TW_strstream&
TW_strstream::operator<<(unsigned int n)
{
  reserve(20);
  sprintf(_p, "%u", n);
  _p += ::strlen(_p);
  width_fill();
  return *this;
}

TW_strstream&
TW_strstream::operator<<(long n)
{
  reserve(30);
  sprintf(_p, "%ld", n);
  _p += ::strlen(_p);
  width_fill();
  return *this;
}

TW_strstream&
TW_strstream::operator<<(unsigned long n)
{
  reserve(30);
  sprintf(_p, "%lu", n);
  _p += ::strlen(_p);
  width_fill();
  return *this;
}

// --------------------------------------------------------------------------------

#ifdef TW_LONG_DBG_FILENAMES
#define __TW_FNAME_SHORTEN(file) file
#else
// shorten /foo/bar/zot/baz.cc  to  zot/baz.cc  (leave one level of subdir context)
static const char* __TW_FNAME_SHORTEN(const char* fname) {
  char* endptr = (char*)fname + ::strlen(fname) - 1;
  while ((endptr > fname) && (*(--endptr) != '/')) {;} // finds last /, if any
  while ((endptr > fname) && (*(--endptr) != '/')) {;} // finds next-to-last /, if any
  if (endptr > fname) return (const char*)(endptr+1);
  return fname;
}
#endif

TW_logger::TW_logger(const char* prefix, FILE* file, FILE* alt_file,
		     const char* flags, const char* srcfile, int srcline)
  : str(), _prefix_buf(0), _file(file), _alt_file(alt_file)
{
  // only use alt_file if it is a real file (not stdout or stderr) and is not same as file
  if ((file == alt_file) || (alt_file == stdout) || (alt_file == stderr)) {
    _alt_file = 0;
  }
  const char* preprefix = "";
  if ((file == stdout) || (file == stderr)) {
    preprefix = "\n";
  }
  if (prefix == (const char*)0) {
    prefix = "";
  }
  const char* flb = " [";
  const char* frb = "]";
  if ((flags == (const char*)0) || (::strlen(flags) == 0)) {
    flb = flags = frb = "";
  }
  if (srcfile == (const char*)0) {
    _prefix_buf = new char[::strlen(preprefix) + ::strlen(prefix) + ::strlen(flb)
			   + ::strlen(flags) + ::strlen(frb) + 2];
    sprintf(_prefix_buf, "%s%s%s%s%s: ", preprefix, prefix, flb, flags, frb);
  } else {
    char srcln[10];
    if (srcline == -1) {
      ::strcpy(srcln, "?LINE?");
    } else {
      sprintf(srcln, "%d", srcline);
    }
    srcfile = __TW_FNAME_SHORTEN(srcfile);
    _prefix_buf = new char[::strlen(preprefix) + ::strlen(prefix) + ::strlen(flb)
			   + ::strlen(flags) + ::strlen(frb)  + ::strlen(srcfile)
			   + ::strlen(srcln) + 5];
    sprintf(_prefix_buf, "%s%s%s%s%s[%s:%s]: ", preprefix, prefix, flb, flags,
	    frb, srcfile, srcln);
  }
}

TW_logger::~TW_logger()
{
  flush(0);
  if (_prefix_buf) {
    delete [] _prefix_buf;
    _prefix_buf = 0;
  }
}

void
TW_logger::write2FILE(FILE* outf, int do_fflush)
{
  if (outf) {
    if (str.len()) {
      fprintf(outf, "%s%s", _prefix_buf, str.buf());
    }
    if (do_fflush) {
      fflush(outf);
    }
  }
}

void
TW_logger::write2FILE_wo_prefix(FILE* outf, int do_fflush)
{
  if (outf) {
    if (str.len()) {
      fprintf(outf, "%s", str.buf());
    }
    if (do_fflush) {
      fflush(outf);
    }
  }
}

void
TW_logger::flush(int do_fflush)
{
  write2FILE(_file, do_fflush);
  if (_alt_file) {
    write2FILE(_alt_file, do_fflush);
  }
  str.clear();
}

void
TW_logger::flush_wo_prefix(int do_fflush)
{
  write2FILE_wo_prefix(_file, do_fflush);
  if (_alt_file) {
    write2FILE_wo_prefix(_alt_file, do_fflush);
  }
  str.clear();
}

// --------------------------------------------------------------------------------

static const char * TW_describe_act[] = {
  "dummy_act",
  "about_to_lock",
  "just_locked",
  "about_to_early_unlock",
  "about_to_weaken",
  "about_to_unlock",
  "about_to_wait",
  "about_to_timedwait",
  "just_relocked"
};

// --------------------------------------------------------------------------------

void
TW_DbgInfo::about_to_lock(void *m, int readlock, const char *resty,
			  const char *whatfn, const char *file, int line)
{
  _push1(TW_about_to_lock_act, m, 0, readlock, resty, whatfn, file, line);
  // describe it in case the lock call hangs
  _describe("ABOUT_TO");
  _pop1(); // pop it, the real lock act is coming next
}

void
TW_DbgInfo::just_locked(void *m, int readlock, const char *resty,
			const char *whatfn, const char *file, int line)
{
  _push1(TW_just_locked_act, m, 0, readlock, resty, whatfn, file, line);
  // OK, describe lock sequence as it may be a longest sequence
  // (ideally would remember and not print out duplicates)
  _describe("LOCK+");
}

void
TW_DbgInfo::just_relocked(void *c, void *m, int readlock, const char *resty,
			  const char *whatfn, const char *file, int line)
{
  _push1(TW_just_relocked_act, m, c, readlock, resty, whatfn, file, line);
  _describe("LOCK+"); // XXX could omit, does not contribute new longest sequence
}

void
TW_DbgInfo::about_to_weaken(void *m, const char *resty,
			    const char *whatfn, const char *file, int line)
{
  _push1(TW_about_to_weaken_act, m, 0, 1, resty, whatfn, file, line);
  if (_length < 2) {
    _describe("FATAL_ERROR: weaken: no locks held", 1); // fatal
  }
  int found = 0;
  // weaken can target any of the locks so we must search for the corresponding lock action
  TW_DbgElt *elt = _head->_prev->_prev; // act just prior to new tail act
  for (; elt != _head; elt = elt->_prev) {
    if ((elt->_act == TW_just_locked_act || elt->_act == TW_just_relocked_act) 
	&& elt->_mutex == m) {
      found = 1;
      break;
    }
  }
  if (!found) {
    _describe("FATAL_ERROR: weaken: no matching lock", 1); // fatal
  }
  if (elt->_weakened == 0 && elt->_readlock) {
    _describe("FATAL_ERROR: weaken: lock always held in read mode", 1); // fatal
  }
  if (::strcmp(elt->_resty, resty)) {
    _describe("FATAL_ERROR: weaken: resty mismatch", 1); // fatal
  }
  if (elt->_weakened && elt->_readlock) {
    _describe("WARNING: weaken: already weakened", 0);
  }
  // modify elt in place
  elt->_readlock = 1;
  elt->_weakened = 1;
  _pop1();  // pop weaken
  // OK, describe modified lock sequence
  _describe("WEAKEN"); // XXX could omit
}

void
TW_DbgInfo::about_to_early_unlock(void *m, int readlock, const char *resty,
				  const char *whatfn, const char *file, int line)
{
  _push1(TW_about_to_early_unlock_act, m, 0, readlock, resty, whatfn, file, line);
  if (_length < 2) {
    _describe("FATAL_ERROR: early_unlock: no locks held", 1); // fatal
  }
  int found = 0;
  // an early unlock can be an 'out of order' release, so we must search for the corresponding lock action
  TW_DbgElt *elt = _head->_prev->_prev; // act just prior to new tail act
  for (; elt != _head; elt = elt->_prev) {
    if ((elt->_act == TW_just_locked_act || elt->_act == TW_just_relocked_act) 
	&& elt->_mutex == m) {
      found = 1;
      break;
    }
  }
  if (!found) {
    _describe("FATAL_ERROR: early_unlock: no matching lock", 1); // fatal
  }
  if (::strcmp(elt->_resty, resty)) {
    _describe("FATAL_ERROR: early_unlock: resty mismatch", 1); // fatal
  }
  _pop1();  // pop unlock
  _rem1(elt); // remove matching lock
  // OK, describe lock sequence as it may be a longest sequence
  // (ideally would remember and not print out duplicates)
  _describe("LOCK-"); // XXX could omit
}

void
TW_DbgInfo::about_to_unlock(void *m, int readlock, const char *resty,
			    const char *whatfn, const char *file, int line)
{
  _push1(TW_about_to_unlock_act, m, 0, readlock, resty, whatfn, file, line);
  if (_length < 2) {
    _describe("FATAL_ERROR: unlock: no locks held", 1); // fatal
  }
  TW_DbgElt *elt = _head->_prev->_prev; // act just prior to new tail act
  if (elt->_act != TW_just_locked_act && elt->_act != TW_just_relocked_act) {
    _describe("FATAL_ERROR: unlock: last-1 should be locked or relocked", 1); // fatal
  }
  if (elt->_mutex != m) {
    _describe("FATAL_ERROR: unlock: last/last-1 mutex mismatch", 1); // fatal
  }
  if (::strcmp(elt->_resty, resty)) {
    _describe("FATAL_ERROR: unlock: last/last-1 resty mismatch", 1); // fatal
  }
  if (elt->_readlock != readlock) {
    _describe("FATAL_ERROR: unlock: last/last-1 readlock mismatch", 1); // fatal
  }
  _pop1(); // pop unlock
  _pop1(); // pop matching lock
  // OK, describe lock sequence as it may be a longest sequence
  // (ideally would remember and not print out duplicates)
  _describe("LOCK-"); // XXX could omit
}

void
TW_DbgInfo::about_to_wait(void *c, void *m, int readlock, const char *resty,
			  const char *whatfn, const char *file, int line)
{
  _push1(TW_about_to_wait_act, m, c, readlock, resty, whatfn, file, line);
  if (_length < 2) {
    _describe("FATAL_ERROR: wait: no locks held", 1); // fatal
  }
  TW_DbgElt *elt = _head->_prev->_prev; // act just prior to new tail act
  if (elt->_act != TW_just_locked_act && elt->_act != TW_just_relocked_act) {
    _describe("FATAL_ERROR: wait: last-1 should be locked or relocked", 1); // fatal
  }
  if (elt->_mutex != m) {
    _describe("FATAL_ERROR: wait: last/last-1 mutex mismatch", 1); // fatal
  }
  if (::strcmp(elt->_resty, resty)) {
    _describe("FATAL_ERROR: wait: last/last-1 resty mismatch", 1); // fatal
  }
  if (elt->_readlock != readlock) {
    _describe("FATAL_ERROR: wait: last/last-1 readlock mismatch", 1); // fatal
  }
  _pop1(); // pop the wait
  _pop1(); // pop matching lock
  _describe("LOCK-"); // XXX could omit
}

void
TW_DbgInfo::about_to_timedwait(void *c, unsigned long secs, unsigned long nanosecs,
			       void *m, int readlock, const char *resty,
			       const char *whatfn, const char *file, int line)
{
  _push1(TW_about_to_timedwait_act, m, c, readlock, resty, whatfn, file, line);
  if (_length < 2) {
    _describe("FATAL_ERROR: timedwait: no locks held", 1); // fatal
  }
  TW_DbgElt *elt = _head->_prev->_prev; // act just prior to new tail act
  if (elt->_act != TW_just_locked_act && elt->_act != TW_just_relocked_act) {
    _describe("FATAL_ERROR: timedwait: last-1 should be locked or relocked", 1); // fatal
  }
  if (elt->_mutex != m) {
    _describe("FATAL_ERROR: timedwait: last/last-1 mutex mismatch", 1); // fatal
  }
  if (::strcmp(elt->_resty, resty)) {
    _describe("FATAL_ERROR: timedwait: last/last-1 resty mismatch", 1); // fatal
  }
  if (elt->_readlock != readlock) {
    _describe("FATAL_ERROR: timedwait: last/last-1 readlock mismatch", 1); // fatal
  }
  _pop1(); // pop the timedwait
  _pop1(); // pop matching lock
  _describe("LOCK-"); // XXX could omit
}

// Format (all on one line):
//     TW_DEBUG: [PREFIX](<thread_id>)[elt|elt|elt]
// where elt is either the default short format:
//     ACT,,,mode,origMode,resty,whatfn,file,line
// or the long format [compile with TW_LONG_DBG_FORMAT]:
//     ACT,mutex_ptr,cv_ptr,mode,origMode,resty,whatfn,file,line
// mode,origMode can be one of :
//     R,r
//     W,w   
//     R,w (weakened)
// (cap letter is current mode, small letter is orig mode)

void
TW_DbgInfo::_describe(const char* prefix, int fatal)
{

  { // introduce logger scope
    TW_DbgLogger(l);
    l.str << "[" << prefix << "](" << TW_ID() << ")[";
    char mode;
    char orig;
    TW_DbgElt *elt = _head->_next; // first act
    for (; elt != _head; elt = elt->_next) {
      if (elt != _head->_next) l.str << '|';
      mode = (elt->_readlock ? 'R' : 'W');
      orig = (elt->_weakened ? 'w' : mode);
#ifndef TW_LONG_DBG_FORMAT
      l.str << TW_describe_act[elt->_act]  << ",,,"
	    << mode                                   << ","
	    << orig                                   << ","
	    << elt->_resty                            << ","
	    << elt->_whatfn                           << ","
	    << elt->_file                             << ","
	    << elt->_line;
#else
      l.str << TW_describe_act[elt->_act]                     << ",";
      if (elt->_mutex) {
	l.str << (void*)elt->_mutex << ",";
      } else {
	l.str << "0x0,";
      }
      if (elt->_cv) {
	l.str << (void*)elt->_cv << ",";
      } else {
	l.str << "0x0,";
      }
      l.str << mode                                           << ","
	    << orig                                           << ","
	    << elt->_resty                                    << ","
	    << elt->_whatfn                                   << ","
	    << elt->_file                                     << ","
	    << elt->_line;
#endif
    }
    l.str << "]\n";
  } // end logger scope, output has been written
  if (fatal) {
    abort();
  }
}

void
TW_DbgInfo::_push1(TW_DbgAct act, void *mutex, void *cv, int readlock, const char *resty,
		   const char *whatfn, const char *file, int line)
{
  const char *modfile = __TW_FNAME_SHORTEN(file);
  TW_DbgElt *e = new TW_DbgElt(act, mutex, cv, readlock, resty, whatfn, modfile, line);
  TW_DbgElt_APPEND(_head, e);
  _length++;
}

void
TW_DbgInfo::_pop1()
{
  TW_DbgElt_POP1(_head);
  _length--;
}

void
TW_DbgInfo::_rem1(TW_DbgElt *e)
{
  TW_DbgElt_DEL1(e);
  _length--;
}

void
TW_DbgInfo_cleanup(void* i)
{
  TW_DbgInfo* info = (TW_DbgInfo*)i;
  delete info;
}

// --------------------------------------------------------------------------------

#endif  /* TW_DEBUG */

#endif /* __THREAD_WRAPPERS_IMPL_H__ */

