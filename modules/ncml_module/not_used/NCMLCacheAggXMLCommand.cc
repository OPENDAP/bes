//////////////////////////////////////////////////////////////////////////////
// This file is part of the "NcML Module" project, a BES module designed
// to allow NcML files to be used to be used as a wrapper to add
// AIS to existing datasets of any format.
//
// Copyright (c) 2010 OPeNDAP, Inc.
// Author: Michael Johnson  <m.johnson@opendap.org>
//
// For more information, please also see the main website: http://opendap.org/
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
// Please see the files COPYING and COPYRIGHT for more information on the GLPL.
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
/////////////////////////////////////////////////////////////////////////////
#include "NCMLCacheAggXMLCommand.h"

#include "BESXMLUtils.h"
#include "BESUtil.h"
#include "BESSyntaxUserError.h"
#include "BESDebug.h"

#include "NCMLDebug.h"
#include "NCMLResponseNames.h"

using std::string;

namespace ncml_module {

NCMLCacheAggXMLCommand::NCMLCacheAggXMLCommand(const BESDataHandlerInterface& baseDHI) :
    BESXMLCommand(baseDHI)
{
}

/* virtual */
NCMLCacheAggXMLCommand::~NCMLCacheAggXMLCommand()
{
}

/* virtual */
void NCMLCacheAggXMLCommand::parse_request(xmlNode* pNode)
{
    BESDEBUG(ModuleConstants::NCML_NAME, "NCMLCacheAggXMLCommand::parse_request() called...." << endl);

    string cmdName;
    string contentValue;
    map<string, string> attrs;
    BESXMLUtils::GetNodeInfo(pNode, cmdName, contentValue, attrs);
    if (cmdName != ModuleConstants::CACHE_AGG_RESPONSE) {
        THROW_NCML_PARSE_ERROR(-1, "Got unexpected command name=" + cmdName);
    }

    if (!contentValue.empty()) {
        THROW_NCML_PARSE_ERROR(-1, cmdName + ": should not have xml content!");
    }

    // Grab the filename we are to run on.
    d_xmlcmd_dhi.data[ModuleConstants::CACHE_AGG_LOCATION_DATA_KEY] = attrs[ModuleConstants::CACHE_AGG_LOCATION_XML_ATTR];

    // It had to be there, or it's an error.
    if (d_xmlcmd_dhi.data[ModuleConstants::CACHE_AGG_LOCATION_DATA_KEY].empty()) {
        THROW_NCML_PARSE_ERROR(-1,
            cmdName + ": we did find the required aggregation location specified in the attribute="
                + ModuleConstants::CACHE_AGG_LOCATION_XML_ATTR + " and cannot continue the caching.");
    }

    string childCmdName;
    string childContentValue;
    map<string, string> childAttrs;
    xmlNode *pChildNode = BESXMLUtils::GetFirstChild(pNode, childCmdName, childContentValue, childAttrs);
    if (pChildNode) {
        THROW_NCML_PARSE_ERROR(-1, cmdName + ": should not have child elements!");
    }

    if (!childAttrs.empty()) {
        THROW_NCML_PARSE_ERROR(-1, cmdName + ": should not have attributes!");
    }

    // Grab the response handler for this action
    d_xmlcmd_dhi.action = ModuleConstants::CACHE_AGG_RESPONSE;
    BESXMLCommand::set_response();
}

/* virtual */
bool NCMLCacheAggXMLCommand::has_response()
{
    return true;
}

/* virtual */
void NCMLCacheAggXMLCommand::prep_request()
{
    BESDEBUG(ModuleConstants::NCML_NAME, "NCMLCacheAggXMLCommand::prep_request() called..." << endl);
}

/* virtual */
void NCMLCacheAggXMLCommand::dump(ostream& strm) const
{
    strm << BESIndent::LMarg << "NCMLCacheAggXMLCommand::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    BESXMLCommand::dump(strm);
    BESIndent::UnIndent();
}

/* static */
BESXMLCommand*
NCMLCacheAggXMLCommand::makeInstance(const BESDataHandlerInterface& baseDHI)
{
    return new NCMLCacheAggXMLCommand(baseDHI);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

NCMLCacheAggResponseHandler::NCMLCacheAggResponseHandler(const string &name) :
    BESResponseHandler(name)
{
    BESDEBUG("ncml", "NCMLCacheAggResponseHandler::NCMLCacheAggResponseHandler() called..." << endl);
}

/* virtual */
NCMLCacheAggResponseHandler::~NCMLCacheAggResponseHandler()
{
}

/* virtual */
void NCMLCacheAggResponseHandler::execute(BESDataHandlerInterface& dhi)
{
    BESDEBUG("ncml",
        "NCMLCacheAggResponseHandler::execute() called for command:" << ModuleConstants::CACHE_AGG_RESPONSE << endl);

    const std::string& loc = dhi.data[ModuleConstants::CACHE_AGG_LOCATION_DATA_KEY];
    BESDEBUG("ncml", "We got a cacheAgg request for the aggregation location = " << loc << endl);
}

/* virtual */
void NCMLCacheAggResponseHandler::transmit(BESTransmitter* /* pTransmitter */, BESDataHandlerInterface& /* dhi */)
{
    BESDEBUG("ncml",
        "NCMLCacheAggResponseHandler::transmit() called for command: " << ModuleConstants::CACHE_AGG_RESPONSE << endl);
}

/* virtual */
void NCMLCacheAggResponseHandler::dump(ostream & /* strm */) const
{
    BESDEBUG("ncml",
        "NCMLCacheAggResponseHandler::dump() called for command: " << ModuleConstants::CACHE_AGG_RESPONSE << endl);
}

/* static */
BESResponseHandler *
NCMLCacheAggResponseHandler::makeInstance(const string &name)
{
    return new NCMLCacheAggResponseHandler(name);
}

}
;
// namespace ncml_module
