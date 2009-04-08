// pptcapi_tcp_connect.c

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>

#include "pptcapi.h"
#include "pptcapi_utils.h"
#include "pptcapi_debug.h"

extern void pptcapi_initialize_connection_struct( struct pptcapi_connection *connection ) ;

extern int pptcapi_receive_buffer_size ;
extern char *pptcapi_receive_buffer ;

extern int pptcapi_send_buffer_size ;
extern char *pptcapi_send_buffer ;

int set_tcp_recv_buffer_size( int socket, int buffer_size, char **error ) ;
int set_tcp_send_buffer_size( int socket, int buffer_size, char **error ) ;

void get_tcp_recv_buffer_size( int socket ) ;
void get_tcp_send_buffer_size( int socket ) ;

struct pptcapi_connection *
pptcapi_tcp_connect( const char *host, int portval, int timeout,
		     int tcp_recv_buffer_size,
		     int tcp_send_buffer_size,
		     char **error )
{
    PPTCAPI_DEBUG00( "tcp connect entered\n" ) ;

    void *vconnection = malloc( sizeof( struct pptcapi_connection ) ) ;
    if( !vconnection )
    {
	*error = (char *)malloc( PPTCAPI_ERR_LEN ) ;
	if( *error )
	{
	    snprintf( *error, PPTCAPI_ERR_LEN,
		      "Failed to allocate memory for connection struct" ) ;
	}
	return 0 ;
    }
    struct pptcapi_connection *connection =
	(struct pptcapi_connection *)vconnection ;
    pptcapi_initialize_connection_struct( connection ) ;

    connection->is_tcp = 1 ;
    if( !host )
    {
	char *localhost = "localhost" ;
	connection->host = (char *)malloc( strlen( localhost ) + 1 ) ;
	if( !connection->host )
	{
	    *error = (char *)malloc( PPTCAPI_ERR_LEN ) ;
	    if( *error )
	    {
		snprintf( *error, PPTCAPI_ERR_LEN,
			  "Failed to allocate memory for connection host" ) ;
	    }
	    pptcapi_free_connection_struct( connection ) ;
	    return 0 ;
	}
	strcpy( connection->host, localhost ) ;
    }
    else
    {
	int len = strlen( host ) ;
	if( len > PPTCAPI_MAX_STR_LEN )
	    len = PPTCAPI_MAX_STR_LEN ;
	connection->host = (char *)malloc( len + 1 ) ;
	if( !connection->host )
	{
	    *error = (char *)malloc( PPTCAPI_ERR_LEN ) ;
	    if( *error )
	    {
		snprintf( *error, PPTCAPI_ERR_LEN,
			  "Failed to allocate memory for connection host" ) ;
	    }
	    pptcapi_free_connection_struct( connection ) ;
	    return 0 ;
	}
	strncpy( connection->host, host, len ) ;
	connection->host[len] = '\0' ;
    }
    PPTCAPI_DEBUG03( "  TCP connection to host %s, port %d, timeout %d\n",
		     connection->host, portval, timeout )

    connection->port = portval ;

    if( timeout > PPTCAPI_MAX_TIMEOUT )
	connection->timeout = PPTCAPI_MAX_TIMEOUT ;
    else
	connection->timeout = timeout ;

    struct protoent *pProtoEnt ;
    struct sockaddr_in sin ;
    struct hostent *ph ;
    long address ;
    if( isdigit( connection->host[0] ) )
    {
	if( ( address = inet_addr( connection->host ) ) == -1 )
	{
	    *error = (char *)malloc( PPTCAPI_ERR_LEN ) ;
	    if( *error )
	    {
		snprintf( *error, PPTCAPI_ERR_LEN,
			  "Invalid host ip address %s", connection->host ) ;
	    }
	    pptcapi_free_connection_struct( connection ) ;
	    return 0 ;
	}
	sin.sin_addr.s_addr = address ;
	sin.sin_family = AF_INET ;
    }
    else
    {
	if( ( ph = gethostbyname( connection->host ) ) == NULL )
	{
	    *error = (char *)malloc( PPTCAPI_ERR_LEN ) ;
	    if( *error )
	    {
		switch( h_errno )
		{
		    case HOST_NOT_FOUND:
			{
			    snprintf( *error, PPTCAPI_ERR_LEN,
				      "No such host %s", connection->host ) ;
			    pptcapi_free_connection_struct( connection ) ;
			    return 0 ;
			}
		    case TRY_AGAIN:
			{
			    snprintf( *error, PPTCAPI_ERR_LEN,
				      "Host %s is busy, try again later",
				      connection->host ) ;
			    pptcapi_free_connection_struct( connection ) ;
			    return 0 ;
			}
		    case NO_RECOVERY:
			{
			    snprintf( *error, PPTCAPI_ERR_LEN,
				      "DNS error for host %s",
				      connection->host ) ;
			    pptcapi_free_connection_struct( connection ) ;
			    return 0 ;
			}
		    case NO_ADDRESS:
			{
			    snprintf( *error, PPTCAPI_ERR_LEN,
				      "No IP address for host %s",
				      connection->host ) ;
			    pptcapi_free_connection_struct( connection ) ;
			    return 0 ;
			}
		    default:
			{
			    snprintf( *error, PPTCAPI_ERR_LEN,
				      "unknown error getting info for host ",
				      connection->host ) ;
			    pptcapi_free_connection_struct( connection ) ;
			    return 0 ;
			}
		}
	    }
	}
	else
	{
	    sin.sin_family = ph->h_addrtype ;
	    char **p ;
	    for( p = ph->h_addr_list; *p != NULL; p++ )
	    {
		struct in_addr in ;
		(void)memcpy( &in.s_addr, *p, sizeof( in.s_addr ) ) ;
		memcpy( (char*)&sin.sin_addr, (char*)&in, sizeof( in ) ) ;
	    }
	}
    }

    sin.sin_port = htons( connection->port ) ;
    pProtoEnt = getprotobyname( "tcp" ) ;
    if( !pProtoEnt )
    {
	*error = (char *)malloc( PPTCAPI_ERR_LEN ) ;
	if( *error )
	{
	    snprintf( *error, PPTCAPI_ERR_LEN,
		      "Error retreiving tcp protocol information" ) ;
	}
	pptcapi_free_connection_struct( connection ) ;
	return 0 ;
    }
    
    int descript = socket( AF_INET, SOCK_STREAM, pProtoEnt->p_proto ) ;
    int my_errno = errno ;
    
    if( descript == -1 ) 
    {
	*error = (char *)malloc( PPTCAPI_ERR_LEN ) ;
	if( *error )
	{
	    const char* error_info = strerror( my_errno ) ;
	    if( error_info )
		snprintf( *error, PPTCAPI_ERR_LEN,
			  "getting socket descriptor: %s", error_info ) ;
	    else
		snprintf( *error, PPTCAPI_ERR_LEN,
			  "getting socket descriptor: unknown error" ) ;
	}
	pptcapi_free_connection_struct( connection ) ;
	return 0 ;
    } else {
        long holder ;
        connection->socket = descript;

        //set socket to non-blocking mode
        holder = fcntl( connection->socket, F_GETFL, NULL ) ;
        holder = holder | O_NONBLOCK ;
        fcntl( connection->socket, F_SETFL, holder ) ;
      
	// we must set the send and receive buffer sizes before the connect call
	int isok = set_tcp_recv_buffer_size( descript,
					     tcp_recv_buffer_size,
					     error ) ;
	if( isok != PPTCAPI_OK )
	{
	    pptcapi_free_connection_struct( connection ) ;
	    return 0 ;
	}
	isok = set_tcp_send_buffer_size( descript,
					 tcp_send_buffer_size,
					 error ) ;
	if( isok != PPTCAPI_OK )
	{
	    pptcapi_free_connection_struct( connection ) ;
	    return 0 ;
	}
      
        int res = connect( descript, (struct sockaddr*)&sin, sizeof( sin ) ) ;
	my_errno = errno ;
        if( res == -1 ) 
        {
            if( my_errno == EINPROGRESS )
	    {
                fd_set write_fd ;
                struct timeval timeout ;
                int maxfd = connection->socket ;
	  
                timeout.tv_sec = 5 ;
                timeout.tv_usec = 0 ;
	  
                FD_ZERO( &write_fd ) ;
                FD_SET( connection->socket, &write_fd ) ;
	  
		res = select( maxfd+1, NULL, &write_fd, NULL, &timeout) ;
		my_errno = errno ;
                if( res < 0 )
		{
                    //reset socket to blocking mode
                    holder = fcntl( connection->socket, F_GETFL, NULL ) ;
                    holder = holder & (~O_NONBLOCK) ;
                    fcntl( connection->socket, F_SETFL, holder ) ;
	    
                    //throw error - select could not resolve socket
		    *error = (char *)malloc( PPTCAPI_ERR_LEN ) ;
		    if( *error )
		    {
			const char *error_info = strerror( my_errno ) ;
			if( error_info )
			    snprintf( *error, PPTCAPI_ERR_LEN,
				      "error selecting sockets: %s",
				      error_info ) ;
			else
			    snprintf( *error, PPTCAPI_ERR_LEN,
				      "error selecting sockets: unknown" ) ;
		    }
		    pptcapi_free_connection_struct( connection ) ;
		    return 0 ;
                } 
                else 
                {
                    //check socket status
                    socklen_t lon ;
                    int valopt ;
                    lon = sizeof( int ) ;
                    getsockopt( connection->socket, SOL_SOCKET, SO_ERROR,
		                (void*) &valopt, &lon ) ;
	    
                    if( valopt )
                    {
                        //reset socket to blocking mode
                        holder = fcntl( connection->socket, F_GETFL, NULL ) ;
                        holder = holder & (~O_NONBLOCK) ;
                        fcntl( connection->socket, F_SETFL, holder ) ;
	      
                        //throw error - did not successfully connect
			*error = (char *)malloc( PPTCAPI_ERR_LEN ) ;
			if( *error )
			{
			    snprintf( *error, PPTCAPI_ERR_LEN, "Did not successfully connect to server. Server may be down or you may be trying on the wrong port" ) ;
			}
			pptcapi_free_connection_struct( connection ) ;
			return 0 ;
                    } 
                    else 
                    {
                        //reset socket to blocking mode
                        holder = fcntl( connection->socket, F_GETFL, NULL ) ;
                        holder = holder & (~O_NONBLOCK) ;
                        fcntl( connection->socket, F_SETFL, holder ) ;
                    }
                }
            }
            else 
            {

                //reset socket to blocking mode
                holder = fcntl( connection->socket, F_GETFL, NULL ) ;
                holder = holder & (~O_NONBLOCK) ;
                fcntl( connection->socket, F_SETFL, holder ) ;
	  
                //throw error - errno was not EINPROGRESS
		*error = (char *)malloc( PPTCAPI_ERR_LEN ) ;
		if( *error )
		{
		    const char *error_info = strerror( my_errno ) ;
		    if( error_info )
			snprintf( *error, PPTCAPI_ERR_LEN,
				  "error connecting to socket: %s",
				  error_info ) ;
		    else
			snprintf( *error, PPTCAPI_ERR_LEN,
				  "error connecting to socket: unknown" ) ;
		}
		pptcapi_free_connection_struct( connection ) ;
		return 0 ;
            }
        }
        else
        {
            // The socket connect request completed immediately
            // even that the socket was in non-blocking mode
            
            //reset socket to blocking mode
            holder = fcntl( connection->socket, F_GETFL, NULL ) ;
            holder = holder & (~O_NONBLOCK) ;
            fcntl( connection->socket, F_SETFL, holder ) ;
        }
    }

    // we set the send and receive buffer sizes above. Now we need to get
    // the final sizes. We do this because, even though we may have set the
    // buffer sizes for the send and receive buffers, the system might have
    // changed that size. For example, the size was set too big, so the
    // system changes the value.

    get_tcp_recv_buffer_size( connection->socket ) ;
    get_tcp_send_buffer_size( connection->socket ) ;

    PPTCAPI_DEBUG00( "tcp connect complete\n" ) ;

    return connection ;
}

