#ifdef __cplusplus
extern "C" {
#define READLINE_LIBRARY
#include <stdio.h> 
#include <assert.h>
#include "readline.h"
#include "history.h"
}
#endif

#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

#include "pthread.h"

// -- C++ --
#include <iostream>
#include <fstream>

using std::cerr ;
using std::cout ;
using std::endl ;
using std::flush ;
using std::ios ;
using std::ofstream ;
using std::ifstream ;

#include "Client.h"

#include "PPTClientServerSessionProtocol.h"
#include "PPTBuffer.h"

#define SIZE_COMMUNICATION_BUFFER 4096*4096

extern int debug_server ;

extern const char *NameProgram ;

Client *Client::singletonRef = 0 ;

DODSClient *Client::_dodsclient = 0 ;

extern int errno ;

int
Client::get_rex_buffer( string &mess ) const
{
    int len = 0 ;
    char *buf = (char*)NULL ;
    buf = readline( "dodsclient> " ) ;
    if( buf && *buf )
    {
	len = strlen( buf ) ;
	add_history( buf ) ;
	if( len > SIZE_COMMUNICATION_BUFFER )
	{
	    cerr << __FILE__ << __LINE__
		 << ": incoming data buffer exceeds maximum capacity with lenght "
		 << len << endl ;
	    exit( 1 ) ;
	}
	else
	{
	    mess = buf ;
	}
    }
    if( buf )
    {
	free( buf ) ;
	buf = (char*)NULL ;
    }
    return len ;
}

void
Client::signal_can_not_connect( int sig )
{
    if( sig == SIGCONT )
    {
	if( !_dodsclient->is_connected() )
	{
	    cout << NameProgram
	         << ": No respond, server may be down or "
		 << "busy with another incoming connection. exiting!\n" ;
	    exit( 1 ) ;
	}
	else
	{
	    if( debug_server ) cout << "OK, go ahead!\n" ;
	}
    }
}

void
Client::signal_terminate(int sig)
{
    if( sig == SIGINT )
    {
	cout << NameProgram
	     << ": please type exit to terminate the session" << endl ;
    }
    if( signal( SIGINT, signal_terminate ) == SIG_ERR )
    {
	cerr << NameProgram << "Could not re-register signal\n" ;
    }
}

void
Client::signal_broken_pipe(int sig)
{
    if( sig == SIGPIPE )
    {
	cout << NameProgram
	     << ": got a broken pipe, server may be down or the port invalid."
	     << endl
	     << "Please check parameters and try again" << endl ;
	_dodsclient->broken_pipe();
	delete _dodsclient ;
	_dodsclient = 0 ;
	exit( 1 ) ;
    }
}

void
Client::register_signals() const
{
    // Registering SIGCONT for connection unblocking
    if( debug_server )
    {
	cout << NameProgram << ": registering signal SIGCONT..." << flush ;
    }
    if( signal( SIGCONT, signal_can_not_connect ) == SIG_ERR )
    {
	if( debug_server ) cout << "could not do it!" << endl ;
	exit( 1 ) ;
    }
    if( debug_server ) cout << "OK" << endl ;

    // Registering SIGINT to disable Ctrl-C from the user in order to avoid
    // server instability
    if( debug_server )
    {
	cout << NameProgram << ": registering signal SIGINT..." << flush ;
    }
    if( signal( SIGINT, signal_terminate ) == SIG_ERR )
    {
	if( debug_server ) cout << "could not do it!" << endl ;
	exit( 1 ) ;
    }
    if( debug_server ) cout << "OK" << endl ;

    // Registering SIGPIE for broken pipes managment.
    if( debug_server )
    {
	cout << NameProgram << ": registering signal SIGPIPE..." << flush ;
    }
    if( signal( SIGPIPE, signal_broken_pipe ) == SIG_ERR )
    {
	if( debug_server ) cout << "could not do it!" << endl ;
	exit( 1 ) ;
    }
    if( debug_server ) cout << "OK" << endl ;
}

