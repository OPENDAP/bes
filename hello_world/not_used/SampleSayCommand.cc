// SampleSayCommand.cc

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

#include "SampleSayCommand.h"
#include "BESTokenizer.h"
#include "BESResponseHandlerList.h"
#include "BESSyntaxUserError.h"
#include "SampleResponseNames.h"

BESResponseHandler *
SampleSayCommand::parse_request(BESTokenizer &tokenizer, BESDataHandlerInterface &dhi)
{
    string my_token;

    /* No sub command, so proceed with the default command
     */
    dhi.action = SAY_RESPONSE;
    BESResponseHandler *retResponse = BESResponseHandlerList::TheList()->find_handler( SAY_RESPONSE);
    if (!retResponse) {
        string s = (string) "No response handler for command " + SAY_RESPONSE;
        throw BESSyntaxUserError(s, __FILE__, __LINE__);
    }

    my_token = tokenizer.get_next_token();
    if (my_token == ";") {
        tokenizer.parse_error(my_token + " not expected\n");
    }

    // Here is where your code would parse the tokens
    dhi.data[SAY_WHAT] = my_token;

    // Next token should be the token "to"
    my_token = tokenizer.get_next_token();
    if (my_token != "to") {
        tokenizer.parse_error(my_token + " not expected\n");
    }

    // Next token should be what is being said
    my_token = tokenizer.get_next_token();
    if (my_token == ";") {
        tokenizer.parse_error(my_token + " not expected\n");
    }
    dhi.data[SAY_TO] = my_token;

    // Last token should be the terminating semicolon (;)
    my_token = tokenizer.get_next_token();
    if (my_token != ";") {
        tokenizer.parse_error(my_token + " not expected\n");
    }

    return retResponse;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void SampleSayCommand::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "SampleSayCommand::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    BESCommand::dump(strm);
    BESIndent::UnIndent();
}

