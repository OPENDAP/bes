// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES, a component
// of the Hyrax Data Server

// Copyright (c) 2016 OPeNDAP, Inc.
// Authors: Nathan Potter <ndp@opendap.org>
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

#include <D4RValue.h>
#include <DMR.h>
#include <DDS.h>
#include <BaseType.h>
#include <Str.h>
#include <Structure.h>
#include <D4Group.h>

#include <BESDebug.h>
#include <BESUtil.h>
#include <BESInternalError.h>

#include "DapFunctionUtils.h"

#define DEBUG_KEY "functions"

/**
 * @brief Copies the passed Structure's Attributes into the global AttrTable in
 * the passed DDS.
 */
void promote_atributes_to_global(libdap::Structure *sourceObj, libdap::DDS *fdds){

    libdap::AttrTable sourceAttrTable = sourceObj->get_attr_table();

    libdap::AttrTable::Attr_iter endIt = sourceAttrTable.attr_end();
    libdap::AttrTable::Attr_iter it;
    for (it = sourceAttrTable.attr_begin(); it != endIt; ++it) {
        std::string childName = sourceAttrTable.get_name(it);
        bool childIsContainer = sourceAttrTable.is_container(it);
        BESDEBUG(DEBUG_KEY, "DFU::promote_atributes_to_global() - Processing attribute " << childName << endl );
        if (childIsContainer) {
            libdap::AttrTable* pClonedAttrTable = new libdap::AttrTable(*sourceAttrTable.get_attr_table(it));
            fdds->get_attr_table().append_container(pClonedAttrTable, childName);
        }
        else {
            string type = sourceAttrTable.get_type(it);
            vector<string>* pAttrTokens = sourceAttrTable.get_attr_vector(it);
            // append_attr makes a copy of the vector, so we don't have to do so here.
            fdds->get_attr_table().append_attr(childName, type, pAttrTokens);
        }
    }
}


/**
 * @brief For a Structure in the given DDS, if that Structure's name ends with
 * "_unwrap" take each variable from the structure and 'promote it' to the
 * top level of the DDS.
 *
 * @note Here we remove top-level Structure variables that have been added by server
 * functions which return multiple values. This will necessarily be a hack since
 * DAP2 was never designed to do this sort of thing - support functions that
 * return computed values.
 *
 * @note The DDS referenced by 'fdds' may have one or more variables because there may
 * have been one or more function calls given in the CE supplied by the CE. For
 * example, "linear_scale(SST),linear_scale(AIRT)" would have two variables. It is
 * possible for a CE that contains function calls to also include a 'regular'
 * projection (i.e., "linear_scale(SST),SST[0][0:179]") but this means run the function(s)
 * and build a new DDS and then apply the remaining constraints to that new DDS. So,
 * this (hack) code can assume that there is one variable per function call and no
 * other variables. Furthermore, lets adopt a simple convention that functions use
 * a Structure named <something>_unwrap when they want the function result to be
 * unwrapped and something else when they want this code to leave the Structure as
 * it is.
 *
 * @param dds The source DDS - look for Structures here
 * @return A new DDS with new instances such that the Structures named
 * *_unwrap have been removed and their members 'promoted' up to the new
 * DDS's top level scope.
 */
