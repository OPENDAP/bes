// SampleModule.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004-2009 University Corporation for Atmospheric Research
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

// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include "config.h"

#include <iostream>

using std::endl;
using std::ostream;
using std::string;

#include "BESRequestHandlerList.h"

#include "BESDebug.h"
#include "BESResponseHandlerList.h"
#include "BESResponseNames.h"
#include "BESXMLCommand.h"

#include "SampleModule.h"
#include "SampleRequestHandler.h"
#include "SampleResponseNames.h"
#include "SampleSayResponseHandler.h"
#include "SampleSayXMLCommand.h"

void SampleModule::initialize(const string &modname)
{
    BESDEBUG(modname, "Initializing Sample Module " << modname << endl);

    BESRequestHandlerList::TheList()->add_handler(modname, new SampleRequestHandler(modname));

    BESResponseHandlerList::TheList()->add_handler(SAY_RESPONSE, SampleSayResponseHandler::SampleSayResponseBuilder);

    BESXMLCommand::add_command(SAY_RESPONSE, SampleSayXMLCommand::CommandBuilder);

    BESDebug::Register(modname);

    BESDEBUG(modname, "Done Initializing Sample Module " << modname << endl);
}

void SampleModule::terminate(const string &modname)
{
    BESDEBUG(modname, "Cleaning Sample module " << modname << endl);

    BESRequestHandler *rh = BESRequestHandlerList::TheList()->remove_handler(modname);
    if (rh) delete rh;

    BESResponseHandlerList::TheList()->remove_handler( SAY_RESPONSE);

    BESXMLCommand::del_command( SAY_RESPONSE);

    BESDEBUG(modname, "Done Cleaning Sample module " << modname << endl);
}

extern "C" {
BESAbstractModule *maker()
{
    return new SampleModule;
}
}

void SampleModule::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "SampleModule::dump - (" << (void *) this << ")" << endl;
}

