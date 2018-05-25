//////////////////////////////////////////////////////////////////////////////
// This file is part of the "NcML Module" project, a BES module designed
// to allow NcML files to be used to be used as a wrapper to add
// AIS to existing datasets of any format.
//
// Copyright (c) 2009 OPeNDAP, Inc.
// Author: Michael Johnson  <m.johnson@opendap.org>
//
// For more information, please also see the main website: http://opendap.org/
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
// Please see the files COPYING and COPYRIGHT for more information on the GLPL.
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
/////////////////////////////////////////////////////////////////////////////

#include <ctype.h>

#include <Array.h>
#include "Constructor.h"
#include "DAS.h"
#include "DDS.h"
#include "Grid.h"
#include <DataDDS.h>
#include <AttrTable.h>

#include "BESDapResponse.h"
#include "BESDataDDSResponse.h"
#include "BESDDSResponse.h"
#include "BESDebug.h"
#include "BESInternalError.h"

#include "NCMLUtil.h"
#include "NCMLDebug.h"

using namespace libdap;
using namespace std;

namespace ncml_module {

const std::string NCMLUtil::WHITESPACE = " \t\n";

int NCMLUtil::tokenize(const string& str, vector<string>& tokens, const string& delimiters)
{
    BESDEBUG("ncml", "NCMLUtil::tokenize value of str:" << str << endl);

    // start empty
    tokens.resize(0);
    // Skip delimiters at beginning.
    string::size_type lastPos = str.find_first_not_of(delimiters, 0);
    // Find first "non-delimiter".
    string::size_type pos = str.find_first_of(delimiters, lastPos);

    int count = 0; // how many we added.
    while (string::npos != pos || string::npos != lastPos) {
        // Found a token, add it to the vector.
        tokens.push_back(str.substr(lastPos, pos - lastPos));
        count++;
        // Skip delimiters.  Note the "not_of"
        lastPos = str.find_first_not_of(delimiters, pos);
        // Find next "non-delimiter"
        pos = str.find_first_of(delimiters, lastPos);
    }
    return count;
}

int NCMLUtil::tokenizeChars(const string& str, vector<string>& tokens)
{
    tokens.resize(0);
    // push each char as a token
    for (unsigned int i = 0; i < str.size(); ++i) {
        string val = "";
        val += str[i];
        tokens.push_back(val);
    }
    return str.size();
}

bool NCMLUtil::isAscii(const string& str)
{
    string::const_iterator endIt = str.end();
    for (string::const_iterator it = str.begin(); it != endIt; ++it) {
        if (!isascii(*it)) {
            return false;
        }
    }
    return true;
}

bool NCMLUtil::isAllWhitespace(const string& str)
{
    return (str.find_first_not_of(" \t\n") == string::npos);
}

void NCMLUtil::trimLeft(std::string& input, const std::string& trimChars /* = WHITESPACE */)
{
    size_t firstValid = input.find_first_not_of(trimChars);
    input.erase(0, firstValid);
}

/** Trim off any number of any character in trimChars from the right side of input.
 *  @return the substring after removing all trailing characters in trimChars.
 */
void NCMLUtil::trimRight(std::string& input, const std::string& trimChars /* = WHITESPACE */)
{
    size_t lastValid = input.find_last_not_of(trimChars);
    if (lastValid != string::npos) {
        input.erase(lastValid + 1, string::npos);
    }
}

void NCMLUtil::trimAll(std::vector<std::string>& tokens, const std::string& trimChars /* = WHITESPACE */)
{
    unsigned int num = tokens.size();
    for (unsigned int i = 0; i < num; ++i) {
        trim(tokens[i], trimChars);
    }
}

bool NCMLUtil::toUnsignedInt(const std::string& stringVal, unsigned int& oVal)
{
    bool success = true;
    oVal = 0;
    istringstream iss(stringVal);
    iss >> oVal;
    if (iss.fail() || (stringVal[0] == '-') // parsing negatives is locale-dependent, but we DO NOT want them allowed.
        ) {
        success = false;
    }
    return success;
}

/**
 * Does this AttrTable have descendants that are scalar or vector attributes?
 *
 * @param a The AttrTable
 * @return true if the table contains a scalar- or vector-valued attribute,
 * otherwise false.
 */
static bool
has_dap2_attributes(AttrTable &a)
{
    for (AttrTable::Attr_iter i = a.attr_begin(), e = a.attr_end(); i != e; ++i) {
        if (a.get_attr_type(i) != Attr_container)
            return true;
        else
            return has_dap2_attributes(*a.get_attr_table(i));
    }

    return false;
}

static bool
has_dap2_attributes(BaseType *btp)
{
    if (btp->get_attr_table().get_size() && has_dap2_attributes(btp->get_attr_table())) {
        return true;
    }

    Constructor *cons = dynamic_cast<Constructor *>(btp);
    if (cons) {
        Grid* grid = dynamic_cast<Grid*>(btp);
        if(grid){
            return has_dap2_attributes(grid->get_array());
        }
        else {
            for (Constructor::Vars_iter i = cons->var_begin(), e = cons->var_end(); i != e; i++) {
                if (has_dap2_attributes(*i)) return true;
            }
        }
    }

    return false;
}

/** Recursion helper:
 *  Recurse on the members of composite variable consVar and recursively add their AttrTables
 *  to the given dasTable for the container.
 */
static void populateAttrTableForContainerVariableRecursive(AttrTable* dasTable, Constructor* consVar)
{
    VALID_PTR(dasTable);
    VALID_PTR(consVar);

    if(!has_dap2_attributes(consVar))
        return;


    Grid* grid = dynamic_cast<Grid*>(consVar);
    if(grid){
        // Here we take the Attributes from the Grid Array variable and copy them into the DAS container for the Grid.
        // This essentially flattens the Grid in the DAS. Note too that we do now pursue the MAP vectors so they
        // do not appear in the DAS container for the Grid.
        BESDEBUG("ncml",  __func__ << "() The variable " << grid->name() << " is a Grid. So, we promote the Grid Array AttrTable content to the DAS container for Grid " << grid->name() << endl);
        Array *gArray = grid->get_array();
        AttrTable arrayAT = gArray->get_attr_table();
        for( AttrTable::Attr_iter atIter = arrayAT.attr_begin(); atIter!=arrayAT.attr_end(); ++atIter){
            AttrType type = arrayAT.get_attr_type(atIter);
            string childName = arrayAT.get_name(atIter);
            if (type == Attr_container){
                BESDEBUG("ncml", __func__ << "() Adding child AttrTable '" << childName << "' to Grid " << grid->name() << endl);
                dasTable->append_container( new AttrTable(*arrayAT.get_attr_table(atIter)), childName);
            }
            else {
                vector<string>* pAttrTokens = arrayAT.get_attr_vector(atIter);
                // append_attr makes a copy of the vector, so we don't have to do so here.
                BESDEBUG("ncml", __func__ << "() Adding child Attrbute '" << childName << "' to Grid " << grid->name() << endl);
                dasTable->append_attr(childName, AttrType_to_String(type), pAttrTokens);
            }
        }
    }
    else {
        // It's not a Grid but it's still a Constructor.
        BESDEBUG("ncml",  __func__ << "() Adding attribute tables for children of a Constructor type variable " << consVar->name() << endl);
        Constructor::Vars_iter endIt = consVar->var_end();
        for (Constructor::Vars_iter it = consVar->var_begin(); it != endIt; ++it) {
            BaseType* var = *it;
            VALID_PTR(var);

            if(has_dap2_attributes(var)){
                BESDEBUG("ncml",  __func__ << "() Adding attribute table for var: " << var->name() << endl);
                // Make a new table for the child variable
                AttrTable* newTable = new AttrTable(var->get_attr_table());
                // Add it to the DAS's attribute table for the consVar scope.
                dasTable->append_container(newTable, var->name());

                // If it's a container type, we need to recurse.
                if (var->is_constructor_type()) {
                    Constructor* child = dynamic_cast<Constructor*>(var);
                    if (!child) {
                        throw BESInternalError("Type cast error", __FILE__, __LINE__);
                    }
                    BESDEBUG("ncml", __func__ << "() Var " << child->name() << " is Constructor type, recursively adding attribute tables" << endl);
                    populateAttrTableForContainerVariableRecursive(newTable, child);
                }
            }
            else {
                BESDEBUG("ncml", __func__ << "() Variable '" << var->name() << "' has no dap2 attributes,. Skipping."<< endl);
            }
        }
    }
}

// This is basically the opposite of transfer_attributes.
void NCMLUtil::populateDASFromDDS(DAS* das, const DDS& dds_const)
{
    BESDEBUG("ncml", "Populating a DAS from a DDS...." << endl);

    VALID_PTR(das);

    // Make sure the DAS is empty to start.
    das->erase();

    // dds is semantically const in this function, but the calls to it aren't...
    DDS& dds = const_cast<DDS&>(dds_const);

    // First, make sure we don't have a container at top level since we're assuming for now
    // that we only have one dataset per call (right?)
    if (dds.container()) {
        BESDEBUG("ncml", __func__ << "()  Got unexpected container " << dds.container_name() << " and is failing." << endl);
        throw BESInternalError("Unexpected Container Error creating DAS from DDS in NCMLHandler", __FILE__, __LINE__);
    }

    // Copy over the global attributes table
    //BESDEBUG("ncml", "Coping global attribute tables from DDS to DAS..." << endl);
    *(das->get_top_level_attributes()) = dds.get_attr_table();

    // For each variable in the DDS, make a table in the DAS.
    //  If the variable in composite, then recurse
    // BESDEBUG("ncml", "Adding attribute tables for all DDS variables into DAS recursively..." << endl);
    DDS::Vars_iter endIt = dds.var_end();
    for (DDS::Vars_iter it = dds.var_begin(); it != endIt; ++it) {
        // For each BaseType*, copy its table and add to DAS under its name.
        BaseType* var = *it;
        VALID_PTR(var);

        // By adding this test we stop adding empty top=level containers to the DAS.
        if(has_dap2_attributes(var)){
            BESDEBUG("ncml", "Adding attribute table for variable: " << var->name() << endl);
            AttrTable* clonedVarTable = new AttrTable(var->get_attr_table());
            VALID_PTR(clonedVarTable);
            das->add_table(var->name(), clonedVarTable);

            // If it's a container type, we need to recurse.
            if (var->is_constructor_type()) {
                Constructor* consVar = dynamic_cast<Constructor*>(var);
                if (!consVar) {
                    throw BESInternalError("Type cast error", __FILE__, __LINE__);
                }
                populateAttrTableForContainerVariableRecursive(clonedVarTable, consVar);
            }
        }
        else {
            BESDEBUG("ncml", __func__ << "() Variable '" << var->name() << "' has no dap2 attributes,. Skipping."<< endl);
        }
    }
}

// This function was added since DDS::operator= had some bugs we need to fix.
// At that point, we can just use that function, probably.
void NCMLUtil::copyVariablesAndAttributesInto(DDS* dds_out, const DDS& dds_in)
{
    VALID_PTR(dds_out);

    // Avoid obvious bugs
    if (dds_out == &dds_in) {
        return;
    }

    // handle semantic constness
    DDS& dds = const_cast<DDS&>(dds_in);

    // Copy the global attribute table
    dds_out->get_attr_table() = dds.get_attr_table();

    // copy the things pointed to by the variable list, not just the pointers
    // add_var is designed to deepcopy *i, so this should get all the children
    // as well.
    for (DDS::Vars_iter i = dds.var_begin(); i != dds.var_end(); ++i) {
        dds_out->add_var(*i); // add_var() dups the BaseType.
    }
}

libdap::DDS*
NCMLUtil::getDDSFromEitherResponse(BESDapResponse* response)
{
    DDS* pDDS = 0;
    BESDDSResponse* pDDXResponse = dynamic_cast<BESDDSResponse*>(response);
    BESDataDDSResponse* pDataDDSResponse = dynamic_cast<BESDataDDSResponse*>(response);

    if (pDDXResponse) {
        pDDS = pDDXResponse->get_dds();
    }
    else if (pDataDDSResponse) {
        pDDS = pDataDDSResponse->get_dds(); // return as superclass ptr
    }
    else {
        pDDS = 0; // return null on error
    }

    return pDDS;
}

// This little gem takes attributes that have been added to the top level
// attribute table (which is allowed in DAP4) and moves them all to a single
// container. In DAP2, only containers are allowed at the top level of the
// DAS. By _convention_ the name of the global attributes is NC_GLOBAL although
// other names are equally valid...
//
// How this works: The top-level attribute table is filled with various global
// attributes. To follow the spec for DAP2 that top-level container must contain
// _only_ other containers, each of which must be named. There are four cases...
//
// jhrg 12/15/11
void NCMLUtil::hackGlobalAttributesForDAP2(libdap::AttrTable &global_attributes,
    const std::string &global_container_name)
{
    if (global_container_name.empty()) return;

    // Cases: 1. only containers at the top --> return
    //        2. only simple attrs at the top --> move them into one container
    //        3. mixture of simple and containers --> move the simples into a new container
    //        4. mixture ...  and global_container_name exists --> move simples into that container

    // Look at the top-level container and see if it has any simple attributes.
    // If it is empty or has only containers, do nothing.
    bool simple_attribute_found = false;
    AttrTable::Attr_iter i = global_attributes.attr_begin();
    while (!simple_attribute_found && i != global_attributes.attr_end()) {
        if (!global_attributes.is_container(i)) simple_attribute_found = true;
        ++i;
    }

    // Case 1
    if (!simple_attribute_found) return;
#if 0
    // Now determine if there are _only_ simple attributes
    bool only_simple_attributes = true;
    i = global_attributes.attr_begin();
    while (only_simple_attributes && i != global_attributes.attr_end()) {
        if (global_attributes.is_container(i))
        only_simple_attributes = false;
        ++i;
    }

    // Case 2
    // Note that the assignment operator first clears the destination and
    // then performs a deep copy, so the 'new_global_attr_container' will completely
    // replace the existing collection of attributes at teh top-level.
    if (only_simple_attributes)
    {
        AttrTable *new_global_attr_container = new AttrTable();
        AttrTable *new_attr_container = new_global_attr_container->append_container(global_container_name);
        *new_attr_container = global_attributes;
        global_attributes = *new_global_attr_container;

        return;
    }
#endif
    // Cases 2, 3 & 4
    AttrTable *new_attr_container = global_attributes.find_container(global_container_name);
    if (!new_attr_container) new_attr_container = global_attributes.append_container(global_container_name);

    // Now we have a destination for all the simple attributes
    i = global_attributes.attr_begin();
    while (i != global_attributes.attr_end()) {
        if (!global_attributes.is_container(i)) {
            new_attr_container->append_attr(global_attributes.get_name(i), global_attributes.get_type(i),
                global_attributes.get_attr_vector(i));
        }
        ++i;
    }

    // Now delete the simple attributes we just moved; they are not deleted in the
    // above loop because deleting things in a container invalidates iterators
    i = global_attributes.attr_begin();
    while (i != global_attributes.attr_end()) {
        if (!global_attributes.is_container(i)) {
            global_attributes.del_attr(global_attributes.get_name(i));
            //  delete invalidates iterators; must restart the loop
            i = global_attributes.attr_begin();
        }
        else {
            ++i;
        }
    }

    return;
}

void NCMLUtil::setVariableNameProperly(libdap::BaseType* pVar, const std::string& name)
{
    VALID_PTR(pVar);
    pVar->set_name(name);
    // if template, set it too since it's used to print dds...
    BaseType* pTemplate = pVar->var();
    if (pTemplate) {
        pTemplate->set_name(name);
    }
}
} // namespace ncml_module
