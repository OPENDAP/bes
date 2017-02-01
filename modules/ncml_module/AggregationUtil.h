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
#ifndef __AGG_UTIL__AGGREGATION_UTIL_H__
#define __AGG_UTIL__AGGREGATION_UTIL_H__

#include <AttrTable.h>
#include <iostream>
#include <string>
#include <vector>

namespace libdap {
class Array;
class BaseType;
class Constructor;
class DDS;
class Grid;
}

namespace agg_util {
class AggMemberDataset;
struct Dimension;   // jhrg 4/16/14 class Dimension;
}

namespace agg_util {

/**
 * Helper class hierarchy for acquiring variable of a certain type
 * from a DDS.  Will be passed into a
 * static helper function within AggregationUtil.
 */

/** Interface class for the functor */
struct ArrayGetterInterface {
    virtual ~ArrayGetterInterface();

    /** Virtual constructor idiom */
    virtual ArrayGetterInterface* clone() const = 0;

    /** Find, constrain, read, and return an Array inside the DDS
     * using the given name information (somehow, as specified in subclasses)
     *
     * NOTE: this is a functor so sbuclasses may do searches,
     * may dig into Grid's, etc in order to find the item it needs.
     *
     * @param name  name information on finding the correct Array
     *          note: this need not be the exact FQN, but can be
     *          interpreted by subclasses as "name.name" in the
     *          case of a Grid array, e.g.
     * @param dds  the DDS to search for the Array
     * @param pConstraintTemplate  if not NULL, use this Array
     *            as a template from which to copy constraints
     *            onto the returned Array prior to read.
     * @param debugChannel  if !empty(), the channel to print
     *            out debug information.
     *  */
    virtual libdap::Array* readAndGetArray(const std::string& name, const libdap::DDS& dds,
        const libdap::Array* const pConstraintTemplate, const std::string& debugChannel) const = 0;
};
// class ArrayGetterInterface

/** Concrete impl that simply finds the
 * Array by looking for a variable of the
 * given name at the top level of the DDS
 * (i.e. doesn't recurse using field notation!)
 * and making sure it
 * is of the proper type.
 */
struct TopLevelArrayGetter: public ArrayGetterInterface {
    TopLevelArrayGetter();
    virtual ~TopLevelArrayGetter();

    virtual TopLevelArrayGetter* clone() const;

    /** Lookup name within the top level of DDS ONLY and
     * return it as an Array* if it is a subclass of Array.
     * May return NULL if not found or other problem.
     * @throw AggregationException if not found or if found but
     *          cannot be cast properly into an Array*
     */
    virtual libdap::Array* readAndGetArray(const std::string& name, const libdap::DDS& dds,
        const libdap::Array* const pConstraintTemplate, const std::string& debugChannel) const;
};
// class TopLevelArrayGetter

struct TopLevelGridDataArrayGetter: public ArrayGetterInterface {
    TopLevelGridDataArrayGetter();
    virtual ~TopLevelGridDataArrayGetter();

    virtual TopLevelGridDataArrayGetter* clone() const;

    /** Find the array as the data Array of a
     * TOP-LEVEL Grid with name.  Therefore,
     * the Array would have name "name.name" as an
     * qualified name w.r.t. the DDS.
     * @throw AggregationException if not found or illegal shape.
     * @param name name of the Grid to find the Array in.
     * @param dds  DDS to search
     * @param pConstraintTemplate  template for constraints
     * @param debugChannel if !empty() the channel to print to.
     * @return the read in Array* or 0 if not found.
     */
    virtual libdap::Array* readAndGetArray(const std::string& name, const libdap::DDS& dds,
        const libdap::Array* const pConstraintTemplate, const std::string& debugChannel) const;
};
// class TopLevelGridDataArrayGetter

struct TopLevelGridMapArrayGetter: public ArrayGetterInterface {
    TopLevelGridMapArrayGetter(const std::string& gridName);
    virtual ~TopLevelGridMapArrayGetter();

    virtual TopLevelGridMapArrayGetter* clone() const;

