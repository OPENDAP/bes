// DODSResponseHandler.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef I_DODSResponseHandler_h
#define I_DODSResponseHandler_h 1

#include <string>

using std::string ;

#include "DODSDataHandlerInterface.h"
#include "DODSTransmitter.h"

class DODSResponseObject ;
class DODSTokenizer ;

/** @brief handler object that knows how to build a specific response object
 *
 * A response handler object is an object that knows how to build a response
 * object, such as a DAS, DDS, DDX response object. It knows how to
 * construct the requested response object, and how to build that object
 * given the list of containers that are a part of the request.
 *
 * A response handler object also knows how to transmit the response object
 * using a DODSTransmitter object.
 *
 * This is an abstract base class for response handlers. Derived classes
 * implement the methods execute and transmit.
 *
 * @see DODSResponseObject
 * @see DODSContainer
 * @see DODSRequestHandler
 * @see DODSResponseHandlerList
 * @see DODSTransmitter
 * @see DODSTokenizer
 */
class DODSResponseHandler {
protected:
    string			_response_name ;
    DODSResponseObject		*_response ;
public:
				DODSResponseHandler( string name ) ;
    virtual			~DODSResponseHandler(void) ;

    virtual DODSResponseObject  *get_response_object() ;
    // FIX: CLUDGE
    virtual void		set_response_object( DODSResponseObject *o ) ;

    /** @brief parse the request to be used to build this response
     *
     * Derived instances of this abstract base class know how to parse a
     * request string to determine how to build the response object.
     *
     * @param tokenizer holds the list of tokens to use to build the request
     * plan
     * @throws DODSParserException if there is a problem building the
     * request plan.
     * @see DODSTokenizer
     * @see _DODSDataHandlerInterface
     */
    virtual void		parse( DODSTokenizer &tokenizer,
                                       DODSDataHandlerInterface &dhi ) = 0 ;

    /** @brief knows how to build a requested response object
     *
     * Derived instances of this abstract base class know how to build a
     * specific response object, which containers to use, which request
     * handlers to go to.
     *
     * @param dhi structure that holds request and response information
     * @throws DODSResponseException if there is a problem building the
     * response object
     * @see _DODSDataHandlerInterface
     * @see DODSResponseObject
     */
    virtual void		execute( DODSDataHandlerInterface &dhi ) = 0 ;

    /** @brief transmit the respobse object built by the execute command
     * using the specified transmitter object
     *
     * @param transmitter
     * @param dhi
     * @see DODSResponseObject
     * @see DODSTransmitter
     * @see _DODSDataHandlerInterface
     */
    virtual void		transmit( DODSTransmitter *transmitter,
                                          DODSDataHandlerInterface &dhi ) = 0 ;

    /** @brief return the name of this response object, e.g. das
     *
     * This name is used to determine which response handler can handle a
     * requested responose, such as das, dds, ddx, tab, info, version, help,
     * etc...
     *
     * @return response name
     */
    virtual string		get_name( ) { return _response_name ; }
};

#endif // I_DODSResponseHandler_h

// $Log: DODSResponseHandler.h,v $
// Revision 1.5  2005/03/15 19:58:35  pwest
// using DODSTokenizer to get first and next tokens
//
// Revision 1.4  2005/02/01 17:48:17  pwest
//
// integration of ESG into opendap
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
