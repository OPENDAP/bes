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

#ifndef _global_metadata_cache_h
#define _global_metadata_cache_h

#include <string>
#include <functional>

#include "BESFileLockingCache.h"

namespace libdap {
class DDS;
class DMR;
}

namespace bes {

/**
 * @brief Cache the results from server functions.
 *
 * @author jhrg
 */

class GlobalMetadataStore: public BESFileLockingCache {
private:
    typedef void (libdap::DDS::*print_method_t)(std::ostream &);

    std::string d_inventory_name;   ///> Name of the inventory file
    std::string d_inventory_entry;  ///> Built up as info is added, written on success

    static bool d_enabled;
    static GlobalMetadataStore *d_instance;

    // Called by atexit()
    static void delete_instance() {
        delete d_instance;
        d_instance = 0;
    }

    void write_inventory();

    std::string get_hash(const std::string &name);

    /**
     * This class is an abstract base class that provides a way to
     * define a functor that writes the difference DAP metadata
     * responses using a DDS. The concrete specializations StreamDDS,
     * StreamDAS and StreamDMR are instantiated and passed to
     * store_dap_response().
     */
    struct StreamDAP : public std::unary_function<libdap::DDS*, void> {
        libdap::DDS *d_dds;

        StreamDAP(libdap::DDS *dds) : d_dds(dds) { }

        virtual void operator()(std::ostream &os) = 0;
    };

    /**
     * Instantiate with a DDS and use to write the DDS response.
     */
    struct StreamDDS : public StreamDAP {
        StreamDDS(libdap::DDS *dds) : StreamDAP(dds) { }

        virtual void operator()(ostream &os) {
            d_dds->print(os);
        }
    };

    /**
     * Instantiate with a DDS and use to write the DAS response.
     */
    struct StreamDAS : public StreamDAP {
        StreamDAS(libdap::DDS *dds) : StreamDAP(dds) { }

        virtual void operator()(ostream &os) {
            d_dds->print_das(os);
        }
    };

    /**
     * Instantiate with a DDS and use to write the DMR response.
     */
    struct StreamDMR : public StreamDAP {
        StreamDMR(libdap::DDS *dds) : StreamDAP(dds) { }

        virtual void operator()(ostream &os);
    };

    bool store_dap_response(StreamDAP &writer, const std::string &key);

    // Suppress the automatic generation of these ctors
    GlobalMetadataStore();
    GlobalMetadataStore(const GlobalMetadataStore &src);

    // these are static because they are called by get_instance()
    static string get_cache_dir_from_config();
    static string get_cache_prefix_from_config();
    static unsigned long get_cache_size_from_config();

    friend class GlobalMetadataStoreTest;

public:
    static GlobalMetadataStore *get_instance(const string &cache_dir, const string &prefix, unsigned long long size);
    static GlobalMetadataStore *get_instance();

    GlobalMetadataStore(const string &cache_dir, const string &prefix, unsigned long long size);

    virtual ~GlobalMetadataStore()
    {
    }

    virtual bool add_object(libdap::DDS *dds, const std::string &name);

    virtual void get_dds_response(const std::string &name, ostream &os);
    virtual void get_das_response(const std::string &name, ostream &os);

    virtual bool remove_object(const std::string &name);

#if 0
    virtual bool add_object(libdap::DMR *dmr, const std::string &name) { }

    virtual std::string get_dmr_response(const std::string &name) { }

    // These 'get' methods return null or the empty string if the thing is not in the store.
    virtual libdap::DDS *get_dds_object(const std::string &name) { }
    virtual libdap::DMR *get_dmr_object(const std::string &name) { }
#endif

};

} // namespace bes

#endif // _global_metadata_cache_h
