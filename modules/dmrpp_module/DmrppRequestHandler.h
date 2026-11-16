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
#include <memory>
#include <ostream>

#include "BESRequestHandler.h"
#include "DMZ.h"

class ObjMemCache;  // in bes/dap
class BESContainer;
class BESDataDDSResponse;

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
    // These are now used - not sure when we started using them. We might also
    // look into whether these and the MDS are really appropriate for the DMR++
    // code since it is, effectively, using cached metadata. The MDS caches info
    // as XML, and the DMR++ is XML, so the difference is negligible. In the case
    // of the memory cache, the size of the DMZ in memory may be an issue.
    // jhrg 11/12/21
    static std::unique_ptr<ObjMemCache> das_cache;
    static std::unique_ptr<ObjMemCache> dds_cache;

    static bool d_use_object_cache;
    static int d_object_cache_entries;
    static double d_object_cache_purge_level;


    static void get_dmrpp_from_container_or_cache(BESContainer *container, libdap::DMR *dmr);
    template <class T> static void get_dds_from_dmr_or_cache(BESContainer *container, T *bdds);

    // Allocate a new DMZ for each request? That should work, but may result in more
    // cycling of data in and out of memory. The shared_ptr<> will be passed into
    // instances of BaseType and used from within the specialized read methods.
    // jhrg 11/3/21
    static std::shared_ptr<DMZ> dmz;

public:
	explicit DmrppRequestHandler(const std::string &name);
	~DmrppRequestHandler() override;

    static CurlHandlePool *curl_handle_pool;

    static bool d_use_transfer_threads;
    static unsigned long d_max_transfer_threads;
    static unsigned long d_default_max_transfer_threads;
    // The number of parallel transfers assumed when choosing between the buffer chunk and the super chunk.
    // Never greater than d_max_transfer_threads. See DMRPP.DecisionParallelTransfers in dmrpp.conf.
    static unsigned long d_decision_transfer_threads;
    // The transfer time model used to choose between the buffer chunk and the super chunk:
    //   estimated time (ms) = number of request rounds * d_decision_request_overhead_ms
    //                         + bytes / d_decision_bytes_per_ms
    // Variables (or buffer chunks) smaller than d_decision_small_size bytes always use the buffer chunk.
    static unsigned long d_decision_bytes_per_ms;
    static unsigned long d_decision_request_overhead_ms;
    static unsigned long d_decision_small_size;

    static bool d_use_compute_threads;
    static unsigned long d_max_compute_threads;
    static unsigned long d_default_max_compute_threads;

    static unsigned long long d_contiguous_concurrent_threshold;

    static bool d_require_chunks;

    static bool is_netcdf4_enhanced_response;
    static bool is_netcdf4_classic_response;
    // Leave the following variables in case we need to consider the compression ratio for
    // the fileout netCDF big floating variable write optimization.
#if 0
    static bool float_write_opt_fonc;
    static bool float_write_opt_fonc_no_count_comp;
    static unsigned long long float_write_opt_fonc_buffer_size;
    static float float_write_opt_fonc_comp_ratio;
#endif
    static bool disable_direct_io;
    static bool use_buffer_chunk;

	static bool dap_build_dmr(BESDataHandlerInterface &dhi);
	static bool dap_build_dap4data(BESDataHandlerInterface &dhi);
    static bool dap_build_das(BESDataHandlerInterface &dhi);
    static bool dap_build_dds(BESDataHandlerInterface &dhi);
    static bool dap_build_dap2data(BESDataHandlerInterface &dhi);

	static bool dap_build_vers(BESDataHandlerInterface &dhi);
	static bool dap_build_help(BESDataHandlerInterface &dhi);

	void dump(std::ostream &strm) const override;
};

} // namespace dmrpp

#endif // DmrppRequestHandler.h
