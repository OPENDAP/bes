// Server.cc

#include <iostream>
#include <vector>

using std::cerr ;
using std::cout ;
using std::endl ;
#include "Server.h"

#include "PPTSocket.h"
#include "PPTException.h"
#include "PPTUtilities.h"
#include "PPTServerListener.h"
// The protocol between client and server
#include "PPTClientServerSessionProtocol.h"

#include "ServerStatusControl.h"
#include "ServerExitConditions.h"

#include "DODSDataRequestInterface.h"
#include "DODSWrapper.h"
#include "DODSMemoryGlobalArea.h"
#include "TheDODSKeys.h"
#include "DODSKeysException.h"
#include "DODSStatusReturn.h"

extern int debug_server ;
extern const char *NameProgram ;
extern "C" { extern int errno; }

Server *Server::singletonRef = 0 ;
DODSMemoryGlobalArea *Server::_m = 0 ;

Server::Server() throw()
{
    _m = new DODSMemoryGlobalArea() ;

    write_out( "Trying to register SIGINT..." ) ;
    if( signal( SIGINT, signal_terminate ) == SIG_ERR )
    {
	cerr << "Can not register SIGINT signal handler\n" ;
	exit( SERVER_EXIT_FATAL_CAN_NOT_START ) ;
    }
    else
	write_out( "OK\n" ) ;

    write_out( "Trying to register SIGUSR1..." ) ;
    if( signal( SIGUSR1, signal_restart ) == SIG_ERR )
    {
	cerr << "Can not register SIGUSR1 signal handler\n" ;
	exit( SERVER_EXIT_FATAL_CAN_NOT_START ) ;
    }
    else
	write_out( "OK\n" ) ;

    write_out( "Trying to determine process manager method..." ) ;
    method() ;
    write_out( _method.c_str() ) ;
    write_out( "\n" ) ;
}

Server::~Server()
{
    cout << "Destroying server singleton reference " << endl ; 
    if( _m ) delete _m ;
}

void
Server::signal_terminate( int sig ) throw()
{
    if( sig == SIGINT )
    {
	cout << NameProgram << ":" << getpid()
	     << ": got termination signal, exiting!" << endl ;
	exit( SERVER_EXIT_NORMAL_SHUTDOWN ) ;
    }
}

void
Server::signal_restart( int sig ) throw()
{
    if( sig == SIGUSR1 )
    {
	cout << NameProgram << ":" << getpid()
	     << ": got restart signal." << endl ;
	exit( SERVER_EXIT_RESTART ) ;
    }
}

void
Server::start( int port ) const throw()
{
    PPTServerListener listener ;
    try
	{
	  bool found = false ;
	  string key = "DODS.ServerUnixSocket" ;
	  string unix_socket = TheDODSKeys->get_key( key, found ) ;
	  listener.startListening(port, unix_socket.c_str());
	}
    catch(PPTException &e)
	{
	    cout<<__FILE__<<":"<<__LINE__<<" got an exception"<<endl;
	    cout<<e.getErrorFile()<<":"<<e.getErrorLine()<<" "<<e.getErrorDescription()<<endl;
	    cout<<"Process must exit because it can not start listening!"<<endl;
	    exit(SERVER_EXIT_FATAL_CAN_NOT_START); 
	}

    // Let the user know how to restart or shutdown this instance of the server.
    cout<<NameProgram<<": mounted "<<endl
	<<"to shutdown the server use (signal INT) kill -INT "<<getpid()<<endl
	<<"to restart the server use (signal USR1) kill -USR1 "<<getpid()<<endl;

    // Start managing incoming calls
    for(;;)
	{
	    try
		{
		    PPTSocket socket=listener.acceptConnections();
		    int status=welcome_new_client(socket);
		    switch(status)
			{
			  case NEW_PROCESS:
			    fork_call_handler(socket);
			    break;
			    
			  case UNDEFINED:
			    socket.closeSocket();
			    // do nothing because Server::welcome_new_client could not negotiate.
			    break;
			  default:
			    throw PPTException(__FILE__,__LINE__,"Invalid status on server.");
			}
		}
	    catch(PPTException &e)
		{
		    cout<<__FILE__<<":"<<__LINE__<<" got an exception"<<endl;
		    cout<<e.getErrorFile()<<":"<<e.getErrorLine()<<" "<<e.getErrorDescription()<<endl;
		    cout<<NameProgram<<":"<<getpid()<<": server process halt!"<<endl;
		    exit(SERVER_EXIT_ABNORMAL_TERMINATION); 
		}
	    // This kind of unknown exception may leave the server in an unstable state better leave now
	    catch(...)
		{
		    cout<<__FILE__<<":"<<__LINE__<<" got an exception"<<endl;
		    cout<<NameProgram<<": Undefined exception, must exit!"<<endl;
		    exit(SERVER_EXIT_ABNORMAL_TERMINATION);
		}
	}
}


