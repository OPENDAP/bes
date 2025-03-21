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
#include "config.h"

#include "AggregationUtil.h"

// agg_util includes
#include "AggMemberDataset.h"
#include "AggregationException.h"
#include "Dimension.h"

// libdap includes
#include <libdap/Array.h> // libdap
#include <libdap/AttrTable.h>
#include <libdap/BaseType.h>
#include <libdap/DataDDS.h>
#include <libdap/DDS.h>
#include <libdap/Grid.h>
#include "BESDebug.h"
#include "BESStopWatch.h"

// Outside includes (MINIMIZE THESE!)
#include "NCMLDebug.h" // This the ONLY dependency on NCML Module I want in this class since the macros there are general it's ok...

using libdap::Array;
using libdap::AttrTable;
using libdap::BaseType;
using libdap::Constructor;
using libdap::DataDDS;
using libdap::DDS;
using libdap::Grid;
using libdap::Vector;
using std::string;
using std::vector;

#define MODULE "agg_util"

// AggregationUtil
#define prolog_au string("AggregationUtil::").append(__func__).append("() - ")
// TopLevelArrayGetter
#define prolog_tlag string("TopLevelArrayGetter::").append(__func__).append("() - ")
// TopLevelGridDataArrayGetter
#define prolog_tlgdag string("TopLevelGridDataArrayGetter::").append(__func__).append("() - ")
// TopLevelGridMapArrayGetter
#define prolog_tlgmag string("TopLevelGridMapArrayGetter::").append(__func__).append("() - ")

