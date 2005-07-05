// DODSInfo.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef DODSInfo_h_
#define DODSInfo_h_ 1

#include <string>

using std::string ;

#include "DODSResponseObject.h"
#include "cgi_util.h"

/** @brief informational response object
 *
 * This class provides a means to store information about a DODS dataset, such
 * as help information and version information. The retrieval of this
 * information can be buffered until all information is retrieved, or can be
 * directly output thereby not using memory resources.
 *
 * Information is added to this response object through the add_data method
 * and then output using the print method. If the information is not buffered
 * then the information is output during the add_data processing, otherwise
 * the print method performs the output.
 *
 * This class is can not be directly created but simply provides a base class
 * implementation of DODSResponseObject for simple informational responses.
 *
 * @see DODSResponseObject
 */
class DODSInfo :public DODSResponseObject
{
private:
			DODSInfo() {}
protected:
    ostream		*_strm ;
    bool		_buffered ;
    bool		_header ;
    bool		_is_http ;
    ObjectType		_otype ;

    			DODSInfo( ObjectType type ) ;
    			DODSInfo( bool is_http, ObjectType type ) ;

    virtual void	initialize( string key ) ;
public:
    virtual		~DODSInfo() ;

    virtual void 	add_data( const string &s ) ;
    virtual void 	add_data_from_file( const string &key,
                                            const string &srvr_name ) ;
    virtual void 	print( FILE *out ) ;
    /** @brief return whether the information is to be buffered or not.
     *
     * @return true if information is buffered, false if not
     */
    virtual bool	is_buffered() { return _buffered ; }
    /** @brief returns the type of data
     *
     * @return type of data
     */
    virtual ObjectType	type() { return _otype ; }
};

#endif // DODSInfo_h_

// $Log: DODSInfo.h,v $
// Revision 1.4  2005/04/07 19:55:17  pwest
// added add_data_from_file method to allow for information to be added from a file, for example when adding help information from a file
//
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
