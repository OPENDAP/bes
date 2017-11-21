// OPENDAP_TYPE_module.cc

// Copyright (c) 2013 OPeNDAP, Inc. Author: James Gallagher
// <jgallagher@opendap.org>, Patrick West <pwest@opendap.org>
// Nathan Potter <npotter@opendap.org>
//                                                                            
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2.1 of
// the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
// 02110-1301 U\ SA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI.
// 02874-0112.
#include <iostream>

using std::endl ;

#include "BESInitList.h"
#include "BESRequestHandlerList.h"
#include "OPENDAP_CLASSRequestHandler.h"
#include "BESLog.h"
#include "BESResponseHandlerList.h"
#include "OPENDAP_CLASSResponseNames.h"

static bool
OPENDAP_CLASSInit(int, char**)
{
    if( BESLog::TheLog()->is_verbose() )
	LOG("Initializing OPENDAP_CLASS Handler:" << endl );

    if( BESLog::TheLog()->is_verbose() )
	LOG("    adding " << OPENDAP_CLASS_NAME << " request handler" << endl );
    BESRequestHandlerList::TheList()->add_handler( OPENDAP_CLASS_NAME, new OPENDAP_CLASSRequestHandler( OPENDAP_CLASS_NAME ) ) ;

    return true ;
}

static bool
OPENDAP_CLASSTerm(void)
{
    if( BESLog::TheLog()->is_verbose() )
	LOG("Removing OPENDAP_CLASS Handlers" << endl);
    BESRequestHandler *rh = BESRequestHandlerList::TheList()->remove_handler( OPENDAP_CLASS_NAME ) ;
    if( rh ) delete rh ;
    return true ;
}

FUNINITQUIT( OPENDAP_CLASSInit, OPENDAP_CLASSTerm, 3 ) ;

