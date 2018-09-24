// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of Hyrax, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2018 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
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

#ifndef _dmrpp_metadata_store_h
#define _dmrpp_metadata_store_h

#include <string>
//#include <functional>

#include "GlobalMetadataStore.h"

//#include "DMRpp.h"

namespace libdap {
class DMR;
}

namespace dmrpp {
class DMRpp;
}

namespace bes {

/**
 * @brief Store the DAP DMR++ metadata responses.
 *
 * Provide a way to add DMR++ responses to the Global MDS (see GlobalMetadataStore).
 * This specialization of the MDS adds the ability to add DMR++ responses and
 * extract DMR++ objects. This code is accessible only in the DMR++ handler, while
 * the Global MDS is available anywhere (it's defined in bes/dap). Because the
 * BES framework needs to determine if DMR++ responses are included in the MDS,
 * the Global MDS class has methods to test for that and to 'write' that response.
 *
 * In order for the BES to work with a DMR++ _object_, however, it will need to
 * transfer control to the DMR++ Handler and, if it is going to use the response
 * in the MDS, use this specialization to read from the MDS.
 *
 * The class is implemented as a singleton; use the get_instance() methods to
 * get an instance of the DMR++ metadata store. It uses _the same_ configuration
 * parameters as the Global MDS, so if the defaults are uses, both the singleton
 * Global MDS and this DMR++ MDS will be using the same directory, file prefix and
 * size limit.
 *
 * BES Keys used:
 * - _DAP.GlobalMetadataStore.path_: store root directory (assumes the store is
 *   using a POSIX file system)
 *
 * - _DAP.GlobalMetadataStore.prefix_: prefix for the names of items in the store
 *
 * - _DAP.GlobalMetadataStore.size_: Maximum size of the store. Zero indicates
 *   unlimited size.
 *
 * - _DAP.GlobalMetadataStore.ledger_: Name of the ledger. A relative pathname.
 *   will be interpreted as relative to the directory where the BES was started.
 *   The default name mds_ledger.txt
 *
 * - _BES.LogTimeLocal_: Use local or GMT time for the ledger entries; default is
 *   to use GMT
 *
 * @author jhrg
 */
class DmrppMetadataStore: public GlobalMetadataStore {
private:
    static bool d_enabled;
    static DmrppMetadataStore *d_instance;

    // Called by atexit()
    static void delete_instance() {
        delete d_instance;
        d_instance = 0;
    }

    friend class DmrppMetadataStoreTest;

protected:
    /// Hack use a DMR to write a DMR++ response. WIP
    struct StreamDMRpp : public StreamDAP {
        StreamDMRpp(libdap::DMR *dmrpp) : StreamDAP(dmrpp) {}
        // Use the DMR to write the DMR++ when the DMR has variables of the correct
        // type. Look for this by checking the type of the object at runtime.
        virtual void operator()(std::ostream &os);
    };

    DmrppMetadataStore(const DmrppMetadataStore &src) : bes::GlobalMetadataStore(src) { }

    // Only get_instance() should be used to instantiate this class
    DmrppMetadataStore() : bes::GlobalMetadataStore() { }
    DmrppMetadataStore(const std::string &cache_dir, const std::string &prefix, unsigned long long size) :
        bes::GlobalMetadataStore(cache_dir, prefix, size) { }

public:
    static DmrppMetadataStore *get_instance(const std::string &cache_dir, const std::string &prefix, unsigned long long size);
    static DmrppMetadataStore *get_instance();

    virtual ~DmrppMetadataStore()
    {
    }

    virtual bool add_responses(libdap::DMR *dmrpp, const std::string &name);
    virtual bool add_dmrpp_response(libdap::DMR *dmrpp, const std::string &name);

    virtual libdap::DMR *get_dmr_object(const string &name);

    virtual dmrpp::DMRpp *get_dmrpp_object(const std::string &name);
};

} // namespace bes

#endif // _dmrpp_metadata_store_h
