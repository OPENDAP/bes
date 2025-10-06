// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of HYrax, A C++ implementation of the OPeNDAP Data
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

#include "config.h"

#include <iostream>
#include <string>
#include <sstream>
#include <memory>
#include <typeinfo>

#include <libdap/DMR.h>
#include <libdap/XMLWriter.h>

#include "BESDebug.h"

#include "BESInternalFatalError.h"

#include "DmrppTypeFactory.h"
#include "DmrppMetadataStore.h"

#include <D4Group.h>
#include <DMZ.h>

#include "DMRpp.h"

#define DEBUG_KEY "dmrpp_store"
#define MAINTAIN_STORE_SIZE_EVEN_WHEN_UNLIMITED 0

#ifdef HAVE_ATEXIT
#define AT_EXIT(x) atexit((x))
#else
#define AT_EXIT(x)
#endif

/// If SYMETRIC_ADD_RESPONSES is defined and a true value, then add_responses()
/// will add all of DDS, DAS and DMR when called with _either_ the DDS or DMR
/// objects. If it is not defined (or false), add_responses() called with a DDS
/// will add only the DDS and DAS and add_responses() called with a DMR will add
/// only a DMR.
///
/// There are slight differences in the DAS objects build by the DMR and DDS,
/// especially when the underlying dataset contains types that can be encoded
/// in the DMR (DAP4) but not the DDS (DAP2). jhrg 3/20/18
#undef SYMETRIC_ADD_RESPONSES

using namespace std;
using namespace libdap;
using namespace bes;

using namespace dmrpp;

DmrppMetadataStore *DmrppMetadataStore::d_instance = 0;
bool DmrppMetadataStore::d_enabled = true;

/**
 * @name Get an instance of DmrppMetadataStore
 * @brief  There are two ways to get an instance of DmrppMetadataStore singleton.
 *
 * @note If the cache_dir parameter is the empty string, get_instance() will return null
 * for the pointer to the singleton and caching is disabled. This means that if the cache
 * directory is not set in the bes.conf file(s), then the cache will be disabled. If
 * the cache directory is given (or set in bes.conf) but the prefix or size is not,
 * that's an error. If the directory is named but does not exist, it will
 * be made. If the BES cannot make it, then an error will be signaled.
 *
 * @return A pointer to a DmrppMetadataStore object; null if the cache is disabled.
 */
///@{
/**
 * @brief Get an instance of the DmrppMetadataStore object.
 *
 * This class is a singleton, so the first call to any of two 'get_instance()' methods
 * makes an instance and subsequent calls return a pointer to that instance.
 *
 * @param cache_dir_key Key to use to get the value of the cache directory. If this is
 * the empty string, return null right away.
 * @param prefix_key Key for the item/file prefix. Each item added to the cache uses this
 * as a prefix so cached items can be easily identified when the same directory is used for
 * several caches or /tmp is used for the cache.
 * @param size_key The maximum size of the data stored in the cache, in megabytes
 *
 * @return A pointer to a DmrppMetadataStore object. If the cache is disabled, then
 * the pointer returned will be null and the cache will be marked as not enabled.
 * Subsequent calls will return immediately.
 */
 // FIXME This code will not work correctly in a multi-thread situation. See http::EffectiveUrlCache();
DmrppMetadataStore *
DmrppMetadataStore::get_instance(const string &cache_dir, const string &prefix, unsigned long long size)
{
    if (d_enabled && d_instance == 0) {
        d_instance = new DmrppMetadataStore(cache_dir, prefix, size); // never returns null_ptr
        d_enabled = d_instance->cache_enabled();
        if (!d_enabled) {
            delete d_instance;
            d_instance = 0;

            BESDEBUG(DEBUG_KEY, "DmrppMetadataStore::"<<__func__ << "() - " << "Cache is DISABLED"<< endl);
        }
        else {
            AT_EXIT(delete_instance);

            BESDEBUG(DEBUG_KEY, "DmrppMetadataStore::"<<__func__ << "() - " << "Cache is ENABLED"<< endl);
        }
    }

    BESDEBUG(DEBUG_KEY, "DmrppMetadataStore::get_instance(dir,prefix,size) - d_instance: " << d_instance << endl);

    return d_instance;
}

/**
 * Get an instance of the DmrppMetadataStore using the default values for the cache directory,
 * prefix and size.
 *
 * @return A pointer to a DmrppMetadataStore object; null if the cache is disabled.
 */
