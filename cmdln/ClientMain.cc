#include <unistd.h>
#include <string>
#include <iostream>

using std::cout ;
using std::endl ;
using std::cerr ;

#include "Client.h"

#define CLIENT_VERSION "1.2.4"

const char *NameProgram = 0 ;


void
show_version()
{
    cout << NameProgram << ": version " << CLIENT_VERSION << endl ;
    exit( 0 ) ;
}

void
show_usage()
{
    cout << NameProgram << ": the following flags are available:" << endl ; 
    cout << "-u [UNIX_SOCKET] -h [HOST] -p [PORT] -t [TIMEOUT] -x[QUERY] -f[FILE_NAME] -i[FILE_NAME] -d -v" << endl ;
    cout << "where" << endl ;
    cout << "The flag -u specifies the name of a Unix socket for connecting to the server." << endl ;
    cout << "The flag -h specifies the name of a host for TCP/IP connection" << endl ;
    cout << "The flag -p specifies the port where the server is listening for TCP/IP connection." << endl ;
    cout << "The flag -x makes the client execute a query and exit. This flag requires the -f flag." << endl ;
    cout << "The flag -f sets the target file name for the return stream from the server." << endl ;
    cout << "The flag -i sets the target file name for a sequence of input commands." << endl ;
    cout << "The flag -t sets the timeout in seconds and is optional." << endl ;
    cout << "The flag -d sets the client session for debugging but this is optional." << endl ;
    cout << "The flag -v forces the client to show the version and exit" << endl ;
    cout << NameProgram << ": usage" << endl ;
    cout << "CONNECTION FLAGS: -u or -p -h are used for specifing either a Unix socket or a TCP/IP socket" << endl ;
    cout << "INPUT/OUTPUT FLAGS: you can specify that the input is from the command line argument with the -x flag or that the input must be read from a file with the -i flag. If you specify either -x or -i you must specify the name of a file for the output stream of the server with the -f flag. If not input flag is specified, the client goes into interactive mode." << endl ;
    exit( 1 ) ;
}

int
main( int argc, char* argv[] )
{
    NameProgram = argv[0] ;

    char *host = 0 ;
    char *unix_socket = 0 ;
    char *execute_command = 0 ;
    char *output_file_name = 0 ;
    char *input_file_name = 0 ;
    int port = 0 ;
    int timeout = 0 ;
    bool got_host( false ) ;
    bool got_port( false ) ;
    bool got_timeout( false ) ;
    bool got_unix_socket( false ) ;
    bool got_output_file( false ) ;
    bool got_input_file( false ) ;
    bool execute_and_quit( false ) ;

    int debug_server = 0 ;

    int c ;

    while( ( c = getopt( argc, argv, "dvh:p:t:u:x:f:i:" ) ) != EOF )
    {
	switch( c )
	{
	    case 't':
		got_timeout = true ;
		timeout = atoi( optarg ) ;
		break ;
	    case 'h':
		got_host = true ;
		host = optarg ;
		break ;
	    case 'd':
		debug_server = 1 ;
		break ;
	    case 'v':
		show_version() ;
		break ;
	    case 'p':
		got_port = true ;
		port = atoi( optarg ) ;
		break ;  
	    case 'u':
		got_unix_socket = true ;
		unix_socket = optarg ;
		break ;
	    case 'x':
		execute_and_quit = true ;
		execute_command = optarg ;
		break ;
	    case 'f':
		got_output_file = true ;
		output_file_name = optarg ;
		break ;
	    case 'i':
		got_input_file = true ;
		input_file_name = optarg ;
		break ;
	    case '?':
		show_usage() ;
		break ;
	}
    }

    if( got_unix_socket && got_host )
    {
	cerr << NameProgram
	     << ": please specify either TCP/IP or a Unix socket, not both"
	     << endl ;
	show_usage() ;
    }

    if( execute_and_quit && got_input_file )
    {
	cerr << NameProgram
	     << ": please specify either an input file with the -i option "
	     << "or a command with the -x argument, not both" << endl ;
	show_usage() ;
    }

    bool status = false ;

    try 
      {
	const Client *pclient = Client::instanceOf() ;
	
	if( got_port && got_host )
	  {
	    if( got_timeout )
	      status = pclient->start_client( host, port, debug_server, timeout );
	    else
	      status = pclient->start_client( host, port, debug_server ) ;
	  }
	else if( got_unix_socket )
	  {
	    if( got_timeout )
	      {
		status = pclient->start_client_at_unix_socket( unix_socket,
							       debug_server,
							       timeout ) ;
	      }
	    else
	      {
		status = pclient->start_client_at_unix_socket( unix_socket,
							       debug_server ) ;
	      }
	  }
	else
	  {
	    show_usage() ;
	  }
	
	if( !status )
	  {
	    return 2 ;
	  }
	
	if( execute_and_quit )
	  {
	    if( !output_file_name )
	      {
		cerr << NameProgram
		     << ": when requesting 'execute and quit' "
		     << "you must provide a file name for return stream"
		     << endl ;
		show_usage() ;
	      }
	    else
	      pclient->execute_and_quit( string( execute_command ),
					 string( output_file_name ) ) ;
	  }
	else if( got_input_file )
	  {
	    if( !output_file_name )
	      {
		cerr << NameProgram
		     << ": when requesting execution from an input file "
		     << "you must provide a file name for return stream"
		     << endl ;
		show_usage() ;
	      }
	    else
	      {
		pclient->execute_input_file_and_quit( string( input_file_name ),
						      string( output_file_name ) ) ;
	      }
	  }
	else
	  pclient->interact() ;
      }
    catch(...)
      {
	cerr<<"Unmanaged exception at main level"<<endl;
	exit(1);
      }
}
	