bool
Client::start_client( char *host, int port, int debug, int timeout)
    const throw()
{
    debug_server = debug ;
    try
    {
	register_signals() ;
	_dodsclient = new DODSClient( host, port, debug, timeout ) ;
	if( !_dodsclient->is_connected() )
	{
	    delete _dodsclient ;
	    _dodsclient = 0 ;
	    return false ;
	}
	return true ;
    }
    catch( PPTException &e )
    {
	cout << e.getErrorFile() << ":" << e.getErrorLine()
	     << " " << e.getErrorDescription() << endl ;
	cout << "client process halt!" << endl ;
	exit( 1 ) ; 
    }
    catch( ... )
    {
	cout << __FILE__ << " " << __LINE__
	     << " Got undefined exception!" << endl ;
    }
    return false ;
}

bool
Client::start_client_at_unix_socket( char*unix_socket,
                                     int debug,
				     int timeout ) const throw()
{
    debug_server=debug ;
    try
    {
	register_signals() ;
	_dodsclient= new DODSClient( unix_socket, debug ) ;
	if( !_dodsclient->is_connected() )
	{
	    delete _dodsclient ;
	    _dodsclient = 0 ;
	    return false ;
	}
	return true ;
    }
    catch(PPTException &e)
    {
	cout << e.getErrorFile() << ":" << e.getErrorLine()
	     << " " << e.getErrorDescription() << endl ;
	cout << "client process halt!" << endl ;
	exit( 1 ) ; 
    }
    catch (...)
    {
	cout << __FILE__ << " " << __LINE__ << " Got undefined exception!"
	     << endl ;
    }

    return false ;
}

/*
typedef struct _thread_pars
{
    DDS *pdds ;
    int fileno ;
    pthread_mutex_t *mutex ;
} thread_pars;

void *
read_ahead(void * p)
{
    thread_pars *pars = (thread_pars *) p ;
    pthread_mutex_lock( pars->mutex ) ;
    //cout << __FILE__ << ":" << __LINE__ << ":" << getpid()
    //     << ": Thread locked" << endl ;
    //pars->pdds->parse( pars->fileno ) ;
    pthread_mutex_unlock( pars->mutex ) ;
    //cout << __FILE__ << ":" << __LINE__ << ":" << getpid()
    //     << ": Thread unlocked" << endl ;
    pthread_exit( 0 ) ;
}
*/

