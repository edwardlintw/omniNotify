// -*- Mode: C++; -*-
//                              File      : RDIStringDefs.h
//                               (formerly RDIDefs.h)
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
 
#ifndef __RDI_STRING_DEFS_H__
#define __RDI_STRING_DEFS_H__

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "corba_wrappers.h"

//-------------------------------------------------------------------------
// RDI_DELNULL : only invoke on reference to obj allocated using new
#define RDI_DELNULL(x)      if (x) {delete x; x=0;}
#define RDI_DELNULL2(x,y)   RDI_DELNULL(x) RDI_DELNULL(y)
#define RDI_DELNULL3(x,y,z) RDI_DELNULL(x) RDI_DELNULL(y) RDI_DELNULL(z)

//-------------------------------------------------------------------------
// STRING MANIPULATION FUNCTIONS / UTILS

#define RDI_STRLEN(s)             ::strlen(s)
#define RDI_STRCHR(s, c)          ::strchr(s, c)
#define RDI_STR_RANK(s1,s2)       ::strcmp(s1,s2)

#define RDI_STRCPY(dst, src)      ::strcpy(dst, src)
#define RDI_STRNCPY(dst, src, n)  ::strncpy(dst, src, n)

// conversion routines: on success, *endptr_out should be == 0
// on failure it points to first bad character, or, if there
// is a range error, endptr itself is set to NULL
// Therefore, you must first check endptr_out == 0
// before testing *endptr_out == 0 !! 

inline CORBA::Long RDI_STRTOL(const char* ptr, char** endptr_out) {
  errno = 0;
  CORBA::Long res = (CORBA::Long) ::strtol(ptr, endptr_out, 0);
  if (errno) {
    // range error
    endptr_out = 0;
  }
  return res;
}
inline CORBA::ULong RDI_STRTOUL(const char* ptr, char** endptr_out) {
  errno = 0;
  CORBA::ULong res = (CORBA::ULong) ::strtoul(ptr, endptr_out, 0);
  if (errno) {
    // range error
    endptr_out = 0;
  }
  return res;
}
inline CORBA::LongLong RDI_STRTOLL(const char* ptr, char** endptr_out) {
  errno = 0;
#ifdef __WIN32__
  CORBA::LongLong res = (CORBA::LongLong) ::strtol(ptr, endptr_out, 0);
#else
  CORBA::LongLong res = (CORBA::LongLong) ::strtoll(ptr, endptr_out, 0);
#endif
  if (errno) {
    // range error
    endptr_out = 0;
  }
  return res;
}
inline CORBA::LongLong RDI_STRTOULL(const char* ptr, char** endptr_out) {
  errno = 0;
#ifdef __WIN32__
  CORBA::ULongLong res = (CORBA::ULongLong) ::strtoul(ptr, endptr_out, 0);
#else
  CORBA::ULongLong res = (CORBA::ULongLong) ::strtoull(ptr, endptr_out, 0);
#endif
  if (errno) {
    // range error
    endptr_out = 0;
  }
  return res;
}
inline CORBA::Double RDI_STRTOD(const char* ptr, char** endptr_out) {
  errno = 0;
  CORBA::Double res = (CORBA::Double) ::strtod(ptr, endptr_out);
  if (errno) {
    // range error
    endptr_out = 0;
  }
  return res;
}

#define RDI_STR_EQ(s1,s2)  (RDI_STR_RANK(s1,s2)==0)
#define RDI_STR_NEQ(s1,s2) (RDI_STR_RANK(s1,s2)!=0)
#define RDI_STR_LT(s1,s2)  (RDI_STR_RANK(s1,s2)<0)
#define RDI_STR_LE(s1,s2)  (RDI_STR_RANK(s1,s2)<=0)
#define RDI_STR_GT(s1,s2)  (RDI_STR_RANK(s1,s2)>0)
#define RDI_STR_GE(s1,s2)  (RDI_STR_RANK(s1,s2)>=0)

#define RDI_STRN_EQ(s1,s2,n)     (::strncmp(s1,s2,n)==0)
#define RDI_STRN_NEQ(s1,s2,n)    (::strncmp(s1,s2,n)!=0)

#define RDI_IS_ALPHA_OR_UNDERSCORE(c)     ((c == '_') || isalpha(c))
#define RDI_IS_ALPHANUM_OR_UNDERSCORE(c)  ((c == '_') || isalnum(c))
#define RDI_IS_SPACE(c)                   isspace(c)

// compare, ignoring case
#if defined(_MSC_VER)
#  define RDI_STR_EQ_I(s1,s2)    (_stricmp(s1,s2)==0)
#  define RDI_STRN_EQ_I(s1,s2,n) (_strnicmp(s1,s2,n)==0) // ???
#else
#  define RDI_STR_EQ_I(s1,s2)    (strcasecmp(s1,s2)==0)
#  define RDI_STRN_EQ_I(s1,s2,n) (strncasecmp(s1,s2,n)==0)
#endif

