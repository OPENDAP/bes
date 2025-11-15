// BESDapResponse.h

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

#ifndef I_BESDapResponse
#define I_BESDapResponse 1

#include "BESResponseObject.h"
#include "BESDataHandlerInterface.h"

/** @brief Represents an OPeNDAP DAP response object within the BES
 */
class BESDapResponse: public BESResponseObject {
private:
	std::string d_dap_client_protocol;
	bool d_explicit_containers;

	std::string d_request_xml_base;

protected:
	bool is_dap2();
	void read_contexts();

public:
	BESDapResponse() :
			BESResponseObject(), d_dap_client_protocol("2.0"), d_explicit_containers(true), d_request_xml_base("")
	{
		read_contexts();
	}

	virtual ~BESDapResponse()
	{
	}

	/// Return the dap version string sent by the client (e.g., the OLFS)
	std::string get_dap_client_protocol() const
	{
		return d_dap_client_protocol;
	}

	/// Should containers be explicitly represented in the DD* responses?
	bool get_explicit_containers() const
	{
		return d_explicit_containers;
	}

	/// Return the xml:base URL for this request.
	std::string get_request_xml_base() const
	{
		return d_request_xml_base;
	}

	virtual void set_constraint(BESDataHandlerInterface &dhi);
	virtual void set_dap4_constraint(BESDataHandlerInterface &dhi);
	virtual void set_dap4_function(BESDataHandlerInterface &dhi);

    virtual void set_container(const std::string &cn) = 0;
    virtual void clear_container() = 0;

	void dump(std::ostream &strm) const override;
};

#endif // I_BESDapResponse
