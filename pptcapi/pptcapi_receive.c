// pptcapi_receive.c

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "pptcapi.h"
#include "pptcapi_utils.h"
#include "pptcapi_debug.h"

int pptcapi_receive_buffer_size  = 0 ;
char *pptcapi_receive_buffer  = 0 ;

int
pptcapi_receive( struct pptcapi_connection *connection,
		 struct pptcapi_extensions **extensions,
		 char **buffer, int *len, int *bytes_read,
		 int *bytes_remaining, char **error )
{
    PPTCAPI_DEBUG00( "pptcapi_receive entered\n" )

    *buffer = 0 ;
    *len = 0 ;
    *bytes_read = 0 ;
    int chunk_len = 0 ;

    PPTCAPI_DEBUG01( "  bytes remaining to be read: %d\n",
		     *bytes_remaining )

    if( *bytes_remaining == 0 )
    {
	PPTCAPI_DEBUG00( "  getting ppt header\n" )

	char hdrbuf[PPTCAPI_CHUNK_HEADER_SIZE+1] ;
	int header_read = pptcapi_receive_chunk( connection, hdrbuf,
					         PPTCAPI_CHUNK_HEADER_SIZE,
					         error ) ;

	if( header_read != PPTCAPI_CHUNK_HEADER_SIZE )
	{
	    *error = (char *)malloc( PPTCAPI_ERR_LEN ) ;
	    if( *error )
	    {
		snprintf( *error, PPTCAPI_ERR_LEN,
			  "Failed to read length and type of chunk" ) ;
	    }
	    return PPTCAPI_ERROR ;
	}
	hdrbuf[PPTCAPI_CHUNK_HEADER_SIZE] = '\0' ;
	PPTCAPI_DEBUG01( "  header retrieved: %s\n", hdrbuf )

	char lenbuffer[PPTCAPI_CHUNK_LEN_SIZE+1] ;
	int buf_index = 0 ;
	for( buf_index = 0; buf_index < PPTCAPI_CHUNK_LEN_SIZE; buf_index++ )
	{
	    lenbuffer[buf_index] = hdrbuf[buf_index] ;
	}
	lenbuffer[PPTCAPI_CHUNK_LEN_SIZE] = '\0' ;
	PPTCAPI_DEBUG01( "  length buffer retrieved: %s\n", lenbuffer )

	int hex_result = pptcapi_hexstr_to_i( lenbuffer, &chunk_len, error ) ;
	if( hex_result == PPTCAPI_ERROR )
	{
	    return PPTCAPI_ERROR ;
	}
	PPTCAPI_DEBUG01( "  chunk length: %d\n", chunk_len )
	PPTCAPI_DEBUG01( "  chunk type: %c\n",
			 hdrbuf[PPTCAPI_CHUNK_TYPE_INDEX] )

	if( hdrbuf[PPTCAPI_CHUNK_TYPE_INDEX] == 'x' )
	{
	    return pptcapi_receive_extensions( connection, extensions,
					       chunk_len, error ) ;
	}

	if( hdrbuf[PPTCAPI_CHUNK_TYPE_INDEX] != 'd' )
	{
	    *error = (char *)malloc( PPTCAPI_ERR_LEN ) ;
	    if( *error )
	    {
		snprintf( *error, PPTCAPI_ERR_LEN,
			  "specified chunk type %c should be 'd' or 'x'",
			  hdrbuf[PPTCAPI_CHUNK_TYPE_INDEX] ) ;
	    }
	    return PPTCAPI_ERROR ;
	}
    }
    else
    {
	chunk_len = *bytes_remaining ;
    }

    if( chunk_len == 0 )
    {
	PPTCAPI_DEBUG00( "pptcapi_receive complete and done\n" ) ;
	return PPTCAPI_RECEIVE_DONE ;
    }

    if( !pptcapi_receive_buffer )
    {
	if( !pptcapi_receive_buffer_size )
	{
	    *error = (char *)malloc( PPTCAPI_ERR_LEN ) ;
	    if( *error )
	    {
		snprintf( *error, PPTCAPI_ERR_LEN,
			  "%s, %s", "Unable to create receive buffer",
			  "receive buffer size has not yet been set" ) ;
	    }
	    return PPTCAPI_ERROR ;
	}
	pptcapi_receive_buffer =
	    (char *)malloc(pptcapi_receive_buffer_size+1) ;
	if( !pptcapi_receive_buffer )
	{
	    *error = (char *)malloc( PPTCAPI_ERR_LEN ) ;
	    if( *error )
	    {
		snprintf( *error, PPTCAPI_ERR_LEN,
			  "Unable to allocate memory for receive buffer" ) ;
	    }
	    return PPTCAPI_ERROR ;
	}
    }
    *len = pptcapi_receive_buffer_size + 1 ;

    int read_bytes = 0 ;
    if( chunk_len > pptcapi_receive_buffer_size )
    {
	read_bytes = pptcapi_receive_buffer_size ;
    }
    else
    {
	read_bytes = chunk_len ;
    }
    PPTCAPI_DEBUG01( "  receive %d bytes\n", read_bytes )
    *bytes_read = pptcapi_receive_chunk( connection, pptcapi_receive_buffer,
					 read_bytes,
					 error ) ;
    if( *bytes_read != read_bytes )
    {
	char *old_error = *error ;
	*error = (char *)malloc( PPTCAPI_ERR_LEN ) ;
	if( *error )
	{
	    if( old_error )
	    {
		snprintf( *error, PPTCAPI_ERR_LEN,
			  "Failed to read chunk of size %d, read %d, %s",
			  read_bytes, *bytes_read, old_error ) ;
		free( old_error ) ;
	    }
	    else
	    {
		snprintf( *error, PPTCAPI_ERR_LEN,
			  "Failed to read chunk of size %d, read %d, %s",
			  read_bytes, *bytes_read, "unknown error" ) ;
	    }
	}
	else
	{
	    *error = old_error ;
	}
	*buffer = 0 ;
	*len = 0 ;
	*bytes_read = 0 ;
	return PPTCAPI_ERROR ;
    }
    *buffer = pptcapi_receive_buffer ;
    if( *bytes_read < chunk_len )
    {
	*bytes_remaining = chunk_len - *bytes_read ;
    }
    else
    {
	*bytes_remaining = 0 ;
    }
    PPTCAPI_DEBUG01( "  bytes remaining of chunk to be read: %d\n",
		     bytes_remaining )

    PPTCAPI_DEBUG00( "pptcapi_receive complete\n" ) ;

    return PPTCAPI_OK ;
}

