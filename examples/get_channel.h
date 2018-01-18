// -*- Mode: C++; -*-

#ifndef _GET_CHANNEL_H_
#define _GET_CHANNEL_H_

#include <stdio.h>
#include "CosNotifyShorthands.h"

// SAMPLE code to get a channel reference either from the
// naming service or from a stringified IOR stored in a file

// These routines return nil reference if they fail to get valid channel reference

CosNA::EventChannel_ptr getchan_from_iorfile(CORBA::ORB_ptr orb,
					     const char* ior_file,
					     CORBA::Boolean verbose) {
  char buf[512];
  FILE* ifile;
  CosNA::EventChannel_ptr channel = CosNA::EventChannel::_nil();
  if (!ior_file || strlen(ior_file) == 0) return channel; // empty string -- ignore
  if (! (ifile = fopen(ior_file, "r")) ) {
    cerr << "Failed to open file " << ior_file << " for reading" << endl;
    return channel; // failure
  }
  if (fscanf(ifile, "%s", buf) != 1) {
    cerr << "Failed to get an IOR from file " << ior_file << endl;
    fclose(ifile);
    return channel; // failure
  }
  fclose(ifile);
  try {
    CORBA::Object_var channel_ref = orb->string_to_object(buf);
    if ( CORBA::is_nil(channel_ref) ) {
      cerr << "Failed to turn IOR in file " << ior_file << " into object" << endl;
      return channel; // failure
    }
    channel = CosNA::EventChannel::_narrow(channel_ref);
    if ( CORBA::is_nil(channel) ) {
      cerr << "Failed to narrow object from IOR in file " << ior_file <<
	" to type CosNotifyChannelAdmin::EventChannel" << endl;
      return channel; // failure
    }
  } catch (...) {
    cerr << "Failed to convert IOR in file " << ior_file << " to object" << endl;
    return channel; // failure
  }
  if (verbose)
    cout << "Found valid channel reference" << endl;
  return channel; // success
}

CosNA::EventChannel_ptr getchan_from_ns(CORBA::ORB_ptr orb,
					const char* channel_name,
					CORBA::Boolean verbose) {
  CosNA::EventChannel_ptr channel = CosNA::EventChannel::_nil();
  CosNaming::NamingContext_var name_context;
  CosNaming::Name name;

  if (!channel_name || strlen(channel_name) == 0) return channel; // empty string -- ignore

  if (verbose)
    cout << "Obtaining naming service reference" << endl;
  try {
    CORBA::Object_var name_service;
    name_service = orb->resolve_initial_references("NameService"); 
    name_context = CosNaming::NamingContext::_narrow(name_service);
    if ( CORBA::is_nil(name_context) ) {
      cerr << "Failed to obtain context for NameService" << endl;
      return channel; // failure
    } 
  }
  catch(CORBA::ORB::InvalidName& ex) {
    cerr << "Service required is invalid [does not exist]" << endl;
    return channel; // failure
  } catch (CORBA::COMM_FAILURE& ex) {
    cerr << "Caught system exception COMM_FAILURE" << endl;
    return channel; // failure
  } catch (...) {
    cerr << "Caught exception while resolving the naming service" << endl;
    return channel; // failure
  }

  if (verbose)
    cout << "Looking up channel name " << channel_name << " . " << channel_name << endl;

  name.length(1);
  name[0].id   = CORBA::string_dup((const char*)channel_name);
  name[0].kind = CORBA::string_dup((const char*)channel_name);

  try {
    CORBA::Object_var channel_ref = name_context->resolve(name);
    channel = CosNA::EventChannel::_narrow(channel_ref);
    if ( CORBA::is_nil(channel) ) {
      cerr << "Failed to narrow object found in naming service " <<
	" to type CosNotifyChannelAdmin::EventChannel" << endl;
      return channel; // failure
    }
  }
  catch(CORBA::ORB::InvalidName& ex) {
    cerr << "Invalid name" << endl;
    return channel; // failure
  } catch (CORBA::COMM_FAILURE& ex) {
    cerr << "Caught system exception COMM_FAILURE while resolving event channel name" << endl;
    return channel; // failure
  } catch (...) {
    cerr << "Caught exception while resolving event channel name" << endl;
    return channel; // failure
  }
  if (verbose)
    cout << "Found valid channel reference" << endl;
  return channel; // success
}

