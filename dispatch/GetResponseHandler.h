// GetResponseHandler.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef I_GetResponseHandler_h
#define I_GetResponseHandler_h 1

#include <string>

using std::string ;

#include "DODSResponseHandler.h"

class DODSResponseObject ;

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
 * @see GetResponseHandlerList
 * @see DODSTransmitter
 */
class GetResponseHandler : public DODSResponseHandler
{
private:
    DODSResponseHandler	*	_sub_response ;
public:
				GetResponseHandler( string name ) ;
    virtual			~GetResponseHandler(void) ;

    virtual DODSResponseObject  *get_response_object() ;

    /** @brief knows how to parse a get request
     *
     * This class knows how to parse a get request, building a response
     * handler that actually knows how to build the requested response
     * object, such as das, dds, data, ddx, etc...
     *
     * @param tokenizer holds on to the list of tokens to be parsed
     * @throws DODSParserException if there is a problem parsing the request
     */
    virtual void		parse( DODSTokenizer &tokenizer,
                                       DODSDataHandlerInterface &dhi ) ;

    /** @brief knows how to build a requested response object
     *
     * Derived instances of this base class know how to build a
     * specific response object, which containers to use, which request
     * handlers to go to, etc... This response handler invokes the execute
     * method on the stored derived response handler.
     *
     * @param dhi structure that holds request and response information
     * @throws DODSResponseException if there is a problem building the
     * response object
     * @see _DODSDataHandlerInterface
     * @see DODSResponseObject
     */
    virtual void		execute( DODSDataHandlerInterface &dhi ) ;

    /** @brief transmit the respobse object built by the execute command
     * using the specified transmitter object
     *
     * THis base class simply hands off the transmit request to the stored
     * response handler.
     *
     * @param transmitter
     * @param dhi
     * @see DODSResponseObject
     * @see DODSTransmitter
     * @see _DODSDataHandlerInterface
     */
    virtual void		transmit( DODSTransmitter *transmitter,
                                          DODSDataHandlerInterface &dhi ) ;

    virtual void		set_response_object( DODSResponseObject *o ) ;

    static DODSResponseHandler *GetResponseBuilder( string handler_name ) ;
};

#endif // I_GetResponseHandler_h

// $Log: GetResponseHandler.h,v $
// Revision 1.2  2005/03/26 00:33:33  pwest
// fixed aggregation server invoke problems. Other commands use the aggregation command but no aggregation is needed, so set aggregation to empty string when done
//
// Revision 1.1  2005/02/01 17:48:17  pwest
//
// integration of ESG into opendap
//
