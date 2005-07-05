// -*- C++ -*-

// (c) COPYRIGHT UCAR/HAO 1993-2002
// Please read the full copyright statement in the file COPYRIGHT.

#ifndef PPTServer_h_
#define PPTServer_h_ 1

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include <sys/wait.h>

#include "PPTSocket.h"
#include "PPTException.h"
#include "PPTServerListener.h"

#include "PPTUtilities.h"

/**
 */

class PPTServer
{
  ///
  static PPTServer *singletonRef;
  ///
  PPTServer() throw();
  ///
  PPTServer(const PPTServer&){};
  ///
  void operator delete(void*){};
  ///
  static void signal_terminate(int sig) throw();
  ///
  static void signal_restart(int sig) throw();
  
public:

  ///
  static const PPTServer* instanceOf() throw()
  {
    if (!singletonRef) 
      singletonRef = new PPTServer();
    
    return singletonRef;
  }
  ///
  PPTSocket start (int port, const char*) const throw();
  
};

#endif // PPTServer_h_
