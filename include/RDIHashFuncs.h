// -*- Mode: C++; -*-
//                              File      : RDIHashFuncs.h
//                              Package   : omniNotify-Library
//                              Created on: 15-June-2001
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
//    for use with hash tables
//
 
#ifndef __RDI_HASH_FUNCS_H__
#define __RDI_HASH_FUNCS_H__

#include "RDIStringDefs.h"
#include "corba_wrappers.h"

// HASH AND RANK FUNCTIONS
//   * Hash and Rank function definitions, together with several
//   * specialized implementations for some primitive data types

typedef int          (* RDI_FuncRank) (const void *, const void *);
typedef unsigned int (* RDI_FuncHash) (const void *);

inline int RDI_VoidRank(const void* L, const void* R)
{ return *((unsigned int *) L) - *((unsigned int *) R); }

inline int RDI_VoidStarRank(const void* L, const void* R)
{ return (L ?
	   (
	     R ?
	     (*((int *) L) - *((int *) R)) // both L and R
	     : (*((int*)L))                // just L
	   ) :
	   (
	     R ?
	     0 - (*((int*)R))              // just R
	     : 0                           // neither L nor R
	   )
	  );
}

inline int RDI_SIntRank(const void* L, const void* R)
{ return *((int *) L) - *((int *) R); }

inline int RDI_UIntRank(const void* L, const void* R)
{ unsigned int l = *((unsigned int *) L), r = *((unsigned int *) R); 
  return (l < r) ? -1 : ((l > r) ? 1 : 0); 
}

inline int RDI_SLongRank(const void* L, const void* R)
{ return *((long *) L) - *((long *) R); }

inline int RDI_ULongRank(const void* L, const void* R)
{ unsigned long l = *((unsigned long *) L), r = *((unsigned long *) R);
  return (l < r) ? -1 : ((l > r) ? 1 : 0); 
}

inline int RDI_StrRank(const void* L, const void* R)
{ return RDI_STR_RANK((const char*) L, (const char *) R); }

inline unsigned int RDI_VoidHash(const void* K)
{ return *((unsigned int *) K); }

inline unsigned int RDI_VoidStarHash(const void* K)
{ return K ? (*((unsigned int *) K)) : 0; }

inline unsigned int RDI_SIntHash(const void* K)
{ int l = *((int *) K); return (l < 0) ? -l : l; }

inline unsigned int RDI_UIntHash(const void* K)
{ return *((unsigned int *) K); }

inline unsigned int RDI_SLongHash(const void* K)
{ long l = *((long *) K); return (l < 0) ? -l : l; }

inline unsigned int RDI_ULongHash(const void* K)
{ return *((unsigned long *) K); }

inline unsigned int RDI_StrHash(const void* K)
{
  unsigned char *p = (unsigned char *) K;
  unsigned int   g, h=0;
  while ( (g = *p++) )
        h += (h << 7) + g + 987654321L;
  return h;
}

inline unsigned int RDI_CorbaSLongHash(const void* K)
{ CORBA::Long l = *((CORBA::Long *) K); return (l < 0) ? -l : l; }
    
inline unsigned int RDI_CorbaULongHash(const void* K)
{ return *((CORBA::ULong *) K); }

inline int RDI_CorbaSLongRank(const void* L, const void* R)
{ return *((CORBA::Long *) L) - *((CORBA::Long *) R); }

inline int RDI_CorbaULongRank(const void* L, const void* R)
{ CORBA::ULong l = *((CORBA::ULong *) L), r = *((CORBA::ULong *) R);
  return (l < r) ? -1 : ((l > r) ? 1 : 0); 
}

#endif /*  __RDI_HASH_FUNCS_H__  */

