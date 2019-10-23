// FitsModule.cc

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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301

// (c) COPYRIGHT University Corporation for Atmostpheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include <iostream>

using std::endl;
using std::ostream;
using std::string;

#include "FitsModule.h"
#include <BESRequestHandlerList.h>
#include "FitsRequestHandler.h"
#include <BESDapService.h>
#include <BESDebug.h>
#include <BESContainerStorageList.h>
#include <BESFileContainerStorage.h>
#include <BESCatalogDirectory.h>
#include <BESCatalogList.h>
//#include "FitsResponseNames.h"

#define FITS_CATALOG "catalog"

void FitsModule::initialize(const string &modname)
{
	BESDEBUG("fits", "Initializing Fits module " << modname << endl);

	BESRequestHandlerList::TheList()->add_handler(modname, new FitsRequestHandler(modname));

	BESDapService::handle_dap_service(modname);

	if (!BESCatalogList::TheCatalogList()->ref_catalog(FITS_CATALOG)) {
		BESCatalogList::TheCatalogList()->add_catalog(new BESCatalogDirectory(FITS_CATALOG));
	}
	else {
		BESDEBUG("fits", "    catalog already exists, skipping" << endl);
	}

	if (!BESContainerStorageList::TheList()->ref_persistence(FITS_CATALOG)) {
		BESFileContainerStorage *csc = new BESFileContainerStorage(FITS_CATALOG);
		BESContainerStorageList::TheList()->add_persistence(csc);
	}
	else {
		BESDEBUG("fits", "    storage already exists, skipping" << endl);
	}

	BESDebug::Register("fits");

	BESDEBUG("fits", "Done Initializing Fits module " << modname << endl);
}

void FitsModule::terminate(const string &modname)
{
	BESDEBUG("fits", "Cleaning Fits module " << modname << endl);

	BESRequestHandler *rh = BESRequestHandlerList::TheList()->remove_handler(modname);
	if (rh) delete rh;

	BESContainerStorageList::TheList()->deref_persistence(FITS_CATALOG);

	BESCatalogList::TheCatalogList()->deref_catalog(FITS_CATALOG);

	BESDEBUG("fits", "Done Cleaning Fits module " << modname << endl);
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void FitsModule::dump(ostream &strm) const
{
	strm << BESIndent::LMarg << "FitsModule::dump - (" << (void *) this << ")" << endl;
}

extern "C" {
BESAbstractModule *maker()
{
	return new FitsModule;
}
}

