// pptcapi_debug.h

#ifndef pptcapi_debug_h
#define pptcapi_debug_h 1

#include <stdio.h>

#include "pptcapi.h"

extern unsigned char pptcapi_debugging ;
extern FILE *pptcapi_debug_location ;

extern unsigned char pptcapi_is_debugging() ;
extern FILE *pptcapi_get_debug_location() ;

#define PPTCAPI_DEBUG00( format ) { if( pptcapi_is_debugging() ) fprintf( pptcapi_get_debug_location(), format ) ; }
#define PPTCAPI_DEBUG01( format, p1 ) { if( pptcapi_is_debugging() ) fprintf( pptcapi_get_debug_location(), format, p1 ) ; }
#define PPTCAPI_DEBUG02( format, p1, p2 ) { if( pptcapi_is_debugging() ) fprintf( pptcapi_get_debug_location(), format, p1, p2 ) ; }
#define PPTCAPI_DEBUG03( format, p1, p2, p3 ) { if( pptcapi_is_debugging() ) fprintf( pptcapi_get_debug_location(), format, p1, p2, p3 ) ; }
#define PPTCAPI_DEBUG04( format, p1, p2, p3, p4 ) { if( pptcapi_is_debugging() ) fprintf( pptcapi_get_debug_location(), format, p1, p2, p3, p4 ) ; }


#endif // pptcapi_debug_h
