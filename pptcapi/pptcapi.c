#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "pptcapi.h"

int pptcapi_dosend( struct pptcapi_connection *connection,
		    char *buffer, int len, char **error ) ;

int pptcapi_doreceive( struct pptcapi_connection *connection,
		       char *buffer, int len, char **error ) ;

int pptcapi_receive_chunk( struct pptcapi_connection *connection,
			   char *buffer, int len, char **error ) ;

int pptcapi_receive_extensions( struct pptcapi_connection *connection,
			        struct pptcapi_extensions **extensions,
			        int chunk_len, char **error ) ;

int pptcapi_read_extensions( struct pptcapi_extensions **extensions,
			     char *buffer, char **error ) ;

int pptcapi_send_chunk( struct pptcapi_connection *connection,
		        char type, char *buffer, int len, char **error ) ;

int pptcapi_hexstr_to_i( char *hexstr, int *result, char **error ) ;

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
	sprintf( *error, "length of chunk to send %d is too bif", len ) ;
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

int
pptcapi_receive( struct pptcapi_connection *connection,
		 struct pptcapi_extensions **extensions,
		 char **buffer, int *len, int *bytes_read, char **error )
{
    char *inbuf = *buffer ;
    char inbuf_created = 0 ;
    if( !inbuf )
    {
	inbuf = (char *)malloc( PPTCAPI_MAX_BUFFER_SIZE + 1 ) ;
	*buffer = inbuf ;
	*len = PPTCAPI_MAX_BUFFER_SIZE ;
	inbuf_created = 1 ;
    }
    *bytes_read = pptcapi_receive_chunk( connection, inbuf, 5, error ) ;
    if( *bytes_read != 5 )
    {
	if( inbuf_created )
	{
	    free( inbuf ) ;
	    *buffer = 0 ;
	    *len = 0 ;
	}
	*bytes_read = 0 ;
	*error = (char *)malloc( 512 ) ;
	sprintf( *error, "Failed to read length and type of chunk" ) ;
	return PPTCAPI_ERROR ;
    }

    char lenbuffer[5] ;
    lenbuffer[0] = inbuf[0] ;
    lenbuffer[1] = inbuf[1] ;
    lenbuffer[2] = inbuf[2] ;
    lenbuffer[3] = inbuf[3] ;
    lenbuffer[4] = '\0' ;
    int chunk_len = 0 ;
    int hex_result = pptcapi_hexstr_to_i( lenbuffer, &chunk_len, error ) ;
    if( hex_result == PPTCAPI_ERROR )
    {
	if( inbuf_created )
	{
	    free( inbuf ) ;
	    *buffer = 0 ;
	    *len = 0 ;
	}
	*bytes_read = 0 ;
	return PPTCAPI_ERROR ;
    }

    if( inbuf[4] == 'x' )
    {
	if( inbuf_created )
	{
	    free( inbuf ) ;
	    *buffer = 0 ;
	    *len = 0 ;
	}
	*bytes_read = 0 ;
	return pptcapi_receive_extensions( connection, extensions,
					   chunk_len, error ) ;
    }
    else if( inbuf[4] == 'd' )
    {
	if( chunk_len == 0 )
	{
	    if( inbuf_created )
	    {
		free( inbuf ) ;
		*buffer = 0 ;
		*len = 0 ;
	    }
	    *bytes_read = 0 ;
	    return PPTCAPI_RECEIVE_DONE ;
	}
	else
	{
	    *bytes_read = pptcapi_receive_chunk( connection, inbuf,
						 chunk_len, error ) ;
	    if( *bytes_read != chunk_len )
	    {
		if( inbuf_created )
		{
		    free( inbuf ) ;
		    *buffer = 0 ;
		    *len = 0 ;
		}
		*error = (char *)malloc( 512 ) ;
		sprintf( *error, "Failed to read chunk data of size %d",
				 chunk_len ) ;
		return PPTCAPI_ERROR ;
	    }
	}
    }
    else
    {
	if( inbuf_created )
	{
	    free( inbuf ) ;
	    *buffer = 0 ;
	    *len = 0 ;
	}
	*bytes_read = 0 ;
	*error = (char *)malloc( 512 ) ;
	sprintf( *error, "specified chunk type %c should be 'd' or 'x'",
			 inbuf[4] ) ;
	return PPTCAPI_ERROR ;
    }

    return PPTCAPI_OK ;
}