    /**
     * Find's the Array using name as the name of a map to find
     * within a top level Grid names in the constructor!
     * @throw AggregationException if not found or illegal shape.
     * @param name name of the Grid to find the Array in.
     * @param dds  DDS to search
     * @param pConstraintTemplate  template for constraints
     * @param debugChannel if !empty() the channel to print to.
     * @return the read in Array* or 0 if not found.
     */
    virtual libdap::Array* readAndGetArray(const std::string& name, const libdap::DDS& dds,
        const libdap::Array* const pConstraintTemplate, const std::string& debugChannel) const;

    // The name of the Grid within which the desired map is contained.
    const string _gridName;
};
// class TopLevelGridMapArrayGetter

/**
 *   A static class for encapsulating the aggregation functionality on libdap.
 *   This class should have references to libdap and STL, but should NOT
 *   contain any references to other ncml_module classes.  This will allow us to
 *   potentially package the aggregation functionality into its own lib
 *   or perhaps roll it into libdap.
 */
class AggregationUtil {
private:
    // This is a static class for now...
    AggregationUtil()
    {
    }
    ~AggregationUtil()
    {
    }

    static int d_last_added_cv_position;

public:

    // Typedefs
    typedef std::vector<const libdap::DDS*> ConstDDSList;

    /**
     * Perform an NCML-type union aggregation on the datasets in datasetsInOrder into pOutputUnion.
     * The first named instance of a variable or attribute in a forward iteration of datasetsInOrder
     * ends up in pOutputUnion.  Not that named containers are considered as a unit, so we do not recurse into
     * their children.
     */
    static void performUnionAggregation(libdap::DDS* pOutputUnion, const ConstDDSList& datasetsInOrder);

    /**
     * Merge any attributes in tableToMerge whose names do not already exist within *pOut into pOut.
     * @param pOut the table to merge into.  On exit it will contain its previous contents plus any new attributes
     *        from fromTable
     * @param fromTable
     */
    static void unionAttrsInto(libdap::AttrTable* pOut, const libdap::AttrTable& fromTable);

    /**
     *  Lookup the attribute with given name in inTable and place a reference in attr.
     *  @return whether it was found and attr is validly effected.
     * */
    static bool findAttribute(const libdap::AttrTable& inTable, const string& name, libdap::AttrTable::Attr_iter& attr);

    /**
     * For each variable within the top level of each dataset in datasetsInOrder (forward iteration)
     * add a _clone_ of it to pOutputUnion if a variable with the same name does not exist in
     * pOutputUnion yet.
     */
    static void unionAllVariablesInto(libdap::DDS* pOutputUnion, const ConstDDSList& datasetsInOrder);

    /**
     * Used to reset the class field that tracks where Coordinate Variables (CVs) have been
     * inserted into the DDS. This helps ensure that the CVs appear in the order they were
     * listed in the .ncml file.
     */
    static void resetCVInsertionPosition();

    /**
     * For each variable in fromDDS top level, union it into pOutputUnion if a variable with the same name isn't already there
     * @see addCopyOfVariableIfNameIsAvailable().
     */
    static void unionAllVariablesInto(libdap::DDS* pOutputUnion, const libdap::DDS& fromDDS, bool add_at_top = false);

    /**
     * If a variable does not exist within pOutDDS (top level) with the same name as varProto,
     * then place a clone of varProto (using virtual ctor ptr_duplicate) into pOutDDS.
     *
     * @return whether pOutDDS changed (ie name was free).
     */
    static bool addCopyOfVariableIfNameIsAvailable(libdap::DDS* pOutDDS, const libdap::BaseType& varProto,
        bool add_at_top = false);

    /**
     * If a variable with the name varProto.name() doesn't exist, add a copy of varProto to
     * pOutDDS.
     * If the variable already exists, REPLACE it with a copy of varProto.
     * @param pOutDDS the DDS to change
     * @param varProto prototype to clone and add to pOutDDS.
     */
    static void addOrReplaceVariableForName(libdap::DDS* pOutDDS, const libdap::BaseType& varProto);

