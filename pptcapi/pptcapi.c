#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "pptcapi.h"

void
pptcapi_initialize_connection_struct( struct pptcapi_connection *connection )
{
    connection->is_tcp = 0 ;
    connection->is_unix_socket = 0 ;
    connection->host = 0 ;
    connection->port = 0 ;
    connection->timeout = 0 ;
}

int
pptcapi_initialize_connect( struct pptcapi_connection *connection,
			    char **error )
{
    char *tosend = PPTCLIENT_TESTING_CONNECTION ;
    int send_len = strlen( tosend ) ;
    int retval = pptcapi_dosend( connection, tosend, send_len, error ) ;
    if( retval == PPTCAPI_ERROR )
    {
	return PPTCAPI_ERROR ;
    }

    char inbuf[ PPTCAPI_MAX_BUFFER_SIZE + 1 ] ;
    int rec_len = PPTCAPI_MAX_BUFFER_SIZE ;
    int bytes_read = pptcapi_doreceive( connection, inbuf, rec_len, error ) ;
    if( bytes_read < 1 )
    {
	return PPTCAPI_ERROR ;
    }
    if( bytes_read > PPTCAPI_MAX_BUFFER_SIZE )
	bytes_read = PPTCAPI_MAX_BUFFER_SIZE ;
    inbuf[bytes_read] = '\0' ;

    int undef_len = strlen( PPT_PROTOCOL_UNDEFINED ) ;
    int ok_len = strlen( PPTSERVER_CONNECTION_OK ) ;
    if( bytes_read == undef_len )
    {
	int cmp = strncmp( inbuf, PPT_PROTOCOL_UNDEFINED, undef_len ) ;
	if( cmp == 0 )
	{
	    *error = (char *)malloc( 512 ) ;
	    sprintf( *error, "Could not connect to server, server may be down or busy" ) ;
	    return PPTCAPI_ERROR ;
	}
    }
    /*
    FIXME
    if( status == PPTProtocol::PPTSERVER_AUTHENTICATE )
    {
	authenticateWithServer() ;
    }
    */
    if( bytes_read == ok_len )
    {
	int cmp = strncmp( inbuf, PPTSERVER_CONNECTION_OK, ok_len ) ;
	if( cmp == 0 )
	{
	    return PPTCAPI_OK ;
	}
    }
    *error = (char *)malloc( 512 ) ;
    sprintf( *error, "Server reported an invalid connection \"%s\"",
		     inbuf ) ;

    return PPTCAPI_ERROR ;
}

int
pptcapi_close_connection( struct pptcapi_connection *connection,
			  char **error )
{
    int retval = PPTCAPI_ERROR ;
    if( connection && ( connection->is_tcp || connection->is_unix_socket ) )
    {
	if( connection->socket )
	{
	    int res = close( connection->socket ) ;
	    int my_errno = errno ;
	    if( res == -1 )
	    {
		*error = (char *)malloc( 512 ) ;
		char *str_err = strerror( my_errno ) ;
		if( str_err )
		    sprintf( *error, "Failed to close socket: %s", str_err ) ;
		else
		    sprintf( *error, "Failed to close socket: unknown error" ) ;
	    }
	    else
	    {
		retval = PPTCAPI_OK ;
	    }
	}
	else
	{
	    *error = (char *)malloc( 512 ) ;
	    sprintf( *error, "Attempting to close a 0 socket" ) ;
	}
    }
    else if( !connection )
    {
	*error = (char *)malloc( 512 ) ;
	sprintf( *error, "Attempting to close connection not created" ) ;
    }
    else
    {
	*error = (char *)malloc( 512 ) ;
	sprintf( *error, "Attempting to close an incomplete connection" ) ;
    }
    return retval ;
}

