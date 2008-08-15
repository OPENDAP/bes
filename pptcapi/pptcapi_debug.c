// pptcapi_debug.c

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "pptcapi.h"
#include "pptcapi_debug.h"

unsigned char pptcapi_debugging = 0 ;
FILE *pptcapi_debug_location = 0 ;
unsigned char pptcapi_debug_location_created = 0 ;

char *
pptcapi_debug_on( char *location )
{
    // if debugging is on then we want to clean up and restart debugging
    pptcapi_debug_off() ;

    pptcapi_debug_location_created = 0 ;
    pptcapi_debug_location = 0 ;
    if( !location )
    {
	pptcapi_debug_location = stderr ;
    }
    else if( !strncmp( location, "stderr", 6 ) )
    {
	pptcapi_debug_location = stderr ;
    }
    else
    {
	pptcapi_debug_location = fopen( location, "w" ) ;
	int myerrno = errno ;
	if( !pptcapi_debug_location )
	{
	    char *serr = strerror( myerrno ) ;
	    char *error = (char *)malloc( 512 ) ;
	    if( serr )
	    {
		sprintf( error, "Failed to open debug file: %s",
				serr ) ;
	    }
	    else
	    {
		sprintf( error, "Failed to open debug file: %s",
				"unkown error" ) ;
	    }
	    return error ;
	}
	pptcapi_debug_location_created = 1 ;
    }
    pptcapi_debugging = 1 ;
    return 0 ;
}

void
pptcapi_debug_off()
{
    if( pptcapi_debugging )
    {
	if( pptcapi_debug_location_created )
	{
	    fclose( pptcapi_debug_location ) ;
	    pptcapi_debug_location_created = 0 ;
	}
	pptcapi_debugging = 0 ;
    }
}

unsigned char
pptcapi_is_debugging()
{
    return pptcapi_debugging ;
}

FILE *
pptcapi_get_debug_location()
{
    return pptcapi_debug_location ;
}

