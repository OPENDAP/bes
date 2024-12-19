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
#include <cmath>

#include <libdap/DDS.h>
#include <libdap/DMR.h>
#include <libdap/D4Group.h>
#include <libdap/Vector.h>
#include <libdap/Array.h>
#include <libdap/Constructor.h>
#include <libdap/XMLWriter.h>

#include "TheBESKeys.h"
#include "BESContextManager.h"
#include "BESUtil.h"
#include "BESLog.h"
#include "BESDebug.h"
#include "BESStopWatch.h"
#include "BESSyntaxUserError.h"
#include "DapUtils.h"

// Because prolog evaluates to a string it cannot be part of a constexpr, so we continue with the macro version.
#define prolog std::string("dap_utils::").append(__func__).append("() - ")

using namespace libdap;

namespace dap_utils {

constexpr auto BES_KEYS_MAX_RESPONSE_SIZE_KEY = "BES.MaxResponseSize.bytes";
constexpr auto BES_KEYS_MAX_VAR_SIZE_KEY = "BES.MaxVariableSize.bytes";
constexpr auto BES_CONTEXT_MAX_RESPONSE_SIZE_KEY = "max_response_size";
constexpr auto BES_CONTEXT_MAX_VAR_SIZE_KEY = "max_variable_size";
constexpr uint64_t twoGB = 2147483648;
constexpr uint64_t fourGB = 4294967296;

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
        INFO_LOG(caller_id + "response size: " + std::to_string(response_size) + "KB"+BESLog::mark+"memory used by process: " +
            std::to_string(mem_size) + "KB");
    }
    else {
        INFO_LOG(caller_id + "response size: " + std::to_string(response_size) + "KB");
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
void log_response_and_memory_size(const std::string &caller_id, DMR &dmr)
{
    // The request_size_kb() method is not marked const. Fix. jhrg 4/6/22
    auto response_size = (long)dmr.request_size_kb(true);
    log_response_and_memory_size_helper(caller_id, response_size);
}

/**
 * Log information about memory used by this request.
 *
 * Use the given DDS to log information about the request. As a bonus, log the
 * RSS for this process.
 *
 * @param dds Use this DDS to get the size of the request.
 */
void log_response_and_memory_size(const std::string &caller_id, libdap::XMLWriter &dmrpp_writer)
{
    auto response_size = (long)dmrpp_writer.get_doc_size() / 1000;
    log_response_and_memory_size_helper(caller_id, response_size);
}

/**
 * @brief Helper function that coalesces the message production for DAP4 type in DAP2 land errors.
 * @param inventory The inventory of problem objects.
 * @param file The file of the calling method (outside of this evaluation activity)
 * @param line The line in file from which this call tree originated.
 */
std::string mk_model_incompatibility_message(const std::vector<std::string> &inventory){
    stringstream msg;
    msg << endl;
    msg << "ERROR: Your have asked this service to utilize the DAP2 data model\n";
    msg << "to process your request. Unfortunately the requested dataset contains\n";
    msg << "data types that cannot be represented in DAP2.\n ";
    msg << "\n";
    msg << "There are " << inventory.size() << " incompatible variables and/or attributes referenced \n";
    msg  << "in your request.\n";
    msg << "Incompatible variables: \n";
    msg << "\n";
    for(const auto &entry: inventory){ msg << "    " << entry << "\n"; }
    msg << "\n";
    msg << "You may resolve these issues by asking the service to use\n";
    msg <<  "the DAP4 data model instead of the DAP2 model.\n";
    msg << "\n";
    msg << " - NetCDF If you wish to receive your response encoded as a\n";
    msg << "   netcdf file please note that netcdf-3 has similar representational\n";
    msg << "   constraints as DAP2, while netcdf-4 does not. In order to request\n";
    msg << "   a DAP4 model nectdf-4 response, change your request URL from \n";
    msg << "   dataset_url.nc to dataset_url.dap.nc4\n";
    msg << "\n";
    msg << " - DAP Clients If you are using a specific DAP client like pyDAP or\n";
    msg << "   Panoply you may be able to signal the tool to use DAP4 by changing\n";
    msg << "   the protocol of the dataset_url from https:// to dap4:// \n";
    msg << "\n";
    msg << " - If you are using the service's Data Request Form for your dataset\n";
    msg << "   you can find the DAP4 version by changing form_url.html to form_url.dmr.html\n";
    msg << "\n";
    return msg.str();
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
        string msg = mk_model_incompatibility_message(inventory);
        throw BESSyntaxUserError(msg, file, line);
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
        string msg = mk_model_incompatibility_message(inventory);
        throw BESSyntaxUserError(msg, file, line);
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
 * @brief Returns a string with the square bracket notation for the arrays showing the constraint as start:stride:stop
 * @return A string with [start:stride:stop] for each dimension.
 */
std::string get_dap_array_dims_str(libdap::Array &a){
    stringstream my_dims;
    for (auto dim_iter = a.dim_begin(), end_iter = a.dim_end(); dim_iter != end_iter; ++dim_iter) {        stringstream ce;
        const auto &dim = *dim_iter;
        ce << dim.start << ":";
        if(dim.stride != 1){
            ce << dim.stride << ":";
        }
        ce  << dim.stop;
        my_dims << "[" << ce.str() << "]";
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
 * Forward declaration
 */
uint64_t crsaibv_process_ctor(const libdap::Constructor *ctor,
                               const uint64_t max_var_size,
                              std::vector<std::string> &too_big );

/**
 * @brief Determines the number of bytes that var will contribute to the response and adds var to the inventory of too_big variables.
 *
 * Assumption: The provided libdap::BaseType has had the constraint expressions applied.
 *
 * @param var The BaseType to evaluate
 * @param max_var_size Size threshold for the inclusion of variables in the inventory.
 * @param too_big An unordered_map fo variable descriptions and their constrained sizes.
 * @return The number of bytes the variable "var" will contribute to a response
 */
uint64_t crsaibv_process_variable(
        BaseType *var,
        const uint64_t max_var_size,
        std::vector<std::string> &too_big
){

    uint64_t response_size = 0;

    if(var->send_p()) {
        if (var->is_constructor_type()) {
            response_size += crsaibv_process_ctor(dynamic_cast<libdap::Constructor *>(var), max_var_size, too_big);
        }
        else {
            // width_ll() returns the number of bytes needed to hold the data
            uint64_t vsize = var->width_ll(true);
            response_size += vsize;

            BESDEBUG(MODULE_VERBOSE, prolog << "  " << get_dap_decl(var) << "(" << vsize << " bytes)" << endl);
            if ( (max_var_size > 0) && (vsize > max_var_size) ) {
                string entry = get_dap_decl(var) + " (" + to_string(vsize) + " bytes)";
                too_big.emplace_back(entry);
                BESDEBUG(MODULE,
                         prolog << get_dap_decl(var) << "(" << vsize
                                << " bytes) is bigger than the max_var_size of "
                                << max_var_size << " bytes. too_big.size(): " << too_big.size() << endl);
            }
        }
    }
    return response_size;
}

/**
 * @brief Assesses the provided libdap::Constructor to identify a set of variables whose size is larger than the provided max_var_size, returns total response size for this Group.
 *
 * Assumption: The provided libdap::Constructor has had the constraint expressions applied.
 *
 * @param ctor The libdap::Constructor type (Parent of Structure, Sequence, etc) to evaluate.
 * @param max_var_size Size threshold for the inclusion of variables in the inventory.
 * @param too_big An unordered_map fo variable descriptions and their constrained sizes.
 */
 uint64_t crsaibv_process_ctor(const libdap::Constructor *ctor,
                               const uint64_t max_var_size,
                               std::vector<std::string> &too_big
    ){
    uint64_t response_size = 0;
    if (ctor) {
        for (auto dap_var: ctor->variables()) {
            response_size += crsaibv_process_variable(dap_var, max_var_size, too_big);
        }
    }
    else {
        BESDEBUG(MODULE,
                 prolog << "ERROR Received a null pointer to Constructor. " <<
                 "It is likely that a dynamic_cast failed.." << endl);
    }
    return response_size;
}


/**
 * @brief Assesses the provided libdap::Group to identify a set of variables whose size is larger than the provided max_var_size, returns total response size for this Group.
 *
 * Assumption: The provided libdap::Group has had the constraint expressions applied.
 *
 * @param grp The libdap::Group to evaluate.
 * @param max_var_size Size threshold for the inclusion of variables in the inventory.
 * @param too_big An unordered_map fo variable descriptions and their constrained sizes.
 */
uint64_t compute_response_size_and_inv_big_vars(
        const libdap::D4Group *grp,
        const uint64_t max_var_size,
        std::vector<std::string> &too_big)
{
    BESDEBUG(MODULE_VERBOSE, prolog << "BEGIN " << grp->type_name() << " " << grp->FQN() << endl);

    uint64_t response_size = 0;
    // Process child variables.
    for(auto dap_var:grp->variables()){
        response_size += crsaibv_process_variable(dap_var, max_var_size, too_big);
    }

    // Process child groups.
    for (const auto child_grp: grp->groups()) {
        if (child_grp->send_p()) {
            response_size += compute_response_size_and_inv_big_vars(child_grp, max_var_size, too_big);
        }
        else {
            BESDEBUG(MODULE_VERBOSE, prolog << "SKIPPING: " << grp->type_name() <<
            " " << child_grp->FQN() << " (No child selected.)" << endl);
        }
    }
    BESDEBUG(MODULE_VERBOSE, prolog << "END " << grp->type_name() << " " << grp->FQN() << " ("
                                       "response_size: " << response_size << ", "<<
                                       "too_big_vars: " << too_big.size() << ")" << endl);
    return response_size;
}

/**
 * @brief Assesses the provided DMR to identify a set of variables whose size is larger than the provided max_var_size and return total response size.
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
        const uint64_t max_var_size,
        std::vector<std::string> &too_big)
{
#ifndef NDEBUG
    BESStopWatch sw;
    sw.start(prolog + "DMR");
#endif

    return compute_response_size_and_inv_big_vars(dmr.root(), max_var_size,too_big);
}

/**
 * @brief Assesses the provided DDS to identify a set of variables whose size is larger than the provided max_var_size and return total response size.
 *
 * ; Assumption
 * : The provided DDS has had the constraint expressions applied - as in parsing some ce.
 *
 * @param dmr The DDS to evaluate.
 * @param max_var_size Size threshold for the inclusion of variables in the inventory.
 * @param too_big An unordered_map fo variable descriptions and their constrained sizes.
 */
uint64_t compute_response_size_and_inv_big_vars(
        const libdap::DDS &dds,
        const uint64_t max_var_size,
        std::vector<std::string> &too_big)
{
#ifndef NDEBUG
    BESStopWatch sw;
    sw.start(prolog + "DDS");
#endif
    uint64_t response_size = 0;
    // Process child variables.
    for(auto dap_var:dds.variables()){
        response_size += crsaibv_process_variable(dap_var, max_var_size, too_big);
    }
    return response_size;
}



/**
 * @brief Determines the values of max_var_size and max_response_size by checking the TheBESKeys and the command context
 * Priority is given to the BES configuration.
 * A request that sets the BESContexts for these values can
 * only decrease the maximum sizes as set in the BES configuration.
 * @param max_var_size The maximum allowable size, in bytes, for a constrained variable.
 * @param max_response_size The maximum allowable total response size, in bytes.
 * @param is_dap2 If true then DAP2 size limitations will be enforced. default: false
 */
void get_max_sizes_bytes(uint64_t &max_response_size_bytes, uint64_t &max_var_size_bytes,  bool is_dap2)
{
#ifndef NDEBUG
    BESStopWatch sw;
    sw.start(prolog + (is_dap2?"DAP2":"DAP4"));
#endif

    // The BES configuration is help in TheBESKeys, so we read from there.
    uint64_t config_max_resp_size = TheBESKeys::TheKeys()->read_uint64_key(BES_KEYS_MAX_RESPONSE_SIZE_KEY, 0);
    BESDEBUG(MODULE, prolog << "config_max_resp_size: " << config_max_resp_size << "\n");
    max_response_size_bytes = config_max_resp_size; // This is the default state, the command can only make it smaller

    uint64_t cmd_context_max_resp_size;
    bool found;
    cmd_context_max_resp_size = BESContextManager::TheManager()->get_context_uint64(BES_CONTEXT_MAX_RESPONSE_SIZE_KEY, found);
    if (!found) {
        BESDEBUG(MODULE,
                 prolog << "Did not locate BESContext key: " << BES_CONTEXT_MAX_RESPONSE_SIZE_KEY << " SKIPPING."
                        << "\n");
    }
    else {
        BESDEBUG(MODULE, prolog << "cmd_context_max_resp_size: " << cmd_context_max_resp_size << "\n");
        // If the cmd_context_max_resp_size==0, then there's nothing to do because
        // we prioritize the bes configuration. If the config_max_resp_size=0 it's a no-op, and if config_max_resp_size
        // is some other value then it's not unlimited, and we're not letting the command context make these values
        // bigger than the one in the BES configuration, only smaller.
        if(cmd_context_max_resp_size != 0 &&  (cmd_context_max_resp_size < config_max_resp_size || config_max_resp_size == 0) ){
            // If the context value is effectively less than the config value, use the context value.
            max_response_size_bytes = cmd_context_max_resp_size;
        }
    }
    BESDEBUG(MODULE, prolog << "max_response_size_bytes: " << max_response_size_bytes << "\n");

    // The BES configuration is help in TheBESKeys, so we read from there.
    uint64_t config_max_var_size = TheBESKeys::TheKeys()->read_uint64_key(BES_KEYS_MAX_VAR_SIZE_KEY, 0);
    BESDEBUG(MODULE, prolog << "config_max_var_size: " << config_max_var_size << "\n");
    max_var_size_bytes = config_max_var_size;

    uint64_t cmd_context_max_var_size=0;
    found = false;
    cmd_context_max_var_size = BESContextManager::TheManager()->get_context_uint64(BES_CONTEXT_MAX_VAR_SIZE_KEY, found);
    if (!found) {
        max_var_size_bytes = config_max_var_size;
        BESDEBUG(MODULE, prolog << "Did not locate BESContext key: " << BES_CONTEXT_MAX_VAR_SIZE_KEY << " SKIPPING." << "\n");
    }
    else if( (cmd_context_max_var_size != 0) &&  (cmd_context_max_var_size < config_max_var_size || config_max_var_size == 0) ){
        // If the context value is effectively less than the config value, use the context value.
        max_var_size_bytes = cmd_context_max_var_size;
    }

    // Enforce DAP2 limits?
    if ( is_dap2){
        if (max_var_size_bytes == 0 || max_var_size_bytes > twoGB) {
            max_var_size_bytes = twoGB;
            BESDEBUG(MODULE, prolog << "Adjusted max_var_size_bytes to DAP2 limit.\n");
        }
        if (max_response_size_bytes == 0 || max_response_size_bytes > fourGB) {
            max_response_size_bytes = fourGB;
            BESDEBUG(MODULE, prolog << "Adjusted max_response_size_bytes to DAP2 limit.\n");
        }
    }
    BESDEBUG(MODULE, prolog << "max_var_size_bytes: " << max_var_size_bytes << "\n");
}

/**
 * Helper function that simply creates the error message prolog.
 * @param max_response_size_bytes
 * @param max_var_size_bytes
 * @return The error message prolog.
 */
std::string too_big_error_prolog(const uint64_t max_response_size_bytes, const uint64_t max_var_size_bytes){
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

/**
 * This is the main worker for checking the response boundaries and, if
 * the response is out of bounds, produces a context sensitive error message
 * that explains the iss and provides some broad suggestions about remediation.
 *
 * @param msg
 * @param max_response_size_bytes
 * @param response_size_bytes
 * @param max_var_size_bytes
 * @param too_big_vars
 * @param is_dap2
 * @return
 */
bool its_too_big(
        stringstream &msg,
        const uint64_t max_response_size_bytes,
        const uint64_t response_size_bytes,
        const uint64_t max_var_size_bytes,
        const std::vector<string> &too_big_vars,
        bool is_dap2=false
        ){

    BESDEBUG(MODULE, prolog << "max_response_size_bytes: " << max_response_size_bytes << "\n");
    BESDEBUG(MODULE, prolog << "max_var_size_bytes: " << max_var_size_bytes << "\n");
    BESDEBUG(MODULE, prolog << "response_size_bytes: " << response_size_bytes << "\n");
    BESDEBUG(MODULE, prolog << "too_big_vars.size(): " << too_big_vars.size() << "\n");

    // Is the whole thing too big? If so flag and start message.
    bool response_too_big = (max_response_size_bytes > 0) && (response_size_bytes > max_response_size_bytes);
    if(response_too_big){
        msg << too_big_error_prolog(max_response_size_bytes, max_var_size_bytes);
        msg << "The submitted DAP" << (is_dap2?"2":"4") << " request will generate a ";
        msg << response_size_bytes << " byte\n";
        msg << "response, which is larger than the maximum allowed response size.\n";
    }

    // Was one or more of the constrained variables too big?
    if(!too_big_vars.empty()){
        if(response_too_big){
            // Is the whole thing too big? Continue message.
//          msg <<"- Consider asking for fewer variables (do you need them all?)"
            msg << "\nIn addition to the overall response being too large for the\n";
            msg << "service to produce, the request references the following\n";
            msg << "variable(s) ";
        }
        else {
            // Start message
            msg << too_big_error_prolog(max_response_size_bytes, max_var_size_bytes);
            msg << "The following is a list of variable(s), identified\n";
            msg << "in the request, ";
        }
        // Add oversoze variable info.
        msg << "that are each too large for the service\n";
        msg << "to process.\n";
        msg << "\nOversized Variable(s): \n";
        for(const auto& var_entry:too_big_vars){
            msg << "    " << var_entry << "\n";
        }
        msg << "\n";
        response_too_big = true;
    }

    if(response_too_big) {
        //  Finish message
        msg << "You can resolve these issues by requesting less.\n";
        msg << " - Consider asking for fewer variables (do you need them all?)\n";
        msg << " - If individual variables are too large you can also subset\n";
        msg << "   them using an index based array subset expression \n";
        msg << "   to request a smaller area or to decimate the variable.\n";
        if(is_dap2){
            msg << "You can find detailed information about DAP2 variable sub-setting\n";
            msg << "expressions in section 4.4 of the DAP2 User Guide located here:\n";
            msg << "https://www.opendap.org/documentation/UserGuideComprehensive.pdf\n";
        }
        else {
            // It's a DAP4 thing...
            msg << "You can find detailed information about DAP4 variable sub-setting here:\n";
            msg << "https://github.com/OPENDAP/dap4-specification/blob/main/";
            msg << "01_data-model-and-serialized-rep.md#8-constraints\n";
        }
        return true;
    }
    return false;
}

/**
 * @brief Throws an exception if the DMR, as marked, would produce a larger than allowed response, or if a requested variable is too big.
 * The maximum size values are produced by the dap_utils::get_max_sizes_bytes() function.
 * @param dmr The DMR to which a ce has been applied, or that is otherwise marked for transmission.
 * @param file The file of the calling code.
 * @param line The line in the calling code.
 */
void throw_if_too_big(libdap::DMR &dmr, const string &file, const unsigned int line)
{
#ifndef NDEBUG
    BESStopWatch sw;
    sw.start(prolog + "DMR");
#endif

    uint64_t max_var_size_bytes=0;
    uint64_t max_response_size_bytes=0;
    std::vector<std::string> too_big_vars;

    get_max_sizes_bytes(max_response_size_bytes, max_var_size_bytes);
    BESDEBUG(MODULE, prolog << "max_var_size_bytes: " << max_var_size_bytes << "\n");
    BESDEBUG(MODULE, prolog << "max_response_size_bytes: " << max_response_size_bytes << "\n");

    auto response_size_bytes = compute_response_size_and_inv_big_vars(dmr, max_var_size_bytes, too_big_vars);
    BESDEBUG(MODULE, prolog << "response_size_bytes: " << response_size_bytes << "\n");
    BESDEBUG(MODULE, prolog << "too_big_vars: " << too_big_vars.size() << "\n");

    stringstream too_big_message;
    if(its_too_big(
            too_big_message,
            max_response_size_bytes,
            response_size_bytes,
            max_var_size_bytes,
            too_big_vars)) {
        BESDEBUG(MODULE, prolog << "It's TOO BIG:\n" << too_big_message.str() << "\n");
        throw BESSyntaxUserError(too_big_message.str(), file, line);
    }
}


/**
 * @brief Throws an exception if the DDS, as marked, would produce a larger than allowed response, or if a requested variable is too big.
 * The maximum size values are produced by the dap_utils::get_max_sizes_bytes() function.
 * @param dds The DDS to which a ce has been applied, or that is otherwise marked for transmission.
 * @param file The file of the calling code.
 * @param line The line in the calling code.
*/
void throw_if_too_big(const libdap::DDS &dds, const std::string &file, const unsigned int line)
{

#ifndef NDEBUG
    BESStopWatch sw;
    sw.start(prolog + "DDS");
#endif

    uint64_t max_var_size_bytes=0;
    uint64_t max_response_size_bytes=0;
    std::vector<std::string> too_big_vars;

    get_max_sizes_bytes(max_response_size_bytes, max_var_size_bytes, true);
    BESDEBUG(MODULE, prolog << "max_var_size_bytes: " << max_var_size_bytes << "\n");
    BESDEBUG(MODULE, prolog << "max_response_size_bytes: " << max_response_size_bytes << "\n");

    auto response_size_bytes = compute_response_size_and_inv_big_vars(dds, max_var_size_bytes, too_big_vars);
    BESDEBUG(MODULE, prolog << "response_size_bytes: " << response_size_bytes << "\n");
    BESDEBUG(MODULE, prolog << "too_big_vars: " << too_big_vars.size() << "\n");

    stringstream too_big_message;
    if(its_too_big(
            too_big_message,
            max_response_size_bytes,
            response_size_bytes,
            max_var_size_bytes,
            too_big_vars,
            true)) {
        BESDEBUG(MODULE, prolog << "It's TOO BIG:\n" << too_big_message.str() << "\n");
        throw BESSyntaxUserError(too_big_message.str(), file, line);
    }
}



}   // namespace dap_utils
