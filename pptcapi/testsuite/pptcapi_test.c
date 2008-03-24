#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#include "pptcapi.h"

void
handle_error( char *msg, char *error )
{
    if( error )
    {
	fprintf( stdout, "%s: %s", msg, error ) ;
	free( error ) ;
    }
    else
    {
	fprintf( stdout, "%s: %s", msg, "unknown error" ) ;
    }
}

int
send_command( struct pptcapi_connection *connection, char *cmd, int ofd )
{
    int len = strlen( cmd ) ;
    char *error = 0 ;
    int result = pptcapi_send( connection, cmd, len, &error ) ;
    if( result != PPTCAPI_OK )
    {
	handle_error( "failed to send command", error ) ;
	return 1 ;
    }

    result = PPTCAPI_OK ;
    char *buffer = 0 ;
    int buffer_len = 0 ;
    int bytes_read = 0 ;
    while( result != PPTCAPI_RECEIVE_DONE )
    {
	struct pptcapi_extensions *extensions = 0 ;
	result = pptcapi_receive( connection, &extensions, &buffer,
				  &buffer_len, &bytes_read, &error ) ;
	if( result == PPTCAPI_ERROR )
	{
	    handle_error( "failed to receive response", error ) ;
	    return 1 ;
	}
	/*
	while( extensions )
	{
	    if( extensions->value )
		fprintf( stdout, "%s = %s\n", extensions->name,
					      extensions->value ) ;
	    else
		fprintf( stdout, "%s\n", extensions->name ) ;
	    extensions = extensions->next ;
	}
	*/
	if( result != PPTCAPI_RECEIVE_DONE && bytes_read != 0 )
	{
	    write( ofd, buffer, bytes_read ) ;
	}
    }
    if( buffer )
    {
	free( buffer ) ;
    }

    return 0 ;
}

int
read_commands( struct pptcapi_connection *connection, int ifd, int ofd )
{
    char buffer[80] ;
    int index = 0 ;
    char done = 0 ;
    char c = 0 ;
    while( !done )
    {
	int bytes_read = read( ifd, &c, 1 ) ;
	if( bytes_read < 0 )
	{
	    printf( "failed to read from file\n" ) ;
	    close( ifd ) ;
	    if( ofd != 1 ) close( ofd ) ;
	    return 1 ;
	}
	if( bytes_read == 0 )
	{
	    done = 1 ;
	}
	else
	{
	    if( c == '\n' || c == '\r' )
	    {
		if( index != 0 )
		{
		    buffer[index] = '\0' ;
		    int result = send_command( connection, buffer, ofd ) ;
		    if( result )
		    {
			close( ifd ) ;
			if( ofd != 1 ) close( ofd ) ;
			return result ;
		    }
		}
		index = 0 ;
	    }
	    else
	    {
		buffer[index] = c ;
		index++ ;
	    }
	}
    }
    close( ifd ) ;
    if( ofd != 1 ) close( ofd ) ;
    return 0 ;
}

int
main( int argc, char **argv )
{
    if( argc < 2 || argc > 3 )
    {
	printf( "usage: %s <input_file>\n", argv[0] ) ;
	return 1 ;
    }

    /* open the input file to make sure it exists */
    int ifd = open( argv[1], O_RDONLY, 0 ) ;
    int my_errno = errno ;
    if( !ifd )
    {
	char *err_info = strerror( my_errno ) ;
	if( err_info )
	{
	    printf( "unable to open the input file %s: %s\n",
		    argv[1], err_info );
	}
	else
	{
	    printf( "unable to open the input file %s: %s\n",
		    argv[1], "unknown error" );
	}

	return 1 ;
    }

    int ofd = 1 ;
    if( argc == 3 )
    {
	ofd = open( argv[2], O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR ) ;
	my_errno = errno ;
	if( !ofd )
	{
	    close( ifd ) ;
	    char *err_info = strerror( my_errno ) ;
	    if( err_info )
	    {
		printf( "unable to open the input file %s: %s\n",
			argv[1], err_info );
	    }
	    else
	    {
		printf( "unable to open the input file %s: %s\n",
			argv[1], "unknown error" );
	    }

	    return 1 ;
	}
    }


    char *error = 0 ;

    struct pptcapi_connection *connection =
	pptcapi_tcp_connect( "localhost", 10002, 5, &error ) ;
	//pptcapi_socket_connect( "/tmp/bes.socket", 5, &error ) ;
    if( !connection )
    {
	handle_error( "failed to connect", error ) ;
	close( ifd ) ;
	if( ofd != 1 ) close( ofd ) ;
	return 1 ;
    }

    int result = pptcapi_initialize_connect( connection, &error ) ;
    if( result != PPTCAPI_OK )
    {
	handle_error( "failed to initialize connection", error ) ;
	close( ifd ) ;
	if( ofd != 1 ) close( ofd ) ;
	pptcapi_free_connection_struct( connection ) ;
	return 1 ;
    }

    // if read_commands fails then it will close the file stream
    if( read_commands( connection, ifd, ofd ) )
    {
	pptcapi_free_connection_struct( connection ) ;
	return 1 ;
    }

    result = pptcapi_send_exit( connection, &error ) ;
    if( result != PPTCAPI_OK )
    {
	handle_error( "failed to send exit", error ) ;
	pptcapi_free_connection_struct( connection ) ;
	return 1 ;
    }

    result = pptcapi_close_connection( connection, &error ) ;
    if( result != PPTCAPI_OK )
    {
	handle_error( "failed to close connection", error ) ;
	pptcapi_free_connection_struct( connection ) ;
	return 1 ;
    }

    pptcapi_free_connection_struct( connection ) ;

    return 0 ;
}

