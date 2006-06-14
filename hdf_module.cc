// This file is part of the hdf4 data handler for the OPeNDAP data server.

// Copyright (c) 2005 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
//
// This is free software; you can redistribute it and/or modify it under the
// terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 2.1 of the License, or (at your
// option) any later version.
// 
// This software is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
// License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this software; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
 
#include <iostream>

using std::endl ;

#include "BESInitList.h"
#if 0
#include "TheRequestHandlerList.h"
#endif
#include "HDFRequestHandler.h"
#if 0
#include "TheBESLog.h"
#endif

static bool
HDFInit(int, char**)
{
    if( TheBESLog->is_verbose() )
       (*TheBESLog) << "Initializing HDF:" << endl ;

    if( TheBESLog->is_verbose() )
        (*TheBESLog) << "    adding hdf4 request handler" << endl ;

    TheRequestHandlerList->add_handler( "hdf4",
                                       new HDFRequestHandler( "hdf4" ) ) ;

    return true ;
}

static bool
HDFTerm(void)
{
    if( TheBESLog->is_verbose() )
        (*TheBESLog) << "Removing hdf4 Handlers" << endl;

    BESRequestHandler *rh = TheRequestHandlerList->remove_handler( "hdf4" ) ;
    if( rh ) delete rh ;
    return true ;
}

FUNINITQUIT( HDFInit, HDFTerm, 3 ) ;
