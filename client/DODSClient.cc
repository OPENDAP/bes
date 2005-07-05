#include <signal.h>
#include <netdb.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <poll.h>
#include <sys/un.h> // For Unix sockets

#include <string>
#include <iostream>
#include <fstream>

using std::string ;
using std::cerr ;
using std::cout ;
using std::endl ;
using std::flush ;

#include "DODSClient.h"

#include "PPTClientServerSessionProtocol.h"
#include "PPTException.h"
#include "PPTSocket.h"
#include "PPTUtilities.h"


// -- C++ --

extern const char *NameProgram;
extern "C" {int errno;}

void
DODSClient::signal_broken_pipe(int sig)
{
    if( sig == SIGPIPE )
    {
      cerr<< __FILE__<<":"<<__LINE__<<": got a broken pipe signal"<<endl;
    }
    if(_previous_handler)
      {
	if(  signal( SIGPIPE, _previous_handler) == SIG_ERR )
	  {
	    cerr<<__FILE__<<":"<<__LINE__
		<<"Fatal: can not obtain previous disposition for handling broken pipe signals!" << endl ;
	    // OK this is really bad, I can not leave the program 
	    // without a suitable handler to manage broken pipes signals
	    // must exit, total fatal error.
	    // BTW, if you get here, do not panic because this library is doing exit calls for you!
	    // It should never happen in normal processes that
	    // signals fail to register so this
	    // type of cases are reason enough not to release this code anyway...
	    // and debug why you ever got here.
	    exit(1);
	  }
	else
	    cerr<<__FILE__<<":"<<__LINE__<<": succesfully re-register previous brokwn pipe handler"<<endl;
      }
    else
      {
	cerr<<__FILE__<<":"<<__LINE__
	    <<"Where is the previous signal handler I am supposed to have???"<<endl;
	exit(1);
      }
    
}

void (*DODSClient::_previous_handler)(int)=0 ;


