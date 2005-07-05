// -*- C++ -*-

// (c) COPYRIGHT UCAR/HAO 1993-2003
// Please read the full copyright statement in the file COPYRIGHT.

#ifndef OPeNDAPConnector_h_
#define OPeNDAPConnector_h_ 1

/*
  This wrapper is necessary because trying to include 
  DODS.h directly from Apache cause a conflict since DODS.h
  uses the header file <string> which then calls: 
  $(HOME_OF_GCC)/lib/gcc-lib//sparc-sun-solaris2.5.1/2.95.1/../../../../include/g++-3/std/bastring.h
  which in line 38 has the statement:
  #include <alloc.h>
  which really is:
  $(HOME_OF_GCC)/lib/gcc-lib/sparc-sun-solaris2.5.1/2.95.1/../../../../include/g++-3/alloc.h
  which is in conflict which 
  $(APACHE_HOME)/src/include/alloc.h
*/

#include "DODSStatusReturn.h"

class OPeNDAPConnector
{
  char* _data_request;
public:
  ///
  OPeNDAPConnector();
  ///
  ~OPeNDAPConnector();
  ///
  int execute(const  DODSDataRequestInterface & re);
  ///
  const char * process_request(const char*s);
};

#endif // OPeNDAPConnector_h_
