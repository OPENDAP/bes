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
#include <fstream>

#include "BESFileLockingCache.h"
#include "BESInternalFatalError.h"
#include "BESContainer.h"

/// Setting XML_BASE_MISSING_MEANS_OMIT_ATTRIBUTE to zero causes the MDS to throw
/// BESInternalError when the xml:base context is not defined and a DMR/++ response
/// is accessed. This will break tests in dapreader and dmrpp_module. jhrg 6/6/18
#define XML_BASE_MISSING_MEANS_OMIT_ATTRIBUTE 1

namespace libdap {
class DapObj;
class DAS;
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
    std::string d_xml_base;     // The value of the context xml:basse

    static bool d_enabled;

    std::ofstream of;

    friend class DmrppMetadataStore;
    friend class DmrppMetadataStoreTest;
    friend class GlobalMetadataStoreTest;

    // Only get_instance() should be used to instantiate this class
    GlobalMetadataStore();
    GlobalMetadataStore(const std::string &cache_dir, const std::string &prefix, unsigned long long size);

protected:
    std::string d_ledger_entry; // Built up as info is added, written on success
    void write_ledger();

    std::string get_hash(const std::string &name);

    /**
     * This class is an abstract base class that
     * defines a functor that writes the different DAP metadata
     * responses using a DDS or DMR. The concrete specializations StreamDDS,
     * StreamDAS and StreamDMR are instantiated and passed to
     * store_dap_response().
     *
     * @note These classes were written so that either the DDS _or_ DMR could be
     * used to write all of the three DAP2/4 metadata responses. That feature
     * worked for the most part, but highlighted some differences between the
     * two protocol versions that make it hard to produce identical responses
     * using both the DDS or DMR from the same dataset. This made testing hard
     * and meant that the result was 'unpredictable' for some edge cases. The symbol
     * SYMMETRIC_ADD_RESPONSES controls if this feature is on or not; currently it
     * is turned off.
     */
    struct StreamDAP : public std::unary_function<libdap::DapObj*, void> {
        libdap::DDS *d_dds;
        libdap::DMR *d_dmr;

        StreamDAP() : d_dds(0), d_dmr(0) {
            throw BESInternalFatalError("Unknown DAP object type.", __FILE__, __LINE__);
        }
        StreamDAP(libdap::DDS *dds) : d_dds(dds), d_dmr(0) { }
        StreamDAP(libdap::DMR *dmr) : d_dds(0), d_dmr(dmr) { }

        virtual void operator()(std::ostream &os) = 0;
    };

    /// Instantiate with a DDS or DMR and use to write the DDS response.
    struct StreamDDS : public StreamDAP {
        StreamDDS(libdap::DDS *dds) : StreamDAP(dds) { }
        StreamDDS(libdap::DMR *dmr) : StreamDAP(dmr) { }

        virtual void operator()(std::ostream &os);
    };

    /// Instantiate with a DDS or DMR and use to write the DAS response.
    struct StreamDAS : public StreamDAP {
        StreamDAS(libdap::DDS *dds) : StreamDAP(dds) { }
        StreamDAS(libdap::DMR *dmr) : StreamDAP(dmr) { }

        virtual void operator()(std::ostream &os);
    };

    /// Instantiate with a DDS or DMR and use to write the DMR response.
    struct StreamDMR : public StreamDAP {
        StreamDMR(libdap::DDS *dds) : StreamDAP(dds) { }
        StreamDMR(libdap::DMR *dmr) : StreamDAP(dmr) { }

        virtual void operator()(std::ostream &os);
    };

    bool store_dap_response(StreamDAP &writer, const std::string &key, const std::string &name, const std::string &response_name);

    void write_response_helper(const std::string &name, std::ostream &os, const std::string &suffix,
        const std::string &object_name);

    // This version adds xml:base to the DMR/DMR++
    void write_response_helper(const std::string &name, std::ostream &os, const std::string &suffix,
        const std::string &xml_base, const std::string &object_name);

    bool remove_response_helper(const std::string& name, const std::string &suffix, const std::string &object_name);

