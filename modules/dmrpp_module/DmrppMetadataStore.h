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
 * @brief Store the DAP metadata responses.
 *
 * Provide a global persistent store for the DAP metadata responses. Using either
 * a DDS or a DMR, write the three DAP metadata responses to the global metadata
 * store. This class also provides methods to get those responses and write them
 * to an output stream and a method to remove the responses.
 *
 * The class maintains a ledger of 'add' and 'remove' operations.
 *
 * The class is implemented as a singleton; use the get_instance() methods to
 * get an instance of the metadata store.
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
class DmrppMetadataStore: public bes::GlobalMetadataStore {
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

    virtual dmrpp::DMRpp *get_dmrpp_object(const std::string &name);
};

} // namespace bes

#endif // _dmrpp_metadata_store_h
