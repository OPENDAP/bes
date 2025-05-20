///////////////////////////////////////////////////////////////////////////////
// This file is part of the "NcML Module" project, a BES module designed
// to allow NcML files to be used to be used as a wrapper to add
// AIS to existing datasets of any format.
//
// Copyright (c) 2010 OPeNDAP, Inc.
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


#include <libdap/Array.h> // libdap
#include <libdap/D4Group.h>
#include <libdap/Constructor.h>
#include <libdap/D4Maps.h>
#include <libdap/InternalErr.h>

#include "BESStopWatch.h"

#include "AggregationUtil.h" // agg_util
#include "GridAggregationBase.h" // agg_util

#include "NCMLDebug.h"

using libdap::Array;
using libdap::BaseType;
using libdap::Grid;

using libdap::D4Group;
using libdap::Constructor;
using libdap::InternalErr;
using libdap::D4Maps;
using libdap::D4Map;

// Local debug flags
#define DEBUG_CHANNEL "agg_util"
#define prolog std::string("GridAggregationBase::").append(__func__).append("() - ")

namespace agg_util {
GridAggregationBase::GridAggregationBase(const libdap::Grid& proto, const AMDList& memberDatasets, const DDSLoader& loaderProto) :
    Grid(proto), _loader(loaderProto.getDHI()), _pSubGridProto(cloneSubGridProto(proto)), _memberDatasets(memberDatasets)
{
}

GridAggregationBase::GridAggregationBase(const string& name, const AMDList& memberDatasets, const DDSLoader& loaderProto) :
    Grid(name), _loader(loaderProto.getDHI()), _memberDatasets(memberDatasets)
{
}

GridAggregationBase::GridAggregationBase(const GridAggregationBase& proto) :
    Grid(proto), _loader(proto._loader.getDHI())
{
    duplicate(proto);
}

/* virtual */
GridAggregationBase::~GridAggregationBase()
{
    cleanup();
}

GridAggregationBase&
GridAggregationBase::operator=(const GridAggregationBase& rhs)
{
    if (this != &rhs) {
        cleanup();
        Grid::operator=(rhs);
        duplicate(rhs);
    }
    return *this;
}


void
GridAggregationBase::transform_to_dap4(D4Group *root, Constructor *container)
{
    Grid::transform_to_dap4(root,container);

#if 0 // I removed this method because I think the parent class implementation should work correctly.
    BaseType *btp = array_var()->transform_to_dap4(root, container);
    Array *coverage = static_cast<Array*>(btp);
    if (!coverage) throw InternalErr(__FILE__, __LINE__, "Expected an Array while transforming a Grid (coverage)");

    coverage->set_parent(container);

    // Next find the maps; add them to the coverage and to the container,
    // the latter only on the condition that they are not already there.

    for (Map_iter i = map_begin(), e = map_end(); i != e; ++i) {
        btp = (*i)->transform_to_dap4(root, container);
        Array *map = static_cast<Array*>(btp);
        if (!map) throw InternalErr(__FILE__, __LINE__, "Expected an Array while transforming a Grid (map)");

        // map must be non-null (Grids cannot contain Grids in DAP2)
        if (map) {
            // Only add the map/array if it not already present; given the scoping rules
            // for DAP2 and the assumption the DDS is valid, testing for the same name
            // is good enough.
            if (!root->var(map->name())) {
                map->set_parent(container);
                container->add_var_nocopy(map);	// this adds the array to the container
            }
            D4Map *dap4_map = new D4Map(map->name(), map, coverage);	// bind the 'map' to the coverage
            coverage->maps()->add_map(dap4_map);	// bind the coverage to the map
        }
        else {
            throw InternalErr(__FILE__, __LINE__,
                "transform_to_dap4() returned a null value where there can be no Grid.");
        }
    }

    container->add_var_nocopy(coverage);
#endif
}

void GridAggregationBase::setShapeFrom(const libdap::Grid& constProtoSubGrid, bool addMaps)
{
    // calls used are semantically const, but not syntactically.
    Grid& protoSubGrid = const_cast<Grid&>(constProtoSubGrid);

    // Save a clone of the template for read() to use.
    // We always use these maps...
    _pSubGridProto = unique_ptr<Grid>(cloneSubGridProto(protoSubGrid));

    // Pass in the data array and maps from the proto by hand.
    Array* pDataArrayTemplate = protoSubGrid.get_array();
    VALID_PTR(pDataArrayTemplate);
    set_array(static_cast<Array*>(pDataArrayTemplate->ptr_duplicate()));

    // Now the maps in order if asked
    if (addMaps) {
        Grid::Map_iter endIt = protoSubGrid.map_end();
        for (Grid::Map_iter it = protoSubGrid.map_begin(); it != endIt; ++it) {
            // have to case, the iter is for some reason BaseType*
            Array* pMap = dynamic_cast<Array*>(*it);
            VALID_PTR(pMap);
            add_map(pMap, true); // add as a copy
        }
    }
}

/* virtual */
const AMDList&
GridAggregationBase::getDatasetList() const
{
    return _memberDatasets;
}

/* virtual */
bool GridAggregationBase::read()
{
    BESDEBUG_FUNC(DEBUG_CHANNEL, prolog << "Function entered..." << endl);

    if (read_p()) {
        BESDEBUG_FUNC(DEBUG_CHANNEL, prolog << "read_p() set, early exit!");
        return true;
    }

   // Call the subclass hook methods to do this work properly
    readAndAggregateConstrainedMapsHook();

    // Now make the read call on the data array.
    // The aggregation subclass will do the right thing.
    Array* pAggArray = get_array();
    VALID_PTR(pAggArray);

    // Only do this portion if the array part is supposed to serialize!
    if (pAggArray->send_p() || pAggArray->is_in_selection()) {
        pAggArray->read();
    }

    // Set the cache bit.
    set_read_p(true);
    return true;
}

#define PIPELINING 1

/**
 * Pipelining data reads and network writes. This version of serialize(),
 * like the versions in ArrayAggregation... also here in the NCML module,
 * handles interleaved data reads and network write operations.
 *
 * @note The read() method for this class reads the entire Aggregated
 * Grid variable into memory, so code that depends on that behavior will
 * continue to work. When this serialize() code is called and the variable
 * is 'loaded' with data, the libdap::Grid::serialize() code is called,
 * so that will work as well. When this method is called and the data still
 * need to be read, the new behavior will take over and latency, from the
 * client program's perspective, will be small compared to reading then
 * entire variable and then transmitting its values.
 *
 * @param eval
 * @param dds
 * @param m
 * @param ce_eval
 * @return
 */
bool
GridAggregationBase::serialize(libdap::ConstraintEvaluator &eval, libdap::DDS &dds, libdap::Marshaller &m,
    bool ce_eval)
{
    BES_STOPWATCH_START(DEBUG_CHANNEL, prolog + "Timer");

    bool status = false;

    if (!read_p()) {
        // Call the subclass hook methods to do this work properly
        // *** Replace Map code readAndAggregateConstrainedMapsHook();

        // Transfers constraints to the proto grid and reads it
        readProtoSubGrid();

        // Make the call to serialize the data array.
        // The aggregation subclass will do the right thing.
        Array* pAggArray = get_array();
        VALID_PTR(pAggArray);

        // Only do this portion if the array part is supposed to serialize!
        if (pAggArray->send_p() || pAggArray->is_in_selection()) {
#if PIPELINING
            pAggArray->serialize(eval, dds, m, ce_eval);
#else
            pAggArray->read();
#endif
        }

        // Get the read-in, constrained maps from the proto grid and serialize them.
        // *** Replace copyProtoMapsIntoThisGrid(getAggregationDimension());

        Grid* pSubGridTemplate = getSubGridTemplate();
        VALID_PTR(pSubGridTemplate);

        Map_iter mapIt;
        Map_iter mapEndIt = map_end();
        for (mapIt = map_begin(); mapIt != mapEndIt; ++mapIt) {
            Array* pOutMap = static_cast<Array*>(*mapIt);
            VALID_PTR(pOutMap);

            // If it isn't getting dumped, then don't bother with it
            if (!(pOutMap->send_p() || pOutMap->is_in_selection())) {
                continue;
            }

            // We don't want to touch the aggregation dimension since it's
            // handled specially.
            if (pOutMap->name() == getAggregationDimension().name) {
                // Make sure it's read with these constraints.
#if PIPELINING
                pOutMap->serialize(eval, dds, m, ce_eval);
#else
                pOutMap->read();
#endif
                continue;
            }

            // Otherwise, find the map in the protogrid and copy it's data into this.
            Array* pProtoGridMap = const_cast<Array*>(AggregationUtil::findMapByName(*pSubGridTemplate, pOutMap->name()));
            NCML_ASSERT_MSG(pProtoGridMap, "Couldn't find map in prototype grid for map name=" + pOutMap->name());
            BESDEBUG_FUNC(DEBUG_CHANNEL, prolog << "Calling read() on prototype map vector name=" << pOutMap->name() << " and calling transfer constraints..." << endl);

            // Make sure the protogrid maps were properly read
            NCML_ASSERT_MSG(pProtoGridMap->read_p(), "Expected the prototype map to have been read but it wasn't.");

            // Make sure the lengths match to be sure we're not gonna blow memory up
            NCML_ASSERT_MSG(pOutMap->length() == pProtoGridMap->length(),
                "Expected the prototype and output maps to have same size() after transfer of constraints, but they were not so we can't copy the data!");

            // The dimensions will have been set up correctly now so size() is correct...
            // We assume the pProtoGridMap matches at this point as well.
            // So we can use this call to copy from one vector to the other
            // so we don't use temp storage in between
#if PIPELINING
            pProtoGridMap->serialize(eval, dds, m, ce_eval);
#else
            pOutMap->reserve_value_capacity(); // reserves mem for length
            pOutMap->set_value_slice_from_row_major_vector(*pProtoGridMap, 0);
#endif
            pOutMap->set_read_p(true);
        }

       // Set the cache bit.
        set_read_p(true);

#if PIPELINING
        status = true;
#else
    status = libdap::Grid::serialize(eval, dds, m, ce_eval);
#endif
    }
    else {
        status = libdap::Grid::serialize(eval, dds, m, ce_eval);
    }

    return status;
}

/////////////////////////////////////////
///////////// Helpers

Grid*
GridAggregationBase::getSubGridTemplate()
{
    return _pSubGridProto.get();
}

void GridAggregationBase::duplicate(const GridAggregationBase& rhs)
{
    _loader = DDSLoader(rhs._loader.getDHI());

    _pSubGridProto.reset((rhs._pSubGridProto.get()) ? (static_cast<Grid*>(rhs._pSubGridProto->ptr_duplicate())) : nullptr);

    _memberDatasets = rhs._memberDatasets;
}

void GridAggregationBase::cleanup()
{
    _loader.cleanup();

    _memberDatasets.clear();
    _memberDatasets.resize(0);
}

/* virtual */
void GridAggregationBase::readAndAggregateConstrainedMapsHook()
{
    // Transfers constraints to the proto grid and reads it
    readProtoSubGrid();

    // Copy the read-in, constrained maps from the proto grid
    // into our output maps.
    copyProtoMapsIntoThisGrid(getAggregationDimension());
}

/* static */
libdap::Grid*
GridAggregationBase::cloneSubGridProto(const libdap::Grid& proto)
{
    return static_cast<Grid*>(const_cast<Grid&>(proto).ptr_duplicate());
}

void GridAggregationBase::printConstraints(const Array& fromArray)
{
    ostringstream oss;
    AggregationUtil::printConstraints(oss, fromArray);
    BESDEBUG("ncml:2", prolog << "Constraints for Grid: " << name() << ": " << oss.str() << endl);
}

void GridAggregationBase::readProtoSubGrid()
{
    Grid* pSubGridTemplate = getSubGridTemplate();
    VALID_PTR(pSubGridTemplate);

    // Call the specialized subclass constraint transfer method
    transferConstraintsToSubGridHook(pSubGridTemplate);

    // Pass it the values for the aggregated grid...
    pSubGridTemplate->set_send_p(send_p());
    pSubGridTemplate->set_in_selection(is_in_selection());

    // Those settings will be used by read.
    pSubGridTemplate->read();

    // For some reason, some handlers only set read_p for the parts, not the whole!!
    pSubGridTemplate->set_read_p(true);
}

void GridAggregationBase::copyProtoMapsIntoThisGrid(const Dimension& aggDim)
{
    Grid* pSubGridTemplate = getSubGridTemplate();
    VALID_PTR(pSubGridTemplate);

    Map_iter mapIt;
    Map_iter mapEndIt = map_end();
    for (mapIt = map_begin(); mapIt != mapEndIt; ++mapIt) {
        Array* pOutMap = static_cast<Array*>(*mapIt);
        VALID_PTR(pOutMap);

        // If it isn't getting dumped, then don't bother with it
        if (!(pOutMap->send_p() || pOutMap->is_in_selection())) {
            continue;
        }

        // We don't want to touch the aggregation dimension since it's
        // handled specially.
        if (pOutMap->name() == aggDim.name) {
            // Make sure it's read with these constraints.
            pOutMap->read();
            continue;
        }

        // Otherwise, find the map in the protogrid and copy it's data into this.
        Array* pProtoGridMap = const_cast<Array*>(AggregationUtil::findMapByName(*pSubGridTemplate, pOutMap->name()));
        NCML_ASSERT_MSG(pProtoGridMap, "Couldn't find map in prototype grid for map name=" + pOutMap->name());
        BESDEBUG_FUNC(DEBUG_CHANNEL, prolog << "Calling read() on prototype map vector name=" << pOutMap->name() << " and calling transfer constraints..." << endl);

        // Make sure the protogrid maps were properly read
        NCML_ASSERT_MSG(pProtoGridMap->read_p(), "Expected the prototype map to have been read but it wasn't.");

        // Make sure the lengths match to be sure we're not gonna blow memory up
        NCML_ASSERT_MSG(pOutMap->length() == pProtoGridMap->length(),
            "Expected the prototype and output maps to have same size() "
                "after transfer of constraints, but they were not so we can't "
                "copy the data!");

        // The dimensions will have been set up correctly now so size() is correct...
        // We assume the pProtoGridMap matches at this point as well.
        // So we can use this call to copy from one vector to the other
        // so we don't use temp storage in between
        pOutMap->reserve_value_capacity(); // reserves mem for length
        pOutMap->set_value_slice_from_row_major_vector(*pProtoGridMap, 0);
        pOutMap->set_read_p(true);
    }
}

/* virtual */
void GridAggregationBase::transferConstraintsToSubGridHook(Grid* /*pSubGrid*/)
{
    THROW_NCML_INTERNAL_ERROR("Impl me!");
}

}
