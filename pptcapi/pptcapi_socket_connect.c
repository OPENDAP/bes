// pptcapi_socket_connect.c

#include "pptcapi.h"

struct pptcapi_connection *
pptcapi_socket_connect( const char *unix_socket, int timeout, char **error )
{
    *error = (char *)malloc( 512 ) ;
    sprintf( *error, "Unix Socket connect not yet implemented in PPT C API" ) ;
    return PPTCAPI_ERROR ;
}

