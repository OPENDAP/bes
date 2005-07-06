// GetResponseHandler.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef I_GetResponseHandler_h
#define I_GetResponseHandler_h 1

#include <string>

using std::string ;

#include "DODSResponseHandler.h"

class DODSResponseObject ;

/** @brief handler object that knows how to build the response object for a
 * get command
 *
 * A response handler object is an object that knows how to build a response
 * object, such as a DAS, DDS, DDX response object. It knows how to
 * construct the requested response object, and how to build that object
 * given the list of containers that are a part of the request. A get response
 * handler knows how to build a response object in response to a get request,
 * such as get das, or get dds given the name of a definition and an optional
 * method of returning that response object.
 *
 * A response handler object also knows how to transmit the response object
 * using a DODSTransmitter object.
 *
 * The GetResponseHandler actually builds a sub response handler specific to
 * the response object being requested, such as a DASResponseHandler object
 * and redirects calls to this object to the sub handler.
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

    virtual void		parse( DODSTokenizer &tokenizer,
                                       DODSDataHandlerInterface &dhi ) ;

    virtual void		execute( DODSDataHandlerInterface &dhi ) ;

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