DODSClient::DODSClient (const char* host, int port, int debug, int timeout) throw()
{
  try
    {
      __timeout=timeout;
      __debugging=debug;
      __connected=false;
      __broken_pipe=false;
      
      struct protoent *pProtoEnt;
      struct sockaddr_in sin;
      struct hostent *ph;
      long address;
      if (isdigit(host[0]))
	{
	  if((address=inet_addr(host))==-1)
	    {
	      cerr<<__FILE__<<":"<<__LINE__<<": invalid host ip address "<<host<<endl;
	      exit(1);
	    }
	  sin.sin_addr.s_addr=address;
	  sin.sin_family=AF_INET;
	}
      else
	{
	  if((ph=gethostbyname(host))==NULL)
	    {
	      extern int h_error;
	      switch (h_errno)
		{
		case HOST_NOT_FOUND:
		  cerr<<__FILE__<<":"<<__LINE__<<": no such host "<<host<<endl;
		  exit(1);
		case TRY_AGAIN:
		  cerr<<__FILE__<<":"<<__LINE__<<": host "<<host<< " try again later."<<endl;
		  exit(1);
		case NO_RECOVERY:
		  cerr<<__FILE__<<":"<<__LINE__<<": host "<<host<<"  DNS error."<<endl;
		  exit(1);
		case NO_ADDRESS:
		  cerr<<__FILE__<<":"<<__LINE__<<": no IP address for "<<host<<endl;
		  exit(1);
		default:
		  cerr<<__FILE__<<":"<<__LINE__<<": unknown error; "<<h_errno<<endl;
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
	    }
	}	
      sin.sin_port=htons( port ) ;
      pProtoEnt=getprotobyname("tcp");
      int SocketDescriptor=socket (AF_INET,SOCK_STREAM,pProtoEnt->p_proto);
      if (SocketDescriptor!=-1)
	{
	  if (__debugging) cout<<__FILE__<<":"<<__LINE__<<": Trying to get socket connected...";
	  if (connect(SocketDescriptor,(struct sockaddr*)&sin, sizeof (sin))!=1)
	    {
	      if (__debugging) cout<<"OK got socket connected to "<<host<<" = "<<inet_ntoa(sin.sin_addr)<<endl;
	      __pptsocket = PPTSocket(SocketDescriptor);
	      __pptsocket._type=PPT_TCP_SOCKET;
	      
	      //very critical stage, we in inside the constructor, thus we can not get a brokwn pipe signal right now...
	      _previous_handler=signal( SIGPIPE, signal_broken_pipe );
	      if(  _previous_handler == SIG_ERR )
		{
		  cerr<<__FILE__
		  <<":"<<__LINE__
		  <<"Fatal: can not obtain previous disposition for handling broken pipe signals!" << endl ;
		}
	      init_connection();
	      // did we survive? if so re-register previous handler...
	      if(_previous_handler)
		{
		  if(  signal( SIGPIPE, _previous_handler) == SIG_ERR )
		    {
		      cerr<<__FILE__<<":"<<__LINE__
			  <<"Fatal: can not obtain previous disposition for handling broken pipe signals!" << endl ;
		      // OK this is really bad, I can not leave the program 
		      // without a suitable handler to manage broken pipes signals
		      // must exit, total fatal error.
		      // BTW, if you get here, do not panic because this library is doing exit calls for you!
		      // It should never happen in normal processes that
		      // signals fail to register so this
		      // type of cases are reason enough not to release this code anyway...
		      // and debug why you ever got here.
		      exit(1);
		    }
		  else
		    if (__debugging)
		      cout<<__FILE__<<":"<<__LINE__<<": succesfully re-register previous brokwn pipe handler"<<endl;
		}
	    }
	}
    }
  catch(PPTException &e)
    {
      cerr<<__FILE__<<":"<<__LINE__<<": managin exception type PPTException"<<endl;
      cerr<<e.getErrorFile()<<":"<<e.getErrorLine()<<" "<<e.getErrorDescription()<<endl;
    }
  
  catch (...)
    {
      cout<<__FILE__<<" "<<__LINE__<<" Got undefined exception!"<<endl;
    }
}
DODSClient::DODSClient (const char *unix_socket, int debug, int timeout) throw()
{
    try
	{
	    __timeout=timeout;
	    __debugging=debug;
	     __connected=false;
	     __broken_pipe=false;
	     __pptsocket._type=PPT_UNIX_SOCKET;
	    
	     char path[500]="";
	     getcwd(path,sizeof (path));
	     tem=path;
	     tem+="/";
	     tem+=create_temp_name();
	     tem+=".unix_socket";
	     // maximum path for struct sockaddr_un.sun_path is 108
	     // get sure we will not exceed to max for creating sockets
	     if (__debugging) cout<<NameProgram<<": creating client unix socket@"<<tem<<endl;
	     if(tem.length()>108)
	       {
		 cerr<<__FILE__<<":"<<__LINE__<<": path generated is too long"<<endl;
		 exit(1);
	       }
	     
	    struct sockaddr_un client_addr;
	    struct sockaddr_un server_addr;

	    strcpy(server_addr.sun_path, unix_socket);
	    server_addr.sun_family=AF_UNIX;
	    
	    int SocketDescriptor=socket (AF_UNIX,SOCK_STREAM,0);
	    if (SocketDescriptor!=-1)
		{
		  strcpy(client_addr.sun_path,tem.c_str());
		  client_addr.sun_family=AF_UNIX;
	    		  
		  if (__debugging) cout<<NameProgram<<": Trying to get socket binded...";
		  if ( bind(SocketDescriptor,(struct sockaddr*)&client_addr, sizeof(client_addr.sun_family) + strlen(client_addr.sun_path))!= -1)
		    {
		      if (__debugging) cout<<"OK got socket binded"<<endl;
		      if (__debugging) cout<<NameProgram<<": Trying to get socket connected..."<<flush;
		      if (connect(SocketDescriptor,(struct sockaddr*)&server_addr, sizeof(server_addr.sun_family) + strlen(server_addr.sun_path))!=-1)
			{
			  if (__debugging) cout<<"OK got Unix socket connected"<<endl;
			  __pptsocket = PPTSocket(SocketDescriptor);
			  __pptsocket._type=PPT_UNIX_SOCKET;
			  init_connection();
			}
		      else
			{
			  cerr<<"could not connected via "<<unix_socket<<endl;
			  perror("connect");
			}
		    }
		  else
		    {
		      cerr<<NameProgram<<": could not get Unix socket binded!"<<endl;
		      perror("bind");
		    }
		}
	    else
	      {
		cerr<<NameProgram<<": could not get Unix socket created!"<<endl;
		perror("socket");
	      }
	} 
    catch(PPTException &e)
	{
	    cout<<e.getErrorFile()<<":"<<e.getErrorLine()<<" "<<e.getErrorDescription()<<endl;
	    cout<<"client process halt!"<<endl;
	    exit(1); 
	}
   
    catch (...)
	{
	    cout<<__FILE__<<" "<<__LINE__<<" Got undefined exception!"<<endl;
	}
}

DODSClient::~DODSClient()
{
  if(__debugging)
    cout<<__FILE__<<":"<<__LINE__<<": destroying client object"<<endl;
  close_connection();
  clean_state();
  
}

void DODSClient::init_connection()
{
  string status= PPT_PROTOCOL_UNDEFINED;

  if (__debugging) cout<<__FILE__<<":"<<__LINE__<<": trying to get server recognition..."<<flush;

  
  PPTBuffer buf (4096);
  strcpy((char*)buf.data,PPTCLIENT_TESTING_CONNECTION);
  buf.valid=strlen(PPTCLIENT_TESTING_CONNECTION);
  __pptsocket<<buf;
  

  // A call like "__pptsocket>>status;" at this level  will block
  // unless the server responds right away so -> PANIC!!!! 
  // I have to write a method for non System V aka non Solaris
  // Alpha OSF1, INTELx86 Linux

  // September 23: NO PANIC, here we go...

  //extern int errno;
  struct pollfd p;
  p.fd=__pptsocket.getSocketDescriptor();
  p.events=POLLIN;
  struct pollfd arr[1];
  arr[0]=p;
  // Lets loop 6 times with a delay block on poll of 1000 milliseconds
  // (duh! 6 seconds) and see if there is any data.
  for (int j=0; j<__timeout;j++)
    {
      if(poll(arr, 1, 1000)<0)
	{
	  string error("poll error");
	  const char* error_info=strerror(errno);
	  if (error_info)
	    error+=" "+(string)error_info;
	  throw PPTException(__FILE__,__LINE__,error);
	}
      else
	{
	  if (arr[0].revents==POLLIN)
	    {
		PPTBuffer *buf1=new PPTBuffer(4096);
		__pptsocket>>(*buf1);
		char proto[buf1->valid+1];
		memcpy(proto,buf1->data,buf1->valid);
		proto[buf1->valid]='\0';
		status=string(proto);
		delete buf1;
		buf1=0;
		// Got a message, lets get out of here
		break;
	    }
	  else
	    cout<<" "<<j<<flush;
	}
    }
  if (status==PPT_PROTOCOL_UNDEFINED)
    {
      cout<<" sorry, could not connect. Server may be down or busy."<<endl;
    }
  else if(status!=PPTSERVER_CONNECTION_OK)
    {
      if (__debugging) cout<<"Server reported invalid connection string (got "<<status<<")"<<endl;
      
    }
  else
    {
      if (__debugging) cout<<__FILE__<<":"<<__LINE__<<": server authentication acknowledged. ("<<status<<")"<<endl;
      __connected=true;
    }

}

void DODSClient::close_connection()
{
  if(__connected && !__broken_pipe)
    {
      if(__debugging)
	cout<<__FILE__<<":"<<__LINE__<<": sending server PPT exit token..."<<flush; 
      PPTBuffer b(4096);
      strcpy((char*)b.data,PPTCLIENT_EXIT_NOW);
      b.valid=strlen(PPTCLIENT_EXIT_NOW);
      __pptsocket<<b;
    }
  __pptsocket.closeSocket();
  __connected=false;
  if(__debugging)
    cout<<__FILE__<<":"<<__LINE__<<": completed closing connection."<<endl; 
}

void 
DODSClient::clean_state()
{
  if(__debugging)
	cout<<__FILE__<<__LINE__<<": cleaning object state...\n";
  if(__pptsocket._type==PPT_UNIX_SOCKET)
    {
      if(__debugging)
	cout<<__FILE__<<__LINE__<<": removing socket unix type...";
      if(!access(tem.c_str(), F_OK))
	{
	  remove(tem.c_str());
	  if(__debugging)
	    cout<<"OK"<<endl;
	}
      else
	{
	  if(__debugging)
	    {
	      cout<<"failed"<<endl;
	      switch (errno)
		{
		case EFAULT:
		  cerr<<"EFAULT: "<<strerror(errno)<<endl;
		  break;
		case ELOOP:
		  cerr<<"ELOOP: "<<strerror(errno)<<endl;
		  break;
		case ENAMETOOLONG: 
		  cerr<<"ENAMETOOLONG: "<<strerror(errno)<<endl;
		  break;
		case  ENOENT:
		  cerr<<"ENOENT: "<<strerror(errno)<<endl;
		  break;
		case ENOLINK:
		  cerr<<"ENOLINK: "<<strerror(errno)<<endl;
		  break;
		case ENOTDIR:
		  cerr<<"ENOTDIR: "<<strerror(errno)<<endl;
		  break;
		case EROFS: 
		  cerr<<"EROFS: "<<strerror(errno)<<endl;
		  break;
		case EINVAL:
		  cerr<<"EINVAL: "<<strerror(errno)<<endl;
		  break;
		case ETXTBSY:
		  cerr<<"ETXBSY: "<<strerror(errno)<<endl;
		  break;
		default:
		  cerr<<"can not map errno"<<endl;
		  break;
		}
	      cerr<<"Warning: may have failed to remove temporary client Unix socket!"<<endl ;
	      cerr<<"Please check for file "<<tem<<" and remove if necessary"<<endl;
	    }
	} 
    }
}

bool DODSClient::write_buffer(PPTBuffer &buf) throw()
{
  try 
    {
      __pptsocket<<buf;
    }
  catch( PPTException &e )
    {
      cerr<<__FILE__<<":"<<__LINE__<<": error while writing data."<<endl;
      cout<<e.getErrorFile()<<":"<<e.getErrorLine()<<" "<<e.getErrorDescription()<<endl;
      throw ;
    }
  catch(...)
    {
      cerr<<__FILE__<<":"<<__LINE__<<": error while writing data."<<endl;
      throw ;
    }
  return true;
}

bool
DODSClient::read_buffer(PPTBuffer *buf, bool &eof) throw()
{
    try
    {
	__pptsocket >> (*buf) ;
	if( server_complete_transmition( buf ) )
	    eof = true ;
	else
	    eof = false ;
    }
    catch( PPTException &e )
    {
	cerr << __FILE__ << ":" << __LINE__
	     << ": error while reading data." << endl ;
	cout << e.getErrorFile() << ":"
	     << e.getErrorLine() << " "
	     << e.getErrorDescription() << endl ;
	__pptsocket.closeSocket();
	clean_state();
	exit(1);
    }
    catch(...)
    {
	cerr << __FILE__ << ":" << __LINE__
	     << ": error while reading data." << endl ;
	__pptsocket.closeSocket();
	clean_state();
	exit(1) ;
    }
    return true ;
}

bool DODSClient::server_complete_transmition(PPTBuffer *b)
{
    int len=b->valid;
    int len1=strlen(PPTSERVER_COMPLETE_DATA_TRANSMITION);
    if (len >= len1)
	{
	    char * ff= new char [len1+1];
	    for (int j=0; j<len1; j++)
		ff[j]=b->data[(len-len1)+j];
	    ff[len1]='\0';
	    string closing_token=string(ff);
	    delete [] ff;
	    if (closing_token==PPTSERVER_COMPLETE_DATA_TRANSMITION)
		{
		    b->valid=b->valid-len1;
		    if (__debugging)
		      cout<<__FILE__<<":"<<__LINE__<<": server ready sending data ("<<closing_token<<")"<<endl;
		    return true;
		}
	}
    len1=strlen(PPTSERVER_EXIT_NOW);
    if (len >= len1)
	{
	   char * ff= new char [len1+1]; 
	   for (int j=0; j<len1; j++)
		ff[j]=b->data[(len-len1)+j];
	   ff[len1]='\0';
	   string closing_token=string(ff);
	   delete [] ff;
	   if (closing_token==PPTSERVER_EXIT_NOW)
	       {
		 b->valid=b->valid-len1;
		 cerr<<__FILE__<<":"<<__LINE__<<": dodsclient session must terminate now"<<endl;
		 if(b->valid!=0)
		   {
		     cerr<<"The server is reporting the following fatal error:"<<endl;
		     cout.write((const char *)b->data, b->valid);
		   }
		 __pptsocket.closeSocket();
		 clean_state();
		 exit(3);
	       }
	}
    if (__debugging)
      cout<<__FILE__<<":"<<__LINE__<<": server not ready sending data"<<endl;

    return false;
    
}

