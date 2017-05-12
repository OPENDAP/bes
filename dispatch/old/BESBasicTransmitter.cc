// BESTransmitter.cc

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

#if 0
#include <iostream>
#include <string>
#include <algorithm>
#include <unistd.h>

#include "TheBESKeys.h"
#include "BESTransmitter.h"
#include "BESInfo.h"
#include "BESUtil.h"
#include "BESContextManager.h"
#include "BESDebug.h"


#if 0

/**
 * If the value of the BES Key BES.CancelTimeoutOnSend is true, cancel the
 * timeout. The intent of this is to stop the timeout counter once the
 * BES starts sending data back since, the network link used by a remote
 * client may be low-bandwidth and data providers might want to ensure those
 * users get their data (and don't submit second, third, ..., requests when/if
 * the first one fails). The timeout is initiated in the BES framework when it
 * first processes the request.
 *
 * @note The BES timeout is set/controlled in bes/dispatch/BESInterface
 * in the 'int BESInterface::execute_request(const string &from)' method.
 *
 * @see BESTransmitter::send_data(BESResponseObject *obj, BESDataHandlerInterface &dhi)
 * methods of the child classes
 */
void BESTransmitter::conditional_timeout_cancel()
{
    bool cancel_timeout_on_send = false;
    bool found = false;
    string doset ="";
    const string dosettrue ="true";
    const string dosetyes = "yes";

    TheBESKeys::TheKeys()->get_value( BES_KEY_TIMEOUT_CANCEL, doset, found ) ;
    if( true == found ) {
        doset = BESUtil::lowercase( doset ) ;
        if( dosettrue == doset  || dosetyes == doset )
            cancel_timeout_on_send =  true;
    }
    BESDEBUG("transmit",__func__ << "() - cancel_timeout_on_send: " <<(cancel_timeout_on_send?"true":"false") << endl);

    if (cancel_timeout_on_send)
        alarm(0);
}
#endif

void BESTransmitter::send_text(BESInfo &info, BESDataHandlerInterface &dhi)
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

void BESTransmitter::send_html(BESInfo &info, BESDataHandlerInterface &dhi)
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
void BESTransmitter::dump(ostream &strm) const
{
	strm << BESIndent::LMarg << "BESTransmitter::dump - (" << (void *) this << ")" << endl;
	BESIndent::Indent();
	BESTransmitter::dump(strm);
	BESIndent::UnIndent();
}
#endif
