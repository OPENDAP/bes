// pptcapi.h

#ifndef pptcapi_h
#define pptcapi_h 1

/** @brief
 *
 * pptcapi is the c client api to the BES using the PPT protocol (point to
 * point transport). The following is a simple transaction between the
 * client and the server
<pre>
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
main( int argc, char **argv )
{
    char *error = 0 ;

    struct pptcapi_connection *connection =
	pptcapi_tcp_connect( "localhost", 10002, 5, &error ) ;
	//pptcapi_socket_connect( "/tmp/bes.socket", 5, &error ) ;
    if( !connection )
    {
	handle_error( "failed to connect", error ) ;
	return 1 ;
    }

    int result = pptcapi_initialize_connect( connection, &error ) ;
    if( result != PPTCAPI_OK )
    {
	handle_error( "failed to initialize connection", error ) ;
	pptcapi_free_connection_struct( connection ) ;
	return 1 ;
    }

    char cmd[64] ;
    sprintf( cmd, "show version;" ) ;
    int len = strlen( cmd ) ;
    result = pptcapi_send( connection, cmd, len, &error ) ;
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
	    pptcapi_free_connection_struct( connection ) ;
	    return 1 ;
	}

	struct pptcapi_extensions *next_extension = extensions ;
	while( next_extension )
	{
	    if( extensions->value )
		fprintf( stdout, "%s = %s\n", extensions->name,
					      extensions->value ) ;
	    else
		fprintf( stdout, "%s\n", extensions->name ) ;
	    next_extension = next_extension->next ;
	}

	pptcapi_free_extensions_struct( extensions ) ;
	extensions = 0 ;
	if( bytes_read != 0 )
	{
	    write( 1, buffer, bytes_read ) ;
	}
    }

    if( buffer )
    {
	free( buffer ) ;
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

</pre>
 */

/* maximum type sizes and default values */
#define PPTCAPI_MAX_STR_LEN 256
#define PPTCAPI_MAX_BUFFER_SIZE 65535
#define PPTCAPI_MAX_TIMEOUT 5

/* chunk header sizes ... still need to change code in pptcapi_send in the
 * sprintf call that creates the header */
#define PPTCAPI_CHUNK_LEN_SIZE 7
#define PPTCAPI_CHUNK_TYPE_SIZE 1
#define PPTCAPI_CHUNK_TYPE_INDEX 7
#define PPTCAPI_CHUNK_HEADER_SIZE PPTCAPI_CHUNK_LEN_SIZE + PPTCAPI_CHUNK_TYPE_SIZE

/* ppt protocol tokens */
#define PPT_PROTOCOL_UNDEFINED "PPT_PROTOCOL_UNDEFINED"
#define PPT_EXIT_NOW "PPT_EXIT_NOW"
#define PPTCLIENT_TESTING_CONNECTION "PPTCLIENT_TESTING_CONNECTION"
#define PPTCLIENT_REQUEST_AUTHPORT "PPTCLIENT_REQUEST_AUTHPORT"
#define PPTSERVER_CONNECTION_OK "PPTSERVER_CONNECTION_OK"
#define PPTSERVER_AUTHENTICATE "PPTSERVER_AUTHENTICATE"

/** @brief pptcapi funciton return values
 *
 * <pre>
 * PPTCAPI_OK		if function completes succesfully
 * PPTCAPI_ERROR	if function encounters an error
 * PPTCAPI_RECEIVE_DONE returned from a call to pptcapi_receive when the
 *                      last chunk of information is received.
 * </pre>
 */
#define PPTCAPI_OK 1
#define PPTCAPI_ERROR 2
#define PPTCAPI_RECEIVE_DONE 3

/** @brief connection structure used by pptcapi.
 *
 * Calling pptcapi_socket_connect or pptcapi_tcp_connect you will get this
 * structure back with your connection information. Use this structure when
 * making calls to send and receive. When done with processing use the
 * function pptcapi_free_connection_struct to free the structure.
 *
 * is_tcp - set to 1 if this is a connection using tcp
 * is_unix_socket - set to 1 if this is a connection using a unix socket
 * host - string representing the name of the machine you wish to connect to
 * port - port number of the BES server on the specified host
 * unix_socket - full path to the unix socket used in socket sonnections
 * temp_socket - 
 * timeout - how long to wait before timing out on connect
 * socket - tcp socket of connection to BES server
 * ssl_cert_file - file with ssl certificate information
 * ssl_key_file - file with ssl connection keys
 */
struct pptcapi_connection {
    int				is_tcp ;
    int				is_unix_socket ;
    char *			host ;
    int				port ;
    char *			unix_socket ;
    char *			temp_socket ;
    int				timeout ;
    int				socket ;

    char *			ssl_cert_file ;
    char *			ssl_key_file ;
};

/** @brief structure holding extensions sent or received by this client
 *
 * A chunk of data can either contain actual data (a data response for
 * example) or extensions, which are name/value pairs. This structure holds
 * one extension with a pointer to the next extension. The last extension in
 * the linked list will have next = 0;
 *
 * name - name of extension
 * value - string value of the extension
 * next - pointer to the next extension on the linked list, 0 if no more.
 */
struct pptcapi_extensions {
    char *			name ;
    char *			value ;
    struct pptcapi_extensions *	next ;
};