void
Client::take_server_control() const
{
    try
    {
	//time_t t1, t2 ;
	for(;;)
	{
	    string message = "" ;
	    int len = get_rex_buffer( message ) ;
	    if( len != 0 && message != "" )
	    {
		if( message == "exit" )
		{
		    cout << "Bye!" << endl ;
		    return ;
		}
		else
		{
		    PPTBuffer b( message.size() + 1 ) ;
		    strcpy( (char*)b.data, message.c_str() ) ;
		    b.valid=strlen( message.c_str() ) ;
		    _dodsclient->write_buffer( b ) ;
		    PPTBuffer *buf1 = new PPTBuffer( 4096 ) ;
		    strcpy( (char*)buf1->data,
		            PPTCLIENT_COMPLETE_DATA_TRANSMITION ) ;
		    buf1->valid=strlen( PPTCLIENT_COMPLETE_DATA_TRANSMITION ) ;
		    _dodsclient->write_buffer( *buf1 ) ;
		    delete buf1 ;
		    buf1 = 0 ;
		    bool eof( false ) ;
		    PPTBuffer t( 4096 ) ;
		    //int fd[2] ;
		    //if( pipe( fd ) < 0 )
		    //{
		    //	cerr << __FILE__ << ":" << __LINE__
		    //	     << ": pipe error" << endl ;
		    //	perror( "pipe" ) ;
		    //	return ;
		    //}
		    //if( fcntl( fd[1], F_SETFL, O_NONBLOCK ) == -1 )
		    //{
		    //	perror( "fnctl" ) ;
		    //	return ;
		    //}
		    //pthread_t *my_thread = new pthread_t() ;
		    //thread_pars *pars = new thread_pars ;
		    //pars->pdds = new DDS() ;
		    //pars->fileno = fd[0] ;
		    //pars->mutex = pars->mutex = new pthread_mutex_t() ;
		    //pthread_mutex_init( pars->mutex, NULL ) ;
		    //if( pthread_create( my_thread, NULL, read_ahead, (void*)pars) )
		    //{
		    //cerr << "Could not create read ahead thread" << endl ;
		    //return ;
		    //}

		    //t1 = time( 0 ) ;
		    _dodsclient->read_buffer( &t, eof );
		    cout.write( (const char *)t.data, t.valid ) ;
		    //write( fd[1], t.data, t.valid ) ;
		    while( !eof )
		    {
			_dodsclient->read_buffer( &t, eof ) ;
			cout.write( (const char *)t.data, t.valid ) ;
			//while( write( fd[1], t.data, t.valid ) == -1 )
			//{
			    /*
			    switch( errno )
			    {
				case EPIPE:
				    cerr << "Pipe problem" << endl ;
				    break ;
				case EAGAIN:
				    cerr << "Blocking problem" << endl ;
				    break ;
				case ENOSPC:
				    cerr << "Space problem" << endl ;
				    break ;
				case EFAULT:
				    cerr << "Bad buffer" << endl ;
				    break ;
				case EINTR:
				    cerr << "Interrupt problem" << endl ;
				    break ;
				case EFBIG:
				    cerr << "An attempt was made to write a file that exceeds the implementation-defined maximum file size or the process" << endl ;
				    break;
				case EIO:
				    cerr << "A low-level I/O error occurred while modifying the inode" << endl ;
				    break ;
				case ESPIPE:
				    cerr << "Illegal seek" << endl ;
				    break ;
				default:
				    cerr << "Other Problem=" << errno << endl ;
				    break ;
			    }
			    perror( "write" ) ;
			    return ;
			    */
			//}
			//read( fd[0], t.data, t.valid ) ;
		    }
		    //char seteof[1] ;
		    //seteof[0] = '' ;
		    //write( fd[1], seteof, 1 ) ;
		    //close( fd[1] ) ;

		    //cout << __FILE__ << ":" << __LINE__ << ":" << getpid()
		    //     << ": Done writing, trying to join thread" << endl ;
		    //pthread_mutex_lock( pars->mutex ) ;
		    //pthread_mutex_unlock( pars->mutex ) ;
		    //assert( !pthread_join( *my_thread, NULL ) ) ;
		    //delete my_thread ;
		    //pthread_mutex_destroy( pars->mutex ) ;
		    //delete pars->mutex ;
		    //lose( fd[0] ) ;
		    //try
		    //{
		    //	int stat = 0 ;
		    //ClientPipes::dds_magic_stdout(pars->pdds);
		    //if( stat )
		    //{
		    //    delete pars->pdds ;
		    //    delete pars ;
		    //    pars = 0 ;
		    //    return ;
		    //}
		    //delete pars->pdds ;
		    //delete pars ;
		    //pars = 0 ;
		    //}
		    //catch( InternalErr &i )
		    //{
		    //cout << "DODS internal exception" << endl ;
		    //i.print() ;
		    //return ;
		    //}
		    //catch(Error &e)
		    //{
		    //cout << "DODS exception" << endl ;
		    //e.print() ;
		    //return ;
		    //}
		    //cout << "Done getting data" << endl ;
		    //t2 = time( 0 ) ;
		    //cout << "Total time is " << t2-t1 << endl ;
		}
	    }
	}
    }
    catch(...)
    {
	cerr << NameProgram << ": got exception managing client" << endl ;
	return ;
    }
}

