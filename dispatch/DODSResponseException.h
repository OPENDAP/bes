// DODSResponseException.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef DODSResponseException_h_
#define DODSResponseException_h_ 1

#include "DODSBasicException.h"

/** @brief an exception object representing any exceptions thrown during the
 * building of a response object in a response handler
 *
 * A DODSResponseException can be built by either passing the error string
 * to the constructor or by instantiating an empty object and using the
 * derived set_error_description method.
 *
 * To retreive the error string simpy use the get_error_description method.
 *
 * @see DODSBasicException
 * @see DODSResponseHandler
 * @see DODSResponseObject
 */
class DODSResponseException: public DODSBasicException
{
public:
      			DODSResponseException() {}
      			DODSResponseException( const string &s )
			    : DODSBasicException( s ) {}
      virtual		~DODSResponseException() {}
};

#endif // DODSResponseException_h_ 

// $Log: DODSResponseException.h,v $
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