int Server::welcome_new_client(PPTSocket socket) const
{
  try
    {
      if(debug_server)
	cout<<NameProgram<<":"<<getpid()<<": incoming connection, initiating handshake..."<<endl;
      PPTBuffer buf(4096);
      socket>>buf;
      string status((char*)buf.data, buf.valid);
      // char *t=new char [buf.valid+1];
      //memcpy(t,buf.data,buf.valid);
      //t[buf.valid]='\0';
      //string status=string(t);
      if(status!=PPTCLIENT_TESTING_CONNECTION)
	{
	  cout<<"Listener control: client has started connection with string "<<status<<endl;
	  cout<<"Listener control: PPT can not negotiate"<<endl;
	  return UNDEFINED;
	}
      else
	{
	  PPTBuffer buf1(4096);
	  strcpy((char*)buf1.data,PPTSERVER_CONNECTION_OK);
	  buf1.valid=strlen(PPTSERVER_CONNECTION_OK);
	  socket<<buf1;
	  return NEW_PROCESS;
	}
    }
  //catch PPTException, report and return. Doing this mantains the server state clean and happy
  catch(PPTException &e)
    {
      cout<<__FILE__<<":"<<__LINE__<<" got an exception"<<endl;
      cout<<e.getErrorFile()<<":"<<e.getErrorLine()<<" "<<e.getErrorDescription()<<endl;
      cout<<NameProgram<<":"<<getpid()<<": continuing..."<<endl;
      // This means that something went wrong so ignore this socket and keep going.
      return UNDEFINED;
    }
  // This kind of unknown exception may leave the server in an unstable state better leave now
  catch(...)
    {
      cout<<__FILE__<<":"<<__LINE__<<" got an exception"<<endl;
      cout<<NameProgram<<": Undefined exception, must exit!"<<endl;
      exit(SERVER_EXIT_ABNORMAL_TERMINATION);
    }
}


