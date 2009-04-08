// pptcapi.h

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004-2009 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.edu> and Jose Garcia <jgarcia@ucar.edu>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301

// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#ifndef pptcapi_h
#define pptcapi_h 1

/** @mainpage
 *
 * The BES PPT C API is a C API for communicating with an OPeNDAP Back-End
 * Server (BES). PPT stands for Point to Point Transport.
 *
 * For more information please refer to the document at
 * http://docs.opendap.org entited Hyrax - BES PPT.
 *
 * In general, a client first does some handshaking with the BES using PPT
 * to establish a connection, which could include SSL authentication. Once
 * connected, requests and responses are sent and received using an
 * HTTP-like chunking scheme, where the first part of the buffer contains
 * the length of the chunk and the type of the chunk, followed by that much
 * data. A request to the BES includes closing the connection.
 *
 * For an example of using the PPT C API you can the namespace pptcapi
 * included in this documentation. This example is also available on the
 * OPeNDAP documentation wiki.
 */

/** @namespace pptcapi
 *  @brief The BES PPT C API is a C API for communication to an OPeNDAP
 *  Back-End Server (BES)
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

    // for debugging you would call the following function
    // error = pptcapi_debug_on( "./pptcapi.debug" ) ;
    // if( error )
    // {
    //     handle_error( "failed to turn debugging on", error ) ;
    //     return 1 ;
    // }

    // advanced tcp tuning to maximize tcp communication
    int tcp_receive_buffer_size = 0 ;
    int tcp_send_buffer_size = 0 ;
    int connection_timeout = 5 ;
    struct pptcapi_connection *connection =
	pptcapi_tcp_connect( "localhost", 10002, connection_timeout,
	                     tcp_receive_buffer_size,
			     tcp_send_buffer_size,
			     &error ) ;
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

    char cmd[128] ;
    sprintf( cmd, "<?xml version=\"1.0\" encoding=\"UTF-8\"?><request reqID=\"some_unique_value\" ><showVersion /></request>" ) ;
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
	    pptcapi_free_connection_struct( connection ) ;
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
		    DO SOME ERROR PROCESSING HERE
		}
	    }
	    next_extension = next_extension->next ;
	}
	pptcapi_free_extensions_struct( extensions ) ;
	extensions = 0 ;

	if( bytes_read != 0 )
	{
	    write( 1, buffer, bytes_read ) ;
	}
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

    // if debugging was turned on you would need to turn it off here
    // pptcapi_debug_off() ;

    return 0 ;
}

</pre>
 */

/** @brief maximum string length */
#define PPTCAPI_MAX_STR_LEN 256
/** @brief default send and receive tcp buffer sizes */
#define PPTCAPI_DEFAULT_BUFFER_SIZE 65535
/** @brief maximum timeout for making connection to the BES */
#define PPTCAPI_MAX_TIMEOUT 5
/** @brief maximum error message string length */
#define PPTCAPI_ERR_LEN 512

/* chunk header sizes ... still need to change code in pptcapi_send in the
 * sprintf call that creates the header */
#define PPTCAPI_CHUNK_LEN_SIZE 7
#define PPTCAPI_CHUNK_TYPE_SIZE 1
#define PPTCAPI_CHUNK_TYPE_INDEX 7
#define PPTCAPI_CHUNK_HEADER_SIZE PPTCAPI_CHUNK_LEN_SIZE + PPTCAPI_CHUNK_TYPE_SIZE

/**
 * @defgroup PPT Protocol Tokens for initial handshake
 */
/*@{*/

#define PPT_PROTOCOL_UNDEFINED "PPT_PROTOCOL_UNDEFINED"
#define PPT_EXIT_NOW "PPT_EXIT_NOW"
#define PPTCLIENT_TESTING_CONNECTION "PPTCLIENT_TESTING_CONNECTION"
#define PPTCLIENT_REQUEST_AUTHPORT "PPTCLIENT_REQUEST_AUTHPORT"
#define PPTSERVER_CONNECTION_OK "PPTSERVER_CONNECTION_OK"
#define PPTSERVER_AUTHENTICATE "PPTSERVER_AUTHENTICATE"

/*@}*/

/**
 * @defgroup PPT C API Status Extension Macros
 *
 * Extensions are variable/value pairs passed in a chunk and can contain
 * such things as a status code.
 */
/*@{*/

#define PPT_STATUS_EXT "status"
#define PPT_STATUS_EXT_LEN 6
#define PPT_STATUS_ERR "error"
#define PPT_STATUS_ERR_LEN 5

/*@}*/

/**
 * @defgroup PPT C API function return values
 *
 * <pre>
 * PPTCAPI_OK		if function completes succesfully
 * PPTCAPI_ERROR	if function encounters an error
 * PPTCAPI_RECEIVE_DONE returned from a call to pptcapi_receive when the
 *                      last chunk of information is received.
 * </pre>
 */
/*@{*/

#define PPTCAPI_OK 1
#define PPTCAPI_ERROR 2
#define PPTCAPI_RECEIVE_DONE 3

/*@}*/

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
    char			is_tcp ;
    char			is_unix_socket ;
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
 * System administrators can use the tcp_recv_buffer_size and
 * tcp_send_buffer_size parameters to tune the buffer sizes used by pptcapi
 * in receiving and sending data respectively. Please refer to online
 * documentation regarding tcp tuning for more information. If the
 * parameters are set to 0 then pptcapi will determine the current set
 * window sizes in setting the buffer sizes.
 *
 * @param host name of the host machine trying to attach to
 * @param portval port of the server on the specified host
 * @param timeout how long to wait before timing out on connect
 * @param tcp_recv_buffer_size tcp tuning allows you to set buffer sizes to
 * maximize performance
 * @param tcp_send_buffer_size tcp tuning allows you to set buffer sizes to
 * maximize performance
 * @param error out variable representing any errors received on connect
 * @return pptcapi_connection structure if connection successful, 0 if not
 * with error set
 */
struct pptcapi_connection *pptcapi_tcp_connect( const char *host,
					        int portval,
					        int timeout,
						int tcp_recv_buffer_size,
						int tcp_send_buffer_size,
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
 * @param bytes_remaining in/out variable representing the number of bytes
 * remaining to be read for this chunk.
 * @param error out variable representing any errors received during the
 * receive of extensions or data
 * @return pptcapi function return values specified above. Returns
 * PPTCAPI_RECEIVE_DONE when the entire response has been read.
 */
int pptcapi_receive( struct pptcapi_connection *connection,
		     struct pptcapi_extensions **extensions,
		     char **fd, int *len, int *bytes_read,
		     int *bytes_remaining, char **error ) ;

/** @brief turn debugging on in pptcapi and dump to specified location
 *
 * The location string can be set to stderr to have debugging information
 * dump to the standard error or to a file name, such as ./pptcapi.debug. If
 * debugging was already turned on then we switch to the specified location,
 * if different.
 *
 * @param location where to dump debugging information to
 */
char *pptcapi_debug_on( char *location ) ;

/** @brief turn debugging off in pptcapi
 *
 */
void pptcapi_debug_off() ;

#endif // pptcapi_h

