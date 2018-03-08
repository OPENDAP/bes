// BESDapModule.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004-2009 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.edu>
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301

// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>

#include <iostream>

using std::endl;

#include "BESDapModule.h"

#include "BESDapRequestHandler.h"
#include "BESRequestHandlerList.h"

#include "BESDapNames.h"
#include "BESNames.h"
#include "BESResponseHandlerList.h"

#include "BESDASResponseHandler.h"
#include "BESDDSResponseHandler.h"
#include "BESDataResponseHandler.h"
#include "BESDDXResponseHandler.h"
#include "BESDataDDXResponseHandler.h"
#include "BESDMRResponseHandler.h"
#include "BESDap4ResponseHandler.h"

#include "BESCatalogResponseHandler.h"

#include "BESServiceRegistry.h"

// These I added to test the Null AggregationServer code jhrg 1/30/15
#include "BESDapNullAggregationServer.h"
// Removed jhrg 3/17/15 #include "BESDapSequenceAggregationServer.h"
#include "BESAggFactory.h"

#include "BESDapTransmit.h"
#include "BESTransmitter.h"
#include "BESReturnManager.h"
#include "BESTransmitterNames.h"

#include "BESDebug.h"
#include "BESInternalFatalError.h"
#include "BESExceptionManager.h"
#include "BESDapError.h"

#include "DapFunctionUtils.h"
#include "ServerFunctionsList.h"
#include "ShowPathInfoResponseHandler.h"


void BESDapModule::initialize(const string &modname)
{
	BESDEBUG("dap", "Initializing DAP Modules:" << endl);

	BESDEBUG("dap", "    adding " << modname << " request handler" << endl);
	BESRequestHandlerList::TheList()->add_handler(modname, new BESDapRequestHandler(modname));

	BESDEBUG("dap", "    adding " << DAS_RESPONSE << " response handler" << endl);
	BESResponseHandlerList::TheList()->add_handler(DAS_RESPONSE, BESDASResponseHandler::DASResponseBuilder);

	BESDEBUG( "dap", "    adding " << DDS_RESPONSE << " response handler" << endl );
	BESResponseHandlerList::TheList()->add_handler( DDS_RESPONSE, BESDDSResponseHandler::DDSResponseBuilder );

	BESDEBUG("dap", "    adding " << DDX_RESPONSE << " response handler" << endl);
	BESResponseHandlerList::TheList()->add_handler(DDX_RESPONSE, BESDDXResponseHandler::DDXResponseBuilder);

	BESDEBUG("dap", "    adding " << DATA_RESPONSE << " response handler" << endl);
	BESResponseHandlerList::TheList()->add_handler(DATA_RESPONSE, BESDataResponseHandler::DataResponseBuilder);

	BESDEBUG("dap", "    adding " << DATADDX_RESPONSE << " response handler" << endl);
	BESResponseHandlerList::TheList()->add_handler(DATADDX_RESPONSE, BESDataDDXResponseHandler::DataDDXResponseBuilder);

	BESDEBUG("dap", "    adding " << DMR_RESPONSE << " response handler" << endl);
	BESResponseHandlerList::TheList()->add_handler(DMR_RESPONSE, BESDMRResponseHandler::DMRResponseBuilder);

	BESDEBUG("dap", "    adding " << DAP4DATA_RESPONSE << " response handler" << endl);
	BESResponseHandlerList::TheList()->add_handler(DAP4DATA_RESPONSE, BESDap4ResponseHandler::Dap4ResponseBuilder);

	BESDEBUG("dap", "    adding " << CATALOG_RESPONSE << " response handler" << endl);
	BESResponseHandlerList::TheList()->add_handler(CATALOG_RESPONSE, BESCatalogResponseHandler::CatalogResponseBuilder);

	BESDEBUG("dap", "Adding " << OPENDAP_SERVICE << " services:" << endl);
	BESServiceRegistry *registry = BESServiceRegistry::TheRegistry();
	registry->add_service(OPENDAP_SERVICE);
	registry->add_to_service(OPENDAP_SERVICE, DAS_SERVICE, DAS_DESCRIPT, DAP2_FORMAT);
	registry->add_to_service(OPENDAP_SERVICE, DDS_SERVICE, DDS_DESCRIPT, DAP2_FORMAT);
	registry->add_to_service(OPENDAP_SERVICE, DDX_SERVICE, DDX_DESCRIPT, DAP2_FORMAT);
	registry->add_to_service(OPENDAP_SERVICE, DATA_SERVICE, DATA_DESCRIPT, DAP2_FORMAT);
	registry->add_to_service(OPENDAP_SERVICE, DATADDX_SERVICE, DATADDX_DESCRIPT, DAP2_FORMAT);

	registry->add_to_service(OPENDAP_SERVICE, DMR_SERVICE, DMR_DESCRIPT, DAP2_FORMAT);
	registry->add_to_service(OPENDAP_SERVICE, DAP4DATA_SERVICE, DAP4DATA_DESCRIPT, DAP2_FORMAT);

	BESDEBUG("dap", "Initializing DAP Basic Transmitters:" << endl);
	BESReturnManager::TheManager()->add_transmitter(DAP2_FORMAT, new BESDapTransmit());
	// TODO ?? BESReturnManager::TheManager()->add_transmitter( DAP4_FORMAT, new BESDapTransmit( ) );

	BESDEBUG("dap", "    adding dap exception handler" << endl);
	BESExceptionManager::TheEHM()->add_ehm_callback(BESDapError::handleException);

#if 0
	// Aggregations are no longer run. jhrg 11/9/17
	// Add the new 'Null' AggregationServer. jhrg 1/30/15
	// TODO Add these names to BESDapNames.h
	BESDEBUG("dap", "    adding null aggregation handler" << endl);
    BESAggFactory::TheFactory()->add_handler("null.aggregation", BESDapNullAggregationServer::NewBESDapNullAggregationServer);
#endif
#if 0
    // Removed jhrg 3/17/15
    BESAggFactory::TheFactory()->add_handler("sequence.aggregation", BESDapSequenceAggregationServer::NewBESDapSequenceAggregationServer);
#endif

    BESDEBUG("dap", "    adding DAP Utility Function 'wrapitup'()" << endl);
    WrapItUp *wiu = new WrapItUp();
    libdap::ServerFunctionsList::TheList()->add_function(wiu);

    BESDEBUG("dap", "    adding " << SHOW_PATH_INFO_RESPONSE << " response handler" << endl ) ;
    BESResponseHandlerList::TheList()->add_handler( SHOW_PATH_INFO_RESPONSE, ShowPathInfoResponseHandler::ShowPathInfoResponseBuilder ) ;

	BESDEBUG("dap", "    adding dap debug context" << endl);
	BESDebug::Register("dap");

	BESDEBUG("dap", "Done Initializing DAP Modules:" << endl);
}

