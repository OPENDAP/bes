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
#include <map>

#include <libdap/DDS.h>
#include <libdap/DMR.h>
#include <libdap/D4Group.h>

#include "BESUtil.h"
#include "BESInternalError.h"
#include "BESLog.h"
#include "BESSyntaxUserError.h"
#include "DapUtils.h"

using namespace libdap;

#define prolog std::string("DapUtils::").append(__func__).append("() - ")

namespace dap_utils {

/**
 *
 * @param req_size
 */
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
void log_request_and_memory_size(DDS *const *dds)
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
void log_request_and_memory_size(/*const*/ DMR &dmr)
{
    // The request_size_kb() method is not marked const. Fix. jhrg 4/6/22
    auto req_size = (long)dmr.request_size_kb(true);
    log_request_and_memory_size_helper(req_size);
}



/**
 * @brief Throws an exception if the projected variables and or attributes of the DDS have dap4 types.
 * @param dds The DDS to examine.
 * @param file The file of the calling function/method
 * @param line The line of the calling function/method.
 */
void throw_for_dap4_typed_vars_or_attrs(DDS *dds, const std::string &file, unsigned int line)
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
        throw BESSyntaxUserError(msg.str(), file, line);
    }
}

/**
 * @brief Throws an exception if the projected variables and or attributes of the DAS have dap4 types.
 * @param dds The DDS to examine.
 * @param file The file of the calling function/method
 * @param line The line of the calling function/method.
 */
void throw_for_dap4_typed_attrs(DAS *das, const std::string &file, unsigned int line)
{
    vector<string> inventory;
    if(das->get_top_level_attributes()->has_dap4_types("/",inventory)){
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
        throw BESSyntaxUserError(msg.str(), file, line);
    }
}

/**
 * @brief convenience function for the response limit test.
 * The DDS stores the response size limit in Bytes even though the context
 * param uses KB. The DMR uses KB throughout.
 * @param dds
 */
void throw_if_dap2_response_too_big(DDS *dds, const std::string &file, unsigned int line)
{
    if (dds->too_big()) {
        stringstream msg;
        msg << "The submitted DAP2 request will generate a " << dds->get_request_size_kb(true);
        msg <<  " kilobyte response, which is too large. ";
        msg << "The maximum response size for this server is limited to " << dds->get_response_limit_kb();
        msg << " kilobytes.";
        throw BESSyntaxUserError(msg.str(),file, line);
    }
}

void throw_if_dap4_response_too_big(DMR &dmr, const std::string &file, unsigned int line)
{
    if (dmr.too_big()) {
        stringstream msg;
        msg << "The submitted DAP4 request will generate a " << dmr.request_size_kb(true);
        msg <<  " kilobyte response, which is too large. ";
        msg << "The maximum response size for this server is limited to " << dmr.response_limit_kb();
        msg << " kilobytes.";
        throw BESSyntaxUserError(msg.str(), file, line);
    }
}


u_int64_t get_response_size_and_vars_too_big(Constructor *ctr, u_int64_t variable_max, map<string,u_int64_t> &big_bad_vars)
{
    u_int64_t var_size;
    u_int64_t response_size = 0;

    for(auto btp : ctr->variables()){
        if(btp->is_constructor_type()){
            response_size += get_response_size_and_vars_too_big(dynamic_cast<Constructor *>(btp), variable_max, big_bad_vars);
        }
        else {
            var_size = btp->width_ll(true);
            response_size += var_size;
            if(var_size > variable_max){
                big_bad_vars.emplace(btp->FQN(),var_size);
            }
        }
    }
    return response_size;
}

void new_throw_if_dap4_response_too_big(DMR &dmr, u_int64_t variable_max, const std::string &file, unsigned int line)
{
    u_int64_t response_size;
    map<string,u_int64_t> too_big_vars;

    response_size = get_response_size_and_vars_too_big(dmr.root(), variable_max, too_big_vars);

    stringstream msg;

    if(response_size > dmr.response_limit_kb()){
        msg << "The submitted DAP4 request will generate a " << response_size <<" kilobyte response, which is too large. ";
        msg << "The maximum response size for this server is limited to " << dmr.response_limit_kb() << " kilobytes.";
    }

    if(!too_big_vars.empty()){
        msg << "The following requested variables are too large for us to process. We recommend that ";
        msg << "you subset the variables and make multiple requests for the data. You may be able to retrieve ";
        msg << "multiple subset variables in a single request as long as the response size is less than ";
        msg << dmr.response_limit_kb() << " kilobytes.";
        msg << "The maximum response size for a variable on this server is limited to " << variable_max << " kilobytes.";
    }

    if(!msg.str().empty())
        throw BESSyntaxUserError(msg.str(), file, line);
}

}   // namespace dap_utils
