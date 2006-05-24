// SetContainerResponseHandler.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.org> and Jose Garcia <jgarcia@ucar.org>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// 
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301
 
// (c) COPYRIGHT University Corporation for Atmostpheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include "SetContainerResponseHandler.h"
#include "DODSInfo.h"
#include "OPeNDAPSilentInfo.h"
#include "ContainerStorageList.h"
#include "ContainerStorage.h"
#include "ContainerStorageException.h"
#include "OPeNDAPDataNames.h"

SetContainerResponseHandler::SetContainerResponseHandler( string name )
    : DODSResponseHandler( name )
{
}

SetContainerResponseHandler::~SetContainerResponseHandler( )
{
}

/** @brief executes the command to create a new container or replaces an
 * already existing container based on the provided symbolic name.
 *
 * The symbolic name is first looked up in the specified container storage
 * object, volatile if no container storage name is specified. If the
 * symbolic name already exists (the container already exists) then the
 * already existing container is removed and the new one added using the
 * given real name (usually a file name) and the type of data represented
 * by this container (e.g. cedar, cdf, netcdf, hdf, etc...)
 *
 * An informational response object DODSInfo is created to hold whether or
 * not the container was successfully added/replaced. Possible responses are:
 *
 * Successfully added container "&lt;sym_name&gt;" in container storage "&lt;store_name&gt;"
 * <BR />
 * <BR />
 * Successfully replaced container "&lt;sym_name&gt;" in container storage "&lt;store_name&gt;"
 * <BR />
 * <BR />
 * Unable to add container "&lt;sym_name&gt;" to container storage "&lt;store_name&gt;"
 * <BR />
 * <BR />
 * Unable to add container "&lt;sym_name&gt;" to container storage "&lt;store_name&gt;"
 * <BR />
 * Container storage "&lt;store_name&gt;" does not exist
 *
 * @param dhi structure that holds request and response information
 * @throws DODSHandlerException if there is a problem building the
 * response object
 * @throws DODSResponseException upon fatal error building the response
 * object
 * @see _DODSDataHandlerInterface
 * @see DODSInfo
 * @see ContainerStorageList
 * @see ContainerStorage
 * @see DODSContainer
 */
void
SetContainerResponseHandler::execute( DODSDataHandlerInterface &dhi )
{
    DODSInfo *info = 0 ;
    if( dhi.data[SILENT] == "yes" )
    {
	info = new OPeNDAPSilentInfo() ;
    }
    else
    {
	info = new DODSInfo( dhi.transmit_protocol == "HTTP" ) ;
    }
    _response = info ;

    string store_name = dhi.data[STORE_NAME] ;
    string symbolic_name = dhi.data[SYMBOLIC_NAME] ;
    string real_name = dhi.data[REAL_NAME] ;
    string container_type = dhi.data[CONTAINER_TYPE] ;
    ContainerStorage *cp =
	ContainerStorageList::TheList()->find_persistence( store_name );
    if( cp )
    {
	try
	{
	    string def_type = "added" ;
	    bool deleted = cp->del_container( symbolic_name ) ;
	    if( deleted == true )
	    {
		def_type = "replaced" ;
	    }
	    cp->add_container( symbolic_name, real_name, container_type ) ;
	    string ret = (string)"Successfully " + def_type + " container \""
			 + symbolic_name + "\" in container storage \""
			 + store_name + "\"\n" ;
	    info->add_data( ret ) ;
	}
	catch( ContainerStorageException &e )
	{
	    string ret = (string)"Unable to add container \""
			 + symbolic_name + "\" to container storage \""
			 + store_name + "\"\n" ;
	    info->add_data( ret ) ;
	    info->add_data( e.get_error_description() ) ;
	}
    }
    else
    {
	string ret = (string)"Unable to add container \""
		     + symbolic_name + "\" to container storage \""
		     + store_name + "\"\n" ;
	info->add_data( ret ) ;
	info->add_data( (string)"Container storage \"" + store_name + "\" does not exist" ) ;
    }
}

/** @brief transmit the response object built by the execute command
 * using the specified transmitter object
 *
 * If a response object was built then transmit it as text using the specified
 * transmitter object.
 *
 * @param transmitter object that knows how to transmit specific basic types
 * @param dhi structure that holds the request and response information
 * @see DODSInfo
 * @see DODSTransmitter
 * @see _DODSDataHandlerInterface
 */
void
SetContainerResponseHandler::transmit( DODSTransmitter *transmitter,
                                  DODSDataHandlerInterface &dhi )
{
    if( _response )
    {
	DODSInfo *info = dynamic_cast<DODSInfo *>(_response) ;
	transmitter->send_text( *info, dhi ) ;
    }
}

DODSResponseHandler *
SetContainerResponseHandler::SetContainerResponseBuilder( string handler_name )
{
    return new SetContainerResponseHandler( handler_name ) ;
}

