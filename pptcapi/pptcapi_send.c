// pptcapi_send.c

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "pptcapi.h"
#include "pptcapi_utils.h"

int
pptcapi_send( struct pptcapi_connection *connection,
	      char *buffer, int len, char **error )
{
    /* if the length len is less than or equal to the max chunk size then we
     * can just send it
     */
    if( len <= PPTCAPI_MAX_BUFFER_SIZE )
    {
	int res = pptcapi_send_chunk( connection, 'd', buffer, len, error ) ;
	if( res != PPTCAPI_OK )
	{
	    return res ;
	}
	return pptcapi_send_chunk( connection, 'd', "", 0, error ) ;
    }

    /* if the length len is greater than the max chunk size then we need to
     * break it apart, send the first chunk size then call this function
     * again with the remainder
     */
    int send_result = pptcapi_send_chunk( connection, 'd', buffer,
					  PPTCAPI_MAX_BUFFER_SIZE, error ) ;
    if( send_result != PPTCAPI_OK )
    {
	return send_result ;
    }

    char *next_buffer = buffer + PPTCAPI_MAX_BUFFER_SIZE ;
    int next_len = len - PPTCAPI_MAX_BUFFER_SIZE ;

    return pptcapi_send( connection, next_buffer, next_len, error ) ;
}

int
pptcapi_send_extensions( struct pptcapi_connection *connection,
			 struct pptcapi_extensions *extensions,
			 char **error )
{
    char inbuf[ PPTCAPI_MAX_BUFFER_SIZE ] ;
    int first = 1 ;
    while( extensions )
    {
	if( extensions->value )
	{
	    if( first )
		sprintf( inbuf, "%s=%s;", extensions->name,
			 extensions->value ) ;
	    else
		sprintf( inbuf, "%s%s=%s;", inbuf, extensions->name,
			 extensions->value ) ;
	}
	else
	{
	    if( first )
		sprintf( inbuf, "%s;", extensions->name ) ;
	    else
		sprintf( inbuf, "%s%s;", inbuf, extensions->name ) ;
	}
	first = 0 ;
	extensions = extensions->next ;
    }

    int xlen = strlen( inbuf ) ;

    int res = pptcapi_send_chunk( connection, 'x', inbuf, xlen, error ) ;
    if( res != PPTCAPI_OK )
    {
	return res ;
    }
    return pptcapi_send_chunk( connection, 'd', 0, 0, error ) ;
}

int
pptcapi_send_exit( struct pptcapi_connection *connection, char **error )
{
    struct pptcapi_extensions extensions ;
    extensions.name = "status" ;
    extensions.value = PPT_EXIT_NOW ;
    extensions.next = 0 ;
    return pptcapi_send_extensions( connection, &extensions, error ) ;
}

int
pptcapi_send_chunk( struct pptcapi_connection *connection,
		    char type, char *buffer, int len, char **error )
{
    if( len > PPTCAPI_MAX_BUFFER_SIZE )
    {
	*error = (char *)malloc( 512 ) ;
	sprintf( *error, "length of chunk to send %d is too big", len ) ;
	return PPTCAPI_ERROR ;
    }

    char header[5] ;
    sprintf( header, "%04x%c", len, type ) ;
    int header_send = pptcapi_dosend( connection, header, 5, error ) ;
    if( header_send != PPTCAPI_OK )
    {
	return header_send ;
    }
    if( len > 0 )
    {
	return pptcapi_dosend( connection, buffer, len, error ) ;
    }

    return PPTCAPI_OK ;
}

int
pptcapi_dosend( struct pptcapi_connection *connection,
		char *buffer, int len, char **error )
{
    int retval = PPTCAPI_OK ;

    int bytes_written = write( connection->socket, buffer, len ) ;
    int my_errno = errno ;
    if( bytes_written == -1 )
    {
	*error = (char *)malloc( 512 ) ;
	const char* error_info = strerror( my_errno ) ;
	if( error_info )
	    sprintf( *error, "socket failure, writing on stream socket: %s",
			     error_info ) ;
	else
	    sprintf( *error, "socket failure, writing on stream socket: %s",
			     "unknown error" ) ;
	retval = PPTCAPI_ERROR ;
    }

    return retval ;
}