void
Client::interact() const throw()
{
    if( _dodsclient )
    {
	if( _dodsclient->is_connected() )
	{
	    take_server_control() ;
	}
	else
	{
	    cerr << NameProgram << ": client is not connected" << endl ;
	}
	_dodsclient->close_connection() ;
	delete _dodsclient ;
	_dodsclient = 0 ;
    }
    else
	cerr << NameProgram << ": client element is invalid" << endl ;
}

void
Client::execute_and_quit( const string &message,
                          const string &file_name ) const throw()
{
    try
    {
	if( _dodsclient )
	{
	    if( _dodsclient->is_connected() )
	    {
		if( message.size() < 4096 )
		{
		    ofstream my_file( file_name.c_str() ) ;
		    PPTBuffer b( 4096 ) ;
		    strcpy( (char*)b.data, message.c_str() ) ;
		    b.valid = strlen( message.c_str() ) ;
		    _dodsclient->write_buffer( b ) ;
		    PPTBuffer *buf1 = new PPTBuffer( 4096 ) ;
		    strcpy( (char*)buf1->data,
		            PPTCLIENT_COMPLETE_DATA_TRANSMITION ) ;
		    buf1->valid = strlen( PPTCLIENT_COMPLETE_DATA_TRANSMITION );
		    _dodsclient->write_buffer( *buf1 ) ;
		    delete buf1 ; 
		    buf1 = 0 ;
		    bool eof( false ) ;
		    PPTBuffer t( 4096 ) ;
		    while( !eof )
		    {
			_dodsclient->read_buffer( &t, eof ) ;
			my_file.write( (const char *)t.data, t.valid ) ;
		    }
		    my_file.close() ;
		}
		else
		    cerr << NameProgram
		         << ": size of command is larger than maximun of 4096."
			 << " Please use the -i method"
			 << endl ;
		_dodsclient->close_connection() ;
		delete _dodsclient ;
		_dodsclient = 0 ;
	    }
	    else
		cerr << NameProgram << ": client is not connected" << endl ;
	}
	else
	    cerr << NameProgram << ": client element is invalid" << endl ;
    }
    catch(PPTException &e)
    {
	cout << e.getErrorFile() << ":" << e.getErrorLine()
	     << " " << e.getErrorDescription() << endl ;
	cout << "client process halt!" << endl ;
	exit( 1 ) ; 
    }
    catch(...)
    {
	cerr << NameProgram << ": exception from PPT level manager" << endl ;
	cerr << NameProgram
	     << ": exception manager is not turned on."
	     << " No more information available" << endl ;
	exit( 2 ) ;
    }
}

unsigned int first_quotes[1000] ;
unsigned int second_quotes[1000] ;

void
scan_for_quotes( const string &data )
{
    unsigned int num_first_quotes = 0 ;
    unsigned int num_second_quotes = 0 ;
    first_quotes[num_first_quotes] = data.find( "\"" ) ;
    second_quotes[num_second_quotes] = string::npos ;
    while( first_quotes[num_first_quotes] != string::npos )
    {
	second_quotes[num_second_quotes] =
		data.find( "\"", first_quotes[num_first_quotes] + 1 ) ;
	num_first_quotes++ ;
	if( second_quotes[num_second_quotes] == string::npos )
	{
	    first_quotes[num_first_quotes] = string::npos ;
	}
	else
	{
	    first_quotes[num_first_quotes] =
		    data.find( "\"", second_quotes[num_second_quotes] + 1 ) ;
	    num_second_quotes++ ;
	}
    }
    second_quotes[num_second_quotes] = string::npos ;
}

