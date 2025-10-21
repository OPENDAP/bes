// BESDataDDSResponse.h

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

#ifndef I_BESDataDDSResponse
#define I_BESDataDDSResponse 1

#include <libdap/ConstraintEvaluator.h>

#include "BESDapResponse.h"

namespace libdap {
class DataDDS;
}

/** @brief Represents an OPeNDAP DataDDS DAP2 data object within the BES
 */
class BESDataDDSResponse: public BESDapResponse {
private:
    libdap::DDS * _dds;
    libdap::ConstraintEvaluator _ce;
    bool include_attrs;


public:
    // Be dedault, the include_attrs flag is always true. This flag
    // will be set to false only for those handlers that support the
    // data access without the need to generate attributes. KY 10/30/19
    BESDataDDSResponse(libdap::DDS * dds) :
        BESDapResponse(), _dds(dds),include_attrs(true)
    {
    }

    virtual ~BESDataDDSResponse();

    virtual void set_container(const std::string &cn);
    virtual void clear_container();

    void dump(std::ostream & strm) const override;

    /**
     * Set the response object's DDS. The caller should probably
     * free the existing DDS object before calling this method.
     */
    void set_dds(libdap::DDS *ddsIn)
    {
        _dds = ddsIn;
    }

    libdap::DDS * get_dds()
    {
        return _dds;
    }

    libdap::ConstraintEvaluator & get_ce()
    {
        return _ce;
    }

    void set_ia_flag(bool ia_flag) {include_attrs = ia_flag;}
    bool get_ia_flag() {return include_attrs;}
};

#endif // I_BESDataDDSResponse

