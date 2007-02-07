// BESExceptionManager.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.edu> and Jose Garcia <jgarcia@ucar.edu>
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

#include "BESExceptionManager.h"

#include "BESDatabaseException.h"
#include "BESContainerStorageException.h"
#include "BESKeysException.h"
#include "BESLogException.h"
#include "BESHandlerException.h"
#include "BESIncorrectRequestException.h"
#include "BESResponseException.h"
#include "BESTransmitException.h"
#include "BESAggregationException.h"

#include "BESStatusReturn.h"
#include "TheBESKeys.h"
#include "BESInfoList.h"

#define DEFAULT_ADMINISTRATOR "support@unidata.ucar.edu"

BESExceptionManager *BESExceptionManager::_instance = 0 ;

BESExceptionManager::BESExceptionManager()
{
}

BESExceptionManager::~BESExceptionManager()
{
}

void
BESExceptionManager::add_ehm_callback( p_bes_ehm ehm )
{
    _ehm_list.push_back( ehm ) ;
}

/** @brief Manage any exceptions thrown during the whole process

    Specific responses are generated given a specific Exception caught.

    @param e excption to be managed
    @param dhi information related to request and response
    @return status after exception is handled
    @see BESException
    @see BESIncorrectException
    @see BESDatabaseException
    @see BESContainerStorageException
    @see BESKeysException
    @see BESLogException
    @see BESHandlerException
    @see BESResponseException
 */
int
BESExceptionManager::handle_exception( BESException &e,
				       BESDataHandlerInterface &dhi )
{
    // Let's see if any of these exception callbacks can handle the
    // exception. The first callback that can handle the exception wins
    ehm_iter i = _ehm_list.begin() ;
    for( ; i != _ehm_list.end(); i++ )
    {
	p_bes_ehm p = *i ;
	int handled = p( e, dhi ) ;
	if( handled )
	{
	    return handled ;
	}
    }

    dhi.error_info = BESInfoList::TheList()->build_info() ;
    string action_name = dhi.action_name ;
    if( action_name == "" )
	action_name = "BES" ;
    dhi.error_info->begin_response( action_name ) ;

    string administrator = "" ;
    try
    {
	bool found = false ;
	administrator =
	    TheBESKeys::TheKeys()->get_key( "BES.ServerAdministrator", found ) ;
    }
    catch( ... )
    {
	administrator = DEFAULT_ADMINISTRATOR ;
    }
    dhi.error_info->add_tag( "Administrator", administrator ) ;
    dhi.error_info->add_exception( e ) ;
    dhi.error_info->end_response() ;
    return e.get_return_code() ;

    /* Replaced with the above three lines
    BESIncorrectRequestException *ireqx=dynamic_cast<BESIncorrectRequestException*>(&e);
    if( ireqx )
    {
	dhi.error_info->add_exception( "Request", e ) ;
	dhi.error_info->end_response() ;
	return BES_REQUEST_INCORRECT;
    }
    BESDatabaseException *ce=dynamic_cast<BESDatabaseException*>(&e);
    if(ce)
    {
	dhi.error_info->add_exception( "Database", e ) ;
	dhi.error_info->end_response() ;
	return BES_DATABASE_FAILURE;
    }
    BESContainerStorageException *dpe=dynamic_cast<BESContainerStorageException*>(&e);
    if(dpe)
    {
	dhi.error_info->add_exception( "ContainerStore", e ) ;
	dhi.error_info->end_response() ;
	return BES_CONTAINER_PERSISTENCE_ERROR;
    }  
    BESKeysException *keysex=dynamic_cast<BESKeysException*>(&e);
    if(keysex)
    {
	dhi.error_info->add_exception( "Configuration", e ) ;
	dhi.error_info->end_response() ;
	return BES_INITIALIZATION_FILE_PROBLEM;
    }  
    BESLogException *logex=dynamic_cast<BESLogException*>(&e);
    if(logex)
    {
	dhi.error_info->add_exception( "Log", e ) ;
	dhi.error_info->end_response() ;
	return BES_LOG_FILE_PROBLEM;
    }
    BESHandlerException *hanex=dynamic_cast <BESHandlerException*>(&e);
    if(hanex)
    {
	dhi.error_info->add_exception( "DataHandlerProblem", e ) ;
	dhi.error_info->end_response() ;
	return BES_DATA_HANDLER_PROBLEM;
    }
    BESResponseException *ranex=dynamic_cast <BESResponseException*>(&e);
    if(ranex)
    {
	dhi.error_info->add_exception( "DataHandlerFailure", e ) ;
	dhi.error_info->end_response() ;
	return BES_DATA_HANDLER_FAILURE;
    }
    BESTransmitException *transex=dynamic_cast <BESTransmitException*>(&e);
    if(transex)
    {
	dhi.error_info->add_exception( "TransmitProblem", e ) ;
	dhi.error_info->end_response() ;
	return BES_DATA_HANDLER_PROBLEM;
    }
    BESAggregationException *aanex=dynamic_cast <BESAggregationException*>(&e);
    if(aanex)
    {
	dhi.error_info->add_exception( "Aggregation", e ) ;
	dhi.error_info->end_response() ;
	return BES_AGGREGATION_EXCEPTION ;
    }

    dhi.error_info->add_exception( "Unknown", e ) ;
    dhi.error_info->end_response() ;
    return BES_TERMINATE_IMMEDIATE;
    */
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with the number of
 * registered exception handler callbacks. Currently there is no way of
 * telling what callbacks are registered, as no names are passed to the add
 * method.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
BESExceptionManager::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "BESExceptionManager::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    strm << BESIndent::LMarg << "# registered callbacks: " << _ehm_list.size() << endl ;
    BESIndent::UnIndent() ;
}

BESExceptionManager *
BESExceptionManager::TheEHM()
{
    if( _instance == 0 )
    {
	_instance = new BESExceptionManager( ) ;
    }
    return _instance ;
}

