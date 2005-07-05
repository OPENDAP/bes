// -*- C++ -*-

// (c) COPYRIGHT UCAR/HAO 1993-2002
// Please read the full copyright statement in the file COPYRIGHT.

#ifndef PPTClient_h_
#define PPTClient_h_ 1

//forward declaration
class PPTSocket;

/**
 */

class PPTClient
{
  ///
  static int __timeout;
  ///
  static int __CONNECTION_OK;
  ///
  static PPTClient *singletonRef;
  ///
  static int __debugging;
  
 
  PPTClient() throw(){};
  ///
  PPTClient(const PPTClient&)throw(){};
  ///
  void operator delete(void*)throw(){};

public:
  
  ///
  static const PPTClient* instanceOf() throw()
  {
    if (!singletonRef) 
      singletonRef = new PPTClient();
    
    return singletonRef;
  }

  ///
  PPTSocket start_client(const char *host, int port, int debug, int timeout=5) const throw();
  

};

#endif // PPTClient_h_
