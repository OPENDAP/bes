// pptcapi.c

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
    connection->unix_socket = 0 ;
    connection->temp_socket = 0 ;
    connection->timeout = 0 ;
    connection->ssl_cert_file = 0 ;
    connection->ssl_key_file = 0 ;
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
    int auth_len = strlen( PPTSERVER_AUTHENTICATE ) ;
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
    if( bytes_read == ok_len )
    {
	int cmp = strncmp( inbuf, PPTSERVER_CONNECTION_OK, ok_len ) ;
	if( cmp == 0 )
	{
	    return PPTCAPI_OK ;
	}
    }
    if( bytes_read == auth_len )
    {
	int cmp = strncmp( inbuf, PPTSERVER_AUTHENTICATE, auth_len ) ;
	if( cmp == 0 )
	{
	    return pptcapi_authenticate( connection, error ) ;
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
		if( connection->is_unix_socket )
		{
		    if( connection->temp_socket )
		    {
			if( !access( connection->temp_socket, F_OK ) )
			{
			    remove( connection->temp_socket ) ;
			}
		    }
		}
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

void
pptcapi_free_connection_struct( struct pptcapi_connection *connection )
{
    if( connection )
    {
	connection->is_tcp = 0 ;
	connection->is_unix_socket = 0 ;
	if( connection->host )
	{
	    free( connection->host ) ;
	    connection->host = 0 ;
	}
	connection->port = 0 ;
	if( connection->unix_socket )
	{
	    free( connection->unix_socket ) ;
	    connection->unix_socket = 0 ;
	}
	if( connection->temp_socket )
	{
	    free( connection->temp_socket ) ;
	    connection->temp_socket = 0 ;
	}
	connection->timeout = 0 ;
	connection->socket = 0 ;
	if( connection->ssl_cert_file )
	{
	    free( connection->ssl_cert_file ) ;
	    connection->ssl_cert_file = 0 ;
	}
	if( connection->ssl_key_file )
	{
	    free( connection->ssl_key_file ) ;
	    connection->ssl_key_file = 0 ;
	}

	free( connection ) ;
    }
}

void
pptcapi_free_extensions_struct( struct pptcapi_extensions *extensions )
{
    if( extensions )
    {
	struct pptcapi_extensions *next_extension = extensions->next ;
	if( extensions->name )
	{
	    free( extensions->name ) ;
	    extensions->name = 0 ;
	}
	if( extensions->value )
	{
	    free( extensions->value ) ;
	    extensions->value = 0 ;
	}
	free( extensions ) ;
	pptcapi_free_extensions_struct( next_extension ) ;
    }
}

