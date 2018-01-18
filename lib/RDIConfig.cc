// -*- Mode: C++; -*-
//                              File      : RDIConfig.cc
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
//    Implementation of RDI_Config
//
 
#include "RDIstrstream.h"
#include "RDI.h"
#include "RDILimits.h"
#include "RDIConfig.h"
#include "RDIStringDefs.h"
#include "RDINotifServer.h"
#include "CosNotification_i.h"

static inline unsigned int MyStringHash(const void* K)
{
  unsigned int   h=0;
  unsigned char* p=0;
  for ( p = (unsigned char *) K; *p; p++ )
    h = 5*h + *p;
  return h;
}

RDI_Config::RDI_Config()
{
  for (unsigned int indx=0; indx < HASH_SIZE; indx++)
    _htbl[indx] = 0;	
}

RDI_Config::~RDI_Config()
{
  for (unsigned int indx=0; indx < HASH_SIZE; indx++) {
    while ( _htbl[indx] ) {
      node_t* tmp = _htbl[indx];
      _htbl[indx] = _htbl[indx]->_next;
      delete tmp;
    }
  }
}

CORBA::Boolean
RDI_Config::parse_arguments(RDIstrstream& str, int& argc, char** argv, CORBA::Boolean rm_args)
{
  CORBA::Boolean bad_param = 0;
  char *pname=0, *value=0;
  int  indx=0;

  while ( ++indx < argc ) {
    if ( (RDI_STRLEN(argv[indx]) < 2) || argv[indx][0] != '-' || argv[indx][1] != 'D' ) {
      continue;
    }
    if ( (RDI_STRLEN(argv[indx]) < 5) || (argv[indx][2] == '=') ) {
      str << "Command-line argument error:\n"
	  << "  Badly formed -D option: " << argv[indx] << "\n"
	  << "  (must have the form -D<name>=<value>)\n";
      bad_param = 1;
      goto end_of_loop;
    }
    pname = &argv[indx][2];
    value = pname;
    while ( *value && (*value != '=') ) {
      ++value;
    }
    if ( *value != '=' ) {
      str << "Command-line argument error:\n"
	  << "  Badly formed -D option: " << argv[indx] << "\n"
	  << "  (must have the form -D<name>=<value>)\n";
      bad_param = 1;
      goto end_of_loop;
    }
    if ( *(value+1) == '\0') {
      str << "Command-line argument error:\n"
	  << "  Badly formed -D option: " << argv[indx] << "\n"
	  << "  (must have the form -D<name>=<value>)\n";
      bad_param = 1;
      goto end_of_loop;
    }
    *value++ = '\0';
    if ( RDI_STR_EQ(pname, "CONFIGFILE") ) {
      bad_param = this->import_settings(str, value);
    } else if ( RDINotifServer::is_startup_prop(pname) || RDI_ServerQoS::is_server_prop(pname) || 
		RDI_AdminQoS::is_admin_prop(pname) || RDI_NotifQoS::is_qos_prop(pname) ) {
      if ( this->set_value(pname, value) ) {
	*(value-1) = '=';
	str << "Command-line argument error:\n"
	    << "  Badly formed -D option: " << argv[indx] << "\n"
	    << "  (must have the form -D<name>=<value>)\n";
	bad_param = 1;
      }
    } else {
      str << "Command-line argument error:\n"
	  << "  Property name \"" << pname << "\" is not a valid Server, QoS or Admin Property name\n";
      bad_param = 1;
    }
  end_of_loop:
    if ( rm_args ) {
      RDI_rm_arg(argc, argv, indx);
      indx -= 1;
    }
  }
  return bad_param;
}

