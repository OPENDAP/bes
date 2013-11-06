
// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2013 OPeNDAP, Its.
// Author: James Gallagher <jgallagher@opendap.rg>
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
// Foundation, Its., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301

#include <iostream>

using std::endl;

#include <BESRequestHandlerList.h>
#include <BESDapService.h>
#include <BESContainerStorageList.h>
#include <BESContainerStorageCatalog.h>
#include <BESCatalogDirectory.h>
#include <BESCatalogList.h>
#include <BESDebug.h>

#include "TestModule.h"
#include "TestRequestHandler.h"

#define TEST_CATALOG "catalog"

void TestModule::initialize( const string & modname )
{
    BESDEBUG( "ts", "Initializing Test Server module " << modname << endl ) ;

    BESRequestHandler *handler = new TestRequestHandler( modname ) ;
    BESRequestHandlerList::TheList()->add_handler( modname, handler ) ;

    BESDapService::handle_dap_service( modname ) ;

    BESDEBUG( "ts", "    adding " << TEST_CATALOG << " catalog" << endl ) ;
	if (!BESCatalogList::TheCatalogList()->ref_catalog(TEST_CATALOG)) {
		BESCatalogList::TheCatalogList()->add_catalog(new BESCatalogDirectory(TEST_CATALOG));
	}

	if (!BESContainerStorageList::TheList()->ref_persistence(TEST_CATALOG)) {
		BESContainerStorageCatalog *csc = new BESContainerStorageCatalog(TEST_CATALOG);
		BESContainerStorageList::TheList()->add_persistence(csc);
	}

    BESDebug::Register( "ts" ) ;

    BESDEBUG( "ts", "Done Initializing NC module " << modname << endl ) ;
}

void TestModule::terminate( const string & modname )
{
    BESDEBUG( "ts", "Cleaning NC module " << modname << endl ) ;

    BESRequestHandler *rh = BESRequestHandlerList::TheList()->remove_handler( modname ) ;
    if( rh ) delete rh ;

    BESContainerStorageList::TheList()->deref_persistence( TEST_CATALOG ) ;

    BESCatalogList::TheCatalogList()->deref_catalog( TEST_CATALOG ) ;

    BESDEBUG( "ts", "Done Cleaning NC module " << modname << endl ) ;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void TestModule::dump( ostream & strm ) const
{
    strm << BESIndent::LMarg << "TestModule::dump - (" << (void *) this << ")"  << endl;
}

extern "C" BESAbstractModule * maker()
{
    return new TestModule ;
}

