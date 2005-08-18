// DeleteResponseHandler.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include "DeleteResponseHandler.h"
#include "DODSTokenizer.h"
#include "DODSTextInfo.h"
#include "TheDefineList.h"
#include "DODSDefine.h"
#include "ThePersistenceList.h"
#include "DODSContainerPersistence.h"
#include "DODSContainer.h"
#include "OPeNDAPDataNames.h"

DeleteResponseHandler::DeleteResponseHandler( string name )
    : DODSResponseHandler( name )
{
}

DeleteResponseHandler::~DeleteResponseHandler( )
{
}

/** @brief executes the command to delete a container, a definition, or all
 * definitions.
 *
 * Removes definitions from TheDefineList and containers from volatile
 * container persistence object, which is found in ThePersistenceList.
 *
 * The response object built is a DODSTextInfo object. Status of the delition
 * will be added to the informational object, one of:
 *
 * Successfully deleted all definitions
 * <BR />
 * Successfully deleted definition "&lt;_def_name&gt;"
 * <BR />
 * Definition "&lt;def_name&gt;" does not exist.  Unable to delete.
 * <BR />
 * Successfully deleted container "&lt;container_name&gt;" from persistent store "volatile"
 * <BR />
 * Unable to delete container. The container "&lt;container_name&gt;" does not exist in the persistent store "volatile"
 * <BR />
 * The persistence store "volatile" does not exist. Unable to delete container "&lt;container_name&gt;"
 *
 * @param dhi structure that holds request and response information
 * @throws DODSResponseException if there is a problem building the
 * response object
 * @see _DODSDataHandlerInterface
 * @see DODSTextInfo
 * @see TheDefineList
 * @see DODSDefine
 * @see ThePersistenceList
 * @see DODSContainerPersistence
 */
void
DeleteResponseHandler::execute( DODSDataHandlerInterface &dhi )
{
    DODSTextInfo *info = new DODSTextInfo( dhi.transmit_protocol == "HTTP" ) ;
    _response = info ;

    if( dhi.data[DEFINITIONS] == "true" )
    {
	TheDefineList->remove_defs() ;
	info->add_data( "Successfully deleted all definitions\n" ) ;
    }
    else if( dhi.data[DEF_NAME] != "" )
    {
	bool deleted = TheDefineList->remove_def( dhi.data[DEF_NAME] ) ;
	if( deleted == true )
	{
	    string line = (string)"Successfully deleted definition \""
	                  + dhi.data[DEF_NAME]
			  + "\"\n" ;
	    info->add_data( line ) ;
	}
	else
	{
	    string line = (string)"Definition \""
	                  + dhi.data[DEF_NAME]
			  + "\" does not exist.  Unable to delete.\n" ;
	    info->add_data( line ) ;
	}
    }
    else if( dhi.data[STORE_NAME] != "" && dhi.data[CONTAINER_NAME] != "" )
    {
	DODSContainerPersistence *cp =
		ThePersistenceList->find_persistence( dhi.data[STORE_NAME] ) ;
	if( cp )
	{
	    bool deleted =  cp->rem_container( dhi.data[CONTAINER_NAME] ) ;
	    if( deleted == true )
	    {
		string line = (string)"Successfully deleted container \""
		              + dhi.data[CONTAINER_NAME]
			      + "\" from persistent store \""
			      + dhi.data[STORE_NAME]
			      + "\"\n" ;
		info->add_data( line ) ;
	    }
	    else
	    {
		string line = (string)"Unable to delete container. "
		              + "The container \""
		              + dhi.data[CONTAINER_NAME]
			      + "\" does not exist in the persistent store \""
			      + dhi.data[STORE_NAME]
			      + "\"\n" ;
		info->add_data( line ) ;
	    }
	}
	else
	{
	    string line = (string)"The persistence store \""
	                  + dhi.data[STORE_NAME]
			  + "\" does not exist. "
			  + "Unable to delete container \""
			  + dhi.data[CONTAINER_NAME]
			  + "\"\n" ;
	    info->add_data( line ) ;
	}
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
DeleteResponseHandler::transmit( DODSTransmitter *transmitter,
                               DODSDataHandlerInterface &dhi )
{
    if( _response )
    {
	DODSTextInfo *info = dynamic_cast<DODSTextInfo *>(_response) ;
	transmitter->send_text( *info, dhi );
    }
}

DODSResponseHandler *
DeleteResponseHandler::DeleteResponseBuilder( string handler_name )
{
    return new DeleteResponseHandler( handler_name ) ;
}

// $Log: DeleteResponseHandler.cc,v $
// Revision 1.1  2005/03/17 19:26:22  pwest
// added delete command to delete a specific container, a specific definition, or all definitions
//
