// DmrppRequestHandler.h

// Copyright (c) 2016 OPeNDAP, Inc. Author: James Gallagher
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
#include <ostream>

#include "BESRequestHandler.h"

class ObjMemCache;  // in bes/dap
class BESContainer;

namespace libdap {
	class DMR;
	class DDS;
}

namespace dmrpp {

class CurlHandlePool;

class DmrppRequestHandler: public BESRequestHandler {

private:
    // These are not used. See the netcdf handler for an example of their use.
    // jhrg 4/24/18
    static ObjMemCache *das_cache;
    static ObjMemCache *dds_cache;
    static ObjMemCache *dmr_cache;

	// These are static because they are used by the static public methods.
	static void build_dmr_from_file(BESContainer *container, libdap::DMR* dmr);

public:
	DmrppRequestHandler(const std::string &name);
	virtual ~DmrppRequestHandler();

    static CurlHandlePool *curl_handle_pool;

    static bool d_use_parallel_transfers;
    static unsigned int d_max_parallel_transfers;

    static int d_min_size;

	static bool dap_build_dmr(BESDataHandlerInterface &dhi);
	static bool dap_build_dap4data(BESDataHandlerInterface &dhi);
    static bool dap_build_das(BESDataHandlerInterface &dhi);
    static bool dap_build_dds(BESDataHandlerInterface &dhi);
    static bool dap_build_dap2data(BESDataHandlerInterface &dhi);

	static bool dap_build_vers(BESDataHandlerInterface &dhi);
	static bool dap_build_help(BESDataHandlerInterface &dhi);

	virtual void dump(std::ostream &strm) const;
};

} // namespace dmrpp

#endif // DmrppRequestHandler.h