    /**
     * Find a variable with name at the top level of the DDS.
     * Do not recurse.  DDS::var() will look inside containers, so we can't use that.
     * @return the variable within dds or null if not found.
     */
    static libdap::BaseType* findVariableAtDDSTopLevel(const libdap::DDS& dds, const string& name);

    /**
     * Template wrapper for findVariableAtDDSTopLevel() which
     * does the find but only return non-NULL if the found
     * BaseType* can be dynamically cast to template type LibdapType.
     * @param dds the dds to search
     * @param name the name of the variable to find
     * @return the pointer to the found object if it's dynamically castable
     *         to LibdapType*, else NULL.
     * @see findVariableAtDDSTopLevel()
     */
    template<class LibdapType> static LibdapType* findTypedVariableAtDDSTopLevel(const libdap::DDS& dds,
        const string& name);

#if 0
    /**
     *  Basic joinNew aggregation into pJoinedArray on the array of inputs fromVars.
     *
     *  If all the Arrays in fromVars are rank D, then the joined Array will be rank D+1
     *  with the new outer dimension with dimName which will be of cardinality fromVars.size().
     *  The row major order of the data in the new Array will be assumed in the order the fromVars are given.
     *
     *  Data values will ONLY be copied if copyData is set.
     *
     *  fromVars[0] will be used as the "prototype" in terms of attribute metadata, so only its Attributes
     *  will be copied into the resulting new Array.
     *
     *  Errors:
     *  The shape and type of all vars in fromVars MUST match each other or an exception is thrown!
     *  There must be at least one array in fromVars or an exception is thrown.
     *
     * @param pJoinedArray      the array to make into the new array.  It is assumed to have no dimensions
     *                          to start and should be an empty prototype.
     *
     * @param joinedArrayName   the name of pJoinedArray will be set to this
     * @param newOuterDimName   the name of the new outer dimension will be set to this
     * @param fromVars          the input Array's the first of which will be the "template"
     * @param copyData          whether to copy the data from the Array's in fromVars or just create the shape.
     */
    static void produceOuterDimensionJoinedArray(libdap::Array* pJoinedArray, const std::string& joinedArrayName,
        const std::string& newOuterDimName, const std::vector<libdap::Array*>& fromVars, bool copyData);
#endif

    /**
     * Scan all the arrays in _arrays_ using the first as a template
     * and make sure that they all have the same data type and they all
     * have the same dimensions.  (NOTE: we only use the sizes to
     * validate dimensions, not the "name", unless enforceMatchingDimNames is set)
     */
    static bool validateArrayTypesAndShapesMatch(const std::vector<libdap::Array*>& arrays,
        bool enforceMatchingDimNames);

    /** Do the lhs and rhs have the same data type? */
    static bool doTypesMatch(const libdap::Array& lhs, const libdap::Array& rhs);

    /** Do the lhs and rhs have the same shapes?  Only use size for dimension compares unless
     * checkDimNames
     */
    static bool doShapesMatch(const libdap::Array& lhs, const libdap::Array& rhs, bool checkDimNames);

    /**
     * Fill in varArrays with Array's named collectVarName from each DDS in datasetsInOrder, in that order.
     * In other words, the ordering found in datasetsInOrder will be preserved in varArrays.
     * @return the number of variables added to varArrays.
     * @param varArrays  the array to push the found variables onto.  Will NOT be cleared, so can be added to.
     * @param collectVarName  the name of the variable to find at top level DDS in datasetsInOrder
     * @param datasetsInOrder the datasets to search for the Array's within.
     */
    static unsigned int collectVariableArraysInOrder(std::vector<libdap::Array*>& varArrays,
        const std::string& collectVarName, const ConstDDSList& datasetsInOrder);

