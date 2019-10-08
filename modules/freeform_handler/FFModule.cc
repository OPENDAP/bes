// FFModule.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.org>
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
using std::ostream;

#include <BESRequestHandlerList.h>
#include <BESDapService.h>
#include <BESContainerStorageList.h>
#include <BESFileContainerStorage.h>
#include <BESCatalogDirectory.h>
#include <BESCatalogList.h>
#include <BESDebug.h>

#include "FFModule.h"
#include "FFRequestHandler.h"

#define FF_CATALOG "catalog"

void FFModule::initialize(const string &modname)
{
	BESDEBUG("ff", "Initializing FF module " << modname << endl);

	BESRequestHandler *handler = new FFRequestHandler(modname);
	BESRequestHandlerList::TheList()->add_handler(modname, handler);

	BESDapService::handle_dap_service(modname);

	if (!BESCatalogList::TheCatalogList()->ref_catalog(FF_CATALOG)) {
		BESCatalogList::TheCatalogList()->add_catalog(new BESCatalogDirectory(FF_CATALOG));
	}
	else {
		BESDEBUG("ff", "    catalog already exists, skipping" << endl);
	}

	if (!BESContainerStorageList::TheList()->ref_persistence(FF_CATALOG)) {
		BESContainerStorageList::TheList()->add_persistence(new BESFileContainerStorage(FF_CATALOG));
	}
	else {
		BESDEBUG("ff", "    storage already exists, skipping" << endl);
	}

	BESDebug::Register("ff");

	BESDEBUG("ff", "Done Initializing FF module " << modname << endl);
}

void FFModule::terminate(const string &modname)
{
	BESDEBUG("ff", "Cleaning FF module " << modname << endl);

	BESRequestHandler *rh = BESRequestHandlerList::TheList()->remove_handler(modname);
	if (rh) delete rh;

	BESContainerStorageList::TheList()->deref_persistence( FF_CATALOG);

	BESCatalogList::TheCatalogList()->deref_catalog( FF_CATALOG);

	BESDEBUG("ff", "Done Cleaning FF module " << modname << endl);
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void FFModule::dump(ostream &strm) const
{
	strm << BESIndent::LMarg << "FFModule::dump - (" << (void *) this << ")" << endl;
}

extern "C" {
BESAbstractModule *maker()
{
	return new FFModule;
}
}

