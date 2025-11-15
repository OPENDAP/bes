// FitsRequestHandler.h

// This file is part of fits_handler, a data handler for the OPeNDAP data
// server.

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
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

// (c) COPYRIGHT University Corporation for Atmostpheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#ifndef I_FitsRequestHandler_H
#define I_FitsRequestHandler_H

#include "BESRequestHandler.h"

class FitsRequestHandler: public BESRequestHandler {
public:
	FitsRequestHandler(const std::string &name);
	virtual ~FitsRequestHandler(void);

	void dump(std::ostream &strm) const override;

	static bool fits_build_das(BESDataHandlerInterface &dhi);
	static bool fits_build_dds(BESDataHandlerInterface &dhi);
	static bool fits_build_data(BESDataHandlerInterface &dhi);

	static bool fits_build_dmr(BESDataHandlerInterface &dhi);

	static bool fits_build_vers(BESDataHandlerInterface &dhi);
	static bool fits_build_help(BESDataHandlerInterface &dhi);

    // This handler supports the "not including attributes" in
    // the data access feature. Attributes are generated only
    // if necessary. KY 10/30/19
    void add_attributes(BESDataHandlerInterface &dhi);

};

#endif

