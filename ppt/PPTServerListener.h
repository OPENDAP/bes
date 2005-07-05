// -*- C++ -*-

// (c) COPYRIGHT UCAR/HAO 1993-2002
// Please read the full copyright statement in the file COPYRIGHT.

#ifndef PPTServerListener_h_
#define PPTServerListener_h_ 1


// forward declaration
class PPTSocket;

/**
 */

class PPTServerListener
{
  ///
  int _socket;
  ///
  int _unix_socket;
  ///
  string _unix_socket_name;
  ///
  void initializeTCPSocket(int);
  ///
  void initializeUnixSocket();
public:
  ///
  PPTServerListener();
  ///
  ~PPTServerListener();
  ///
  void startListening(int, const char*);
  ///
  PPTSocket acceptConnections();
};

#endif // PPTServerListener
