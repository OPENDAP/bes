// BESDMRResponse.h

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2013 OPeNDAP, Inc.
// Author: Patrick West <pwest@rpi.edu>
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

#ifndef I_BESDMRResponse
#define I_BESDMRResponse 1

#include <libdap/DMR.h>
//#include <libdap/ConstraintEvaluator.h>

#include "BESDapResponse.h"

using namespace libdap; 
#if 0
{
class DMR;
class ConstraintEvaluator;
    
}
#endif
// Remove this if we can get the ConstraintEvalutor out of this code.
#include <libdap/ConstraintEvaluator.h>


//class DMR;

/** @brief Represents an OPeNDAP DMR DAP4 data object within the BES
 */
class BESDMRResponse: public BESDapResponse {
private:
	DMR * _dmr;
	ConstraintEvaluator _ce; //FIXME Use Dap4 CE stuff
public:
	BESDMRResponse(DMR *dmr);

	virtual ~BESDMRResponse();

	virtual void set_container(const std::string &cn);
	virtual void clear_container();

	void dump(std::ostream &strm) const override;

	DMR *get_dmr() { return _dmr; }
	void set_dmr(DMR *dmr) { _dmr = dmr; }

    ConstraintEvaluator &get_ce() { return _ce; } // FIXME too...
};

#endif // I_BESDMRResponse
