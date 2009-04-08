// pptcapi_authenticate.c

#include <stdlib.h>
#include <stdio.h>

#include "pptcapi.h"
#include "pptcapi_utils.h"

int
pptcapi_authenticate( struct pptcapi_connection *connection, char **error )
{
    *error = (char *)malloc( PPTCAPI_ERR_LEN ) ;
    if( *error )
    {
	snprintf( *error, PPTCAPI_ERR_LEN,
		  "SSL Authentication not yet implemented in PPT C API" ) ;
    }
    return PPTCAPI_ERROR ;

    // ask the server what the server port is
    // receive the server port
    // set up ssl (see ppt/SSLClient.cc)
}