void BESDapModule::terminate(const string &modname)
{
	BESDEBUG("dap", "Removing DAP Modules:" << endl);

	BESResponseHandlerList::TheList()->remove_handler(DAS_RESPONSE);
	BESResponseHandlerList::TheList()->remove_handler(DDS_RESPONSE);
	BESResponseHandlerList::TheList()->remove_handler(DDX_RESPONSE);
	BESResponseHandlerList::TheList()->remove_handler(DATA_RESPONSE);
	BESResponseHandlerList::TheList()->remove_handler(DATADDX_RESPONSE);

	BESResponseHandlerList::TheList()->remove_handler(CATALOG_RESPONSE);

	BESResponseHandlerList::TheList()->remove_handler(DMR_RESPONSE);
	BESResponseHandlerList::TheList()->remove_handler(DAP4DATA_RESPONSE);

#if 0
	BESResponseHandlerList::TheList()->remove_handler(CATALOG_RESPONSE);
#endif

	BESDEBUG("dap", "    removing " << OPENDAP_SERVICE << " services" << endl);
	BESServiceRegistry::TheRegistry()->remove_service(OPENDAP_SERVICE);

	BESDEBUG("dap", "    removing dap Request Handler " << modname << endl);
	BESRequestHandler *rh = BESRequestHandlerList::TheList()->remove_handler(modname);
	if (rh) delete rh;

	BESReturnManager::TheManager()->del_transmitter(DAP2_FORMAT);
	// TODO ?? BESReturnManager::TheManager()->del_transmitter( DAP4_FORMAT );

#if 0
	BESAggFactory::TheFactory()->remove_handler("null.aggregation");
#endif

	BESDEBUG("dap", "Done Removing DAP Modules:" << endl);
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESDapModule::dump(ostream &strm) const
{
	strm << BESIndent::LMarg << "BESDapModule::dump - (" << (void *) this << ")" << endl;
}

extern "C" {
BESAbstractModule *maker()
{
	return new BESDapModule;
}
}

