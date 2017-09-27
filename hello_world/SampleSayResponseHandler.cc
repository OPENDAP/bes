// SampleSayResponseHandler.cc

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

#include "SampleSayResponseHandler.h"
#include "BESInfoList.h"
#include "BESInfo.h"
#include "SampleResponseNames.h"

SampleSayResponseHandler::SampleSayResponseHandler(const string &name) :
    BESResponseHandler(name)
{
}

SampleSayResponseHandler::~SampleSayResponseHandler()
{
}

void SampleSayResponseHandler::execute(BESDataHandlerInterface &dhi)
{
    // This is an example. Here you would build the BESResponseObject
    // object and set it to the _response protected data member
    BESInfo *info = BESInfoList::TheList()->build_info();
    _response = info;

    // Here is where your code would fill in the new response object
    info->begin_response( SAY_RESPONSE, dhi);
    string str = dhi.data[ SAY_WHAT] + " " + dhi.data[ SAY_TO];
    info->add_tag("text", str);
    info->end_response();
}

void SampleSayResponseHandler::transmit(BESTransmitter *transmitter, BESDataHandlerInterface &dhi)
{
    // Here is where you would transmit your response object using the
    // specified transmitter object. This is the example using the BESInfo
    // response object
    if (_response) {
        BESInfo *info = dynamic_cast<BESInfo *>(_response);
        info->transmit(transmitter, dhi);
    }
}

void SampleSayResponseHandler::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "SampleSayResponseHandler::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    BESResponseHandler::dump(strm);
    BESIndent::UnIndent();
}

BESResponseHandler *
SampleSayResponseHandler::SampleSayResponseBuilder(const string &name)
{
    return new SampleSayResponseHandler(name);
}

