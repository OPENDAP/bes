#ifndef pptcapi_h
#define pptcapi_h 1

/* maximum type sizes and default values */
#define PPTCAPI_MAX_STR_LEN 256
#define PPTCAPI_MAX_BUFFER_SIZE 4096
#define PPTCAPI_MAX_TIMEOUT 5

/* ppt protocol tokens */
#define PPT_PROTOCOL_UNDEFINED "PPT_PROTOCOL_UNDEFINED"
#define PPT_EXIT_NOW "PPT_EXIT_NOW"
#define PPTCLIENT_TESTING_CONNECTION "PPTCLIENT_TESTING_CONNECTION"
#define PPTCLIENT_REQUEST_AUTHPORT "PPTCLIENT_REQUEST_AUTHPORT"
#define PPTSERVER_CONNECTION_OK "PPTSERVER_CONNECTION_OK"
#define PPTSERVER_AUTHENTICATE "PPTSERVER_AUTHENTICATE"

/* pptcapi return values */
#define PPTCAPI_OK 1
#define PPTCAPI_ERROR 2
#define PPTCAPI_RECEIVE_DONE 3

struct pptcapi_connection {
    int				is_tcp ;
    int				is_unix_socket ;
    char *			host ;
    int				port ;
    int				timeout ;
    int				socket ;
};

struct pptcapi_extensions {
    char *			name ;
    char *			value ;
    struct pptcapi_extensions *	next ;
};

void pptcapi_initialize_connection_struct( struct pptcapi_connection *connection ) ;

struct pptcapi_connection *pptcapi_tcp_connect( const char *host,
					        int portval,
					        int timeout,
						char **error ) ;

struct pptcapi_connection *pptcapi_socket_connect( const char *unix_socket,
						   int timeout,
						   char **error ) ;

int pptcapi_initialize_connect( struct pptcapi_connection *connection,
			        char **error ) ;

int pptcapi_close_connection( struct pptcapi_connection *connection,
			      char **error ) ;

int pptcapi_send( struct pptcapi_connection *connection,
		  char *buffer, int len, char **error ) ;

int pptcapi_send_extensions( struct pptcapi_connection *connection,
			     struct pptcapi_extensions *extensions,
			     char **error ) ;

int pptcapi_send_exit( struct pptcapi_connection *connection,
		       char **error ) ;

int pptcapi_receive( struct pptcapi_connection *connection,
		     struct pptcapi_extensions **extensions,
		     char **fd, int *len, int *bytes_read, char **error ) ;

#endif // pptcapi_h