CORBA::Boolean
RDI_Config::import_settings(RDIstrstream& str, const char* fname)
{
  FILE* file = 0;
  char  line[1024];
  char *pname=0, *value=0;
  unsigned int lnum = 0;
  
  if ( ! fname || ! RDI_STRLEN(fname) ) {
    return 1;
  }
  if ( (file = fopen(fname, "r")) == (FILE *) 0 ) {
    str << "Could not open CONFIGFILE " << fname << " for reading\n";
    return 1;
  }
  CORBA::Boolean configline_err = 0;
  while ( fgets(line, sizeof(line)-1, file) != (char *) 0 ) {
    if (line[RDI_STRLEN(line)-1] == '\n') {
      line[RDI_STRLEN(line)-1] = '\0';
    }
    lnum += 1;
    pname = & line[0];
    // skip leading white space for pname
    while ( *pname == ' ' || *pname == '\t' ) {
      pname++;
    }
    // ignore commented/blank lines
    if ( *pname=='#' || *pname=='\n' || *pname=='\0' ) {
      continue;
    }
    value = pname;
    while ( *value != ' ' && *value != '\t' && *value != '\0' ) {
      value++;
    }
    if ( *value == '\0' ) {
      str << "Error in config file " << fname << " line # " << lnum << ":\n"
	  << "  Badly formed entry starting with \"" << pname << "\"\n"
	  << "  (should be <property-name> <value>, with space between -- value missing?)\n";
      configline_err = 1;
      continue;
    }
    *value++ = '\0';
    // skip leading white space for value
    while ( *value == ' ' || *value == '\t' ) {
      value++;
    }
    // Remove trailing blanks and newlines from value
    while ( value[RDI_STRLEN(value)-1]=='\n' ||
	    value[RDI_STRLEN(value)-1]=='\t' || value[RDI_STRLEN(value)-1]==' ' ) {
      value[ RDI_STRLEN(value) - 1] = '\0';
    }
    if ( RDINotifServer::is_startup_prop(pname) || RDI_ServerQoS::is_server_prop(pname) || 
	 RDI_AdminQoS::is_admin_prop(pname) || RDI_NotifQoS::is_qos_prop(pname) ) {
      if ( this->set_value(pname, value) ) {
	str << "Error in config file " << fname << " line # " << lnum << ":\n"
	    << "  Badly formed entry starting with \"" << pname << "\"\n"
	    << "  (should be <property-name> <value>, with space between -- value missing?)\n";
	configline_err = 1;
      }
    } else {
      str << "Error in config file " << fname << " line # " << lnum << ":\n"
	  << "  Property name \"" << pname << "\" is not a valid Server, QoS or Admin Property name\n";
      configline_err = 1;
    }
  }
  (void) fclose(file);
  return configline_err;
}

int RDI_Config::export_settings(const char* fname, const char* header) const
{
  FILE*   file = 0;
  node_t* node = 0;

  if ( ! fname || ! RDI_STRLEN(fname) ) {
    return -1;
  }
  if ( (file = fopen(fname, "w")) == (FILE *) 0 ) {
    return -1;
  }

  fprintf(file, "# ==================================================\n");
  fprintf(file, "#        R E A D Y  Configuration  Parameters       \n");
  fprintf(file, "#                                                   \n");
  fprintf(file, "# You can modify the value of any variable by either\n");
  fprintf(file, "# editing the file and changing the desired value or\n");
  fprintf(file, "# seting the environment variable of the name to the\n");
  fprintf(file, "# desired value before process execution.           \n");
  fprintf(file, "#===================================================\n\n");

  if ( header && RDI_STRLEN(header) ) {
    fprintf(file, "# %s\n", header);
  }

  for (unsigned int indx=0; indx < HASH_SIZE; indx++) {
    for (node = _htbl[indx]; node != (node_t *) 0; node = node->_next)
      fprintf(file, "%-30s  %s\n", node->_name, node->_value);
  }
  (void) fclose(file);
  return 0;
}

