#ifndef Client_h_
#define Client_h_ 1


#include <iostream>
#include <string>

//forward declaration
class PPTSocket;
class PPTBuffer;

#include "DODSClient.h"


/**
 */

class Client
{
  static Client *singletonRef;
  
  static DODSClient* _dodsclient;

  ///
  int get_rex_buffer(string &)const ;
  ///
  void register_signals() const;
  ///
  static void signal_can_not_connect(int);
  ///
  static void signal_terminate(int);
  ///
  static void signal_broken_pipe(int sig);
  ///
  void take_server_control () const;
  ///
  Client() throw(){};
  ///
  Client(const Client&) throw(){};
  ///
  ~Client(){};
  ///
  void operator delete(void*)throw(){};

public:
  ///
  static const Client* instanceOf() throw()
  {
    if (!singletonRef) 
      singletonRef = new Client();
    
    return singletonRef;
  }

  ///
  bool start_client(char*host, int port, int debug, int timeout=5) const throw();
  ///
  bool start_client_at_unix_socket(char*unix_socket, int debug, int timeout=5) const throw();
  ///
  void execute_and_quit(const string &, const string &) const throw();
  ///
  void execute_input_file_and_quit(const string &, const string &) const throw();
  ///
  void interact() const throw();

};

#endif // Client_h_
