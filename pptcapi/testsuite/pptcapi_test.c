#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
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
    int bytes_remaining = 0 ;
    while( result != PPTCAPI_RECEIVE_DONE )
    {
	struct pptcapi_extensions *extensions = 0 ;
	result = pptcapi_receive( connection, &extensions, &buffer,
				  &buffer_len, &bytes_read,
				  &bytes_remaining, &error ) ;
	if( result == PPTCAPI_ERROR )
	{
	    handle_error( "failed to receive response", error ) ;
	    return 1 ;
	}

	int got_error = 0 ;
	struct pptcapi_extensions *next_extension = extensions ;
	while( next_extension && !got_error )
	{
	    if( extensions->name &&
	        !strncmp( extensions->name, PPT_STATUS_EXT, PPT_STATUS_EXT_LEN))
	    {
		if( extensions->value &&
		    !strncmp( extensions->value, PPT_STATUS_ERR,
		    PPT_STATUS_ERR_LEN ) )
		{
		    got_error = 1 ;
		    /* do some error processing here, but continue to call
		     * pptcapi_receive in order to get the error document
		     * from the BES */
		}
	    }
	    next_extension = next_extension->next ;
	}
	pptcapi_free_extensions_struct( extensions ) ;
	extensions = 0 ;
	if( result != PPTCAPI_RECEIVE_DONE && bytes_read != 0 )
	{
	    write( ofd, buffer, bytes_read ) ;
	}
    }

    return 0 ;
}

int
read_commands( struct pptcapi_connection *connection, int ifd, int ofd )
{
    char buffer[4096] ;
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
	    buffer[index] = c ;
	    index++ ;
	}
    }
    int result = 0 ;
    if( index != 0 )
    {
	buffer[index] = '\0' ;
	result = send_command( connection, buffer, ofd ) ;
    }
    close( ifd ) ;
    if( ofd != 1 ) close( ofd ) ;
    return result ;
}

int
main( int argc, char **argv )
{
    if( argc < 2 || argc > 4 )
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

    /*
    error = pptcapi_debug_on( "./pptcapi.debug" ) ;
    if( error )
    {
	handle_error( "failed to turn debugging on", error ) ;
	fclose( ifd ) ;
	if( ofd != 1 ) close( ofd ) ;
	return 1 ;
    }
    */

    int tcp_rcv_buffer_size = 70000 ;
    int tcp_send_buffer_size = 70000 ;
    int timeout = 5 ;
    struct pptcapi_connection *connection =
	pptcapi_tcp_connect( "localhost", 10002, timeout,
			     tcp_rcv_buffer_size,
			     tcp_send_buffer_size,
			     &error ) ;
	//pptcapi_socket_connect( "/tmp/bes.socket", 5, &error ) ;
    if( !connection )
    {
	handle_error( "failed to connect", error ) ;
	close( ifd ) ;
	if( ofd != 1 ) close( ofd ) ;
	//pptcapi_debug_off() ;
	return 1 ;
    }

    int result = pptcapi_initialize_connect( connection, &error ) ;
    if( result != PPTCAPI_OK )
    {
	handle_error( "failed to initialize connection", error ) ;
	close( ifd ) ;
	if( ofd != 1 ) close( ofd ) ;
	pptcapi_free_connection_struct( connection ) ;
	//pptcapi_debug_off() ;
	return 1 ;
    }

    // if read_commands fails then it will close the file stream
    if( read_commands( connection, ifd, ofd ) )
    {
	pptcapi_free_connection_struct( connection ) ;
	//pptcapi_debug_off() ;
	return 1 ;
    }

    result = pptcapi_send_exit( connection, &error ) ;
    if( result != PPTCAPI_OK )
    {
	handle_error( "failed to send exit", error ) ;
	pptcapi_free_connection_struct( connection ) ;
	//pptcapi_debug_off() ;
	return 1 ;
    }

    result = pptcapi_close_connection( connection, &error ) ;
    if( result != PPTCAPI_OK )
    {
	handle_error( "failed to close connection", error ) ;
	pptcapi_free_connection_struct( connection ) ;
	//pptcapi_debug_off() ;
	return 1 ;
    }

    pptcapi_free_connection_struct( connection ) ;

    //pptcapi_debug_off() ;

    return 0 ;
}

