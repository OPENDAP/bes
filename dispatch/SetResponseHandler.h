// SetResponseHandler.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef I_SetResponseHandler_h
#define I_SetResponseHandler_h 1

#include "DODSResponseHandler.h"

/** @brief response handler that creates a container given the symbolic name,
 * real name, and data type.
 *
 * This request handler creates a new container, or replaces an already
 * existing container given the symbolic name, real name (in most cases a file
 * name), and the type of data represented by this container (e.g. netcdf,
 * cedar, cdf, hdf, etc...) The request has the syntax:
 *
 * set container values * &lt;sym_name&gt;,&lt;real_name&gt;,&lt;data_type&gt;;
 *
 * It returns whether the container was created or replaces successfully in an
 * informational response object and transmits that response obje t.
 *
 * @see DODSResponseObject
 * @see DODSContainer
 * @see DODSTransmitter
 * @see DODSTokenizer
 */
class SetResponseHandler : public DODSResponseHandler
{
private:
    string			_symbolic_name ;
    string			_real_name ;
    string			_type ;
    string			_persistence ;
public:
				SetResponseHandler( string name ) ;
    virtual			~SetResponseHandler( void ) ;

    virtual void		parse( DODSTokenizer &tokenizer,
                                       DODSDataHandlerInterface &dhi ) ;
    virtual void		execute( DODSDataHandlerInterface &dhi ) ;
    virtual void		transmit( DODSTransmitter *transmitter,
                                          DODSDataHandlerInterface &dhi ) ;

    static DODSResponseHandler *SetResponseBuilder( string handler_name ) ;
};

#endif // I_SetResponseHandler_h

// $Log: SetResponseHandler.h,v $
// Revision 1.1  2005/02/01 17:48:17  pwest
//
// integration of ESG into opendap
//