    static void transfer_bytes(int fd, std::ostream &os);
    static void insert_xml_base(int fd, std::ostream &os, const std::string &xml_base);

public:
    /**
     * @brief Unlock and close the MDS item when the ReadLock goes out of scope.
     * @note This needs to be public because software that uses the MDS needs to
     * hold instances of the MDSReadLock.
     * @note Passing in a pointer to an instance of GlobalMetadataStore makes
     * GlobalMetadataStore easier to subclass. If _get_instance()_ is called
     * in this code, then only a GlobalMetadataStore, and not the subclass,
     * will be used to unlock the item.
     */
    struct MDSReadLock : public std::unary_function<std::string, bool> {
        std::string name;
        bool locked;
        GlobalMetadataStore *mds;
        MDSReadLock() : name(""), locked(false), mds(0) { }
        MDSReadLock(const std::string n, bool l, GlobalMetadataStore *store): name(n), locked(l), mds(store) { }
        ~MDSReadLock() {
            if (locked) mds->unlock_and_close(name);
            locked = false;
        }

         virtual bool operator()() { return locked; }

         //used to set 'locked' to false to force reload of file in cache. SBL 6/7/19
         virtual void clearLock() {
        	 if (locked) mds->unlock_and_close(name);
        	 locked = false;
         }//end clearLock()
     };

    using MDSReadLock = struct MDSReadLock;

protected:
    MDSReadLock get_read_lock_helper(const std::string &name, const std::string &suffix, const std::string &object_name);

    // Suppress the automatic generation of these ctors
    GlobalMetadataStore(const GlobalMetadataStore& ) = delete;
    GlobalMetadataStore& operator=(const GlobalMetadataStore&) = delete;

    void initialize();

    // these are static because they are called by the static method get_instance()
    static std::string get_cache_dir_from_config();
    static std::string get_cache_prefix_from_config();
    static unsigned long get_cache_size_from_config();

public:
    static GlobalMetadataStore *get_instance(const std::string &cache_dir, const std::string &prefix,
        unsigned long long size);
    static GlobalMetadataStore *get_instance();

    ~GlobalMetadataStore() override = default;

    virtual bool add_responses(libdap::DDS *dds, const std::string &name);
    virtual bool add_responses(libdap::DMR *dmr, const std::string &name);

    virtual MDSReadLock is_dmr_available(const std::string &name);
    virtual MDSReadLock is_dmr_available(const BESContainer &container);
    //added a third method here to handle case in build_dmrpp.cc - SBL 6.19.19
    virtual MDSReadLock is_dmr_available(const std::string &realName, const std::string &relativeName, const std::string &fileType);

    virtual MDSReadLock is_dds_available(const std::string &name);
    virtual MDSReadLock is_dds_available(const BESContainer &container);

    virtual MDSReadLock is_das_available(const std::string &name);
    virtual MDSReadLock is_das_available(const BESContainer &container);

    virtual MDSReadLock is_dmrpp_available(const std::string &name);
    virtual MDSReadLock is_dmrpp_available(const BESContainer &container);

    virtual bool is_available_helper(const std::string &realName, const std::string &relativeName, const std::string &fileType, const std::string &suffix);

    virtual time_t get_cache_lmt(const std::string &fileName, const std::string &suffix);

    virtual void write_dds_response(const std::string &name, std::ostream &os);
    virtual void write_das_response(const std::string &name, std::ostream &os);

    // @TODO Add a third parameter to enable changing the value of xmlbase in this response.
    // jhrg 2.28.18
    virtual void write_dmr_response(const std::string &name, std::ostream &os);
    virtual void write_dmrpp_response(const std::string &name, std::ostream &os);

    virtual bool remove_responses(const std::string &name);

    virtual libdap::DDS *get_dds_object(const std::string &name);
    virtual libdap::DMR *get_dmr_object(const std::string &name);

    virtual void parse_das_from_mds(libdap::DAS*das, const std::string &name);
};

} // namespace bes

#endif // _global_metadata_cache_h
