//////////////////////////////////////////////////////////////////////////////
// This file is part of the "NcML Module" project, a BES module designed
// to allow NcML files to be used to be used as a wrapper to add
// AIS to existing datasets of any format.
//
// Copyright (c) 2009 OPeNDAP, Inc.
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

#ifndef I_NCMLRequestHandler_H
#define I_NCMLRequestHandler_H

#include "BESRequestHandler.h"
#include "BESDDSResponse.h"

namespace ncml_module {
/**
 * Handler for AIS Using NCML
 */
class NCMLRequestHandler: public BESRequestHandler {
private:
    // rep
    static bool _global_attributes_container_name_set;
    static std::string _global_attributes_container_name;

private:
#if 0
    // Not used jhrg 4/16/14
    // example for loading other locations by hijacking the dhi of the ncml request.
    static bool ncml_build_redirect( BESDataHandlerInterface &dhi, const string& location );
#endif
public:
    NCMLRequestHandler(const std::string &name);
    virtual ~NCMLRequestHandler(void);

    void dump(std::ostream &strm) const override;

    static bool ncml_build_das(BESDataHandlerInterface &dhi);
    static bool ncml_build_dds(BESDataHandlerInterface &dhi);
    static bool ncml_build_data(BESDataHandlerInterface &dhi);
    static bool ncml_build_dmr(BESDataHandlerInterface &dhi);
    static bool ncml_build_vers(BESDataHandlerInterface &dhi);
    static bool ncml_build_help(BESDataHandlerInterface &dhi);

    static std::string get_global_attributes_container_name()
    {
        return _global_attributes_container_name;
    }

};
// class NCMLRequestHandler
}// namespace ncml_module

#endif // NCMLRequestHandler.h

