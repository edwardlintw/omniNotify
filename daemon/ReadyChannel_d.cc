// -*- Mode: C++; -*-
//                              File      : ReadyChannel_d.cc
//                              Package   : omniNotify-daemon
//                              Created on: 1-Jan-1998
//                              Authors   : gruber&panagos
//
//    Copyright (C) 1998-2003 AT&T Laboratories -- Research
//
//    This file is part of executable program omniNotify-daemon
//    and is distributed with the omniNotify release.
//
//    omniNotify-daemon is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
//    USA.
//
// Description:
//    Implementation of the omniNotify daemon process
//

#include <stdio.h>
#include <stdlib.h>

#include "thread_wrappers.h"
#include "corba_wrappers.h"
#include "RDIOSWrappers.h"
#include "RDIsysdep.h"
#include "CosNotifyShorthands.h"
#include "omniNotify.h"
#include "RDIStringDefs.h"
#include "RDIstrstream.h"
#include "RDIInteractiveMode.h"

// ----------------------------------------------------------//

int main(int argc, char** argv)
{
  CORBA::Boolean interactive = 0;
  AttN::Server_ptr server = AttN::Server::_nil();
  int indx=0;
  char* pn = argv[0];
  while (RDI_STRCHR(pn, '/')) { pn = RDI_STRCHR(pn, '/'); pn++; }
  while (RDI_STRCHR(pn, '\\')) { pn = RDI_STRCHR(pn, '\\'); pn++; }
  if (RDI_STRLEN(pn) == 0) { pn = (char*)"notifd"; }
  char* pname = CORBA_STRING_DUP(pn);

  // Increase the limit of open file descriptor to the maximum
  RDI_OS_MAX_OPEN_FILE_DESCRIPTORS;

  while ( indx < argc ) {
    if (RDI_STR_EQ(argv[indx], "-h")) {
      fprintf(stdout, "Usage: %s [-h] [-v] [-i] [-n] [-c <f>]\n", pname);
      fprintf(stdout, "   -h       : print usage information and exit\n");
      fprintf(stdout, "   -v       : print software version and exit\n");
      fprintf(stdout, "   -i       : enter interactive mode\n");
      fprintf(stdout, "   -n       : do NOT register with the NameService\n");
      fprintf(stdout, "   -c <f>   : read config file <f>\n\n");
      return 0;
    }
    if (RDI_STR_EQ(argv[indx], "-v")) {
      fprintf(stdout, "notifd:  -- omniNotify server version %s\n", OMNINOTIFY_VERSION);
      return 0;
    }
    if (RDI_STR_EQ(argv[indx], "-i")) {
      RDI_rm_arg(argc, argv, indx);
      interactive = 1;
    } else {
      indx += 1;
    }
  }

  // Configure the undelying ORB and BOA/POA before creating
  // omniNotify server instance

  // Calls marked with ** only work for certain ORBs.  Others ignore.

  // Increase limit on # of outgoing connections to a given endpoint to 1000.
  // Handles case of large # of connected push consumers and/or
  // pull suppliers all running in the same client process.
  WRAPPED_ORB_SETMAXOUTGOING(1000); // **
  // You can set the following arg to 0 if you want to optimize omniORB4-only deployments
  WRAPPED_ORB_SET1CALLPERCON(1);    // **

  WRAPPED_ORB_OA::init(argc, argv);

  // initialize the server
  server = omniNotify::init_server(argc, argv);
  if (CORBA::is_nil(server)) {
    fprintf(stdout, "\nnotifd: failed to create a new omniNotify server\n");
    return -1;
  }
  WRAPPED_ORB_OA::activate_oas();
  if (interactive) {
    RDI_Interactive_Mode(server, 1); // 1 means spawn a thread
  }
  // wait for server to be destroyed
  omniNotify::wait_for_destroy();
  WRAPPED_ORB_OA::cleanup();
  CORBA_STRING_FREE(pname);
  return 0;
}

