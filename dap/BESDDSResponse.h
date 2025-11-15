// BESlibdap::DDSResponse.h

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

#ifndef I_BESDDSResponse
#define I_BESDDSResponse 1

#include "BESDapResponse.h"

#define FORCE_DAP_VERSION_TO_3_2 false

namespace libdap {
class DDS;
class ConstraintEvaluator;
}

// Remove this if we can get the ConstraintEvalutor out of this code.
#include <libdap/ConstraintEvaluator.h>

/** @brief Holds a DDS object within the BES
 */
class BESDDSResponse: public BESDapResponse {
private:
    libdap::DDS *_dds;
    libdap::ConstraintEvaluator _ce;

public:
    BESDDSResponse(libdap::DDS *dds) : BESDapResponse(), _dds(dds)
    {
    }

    virtual ~BESDDSResponse();

    virtual void set_container(const std::string &cn);
    virtual void clear_container();

    void dump(std::ostream &strm) const override;

    /**
     * Set the response object's DDS. The caller should probably
     * free an existing DDS object held here before calling this method.
     */
    void set_dds(libdap::DDS *ddsIn)
    {
        _dds = ddsIn;
    }

    /**
     * Get the contained DDS object.
     * @return The contained DDS object
     */
    libdap::DDS * get_dds()
    {
        return _dds;
    }

    /**
     * Get a reference to the DAP2 Constraint Evaluator.
     * @todo Remove use of this method - there is no reason code
     * cannot create its own ConstraintEvaluator.
     * @deprecated
     * @return A reference to a ConstraitEvaluator object
     */
    libdap::ConstraintEvaluator & get_ce()
    {
        return _ce;
    }
};

#endif // I_BESDDSResponse