// RDIParseCmd: constructor parses cmd, fills in members argc and argv.
// argv is a newly allocated array that is freed when the object is deleted.
// (cmd is not modified)
//
// Example:
//     do {
//       RDIParseCmd p(cmd);
//       ... use p.argc and p.argv ...
//     } while (0);
// ** p's space is freed up, including p.argv

#define RDI_PARSECMD_MAXARGS 64  // our commands are very simple; leave room anyway

class RDIParseCmd {
public:
  int argc;
  char** argv;
  RDIParseCmd(const char* cmd) {
    argc = 0;
    argv = new char*[RDI_PARSECMD_MAXARGS];
    for (unsigned int i = 0; i < RDI_PARSECMD_MAXARGS; i++) {
      argv[i] = 0;
    }
    char* argloc = (char*)cmd;
    char* cmdend = argloc + RDI_STRLEN(cmd);
    while (1) {
      if (argloc >= cmdend) return; // done
      while (RDI_IS_SPACE(*argloc)) {
	argloc++; // skip leading white space
	if (argloc >= cmdend) return; // done
      }
      // found something
      char* sploc = argloc + 1;
      int len = 1;
      while ((sploc < cmdend) && (! RDI_IS_SPACE(*sploc))) {
	sploc++; // skip the arg
	len++;
      }
      char* buf = new char[len+1];
      RDI_STRNCPY(buf, argloc, len);
      buf[len] = '\0';
      argloc = sploc + 1;
      argv[argc++] = buf;
      // the following test uses MAXARGS-1 to leave space for final NULL in argv
      if (argc == (RDI_PARSECMD_MAXARGS-1)) return; // done
    }
  }
  ~RDIParseCmd() {
    for (unsigned int i = 0; i < RDI_PARSECMD_MAXARGS; i++) {
      if (argv[i]) {
	delete [] argv[i];
	argv[i] = 0;
      }
    }
    delete [] argv;
    argv = 0;
  }
};

// removes arg at index, modifies argc
inline void RDI_rm_arg(int& argc, char** argv, int& index) {
  if (index >= argc) return;
  for (int i = index; i+1 < argc; i++) {
    argv[i] = argv[i+1];
  }
  argc--;
}

//-------------------------------------------------------------------------
// CORBA-specific cases

#define CORBA_STRING_ALLOC(sz) CORBA::string_alloc(sz)
#define CORBA_STRING_DUP(str)  CORBA::string_dup(str)
#define CORBA_STRING_FREE(str) CORBA::string_free(str)

#define RDI_STRDELNULL(x)      if (x) {CORBA_STRING_FREE(x); x=0;}
#define RDI_STRDELNULL2(x,y)   RDI_STRDELNULL(x) RDI_STRDELNULL(y)
#define RDI_STRDELNULL3(x,y,z) RDI_STRDELNULL(x) RDI_STRDELNULL(y) RDI_STRDELNULL(z)

inline char*
RDI_BUILD_STRING(const char* s1) { return CORBA_STRING_DUP(s1); }

inline char*
RDI_BUILD_STRING(const char* s1, 
		 const char* s2,
		 const char* s3 = "",
		 const char* s4 = "",
		 const char* s5 = "",
		 const char* s6 = "",
		 const char* s7 = "",
		 const char* s8 = "") {
  int len = RDI_STRLEN(s1) + RDI_STRLEN(s2) + RDI_STRLEN(s3) + RDI_STRLEN(s4) + 
    RDI_STRLEN(s5) + RDI_STRLEN(s6) + RDI_STRLEN(s7) + RDI_STRLEN(s8);
  char* result = CORBA_STRING_ALLOC(len);
  sprintf(result, "%s%s%s%s%s%s%s%s", s1, s2, s3, s4, s5, s6, s7, s8);
  return(result);
}

// RDIParseId: returns -1 on error.
//   A # >= zero means buf contains an identifier made up of the
//   expected prefix and a non-neg number, with no extraneous characters.

inline CORBA::Long RDIParseId(const char* prefix, char* buf) {
  if (RDI_STRN_EQ_I(prefix, buf, RDI_STRLEN(prefix))) {
    char* delim = 0;
    char* strid = buf + RDI_STRLEN(prefix);
    CORBA::Long id = RDI_STRTOL(strid, &delim);
    if ( (delim == 0) || (delim == strid) || (*delim != '\0') ) {
      return -1;
    }
    return id;
  }
  return -1;
}


#endif  /* __RDI_STRING_DEFS_H__ */



