// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2022 OPeNDAP
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
// Created by James Gallagher on 4/6/22.
//

#include "config.h"

#include <iostream>
#include <sstream>

#include "BESUtil.h"
#include "BESLog.h"
#include "BESSyntaxUserError.h"

#include <libdap/DDS.h>
#include <libdap/DMR.h>

#include "DapUtils.h"


using namespace libdap;

namespace dap_utils {

static void log_request_and_memory_size_helper(long req_size) {
    auto mem_size = BESUtil::get_current_memory_usage();    // size in KB or 0. jhrg 4/6/22
    if (mem_size) {
        INFO_LOG("request size: " << req_size << "KB|&|memory used by process: " << mem_size << "KB" << endl);
    }
    else {
        INFO_LOG("request size: " << req_size << "KB" << endl);
    }
}

/**
 * Log information about memory used by this request.
 *
 * Use the given DDS to log information about the request. As a bonus, log the
 * RSS for this process.
 *
 * @param dds Use this DDS to get the size of the request.
 */
void
log_request_and_memory_size(DDS *const *dds)
{
    auto req_size = (long)(*dds)->get_request_size_kb(true);
    log_request_and_memory_size_helper(req_size);
}

/**
 * Log information about memory used by this request.
 *
 * Use the given DDS to log information about the request. As a bonus, log the
 * RSS for this process.
 *
 * @param dmr Use this DMR to get the size of the request.
 */
void
log_request_and_memory_size(/*const*/ DMR &dmr)
{
    // The request_size_kb() method is not marked const. Fix. jhrg 4/6/22
    auto req_size = (long)dmr.request_size_kb(true);
    log_request_and_memory_size_helper(req_size);
}

/**
 *
 * @param dds
 */
void throw_for_dap4_typed_vars_or_attrs(unique_ptr<libdap::DDS> dds){
    vector<string> inventory;
    if(dds->is_dap4_projected(inventory)){
        stringstream msg;
        msg << endl;
        msg << "ERROR: Unable to convert a DAP4 DMR for this dataset to a DAP2 DDS object. " << endl;
        msg << "This dataset contains variables and/or attributes whose data types are not compatible " << endl;
        msg << "with the DAP2 data model." << endl;
        msg << endl;
        msg << "There are " << inventory.size() << " incompatible variables and/or attributes referenced " << endl;
        msg  << "in your request." << endl;
        msg << "Incompatible variables: " << endl;
        msg << endl;
        for(const auto &entry: inventory){
            msg << "    " << entry << endl;
        }
        throw BESSyntaxUserError(msg.str(), __FILE__, __LINE__);
    }

}



void throw_for_dap4_typed_vars_or_attrs(DDS *dds)
{
    vector<string> inventory;
    if(dds->is_dap4_projected(inventory)){
        stringstream msg;
        msg << endl;
        msg << "ERROR: Unable to convert a DAP4 DMR for this dataset to a DAP2 DDS object. " << endl;
        msg << "This dataset contains variables and/or attributes whose data types are not compatible " << endl;
        msg << "with the DAP2 data model." << endl;
        msg << endl;
        msg << "There are " << inventory.size() << " incompatible variables and/or attributes referenced " << endl;
        msg  << "in your request." << endl;
        msg << "Incompatible variables: " << endl;
        msg << endl;
        for(const auto &entry: inventory){
            msg << "    " << entry << endl;
        }
        throw BESSyntaxUserError(msg.str(), __FILE__, __LINE__);
    }
}

void throw_for_dap4_typed_attrs(DAS *das)
{
    vector<string> inventory;
    if(has_dap4_typed_attributes("", das->container(), inventory)){
        stringstream msg;
        msg << endl;
        msg << "ERROR: Unable to convert a DAP4 DMR for this dataset to a DAP2 DAS object. " << endl;
        msg << "This dataset contains attributes whose data types are not compatible " << endl;
        msg << "with the DAP2 data model." << endl;
        msg << endl;
        msg << "There are " << inventory.size() << " incompatible attributes referenced " << endl;
        msg  << "in your request." << endl;
        msg << "Incompatible attributes: " << endl;
        msg << endl;
        for(const auto &entry: inventory){
            msg << "    " << entry << endl;
        }
        throw BESSyntaxUserError(msg.str(), __FILE__, __LINE__);
    }
}


bool has_dap4_typed_attributes(const std::string &path, AttrTable *atable, std::vector<std::string> &inventory)
{
    if(!atable)
        return false;

    bool has_d4_attr = false;
    for(auto aitr=atable->attr_begin();aitr!=atable->attr_end();aitr++){
        bool ima_d4_attr = false;
        string attr_fqn = path + "@" + (*aitr)->name;
        switch ((*aitr)->type) {
            case Attr_int8:
            case Attr_int64:
            case Attr_uint64:
            case Attr_enum:
            case Attr_opaque:
            {
                ima_d4_attr = true;
                break;
            }
            case Attr_container:
            {
                ima_d4_attr |= has_dap4_typed_attributes(attr_fqn, (*aitr)->attributes, inventory);
                break;
            }
            default:
                //noop;
                break;
        }
        if(ima_d4_attr){
            inventory.emplace_back(AttrType_to_String((*aitr)->type) + " " + attr_fqn);
        }
        has_d4_attr |= ima_d4_attr;
    }
    return has_d4_attr;
}

/**
 * @brief convenience function for the response limit test.
 * The DDS stores the response size limit in Bytes even though the context
 * param uses KB. The DMR uses KB throughout.
 * @param dds
 */
void throw_if_dap2_response_too_big(DDS *dds)
{
    if (dds->too_big()) {
#if 0
        stringstream msg;
        msg << "The Request for " << request_size / 1024 << " kilobytes is too large; ";
        msg << "requests on this server are limited to "
            + long_to_string(dds->get_response_limit() /1024) + "KB.";
        throw Error(msg.str());
#endif
        stringstream msg;
        msg << "The submitted DAP2 request will generate a " << dds->get_request_size_kb(true);
        msg <<  " kilobyte response, which is too large. ";
        msg << "The maximum response size for this server is limited to " << dds->get_response_limit_kb();
        msg << " kilobytes.";
        throw BESSyntaxUserError(msg.str(),__FILE__,__LINE__);
    }
}

void throw_if_dap4_response_too_big(DMR &dmr)
{
    if (dmr.too_big()) {
        stringstream msg;
        msg << "The submitted DAP4 request will generate a " << dmr.request_size_kb(true);
        msg <<  " kilobyte response, which is too large. ";
        msg << "The maximum response size for this server is limited to " << dmr.response_limit_kb();
        msg << " kilobytes.";
        throw BESSyntaxUserError(msg.str(),__FILE__,__LINE__);
    }
}

}   // namespace dap_utils
