// -*- Mode: C++; -*-
//                              File      : RDIConfig.h
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
 
#ifndef _RDI_CONFIG_H_
#define _RDI_CONFIG_H_

#include "corba_wrappers.h"
#include "RDIstrstream.h"

// --------------------------------------------------------------- //
// Generic class for parsing command line arguments and/or reading //
// configuration parameters from files.  The format of the command //
// line arguments is: -Dparam_name=param_value -- no spaces should //
// be used.  The configuration file  contains entries of the form: //
// param_name param_value. Tabs and spaces can be used to separate //
// names from values and lines starting with '#' are considered to //
// be comments and are ignored.  Finally, calling                  //
//   env_update(pname) will override any current value for pname   //
// with an env variable setting (if set in the user's environment) //
// and calling env_update() will scan all of the param names that  //
// have been set and override any name that has an env variable    //
// setting.                                                        //
//                                                                 //
// NOTE: using -DCONFIGFILE=name as a command line argument forces //
//       the loading of configuration parameters from file 'name'. //
//       Parameters that have been set already, will be updated by //
//       using the values found in the file, if any.               //
// --------------------------------------------------------------- //

class RDI_Config {
public:
  RDI_Config();
  ~RDI_Config();

  // Parse command line arguments and extract configuration
  // parameters. If 'rm_args' is set, 'argc' and 'argv' are
  // updated to reflect the removal of these parameters.
  // Returns 1 on error, 0 if no errors.

  CORBA::Boolean parse_arguments(RDIstrstream& str, int& argc, char** argv, CORBA::Boolean rm_args);

  // Import/Export configuration parameters from/to a file
  //   import_settings returns 1 on error, 0 if no errors.
  CORBA::Boolean import_settings(RDIstrstream& str, const char* fname);
  int export_settings(const char* fname=0, const char* header=0) const;

  // Retrieve the value of a configuration parameter. Returns:
  //    0 : found it
  //   -1 : not found
  //   -2 : found, but wrong type 
  int get_value(RDIstrstream& str, const char* pname, char*&  value, CORBA::Boolean log_type_errors) const;
  int get_value(RDIstrstream& str, const char* pname, CORBA::Boolean& value, CORBA::Boolean log_type_errors) const;
  int get_value(RDIstrstream& str, const char* pname, CORBA::Short& value, CORBA::Boolean log_type_errors) const;
  int get_value(RDIstrstream& str, const char* pname, CORBA::UShort& value, CORBA::Boolean log_type_errors) const;
  int get_value(RDIstrstream& str, const char* pname, CORBA::Long& value, CORBA::Boolean log_type_errors) const;
  int get_value(RDIstrstream& str, const char* pname, CORBA::ULong& value, CORBA::Boolean log_type_errors) const;
#ifndef NO_FLOAT
  int get_value(RDIstrstream& str, const char* pname, CORBA::Double& value, CORBA::Boolean log_type_errors) const;
#endif

  // Change the value of/create a configuration parameter.
  // In case of an error, -1 is returned

  int set_value(const char* pname, const char* value);

  // Change the value of a configuration parameter by using
  // the corresponding environment variable, if defined. If
  // no name is given, update the existing parameters using
  // their corresponding environment variables, if defined.

  int env_update(const char* pname=0);

  RDIstrstream& log_output(RDIstrstream& str) const;
private:
  enum { HASH_SIZE = 32, HASH_MASK = HASH_SIZE - 1 };
  struct node_t {
    char *_name, *_value; node_t* _next;
    node_t() : _name(0), _value(0), _next(0) {;}
    ~node_t()  { if ( _name )  delete [] _name;  _name  = 0;
    if ( _value ) delete [] _value; _value = 0; }
  };
  node_t* _htbl[ HASH_SIZE ];
};

////////////////////////////////////////
// Logging

inline RDIstrstream& operator<< (RDIstrstream& str, const RDI_Config& cfg) { return cfg.log_output(str); }

#endif