    /**
     * @return whether this is a 1D Array whose dimension name is the same as its variable name.
     * We consider this to define a "coordinate variable" in the sense of an NCML dataset
     * and will use it as a Grid map vector.
     */
    static bool couldBeCoordinateVariable(libdap::BaseType* pBT);

#if 0
    /**
     * Copy the simple type data Vector for each Array in varArrays into pAggArray, sequentially,
     * effectively appending all the row major data in each entry in varArray into the row major order
     * of pAggArray.
     *
     * If the data in varArray's has not been read, it calls read() on each Array first.

     * If reserveStorage is set, pAggArray will first have enough its capacity reserved to store all the data within varArray's
     * (the sum of all lengths).  This should be false if the caller already reserved the proper capacity.
     *
     * @param pAggArray the output to place the appended data
     * @param varArrays the Array's whose data is to be copied into pAggArray, in the order of the vector.
     * @param reserveStorage  if true, sets the capacity of pAggArray to be enough to contain all the elements in varArrays.
     *                        Note: this might not match the length of the ouput, so the caller really should do this!
     * @param clearDayaAfterUse  if true, Vector::clear_local_data() will be called on each member of varArray's after it is copied into the output.
     *                           this should tighten up memory if its known the data will no longer be needed.
     *
     * @exception if any Array in varArray's does not have the same element type as pAggArray.
     * @exception if there is not enough storage in pAggArray to collect all the data.
     * @exception on any problems will a read() call on any Array in varArrays.
     */
    static void joinArrayData(libdap::Array* pAggArray, const std::vector<libdap::Array*>& varArrays,
        bool reserveStorage = true, bool clearDataAfterUse = false);
#endif

    /** Print out the dimensions name and size for the given Array into os */
    static void printDimensions(std::ostream& os, const libdap::Array& fromArray);

    /**
     * Stream out the current constraints for all the dimensions in the Array.
     * @param os  the output stream
     * @param fromArray the array whose dimensions to print.
     */
    static void printConstraints(std::ostream& os, const libdap::Array& fromArray);

    /** Output using BESDEBUG to the debugChannel channel.
     * Prints the constraints on the dimensions of fromArray.
     * @param debugChannel name of the output channel
     * @param fromArray  the Array whose constraints should be printed to the debugChannel
     */
    static void printConstraintsToDebugChannel(const std::string& debugChannel, const libdap::Array& fromArray);

    /**
     * Copy the constraints from the from Array into the pToArray
     * in Dim_iter order.
     *
     * if skipFirstFromDim, the first dimension of fromArray will be skipped,
     * for the case of copying from a joinNew aggregated array to a
     * granule subset array
     *
     * if skipFirstToDim the first dimension of toArray will be skipped,
     * for the case where presumably both first dims are skipped for a
     * joinExisting aggregation where constraints on outer dim will be
     * calculated by the caller.
     *
     * @param pToArray array to put constraints into
     * @param fromArray array to take constraints from
     * @param skipFirstFromDim whether the first dim of fromArray is aggregated and
     *                  should be skipped.
     * @param skipFirstToDim whether the first dim of toArray is aggregated and
     *                  should be skipped.
     */
    static void transferArrayConstraints(libdap::Array* pToArray, const libdap::Array& fromArray, bool skipFirstFromDim,
        bool skipFirstToDim, bool printDebug = false, const std::string& debugChannel = "agg_util");

    /**
     * Return the variable in dds top level (no recursing, no fully qualified name dot notation)
     * if it exists, else 0.
     * The name IS ALLOWED to contain a dot '.', but this is interpreted as PART OF THE NAME
     * and not as a field separator!
     */
    static libdap::BaseType* getVariableNoRecurse(const libdap::DDS& dds, const std::string& name);

    /**
     * Return the variable in dds top level (no recursing, no fully qualified name dot notation)
     * if it exists, else 0.
     * The name IS ALLOWED to contain a dot '.', but this is interpreted as PART OF THE NAME
     * and not as a field separator!
     */
    static libdap::BaseType* getVariableNoRecurse(const libdap::Constructor& varContainer, const std::string& name);

    /**
     * If pBT is an Array type, cast and return it as the Array.
     * If pBT is a Grid type, return the data Array field.
     * Otherwise, return NULL.
     * @param pBT the variable to adapt
     * @return the array pointer or null if it cannot be adapted.
     */
    static libdap::Array* getAsArrayIfPossible(libdap::BaseType* pBT);

    /** Find the given map name in the given Grid and return it if found, else NULL */
    static const libdap::Array* findMapByName(const libdap::Grid& inGrid, const std::string& findName);

