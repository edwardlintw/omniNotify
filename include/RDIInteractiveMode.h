// -*- Mode: C++; -*-
//                              File      : RDIInteractiveMode.h
//                              Package   : omniNotify-Library
//                              Created on: 26-Feb-2002
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
//    static function RDI_Interactive_Mode available as an include file;
//    normally included by main program.  See ReadyChannel_d.cc for an example.
 
#include "omniNotify.h"

#ifndef __RDI_INTERACTIVE_MODE__
#define __RDI_INTERACTIVE_MODE__

#include "CosNotifyShorthands.h"
#include "RDIStringDefs.h"

////////////////////////////////////////////////////////////////////////////////
// Interactive Mode

static void RDI_DoIMode(void*);

struct RDI_DoIMode_Struct {
  AttN::Server_ptr s_ptr;
};

static void
RDI_Interactive_Mode(AttN::Server_ptr server, CORBA::Boolean spawn_thread)
{
  RDI_DoIMode_Struct* s = new RDI_DoIMode_Struct;
  s->s_ptr = server;
  if (spawn_thread) {
    (new TW_Thread(RDI_DoIMode, (void*)s))->start();
  } else {
    RDI_DoIMode((void*)s);
  }
}

static void
RDI_DoIMode(void* arg)
{
  // Process user input
  try {
    RDI_DoIMode_Struct* s = (RDI_DoIMode_Struct*)arg;
    if (!s) {
      fprintf(stdout, "** Error initializing Interactive Mode\n");
      fflush(stderr); fflush(stdout);
      return;
    }
    AttN::Server_ptr server = s->s_ptr;
    delete s;
    char cmnd[1024], final_nm[1024];
    CORBA::Boolean success = 1, target_changed = 1;
    CORBA::String_var cmdres;
    AttN::NameSeq* target_nm = 0;
    AttN::Interactive_var target = AttN::Interactive::_nil();
    AttN::Interactive_var next_target = AttN::Interactive::_duplicate(server);
    CORBA::Boolean docmd_problem = 0;
    sprintf(final_nm, "server");
  
    fprintf(stdout, "\n\t\t------------------------------------");
    fprintf(stdout, "\n\t\tomniNotify Server Version %s", OMNINOTIFY_VERSION);
    fprintf(stdout, "\n\t\t AT&T Laboratories -- Research ");
    fprintf(stdout, "\n\t\t------------------------------------\n\n");

    while ( 1 ) {
      if (target_changed) {
	target = next_target;
	next_target = AttN::Interactive::_nil();
	target_changed = 0;
	if (CORBA::is_nil(target)) {
	  goto server_problem;
	}
	if (target_nm) {
	  delete target_nm; target_nm = 0;
	}
	CORBA::Boolean name_problem = 0;
	try {
	  target_nm = target->my_name();
	} 
	catch ( CORBA::INV_OBJREF& e ) { name_problem = 1; } \
	catch ( CORBA::OBJECT_NOT_EXIST& e ) { name_problem = 1; } \
	catch ( CORBA::COMM_FAILURE& e ) { name_problem = 1; }
	if (name_problem || (!target_nm) || (target_nm->length() == 0)) {
	  // target may have become invalid
	  if (RDI_STR_EQ(final_nm, "server")) {
	    fprintf(stdout, "\nInteractive Mode: server not available\n");
	    goto server_problem;
	  }
	  fprintf(stdout, "\nInteractive Mode: target %s not available, reverting to top-level server target\n", final_nm);
	  target = AttN::Interactive::_narrow(server);
	  fprintf(stdout, "\nInteractive Mode: new target ==> server\n");
	  sprintf(final_nm, "server");
	  name_problem = 0;
	  try {
	    target_nm = target->my_name();
	  } 
	  catch ( CORBA::INV_OBJREF& e ) { name_problem = 1; } \
	  catch ( CORBA::OBJECT_NOT_EXIST& e ) { name_problem = 1; } \
	  catch ( CORBA::COMM_FAILURE& e ) { name_problem = 1; }
	  if (name_problem || (target_nm->length() == 0)) {
	    // something is very wrong
	    goto server_problem;
	  }
	}
	sprintf(final_nm, "%s", (*target_nm)[target_nm->length()-1].in());
      }
      success = 1;
      do {
	fprintf(stdout, "\nInteractive Mode [");
	for (unsigned int i = 0; i < target_nm->length(); i++) {
	  if (i > 0) fprintf(stdout, ".");
	  fprintf(stdout, "%s", (*target_nm)[i].in());
	}
	fprintf(stdout, "]: ");
      } while (0);
      fflush(stderr); fflush(stdout);
      TW_YIELD();
      if ( fgets(cmnd, 1024, stdin) ) {
	cmnd[ RDI_STRLEN(cmnd) - 1 ] = '\0';    // Remove new line
	RDIParseCmd p(cmnd);
	fprintf(stdout, "\n");
	if (p.argc == 0) {
	  continue;
	}
	// first see if daemon can handle request 
	if ((p.argc == 1) && (RDI_STR_EQ_I(p.argv[0], "exit") || RDI_STR_EQ_I(p.argv[0], "quit"))) {
	  fprintf(stdout, "Interactive Mode exiting....\n\n");
	  goto normal_return;
	}
	if ((p.argc == 1) && RDI_STR_EQ_I(p.argv[0], "shutdown")) {
	  fprintf(stdout, "Interactive Mode:  about to shutdown (destroy) server and exit....\n\n");
	  fflush(stdout);
	  CORBA::Boolean destroy_problem = 0;
	  try {
	    server->destroy();
	  }
	  catch ( CORBA::INV_OBJREF& e ) { destroy_problem = 1; } \
	  catch ( CORBA::OBJECT_NOT_EXIST& e ) { destroy_problem = 1; } \
	  catch ( CORBA::COMM_FAILURE& e ) { destroy_problem = 1; }
	  if (destroy_problem) {
	    fprintf(stdout, "Interactive Mode: destroy server failed, exiting anyway....\n\n");
	  } else {
	    fprintf(stdout, "Interactive Mode: destroy server succeeded, exiting....\n\n");
	  }
	  goto normal_return;
	}
	if ((p.argc == 1) && RDI_STR_EQ_I(p.argv[0], "children")) {
	  AttN::NameSeq* child_names = 0;
	  CORBA::Boolean cnames_problem = 0;
	  try {
	    child_names = target->child_names();
	  } 
	  catch ( CORBA::INV_OBJREF& e ) { cnames_problem = 1; } \
	  catch ( CORBA::OBJECT_NOT_EXIST& e ) { cnames_problem = 1; } \
	  catch ( CORBA::COMM_FAILURE& e ) { cnames_problem = 1; }
	  if (cnames_problem) {
	    fprintf(stdout, "\nInteractive Mode: target %s not available, reverting to top-level server target\n", final_nm);
	    next_target = AttN::Interactive::_narrow(server);
	    target_changed = 1;
	    fprintf(stdout, "\nInteractive Mode: new target ==> server\n");
	    sprintf(final_nm, "server");
	    continue;
	  }
	  if (child_names) {
	    if (child_names->length()) {
	      fprintf(stdout, "Children of %s : ", final_nm);
	      for (unsigned int i = 0; i < child_names->length(); i++) {
		if (i > 0) fprintf(stdout, ", ");
		fprintf(stdout, "%s", (*child_names)[i].in());
	      }
	      fprintf(stdout, "\n  (Use 'go <name>' to go to one of the children)\n");
	    } else {
	      fprintf(stdout, "No children\n");
	    }
	    delete child_names;
	  } else {
	    fprintf(stdout, "**Error getting children**\n");
	  }
	  continue;
	}
	if ((p.argc == 2) && RDI_STR_EQ_I(p.argv[0], "go") && RDI_STR_EQ_I(p.argv[1], "server")) {
	  next_target = AttN::Interactive::_narrow(server);
	  target_changed = 1;
	  fprintf(stdout, "\nInteractive Mode: new target ==> server\n");
	  sprintf(final_nm, "server");
	  continue;
	}
	// now see if server or target can handle cmnd
	// direct some commands to server regardless of currrent target
	docmd_problem = 0;
	if ( ( (p.argc == 2) && RDI_STR_EQ_I(p.argv[0], "go") && 
	       ( RDI_STR_EQ_I(p.argv[1], "chanfact") ||
		 RDI_STR_EQ_I(p.argv[1], "filtfact") ||
		 RDI_STRN_EQ_I(p.argv[1], "chanfact.", 9) ||
		 RDI_STRN_EQ_I(p.argv[1], "filtfact.", 9) ) ) ||
	     ( (p.argc == 1) && 
	       ( RDI_STR_EQ_I(p.argv[0], "flags") ||
		 ((RDI_STRLEN(p.argv[0]) > 1) && ((p.argv[0][0] == '+') || (p.argv[0][0] == '-'))))) ) {
	  try {
	    cmdres = server->do_command(cmnd, success, target_changed, next_target);
	  }
	  catch ( CORBA::INV_OBJREF& e ) { docmd_problem = 1; } \
	  catch ( CORBA::OBJECT_NOT_EXIST& e ) { docmd_problem = 1; } \
	  catch ( CORBA::COMM_FAILURE& e ) { docmd_problem = 1; }
	  if (docmd_problem) {
	    // something is very wrong
	    goto server_problem;
	  }
	} else {
	  try {
	    cmdres = target->do_command(cmnd, success, target_changed, next_target);
	  }
	  catch ( CORBA::INV_OBJREF& e ) { docmd_problem = 1; } \
	  catch ( CORBA::OBJECT_NOT_EXIST& e ) { docmd_problem = 1; } \
	  catch ( CORBA::COMM_FAILURE& e ) { docmd_problem = 1; }
	}
	if (docmd_problem) {
	  fprintf(stdout, "\nInteractive Mode: target %s not available, reverting to top-level server target\n", final_nm);
	  next_target = AttN::Interactive::_narrow(server);
	  target_changed = 1;
	  fprintf(stdout, "\nInteractive Mode: new target ==> server\n");
	  sprintf(final_nm, "server");
	  continue;
	}
	if (! success) {
	  fprintf(stdout, "\nUse 'help' for a list of valid commands\n");
	}
	fprintf(stdout, "\n%s\n", cmdres.in());
	if ((p.argc == 1) && RDI_STR_EQ_I(p.argv[0], "help")) {
	  fprintf(stdout, "The following commands work regardless of the current target:\n");
	  fprintf(stdout, "  exit              : exit interactive mode\n");
	  fprintf(stdout, "  shutdown          : destroy the server, exit interactive mode\n");
	  fprintf(stdout, "  go server         : change target to server\n");
	  fprintf(stdout, "  go chanfact       : change target to channel factory\n");
	  fprintf(stdout, "  go filtfact       : change target to filter factory\n");
	  fprintf(stdout, "  children          : list children of current target\n");
	  fprintf(stdout, "  flags             : show debug/report flag settings\n");
	  fprintf(stdout, "  +<flag-name>      : enable a debug/report flag\n");
	  fprintf(stdout, "  -<flag-name>      : disable a debug/report flag\n");
	  fprintf(stdout, "  +alldebug         : enable all debug flags\n");
	  fprintf(stdout, "  -alldebug         : disable all debug flags\n");
	  fprintf(stdout, "  +allreport        : enable all report flags\n");
	  fprintf(stdout, "  -allreport        : disable all report flags\n");
	}
      }
    }
  }
  catch(CORBA::SystemException&) {
    fprintf(stdout, "Interactive Mode caught CORBA::SystemException.\n");
  }                                                    
  catch(CORBA::Exception&) {                           
    fprintf(stdout, "Interactive Mode caught CORBA::Exception.\n");
  }                                                    
  catch(omniORB::fatalException& fe) {                 
    fprintf(stdout, "Interactive Mode caught omniORB::fatalException:\n");
    fprintf(stdout, "  file: %s\n", fe.file());
    fprintf(stdout, "  line: %d\n", fe.line());
    fprintf(stdout, "  mesg: %s\n", fe.errmsg());
  }
  catch(...) {
    // nameclt comment says it is a bad idea to report an error here 
  }
  fprintf(stdout, "Interactve Mode caught an exception.\n\n");
  fprintf(stdout, "QUITTING Interactive Mode due to error\n");
  fflush(stderr); fflush(stdout);
  return;
 server_problem:
  fprintf(stdout, "QUITTING Interactive Mode -- lost ability to communicate with server\n");
  fflush(stderr); fflush(stdout);
  return;
 normal_return:
  fflush(stderr); fflush(stdout);
  return;
}

#endif  /*  __RDI_INTERACTIVE_MODE__  */