void promote_function_output_structures(libdap::DDS *fdds)
{
    BESDEBUG(DEBUG_KEY, "DFU::promote_function_output_structures() - BEGIN" << endl);

    // Dump pointers to the values here temporarily... If we had methods in libdap
    // that could be used to access the underlying erase() and insert() methods, we
    // could skip the (maybe expensive) copy operations I use below. What we would
    // need are ways to delete a Structure/Constructor without calling delete on its
    // fields and ways to call vector::erase() and vector::insert(). Some of this
    // exists, but it's not quite enough.
    //
    // Proposal: Make it so that the Structure/Constructor has a release() method
    // that allows you take the members out of the instance with the contract
    // that "The released members will be deleted deleted so that the
    // Structure/Constructor object only need to drop it's reference to them".
    // With that in place we might then be able to manipulate the DAP objects
    // without excessive copying. We can use add_var_nocopy() to put things in,
    // and we can use release() to pull things out.
    //
    // Assumption: add_var_nocopy() has the contract that the container to which
    // the var is added will eventually delete the var.

    std::vector<libdap::BaseType *> upVars;
    std::vector<libdap::BaseType *> droppedContainers;
    for (libdap::DDS::Vars_citer di = fdds->var_begin(), de = fdds->var_end(); di != de; ++di) {

        libdap::Structure *collection = dynamic_cast<libdap::Structure *>(*di);
        if (collection && BESUtil::endsWith(collection->name(), "_unwrap")) {
            BESDEBUG(DEBUG_KEY, "DFU::promote_function_output_structures() - Promoting members of collection '" << collection->name() << "'" << endl);
            // Once we promote the members we need to drop the parent collection
            // but we can't do that while we're iterating over its contents
            // so we'll cache the reference for later.
            droppedContainers.push_back(collection);
            // Promote the structures attributes to the DDS AttrTable.
            promote_atributes_to_global(collection,fdds);
            // We're going to 'flatten this structure' and return its fields
            libdap::Structure::Vars_iter vi;
            for (vi =collection->var_begin(); vi != collection->var_end(); ++vi) {
                libdap::BaseType *origVar = *vi;
                // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                // This performs a deep copy on origVar (ouch!), and we do it because in the current
                // libdap API, when we delete parent structure the variable will be deleted too.
                // Because we can't pluck a variable out of a DAP object without deleting it.
                // @TODO Fix the libdap API to allow this operation without the copy/delete bits.
                libdap::BaseType *newVar = origVar->ptr_duplicate();
                // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                // This ensures that the variable's semantics are consistent
                // with a top level variable.
                newVar->set_parent(0);
                // Add the new variable to the list of stuff to add back to the dataset.
                upVars.push_back(newVar);
            }
        }
    }
    // Drop Promoted Containers
    for(std::vector<libdap::BaseType *>::iterator it=droppedContainers.begin(); it != droppedContainers.end(); ++it) {
        libdap::BaseType *bt = *it;
        BESDEBUG(DEBUG_KEY, "DFU::promote_function_output_structures() - Deleting Promoted Collection '" << bt->name() << "' ptr: " << bt << endl);
        // Delete the Container variable and ALL of it's children.
        // @TODO Wouldn't it be nice if at this point it had no children? I think so too.
        fdds->del_var(bt->name());
    }

    // Add (copied) promoted variables to top-level of DDS
    for( std::vector<libdap::BaseType *>::iterator it = upVars.begin(); it != upVars.end(); it ++) {
        libdap::BaseType *bt = *it;
        BESDEBUG(DEBUG_KEY, "DFU::promote_function_output_structures() - Adding Promoted Variable '" << bt->name() << "' to DDS. ptr: " << bt << endl);
        fdds->add_var(bt);
        delete bt;
    }

    BESDEBUG(DEBUG_KEY, "DFU::promote_function_output_structures() - END" << endl);
}



