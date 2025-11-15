// -*- mode: c++; c-basic-offset:4 -*-
//
// SiteMapResponseHandler.h
//
// This file is part of the BES default command set
//
// Copyright (c) 2018 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
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
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
// Please read the full copyright statement in the file COPYRIGHT_URI.
//

#ifndef I_SiteMapResponseHandler_h
#define I_SiteMapResponseHandler_h 1

#include <string>
#include <vector>
#include <ostream>

#include "BESResponseHandler.h"
#include "SiteMapCommand.h"

/** @brief Response handler that returns a site map
 *
 * This ResponseHandler returns a simple text site map for the BES's catalogs
 * (or a specific catalog, if that optional parameter is given in the command).
 * Previous versions of the command wrote the site map to a file at the top
 * level of the file system the BES uses for the default catalog. This version
 * returns the site map to the caller (and does not store it on the BES).
 *
 * By design, the BES allows users to choose the kinds of 'Info' objects that
 * are returned for these kinds of 'show' commands. However, the BESXMLInfo
 * response object is not really correct for the site map since there is a
 * specific 'XML format' for site maps and it's much more structured than the
 * simple text listing of pathnames we're using. For this reason, the site map
 * response handler uses a BESTextInfo object to return the response, regardless
 * of the BES's default Info object configuration.
 *
 * @see BESResponseObject
 * @see BESContainer
 * @see BESTransmitter
 */
class SiteMapResponseHandler: public BESResponseHandler {
public:
    SiteMapResponseHandler(const std::string &name);
    virtual ~SiteMapResponseHandler();

    virtual void execute(BESDataHandlerInterface &dhi);
    virtual void transmit(BESTransmitter *transmitter, BESDataHandlerInterface &dhi);

    void dump(std::ostream &strm) const override;

    static BESResponseHandler *SiteMapResponseBuilder(const std::string &name);
};

#endif // I_SiteMapResponseHandler_h

