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

#include <sstream>
#include <memory> // auto_ptr


#include "DataDDS.h" // libdap
#include "DDS.h" // libdap
#include "Grid.h" // libdap

#include "DDSLoader.h" // agg_util
#include "Dimension.h" // agg_util
#include "NCMLDebug.h" // ncml_module
#include "NCMLUtil.h"  // ncml_module

#include "GridAggregateOnOuterDimension.h"

#include "AggregationException.h"
#include "AggregationUtil.h" // agg_util
#include "Array.h" // libdap
#include "ArrayAggregateOnOuterDimension.h" // agg_util

using libdap::Array;
using libdap::DataDDS;
using libdap::DDS;
using libdap::Grid;

namespace agg_util {

// Local flag for whether to print constraints, to help debugging
// unused jhrg 4/16/14 static const bool PRINT_CONSTRAINTS = true;

// BES Debug output channel for this file.
static const string DEBUG_CHANNEL("ncml:2");

// Copy local data
void GridAggregateOnOuterDimension::duplicate(const GridAggregateOnOuterDimension& rhs)
{
    _newDim = rhs._newDim;
}

GridAggregateOnOuterDimension::GridAggregateOnOuterDimension(const Grid& proto, const Dimension& newDim,
    const AMDList& memberDatasets, const DDSLoader& loaderProto)
// this should give us map vectors and the member array rank (without new dim).
:
    GridAggregationBase(proto, memberDatasets, loaderProto), _newDim(newDim)
{
    BESDEBUG(DEBUG_CHANNEL, "GridAggregateOnOuterDimension() ctor called!" << endl);

    createRep(memberDatasets);
}

GridAggregateOnOuterDimension::GridAggregateOnOuterDimension(const GridAggregateOnOuterDimension& proto) :
    GridAggregationBase(proto), _newDim()
{
    BESDEBUG(DEBUG_CHANNEL, "GridAggregateOnOuterDimension() copy ctor called!" << endl);
    duplicate(proto);
}

GridAggregateOnOuterDimension*
GridAggregateOnOuterDimension::ptr_duplicate()
{
    return new GridAggregateOnOuterDimension(*this);
}

GridAggregateOnOuterDimension&
GridAggregateOnOuterDimension::operator=(const GridAggregateOnOuterDimension& rhs)
{
    if (this != &rhs) {
        cleanup();
        GridAggregationBase::operator=(rhs);
        duplicate(rhs);
    }
    return *this;
}

GridAggregateOnOuterDimension::~GridAggregateOnOuterDimension()
{
    BESDEBUG(DEBUG_CHANNEL, "~GridAggregateOnOuterDimension() dtor called!" << endl);
    cleanup();
}

/////////////////////////////////////////////////////
// Helpers

void GridAggregateOnOuterDimension::createRep(const AMDList& memberDatasets)
{
    BESDEBUG_FUNC(DEBUG_CHANNEL, "Replacing the Grid's data Array with an ArrayAggregateOnOuterDimension..." << endl);

    // This is the prototype we need.  It will have been set in the ctor.
    Array* pArr = static_cast<Array*>(array_var());
    NCML_ASSERT_MSG(pArr, "Expected to find a contained data Array but we did not!");

    // Create the Grid version of the read getter and make a new AAOOD from our state.
    std::auto_ptr<ArrayGetterInterface> arrayGetter(new TopLevelGridDataArrayGetter());

    // Create the subclass that does the work and replace our data array with it.
    // Note this ctor will prepend the new dimension itself, so we do not.
    std::auto_ptr<ArrayAggregateOnOuterDimension> aggDataArray(new ArrayAggregateOnOuterDimension(*pArr, // prototype, already should be setup properly _without_ the new dim
        memberDatasets, arrayGetter, _newDim));

    // Make sure null since sink function
    // called on the auto_ptr
    NCML_ASSERT(!(arrayGetter.get()));

    // Replace our data Array with this one.  Will delete old one and may throw.
    set_array(aggDataArray.get());

    // Release here on successful set since set_array uses raw ptr only.
    // In case we threw then auto_ptr cleans up itself.
    aggDataArray.release();
}

void GridAggregateOnOuterDimension::cleanup() throw ()
{
}

/* virtual */
void GridAggregateOnOuterDimension::transferConstraintsToSubGridHook(Grid* pSubGrid)
{
    VALID_PTR(pSubGrid);
    transferConstraintsToSubGridMaps(pSubGrid);
    transferConstraintsToSubGridArray(pSubGrid);
}

/* virtual */
const Dimension&
GridAggregateOnOuterDimension::getAggregationDimension() const
{
    return _newDim;
}

void GridAggregateOnOuterDimension::transferConstraintsToSubGridMaps(Grid* pSubGrid)
{
    BESDEBUG(DEBUG_CHANNEL, "Transferring constraints to the subgrid maps..." << endl);
    Map_iter subGridMapIt = pSubGrid->map_begin();
    for (Map_iter it = map_begin(); it != map_end(); ++it) {
        // Skip the new outer dimension
        if (it == map_begin()) {
            continue;
        }
        Array* subGridMap = static_cast<Array*>(*subGridMapIt);
        Array* superGridMap = static_cast<Array*>(*it);
        agg_util::AggregationUtil::transferArrayConstraints(subGridMap, *superGridMap, false, // skipFirstDim = false since map sizes consistent
            false, // same rank, dont skip this one either
            true, // printDebug
            DEBUG_CHANNEL); // debugChannel
        ++subGridMapIt; // keep iterators in sync
    }
}

void GridAggregateOnOuterDimension::transferConstraintsToSubGridArray(Grid* pSubGrid)
{
    BESDEBUG(DEBUG_CHANNEL, "Transferring constraints to the subgrid array..." << endl);

    Array* pSubGridArray = static_cast<Array*>(pSubGrid->get_array());
    VALID_PTR(pSubGridArray);
    Array* pThisArray = static_cast<Array*>(array_var());
    VALID_PTR(pThisArray);

    // transfer, skipping first dim which is the new one.
    agg_util::AggregationUtil::transferArrayConstraints(pSubGridArray, // into the prototype
        *pThisArray, // from the output array (with valid constraints)
        true, // skipFirstDim: need to skip since the ranks differ
        false, // but not into the to array
        true,  // printDebug
        DEBUG_CHANNEL);
}

} // namespace agg_util