CosNA::EventChannel_ptr getchanid_from_ns(CORBA::ORB_ptr orb,
					  CosNA::ChannelID chan_id,
					  const char* factory_name,
					  CORBA::Boolean verbose) {
  CosNA::EventChannelFactory_ptr factory = CosNA::EventChannelFactory::_nil();
  CosNA::EventChannel_ptr channel = CosNA::EventChannel::_nil();
  CosNaming::NamingContext_var name_context;
  CosNaming::Name name;

  if (verbose)
    cout << "Obtaining naming service reference" << endl;
  if (!factory_name || strlen(factory_name) == 0) return channel; // empty string -- ignore
  try {
    CORBA::Object_var name_service;
    name_service = orb->resolve_initial_references("NameService"); 
    name_context = CosNaming::NamingContext::_narrow(name_service);
    if ( CORBA::is_nil(name_context) ) {
      cerr << "Failed to obtain context for NameService" << endl;
      return channel; // failure
    } 
  }
  catch(CORBA::ORB::InvalidName& ex) {
    cerr << "Service required is invalid [does not exist]" << endl;
    return channel; // failure
  } catch (CORBA::COMM_FAILURE& ex) {
    cerr << "Caught system exception COMM_FAILURE" << endl;
    return channel; // failure
  } catch (...) {
    cerr << "Caught exception while resolving the naming service" << endl;
    return channel; // failure
  }

  if (verbose)
    cout << "Looking up channel factory name " << factory_name << " . " << factory_name << endl;

  name.length(1);
  name[0].id   = CORBA::string_dup((const char*)factory_name);
  name[0].kind = CORBA::string_dup((const char*)factory_name);

  try {
    CORBA::Object_var factory_ref = name_context->resolve(name);
    factory = CosNA::EventChannelFactory::_narrow(factory_ref);
    if ( CORBA::is_nil(factory) ) {
      cerr << "Failed to narrow object found in naming service " <<
	" to type CosNotifyChannelAdmin::EventChannelFactory" << endl;
      return channel; // failure
    }
  }
  catch(CORBA::ORB::InvalidName& ex) {
    cerr << "Invalid name" << endl;
    return channel; // failure
  } catch (CORBA::COMM_FAILURE& ex) {
    cerr << "Caught system exception COMM_FAILURE while resolving event channel factory name" << endl;
    return channel; // failure
  } catch (...) {
    cerr << "Caught exception while resolving event channel factory name" << endl;
    return channel; // failure
  }
  if (verbose) {
    cout << "Found valid channel factory reference" << endl;
    cout << "Looking up channel id " << chan_id << endl;
  }

  try {
    channel = factory->get_event_channel( chan_id );
    if ( CORBA::is_nil(channel) ) {
      cerr << "Failed to find channel id " << chan_id << endl;
      return channel; // failure
    }
  } catch (CORBA::COMM_FAILURE& ex) {
    cerr << "Caught system exception COMM_FAILURE while invoking get_event_channel" << endl;
    return channel; // failure
  } catch (...) {
    cerr << "Caught exception while invoking get_event_channel" << endl;
    return channel; // failure
  }
  if (verbose)
    cout << "Found valid channel reference" << endl;
  return channel; // success
}

