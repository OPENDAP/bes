// SampleSayXMLCommand.cc

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

#include "BESXMLUtils.h"
#include "BESUtil.h"
#include "BESSyntaxUserError.h"
#include "BESDebug.h"

#include "SampleSayXMLCommand.h"
#include "SampleResponseNames.h"

using std::endl;
using std::ostream;

SampleSayXMLCommand::SampleSayXMLCommand(const BESDataHandlerInterface &base_dhi) :
    BESXMLCommand(base_dhi)
{
}

/** @brief parse a show command. No properties or children elements
 *
 <say what="what" to="whom" />
 *
 * @param node xml2 element node pointer
 */
void SampleSayXMLCommand::parse_request(xmlNode *node)
{
    string name;
    string value;
    map<string, string> props;
    BESXMLUtils::GetNodeInfo(node, name, value, props);
    if (name != SAY_RESPONSE) {
        string err = "The specified command " + name + " is not a say command";
        throw BESSyntaxUserError(err, __FILE__, __LINE__);
    }

    if (!value.empty()) {
        string err = name + " command: should not have xml element values";
        throw BESSyntaxUserError(err, __FILE__, __LINE__);
    }

    string child_name;
    string child_value;
    map<string, string> child_props;
    xmlNode *child_node = BESXMLUtils::GetFirstChild(node, child_name, child_value, child_props);
    if (child_node) {
        string err = name + " command: should not have child elements";
        throw BESSyntaxUserError(err, __FILE__, __LINE__);
    }

    d_xmlcmd_dhi.data[SAY_WHAT] = props["what"];
    if (d_xmlcmd_dhi.data[SAY_WHAT].empty()) {
        string err = name + " command: Must specify to whom to say";
        throw BESSyntaxUserError(err, __FILE__, __LINE__);
    }

    d_xmlcmd_dhi.data[SAY_TO] = props["to"];
    if (d_xmlcmd_dhi.data[SAY_WHAT].empty()) {
        string err = name + " command: Must specify what to say";
        throw BESSyntaxUserError(err, __FILE__, __LINE__);
    }

    d_xmlcmd_dhi.action = SAY_RESPONSE;

    // now that we've set the action, go get the response handler for the
    // action
    BESXMLCommand::set_response();
}

void SampleSayXMLCommand::prep_request()
{
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void SampleSayXMLCommand::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "SampleSayXMLCommand::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    BESXMLCommand::dump(strm);
    BESIndent::UnIndent();
}

BESXMLCommand *
SampleSayXMLCommand::CommandBuilder(const BESDataHandlerInterface &base_dhi)
{
    return new SampleSayXMLCommand(base_dhi);
}

