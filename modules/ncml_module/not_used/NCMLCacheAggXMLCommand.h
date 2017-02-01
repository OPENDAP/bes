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
#ifndef __NCML_MODULE__NCML_CACHE_AGG_XML_COMMAND_H__
#define __NCML_MODULE__NCML_CACHE_AGG_XML_COMMAND_H__

#include "BESDataHandlerInterface.h"
#include "BESResponseHandler.h"
#include "BESXMLCommand.h"

namespace ncml_module {

/**
 * The BESXMLCommand for the command to recalculate the aggregation caches.
 *
 * @note This code is not currently used. jhrg 4/16/14
 */
class NCMLCacheAggXMLCommand: public BESXMLCommand {
public:
    NCMLCacheAggXMLCommand(const BESDataHandlerInterface& baseDHI);

    virtual ~NCMLCacheAggXMLCommand();

    virtual void parse_request(xmlNode* pNode);

    virtual bool has_response();

    virtual void prep_request();

    virtual void dump(ostream& strm) const;

    static BESXMLCommand* makeInstance(const BESDataHandlerInterface& baseDHI);
};
// class NCMLCacheAggXMLCommand

/**
 * The response handler for the NCMLCacheAggXMLCommand
 */
class NCMLCacheAggResponseHandler: public BESResponseHandler {
public:
    NCMLCacheAggResponseHandler(const string &name);
    virtual ~NCMLCacheAggResponseHandler();

    virtual void execute(BESDataHandlerInterface &dhi);

    virtual void transmit(BESTransmitter *pTransmitter, BESDataHandlerInterface &dhi);

    virtual void dump(ostream &strm) const;

    static BESResponseHandler *makeInstance(const string &name);
};
// class NCMLCacheAggResponseHandler

}// namespace ncml_module

#endif /* __NCML_MODULE__NCML_CACHE_AGG_XML_COMMAND_H__ */
