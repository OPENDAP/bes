// pptcapi_socket_connect.c

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>   // for unlink
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>

#include "pptcapi.h"
#include "pptcapi_utils.h"
#include "pptcapi_debug.h"

extern void pptcapi_initialize_connection_struct( struct pptcapi_connection *connection ) ;

extern int pptcapi_receive_buffer_size ;
extern char *pptcapi_receive_buffer ;

extern int pptcapi_send_buffer_size ;
extern char *pptcapi_send_buffer ;

char *pptcapi_temp_name() ;

char *pptcapi_ltoa( long value, char *buffer, int base ) ;

struct pptcapi_connection *
pptcapi_socket_connect( const char *unix_socket, int timeout, char **error )
{
    PPTCAPI_DEBUG00( "socket connect entered\n" ) ;

    if( !unix_socket )
    {
	*error = (char *)malloc( 512 ) ;
	sprintf( *error, "No unix socket specified for unix connection" ) ;
	return 0 ;
    }
    PPTCAPI_DEBUG02( "  unix socket connect to socket %s, timeout %d\n",
		     unix_socket, timeout ) ;

    int unix_socket_len = strlen( unix_socket ) ;
    if( unix_socket_len > PPTCAPI_MAX_STR_LEN )
    {
	*error = (char *)malloc( 512 ) ;
	sprintf( *error, "path to unix socket %s is too long", unix_socket ) ;
	return 0 ;
    }

    struct sockaddr_un client_addr ;
    struct sockaddr_un server_addr ;

    // what is the max size of the path to the unix socket
    unsigned int max_len = sizeof( client_addr.sun_path ) ;

    char path[107] = "" ;
    getcwd( path, sizeof( path ) ) ;
    char temp_socket[ PPTCAPI_MAX_STR_LEN ] ;
    sprintf( temp_socket, "%s/%s.unixSocket", path, pptcapi_temp_name() ) ;
    int temp_socket_len = strlen( temp_socket ) ;

    // maximum path for struct sockaddr_un.sun_path is 108
    // get sure we will not exceed to max for creating sockets
    // 107 characters in pathname + '\0'
    if( temp_socket_len > max_len - 1 )
    {
	*error = (char *)malloc( 512 ) ;
	sprintf( *error, "path to temporary unix socket %s is too long",
			 temp_socket ) ;
	return 0 ;
    }
    if( unix_socket_len > max_len - 1 )
    {
	*error = (char *)malloc( 512 ) ;
	sprintf( *error, "path to unix socket %s is too long",
			 unix_socket ) ;
	return 0 ;
    }

    // create the pptcapi_connection structure
    void *vconnection = malloc( sizeof( struct pptcapi_connection ) ) ;
    struct pptcapi_connection *connection =
	(struct pptcapi_connection *)vconnection ;
    pptcapi_initialize_connection_struct( connection ) ;

    // set the unix socket flag
    connection->is_unix_socket = 1 ;

    // save off the unix socket path
    connection->unix_socket = (char *)malloc( unix_socket_len + 1 ) ;
    strncpy( connection->unix_socket, unix_socket, unix_socket_len ) ;
    connection->unix_socket[unix_socket_len] = '\0' ;

    // save off the temp socket path
    connection->temp_socket = (char *)malloc( temp_socket_len + 1 ) ;
    strncpy( connection->temp_socket, temp_socket, temp_socket_len ) ;
    connection->temp_socket[temp_socket_len] = '\0' ;

    strncpy( server_addr.sun_path, unix_socket, unix_socket_len ) ;
    server_addr.sun_path[unix_socket_len] = '\0';
    server_addr.sun_family = AF_UNIX ;

    int descript = socket( AF_UNIX, SOCK_STREAM, 0 ) ;
    int myerrno = errno ;
    if( descript != -1 )
    {
	strncpy( client_addr.sun_path, temp_socket, temp_socket_len ) ;
	client_addr.sun_path[temp_socket_len] = '\0';
	client_addr.sun_family = AF_UNIX ;

	int clen = sizeof( client_addr.sun_family ) ;
	clen += strlen( client_addr.sun_path )  + 1;

	int ret = bind( descript, (struct sockaddr*)&client_addr, clen + 1) ;
	myerrno = errno ;
	if( ret != -1 )
	{
	    int slen = sizeof( server_addr.sun_family ) ;
	    slen += strlen( server_addr.sun_path) + 1;

	    ret = connect( descript, (struct sockaddr*)&server_addr, slen );
	    myerrno = errno ;
	    if( ret != -1)
	    {
		connection->socket = descript ;
	    }
	    else
	    {
		*error = (char *)malloc( 512 ) ;
		char *err_info = strerror( myerrno ) ;
		if( err_info )
		{
		    sprintf( *error, "could not connect via unix socket %s: %s",
				     unix_socket, err_info ) ;
		}
		else
		{
		    sprintf( *error, "could not connect via unix socket %s: %s",
				     unix_socket, "unknown error" ) ;
		}
		pptcapi_free_connection_struct( connection ) ;
		return 0 ;
	    }
	}
	else
	{
	    *error = (char *)malloc( 512 ) ;
	    char *err_info = strerror( myerrno ) ;
	    if( err_info )
	    {
		sprintf( *error, "could not bind to unix socket %s: %s",
				 temp_socket, err_info ) ;
	    }
	    else
	    {
		sprintf( *error, "could not bind to unix socket %s: %s",
				 temp_socket, "unknown error" ) ;
	    }
	    pptcapi_free_connection_struct( connection ) ;
	    return 0 ;
	}
    }
    else
    {
	*error = (char *)malloc( 512 ) ;
	char *err_info = strerror( myerrno ) ;
	if( err_info )
	{
	    sprintf( *error, "could not create a unix socket: %s", err_info ) ;
	}
	else
	{
	    sprintf( *error, "could not create a unix socket: unknown error" ) ;
	}
	pptcapi_free_connection_struct( connection ) ;
	return 0 ;
    }

    PPTCAPI_DEBUG01( "  socket connect: receive/send buffer sizes set to %d\n",
		     PPTCAPI_DEFAULT_BUFFER_SIZE ) ;
    pptcapi_receive_buffer_size = PPTCAPI_DEFAULT_BUFFER_SIZE ;
    pptcapi_send_buffer_size = PPTCAPI_DEFAULT_BUFFER_SIZE ;

    PPTCAPI_DEBUG00( "socket connect complete\n" ) ;
}

char *
pptcapi_temp_name()
{
    char *retbuf = (char *)malloc( PPTCAPI_MAX_STR_LEN ) ;
    char tempbuf1[50] ;
    pptcapi_ltoa( getpid(), tempbuf1, 10 ) ;
    char tempbuf2[50] ;
    unsigned int t = time( NULL ) - 1000000000 ;
    pptcapi_ltoa( t, tempbuf2, 10 ) ;
    sprintf( retbuf, "%s_%s", tempbuf1, tempbuf2 ) ;
    return retbuf ;
}

char *
pptcapi_ltoa( long val, char *buf, int base )
{
    ldiv_t r ;

    if( base > 36 || base < 2 ) // no conversion if wrong base
    {
	*buf = '\0' ;
	return buf ;
    }
    if(val < 0 )
    *buf++ = '-' ;
    r = ldiv( labs( val ), base ) ;

    /* output digits of val/base first */
    if( r.quot > 0 )
	buf = pptcapi_ltoa( r.quot, buf, base ) ;

    /* output last digit */
    *buf++ = "0123456789abcdefghijklmnopqrstuvwxyz"[(int)r.rem] ;
    *buf = '\0' ;
    return buf ;
}

