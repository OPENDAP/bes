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
#include <unordered_map>

#include <libdap/DDS.h>
#include <libdap/DMR.h>
#include <libdap/D4Group.h>
#include <libdap/Vector.h>
#include <libdap/Array.h>
#include <libdap/Constructor.h>

#include "BESUtil.h"
#include "BESLog.h"
#include "BESDebug.h"
#include "BESSyntaxUserError.h"
#include "DapUtils.h"

#define MODULE "dap_utils"
#define prolog std::string("dap_utils::").append(__func__).append("() - ")

using namespace libdap;




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

std::string get_dap_array_dims_str(libdap::Array &a, bool constrained=false){
    stringstream my_dims;
    auto dim_itr = a.dim_begin();
    auto end_dim = a.dim_end();
    while(dim_itr != end_dim){
        auto dim = *dim_itr;
        auto d4dim = dim.dim;
        uint64_t elements = 0;
        if(d4dim){
            if(d4dim->constrained()){
                elements = (d4dim->c_stop() - d4dim->c_start());
                if(d4dim->c_stride()){
                    double num = (elements+0.0)/d4dim->c_stride();
                    elements  = num;
                }
            } else{
                elements = d4dim->size();
            }
        }
        else {
            double num = dim.stop - (dim.start+0.0)/dim.stride;
            elements  = num;
            if(!elements) elements = 1;
        }
        my_dims << "[" << elements << "]";
        dim_itr++;
    }
    return my_dims.str();
}

std::string get_dap_decl(libdap::BaseType *var, bool constrained=false) {

    stringstream ss;
    if(var->is_vector_type()){
        auto myArray = dynamic_cast<libdap::Array *>(var);
        if(myArray) {
            ss << myArray->prototype()->type_name() << " " << var->name();
            ss << get_dap_array_dims_str(*myArray, true);
        }
        else {
            auto myVec = dynamic_cast<libdap::Vector *>(var);
            if(myVec){
                ss << myVec->prototype()->type_name() << " " << var->name();
                ss << "[" << myVec->length() << "]";
            }
        }
    }
    else {
        ss << var->type_name() << var->name();
    }
    return ss.str();
}

/**
 *
 * @param constrctr
 * @param max_size
 * @param too_big
 */
void find_too_big_vars( libdap::Constructor *constrctr, const uint64_t &max_size, std::unordered_map<std::string,int64_t> &too_big)
{

    auto varitr=constrctr->var_begin();
    for(; varitr != constrctr->var_end(); varitr++){
        auto var = *varitr;
        BESDEBUG(MODULE, prolog << "BEGIN " << var->type_name() << "(FQN: " << var->FQN() << ")" << endl);
        if(var->is_constructor_type()){
            auto some_constrctr = dynamic_cast<libdap::Constructor *>(var);
            if(some_constrctr){
                find_too_big_vars(some_constrctr, max_size,too_big);
            }
        }
        else {
            auto vsize = var->width_ll(true);
            BESDEBUG(MODULE, prolog << get_dap_decl(var,true) << " (size: " << vsize  << ")" << endl);
            if (vsize > max_size) {
                BESDEBUG(MODULE, prolog << var->FQN() << " is bigger than max_size: " << max_size << endl);
                too_big.emplace(pair<string, uint64_t>(get_dap_decl(var,true), vsize));
            }
        }
        BESDEBUG(MODULE, prolog << "END " << var->type_name() << "(FQN: " << var->FQN() << ")" << endl);
    }
}


/**
 *
 * @param grp
 * @param max_size
 * @param too_big
 */
void find_too_big_vars( libdap::D4Group *grp, const uint64_t &max_size, std::unordered_map<std::string,int64_t> &too_big)
{
    auto cnstrctr = dynamic_cast<libdap::Constructor *>(grp);
    if (cnstrctr) {
        find_too_big_vars(cnstrctr, max_size, too_big);
    }
    for (auto child_grp: grp->groups()) {
        BESDEBUG(MODULE, prolog << "BEGIN " << grp->type_name() << "(" << grp->FQN() << ")" << endl);
        find_too_big_vars(child_grp, max_size, too_big);
        BESDEBUG(MODULE, prolog << "END " << grp->type_name() << "(" << grp->FQN() << ")" << endl);
    }
}

/**
 *
 * @param dmr
 * @param max_size
 * @param too_big
 */
void find_too_big_vars( libdap::DMR &dmr, const uint64_t &max_size, std::unordered_map<std::string,int64_t> &too_big)
{
    find_too_big_vars(dmr.root(), max_size,too_big);
}

}   // namespace dap_utils
