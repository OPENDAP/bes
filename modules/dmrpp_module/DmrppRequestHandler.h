// DmrppRequestHandler.h

// Copyright (c) 2013 OPeNDAP, Inc. Author: James Gallagher
// <jgallagher@opendap.org>, Patrick West <pwest@opendap.org>
// Nathan Potter <npotter@opendap.org>
//
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2.1 of
// the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
// 02110-1301 U\ SA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI.
// 02874-0112.

#ifndef I_DmrppRequestHandler_H
#define I_DmrppRequestHandler_H

#include <string>

#include "BESRequestHandler.h"

#undef DAP2

namespace libdap {
	class DMR;
	class DDS;
}

class DmrppRequestHandler: public BESRequestHandler {
#if 0
	static bool d_use_series_values;
	static bool d_use_series_values_set;

	static bool d_use_test_types;
	static bool d_use_test_types_set;
#endif

	// These are static because they are used by the static public methods.
	static void build_dmr_from_file(const string& accessed, bool explicit_containers, libdap::DMR* dmr);
#if DAP2
	static void build_dds_from_file(const string& accessed, bool explicit_containers, libdap::DDS* dds);
	static void load_dds_from_data_file(const string &accessed, libdap::DDS &dds);
#endif
public:
	DmrppRequestHandler(const string &name);
	virtual ~DmrppRequestHandler(void) { }

#if DAP2
	static bool dap_build_das(BESDataHandlerInterface &dhi);
	static bool dap_build_dds(BESDataHandlerInterface &dhi);
	static bool dap_build_data(BESDataHandlerInterface &dhi);
#endif
	static bool dap_build_dmr(BESDataHandlerInterface &dhi);
	static bool dap_build_dap4data(BESDataHandlerInterface &dhi);

	static bool dap_build_vers(BESDataHandlerInterface &dhi);
	static bool dap_build_help(BESDataHandlerInterface &dhi);

	virtual void dump(ostream &strm) const;
};

#endif // DmrppRequestHandler.h
