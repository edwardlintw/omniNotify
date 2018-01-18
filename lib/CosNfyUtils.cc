// -*- Mode: C++; -*-
//                              File      : CosNfyUtils.cc
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
//    Logging utils for CosN, CosNA, CosNF, ... types
//

#include "RDITimeWrappers.h" 
#include "CosNfyUtils.h"

RDIstrstream&
operator<< (RDIstrstream& str, const CosN::QoSError_code & ecode) {
  switch (ecode) {
  case CosN::UNSUPPORTED_PROPERTY:  return str << "UNSUPPORTED_PROPERTY";
  case CosN::UNAVAILABLE_PROPERTY:  return str << "UNAVAILABLE_PROPERTY";
  case CosN::UNSUPPORTED_VALUE:     return str << "UNSUPPORTED_VALUE";
  case CosN::UNAVAILABLE_VALUE:     return str << "UNAVAILABLE_VALUE";
  case CosN::BAD_PROPERTY:          return str << "BAD_PROPERTY";
  case CosN::BAD_TYPE:              return str << "BAD_TYPE";
  case CosN::BAD_VALUE:             return str << "BAD_VALUE";
  }
  return str << "(XXX unexpected CosN::QoSError_code)" << (unsigned long)ecode;
}

RDIstrstream&
operator<< (RDIstrstream& str, const CosNA::ProxyType & ptype) {
  switch (ptype) {
  case CosNA::PUSH_ANY:             return str << "PUSH_STRUCTURED";
  case CosNA::PULL_ANY:             return str << "PULL_ANY";
  case CosNA::PUSH_STRUCTURED:      return str << "PUSH_STRUCTURED";
  case CosNA::PULL_STRUCTURED:      return str << "PULL_STRUCTURED";
  case CosNA::PUSH_SEQUENCE:        return str << "PUSH_SEQUENCE";
  case CosNA::PULL_SEQUENCE:        return str << "PULL_SEQUENCE";
  case CosNA::PUSH_TYPED:           return str << "PUSH_TYPED";
  case CosNA::PULL_TYPED:           return str << "PULL_TYPED";
  }
  return str << "(XXX unexpected CosNA::ProxyType)" << (unsigned long)ptype;
}

RDIstrstream&
operator<< (RDIstrstream& str, const CosNA::ObtainInfoMode & infomode) {
  switch (infomode) {
  case CosNA::ALL_NOW_UPDATES_OFF:  return str << "ALL_NOW_UPDATES_OFF";
  case CosNA::ALL_NOW_UPDATES_ON:   return str << "ALL_NOW_UPDATES_ON";
  case CosNA::NONE_NOW_UPDATES_OFF: return str << "NONE_NOW_UPDATES_OFF";
  case CosNA::NONE_NOW_UPDATES_ON:  return str << "NONE_NOW_UPDATES_ON";
  }
  return str << "(XXX unexpected CosNA::ObtainInfoMode)" << (unsigned long)infomode;
}

RDIstrstream&
operator<< (RDIstrstream& str, const CosNA::ClientType & ctype) {
  switch (ctype) {
  case CosNA::ANY_EVENT:            return str << "ANY_EVENT";
  case CosNA::STRUCTURED_EVENT:     return str << "STRUCTURED_EVENT";
  case CosNA::SEQUENCE_EVENT:       return str << "SEQUENCE_EVENT";
  }
  return str << "(XXX unexpected CosNA::ClientType)" << (unsigned long)ctype;
}

RDIstrstream&
operator<< (RDIstrstream& str, const CosNA::InterFilterGroupOperator & ifilterop) {
  switch (ifilterop) {
  case CosNA::AND_OP:               return str << "AND_OP";
  case CosNA::OR_OP:                return str << "OR_OP";
  }
  return str << "(XXX unexpected CosNA::InterFilterGroupOperator)" << (unsigned long)ifilterop;
}

RDIstrstream&
operator<< (RDIstrstream& str, const CosN::EventType & t) {
  const char* dname = (t.domain_name ? (const char*)t.domain_name : "*");
  const char* tname = (t.type_name ? (const char*)t.type_name : "*");
  return str << dname << "::" << tname;
}

RDIstrstream&
operator<< (RDIstrstream& str, const CosN::EventTypeSeq & seq) {
  str << "EventTypeSeq:{ ";
  for (unsigned int i = 0; i < seq.length(); i++) {
    if (i>0) str << ", ";
    str << seq[i];
  }
  return str << " }";
}

RDIstrstream&
operator<< (RDIstrstream& str, const CosN::Property& prop) {
  return str << "name: " << prop.name << " value: " << prop.value;
}

RDIstrstream&
operator<< (RDIstrstream& str, const CosN::PropertySeq& propseq) {
  for (unsigned int i = 0; i < propseq.length(); i++) {
    str << propseq[i] << '\n';
  }
  return str;
}

RDIstrstream&
operator<< (RDIstrstream& str, const CosN::PropertyRange &range) {
    return str << "range "
	       << " : [" << range.low_val << " , " << range.high_val << "]";
}