int
pptcapi_receive_chunk( struct pptcapi_connection *connection,
		       char *buffer, int len, char **error )
{
    int bytes_read = pptcapi_doreceive( connection, buffer, len, error ) ;
    if( bytes_read < 0 )
    {
	return bytes_read ;
    }
    if( bytes_read < len )
    {
	char *new_buffer = buffer + bytes_read ;
	int new_len = len - bytes_read ;
	int new_bytes_read = pptcapi_receive_chunk( connection, new_buffer,
						    new_len, error ) ;
	if( new_bytes_read < 0 )
	{
	    return new_bytes_read ;
	}
	else
	bytes_read += new_bytes_read ;
    }

    return bytes_read ;
}

int
pptcapi_receive_extensions( struct pptcapi_connection *connection,
			    struct pptcapi_extensions **extensions,
			    int chunk_len, char **error )
{
    char inbuf[ chunk_len + 1] ;
    int bytes_read = pptcapi_receive_chunk( connection, inbuf,
					    chunk_len, error ) ;
    if( bytes_read != chunk_len )
    {
	*error = (char *)malloc( 512 ) ;
	sprintf( *error, "malformed extension, failed to read the extension" ) ;
	return PPTCAPI_ERROR ;
    }

    inbuf[chunk_len] = '\0' ;

    return pptcapi_read_extensions( extensions, (char *)inbuf, error ) ;
}

int
pptcapi_read_extensions( struct pptcapi_extensions **extensions,
			 char *buffer, char **error )
{
    char *next_extension = buffer ;
    while( next_extension )
    {
	char *name = 0 ;
	char *value = 0 ;
	char *semi = strchr( next_extension, ';' ) ;
	if( semi )
	{
	    char *eq = strchr( next_extension, '=' ) ;
	    if( eq != NULL && eq < semi )
	    {
		int value_len = semi - eq - 1 ;
		value = (char *)malloc( value_len + 1 ) ;
		strncpy( value, eq+1, value_len ) ;
		value[value_len] = '\0' ;
	    }
	    else
	    {
		value = 0 ;
		eq = semi ;
	    }
	    int name_len = eq - next_extension ;
	    name = (char *)malloc( name_len + 1 ) ;
	    strncpy( name, next_extension, name_len ) ;
	    name[name_len] = '\0' ;
	}
	else
	{
	    *error = (char *)malloc( 512 ) ;
	    sprintf( *error, "malformed extensions, missing semicolon in %s",
			     next_extension ) ;
	    return PPTCAPI_ERROR ;
	}

	*extensions = (struct pptcapi_extensions *)malloc( sizeof( struct pptcapi_extensions ) ) ;
	struct pptcapi_extensions *curr_extension = *extensions ;
	curr_extension->name = name ;
	curr_extension->value = value ;
	curr_extension->next = 0 ;
	extensions = &(curr_extension->next) ;

	if( semi[1] == '\0' )
	{
	    next_extension = 0 ;
	}
	else
	{
	    next_extension = semi + 1 ;
	}
    }
    return PPTCAPI_OK ;
}

int
pptcapi_doreceive( struct pptcapi_connection *connection,
		   char *buffer, int len, char **error )
{
    int bytes_read = read( connection->socket, buffer, len ) ;
    int my_errno = errno ;
    if( bytes_read == -1 )
    {
	*error = (char *)malloc( 512 ) ;
	const char *error_info = strerror( my_errno ) ;
	if( error_info )
	    sprintf( *error, "socket failure, reading on stream socket: %s",
			     error_info ) ;
	else
	    sprintf( *error, "socket failure, reading on stream socket: %s",
			     "unknown error" ) ;
    }

    return bytes_read ;
}

int
pptcapi_hexstr_to_i( char *hexstr, int *result, char **error )
{
    *result = 0 ;

    int len = strlen( hexstr ) ;
    int i = 0 ;
    int base = 1 ;
    for( i = len; i > 0; i-- )
    {
	char c = hexstr[i - 1] ;
	if( c < '0' || c > '9' )
	{
	    if( c < 'a' || c > 'f' )
	    {
		*error = (char *)malloc( 512 ) ;
		sprintf( *error, "bad hex value specified %s at %c",
				 hexstr, c ) ;
		return PPTCAPI_ERROR ;
	    }
	}
	int val = 0 ;
	if( c >= '0' && c <= '9' )
	{
	    val = c - '0' ;
	}
	else
	{
	    val = c - 'a' + 10 ;
	}
	*result = *result + ( val * base ) ;
	base = base * 16 ;
    }

    return PPTCAPI_OK ;
}

