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
#define MODULE_VERBOSE "dap_utils_verbose"
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

/**
 * @brief - determines the number of requested elements for the D4Dimension d4dim.
 * @param d4dim The dimension to examine
 * @return The number elements marked for transmission.
 */
uint64_t count_requested_elements(const D4Dimension *d4dim){
    uint64_t elements = 0;
    if(d4dim->constrained()){
        elements = (d4dim->c_stop() - d4dim->c_start());
        if(d4dim->c_stride()){
            double num = (static_cast<double>(elements)) / d4dim->c_stride();
            elements  = floor(num);
        }
    } else{
        elements = d4dim->size();
    }
    if(!elements)
        elements = 1;

    return elements;
}

/**
 * @brief - determines the number of requested elements for the Array::dimension dim.
 * @param d4dim The dimension to examine
 * @return The number elements marked for transmission.
 */
uint64_t count_requested_elements(const Array::dimension &dim){
    uint64_t elements = 0;
    double num = (static_cast<double>(dim.stop) - static_cast<double>(dim.start)) / static_cast<double>(dim.stride);
    elements  = floor(num);
    if(!elements)
        elements = 1;

    return elements;
}

/**
 * @brief Returns a string with the square bracket notation for the arrays dimension sizes as constrained.
 * @param a The Array to evaluate.
 * @return A string with the square brackety business.
 */
std::string get_dap_array_dims_str(libdap::Array &a){
    stringstream my_dims;
    auto dim_itr = a.dim_begin();
    auto end_dim = a.dim_end();
    while(dim_itr != end_dim){
        auto dim = *dim_itr;
        auto d4dim = dim.dim;
        uint64_t elements = 0;
        if(d4dim){
            elements = count_requested_elements(d4dim);
        }
        else {
            elements  = count_requested_elements(dim);
        }
        my_dims << "[" << elements << "]";
        dim_itr++;
    }
    return my_dims.str();
}

/**
 * @brief Returns the declaration of the passed variable as "TypeName VarName" followed by a the [] expression of the constrained dimension sizes is the variable is an Array.
 * @param var The variable to evaluate
 * @return The declaration string.
 */
std::string get_dap_decl(libdap::BaseType *var) {

    stringstream ss;
    if(var->is_vector_type()){
        auto myArray = dynamic_cast<libdap::Array *>(var);
        if(myArray) {
            ss << myArray->prototype()->type_name() << " " << var->FQN();
            ss << get_dap_array_dims_str(*myArray);
        }
        else {
            auto myVec = dynamic_cast<libdap::Vector *>(var);
            if(myVec){
                ss << myVec->prototype()->type_name() << " " << var->FQN();
                ss << "[" << myVec->length() << "]";
            }
        }
    }
    else {
        ss << var->type_name() << var->FQN();
    }
    return ss.str();
}


/**
 * @brief Determines the number of bytes that var will contribute to the response and adds var to the inventory of too_big variables.
 *
 * Assumption: The provided libdap::Constructor has had the constraint expressions applied.
 *
 * This code also handles the libdap::D4Group instances as they children of libdap::Constructor.
 *
 * @param constrctr The Constructor to evaluate
 * @param max_var_size Size threshold for the inclusion of variables in the inventory.
 * @param too_big An unordered_map fo variable descriptions and their constrained sizes.
 * @return The number of bytes the variable var will conribute to the response
 */
uint64_t process_variable(BaseType *var, const uint64_t &max_var_size, std::unordered_map<std::string,int64_t> &too_big){
    uint64_t response_size = 0;
    if (var->send_p()) {
        uint64_t vsize = var->width_ll(true);
        response_size += vsize;

        string vdecl = get_dap_decl(var);
        BESDEBUG(MODULE_VERBOSE, prolog << "  " << vdecl << "(" << vsize << " bytes)" << endl);
        if (vsize > max_var_size) {
            too_big.emplace(pair<string, uint64_t>(vdecl, vsize));
            BESDEBUG(MODULE,
                     prolog << vdecl << "(" << vsize << " bytes) is bigger than the max_var_size of "
                            << max_var_size << " bytes. too_big.size(): " << too_big.size() << endl);
        }
    }
    return response_size;
}




/**
 * @brief Assesses the provided libdap::Constructor to identify a set of variables whose size is larger than the provided max_var_size, returns total response size for the Constructor.
 *
 * Assumption: The provided libdap::Constructor has had the constraint expressions applied.
 *
 * This code also handles the libdap::D4Group instances as they children of libdap::Constructor.
 *
 * @param constrctr The Constructor to evaluate
 * @param max_var_size Size threshold for the inclusion of variables in the inventory.
 * @param too_big An unordered_map fo variable descriptions and their constrained sizes.
 */