libdap::BaseType *wrapitup_worker(vector<libdap::BaseType*> argv, libdap::AttrTable globals) {

    BESDEBUG(DEBUG_KEY, "DFU::wrapitup_worker() - BEGIN" << endl);

    std::string wrap_name="thing_to_unwrap";
    libdap::Structure *dapResult = new libdap::Structure(wrap_name);
    int argc = argv.size();
    if(argc>0){
        BESDEBUG(DEBUG_KEY, "DFU::wrapitup_worker() - Attempting to return arguments bundled into "<< wrap_name << endl);

        for(int i=0; i<argc ; i++){
            libdap::BaseType *bt = argv[i];
            BESDEBUG(DEBUG_KEY, "DFU::wrapitup_worker() - Reading  "<< bt->name() << endl);
            bt->read();
            BESDEBUG(DEBUG_KEY, "DFU::wrapitup_worker() - Adding a copy of "<< bt->name() << " to " << dapResult->name() << endl);

            // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            // This performs a deep copy on bt (ouch!), and we do it because in the current
            // libdap API, when we delete parent structure the variable will be deleted too.
            // Because we can't pluck a variable out of a DAP object without deleting it.
            // @TODO Fix the libdap API to allow this operation without the copy/delete bits.
            dapResult->add_var_nocopy(bt->ptr_duplicate());
            // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        }
        BESDEBUG(DEBUG_KEY, "DFU::wrapitup_worker() - Copying Global Attributes" << endl << globals << endl);

        libdap::AttrTable *newDatasetAttrTable = new libdap::AttrTable(globals);
        dapResult->set_attr_table(*newDatasetAttrTable);
        BESDEBUG(DEBUG_KEY, "DFU::wrapitup_worker() - Result Structure attrs: " << endl << dapResult->get_attr_table() << endl);

    }
    else {
        BESDEBUG(DEBUG_KEY, "DFU::wrapitup_worker() - Creating response object called "<< dapResult->name() << endl);

        libdap::Str *message = new libdap::Str("promoted_message");
        message->set_value("This libdap:Str object should appear at the top level of the response and not as a member of a libdap::Constructor type.");
        dapResult->add_var_nocopy(message);

        // Mark String as read and queue for transmission
        message->set_read_p(true);
        message->set_send_p(true);

    }

    // Mark dapResult Structure as read and queue for transmission
    dapResult->set_read_p(true);
    dapResult->set_send_p(true);



    BESDEBUG(DEBUG_KEY, "DFU::wrapitup_worker() - END" << endl);
    return dapResult;

}



/**
 *  @brief Wraps the passed arguments (argv) in a Structure
 *  whose name "thing_to_unwrap" meets the criteria for function
 *  result promotion in a Transmitter. The criteria is expected to
 *  be: The Structure name ends with "_unwrap".
 *  If parameters are not provided then wrapitup() will make
 *  a structure with the above name and put a String variable into it.
 *
 * @param argc Count of the function's arguments
 * @param argv Array of pointers to the functions arguments
 * @param dds Reference to the DDS object for the complete dataset.
 * This holds pointers to all of the variables and attributes in the
 * dataset.
 * @param btpp Return the function result in an instance of BaseType
 * referenced by this pointer to a pointer. We could have used a
 * BaseType reference, instead of pointer to a pointer, but we didn't.
 * This is a value-result parameter.
 *
 * @return void
 *
 * @exception Error Thrown If the Array is not a one dimensional
 * array.
 **/
#if 0
void wrapitup(int argc, libdap::BaseType *argv[], libdap::DDS &dds, libdap::BaseType **btpp) {

    std::string wrap_name=dds.get_dataset_name()+"_unwrap";

    BESDEBUG(DEBUG_KEY, "DFU::wrapitup() - BEGIN" << endl);

    libdap::Structure *dapResult = new libdap::Structure(wrap_name);


    if(argc>0){
        BESDEBUG(DEBUG_KEY, "DFU::wrapitup() - Attempting to return arguments bundled into "<< wrap_name << endl);

        for(int i=0; i<argc ; i++){
            libdap::BaseType *bt = argv[i];
            BESDEBUG(DEBUG_KEY, "DFU::wrapitup() - Reading  "<< bt->name() << endl);
            bt->read();
            BESDEBUG(DEBUG_KEY, "DFU::wrapitup() - Adding a copy of "<< bt->name() << " to " << dapResult->name() << endl);

            // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            // This performs a deep copy on bt (ouch!), and we do it because in the current
            // libdap API, when we delete parent structure the variable will be deleted too.
            // Because we can't pluck a variable out of a DAP object without deleting it.
            // @TODO Fix the libdap API to allow this operation without the copy/delete bits.
            dapResult->add_var_nocopy(bt->ptr_duplicate());
            // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        }
        libdap::AttrTable &globals = dds.get_attr_table();
        BESDEBUG(DEBUG_KEY, "DFU::wrapitup() - Copying Global Attributes" << endl << globals << endl);

        libdap::AttrTable *newDatasetAttrTable = new libdap::AttrTable(globals);
        dapResult->set_attr_table(*newDatasetAttrTable);
        BESDEBUG(DEBUG_KEY, "DFU::wrapitup() - Result Structure attrs: " << endl << dapResult->get_attr_table() << endl);

    }
    else {
        BESDEBUG(DEBUG_KEY, "DFU::wrapitup() - Creating response object called "<< dapResult->name() << endl);

        libdap::Str *message = new libdap::Str("promoted_message");
        message->set_value("This libdap:Str object should appear at the top level of the response and not as a member of a libdap::Constructor type.");
        dapResult->add_var_nocopy(message);

        // Mark String as read and queue for transmission
        message->set_read_p(true);
        message->set_send_p(true);

    }

    // Mark dapResult Structure as read and queue for transmission
    dapResult->set_read_p(true);
    dapResult->set_send_p(true);


    *btpp = dapResult;

    BESDEBUG(DEBUG_KEY, "DFU::wrapitup() - END" << endl);
}
#endif


void function_dap2_wrapitup(int argc, libdap::BaseType *argv[], libdap::DDS &dds, libdap::BaseType **btpp)
{
    BESDEBUG(DEBUG_KEY, "function_dap2_wrapitup() - BEGIN" << endl);

    BESDEBUG(DEBUG_KEY, "function_dap2_wrapitup() - Building argument vector for wrapitup_worker()" << endl);
    vector<libdap::BaseType*> args;
    for(int i=0; i< argc; i++){
        libdap::BaseType *bt = argv[i];
        BESDEBUG(DEBUG_KEY, "function_dap2_wrapitup() - Adding argument: "<< bt->name() << endl);
        args.push_back(bt);
    }

    *btpp = wrapitup_worker(args,dds.get_attr_table());

    BESDEBUG(DEBUG_KEY, "function_dap2_wrapitup() - END (result: "<< (*btpp)->name() << ")" << endl);
    return;
}


libdap::BaseType *function_dap4_wrapitup(libdap::D4RValueList *dvl_args, libdap::DMR &dmr){

    BESDEBUG(DEBUG_KEY, "function_dap4_wrapitup() - Building argument vector for wrapitup_worker()" << endl);
    vector<libdap::BaseType*> args;
    for(unsigned int i=0; i< dvl_args->size(); i++){
        libdap::BaseType * bt = dvl_args->get_rvalue(i)->value(dmr);
        BESDEBUG(DEBUG_KEY, "function_dap4_wrapitup() - Adding argument: "<< bt->name() << endl);
        args.push_back(bt);
    }


    libdap::BaseType *result = wrapitup_worker(args, dmr.root()->get_attr_table());

    BESDEBUG(DEBUG_KEY, "function_dap4_wrapitup() - END (result: "<< result->name() << ")" << endl);
    return result;

}

