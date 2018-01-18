#ifndef __ORB_INIT_NAME__
#define __ORB_INIT_NAME__

#if defined(__OMNIORB3__)
#define DO_ORB_INIT(orbvarnm) \
  omniORB::maxTcpConnectionPerServer = 50; \
  CORBA::ORB_ptr orbvarnm = CORBA::ORB_init(argc, argv, "omniORB3")

#elif defined(__OMNIORB4__)
#define DO_ORB_INIT(orbvarnm) \
  const char* options[][2] = { { "maxGIOPConnectionPerServer", "50" }, { (const char*)0, (const char*)0 } }; \
  CORBA::ORB_ptr orbvarnm = CORBA::ORB_init(argc, argv, "omniORB4", options)

#elif defined(__MICO__)
#define DO_ORB_INIT(orbvarnm) \
  CORBA::ORB_ptr orbvarnm = CORBA::ORB_init(argc, argv, "mico-local-orb")

#else
#  error "One of the following must be defined : __OMNIORB3__, __OMNIORB4__, __MICO__"
stop_compiling
#endif

#endif
