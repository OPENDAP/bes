// BESBasicTransmitter.cc

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

#include <iostream>
#include <string>
#include <algorithm>
#include <unistd.h>

#include "TheBESKeys.h"
#include "BESBasicTransmitter.h"
#include "BESInfo.h"
#include "BESUtil.h"
#include "BESContextManager.h"


BESBasicTransmitter::BESBasicTransmitter(){
    bool found = false;
    string cancel_timeout_on_send = "";
    TheBESKeys::TheKeys()->get_value(BES_KEY_TIMEOUT_CANCEL, cancel_timeout_on_send, found);
    if (found && !cancel_timeout_on_send.empty()) {
        // The default value is false.
        std::transform(
            cancel_timeout_on_send.begin(),
            cancel_timeout_on_send.end(),
            cancel_timeout_on_send.begin(),
            ::tolower);
        if (cancel_timeout_on_send == "yes" || cancel_timeout_on_send == "true")
            d_cancel_timeout_on_send = true;
    }

}
void BESBasicTransmitter::conditional_timeout_cancel()
{
    if (d_cancel_timeout_on_send)
        alarm(0);
}

void BESBasicTransmitter::send_text(BESInfo &info, BESDataHandlerInterface &dhi)
{
	bool found = false;
	string context = "transmit_protocol";
	string protocol = BESContextManager::TheManager()->get_context(context, found);
	if (protocol == "HTTP") {
		if (info.is_buffered()) {
			BESUtil::set_mime_text(dhi.get_output_stream());
		}
	}
	info.print(dhi.get_output_stream());
}

void BESBasicTransmitter::send_html(BESInfo &info, BESDataHandlerInterface &dhi)
{
	bool found = false;
	string context = "transmit_protocol";
	string protocol = BESContextManager::TheManager()->get_context(context, found);
	if (protocol == "HTTP") {
		if (info.is_buffered()) {
			BESUtil::set_mime_html(dhi.get_output_stream());
		}
	}
	info.print(dhi.get_output_stream());
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESBasicTransmitter::dump(ostream &strm) const
{
	strm << BESIndent::LMarg << "BESBasicTransmitter::dump - (" << (void *) this << ")" << endl;
	BESIndent::Indent();
	BESTransmitter::dump(strm);
	BESIndent::UnIndent();
}