CosNA::EventChannel_ptr getnewchan_use_ns(CORBA::ORB_ptr orb,
					  const char* factory_name,
					  CORBA::Boolean verbose) {
  CosNA::EventChannelFactory_ptr factory = CosNA::EventChannelFactory::_nil();
  CosNA::EventChannel_ptr channel = CosNA::EventChannel::_nil();
  CosNaming::NamingContext_var name_context;
  CosNaming::Name name;

  if (verbose)
    cout << "Obtaining naming service reference" << endl;
  if (!factory_name || strlen(factory_name) == 0) return channel; // empty string -- ignore
  try {
    CORBA::Object_var name_service;
    name_service = orb->resolve_initial_references("NameService"); 
    name_context = CosNaming::NamingContext::_narrow(name_service);
    if ( CORBA::is_nil(name_context) ) {
      cerr << "Failed to obtain context for NameService" << endl;
      return channel; // failure
    } 
  }
  catch(CORBA::ORB::InvalidName& ex) {
    cerr << "Service required is invalid [does not exist]" << endl;
    return channel; // failure
  } catch (CORBA::COMM_FAILURE& ex) {
    cerr << "Caught system exception COMM_FAILURE" << endl;
    return channel; // failure
  } catch (...) {
    cerr << "Caught exception while resolving the naming service" << endl;
    return channel; // failure
  }

  if (verbose)
    cout << "Looking up channel factory name " << factory_name << " . " << factory_name << endl;

  name.length(1);
  name[0].id   = CORBA::string_dup((const char*)factory_name);
  name[0].kind = CORBA::string_dup((const char*)factory_name);

  try {
    CORBA::Object_var factory_ref = name_context->resolve(name);
    factory = CosNA::EventChannelFactory::_narrow(factory_ref);
    if ( CORBA::is_nil(factory) ) {
      cerr << "Failed to narrow object found in naming service " <<
	" to type CosNotifyChannelAdmin::EventChannelFactory" << endl;
      return channel; // failure
    }
  }
  catch(CORBA::ORB::InvalidName& ex) {
    cerr << "Invalid name" << endl;
    return channel; // failure
  } catch (CORBA::COMM_FAILURE& ex) {
    cerr << "Caught system exception COMM_FAILURE while resolving event channel factory name" << endl;
    return channel; // failure
  } catch (...) {
    cerr << "Caught exception while resolving event channel factory name" << endl;
    return channel; // failure
  }
  if (verbose) {
    cout << "Found valid channel factory reference" << endl;
    cout << "Creating new channel" << endl;
  }

  CosNA::ChannelID         chan_id;
  try {
    CosN::QoSProperties      qosP;
    CosN::AdminProperties    admP;

    qosP.length(0);
    admP.length(0);

    channel = factory->create_channel(qosP, admP, chan_id);
    if ( CORBA::is_nil(channel) ) {
      cerr << "Failed to create new channel" << endl;
      return channel; // failure
    }
  } catch (CORBA::COMM_FAILURE& ex) {
    cerr << "Caught system exception COMM_FAILURE while invoking create_channel" << endl;
    return channel; // failure
  } catch (...) {
    cerr << "Caught exception while invoking create_channel" << endl;
    return channel; // failure
  }
  if (verbose)
    cout << "Created a new channel with channel ID " << chan_id << endl;
  return channel; // success
}

CosNA::EventChannel_ptr getnewchan_use_iorfile(CORBA::ORB_ptr orb,
					       const char* ior_file,
					       CORBA::Boolean verbose) {
  char buf[512];
  FILE* ifile;
  CosNA::EventChannelFactory_ptr factory = CosNA::EventChannelFactory::_nil();
  CosNA::EventChannel_ptr channel = CosNA::EventChannel::_nil();

  if (!ior_file || strlen(ior_file) == 0) return channel; // empty string -- ignore
  if (! (ifile = fopen(ior_file, "r")) ) {
    cerr << "Failed to open file " << ior_file << " for reading" << endl;
    return channel; // failure
  }
  if (fscanf(ifile, "%s", buf) != 1) {
    cerr << "Failed to get an IOR from file " << ior_file << endl;
    fclose(ifile);
    return channel; // failure
  }
  fclose(ifile);
  try {
    CORBA::Object_var fact_ref = orb->string_to_object(buf);
    if ( CORBA::is_nil(fact_ref) ) {
      cerr << "Failed to turn IOR in file " << ior_file << " into object" << endl;
      return channel; // failure
    }
    factory = CosNA::EventChannelFactory::_narrow(fact_ref);
    if ( CORBA::is_nil(factory) ) {
      cerr << "Failed to narrow object from IOR in file " << ior_file <<
	" to type CosNotifyChannelAdmin::EventChannelFactory" << endl;
      return channel; // failure
    }
  } catch (...) {
    cerr << "Failed to convert IOR in file " << ior_file << " to object" << endl;
    return channel; // failure
  }
  if (verbose) {
    cout << "Found valid channel factory reference" << endl;
    cout << "Creating new channel" << endl;
  }

  CosNA::ChannelID         chan_id;
  try {
    CosN::QoSProperties      qosP;
    CosN::AdminProperties    admP;

    qosP.length(0);
    admP.length(0);

    channel = factory->create_channel(qosP, admP, chan_id);
    if ( CORBA::is_nil(channel) ) {
      cerr << "Failed to create new channel" << endl;
      return channel; // failure
    }
  } catch (CORBA::COMM_FAILURE& ex) {
    cerr << "Caught system exception COMM_FAILURE while invoking create_channel" << endl;
    return channel; // failure
  } catch (...) {
    cerr << "Caught exception while invoking create_channel" << endl;
    return channel; // failure
  }
  if (verbose)
    cout << "Created a new channel with channel ID " << chan_id << endl;
  return channel; // success
}

#endif

