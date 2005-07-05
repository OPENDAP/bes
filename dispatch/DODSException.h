// DODSException.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef DODSException_h_
#define DODSException_h_ 1

#include <string>

using std::string ;

/** @brief Abstract exception class for OpenDAP with basic string message
 *
 */
class DODSException
{
protected:
    string		_description;
public:
    			DODSException() { _description = "UNDEFINED" ; }
    virtual		~DODSException() {}

    /** @brief set the error message for this exception
     *
     * @param s message string
     */
    virtual void	set_error_description( const string &s ) = 0 ;
    /** @brief get the error message for this exception
     *
     * @return error message
     */
    virtual string	get_error_description() = 0 ;
};

#endif // DODSException_h_ 

// $Log: DODSException.h,v $
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