int
set_tcp_recv_buffer_size( int socket, int buffer_size, char **error )
{
    if( buffer_size <= 0 )
    {
	pptcapi_receive_buffer_size = PPTCAPI_DEFAULT_BUFFER_SIZE ;
	PPTCAPI_DEBUG01( "  pptcapi_receive_buffer_size set to %d\n",
			 pptcapi_receive_buffer_size )
    }
    else
    {
	PPTCAPI_DEBUG01( "  setting receive buffer size to %d\n",
			 buffer_size )
	// call setsockopt
	int err = setsockopt( socket, SOL_SOCKET, SO_RCVBUF,
			  (char *)&buffer_size,
			  (socklen_t)sizeof(buffer_size) ) ;
	int myerrno = errno ;
	if( err == -1 )
	{
	    *error = (char *)malloc( PPTCAPI_ERR_LEN ) ;
	    if( *error )
	    {
		char *serr = strerror( myerrno ) ;
		if( serr )
		{
		    snprintf( *error, PPTCAPI_ERR_LEN,
			     "Failed to set the receive buffer size to %d: %s ",
			     buffer_size, serr ) ;
		}
		else
		{
		    snprintf( *error, PPTCAPI_ERR_LEN,
			     "Failed to set the receive buffer size to %d: %s ",
			     buffer_size, "unknown error occurred" ) ;
		}
	    }
	    return PPTCAPI_ERROR ;
	}
	pptcapi_receive_buffer_size = buffer_size ;
    }
    return PPTCAPI_OK ;
}

