// -*- C++ -*-

// (c) COPYRIGHT UCAR/HAO 1993-2002
// Please read the full copyright statement in the file COPYRIGHT.

#include <signal.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <poll.h>

#include <iostream>

using std::cout ;
using std::cerr ;
using std::endl ;

#include "PPTClient.h"
#include "PPTException.h"
#include "PPTSocket.h"


extern const char *NameProgram;

int PPTClient::__CONNECTION_OK=0;

PPTClient* PPTClient::singletonRef=0;

int PPTClient::__debugging=0;

int PPTClient::__timeout;


PPTSocket PPTClient::start_client (const char *host, int port, int debug, int timeout) const throw()
{
  __timeout=timeout;
  __debugging=debug;

  struct protoent *pProtoEnt;
  struct sockaddr_in sin;
  //struct servent *ps;
  struct hostent *ph;
  long address;
  if (isdigit(host[0]))
    {
      if((address=inet_addr(host))==-1)
	{
	  cerr<<NameProgram<<": invalid host ip address "<<host<<endl;
	  exit(1);
	}
      sin.sin_addr.s_addr=address;
      sin.sin_family=AF_INET;
    }
  else
    {
      if((ph=gethostbyname(host))==NULL)
	{
	  switch (h_errno)
	    {
	    case HOST_NOT_FOUND:
	      cerr<<NameProgram<<": no such host "<<host<<endl;
	      exit(1);
	    case TRY_AGAIN:
	      cerr<<NameProgram<<": host "<<host<< " try again later."<<endl;
	      exit(1);
	    case NO_RECOVERY:
	      cerr<<NameProgram<<": host "<<host<<"  DNS error."<<endl;
	      exit(1);
	    case NO_ADDRESS:
	      cerr<<NameProgram<<": no IP address for "<<host<<endl;
	      exit(1);
	    default:
	      cerr<<NameProgram<<": unknown error; "<<h_errno<<endl;
	      exit(1);
	    }
	}
      else
	{
	  sin.sin_family=ph->h_addrtype;
	  for(char**p=ph->h_addr_list;*p!=NULL;p++)
	    {
	      struct in_addr in;
	      (void)memcpy(&in.s_addr,*p,sizeof(in.s_addr));
	      memcpy((char*) &sin.sin_addr,(char*) &in, sizeof(in));
	    }
	  //bcopy(ph->h_addr,(char *)&sin.sin_addr,ph->h_length);
	}
    }	
  sin.sin_port=port;
  pProtoEnt=getprotobyname("tcp");
  int SocketDescriptor=socket (AF_INET,SOCK_STREAM,pProtoEnt->p_proto);
  if (SocketDescriptor!=-1)
    {
      if (__debugging) cout<<"pptclient: Trying to get socket connected...";
      if (connect(SocketDescriptor,(struct sockaddr*)&sin, sizeof (sin))!=1)
	{
	  if (__debugging) cout<<"OK got socket connected to "<<host<<" = "<<inet_ntoa(sin.sin_addr)<<endl;
	  PPTSocket pptsocket(SocketDescriptor);

	  return pptsocket;
	}
    }
    return PPTSocket( -1 ) ;
}

