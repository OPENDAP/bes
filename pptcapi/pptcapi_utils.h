// pptcapi_h

#ifndef pptcapi_utils_h
#define pptcapi_utils_h 1

#include "pptcapi.h"

int pptcapi_dosend( struct pptcapi_connection *connection,
		    char *buffer, int len, char **error ) ;

int pptcapi_send_chunk( struct pptcapi_connection *connection,
		        char type, char *buffer, int len, char **error ) ;

int pptcapi_send_extensions( struct pptcapi_connection *connection,
			     struct pptcapi_extensions *extensions,
			     char **error ) ;

int pptcapi_doreceive( struct pptcapi_connection *connection,
		       char *buffer, int len, char **error ) ;

int pptcapi_receive_chunk( struct pptcapi_connection *connection,
			   char *buffer, int len, char **error ) ;

int pptcapi_receive_extensions( struct pptcapi_connection *connection,
			        struct pptcapi_extensions **extensions,
			        int chunk_len, char **error ) ;

int pptcapi_read_extensions( struct pptcapi_extensions **extensions,
			     char *buffer, char **error ) ;

int pptcapi_hexstr_to_i( char *hexstr, int *result, char **error ) ;

#endif // pptcapi_utils_h