int
set_tcp_send_buffer_size( int socket, int buffer_size, char **error )
{
    if( buffer_size <= 0 )
    {
	pptcapi_send_buffer_size = PPTCAPI_DEFAULT_BUFFER_SIZE ;
    }
    else
    {
	// call setsockopt
	int err = setsockopt( socket, SOL_SOCKET, SO_SNDBUF,
			  (char *)&buffer_size,
			  (socklen_t)sizeof(buffer_size) ) ;
	int myerrno = errno ;
	if( err == -1 )
	{
	    *error = (char *)malloc( PPTCAPI_ERR_LEN ) ;
	    if( *error )
	    {
		char *serr = strerror( myerrno ) ;
		if( serr )
		{
		    snprintf( *error, PPTCAPI_ERR_LEN,
			     "Failed to set the send buffer size to %d: %s ",
			     buffer_size, serr ) ;
		}
		else
		{
		    snprintf( *error, PPTCAPI_ERR_LEN,
			     "Failed to set the send buffer size to %d: %s ",
			     buffer_size, "unknown error occurred" ) ;
		}
	    }
	    return PPTCAPI_ERROR ;
	}
	pptcapi_send_buffer_size = buffer_size ;
    }
    return PPTCAPI_OK ;
}

void
get_tcp_recv_buffer_size( int socket )
{
    static char pptcapi_have_receive_buffer_size = 0 ;
    if( !pptcapi_have_receive_buffer_size )
    {
	// call getsockopt and set the internal variables to the result
	unsigned int sizenum = 0 ;
	socklen_t sizelen = sizeof(sizenum) ;
	int err = getsockopt( socket, SOL_SOCKET, SO_RCVBUF,
			  (char *)&sizenum, (socklen_t *)&sizelen ) ;
	int myerrno = errno ;
	if( err == -1 )
	{
	    char *serr = strerror( myerrno ) ;
	    if( serr )
	    {
		fprintf( stderr,
			 "Failed to get the socket receive buffer size: %s\n",
			 serr ) ;
	    }
	    else
	    {
		fprintf( stderr,
			 "Failed to get the socket receive buffer size: %s\n",
			 "unknown error occurred" ) ;
	    }
	    pptcapi_receive_buffer_size = PPTCAPI_DEFAULT_BUFFER_SIZE ;
	}
	else
	{
	    pptcapi_receive_buffer_size = sizenum ;
	}
	pptcapi_have_receive_buffer_size = 1 ;
	PPTCAPI_DEBUG01( "  pptcapi_receive_buffer_size get to %d\n",
			 pptcapi_receive_buffer_size )
    }
}

