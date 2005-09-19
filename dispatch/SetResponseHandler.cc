// SetResponseHandler.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include "SetResponseHandler.h"
#include "DODSTextInfo.h"
#include "DODSParserException.h"
#include "DODSTokenizer.h"
#include "DODSContainerPersistenceList.h"
#include "DODSContainerPersistence.h"
#include "DODSContainerPersistenceException.h"
#include "OPeNDAPDataNames.h"

SetResponseHandler::SetResponseHandler( string name )
    : DODSResponseHandler( name )
{
}

SetResponseHandler::~SetResponseHandler( )
{
}

/** @brief executes the command to create a new container or replaces an
 * already existing container based on the provided symbolic name.
 *
 * The symbolic name is first looked up in the volatile persistence object. If
 * the symbolic name already exists (the container already exists) then the
 * already existing container is removed and the new one added using the given
 * real name (usually a file name) and the type of data represented by this
 * container (e.g. cedar, cdf, netcdf, hdf, etc...)
 *
 * An informational response object DODSTextInfo is created to hold whether or
 * not the container was successfully added/replaced. Possible responses are:
 *
 * Successfully added container &lt;sym_name&gt; to persistent store volatile
 * Successfully replaced container &lt;sym_name&gt; to persistent store volatile
 *
 * Unable to add container &lt;sym_name&gt; to persistent store volatile
 *
 * @param dhi structure that holds request and response information
 * @throws DODSResponseException if there is a problem building the
 * response object
 * @see _DODSDataHandlerInterface
 * @see DODSTextInfo
 * @see DODSContainerPersistenceList
 * @see DODSContainerPersistence
 * @see DODSContainer
 */
void
SetResponseHandler::execute( DODSDataHandlerInterface &dhi )
{
    DODSTextInfo *info = new DODSTextInfo( dhi.transmit_protocol == "HTTP" ) ;
    _response = info ;

    string store_name = dhi.data[STORE_NAME] ;
    string symbolic_name = dhi.data[SYMBOLIC_NAME] ;
    string real_name = dhi.data[REAL_NAME] ;
    string container_type = dhi.data[CONTAINER_TYPE] ;
    DODSContainerPersistence *cp =
	DODSContainerPersistenceList::TheList()->find_persistence( store_name );
    if( cp )
    {
	try
	{
	    string def_type = "added" ;
	    bool deleted = cp->rem_container( symbolic_name ) ;
	    if( deleted == true )
	    {
		def_type = "replaced" ;
	    }
	    cp->add_container( symbolic_name, real_name, container_type ) ;
	    string ret = (string)"Successfully " + def_type + " container "
			 + symbolic_name + " to persistent store "
			 + store_name + "\n" ;
	    info->add_data( ret ) ;
	}
	catch( DODSContainerPersistenceException &e )
	{
	    string ret = (string)"Unable to add container "
			 + symbolic_name + " to persistent store "
			 + store_name + "\n" ;
	    info->add_data( ret ) ;
	    info->add_data( e.get_error_description() ) ;
	}
    }
    else
    {
	string ret = (string)"Unable to add container "
		     + symbolic_name + " to persistent store "
		     + store_name + "\n" ;
	info->add_data( ret ) ;
	info->add_data( (string)"Persistence store " + store_name + " does not exist" ) ;
    }
}

/** @brief transmit the response object built by the execute command
 * using the specified transmitter object
 *
 * If a response object was built then transmit it as text.
 *
 * @param transmitter object that knows how to transmit specific basic types
 * @param dhi structure that holds the request and response information
 * @see DODSTextInfo
 * @see DODSTransmitter
 * @see _DODSDataHandlerInterface
 */
void
SetResponseHandler::transmit( DODSTransmitter *transmitter,
                                  DODSDataHandlerInterface &dhi )
{
    if( _response )
    {
	DODSTextInfo *info = dynamic_cast<DODSTextInfo *>(_response) ;
	transmitter->send_text( *info, dhi ) ;
    }
}

DODSResponseHandler *
SetResponseHandler::SetResponseBuilder( string handler_name )
{
    return new SetResponseHandler( handler_name ) ;
}

// $Log: SetResponseHandler.cc,v $
// Revision 1.5  2005/03/17 19:23:58  pwest
// deleting the container in rem_container instead of returning the removed container, returning true if successfully removed and false otherwise
//
// Revision 1.4  2005/03/15 19:58:35  pwest
// using DODSTokenizer to get first and next tokens
//
// Revision 1.3  2005/02/10 18:48:23  pwest
// missing escaped quote in response text
//
// Revision 1.2  2005/02/02 00:03:13  pwest
// ability to replace containers and definitions
//
// Revision 1.1  2005/02/01 17:48:17  pwest
//
// integration of ESG into opendap
//