DmrppMetadataStore *
DmrppMetadataStore::get_instance()
{
    if (d_enabled && d_instance == 0) {
        d_instance = new DmrppMetadataStore();
        d_enabled = d_instance->cache_enabled();
        if (!d_enabled) {
            delete d_instance;
            d_instance = NULL;
            BESDEBUG(DEBUG_KEY, "DmrppMetadataStore::"<<__func__ << "() - " << "Cache is DISABLED"<< endl);
        }
        else {
            AT_EXIT(delete_instance);

            BESDEBUG(DEBUG_KEY, "DmrppMetadataStore::"<<__func__ << "() - " << "Cache is ENABLED"<< endl);
        }
    }

    BESDEBUG(DEBUG_KEY, "DmrppMetadataStore::get_instance() - d_instance: " << (void *) d_instance << endl);

    return d_instance;
}
///@}

void DmrppMetadataStore::StreamDMRpp::operator()(ostream &os)
{
    // Even though StreamDMRpp is-a StreamDAP and the latter has a d_dds
    // field, we cannot use it for this version of the output operator.
    // jhrg 5/17/18
    if (d_dmr && typeid(*d_dmr) == typeid(dmrpp::DMRpp)) {
        // FIXME This is where we will add the href that points toward the data file in S3. jhrg 5/17/18
        DMRpp *dmrpp = static_cast<dmrpp::DMRpp*>(d_dmr);
        dmrpp->set_print_chunks(true);
        XMLWriter xml;
        dmrpp->print_dap4(xml);

        os << xml.get_doc();
    }
    else {
        throw BESInternalFatalError("StreamDMRpp output operator call with non-DMRpp instance.", __FILE__, __LINE__);
    }
}

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
bool
DmrppMetadataStore::add_responses(DMR *dmr, const string &name)
{
    bool stored_dmr = GlobalMetadataStore::add_responses(dmr, name);

    bool stored_dmrpp = false;
    if (typeid(*dmr) == typeid(dmrpp::DMRpp)) {
        d_ledger_entry = string("add DMR++ ").append(name);

        StreamDMRpp write_the_dmrpp_response(dmr);
        stored_dmrpp = store_dap_response(write_the_dmrpp_response, get_hash(name + "dmrpp_r"), name, "DMRpp");

        write_ledger(); // write the index line
    }
    else {
        stored_dmrpp = true;    // if dmr is not a DMRpp, not writing the object is 'success.'
    }

    return(stored_dmr && stored_dmrpp);
}

bool
DmrppMetadataStore::add_dmrpp_response(libdap::DMR *dmrpp, const std::string &name)
{
    bool stored_dmrpp = false;
    if (typeid(*dmrpp) == typeid(dmrpp::DMRpp)) {
        d_ledger_entry = string("add DMR++ ").append(name);

        StreamDMRpp write_the_dmrpp_response(dmrpp);
        stored_dmrpp = store_dap_response(write_the_dmrpp_response, get_hash(name + "dmrpp_r"), name, "DMRpp");

        write_ledger(); // write the index line
    }
    else {
        stored_dmrpp = true;    // if dmr is not a DMRpp, not writing the object is 'success.'
    }

    return(stored_dmrpp);
}

/**
 * @brief Use the DMR response to build a DMR with Dmrpp Types
 * @param name The pathname to the dataset, relative to the BES
 * data root directory
 * @return A DMRpp using a pointer to the base class.
 */
DMR *
DmrppMetadataStore::get_dmr_object(const string &name)
{
    // Get the DMR response, but then parse that so the resulting binary
    // object is built with DMR++ types so that the chunk information can be
    // stored in them.
    stringstream oss;
    write_dmr_response(name, oss);    // throws BESInternalError if not found

    DmrppTypeFactory dmrpp_btf;
    unique_ptr<DMRpp> dmrpp(new DMRpp(&dmrpp_btf, "mds"));

    DMZ dmz;
    dmz.parse_xml_string(oss.str());
    dmz.build_thin_dmr(dmrpp.get());
    dmz.load_all_attributes(dmrpp.get());
    dmrpp->set_factory(nullptr);

    return dmrpp.release();
}

/**
 * @brief Build a DMR++ object from the cached Response
 *
 * Read and parse a DMR++ response , building a binary DMR++ object. The
 * object is returned with a null factory. The variables are built using
 * DmrppTypeFactory
 *
 * @param name Name of the dataset
 * @return A pointer to the DMR object; the caller must delete this object.
 * @exception BESInternalError is thrown if \arg name does not have a
 * cached DMR response.
 */
DMRpp *
DmrppMetadataStore::get_dmrpp_object(const string &name)
{
    stringstream oss;
    write_dmrpp_response(name, oss);    // throws BESInternalError if not found

    DmrppTypeFactory dmrpp_btf;
    unique_ptr<DMRpp> dmrpp(new DMRpp(&dmrpp_btf, "mds"));

    DMZ dmz;
    dmz.parse_xml_string(oss.str());
    dmz.build_thin_dmr(dmrpp.get());
    dmz.load_all_attributes(dmrpp.get());
    dmz.load_chunks(dmrpp.get()->root());
    dmrpp->set_factory(nullptr);

    return dmrpp.release();
}
