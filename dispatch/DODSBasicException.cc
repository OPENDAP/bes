// DODSBasicException.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include "DODSBasicException.h"

/** @brief default constructor, error message is set to UNDEFINED
 */
DODSBasicException::DODSBasicException()
{
}

/** @brief constructor that takes an error message to use for this exception
 *
 * @param s error message to use for this exception
 */
DODSBasicException::DODSBasicException( const string &s )
{
    _description = s ;
}

DODSBasicException::~DODSBasicException()
{
}

/** @brief set the error message for this exception to the given string
 *
 * @param s error message to use for this exception
 */
void
DODSBasicException::set_error_description( const string &s )
{
    _description = s ;
}

/** @brief get the error message for this exception and return to caller.
 *
 * @return error message for this exception
 */
string
DODSBasicException::get_error_description()
{
    return _description ;
}

// $Log: DODSBasicException.cc,v $
// Revision 1.3  2004/12/15 17:39:03  pwest
// Added doxygen comments
//
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
