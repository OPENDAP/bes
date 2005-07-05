// -*- C++ -*-

// (c) COPYRIGHT UCAR/HAO 1993-2002
// Please read the full copyright statement in the file COPYRIGHT.

#include "PPTServer.h"

#include <iostream>

using std::cout ;
using std::endl ;

extern const char* NameProgram;

PPTServer *PPTServer::singletonRef=0;

PPTServer::PPTServer() throw()
{

}



PPTSocket PPTServer::start(int port, const char* s) const throw()
{
  PPTServerListener listener;
  try
    {
      listener.startListening(port, s);
    }
  catch(PPTException &e)
    {
      cout<<__FILE__<<":"<<__LINE__<<" got an exception"<<endl;
      cout<<e.getErrorFile()<<":"<<e.getErrorLine()<<" "<<e.getErrorDescription()<<endl;
      cout<<"Process must exit because it can not start listening!"<<endl;
      exit(1);
    }


  try
      {
	  PPTSocket pptsocket=listener.acceptConnections();
	  return pptsocket;
	}
      catch(PPTException &e)
	{
	  cout<<__FILE__<<":"<<__LINE__<<" got an exception"<<endl;
	  cout<<e.getErrorFile()<<":"<<e.getErrorLine()<<" "<<e.getErrorDescription()<<endl;
	  cout<<"server process halt!"<<endl;
	  exit(1);
	}
      // This kind of unknown exception may leave the server in an unstable state better leave now
      catch(...)
	{
	  cout<<__FILE__<<":"<<__LINE__<<" got an exception"<<endl;
	  cout<<NameProgram<<"Undefined exception, must exit!"<<endl;
	  exit(1);
	}
}



	     