void
get_tcp_send_buffer_size( int socket )
{
    static char pptcapi_have_send_buffer_size = 0 ;
    if( !pptcapi_have_send_buffer_size )
    {
	// call getsockopt and set the internal variables to the result
	unsigned int sizenum = 0 ;
	socklen_t sizelen = sizeof(sizenum) ;
	int err = getsockopt( socket, SOL_SOCKET, SO_SNDBUF,
			  (char *)&sizenum, (socklen_t *)&sizelen ) ;
	int myerrno = errno ;
	if( err == -1 )
	{
	    char *serr = strerror( myerrno ) ;
	    if( serr )
	    {
		fprintf( stderr,
			 "Failed to get the socket send buffer size: %s\n",
			 serr ) ;
	    }
	    else
	    {
		fprintf( stderr,
			 "Failed to get the socket send buffer size: %s\n",
			 "unknown error occurred" ) ;
	    }
	    pptcapi_send_buffer_size = PPTCAPI_DEFAULT_BUFFER_SIZE ;
	}
	else
	{
	    pptcapi_send_buffer_size = sizenum ;
	}
	pptcapi_have_send_buffer_size = 1 ;
	PPTCAPI_DEBUG01( "  pptcapi_send_buffer_size get to %d\n",
			 pptcapi_receive_buffer_size )
    }
}

