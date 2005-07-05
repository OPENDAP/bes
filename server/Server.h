#ifndef Server_h_
#define Server_h_ 1

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include <sys/wait.h>

#include <string>

//#include <algo.h>

// forward declaration
class PPTSocket;
class PPTBuffer;

/**
 */

class DODSMemoryGlobalArea;

class Server
{
  /// 
  static DODSMemoryGlobalArea *_m; 
  ///
  std::string  _method;
  ///
  static Server *singletonRef;
  ///
  Server() throw();
  ///
  Server(const Server&){};
    ///
    ~Server ();
  ///
  void operator delete(void*){};
  ///
  int welcome_new_client (PPTSocket) const;
  ///
  void fork_call_handler(PPTSocket) const;
  ///
  void entry_in_child(PPTSocket) const;
  ///
  static void signal_terminate(int sig) throw();
  ///
  static void signal_restart(int sig) throw();
  ///
  void method();
  ///
  bool client_complete_transmition(PPTBuffer *) const;

public:

  ///
  static const Server* instanceOf() throw()
  {
    if (!singletonRef) 
      singletonRef = new Server();
    
    return singletonRef;
  }
  ///
  void start (int port) const throw();
  
};

#endif // Server_h_