int RDI_Config::get_value(RDIstrstream& str, const char* pname, char*& value, CORBA::Boolean log_type_errors) const
{
  unsigned int indx = MyStringHash(pname) & HASH_MASK;
  node_t*      node = _htbl[indx];

  while ( node && RDI_STR_NEQ(node->_name, pname) ) {
    node = node->_next;
  }
  if ( ! node ) {
    return -1;
  }
  value = node->_value;
  return 0;
}

int RDI_Config::get_value(RDIstrstream& str, const char* pname, CORBA::Boolean& value, CORBA::Boolean log_type_errors) const
{
  char *delim=0, *strvl=0;
  CORBA::ULong ul_value;

  if ( get_value(str, pname, strvl, 0) ) {
    return -1;
  }
  ul_value = RDI_STRTOUL(strvl, &delim);
  if ( (delim == 0) || (delim == strvl) || (*delim != '\0') ) {
    if (log_type_errors) {
      str << "Value for property " << pname << " invalid : Expected boolean value (0 or 1) -- found " << strvl << '\n';
    }
    return -2;
  }
  if ( (ul_value != 0) && (ul_value != 1) ) {
    if (log_type_errors) {
      str << "Value for property " << pname << " invalid : Expected boolean value (0 or 1) -- found " << strvl << '\n';
    }
    return -2;
  }
  value = (CORBA::Boolean)ul_value;
  return 0;
}

int RDI_Config::get_value(RDIstrstream& str, const char* pname, CORBA::Short& value, CORBA::Boolean log_type_errors) const
{
  char *delim=0, *strvl=0;

  if ( get_value(str, pname, strvl, 0) ) {
    return -1;
  }
  CORBA::Long l_value = RDI_STRTOL(strvl, &delim);
  if ( (delim == 0) || (delim == strvl) || (*delim != '\0') ) {
    if (log_type_errors) {
      str << "Value for property " << pname << " invalid : Expected short integer value -- found " << strvl << '\n';
    }
    return -2;
  }
  if ((l_value < RDI_SHORT_MIN) || (l_value > RDI_SHORT_MAX)) {
    if (log_type_errors) {
      str << "Value for property " << pname << " under/overflow: Expected short integer value -- found " << strvl << '\n';
      str << "  (Min short = " << RDI_SHORT_MIN << " , Max short = " << RDI_SHORT_MAX << ")\n";
    }
    return -2;
  }
  value = (CORBA::Short)l_value;
  return 0;
}

int RDI_Config::get_value(RDIstrstream& str, const char* pname, CORBA::UShort& value, CORBA::Boolean log_type_errors) const
{
  char *delim=0, *strvl=0;

  if ( get_value(str, pname, strvl, 0) ) {
    return -1;
  }
  CORBA::ULong ul_value = RDI_STRTOUL(strvl, &delim);
  if ( (delim == 0) || (delim == strvl) || (*delim != '\0') ) {
    if (log_type_errors) {
      str << "Value for property " << pname << " invalid : Expected unsigned short integer value -- found " << strvl << '\n';
    }
    return -2;
  }
  if (ul_value > RDI_USHORT_MAX) {
    if (log_type_errors) {
      str << "Value for property " << pname << " overflow: Expected unsigned short integer value -- found " << strvl << '\n';
      str << "  (Max unsigned short = " << RDI_USHORT_MAX << ")\n";
    }
    return -2;
  }
  value = (CORBA::UShort)ul_value;
  return 0;
}

int RDI_Config::get_value(RDIstrstream& str, const char* pname, CORBA::Long& value, CORBA::Boolean log_type_errors) const
{
  char *delim=0, *strvl=0;

  if ( get_value(str, pname, strvl, 0) ) {
    return -1;
  }
  value = RDI_STRTOL(strvl, &delim);
  if ( (delim == 0) || (delim == strvl) || (*delim != '\0') ) {
    if (log_type_errors) {
      str << "Value for property " << pname << " invalid : Expected long integer value -- found " << strvl << '\n';
    }
    return -2;
  }
  return 0;
}