RDIstrstream&
operator<< (RDIstrstream& str, const CosN::StructuredEvent& e) {
  const char* ename = (e.header.fixed_header.event_name ? (const char*)e.header.fixed_header.event_name : "");
  const char* dname = (e.header.fixed_header.event_type.domain_name ? (const char*)e.header.fixed_header.event_type.domain_name : "");
  const char* tname = (e.header.fixed_header.event_type.type_name ? (const char*)e.header.fixed_header.event_type.type_name : "");
  str << "Event with event_name = " << ename << '\n';
  str << "  dname::tname = " << dname << "::" << tname << '\n';
  str << "  Variable header fields:" << e.header.variable_header;
  str << "  Filterable data fields:" << e.filterable_data;
  str << "  Body:" << '\n' << e.remainder_of_body << '\n' << '\n';
  return str;
}

RDIstrstream&
operator<< (RDIstrstream& str, const CosN::PropertyError &e) {
  if ((e.code == CosN::BAD_VALUE) || (e.code == CosN::UNSUPPORTED_VALUE) || (e.code == CosN::UNAVAILABLE_VALUE)) {
    return str << "prop name " << e.name << " error " << e.code << " " << e.available_range;
  } else {
    return str << "prop name " << e.name << " error " << e.code;
  }
}

RDIstrstream&
operator<< (RDIstrstream& str, const CosN::PropertyErrorSeq &s) {
  for (unsigned int i = 0; i < s.length(); i++) {
    str << s[i] << '\n';
  }
  return str;
}

RDIstrstream&
operator<< (RDIstrstream& str, const CosN::UnsupportedAdmin &ua) {
  return str << "Exception CosN::UnsupportedAdmin admin_err = "
	     << '\n' << ua.admin_err;
}

RDIstrstream&
operator<< (RDIstrstream& str, const CosN::UnsupportedQoS &uqos) {
  return str << "Exception CosN::UnsupportedQoS qos_err = "
	     << '\n' << uqos.qos_err;
}

// Assuming dotted form is desired
RDIstrstream&
operator<< (RDIstrstream& str, const AttN::NameSeq& names) {
  for (unsigned int i = 0; i < names.length(); i++) {
    if (i > 0) str << ".";
    str << names[i];
  }
  return str;
}

// Currently only pretty-prints values that are found in property error ranges 
void RDI_pp_any(RDIstrstream& str,
		const CORBA::Any& a) {
  CORBA::Short           sval;
  CORBA::UShort          usval;
  CORBA::Long            lval;
  CORBA::ULong           ulval;
  CORBA::Boolean         bval;
  CORBA::Any::to_boolean to_bval(bval);

  if (a >>= to_bval) { if (bval) str << "1"; else str << "0";    return;  }
  if (a >>= sval)    { str << sval;    return;  }
  if (a >>= usval)   { str << usval;   return;  }
  if (a >>= lval)    { str << lval;    return;  }
  if (a >>= ulval)   { str << ulval;   return;  }
#ifdef TIMEBASE_NOLONGLONG
  const TimeBase::TimeT* tval = 0;
  if (a >>= tval)    { str << *tval;   return;  }
#else
  WRAPPED_CORBA_LONGLONG_TYPE   llval;
  WRAPPED_CORBA_LONGLONG_TYPE   ullval;
  if (a >>= llval)   { str << llval;   return;  }
  if (a >>= ullval)  { str << ullval;  return;  }
#endif
}

void RDI_describe_avail_range(RDIstrstream& str,
			      const CosN::PropertyRange &range) {
  str << " min ";
  RDI_pp_any(str, range.low_val);
  str << " max ";
  RDI_pp_any(str, range.high_val);
}

void RDI_describe_prop_errors(RDIstrstream& str,
			      const CosN::PropertyErrorSeq& error) {
  for (unsigned int i = 0; i < error.length(); i++) {
    switch (error[i].code) {
    case CosN::UNSUPPORTED_PROPERTY: {
      str << "The property " << error[i].name << " is not supported for the target object\n";
      break;
    }
    case CosN::UNAVAILABLE_PROPERTY: {
      str << "The property " << error[i].name << " cannot be modified (constrained by other property settings)\n";
      break;
    }
    case CosN::BAD_PROPERTY: {
      str << "The name " << error[i].name << " is not a valid property name\n";
      break;
    }
    case CosN::BAD_TYPE: {
      str << "Value supplied for property " << error[i].name << " has the wrong type\n";
      break;
    }  
    case CosN::BAD_VALUE: {
      str << "Value supplied for property " << error[i].name << " is outside the legal range of values\n  ... legal range:";
      RDI_describe_avail_range(str, error[i].available_range);
      str << '\n';
      break;
    }
    case CosN::UNSUPPORTED_VALUE: {
      str << "Value supplied for property " << error[i].name << " is not supported for the current target\n  ... supported range:";
      RDI_describe_avail_range(str, error[i].available_range);
      str << '\n';
      break;
    }
    case CosN::UNAVAILABLE_VALUE: {
      str << "Value supplied for property " << error[i].name << " is not available (due to other settings)\n  ... available range:";
      RDI_describe_avail_range(str, error[i].available_range);
      str << '\n';
      break;
    }
  }
  }
}
