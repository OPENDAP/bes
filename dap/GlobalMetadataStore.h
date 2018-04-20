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
#include "BESInternalFatalError.h"

namespace libdap {
class DapObj;
class DDS;
class DMR;
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
 * @note To change the xml:base attribute in the DMR response use
 * `DMR::set_request_xml_base()`.
 *
 * @todo Add support for storing binary DDS and DMR objects. This will require
 * modifications to libdap so that we can 'serialize' those and additions to
 * some of the handlers so that they can record extra information used by their
 * specializations of those objects.
 *
 * @author jhrg
 */
class GlobalMetadataStore: public BESFileLockingCache {
private:
    bool d_use_local_time;      // Base on BES.LogTimeLocal
    std::string d_ledger_name;  // Name of the ledger file
    std::string d_ledger_entry; // Built up as info is added, written on success

    static bool d_enabled;
    static GlobalMetadataStore *d_instance;

    // Called by atexit()
    static void delete_instance() {
        delete d_instance;
        d_instance = 0;
    }

    void write_ledger();

    std::string get_hash(const std::string &name);

    /**
     * This class is an abstract base class that
     * defines a functor that writes the different DAP metadata
     * responses using a DDS or DMR. The concrete specializations StreamDDS,
     * StreamDAS and StreamDMR are instantiated and passed to
     * store_dap_response().
     *
     * @note These classes were written to so either the DDS or DMR could be
     * used to build all of the (three) metadata responses. See comments
     * elsewhere about this and why it's turned off for now.
     */
    struct StreamDAP : public std::unary_function<libdap::DapObj*, void> {
        libdap::DDS *d_dds;
        libdap::DMR *d_dmr;

        StreamDAP() {
            throw BESInternalFatalError("Unknown DAP object type.", __FILE__, __LINE__);
        }
        StreamDAP(libdap::DDS *dds) : d_dds(dds), d_dmr(0) { }
        StreamDAP(libdap::DMR *dmr) : d_dds(0), d_dmr(dmr) { }

        virtual void operator()(std::ostream &os) = 0;
    };

    /**
     * Instantiate with a DDS or DMR and use to write the DDS response.
     */
    struct StreamDDS : public StreamDAP {
        StreamDDS(libdap::DDS *dds) : StreamDAP(dds) { }
        StreamDDS(libdap::DMR *dmr) : StreamDAP(dmr) { }

        virtual void operator()(ostream &os);
    };

    /**
     * Instantiate with a DDS or DMR and use to write the DAS response.
     */
    struct StreamDAS : public StreamDAP {
        StreamDAS(libdap::DDS *dds) : StreamDAP(dds) { }
        StreamDAS(libdap::DMR *dmr) : StreamDAP(dmr) { }

        virtual void operator()(ostream &os);
    };

    /**
     * Instantiate with a DDS or DMR and use to write the DMR response.
     */
    struct StreamDMR : public StreamDAP {
        StreamDMR(libdap::DDS *dds) : StreamDAP(dds) { }
        StreamDMR(libdap::DMR *dmr) : StreamDAP(dmr) { }
        // Implementation in .cc since it uses libdap headers.
        virtual void operator()(ostream &os);
    };

    bool store_dap_response(StreamDAP &writer, const std::string &key, const string &name, const string &response_name);

    void get_response_helper(const std::string &name, std::ostream &os, const std::string &suffix,
        const std::string &object_name);

    bool remove_response_helper(const std::string& name, const std::string &suffix, const std::string &object_name);

public:
    /**
     * @brief Unlock and close the MDS item when the ReadLock goes out of scope.
     * @note This needs to be public because software that uses the MDS needs to
     * hold instances of the MDSReadLock.
     */
    struct MDSReadLock : public std::unary_function<std::string, bool> {
        std::string name;
        bool locked;
        MDSReadLock() : name(""), locked(false) { }
        MDSReadLock(const std::string n, bool l): name(n), locked(l) { }
        ~MDSReadLock() {
            if (locked) get_instance()->unlock_and_close(name);
            locked = false;
        }

         virtual bool operator()() { return locked; }
     };

    typedef struct MDSReadLock MDSReadLock;

private:
    MDSReadLock get_read_lock_helper(const string &name, const string &suffix, const string &object_name);

    // Suppress the automatic generation of these ctors
    GlobalMetadataStore();
    GlobalMetadataStore(const GlobalMetadataStore &src);

    // Only get_instance() should be used to instantiate this class
    GlobalMetadataStore(const std::string &cache_dir, const std::string &prefix, unsigned long long size);

    // these are static because they are called by the static method get_instance()
    static string get_cache_dir_from_config();
    static string get_cache_prefix_from_config();
    static unsigned long get_cache_size_from_config();

    friend class GlobalMetadataStoreTest;

public:
    static GlobalMetadataStore *get_instance(const std::string &cache_dir, const std::string &prefix,
        unsigned long long size);
    static GlobalMetadataStore *get_instance();

    virtual ~GlobalMetadataStore()
    {
    }

    // I moved this from the .cc file where it really belongs to here because
    // the docs were not getting built when the comments were in the the .cc
    // file (only these two methods).
    /**
     * @name Add responses to the GlobalMetadataStore
     * @brief Use a DDS or DMR to populate DAP metadata responses in the MDS
     *
     * These methods uses a DDS or DMR object to generate the DDS, DAS and DMR responses
     * for DAP (2 and 4). They store those in the MDS and then update the
     * MDS ledger file with the operation (add), the kind of object used
     * to build the responses (DDS or DMR), name of the granule and hashes/names
     * for each of the three files in the MDS that hold the responses.
     *
     * If verbose logging is on, the bes log also will hold information about
     * the operation. If there is an error, that will always be recorded in
     * the bes log.
     *
     * @return True if the DDS, DAS or DMR were added to the MDS
     */
    ///@{

    /**
     * @brief Add the DAP2 metadata responses using a DDS
     *
     * This method adds only the DDS and DAS unless the code was compiled with
     * the symbol SYMETRIC_ADD_RESPONSES defined.
     *
     * @param name The granule name or identifier
     * @param dds A DDS built from the granule
     * @return True if all of the cache/store entries were written, False if any
     * could not be written.
     */
    virtual bool add_responses(libdap::DDS *dds, const std::string &name);

    /**
     * @brief Add the DAP4 metadata responses using a DMR
     *
     * This method adds only the DMR unless the code was compiled with
     * the symbol SYMETRIC_ADD_RESPONSES defined.
     *
     * @param name The granule name or identifier
     * @param dmr A DMR built from the granule
     * @return True if all of the cache/store entry was written, False if any
     * could not be written.
     */
    virtual bool add_responses(libdap::DMR *dmr, const std::string &name);
    ///@}

    virtual MDSReadLock is_dmr_available(const std::string &name);
    virtual MDSReadLock is_dds_available(const std::string &name);
    virtual MDSReadLock is_das_available(const std::string &name);

    virtual void get_dds_response(const std::string &name, std::ostream &os);
    virtual void get_das_response(const std::string &name, std::ostream &os);

    // Add a third parameter to enable changing the value of xmlbase in this response.
    // jhrg 2.28.18
    virtual void get_dmr_response(const std::string &name, std::ostream &os);

    virtual bool remove_responses(const std::string &name);

#if 1
    virtual libdap::DDS *get_dds_object(const std::string &name);
#endif
#if 1
    virtual libdap::DMR *get_dmr_object(const std::string &name);
#endif

};

} // namespace bes

#endif // _global_metadata_cache_h
