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

#include <ConstraintEvaluator.h>

#include "BESDapResponse.h"

namespace libdap {
class DataDDS;
}

/** @brief Represents an OPeNDAP DataDDS DAP2 data object within the BES
 */
class BESDataDDSResponse: public BESDapResponse {
private:
    libdap::DataDDS * _dds;
    libdap::ConstraintEvaluator _ce;

public:
    BESDataDDSResponse(libdap::DataDDS * dds) :
        BESDapResponse(), _dds(dds)
    {
    }

    virtual ~BESDataDDSResponse();

    virtual void set_container(const std::string &cn);
    virtual void clear_container();

    virtual void dump(std::ostream & strm) const;

    /**
     * Set the response object's DDS. The caller should probably
     * free the existing DDS object before calling this method.
     */
    void set_dds(libdap::DataDDS *ddsIn)
    {
        _dds = ddsIn;
    }

    libdap::DataDDS * get_dds()
    {
        return _dds;
    }

    libdap::ConstraintEvaluator & get_ce()
    {
        return _ce;
    }
};

#endif // I_BESDataDDSResponse