int RDI_Config::get_value(RDIstrstream& str, const char* pname, CORBA::ULong& value, CORBA::Boolean log_type_errors) const
{
  char *delim=0, *strvl=0;

  if ( get_value(str, pname, strvl, 0) ) {
    return -1;
  }
  value = RDI_STRTOUL(strvl, &delim);
  if ( (delim == 0) || (delim == strvl) || (*delim != '\0') ) {
    if (log_type_errors) {
      str << "Value for property " << pname << " invalid : Expected unsigned long integer value -- found " << strvl << '\n';
    }
    return -2;
  }
  return 0;
}

#ifndef NO_FLOAT
int RDI_Config::get_value(RDIstrstream& str, const char* pname, CORBA::Double& value, CORBA::Boolean log_type_errors) const
{
  char *delim=0, *strvl=0;

  if ( get_value(str, pname, strvl, 0) ) {
    return -1;
  }
  value = RDI_STRTOD(strvl, &delim);
  if ( (delim == 0) || (delim == strvl) || (*delim != '\0') ) {
    if (log_type_errors) {
      str << "Value for property " << pname << " invalid : Expected double value -- found " << strvl << '\n';
    }
    return -2;
  }
  return 0;
}
#endif

int RDI_Config::set_value(const char* pname, const char* value)
{
  unsigned int indx = 0;
  node_t*      node = 0;
  char*        temp = 0;

  if ( ! pname || (RDI_STRLEN(pname)==0) || ! value || (RDI_STRLEN(value)==0) ) {
    return -1;
  }

  indx = MyStringHash(pname) & HASH_MASK;
  node = _htbl[indx];

  while ( node && RDI_STR_NEQ(node->_name, pname) ) {
    node = node->_next;
  }
  if ( ! node ) {
    if (!(node=new node_t) || !(node->_name = new char[RDI_STRLEN(pname)+1])) {
      return -1;
    } 
    RDI_STRCPY(node->_name, pname);
    node->_next = _htbl[indx];
    _htbl[indx] = node;
  }
  if ( ! node->_value || RDI_STRLEN(node->_value) < RDI_STRLEN(value) ) {
    if ( ! (temp = new char [RDI_STRLEN(value) + 1]) ) {
      return -1;
    }
    if ( node->_value ) {
      delete [] node->_value;
    }
    node->_value = temp;;
  }
  RDI_STRCPY(node->_value, value);
  return 0;
}

int RDI_Config::env_update(const char* pname)
{
  char*   value = 0;
  node_t* cnode = 0;

  if ( pname ) {
    if ( (value = getenv(pname)) != (char *) 0 ) {
      if ( this->set_value(pname, value) ) {
	return -1;
      }
    }
    return 0;
  }

  // Go over all existing configuration parameters and updated their 
  // values with the corresponding environment values, if specified.

  for (unsigned int indx=0; indx < HASH_SIZE; indx++ ) {
    for (cnode = _htbl[indx]; cnode != (node_t *) 0; cnode = cnode->_next) {
      if ( (value = getenv(cnode->_name)) == (char *) 0 ) {
	continue;
      }
      if ( RDI_STRLEN(cnode->_value) < RDI_STRLEN(value) ) {
	char* temp = new char [ RDI_STRLEN(value) + 1 ];
	if ( ! temp ) {
	  return -1;
	}
	delete [] cnode->_value;
	cnode->_value = temp;
      }
      RDI_STRCPY(cnode->_value, value);
    }
  }

  return 0;
}

////////////////////////////////////////
// Logging

RDIstrstream& RDI_Config::log_output(RDIstrstream& str) const
{
  RDI_Config::node_t* cnode = 0;
  for (unsigned int indx=0; indx < RDI_Config::HASH_SIZE; indx++ ) {
    for ( cnode = _htbl[indx]; cnode; cnode = cnode->_next )
      str << cnode->_name << "\t\t" << cnode->_value << '\n';
  }
  return str;
}