/** @brief connect to a remote BES server using a tcp connection
 *
 * @param host name of the host machine trying to attach to
 * @param portval port of the server on the specified host
 * @param timeout how long to wait before timing out on connect
 * @param error out variable representing any errors received on connect
 * @return pptcapi_connection structure if connection successful, 0 if not
 * with error set
 */
struct pptcapi_connection *pptcapi_tcp_connect( const char *host,
					        int portval,
					        int timeout,
						char **error ) ;

/** @brief connect to a local BES server using a unix socket
 *
 * @param unix_socket path to the unix socket special file
 * @param timeout how long to wait before timing out on connect
 * @param error out variable representing any errors received on connect
 * @return pptcapi_connection structure if connection successful, 0 if not
 * with error set
 */
struct pptcapi_connection *pptcapi_socket_connect( const char *unix_socket,
						   int timeout,
						   char **error ) ;

/** @brief once connected to the server, initialize the connection
 *
 * This function is called after calling either pptcapi_socket_connect or
 * pptcapi_tcp_connect. It is used to initialize the connection with the BES
 * using the PPT connection handshake.
 *
 * @param connection pptcapi_connection structure returned from a call to
 * pptcapi_socket_connect or pptcapi_tcp_connect
 * @param error out variable representing any errors received during
 * connection initialization.
 * @return pptcapi function return values specified above
 */
int pptcapi_initialize_connect( struct pptcapi_connection *connection,
			        char **error ) ;

/** @brief close the specified connection to a BES
 *
 * This function closes a connection to a BES as specified by the
 * pptcapi_connection structure
 *
 * @param connection pptcapi_connection structure representing the
 * connection to the BES
 * @param error out variable representing any errors received during the
 * closing of the connection
 * @return pptcapi function return values specified above
 */
int pptcapi_close_connection( struct pptcapi_connection *connection,
			      char **error ) ;

/** @brief frees the connection structure created during a call to
 * pptcapi_socket_connect or pptcapi_tcp_connect
 *
 * @param connection pptcapi_connection structure to free
 */
void pptcapi_free_connection_struct( struct pptcapi_connection *connection ) ;

/** @brief frees an extension structure pptcapi_extensions
 *
 * Iterates through the linked list of extensions, the first extension
 * pointed to by extensions, and frees the name and value values.
 *
 * @param extensions linked list of pptcapi_extensions structures to free
 */
void pptcapi_free_extensions_struct( struct pptcapi_extensions *extensions ) ;

/** @brief send the command buffer to the BES
 *
 * This function is called by a BES client in order to send a command buffer
 * to the BES. The command should be a null terminated string
 *
 * @param connection pptcapi_connection structure representing the
 * connection to the BES
 * @param buffer null terminated command buffer containing the commands to
 * be sent to the BES
 * @param len length of the command string in buffer
 * @param error out variable representing any errors received during the
 * send of the buffer
 * @return pptcapi function return values specified above
 */
int pptcapi_send( struct pptcapi_connection *connection,
		  char *buffer, int len, char **error ) ;

/** @brief send extensions (name/value pairs) to the BES
 *
 * @param connection pptcapi_connection structure representing the
 * connection to the BES
 * @param extensions linked list of pptcapi_extensions structures
 * representing name/value pairs to send to the BES
 * @param error out variable representing any errors received during the
 * send of the extensions
 * @return pptcapi function return values specified above
 */
int pptcapi_send_extensions( struct pptcapi_connection *connection,
			     struct pptcapi_extensions *extensions,
			     char **error ) ;

/** @brief let the server know that the client is exiting
 *
 * Sends an exit command to the server to let it know that the client is
 * exiting. After calling this funciton you need to free the connection
 * structure.
 *
 * @param connection pptcapi_connection structure representing the
 * connection to the BES
 * @param error out variable representing any errors received during the
 * send of the exit extension
 * @return pptcapi function return values specified above
 */
int pptcapi_send_exit( struct pptcapi_connection *connection,
		       char **error ) ;

/** @brief receive a chunk of information or list of extensions from the BES
 *
 * This function is called repeatedly by the client until
 * PPTCAPI_RECEIVE_DONE is returned. The receive calls receives a chunk of
 * information from the BES, not necessarily the entire response from the
 * BES. This response could be data or could be extensions, but not both. If
 * extensions is 0 upon return then a buffer of data was received.
 *
 * @param connection pptcapi_connection structure representing the
 * connection to the BES
 * @param extensions out variable representing any extensions received from
 * the BES. Either extensions is set or fd is set, not both.
 * @param fd out variable representing any data received from the BES.
 * Either fd is set or extensions is set, not both.
 * @param len out variable representing the length of the buffer pointed to
 * by fd returned from the call
 * @param bytes_read out variable representing the number of bytes read in
 * this call. Might be less than len.
 * @param error out variable representing any errors received during the
 * receive of extensions or data
 * @return pptcapi function return values specified above. Returns
 * PPTCAPI_RECEIVE_DONE when the entire response has been read.
 */
int pptcapi_receive( struct pptcapi_connection *connection,
		     struct pptcapi_extensions **extensions,
		     char **fd, int *len, int *bytes_read, char **error ) ;

#endif // pptcapi_h

