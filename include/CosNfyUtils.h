// -*- Mode: C++; -*-
//                              File      : CosNotifyUtils.h
//                              Package   : omniNotify-Library
//                              Created on: 19-June-2001
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
//   Logging routines for Cos*.idl-based types.
//   (RDITimeWrappers handles TimeBase logging.)

#ifndef __COS_NFY_UTILS_H__
#define __COS_NFY_UTILS_H__

#include "CosNotifyShorthands.h"
#include "RDIstrstream.h"

RDIstrstream& operator<< (RDIstrstream& str, const CosN::QoSError_code & ecode);
RDIstrstream& operator<< (RDIstrstream& str, const CosN::EventType & t);
RDIstrstream& operator<< (RDIstrstream& str, const CosN::EventTypeSeq & seq);
RDIstrstream& operator<< (RDIstrstream& str, const CosN::Property& prop);
RDIstrstream& operator<< (RDIstrstream& str, const CosN::PropertyRange &range);
RDIstrstream& operator<< (RDIstrstream& str, const CosN::StructuredEvent& e);
RDIstrstream& operator<< (RDIstrstream& str, const CosN::PropertyError &e);
RDIstrstream& operator<< (RDIstrstream& str, const CosN::PropertyErrorSeq &s);
RDIstrstream& operator<< (RDIstrstream& str, const CosN::UnsupportedAdmin &ua);
RDIstrstream& operator<< (RDIstrstream& str, const CosN::UnsupportedQoS &uqos);
RDIstrstream& operator<< (RDIstrstream& str, const CosN::PropertySeq& propseq);
RDIstrstream& operator<< (RDIstrstream& str, const CosNA::ProxyType & ptype);
RDIstrstream& operator<< (RDIstrstream& str, const CosNA::ObtainInfoMode & infomode);
RDIstrstream& operator<< (RDIstrstream& str, const CosNA::ClientType & ctype);
RDIstrstream& operator<< (RDIstrstream& str, const CosNA::InterFilterGroupOperator & ifilterop);
RDIstrstream& operator<< (RDIstrstream& str, const AttN::NameSeq& names);

void RDI_pp_any(RDIstrstream& str, const CORBA::Any& a);
void RDI_describe_avail_range(RDIstrstream& str, const CosN::PropertyRange &range);
void RDI_describe_prop_errors(RDIstrstream& str, const CosN::PropertyErrorSeq& error);

#endif
