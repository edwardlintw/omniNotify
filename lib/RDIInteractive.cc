// -*- Mode: C++; -*-
//                              File      : RDIInteractive.cc
//                              Package   : omniNotify-Library
//                              Created on: 10-Sep-2001
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
//    Class to help implement AttN::Interactive
 
#include "RDICatchMacros.h"
#include "RDIInteractive.h"
#include "CosNfyUtils.h"

void
RDIInteractive::cleanup_channels(RDIstrstream& str, AttN::Interactive_ptr chanfact,
				 CORBA::Boolean admins, CORBA::Boolean proxies) {
  CORBA::Boolean prob = 0;
  str << "=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n";
  if (proxies) {
    str << "Destroying Unconnected Proxies for All Channels\n";
  }
  if (admins) {
    str << "Destroying Admins with No Proxies for All Channels\n";
  }
  str << "=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n";
  AttN::IactSeq* chans = 0;
  try {
    chans = chanfact->children(1);
  } CATCH_INVOKE_PROBLEM(prob);
  if (prob || (chans == 0)) {
    str << "**chanfact unavailable**\n";
    return;
  }
  if (chans->length() == 0) {
    str << "No channels to cleanup\n";
    delete chans;
    return;
  }
  for (unsigned int idx = 0; idx < chans->length(); idx++) {
    cleanup_channel(str, (*chans)[idx], admins, proxies);
  }
  delete chans;
}

void
RDIInteractive::cleanup_channel(RDIstrstream& str, AttN::Interactive_ptr chan,
				CORBA::Boolean admins, CORBA::Boolean proxies) {
  CORBA::Boolean prob = 0;
  AttN::NameSeq* nm = 0;
  AttN::IactSeq* ads = 0;
  try {
    nm = chan->my_name();
    ads = chan->children(1);
  } CATCH_INVOKE_PROBLEM(prob);
  if (prob || (nm == 0)) {
    str << "**Channel unavailable**\n";
    if (ads) {
      delete ads;
    }
    return;
  }
  if (ads == 0) {
    str << "**Channel " << *nm << " unavailable**\n";
    delete nm;
    return;
  }
  if (ads->length() == 0) {
    str << "Channel " << *nm << " has no admins to cleanup\n";
    delete nm;
    delete ads;
    return;
  }
  str << "======================================================================\n";
  if (proxies) {
    str << "Destroying Unconnected Proxies for Channel " << *nm << '\n';
  }
  if (admins) {
    str << "Destroying Admins with No Proxies for Channel " << *nm << '\n';
  }
  str << "======================================================================\n";
  CORBA::ULong a_destroyed = 0;
  for (unsigned int idx = 0; idx < ads->length(); idx++) {
    if (cleanup_admin(str, (*ads)[idx], admins, proxies)) {
      a_destroyed++;
    }
  }
  if (admins) {
    str << "# Admins Destroyed: " << a_destroyed << '\n';
  }
  delete nm;
  delete ads;
}

CORBA::Boolean
RDIInteractive::cleanup_admin(RDIstrstream& str, AttN::Interactive_ptr admin,
			      CORBA::Boolean admins, CORBA::Boolean proxies) {
  CORBA::Boolean prob = 0;
  AttN::NameSeq* nm = 0;
  AttN::IactSeq* pxs = 0;
  try {
    nm = admin->my_name();
    if (proxies) {
      pxs = admin->children(1);
    }
  } CATCH_INVOKE_PROBLEM(prob);
  if (prob || (nm == 0)) {
    str << "**Admin unavailable**\n";
    if (pxs) {
      delete pxs;
    }
    return 0;
  }
  if (proxies) {
    if (pxs == 0) {
      str << "**Admin " << *nm << " unavailable**\n";
      delete nm;
      return 0;
    }
    if (pxs->length() == 0) {
      str << "Admin " << *nm << " has no unconnected proxies to cleanup\n";
    } else {
      str << "----------------------------------------------------------------------\n";
      str << "Destroying Unconnected Proxies for Admin " << *nm << '\n';
      str << "----------------------------------------------------------------------\n";
      CORBA::ULong d_destroyed = 0;
      for (unsigned int idx = 0; idx < pxs->length(); idx++) {
	AttN::NameSeq* pnm = 0;
	CORBA::Boolean pdel = 0;
	prob = 0;
	try {
	  pnm = (*pxs)[idx]->my_name();
	  pdel = (*pxs)[idx]->safe_cleanup();
	} CATCH_INVOKE_PROBLEM(prob);
	if ((prob == 0) && pnm) {
	  if (pdel) {
	    str << "Destroyed proxy " << *pnm << '\n';
	    d_destroyed++;
	  } else {
	    str << "Proxy " << *pnm << " not destroyed -- connected proxy\n";
	  }
	} else {
	  if (pnm) {
	    str << "**Proxy " << *pnm << " unavailable**\n";
	  } else {
	    str << "**Proxy unavailable**\n";
	  }
	}
	if (pnm) {
	  delete pnm;
	}
      }
      str << "# Proxies Destroyed: " << d_destroyed << '\n';
    }
  }
  CORBA::Boolean adel = 0;
  if (admins) {
    CORBA::Boolean prob = 0;
    try {
      adel = admin->safe_cleanup();
    } CATCH_INVOKE_PROBLEM(prob);
    if (prob == 0) {
      if (adel) {
	str << "Destroyed admin " << *nm << '\n';
      } else {
	str << "Admin " << *nm << " not destroyed -- default admin and/or admin has connected proxy\n";
      }
    } else {
      str << "**Admin " << *nm << " unavailable**\n";
    }
  }
  delete nm;
  delete pxs;
  return adel;
}