bool
within_quotes( unsigned int index )
{
    bool ret = false ;
    unsigned int q_index = 0 ;
    if( index < first_quotes[q_index] )
    {
	return false ;
    }

    while( ( first_quotes[q_index] != string::npos )
           && ( index > first_quotes[q_index] ) )
    {
	q_index++ ;
    }
    q_index-- ;
    if( ( first_quotes[q_index] != string::npos )
        && ( index > first_quotes[q_index] ) )
    {
	if( ( second_quotes[q_index] == string::npos )
	    || ( index < second_quotes[q_index] ) )
	{
	    ret = true ;
	}
    }

    return ret ;
}

#define SIZE_INPUT_BUFFER_ON_FILE_READ 32

void
Client::execute_input_file_and_quit( const string &in_file,
                                     const string &out_file ) const throw()
{
    try
    {
	if( _dodsclient )
	{
	    if( _dodsclient->is_connected() )
	    {
		ifstream input_stream( in_file.c_str(), ios::in | ios::binary );
		if( !input_stream )
		{
		    cerr << NameProgram
		         << ": fatal, can not open input file \""
			 << in_file << "\"\n" ;
		}
		else
		{
		    ofstream my_file( out_file.c_str() ) ;
		    if( !my_file )
		    {
			cerr << NameProgram
			     << ": fatal, can not open output file \""
			     << out_file << "\"\n" ;
		    }
		    else
		    {
			char buffer[SIZE_INPUT_BUFFER_ON_FILE_READ] ;
			string command ;
			string data ;
			while( !input_stream.eof() )
			{
			    input_stream.read( buffer,
			                       SIZE_INPUT_BUFFER_ON_FILE_READ );
			    data += string( buffer, input_stream.gcount() ) ;
			    scan_for_quotes( data ) ;
			    unsigned int marker = data.find( ";" ) ;
			    if( marker != string::npos )
			    {
				while( marker != string::npos )
				{
				    if( !within_quotes( marker ) )
				    {
					command = data.substr( 0,marker+1 ) ;
					cout << "Executing command "
					     << command << endl ;
					PPTBuffer b( command.size() ) ;
					memcpy( (char*)b.data,
						command.c_str(),
						command.size() ) ;
					b.valid = command.size() ;
					_dodsclient->write_buffer( b ) ;
					PPTBuffer *buf1 = new PPTBuffer( 4096 );
					strcpy( (char*)buf1->data, PPTCLIENT_COMPLETE_DATA_TRANSMITION ) ;
					buf1->valid = strlen( PPTCLIENT_COMPLETE_DATA_TRANSMITION ) ;
					_dodsclient->write_buffer( *buf1 ) ;
					delete buf1 ;
					buf1 = 0 ;
					bool eof( false ) ;
					PPTBuffer t( 4096 ) ;
					while( !eof )
					{
					    _dodsclient->read_buffer( &t, eof );
					    my_file.write( (const char *)t.data,
							   t.valid ) ;
					}
					data =data.substr( marker+1,
							   data.size() ) ;
					scan_for_quotes( data ) ;
					marker = data.find( ";" ) ;
				    }
				    else
				    {
					marker = data.find( ";", marker + 1 ) ;
				    }
				}
			    }
			}
			my_file.close() ;
		    }
		    input_stream.close() ;
		}
		_dodsclient->close_connection() ;
		delete _dodsclient ;
		_dodsclient = 0 ;
	    }
	    else
		cerr << NameProgram << ": client is not connected" << endl ;
	}
	else
	    cerr << NameProgram << ": client element is invalid" << endl ;
    }
    catch(PPTException &e)
    {
	cout << e.getErrorFile() << ":" << e.getErrorLine()
	     << " " << e.getErrorDescription() << endl ;
	cout << "client process halt!" << endl ;
	exit( 1 ) ; 
    }
    catch(...)
    {
	cerr << NameProgram << ": exception from PPT level manager" << endl ;
	cerr << NameProgram << ": exception manager is not turned on,"
	     << " No more information available" << endl ;
	exit( 2 ) ;
    }
}