    /**
     * Read the data that akes up one 'slice' of an aggregation. This method is
     * used by addDatasetArrayDataToAggregationOutputArray() which calls it and
     * then transfers the data from the DAP Array that holds the slice's data
     * to the result (another DAP Array that will eventually hold the entire
     * collection of slices in a single array). It is also used by specialized
     * versions of serialize() so that data can be read and written 'slice by slice'
     * avoiding the latency of reading in all of the data before transmission
     * starts along with the storage overhead of holding all of the data in
     * memory at one time.
     *
     * @param constrainedTemplateArray
     * @param varName
     * @param dataset
     * @param arrayGetter
     * @param debugChannel
     * @return An Array variable with the slice's data
     */
    static libdap::Array* readDatasetArrayDataForAggregation(const libdap::Array& constrainedTemplateArray,
        const std::string& varName, AggMemberDataset& dataset, const ArrayGetterInterface& arrayGetter,
        const std::string& debugChannel);

    /**
     * Load the given dataset's DDS.
     * Find the aggVar of the given name in it, must be Array.
     * Transfer the constraints from the local template to it.
     * Call read() on it.
     * Stream the data into oOutputArray's output buffer
     * at the element index nextElementIndex.
     *
     * @param oOutputArray  the Array to output the data into
     * @param atIndex  where in the output buffer of rOutputArray
     *                          to stream it (note: not byte, element!)
     * @param constrainedTemplateArray  the Array to use as the template for the
     *                       constraints on loading the dataset.
     * @param name the name of the aggVar to find in the DDS
     * @param dataset the dataset to load for this element.
     * @param arrayGetter  the class to use to get the member Array by name from DDS
     * @param debugChannel if not empty(), BESDEBUG channel to use
     */
    static void addDatasetArrayDataToAggregationOutputArray(libdap::Array& oOutputArray, // output location
        unsigned int atIndex, // oOutputArray[atIndex] will be where data put
        const libdap::Array& constrainedTemplateArray, // for copying constraints
        const string& varName, // top level var to find in dataset DDS
        AggMemberDataset& dataset, // Dataset who's DDS should be searched
        const ArrayGetterInterface& arrayGetter, // alg for getting Array from DDS
        const string& debugChannel // if !"", debug output goes to this channel.
        );

    /**
     * Union fromVar's AttrTable (initially) with pIntoVar's AttrTable
     * and replace pIntoVar's AttrTable with this union.
     * Essentially uses fromVar's AttrTable as a set of changes to pIntoVar's
     * table.
     * @param pIntoVar  the var whose AttrTable is the output
     * @param fromVar the var to use as changes to the output
     */
    static void gatherMetadataChangesFrom(libdap::BaseType* pIntoVar, const libdap::BaseType& fromVar);

};
// class AggregationUtil

/**
 * For each ptr element pElt in vecToClear, delete pElt and remove it
 * from vecToClear.
 * On exit, vecToClear.empty() and all ptrs it used to contain have been delete'd.
 * @param vecToClear
 */
template<typename T>
void clearVectorAndDeletePointers(std::vector<T*>& vecToClear)
{
    while (!vecToClear.empty()) {
        T* pElt = vecToClear.back();
        delete pElt;
        vecToClear.pop_back();
    }
}

/** Assumes T has unref() */
template<class T>
void clearAndUnrefAllElements(std::vector<T*>& vecToClear)
{
    while (!vecToClear.empty()) {
        T* pElt = vecToClear.back();
        pElt->unref();
        vecToClear.pop_back();
    }
}

/** Assumes T has ref() */
template<class T>
void appendVectorOfRCObject(std::vector<T*>& intoVec, const std::vector<T*>& fromVec)
{
    class std::vector<T*>::const_iterator it;
    class std::vector<T*>::const_iterator endIt = fromVec.end();
    for (it = fromVec.begin(); it != endIt; ++it) {
        T* pElt = *it;
        if (pElt) {
            pElt->ref();
        }
        intoVec.push_back(pElt);
    }
}

}

#endif /* __AGG_UTIL__AGGREGATION_UTIL_H__ */