void Server::entry_in_child(PPTSocket socket) const
{
    if(debug_server) cout<<"DODS server "<<getpid()<<": starting client management."<<endl;
    for(;;)
	{
	    string message="";
	    PPTBuffer *buf=0;
	    bool keep_reading=true;
	    // here we let the server block waiting for client instructions...
	    while(keep_reading)
	      {
		buf=new PPTBuffer(4096);
		socket>>(*buf);
		keep_reading=!client_complete_transmition(buf);
		char *stuff= new char[buf->valid+1];
		memcpy(stuff, buf->data, buf->valid);
		stuff[buf->valid]='\0';
		message+=string(stuff);
		delete [] stuff;
		delete buf; 
		buf=0;
		
	      }

	    if(message==PPTCLIENT_EXIT_NOW)
		{
		    socket.closeSocket();
		    if(debug_server) cout<<"DODS server "<<getpid()<<": ending client management."<<endl;
		    exit(CHILD_SUBPROCESS_READY);
		}
	    else
		{
		    if(debug_server) cout<<"DODS server "<<getpid()<<": client wrote: "<<message<<endl;
		    int holder=dup(STDOUT_FILENO);
		    dup2(socket.getSocketDescriptor(),STDOUT_FILENO);
		    DODSDataRequestInterface rq;
		    // BEGIN Initialize all the data request elements correctly to a null pointer 
		    rq.server_name=0;
		    rq.server_address=0;
		    rq.server_protocol=0;
		    rq.server_port=0;
		    rq.script_name=0;
		    rq.user_address=0;
		    rq.user_agent=0;
		    rq.request=0;
		    // END Initialize all the data request elements correctly to a null pointer
		    
		    char server_name[256];
		    if (gethostname(server_name,256))
		    {
		      cerr<<__FILE__<<":"<<__LINE__<<": could not get server name"<<endl;
		    }
		    else
		      rq.server_name=server_name;
		    
		    rq.server_address="128.117.16.15";
		    rq.server_protocol="tcp";
		    rq.server_port="2000";
		    rq.script_name="DODS";
		    char *client_add=new char [(strlen(socket.getAddress()))+1];
		    strcpy(client_add,socket.getAddress());
		    rq.user_address=client_add;
		    rq.user_agent = "PPT communication library";
		    rq.request=message.c_str();
		    rq.cookie=NULL;
		    DODSWrapper wrapper;
		    int status=wrapper.call_DODS(rq);
		    fflush(stdout);
		    dup2(holder,STDOUT_FILENO);
		    close(holder);
		    delete [] client_add;
		    if (status==DODS_EXECUTED_OK)
			{
			    PPTBuffer *buf1=new PPTBuffer(4096);
			    strcpy((char*)buf1->data,PPTSERVER_COMPLETE_DATA_TRANSMITION);
			    buf1->valid=strlen(PPTSERVER_COMPLETE_DATA_TRANSMITION);
			    socket<<(*buf1);
			    delete buf1; buf1=0;
			}
		    else
		      {
			switch (status)
			  {
			  case DODS_TERMINATE_IMMEDIATE:
			    {
			      cout<<"DODS server "<<getpid()<<": Status not OK, dispatcher returned value "<<status<<endl;
			      PPTBuffer *buf=new PPTBuffer(4096);
			      strcpy((char*)buf->data,"FATAL ERROR; DODS server must terminate!\n");
			      buf->valid=strlen("FATAL ERROR; DODS server must terminate!\n");
			      socket<<(*buf);
			      delete buf; buf=0;
			      PPTBuffer *buf1=new PPTBuffer(4096);
			      strcpy((char*)buf1->data,PPTSERVER_EXIT_NOW);
			      buf1->valid=strlen(PPTSERVER_EXIT_NOW);
			      socket<<(*buf1);
			      delete buf1; buf1=0;
			      exit(CHILD_SUBPROCESS_READY);
			    }
			    break;
			  case DODS_DATA_HANDLER_FAILURE:
			    {
			      cout<<"DODS server "<<getpid()<<": Status not OK, dispatcher returned value "<<status<<endl;
			      PPTBuffer *buf=new PPTBuffer(4096);
			      strcpy((char*)buf->data,"Data Handler ERROR; DODS server must terminate!\n");
			      buf->valid=strlen("Data Handler ERROR; DODS server must terminate!\n");
			      socket<<(*buf);
			      delete buf; buf=0;
			      PPTBuffer *buf1=new PPTBuffer(4096);
			      strcpy((char*)buf1->data,PPTSERVER_EXIT_NOW);
			      buf1->valid=strlen(PPTSERVER_EXIT_NOW);
			      socket<<(*buf1);
			      delete buf1; buf1=0;
			      exit(CHILD_SUBPROCESS_READY);
			    }
			    break;
			  case DODS_REQUEST_INCORRECT: 
			  case DODS_MEMORY_EXCEPTION:
			  case DODS_MYSQL_CONNECTION_FAILURE: 
			  case DODS_MYSQL_BAD_QUERY:
			  case DODS_PARSER_ERROR:
			  case DODS_CONTAINER_PERSISTENCE_ERROR:
			  case DODS_INITIALIZATION_FILE_PROBLEM:
			  case DODS_LOG_FILE_PROBLEM:
			  case DODS_AGGREGATION_EXCEPTION:
			  case OPeNDAP_FAILED_TO_EXECUTE_COMMIT_COMMAND:
			    {
			      PPTBuffer *buf1=new PPTBuffer(4096);
			      strcpy((char*)buf1->data,PPTSERVER_COMPLETE_DATA_TRANSMITION);
			      buf1->valid=strlen(PPTSERVER_COMPLETE_DATA_TRANSMITION);
			      socket<<(*buf1);
			      delete buf1; buf1=0;
			    }
			    break;
			  }
		      }
		}
	}
}
void Server::method()
{
    try
	{
	    bool found = false ;
	    _method = TheDODSKeys->get_key( "DODS.ProcessManagerMethod", found ) ;
	    if (_method!="multiple" && _method!="single")
		throw DODSKeysException("Undefined method for handling clients.");
	}
    catch(DODSException &ex)
	{
	    cerr<<NameProgram<<": "<<ex.get_error_description()<<endl;
	    exit(SERVER_EXIT_FATAL_CAN_NOT_START);
	}
    catch(...)
	{
	    cerr<<NameProgram<<": undefined exception while attempting to get process handler method."<<endl;
	    exit(SERVER_EXIT_FATAL_CAN_NOT_START);
	}
	    
}