uint64_t compute_response_size_and_inv_big_vars(
        const libdap::Constructor *constrctr,
        const uint64_t &max_var_size,
        std::unordered_map<std::string,int64_t> &too_big)
{

    BESDEBUG(MODULE_VERBOSE, prolog << "BEGIN " << constrctr->type_name() << "(FQN: " << constrctr->FQN() << ")" << endl);

    uint64_t response_size = 0;
    for(auto var: constrctr->variables()){
        BESDEBUG(MODULE_VERBOSE, prolog << "BEGIN " << var->type_name() << "(FQN: " << var->FQN() << ")" << endl);
        if(var->is_constructor_type()){
            auto some_constrctr = dynamic_cast<libdap::Constructor *>(var);
            if(some_constrctr){
                response_size += compute_response_size_and_inv_big_vars(some_constrctr, max_var_size,too_big);
            }
            else {
                BESDEBUG(MODULE, prolog << "ERROR Failed to cast BaseType pointer var to Constructor type." << endl);
            }
        }
        else {
            response_size += process_variable(var,  max_var_size, too_big);
#if 0
            if (var->send_p()) {

                uint64_t vsize = var->width_ll(true);
                response_size += vsize;

                string vdecl = get_dap_decl(var);
                BESDEBUG(MODULE_VERBOSE, prolog << "  " << vdecl << "(" << vsize << " bytes)" << endl);
                if (vsize > max_var_size) {
                    too_big.emplace(pair<string, uint64_t>(vdecl, vsize));
                    BESDEBUG(MODULE,
                             prolog << vdecl << "(" << vsize << " bytes) is bigger than the max_var_size of "
                                    << max_var_size << " bytes. too_big.size(): " << too_big.size() << endl);
                }
            }
#endif
        }
        BESDEBUG(MODULE_VERBOSE, prolog << "END " << var->type_name() << "(FQN: " << var->FQN() << ")" << endl);
    }
    BESDEBUG(MODULE_VERBOSE, prolog << "END " << constrctr->type_name() << "(FQN: " << constrctr->FQN() << ")"
        << "response_size: " << response_size << endl);
    return response_size;
}


/**
 * @brief Assesses the provided libdap::Group to identify a set of variables whose size is larger than the provided max_var_size, returns total response size for this Group.
 *
 * Assumption: The provided libdap::Group has had the constraint expressions applied.
 *
 * @param grp The Group to evaluate.
 * @param max_var_size Size threshold for the inclusion of variables in the inventory.
 * @param too_big An unordered_map fo variable descriptions and their constrained sizes.
 */
uint64_t compute_response_size_and_inv_big_vars( libdap::D4Group *grp, const uint64_t &max_var_size, std::unordered_map<std::string,int64_t> &too_big)
{
    uint64_t response_size = 0;
    auto cnstrctr = dynamic_cast<libdap::Constructor *>(grp);
    if (cnstrctr) {
        // This is basically always going to be the result since libdap::D4Group is a child of libdap::Constructor,
        // And importantly for this code the actual size computation is handle in thie the follwing call.
        response_size += compute_response_size_and_inv_big_vars(cnstrctr, max_var_size, too_big);
    }

    // Process child groups.
    for (auto child_grp: grp->groups()) {
        BESDEBUG(MODULE_VERBOSE, prolog << "BEGIN " << grp->type_name() << "(" << child_grp->FQN() << ")" << endl);
        response_size += compute_response_size_and_inv_big_vars(child_grp, max_var_size, too_big);
        BESDEBUG(MODULE_VERBOSE, prolog << "END " << grp->type_name() << "(" << child_grp->FQN() << ")" << endl);
    }
    return response_size;
}

/**
 * @brief Assesses the provided DMR to identify a set of variables whose size is larger than the provided max_var_size, returns total response size.
 *
 * ; Assumption
 * : The provided DMR has had the constraint expressions applied - as in parsing some ce, or calling
 * : dmr.root()->set_send_p(true) to mark everything.
 *
 * @param dmr The DMR to evaluate.
 * @param max_var_size Size threshold for the inclusion of variables in the inventory.
 * @param too_big An unordered_map fo variable descriptions and their constrained sizes.
 */
uint64_t compute_response_size_and_inv_big_vars(libdap::DMR &dmr, const uint64_t &max_var_size, std::unordered_map<std::string,int64_t> &too_big)
{
    // TODO: Rather than an unordered_map, or even map, we might consider using vector like this:
    //  std::vector<std::pair<std::string,int64_t>> foo;
    //  Is that better? It preserves to order of addition and we don't ever need any of the map api features
    //  in the usage of the inventory. In fact we might consider just making this a vector<string> and building
    //  each string as "variable decl[dim0]...[dimN] (size: ##### bytes)" or even skipping the vector in favor
    //  of a stringstream to which we just keep adding more stuff:
    //  stringstream too_big_inventory;
    //  too_big_inventory << "variable decl[dim0]...[dimN] (size: ##### bytes)" << endl;

    return compute_response_size_and_inv_big_vars(dmr.root(), max_var_size,too_big);
}

}   // namespace dap_utils
