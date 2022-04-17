// BESDapErrorInfo.cc

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

#include "BESDapErrorInfo.h"

using std::endl;
using std::map;

/** @brief constructs an informational object that doesn't
 *         write any output to the stream
 */
BESDapErrorInfo::BESDapErrorInfo(ErrorCode ec, string msg) :
    BESInfo(), _error_code(ec), _error_msg(std::move(msg))
{
}

/** @brief begin the informational response
 *
 * Because this is silent, there is nothing to do
 *
 * @param response_name name of the response represented by the information
 * @param dhi information about the request and response
 */
void BESDapErrorInfo::begin_response(const string &response_name, BESDataHandlerInterface &dhi)
{
    BESInfo::begin_response(response_name, dhi);
}

/** @brief add tagged information to the informational response
 *
 * @param tag_name name of the tag to add to the informational response
 * @param tag_data information describing the tag
 * @param attrs map of attributes to add to the tag
 */
void BESDapErrorInfo::add_tag(const string & /*tag_name*/, const string & /*tag_data*/, map<string, string> * /*attrs*/)
{
}

/** @brief begin a tagged part of the information, information to follow
 *
 * @param tag_name name of the tag to begin
 * @param attrs map of attributes to begin the tag with
 */
void BESDapErrorInfo::begin_tag(const string &tag_name, map<string, string> * /*attrs*/)
{
    BESInfo::begin_tag(tag_name);
}

/** @brief end a tagged part of the informational response
 *
 * If the named tag is not the current tag then an error is thrown.
 *
 * @param tag_name name of the tag to end
 */
void BESDapErrorInfo::end_tag(const string &tag_name)
{
    BESInfo::end_tag(tag_name);
}

/** @brief add data to the informational object
 *
 * because this is a silent response, nothing is added
 *
 * @param s information to be ignored
 */
void BESDapErrorInfo::add_data(const string & /*s*/)
{
}

/** @brief add a space to the informational response
 *
 * because this is a silent response, nothing is added
 *
 * @param num_spaces number of spaces to add
 */
void BESDapErrorInfo::add_space(unsigned long /*num_spaces*/)
{
}

/** @brief add a line break to the information
 *
 * because this is a silent response, nothing is added
 *
 * @param num_breaks number of line breaks to add
 */
void BESDapErrorInfo::add_break(unsigned long /*num_breaks*/)
{
}

/** @brief ignore data from a file to the informational object.
 *
 * @param key Key from the initialization file specifying the file to be
 * @param name naem information to add to error messages
 * loaded.
 */
void BESDapErrorInfo::add_data_from_file(const string & /*key*/, const string & /*name*/)
{
}

/** @brief ignore exception data to this informational object.
 *
 * @param e exception to be added
 * @param admin The contact information for the person
 * responsible for this error
 */
void BESDapErrorInfo::add_exception(const BESError & /*e*/, const string & /*admin*/)
{
}

/** @brief transmit this informational object
 *
 * transmit this as text to the transmitter
 *
 * @param transmitter The type of transmitter to use to transmit the info
 * @param dhi information to help with the transmission
 */
void BESDapErrorInfo::transmit(BESTransmitter *transmitter, BESDataHandlerInterface &dhi)
{
    transmitter->send_text(*this, dhi);
}

/** @brief ignore printing the information
 *
 * @param strm stream to send output to if not ignored.
 */
void BESDapErrorInfo::print(ostream &strm)
{
    Error new_e(_error_code, _error_msg);
    new_e.print(strm);
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance and calls
 * dump on the parent class
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESDapErrorInfo::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "BESDapErrorInfo::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    BESInfo::dump(strm);
    BESIndent::UnIndent();
}

