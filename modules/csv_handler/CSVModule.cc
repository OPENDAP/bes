// CSVModule.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004-2009 University Corporation for Atmospheric Research
// Author: Stephan Zednik <zednik@ucar.edu> and Patrick West <pwest@ucar.edu>
// and Jose Garcia <jgarcia@ucar.edu>
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
//	zednik      Stephan Zednik <zednik@ucar.edu>
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include <iostream>

using std::endl;

#include <BESRequestHandlerList.h>
#include <BESResponseHandlerList.h>
#include <BESResponseNames.h>

#include <BESContainerStorageList.h>
#include <BESContainerStorageCatalog.h>
#include <BESCatalogDirectory.h>
#include <BESCatalogList.h>
#include <BESDapService.h>

#include <BESLog.h>
#include <BESDebug.h>

#include "CSVRequestHandler.h"
#include "CSVModule.h"

#define CSV_NAME "csv"
#define CSV_CATALOG "catalog"

void CSVModule::initialize(const string &modname)
{
	BESDEBUG("csv", "Initializing CSV Module: " << modname << endl);

	BESRequestHandlerList::TheList()->add_handler(modname, new CSVRequestHandler(modname));

	BESDapService::handle_dap_service(modname);

	if (!BESCatalogList::TheCatalogList()->ref_catalog(CSV_CATALOG)) {
		BESCatalogList::TheCatalogList()->add_catalog(new BESCatalogDirectory(CSV_CATALOG));
	}

	if (!BESContainerStorageList::TheList()->ref_persistence(CSV_CATALOG)) {
		BESContainerStorageList::TheList()->add_persistence(new BESContainerStorageCatalog(CSV_CATALOG));
	}

	BESDebug::Register("csv");

	BESDEBUG("csv", "Done Initializing CSV Handler: " << modname << endl);
}

void CSVModule::terminate(const string &modname)
{
	BESDEBUG("csv", "Cleaning CSV Module: " << modname << endl);

	BESRequestHandler *rh = BESRequestHandlerList::TheList()->remove_handler(modname);
	if (rh) delete rh;

	BESContainerStorageList::TheList()->deref_persistence(CSV_CATALOG);

	BESCatalogList::TheCatalogList()->deref_catalog(CSV_CATALOG);

	BESDEBUG("csv", "Done Cleaning CSV Module: " << modname << endl);
}

extern "C" {
BESAbstractModule *maker()
{
	return new CSVModule;
}
}

void CSVModule::dump(ostream &strm) const
{
	strm << BESIndent::LMarg << "CSVModule::dump - (" << (void *) this << ")" << endl;
}