int
pptcapi_receive_chunk( struct pptcapi_connection *connection,
		       char *buffer, int len,
		       char **error )
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
	int new_bytes_read = pptcapi_receive_chunk( connection,
						    new_buffer,
						    new_len, error ) ;
	if( new_bytes_read < 0 )
	{
	    return new_bytes_read ;
	}
	else
	{
	    bytes_read += new_bytes_read ;
	}
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
	*error = (char *)malloc( PPTCAPI_ERR_LEN ) ;
	if( *error )
	{
	    snprintf( *error, PPTCAPI_ERR_LEN,
		      "malformed extension, failed to read the extension" ) ;
	}
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
		unsigned int value_len = (unsigned int)(semi - eq) - 1 ;
		value = (char *)malloc( value_len + 1 ) ;
		if( !value )
		{
		    *error = (char *)malloc( PPTCAPI_ERR_LEN ) ;
		    if( *error )
		    {
			snprintf( *error, PPTCAPI_ERR_LEN,
			      "Unable to create memory for extension value" ) ;
		    }
		    return PPTCAPI_ERROR ;
		}
		strncpy( value, eq+1, value_len ) ;
		value[value_len] = '\0' ;
	    }
	    else
	    {
		value = 0 ;
		eq = semi ;
	    }
	    unsigned int name_len = (unsigned int )(eq - next_extension) ;
	    name = (char *)malloc( name_len + 1 ) ;
	    if( !name )
	    {
		*error = (char *)malloc( PPTCAPI_ERR_LEN ) ;
		if( *error )
		{
		    snprintf( *error, PPTCAPI_ERR_LEN,
			  "Unable to create memory for extension name" ) ;
		}
		return PPTCAPI_ERROR ;
	    }
	    strncpy( name, next_extension, name_len ) ;
	    name[name_len] = '\0' ;
	}
	else
	{
	    *error = (char *)malloc( PPTCAPI_ERR_LEN ) ;
	    if( *error )
	    {
		snprintf( *error, PPTCAPI_ERR_LEN,
			  "malformed extensions, missing semicolon in %s",
			  next_extension ) ;
	    }
	    return PPTCAPI_ERROR ;
	}

	*extensions = (struct pptcapi_extensions *)malloc( sizeof( struct pptcapi_extensions ) ) ;
	if( !*extensions )
	{
	    *error = (char *)malloc( PPTCAPI_ERR_LEN ) ;
	    if( *error )
	    {
		snprintf( *error, PPTCAPI_ERR_LEN,
		      "Unable to create memory for extension" ) ;
	    }
	    return PPTCAPI_ERROR ;
	}
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
	*error = (char *)malloc( PPTCAPI_ERR_LEN ) ;
	if( *error )
	{
	    const char *error_info = strerror( my_errno ) ;
	    if( error_info )
		snprintf( *error, PPTCAPI_ERR_LEN,
			  "socket failure, reading on stream socket: %s",
			  error_info ) ;
	    else
		snprintf( *error, PPTCAPI_ERR_LEN,
			  "socket failure, reading on stream socket: %s",
			  "unknown error" ) ;
	}
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
		*error = (char *)malloc( PPTCAPI_ERR_LEN ) ;
		if( *error )
		{
		    snprintf( *error, PPTCAPI_ERR_LEN,
			      "bad hex value specified %s at %c",
					   hexstr, c ) ;
		}
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

