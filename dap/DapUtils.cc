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

#include "TheBESKeys.h"
#include "BESContextManager.h"
#include "BESUtil.h"
#include "BESLog.h"
#include "BESDebug.h"
#include "BESSyntaxUserError.h"
#include "DapUtils.h"

// Because prolog evaluates to a string it cannot be part of a constexpr, so we continue with the macro version.
#define prolog std::string("dap_utils::").append(__func__).append("() - ")

using namespace libdap;

constexpr auto BES_KEYS_MAX_RESPONSE_SIZE_KEY = "BES.MaxResponseSize.bytes";
constexpr auto BES_KEYS_MAX_VAR_SIZE_KEY = "BES.MaxVariableSize.bytes";
constexpr auto BES_CONTEXT_MAX_RESPONSE_SIZE_KEY = "max_response_size";
constexpr auto BES_CONTEXT_MAX_VAR_SIZE_KEY = "max_variable_size";

namespace dap_utils {

// We want MODULE and MODULE_VERBOSE to be in the namespace in order to isolate them from potential overlap between
// different bes/modules
constexpr auto MODULE = "dap_utils";
constexpr auto MODULE_VERBOSE = "dap_utils_verbose";

/**
 *
 * @param caller_id A string used to identify the source of the invocation.
 * @param response_size
 */
static void log_response_and_memory_size_helper(const std::string &caller_id, long response_size) {
    auto mem_size = BESUtil::get_current_memory_usage();    // size in KB or 0. jhrg 4/6/22
    if (mem_size) {
        INFO_LOG(caller_id + "response size: " << response_size << "KB|&|memory used by process: " << mem_size << "KB" << endl);
    }
    else {
        INFO_LOG(caller_id + "response size: " << response_size << "KB" << endl);
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
void log_response_and_memory_size(const std::string &caller_id, DDS *const *dds)
{
    auto response_size = (long)(*dds)->get_request_size_kb(true);
    log_response_and_memory_size_helper(caller_id, response_size);
}

/**
 * Log information about memory used by this request.
 *
 * Use the given DMR to log information about the request. As a bonus, log the
 * RSS for this process.
 *
 * @param dmr Use this DMR to get the size of the request.
 */
void log_response_and_memory_size(const std::string &caller_id, /*const*/ DMR &dmr)
{
    // The request_size_kb() method is not marked const. Fix. jhrg 4/6/22
    auto response_size = (long)dmr.request_size_kb(true);
    log_response_and_memory_size_helper(caller_id, response_size);
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
 * This assumes that integer division truncates the fractiona part of the result.
 * @param d4dim The dimension to examine
 * @return The number elements marked for transmission.
 */
uint64_t count_requested_elements(const D4Dimension *d4dim){
    uint64_t elements = 0;
    if(d4dim->constrained()){
        elements = (d4dim->c_stop() - d4dim->c_start());
        if(d4dim->c_stride()){
            elements =  elements / d4dim->c_stride();
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
 * This assumes that integer division truncates the fractiona part of the result.
 * @param d4dim The dimension to examine
 * @return The number elements marked for transmission.
 */
uint64_t count_requested_elements(const Array::dimension &dim){
    uint64_t elements;
    elements = (dim.stop - dim.start) / dim.stride;
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
 * @brief Returns the declaration of the passed variable as "TypeName VarName" and constrained Array dimensions, if any.
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
 * @param var The BaseType to evaluate
 * @param max_var_size Size threshold for the inclusion of variables in the inventory.
 * @param too_big An unordered_map fo variable descriptions and their constrained sizes.
 * @return The number of bytes the variable "var" will contribute to a response
 */
uint64_t crsaibv_process_variable(
        BaseType *var,
        const uint64_t &max_var_size,
        std::vector< pair<std::string,int64_t> > &too_big
){

    uint64_t response_size = 0;

    if (var->is_constructor_type()) {
        auto some_constrctr = dynamic_cast<libdap::Constructor *>(var);
        if (some_constrctr) {
            for(auto dap_var:some_constrctr->variables()) {
                response_size += crsaibv_process_variable(dap_var, max_var_size, too_big);
            }
        } else {
            BESDEBUG(MODULE,
                     prolog << "ERROR Failed to cast BaseType pointer var to Constructor type." << endl);
        }
    }
    else {
        if (var->send_p()) {
            // width_ll() returns the number of bytes needed to hold the data
            uint64_t vsize = var->width_ll(true);
            response_size += vsize;

            BESDEBUG(MODULE_VERBOSE, prolog << "  " << get_dap_decl(var) << "(" << vsize << " bytes)" << endl);
            if (max_var_size > 0 && vsize > max_var_size) {
                too_big.emplace_back(pair<string, uint64_t>(get_dap_decl(var), vsize));
                BESDEBUG(MODULE,
                         prolog << get_dap_decl(var) << "(" << vsize << " bytes) is bigger than the max_var_size of "
                                << max_var_size << " bytes. too_big.size(): " << too_big.size() << endl);
            }
        }
    }
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
uint64_t compute_response_size_and_inv_big_vars(
        libdap::D4Group *grp,
        const uint64_t &max_var_size,
        std::vector< pair<std::string,int64_t> > &too_big)
{
    uint64_t response_size = 0;
    // Process Child Variables.
    for(auto dap_var:grp->variables()){
        response_size += crsaibv_process_variable(dap_var,max_var_size, too_big);
    }
#if 0
    auto cnstrctr = static_cast<libdap::Constructor *>(grp);
    // Since Group is a child of Constructor we can use the Constructor
    // version of this method to handle the variables in the Group. Nifty, Right?
    response_size += compute_response_size_and_inv_big_vars(cnstrctr, max_var_size, too_big);
#endif

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
uint64_t compute_response_size_and_inv_big_vars(
        libdap::DMR &dmr,
        const uint64_t &max_var_size,
        std::vector< pair<std::string,int64_t> > &too_big)
{
    // Consider if something other than an unordered_map, or even map, we might consider using vector like this:
    //  std::vector<std::pair<std::string,int64_t>> foo;
    //  Is that better? It preserves to order of addition and we don't ever need any of the map api features
    //  in the usage of the inventory. In fact we might consider just making this a vector<string> and building
    //  each string as "variable decl[dim0]...[dimN] (size: ##### bytes)" or even skipping the vector in favor
    //  of a stringstream to which we just keep adding more stuff:
    //  stringstream too_big_inventory;
    //  too_big_inventory << "variable decl[dim0]...[dimN] (size: ##### bytes)" << endl;

    return compute_response_size_and_inv_big_vars(dmr.root(), max_var_size,too_big);
}



/**
 * @brief Determines the values of max_var_size and max_response_size by checking the command context and TheBESKeys
 * @param max_var_size The maximum allowable size, in bytes, for a constrained variable.
 * @param max_response_size The maximum allowable total response size, in bytes.
 */
void get_max_sizes_bytes(uint64_t &max_var_size_bytes, uint64_t &max_response_size_bytes){

    bool found = false;
    max_var_size_bytes = BESContextManager::TheManager()->get_context_uint64(BES_CONTEXT_MAX_VAR_SIZE_KEY, found);
    if (!found) {
        // It wasn't in the BESContextManager, so we check TheBESKeys. Note that we pass the default value of 0
        // which means no maximum size will be enforced.
        max_var_size_bytes = TheBESKeys::TheKeys()->read_uint64_key(BES_KEYS_MAX_VAR_SIZE_KEY, 0);
    }

    found = false;
    max_response_size_bytes = BESContextManager::TheManager()->get_context_uint64(BES_CONTEXT_MAX_RESPONSE_SIZE_KEY,found);
    if(!found){
        // It wasn't in the BESContextManager, so we check TheBESKeys. Note that we pass the default value of 0
        // which means no maximum size will be enforced.
        max_response_size_bytes = TheBESKeys::TheKeys()->read_uint64_key(BES_KEYS_MAX_RESPONSE_SIZE_KEY, 0);
    }
}

std::string too_big_opener(uint64_t max_response_size_bytes, uint64_t max_var_size_bytes){
    stringstream msg;
    msg << "\nYou asked for too much! \n";

    msg << "    Maximum allowed response size: ";
    if(max_response_size_bytes == 0){
        msg << "unlimited\n";
    }
    else {
        msg << max_response_size_bytes << " bytes.\n";
    }

    msg << "    Maximum allowed variable size: ";
    if(max_var_size_bytes == 0){
        msg << "unlimited\n";
    }
    else {
        msg << max_var_size_bytes << " bytes.\n";
    }

    return msg.str();
}


void throw_if_too_big(libdap::DMR &dmr, const string &file, const unsigned int line){
    stringstream msg;
    uint64_t max_var_size_bytes=0;
    uint64_t max_response_size_bytes=0;
    std::vector< pair<std::string,int64_t> > too_big_vars;

    get_max_sizes_bytes(max_var_size_bytes, max_response_size_bytes);

    auto response_size_bytes = compute_response_size_and_inv_big_vars(dmr, max_var_size_bytes, too_big_vars);

    // Is the whole thing too big? If so flag and make message.
    bool response_too_big = max_response_size_bytes>0 && response_size_bytes > max_response_size_bytes;
    if(response_too_big){
        msg << too_big_opener(max_response_size_bytes, max_var_size_bytes);
        msg << "The submitted DAP4 request will generate a " << response_size_bytes;
        msg <<  " byte response, which is too large.\n";
    }

    if(!too_big_vars.empty()){
        if(response_too_big){
            msg << "In addition to the overall response being to large for the service to produce,\n";
            msg << "the request references the following variables ";
        }
        else {
            msg << too_big_opener(max_response_size_bytes, max_var_size_bytes);
            msg << "The following is a list of variables, identified in the request,\n";
        }
        msg << "that are individually Too Large for the service to process.\n";
        msg << "\nOversized Variables: \n";
        for(const auto& var_entry:too_big_vars){
            msg << "    " << var_entry.first << " (" << var_entry.second << " bytes)\n";
        }
        msg << "\n";
        response_too_big = true;
    }

    if(response_too_big) {
        msg << "You can resolve these issues by requesting less.\n";
        msg << " - Consider asking for fewer variables (do you need them all?)\n";
        msg << " - If individual variables are too large you can also subset\n";
        msg << "   them using an index based array subset expression \n";
        msg << "   to request a smaller area or to decimate the variable.\n";
        msg << "You can find detailed information about DAP4 variable sub-setting here:\n";
        msg << "https://github.com/OPENDAP/dap4-specification/blob/main/";
        msg << "01_data-model-and-serialized-rep.md#8-constraints\n";
        BESDEBUG(MODULE,msg.str() + "\n");
        throw BESSyntaxUserError(msg.str(), file, line);
    }
}



}   // namespace dap_utils
