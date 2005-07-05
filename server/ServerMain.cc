// ServerMain.cc

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include <iostream>

using std::cout ;
using std::endl ;
using std::cerr ;

#include "Server.h"
#include "DODSGlobalIQ.h"
#include "DODSException.h"

const char *NameProgram;

extern int debug_server;

void show_usage()
{
  cout<<NameProgram<<": -d -v -p [PORT]"<<endl;
  cout<<"-d set the server to debugging mode"<<endl;
  cout<<"-v echos version and exit"<<endl;
  cout<<"-p set port to PORT"<<endl;
  exit(0);
}

void show_version()
{
  cout<<NameProgram<<" version 1.0"<<endl;
  exit(0);
}


int main(int argc, char *argv[])
{
    try
    {
	DODSGlobalIQ::DODSGlobalInit( argc, argv ) ;
    }
    catch( DODSException &e )
    {
	cerr << "Error initializing application" << endl ;
	cerr << "    " << e.get_error_description() << endl ;
	return 1 ;
    }

  int port = 0 ;
  int c = 0 ;
  bool got_port=false;
  NameProgram=argv[0];
  while ((c=getopt(argc,argv,"dvp:"))!=EOF)
    {
      switch (c)
	{
	case 'p':
	  port=atoi(optarg);
	  got_port=true;
	  break;
	case 'd':
	  debug_server=1;
	  break;
	case 'v':
	  show_version();
	  break;
	case '?':
	  show_usage();
	  break;
	}
    }
  if (got_port)
    {
      const Server *pserver=Server::instanceOf();
      pserver->start(port);
    }
  else
    show_usage();

    DODSGlobalIQ::DODSGlobalQuit() ;
}