namespace agg_util {
// Static class member used to track the position of the last CVs insertion
// when building a JoinExisting aggregation.
int AggregationUtil::d_last_added_cv_position = 0;

/////////////////////////////////////////////////////////////////////////////
// ArrayGetterInterface impls

/* virtual */
ArrayGetterInterface::~ArrayGetterInterface()
{
}

/////////////////////////////////////////////////////////////////////////////
// TopLevelArrayGetter impl

TopLevelArrayGetter::TopLevelArrayGetter() :
    ArrayGetterInterface()
{
}

/* virtual */
TopLevelArrayGetter::~TopLevelArrayGetter()
{
}

/* virtual */
TopLevelArrayGetter*
TopLevelArrayGetter::clone() const
{
    return new TopLevelArrayGetter(*this);
}

/* virtual */
libdap::Array*
TopLevelArrayGetter::readAndGetArray(const std::string& name, const libdap::DDS& dds,
    const libdap::Array* const pConstraintTemplate, const std::string& debugChannel) const
{

    BES_STOPWATCH_START(MODULE, prolog_tlag + "Timing");

    // First, look up the BaseType
    BaseType* pBT = AggregationUtil::getVariableNoRecurse(dds, name);

    // Next, if it's not there, throw exception.
    if (!pBT) {
        throw AggregationException("TopLevelArrayGetter: "
            "Did not find a variable named \"" + name + "\" at the top-level of the DDS!");
    }

    // Next, make sure it's an Array before we cast it
    // Prefer using the enum type for speed rather than RTTI
    if (pBT->type() != libdap::dods_array_c) {
        throw AggregationException("TopLevelArrayGetter: "
            "The top-level DDS variable named \"" + name + "\" was not of the expected type!"
            " Expected:Array  Found:" + pBT->type_name());
    }

    libdap::Array* pDatasetArray = static_cast<libdap::Array*>(pBT);

    // If given, copy the constraints over to the found Array
    if (pConstraintTemplate) {
        agg_util::AggregationUtil::transferArrayConstraints(pDatasetArray, // into this dataset array to be read
            *pConstraintTemplate, // from this template
            false, // same rank Array's in template and loaded, don't skip first dim
            false, // or the to dimension.  copy whole thing.
            !(debugChannel.empty()), // printDebug
            debugChannel);
    }

    // Force a read() perhaps with constraints
    pDatasetArray->set_send_p(true);
    pDatasetArray->set_in_selection(true);
    pDatasetArray->read();

    return pDatasetArray;
}

/////////////////////////////////////////////////////////////////////////////
// TopLevelGridDataArrayGetter impl

TopLevelGridDataArrayGetter::TopLevelGridDataArrayGetter() :
    ArrayGetterInterface()
{
}

/* virtual */
TopLevelGridDataArrayGetter::~TopLevelGridDataArrayGetter()
{
}

/* virtual */
TopLevelGridDataArrayGetter*
TopLevelGridDataArrayGetter::clone() const
{
    return new TopLevelGridDataArrayGetter(*this);
}

/* virtual */
libdap::Array*
TopLevelGridDataArrayGetter::readAndGetArray(const std::string& name, const libdap::DDS& dds,
    const libdap::Array* const pConstraintTemplate, const std::string& debugChannel) const
{
    BES_STOPWATCH_START(MODULE, prolog_tlgdag + "Timing");

    // First, look up the BaseType
    BaseType* pBT = AggregationUtil::getVariableNoRecurse(dds, name);

    // Next, if it's not there, throw exception.
    if (!pBT) {
        throw AggregationException("TopLevelGridArrayGetter: "
            "Did not find a variable named \"" + name + "\" at the top-level of the DDS!");
    }

    // Next, make sure it's a Grid before we cast it
    // Prefer using the enum type for speed rather than RTTI
    if (pBT->type() != libdap::dods_grid_c) {
        throw AggregationException("TopLevelGridArrayGetter: "
            "The top-level DDS variable named \"" + name + "\" was not of the expected type!"
            " Expected:Grid  Found:" + pBT->type_name());
    }

    // Grab the array and return it.
    Grid* pDataGrid = static_cast<Grid*>(pBT);
    Array* pDataArray = static_cast<Array*>(pDataGrid->array_var());
    if (!pDataArray) {
        throw AggregationException("TopLevelGridArrayGetter: "
            "The data Array var for variable name=\"" + name + "\" was unexpectedly null!");
    }

    // If given, copy the constraints over to the found Array
    if (pConstraintTemplate) {
        agg_util::AggregationUtil::transferArrayConstraints(pDataArray, // into this data array to be read
            *pConstraintTemplate, // from this template
            false, // same rank Array's in template and loaded, don't skip first dim
            false, // also don't skip in the to array
            !(debugChannel.empty()), // printDebug
            debugChannel);
    }

    // Force the read() on the Grid level since some handlers
    // cannot handle a read on a subobject unless read() is called
    // on the parent object.  We have given the constraints to the
    // data Array already.
    // TODO make an option on whether to load the Grid's map
    // vectors or not!  I think for these cases we do not want them ever!
    pDataGrid->set_send_p(true);
    pDataGrid->set_in_selection(true);
    pDataGrid->read();

    // Also make sure the Array was read and if not call it as well.
    if (!pDataArray->read_p()) {
        pDataArray->set_send_p(true);
        pDataArray->set_in_selection(true);
        pDataArray->read();
    }

    return pDataArray;
}

/////////////////////////////////////////////////////////////////////////////
// TopLevelGridMapArrayGetter impl

TopLevelGridMapArrayGetter::TopLevelGridMapArrayGetter(const std::string& gridName) :
    ArrayGetterInterface(), _gridName(gridName)
{
}

/* virtual */
TopLevelGridMapArrayGetter::~TopLevelGridMapArrayGetter()
{
}

/* virtual */
TopLevelGridMapArrayGetter*
TopLevelGridMapArrayGetter::clone() const
{
    return new TopLevelGridMapArrayGetter(*this);
}

/* virtual */
libdap::Array*
TopLevelGridMapArrayGetter::readAndGetArray(const std::string& arrayName, const libdap::DDS& dds,
    const libdap::Array* const pConstraintTemplate, const std::string& debugChannel) const
{
    BES_STOPWATCH_START(MODULE, prolog_tlgmag + "Timing");

    // First, look up the Grid the map is in
    BaseType* pBT = AggregationUtil::getVariableNoRecurse(dds, _gridName);

    // Next, if it's not there, throw exception.
    if (!pBT) {
        throw AggregationException("Did not find a variable named \"" + _gridName + "\" at the top-level of the DDS!");
    }

    // Next, make sure it's a Grid before we cast it
    // Prefer using the enum type for speed rather than RTTI
    if (pBT->type() != libdap::dods_grid_c) {
        throw AggregationException(
            "The top-level DDS variable named \"" + _gridName + "\" was not of the expected type!"
                " Expected:Grid  Found:" + pBT->type_name());
    }

    // Find the correct map
    Grid* pDataGrid = static_cast<Grid*>(pBT);
    Array* pMap = const_cast<Array*>(AggregationUtil::findMapByName(*pDataGrid, arrayName));
    NCML_ASSERT_MSG(pMap,
        "Expected to find the map with name " + arrayName + " within the Grid " + _gridName + " but failed to find it!");

    // Prepare it to be read in so we can get the data
    pMap->set_send_p(true);
    pMap->set_in_selection(true);

    // If given, copy the constraints over to the found Array
    if (pConstraintTemplate) {
        agg_util::AggregationUtil::transferArrayConstraints(pMap, // into this data array to be read
            *pConstraintTemplate, // from this template
            false, // same rank Array's in template and loaded, don't skip first dim
            false, // also don't skip in the to array
            !(debugChannel.empty()), // printDebug
            debugChannel);
    }

    // Do the read
    pMap->read();

    return pMap;
}

/*********************************************************************************************************
 * AggregationUtil Impl
 */
void AggregationUtil::performUnionAggregation(DDS* pOutputUnion, const ConstDDSList& datasetsInOrder)
{
    VALID_PTR(pOutputUnion);

    // Union any non-aggregated variables from the template dataset into the aggregated dataset
    // Because we want the joinExistingaggregation to build up the Coordinate Variables (CVs)
    // in the order they are declared in the NCML file, we need to track the current position
    // where the last one was inserted. We can do that with a field in the AggregationUtil
    // class. Here we reset that field so that it starts at position 0. 12.13.11 jhrg
    AggregationUtil::resetCVInsertionPosition();

    vector<const DDS*>::const_iterator endIt = datasetsInOrder.end();
    vector<const DDS*>::const_iterator it;
    for (it = datasetsInOrder.begin(); it != endIt; ++it) {
        const DDS* pDDS = *it;
        VALID_PTR(pDDS);

        // Merge in the global attribute tables
        unionAttrsInto(&(pOutputUnion->get_attr_table()),
        // TODO there really should be const version of this in libdap::DDS
            const_cast<DDS*>(pDDS)->get_attr_table());

        // Merge in the variables, which carry their tables along with them since the AttrTable is
        // within the variable's "namespace", or lexical scope.
        unionAllVariablesInto(pOutputUnion, *pDDS);
    }
}

void AggregationUtil::unionAttrsInto(AttrTable* pOut, const AttrTable& fromTableIn)
{
    // semantically const
    AttrTable& fromTable = const_cast<AttrTable&>(fromTableIn);
    AttrTable::Attr_iter endIt = fromTable.attr_end();
    AttrTable::Attr_iter it;
    for (it = fromTable.attr_begin(); it != endIt; ++it) {
        const string& name = fromTable.get_name(it);
        AttrTable::Attr_iter attrInOut;
        bool foundIt = findAttribute(*pOut, name, attrInOut);
        // If it's already in the output, then skip it
        if (foundIt) {
            BESDEBUG("ncml",
                "Union of AttrTable: an attribute named " << name << " already exist in output, skipping it..." << endl);
            continue;
        }
        else // put a copy of it into the output
        {
            // containers need deep copy
            if (fromTable.is_container(it)) {
                AttrTable* pOrigAttrContainer = fromTable.get_attr_table(it);
                NCML_ASSERT_MSG(pOrigAttrContainer,
                    "AggregationUtil::mergeAttrTables(): expected non-null AttrTable for the attribute container: "
                        + name);
                AttrTable* pClonedAttrContainer = new AttrTable(*pOrigAttrContainer);
                VALID_PTR(pClonedAttrContainer);
                pOut->append_container(pClonedAttrContainer, name);
                BESDEBUG("ncml",
                    "Union of AttrTable: adding a deep copy of attribute=" << name << " to the merged output." << endl);
            }
            else // for a simple type
            {
                string type = fromTable.get_type(it);
                vector<string>* pAttrTokens = fromTable.get_attr_vector(it);
                VALID_PTR(pAttrTokens);
                // append_attr makes a copy of the vector, so we don't have to do so here.
                pOut->append_attr(name, type, pAttrTokens);
            }
        }
    }
}

bool AggregationUtil::findAttribute(const AttrTable& inTable, const string& name, AttrTable::Attr_iter& attr)
{
    AttrTable& inTableSemanticConst = const_cast<AttrTable&>(inTable);
    attr = inTableSemanticConst.simple_find(name);
    return (attr != inTableSemanticConst.attr_end());
}

void AggregationUtil::unionAllVariablesInto(libdap::DDS* pOutputUnion, const ConstDDSList& datasetsInOrder)
{
    ConstDDSList::const_iterator endIt = datasetsInOrder.end();
    ConstDDSList::const_iterator it;
    for (it = datasetsInOrder.begin(); it != endIt; ++it) {
        unionAllVariablesInto(pOutputUnion, *(*it));
    }
}

void AggregationUtil::unionAllVariablesInto(libdap::DDS* pOutputUnion, const libdap::DDS& fromDDS, bool add_at_top)
{
    DDS& dds = const_cast<DDS&>(fromDDS); // semantically const
    DDS::Vars_iter endIt = dds.var_end();
    DDS::Vars_iter it;
    for (it = dds.var_begin(); it != endIt; ++it) {
        BaseType* var = *it;
        if (var) {
            bool addedVar = addCopyOfVariableIfNameIsAvailable(pOutputUnion, *var, add_at_top);
            if (addedVar) {
                BESDEBUG("ncml", "Variable name=" << var->name() << " wasn't in the union yet and was added." << endl);
            }
            else {
                BESDEBUG("ncml",
                    "Variable name=" << var->name() << " was already in the union and was skipped." << endl);
            }
        }
    }
}

// This method is used to 'initialize' a new JoinExisting aggregation so that
// A set of Coordinate Variables (CVs) will be inserted _in the order they are
// listed_ in the .ncml file.
void AggregationUtil::resetCVInsertionPosition()
{
    //cerr << "Called resetCVInsertionPosition" << endl;
    d_last_added_cv_position = 0;
}

bool AggregationUtil::addCopyOfVariableIfNameIsAvailable(libdap::DDS* pOutDDS, const libdap::BaseType& varProto,
    bool add_at_top)
{
    bool ret = false;
    BaseType* existingVar = findVariableAtDDSTopLevel(*pOutDDS, varProto.name());
    if (!existingVar) {
        // Add the var.   add_var does a clone, so we don't need to.
        BESDEBUG("ncml2", "AggregationUtil::addCopyOfVariableIfNameIsAvailable: " << varProto.name() << endl);
        if (add_at_top) {
            // This provides a way to remember where the last CV was inserted and adds
            // this one after it. That provides the behavior that all of the CVs are
            // added at the beginning of the DDS but in the order they appear in the NCML.
            // That will translate into a greater chance of success for users, I think ...
            //
            // See also similar code in AggregationElement::createAndAddCoordinateVariableForNewDimensio
            // jhrg 10/17/11
            //cerr << "d_last_added_cv_position: " << d_last_added_cv_position << endl;
            DDS::Vars_iter pos = pOutDDS->var_begin() + d_last_added_cv_position;

            pOutDDS->insert_var(pos, const_cast<BaseType*>(&varProto));

            ++d_last_added_cv_position;
        }
        else {
            pOutDDS->add_var(const_cast<BaseType*>(&varProto));
        }

        ret = true;
    }
    return ret;
}

void AggregationUtil::addOrReplaceVariableForName(libdap::DDS* pOutDDS, const libdap::BaseType& varProto)
{
    BaseType* existingVar = findVariableAtDDSTopLevel(*pOutDDS, varProto.name());

    // If exists, nuke it.
    if (existingVar) {
        pOutDDS->del_var(varProto.name());
    }

    // Add the var.   add_var does a clone, so we don't need to clone it here.
    pOutDDS->add_var(const_cast<BaseType*>(&varProto));
}

libdap::BaseType*
AggregationUtil::findVariableAtDDSTopLevel(const libdap::DDS& dds_const, const string& name)
{
    BaseType* ret = 0;
    DDS& dds = const_cast<DDS&>(dds_const); // semantically const
    DDS::Vars_iter endIt = dds.var_end();
    DDS::Vars_iter it;
    for (it = dds.var_begin(); it != endIt; ++it) {
        BaseType* var = *it;
        if (var && var->name() == name) {
            ret = var;
            break;
        }
    }
    return ret;
}

template<class LibdapType>
LibdapType*
AggregationUtil::findTypedVariableAtDDSTopLevel(const libdap::DDS& dds, const string& name)
{
    BaseType* pBT = findVariableAtDDSTopLevel(dds, name);
    if (pBT) {
        return dynamic_cast<LibdapType*>(pBT);
    }
    else {
        return 0;
    }
}

#if 0
void AggregationUtil::produceOuterDimensionJoinedArray(Array* pJoinedArray, const std::string& joinedArrayName,
    const std::string& newOuterDimName, const std::vector<libdap::Array*>& fromVars, bool copyData)
{
    string funcName = "AggregationUtil::createOuterDimensionJoinedArray:";

    NCML_ASSERT_MSG(fromVars.size() > 0, funcName + "Must be at least one Array in input!");

    // uses the first one as template for type and shape
    if (!validateArrayTypesAndShapesMatch(fromVars, true)) {
        throw AggregationException(
            funcName + " The input arrays must all have the same data type and dimensions but do not!");
    }

    // The first will be used to "set up" the pJoinedArray
    Array* templateArray = fromVars[0];
    VALID_PTR(templateArray);
    BaseType* templateVar = templateArray->var();
    NCML_ASSERT_MSG(templateVar, funcName + "Expected a non-NULL prototype BaseType in the first Array!");

    // Set the template var for the type.
    pJoinedArray->add_var(templateVar);
    // and force the name to be the desired one, not the prototype's
    pJoinedArray->set_name(joinedArrayName);

    // Copy the attribute table from the template over...  We're not merging or anything.
    pJoinedArray->set_attr_table(templateArray->get_attr_table());

    // Create a new outer dimension based on the number of inputs we have.
    // These append_dim calls go left to right, so we need to add the new dim FIRST.
    pJoinedArray->append_dim(fromVars.size(), newOuterDimName);

    // Use the template to add inner dimensions to the new array
    for (Array::Dim_iter it = templateArray->dim_begin(); it != templateArray->dim_end(); ++it) {
        int dimSize = templateArray->dimension_size(it);
        string dimName = templateArray->dimension_name(it);
        pJoinedArray->append_dim(dimSize, dimName);
    }

    if (copyData) {
        // Make sure we have capacity for the full length of the up-ranked shape.
        pJoinedArray->reserve_value_capacity(pJoinedArray->size());
        // Glom the data together in
        joinArrayData(pJoinedArray, fromVars, false, // we already reserved the space
            true); // but please clear the Vector buffers after you use each Array in fromVars to help on memory.
    }
}
#endif

bool AggregationUtil::validateArrayTypesAndShapesMatch(const std::vector<libdap::Array*>& arrays,
    bool enforceMatchingDimNames)
{
    NCML_ASSERT(arrays.size() > 0);
    bool valid = true;
    Array* pTemplate = 0;
    for (vector<Array*>::const_iterator it = arrays.begin(); it != arrays.end(); ++it) {
        // Set the template from the first one.
        if (!pTemplate) {
            pTemplate = *it;
            VALID_PTR(pTemplate);
            continue;
        }

        valid = (valid && doTypesMatch(*pTemplate, **it) && doShapesMatch(*pTemplate, **it, enforceMatchingDimNames));
        // No use continuing
        if (!valid) {
            break;
        }
    }
    return valid;
}

bool AggregationUtil::doTypesMatch(const libdap::Array& lhsC, const libdap::Array& rhsC)
{
    // semantically const
    Array& lhs = const_cast<Array&>(lhsC);
    Array& rhs = const_cast<Array&>(rhsC);
    return (lhs.var() && rhs.var() && lhs.var()->type() == rhs.var()->type());
}

bool AggregationUtil::doShapesMatch(const libdap::Array& lhsC, const libdap::Array& rhsC, bool checkDimNames)
{
    // semantically const
    Array& lhs = const_cast<Array&>(lhsC);
    Array& rhs = const_cast<Array&>(rhsC);

    // Check the number of dims matches first.
    bool valid = true;
    if (lhs.dimensions() != rhs.dimensions()) {
        valid = false;
    }
    else {
        // Then iterate on both in sync and compare.
        Array::Dim_iter rhsIt = rhs.dim_begin();
        for (Array::Dim_iter lhsIt = lhs.dim_begin(); lhsIt != lhs.dim_end(); (++lhsIt, ++rhsIt)) {
            valid = (valid && (lhs.dimension_size(lhsIt) == rhs.dimension_size(rhsIt)));

            if (checkDimNames) {
                valid = (valid && (lhs.dimension_name(lhsIt) == rhs.dimension_name(rhsIt)));
            }
        }
    }
    return valid;
}

unsigned int AggregationUtil::collectVariableArraysInOrder(std::vector<Array*>& varArrays,
    const std::string& collectVarName, const ConstDDSList& datasetsInOrder)
{
    unsigned int count = 0;
    ConstDDSList::const_iterator endIt = datasetsInOrder.end();
    ConstDDSList::const_iterator it;
    for (it = datasetsInOrder.begin(); it != endIt; ++it) {
        DDS* pDDS = const_cast<DDS*>(*it);
        VALID_PTR(pDDS);
        Array* pVar = dynamic_cast<Array*>(findVariableAtDDSTopLevel(*pDDS, collectVarName));
        if (pVar) {
            varArrays.push_back(pVar);
            count++;
        }
    }
    return count;
}

bool AggregationUtil::couldBeCoordinateVariable(BaseType* pBT)
{
    Array* pArr = dynamic_cast<Array*>(pBT);
    if (pArr && (pArr->dimensions() == 1)) {
        // only one dimension, so grab the first and make sure we only got one.
        Array::Dim_iter it = pArr->dim_begin();
        bool matches = (pArr->dimension_name(it) == pArr->name());
        NCML_ASSERT_MSG((++it == pArr->dim_end()),
            "Logic error: NCMLUtil::isCoordinateVariable(): expected one dimension from Array, but got more!");
        return matches;
    }
    else {
        return false;
    }
}

#if 0
void AggregationUtil::joinArrayData(Array* pAggArray, const std::vector<Array*>& varArrays,
    bool reserveStorage/*=true*/, bool clearDataAfterUse/*=false*/)
{
    // Make sure we get a pAggArray with a type var we can deal with.
    VALID_PTR(pAggArray->var());
    NCML_ASSERT_MSG(pAggArray->var()->is_simple_type(),
        "AggregationUtil::joinArrayData: the output Array is not of a simple type!  Can't aggregate!");

    // If the caller wants us to do it, sum up size() and reserve that much.
    if (reserveStorage) {
        // Figure it how much we need...
        unsigned int totalLength = 0;
        {
            vector<Array*>::const_iterator it;
            vector<Array*>::const_iterator endIt = varArrays.end();
            for (it = varArrays.begin(); it != endIt; ++it) {
                Array* pArr = *it;
                if (pArr) {
                    totalLength += pArr->size();
                }
            }
        }
        pAggArray->reserve_value_capacity(totalLength);
    }

    // For each Array, make sure it's read in and copy its data into the output.
    unsigned int nextElt = 0;  // keeps track of where we are to write next in the output
    vector<Array*>::const_iterator it;
    vector<Array*>::const_iterator endIt = varArrays.end();
    for (it = varArrays.begin(); it != endIt; ++it) {
        Array* pArr = *it;
        VALID_PTR(pArr);
        NCML_ASSERT_MSG(pArr->var() && (pArr->var()->type() == pAggArray->var()->type()),
            "AggregationUtil::joinArrayData: one of the arrays to join has different type than output!  Can't aggregate!");

        // Make sure it was read in...
        if (!pArr->read_p()) {
            pArr->read();
        }

        // Copy it in with the Vector call and update our location
        nextElt += pAggArray->set_value_slice_from_row_major_vector(*pArr, nextElt);

        if (clearDataAfterUse) {
            pArr->clear_local_data();
        }
    }

    // That's all folks!
}
#endif

//////// Mnemonic for below calls....
//    struct dimension
//        {
//            int size;  ///< The unconstrained dimension size.
//            string name;    ///< The name of this dimension.
//            int start;  ///< The constraint start index
//            int stop;  ///< The constraint end index
//            int stride;  ///< The constraint stride
//            int c_size;  ///< Size of dimension once constrained
//        };

/** Print out the dimensions name and size for the given Array into os */
void AggregationUtil::printDimensions(std::ostream& os, const libdap::Array& fromArray)
{
    os << "Array dimensions: " << endl;
    Array& theArray = const_cast<Array&>(fromArray);
    Array::Dim_iter it;
    Array::Dim_iter endIt = theArray.dim_end();
    for (it = theArray.dim_begin(); it != endIt; ++it) {
        Array::dimension d = *it;
        os << "Dim = {" << endl;
        os << "name=" << d.name << endl;
        os << "size=" << d.size << endl;
        os << " }" << endl;
    }
    os << "End Array dimensions." << endl;
}

void AggregationUtil::printConstraints(std::ostream& os, const Array& rcArray)
{
    os << "Array constraints: " << endl;
    Array& theArray = const_cast<Array&>(rcArray);
    Array::Dim_iter it;
    Array::Dim_iter endIt = theArray.dim_end();
    for (it = theArray.dim_begin(); it != endIt; ++it) {
        Array::dimension d = *it;
        os << "Dim = {" << endl;
        os << "name=" << d.name << endl;
        os << "start=" << d.start << endl;
        os << "stride=" << d.stride << endl;
        os << "stop=" << d.stop << endl;
        os << " }" << endl;
    }
    os << "End Array constraints" << endl;
}

void AggregationUtil::printConstraintsToDebugChannel(const std::string& debugChannel, const libdap::Array& fromArray)
{
    ostringstream oss;
    BESDEBUG(debugChannel, "Printing constraints for Array: " << fromArray.name() << ": " << oss.str() << endl);
    AggregationUtil::printConstraints(oss, fromArray);
    BESDEBUG(debugChannel, oss.str() << endl);
}

void AggregationUtil::transferArrayConstraints(Array* pToArray, const Array& fromArrayConst, bool skipFirstFromDim,
    bool skipFirstToDim, bool printDebug /* = false */, const std::string& debugChannel /* = "agg_util" */)
{
    VALID_PTR(pToArray);
    Array& fromArray = const_cast<Array&>(fromArrayConst);

    // Make sure there's no constraints on output.  Shouldn't be, but...
    pToArray->reset_constraint();

    // Ensure the dimensionalities will work out.
    int skipDelta = ((skipFirstFromDim) ? (1) : (0));
    // If skipping output as well, subtract out the delta.
    // If we go negative, also an error.
    if (skipFirstToDim) {
        skipDelta -= 1;
    }
    if (skipDelta < 0 || (pToArray->dimensions() + skipDelta != const_cast<Array&>(fromArrayConst).dimensions())) {
        throw AggregationException("AggregationUtil::transferArrayConstraints: "
            "Mismatched dimensionalities!");
    }

    if (printDebug) {
        BESDEBUG(debugChannel,
            "Printing constraints on fromArray name= " << fromArray.name() << " before transfer..." << endl);
        printConstraintsToDebugChannel(debugChannel, fromArray);
    }

    // Only real way to the constraints is with the iterator,
    // so we'll iterator on the fromArray and move
    // to toarray iterator in sync.
    Array::Dim_iter fromArrIt = fromArray.dim_begin();
    Array::Dim_iter fromArrEndIt = fromArray.dim_end();
    Array::Dim_iter toArrIt = pToArray->dim_begin();
    for (; fromArrIt != fromArrEndIt; ++fromArrIt) {
        if (skipFirstFromDim && (fromArrIt == fromArray.dim_begin())) {
            // If we skip first to array as well, increment
            // before the next call.
            if (skipFirstToDim) {
                ++toArrIt;
            }
            continue;
        }
// If aggregates with renaming dimensions do not match each other. SK 07/26/18
//        NCML_ASSERT_MSG(fromArrIt->name == toArrIt->name, "GAggregationUtil::transferArrayConstraints: "
//            "Expected the dimensions to have the same name but they did not.");
        pToArray->add_constraint(toArrIt, fromArrIt->start, fromArrIt->stride, fromArrIt->stop);
        ++toArrIt;
    }

    if (printDebug) {
        BESDEBUG(debugChannel, "Printing constrains on pToArray after transfer..." << endl);
        printConstraintsToDebugChannel(debugChannel, *pToArray);
    }
}

BaseType*
AggregationUtil::getVariableNoRecurse(const libdap::DDS& ddsConst, const std::string& name)
{
    BaseType* ret = 0;
    DDS& dds = const_cast<DDS&>(ddsConst); // semantically const
    DDS::Vars_iter endIt = dds.var_end();
    DDS::Vars_iter it;
    for (it = dds.var_begin(); it != endIt; ++it) {
        BaseType* var = *it;
        if (var && var->name() == name) {
            ret = var;
            break;
        }
    }
    return ret;
}

// Ugh cut and pasted from the other...
// TODO REFACTOR DDS and Constructor really need a common abstract interface,
// like IVariableContainer that declares the iterators and associated methods.
BaseType*
AggregationUtil::getVariableNoRecurse(const libdap::Constructor& varContainerConst, const std::string& name)
{
    BaseType* ret = 0;
    Constructor& varContainer = const_cast<Constructor&>(varContainerConst); // semantically const
    Constructor::Vars_iter endIt = varContainer.var_end();
    Constructor::Vars_iter it;
    for (it = varContainer.var_begin(); it != endIt; ++it) {
        BaseType* var = *it;
        if (var && var->name() == name) {
            ret = var;
            break;
        }
    }
    return ret;
}

/*static*/
Array*
AggregationUtil::getAsArrayIfPossible(BaseType* pBT)
{
    if (!pBT) {
        return 0;
    }

    // After switch():
    // if Array, will be cast to Array.
    // if Grid, will be cast data Array member of Grid.
    // Other types, will be null.
    libdap::Array* pArray(0);
    switch (pBT->type()) {
    case libdap::dods_array_c:
        pArray = static_cast<libdap::Array*>(pBT);
        break;

    case libdap::dods_grid_c:
        pArray = static_cast<Grid*>(pBT)->get_array();
        break;

    default:
        pArray = 0;
        break;
    }
    return pArray;
}

const Array*
AggregationUtil::findMapByName(const libdap::Grid& inGrid, const string& findName)
{
    Grid& grid = const_cast<Grid&>(inGrid);
    Array* pRet = 0;
    Grid::Map_iter it;
    Grid::Map_iter endIt = grid.map_end();
    for (it = grid.map_begin(); it != endIt; ++it) {
        if ((*it)->name() == findName) {
            pRet = static_cast<Array*>(*it);
            break;
        }
    }
    return pRet;
}

Array* AggregationUtil::readDatasetArrayDataForAggregation(const Array& constrainedTemplateArray,
    const std::string& varName, AggMemberDataset& dataset, const ArrayGetterInterface& arrayGetter,
    const std::string& debugChannel)
{
    BES_STOPWATCH_START(MODULE, prolog_au + "Timing");

    const libdap::DDS* pDDS = dataset.getDDS();
    NCML_ASSERT_MSG(pDDS, "GridAggregateOnOuterDimension::read(): Got a null DataDDS "
        "while loading dataset = " + dataset.getLocation());

    // Grab the Array from the DataDDS with the getter
    Array* pDatasetArray = arrayGetter.readAndGetArray(varName, *pDDS, &constrainedTemplateArray, debugChannel);
    NCML_ASSERT_MSG(pDatasetArray, "In aggregation member dataset, failed to get the array! "
        "Dataset location = " + dataset.getLocation());

    // Make sure that the data was read in or I dunno what went on.
    if (!pDatasetArray->read_p()) {
        NCML_ASSERT_MSG(pDatasetArray->read_p(),
            "AggregationUtil::addDatasetArrayDataToAggregationOutputArray: pDatasetArray was not read_p()!");
    }

    // Make sure it matches the prototype or somthing went wrong
    if (!AggregationUtil::doTypesMatch(constrainedTemplateArray, *pDatasetArray)) {
        throw AggregationException(
            "Invalid aggregation! "
                "AggregationUtil::addDatasetArrayDataToAggregationOutputArray: "
                "We found the aggregation variable name=" + varName
                + " but it was not of the same type as the prototype variable!");
    }

    // Make sure the subshapes match! (true means check dimension names too... debate this)
    if (!AggregationUtil::doShapesMatch(constrainedTemplateArray, *pDatasetArray, true)) {
        throw AggregationException(
            "Invalid aggregation! "
                "AggregationUtil::addDatasetArrayDataToAggregationOutputArray: "
                "We found the aggregation variable name=" + varName
                + " but it was not of the same shape as the prototype!");
    }

    // Make sure the length of the data array also matches the proto
    if (constrainedTemplateArray.length() != pDatasetArray->length()) {
        NCML_ASSERT_MSG(constrainedTemplateArray.length() == pDatasetArray->length(),
            "AggregationUtil::addDatasetArrayDataToAggregationOutputArray: "
                "The prototype array and the loaded dataset array size()'s were not equal, even "
                "though their shapes matched. Logic problem.");
    }

    return pDatasetArray;
}

void AggregationUtil::addDatasetArrayDataToAggregationOutputArray(libdap::Array& oOutputArray, unsigned int atIndex,
    const Array& constrainedTemplateArray, const std::string& varName, AggMemberDataset& dataset,
    const ArrayGetterInterface& arrayGetter, const std::string& debugChannel)
{
    BES_STOPWATCH_START(MODULE, prolog_au + "Timing");

    libdap::Array* pDatasetArray = readDatasetArrayDataForAggregation(constrainedTemplateArray, varName, dataset, arrayGetter,
        debugChannel);

    // FINALLY, we get to stream the data!
    oOutputArray.set_value_slice_from_row_major_vector(*pDatasetArray, atIndex);

    // Now that we have copied it - let the memory go! Free! Let the bytes be freed! - ndp 08/12/2015
    pDatasetArray->clear_local_data();
}

void AggregationUtil::gatherMetadataChangesFrom(BaseType* pIntoVar, const BaseType& fromVarC)
{
    BaseType& fromVar = const_cast<BaseType&>(fromVarC); //semantic const
    // The output will end up here.
    AttrTable finalAT;

    // First, seed it with the changes in the fromVar.
    unionAttrsInto(&finalAT, fromVar.get_attr_table());

    // Then union in the items from the to var
    unionAttrsInto(&finalAT, pIntoVar->get_attr_table());

    // HACK BUG In the set_attr_table call through AttrTable operator=
    // means we keep bad memory around.  Workaround until fixed!
    pIntoVar->get_attr_table().erase();

    // Finally, replace the output var's table with the constructed one!
    pIntoVar->set_attr_table(finalAT);
}

} // namespace agg_util
