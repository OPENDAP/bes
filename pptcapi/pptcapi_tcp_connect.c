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

extern void pptcapi_initialize_connection_struct( struct pptcapi_connection *connection ) ;

struct pptcapi_connection *
pptcapi_tcp_connect( const char *host, int portval, int timeout,
		     char **error )
{
    void *vconnection = malloc( sizeof( struct pptcapi_connection ) ) ;
    struct pptcapi_connection *connection =
	(struct pptcapi_connection *)vconnection ;
    pptcapi_initialize_connection_struct( connection ) ;

    connection->is_tcp = 1 ;
    if( !host )
    {
	char *localhost = "localhost" ;
	connection->host = (char *)malloc( strlen( localhost ) + 1 ) ;
	strcpy( connection->host, localhost ) ;
    }
    else
    {
	int len = strlen( host ) ;
	if( len > PPTCAPI_MAX_STR_LEN )
	    len = PPTCAPI_MAX_STR_LEN ;
	connection->host = (char *)malloc( len + 1 ) ;
	strncpy( connection->host, host, len ) ;
	connection->host[len] = '\0' ;
    }

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
	    *error = (char *)malloc( 512 ) ;
	    sprintf( *error, "Invalid host ip address %s", connection->host ) ;
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
	    switch( h_errno )
	    {
		*error = (char *)malloc( 512 ) ;
		case HOST_NOT_FOUND:
                    {
			sprintf( *error, "No such host %s",
				         connection->host ) ;
			pptcapi_free_connection_struct( connection ) ;
			return 0 ;
                    }
		case TRY_AGAIN:
                    {
			sprintf( *error, "Host %s is busy, try again later",
			                 connection->host ) ;
			pptcapi_free_connection_struct( connection ) ;
			return 0 ;
                    }
		case NO_RECOVERY:
                    {
			sprintf( *error, "DNS error for host %s",
					 connection->host ) ;
			pptcapi_free_connection_struct( connection ) ;
			return 0 ;
                    }
		case NO_ADDRESS:
                    {
			sprintf( *error, "No IP address for host %s",
					 connection->host ) ;
			pptcapi_free_connection_struct( connection ) ;
			return 0 ;
                    }
		default:
                    {
			sprintf( *error, "unknown error getting info for host ",
					 connection->host ) ;
			pptcapi_free_connection_struct( connection ) ;
			return 0 ;
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
	*error = (char *)malloc( 512 ) ;
	sprintf( *error, "Error retreiving tcp protocol information" ) ;
	pptcapi_free_connection_struct( connection ) ;
	return 0 ;
    }
    
    int descript = socket( AF_INET, SOCK_STREAM, pProtoEnt->p_proto ) ;
    int my_errno = errno ;
    
    if( descript == -1 ) 
    {
	*error = (char *)malloc( 512 ) ;
        const char* error_info = strerror( my_errno ) ;
        if( error_info )
	    sprintf( *error, "getting socket descriptor: %s", error_info ) ;
	else
	    sprintf( *error, "getting socket descriptor: unknown error" ) ;
	pptcapi_free_connection_struct( connection ) ;
	return 0 ;
    } else {
        long holder ;
        connection->socket = descript;

        //set socket to non-blocking mode
        holder = fcntl( connection->socket, F_GETFL, NULL ) ;
        holder = holder | O_NONBLOCK ;
        fcntl( connection->socket, F_SETFL, holder ) ;
      
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
		    *error = (char *)malloc( 512 ) ;
                    const char *error_info = strerror( my_errno ) ;
                    if( error_info )
			sprintf( *error, "error selecting sockets: %s",
					 error_info ) ;
		    else
			sprintf( *error, "error selecting sockets: unknown" ) ;
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
			*error = (char *)malloc( 512 ) ;
			sprintf( *error, "Did not successfully connect to server. Server may be down or you may be trying on the wrong port" ) ;
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
		*error = (char *)malloc( 512 ) ;
		const char *error_info = strerror( my_errno ) ;
		if( error_info )
		    sprintf( *error, "error connecting to socket: %s",
				     error_info ) ;
		else
		    sprintf( *error, "error connecting to socket: unknown" ) ;
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

    return connection ;
}

