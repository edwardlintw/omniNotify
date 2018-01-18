// -*- Mode: C++; -*-
//                              File      : RDI.cc
//                              Package   : omniNotify-Library
//                              Created on: 5-Jun-2001
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
//    internal class omniNotify

#include "thread_wrappers.h"
#include "RDI.h"
#include "RDIStringDefs.h"
#include "RDINotifServer.h"

// --------------------------------------------------------------------------------
// RDI static routines 

AttN::Server_ptr RDI::init_server(int& argc, char** argv)
{
  if (! _Server_i) {
#ifdef TW_DEBUG
    // TW::DbgLog("dbg.log");
#endif
    _Server_i = RDINotifServer::create(argc, argv);
    if (_Server_i) {
      _Server = WRAPPED_IMPL2OREF(AttNotification::Server, _Server_i);
    }
  }
  return _Server;
}

void RDI::wait_for_destroy()
{
  _Server_i->L_wait_for_destroy(); 
}

RDINotifServer* RDI::get_server_i()
{
  if (!_Server_i || _Server_i->destroyed()) {
    return 0;
  }
  return _Server_i;
}

void RDI::CleanupAll()
{
  RDIOplocks::shutdown();
  RDI::CloseDbgFile();
  RDI::CloseRptFile();
  // XXX WHAT ELSE? XXX
}

RDI_ServerQoS*
RDI::get_server_qos() 
{
  return _Server_i->_server_qos; 
}

// --------------------------------------------------------------------------------

#ifdef RDI_LONG_DBG_FILENAMES
#define __RDI_FNAME_SHORTEN(file) file
#else
// shorten /foo/bar/zot/baz.cc  to  zot/baz.cc  (leave one level of subdir context)
static const char* __RDI_FNAME_SHORTEN(const char* fname) {
  char* endptr = (char*)fname + RDI_STRLEN(fname) - 1;
  while ((endptr > fname) && (*(--endptr) != '/')) {;} // finds last /, if any
  while ((endptr > fname) && (*(--endptr) != '/')) {;} // finds next-to-last /, if any
  if (endptr > fname) return (const char*)(endptr+1);
  return fname;
}
#endif

// --------------------------------------------------------------------------------
// class RDI static members

RDINotifServer*  RDI::_Server_i      = 0;
AttN::Server_var RDI::_Server        = AttN::Server::_nil();
unsigned long    RDI::_DbgInterval_s = 0;
unsigned long    RDI::_DbgInterval_n = 0;
unsigned long    RDI::_RptInterval_s = 0;
unsigned long    RDI::_RptInterval_n = 0;
unsigned long    RDI::_DbgFlags      = 0;
unsigned long    RDI::_RptFlags      = 0;
FILE*            RDI::_DbgFile       = 0;
FILE*            RDI::_RptFile       = 0;

// --------------------------------------------------------------------------------
// class RDI static functions

int RDI::OpenDbgFile(const char* pathnm)
{
  if ( RDI_STR_EQ_I(pathnm, "stdout") ) {
    _DbgFile   = stdout;
  } else if ( RDI_STR_EQ_I(pathnm, "stderr") ) {
    _DbgFile   = stderr;
  } else {
    if ( ! (_DbgFile = fopen(pathnm, "a+")) ) {
      fprintf(stderr, "omniNotify: file open failed for DebugLogFile %s\n", pathnm);
      fprintf(stderr, "            debug logging reverts to stderr\n");
      _DbgFile   = stderr;
      return -1;
    }
  }
  return 0;
}

int RDI::OpenRptFile(const char* pathnm)
{
  if ( RDI_STR_EQ_I(pathnm, "stdout") ) {
    _RptFile   = stdout;
  } else if ( RDI_STR_EQ_I(pathnm, "stderr") ) {
    _RptFile   = stderr;
  } else {
    if ( ! (_RptFile = fopen(pathnm, "a+")) ) {
      fprintf(stdout, "omniNotify: file open failed for ReportLogFile %s\n", pathnm);
      fprintf(stdout, "            report logging reverts to stdout\n");
      _RptFile   = stdout;
      return -1;
    }
  }
  return 0;
}

void RDI::CloseDbgFile()
{
  if ( _DbgFile && (_DbgFile != stdout) && (_DbgFile != stderr) ) {
    fclose(_DbgFile);
  }
  _DbgFile = 0;
}

void RDI::CloseRptFile()
{
  if ( _RptFile && (_RptFile != stdout) && (_RptFile != stderr) ) {
    fclose(_RptFile);
  }
  _RptFile = 0;
}

// --------------------------------------------------------------------------------
// class RDI::logger

RDI::logger::logger(const char* prefix, FILE* file, FILE* alt_file, const char* flags, const char* srcfile, int srcline)
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
    prefix = "omniNotify";
  }
  const char* flb = " [";
  const char* frb = "]";
  if ((flags == (const char*)0) || (RDI_STRLEN(flags) == 0)) {
    flb = flags = frb = "";
  }
  if (srcfile == (const char*)0) {
    _prefix_buf = CORBA_STRING_ALLOC(strlen(preprefix) + strlen(prefix) + strlen(flb) + strlen(flags) + strlen(frb) + 2);
    sprintf(_prefix_buf, "%s%s%s%s%s: ", preprefix, prefix, flb, flags, frb);
  } else {
    char srcln[10];
    if (srcline == -1) {
      RDI_STRCPY(srcln, "?LINE?");
    } else {
      sprintf(srcln, "%d", srcline);
    }
    srcfile = __RDI_FNAME_SHORTEN(srcfile);
    _prefix_buf = CORBA_STRING_ALLOC(strlen(preprefix) + strlen(prefix) + strlen(flb) + strlen(flags) + strlen(frb)  + strlen(srcfile)  + strlen(srcln) + 5);
    sprintf(_prefix_buf, "%s%s%s%s%s[%s:%s]: ", preprefix, prefix, flb, flags, frb, srcfile, srcln);
  }
}

RDI::logger::~logger()
{
  flush(0);
  if (_prefix_buf) {
    CORBA_STRING_FREE(_prefix_buf);
    _prefix_buf = 0;
  }
}

void
RDI::logger::write2FILE(FILE* outf, CORBA::Boolean do_fflush)
{
  if (outf && (str.len() != 0)) {
    fprintf(outf, "%s%s", _prefix_buf, str.buf());
  }
  if (outf && do_fflush) {
    fflush(outf);
  }
}

void
RDI::logger::write2FILE_wo_prefix(FILE* outf, CORBA::Boolean do_fflush)
{
  if (outf && (str.len() != 0)) {
    fprintf(outf, "%s", str.buf());
  }
  if (outf && do_fflush) {
    fflush(outf);
  }
}

void
RDI::logger::flush(CORBA::Boolean do_fflush)
{
  write2FILE(_file, do_fflush);
  if (_alt_file) {
    write2FILE(_alt_file, do_fflush);
  }
  str.clear();
}

void
RDI::logger::flush_wo_prefix(CORBA::Boolean do_fflush)
{
  write2FILE_wo_prefix(_file, do_fflush);
  if (_alt_file) {
    write2FILE_wo_prefix(_alt_file, do_fflush);
  }
  str.clear();
}