void Server::fork_call_handler(PPTSocket socket) const
{
    if(_method=="single")
	entry_in_child(socket);
    else
	{

	    int main_process=getpid();
	    pid_t pid;
	    if ((pid=fork())<0)
		{
		    string error("fork error");
		    const char* error_info=strerror(errno);
		    if (error_info)
			error+=" "+(string)error_info;
		    throw PPTException(__FILE__,__LINE__,error);
		}
	    else if (pid==0) /* child process */
		{
		    pid_t pid1;
		    // we fork twice so we do not have zombie children
		    if ((pid1=fork())<0)
			{
			    // we must send a signal of inmediate termination to the main server 
			    kill(main_process,9);
			    perror("fork error");
			    exit(SERVER_EXIT_CHILD_SUBPROCESS_ABNORMAL_TERMINATION);
			} 
		    else if (pid1==0)
			{
			    entry_in_child(socket);
			}
		    sleep(1);
		    socket.closeSocket();
		    exit(SERVER_EXIT_CHILD_SUBPROCESS_NORMAL_TERMINATION);
		}
	    if (waitpid(pid,NULL,0)!=pid)
		{
		    string error("waitpid error");
		    const char* error_info=strerror(errno);
		    if (error_info)
			error+=" "+(string)error_info;
		    throw PPTException(__FILE__,__LINE__,error);
		} 
	    socket.closeSocket();
	}
}

bool Server::client_complete_transmition(PPTBuffer *b) const
{
    int len=b->valid;
    int len1=strlen(PPTCLIENT_COMPLETE_DATA_TRANSMITION);
    if (len >= len1)
	{
	    char * ff= new char [len1+1];
	    for (int j=0; j<len1; j++)
		ff[j]=b->data[(len-len1)+j];
	    ff[len1]='\0';
	    string closing_token=string(ff);
	    delete [] ff;
	    if (closing_token==PPTCLIENT_COMPLETE_DATA_TRANSMITION)
		{
		    b->valid=b->valid-len1;
		    if (debug_server)
		      cout<<__FILE__<<":"<<__LINE__<<": client ready sending data ("<<closing_token<<")"<<endl;
		    return true;
		}
	}
    len1=strlen(PPTCLIENT_EXIT_NOW);
    if (len >= len1)
	{
	   char * ff= new char [len1+1]; 
	   for (int j=0; j<len1; j++)
		ff[j]=b->data[(len-len1)+j];
	   ff[len1]='\0';
	   string closing_token=string(ff);
	   delete [] ff;
	   if (closing_token==PPTCLIENT_EXIT_NOW)
	       {
		 cerr<<__FILE__<<":"<<__LINE__<<": client requested exit"<<endl;
		 exit(3);
	       }
	}
    if (debug_server)
      cout<<__FILE__<<":"<<__LINE__<<": client not ready sending data"<<endl;

    return false;
    
}


