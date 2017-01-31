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
#ifndef __AGG_UTIL__ARRAY_JOIN_EXISTING_AGGREGATION_H__
#define __AGG_UTIL__ARRAY_JOIN_EXISTING_AGGREGATION_H__

#include "AggMemberDataset.h" // agg_util
#include "ArrayAggregationBase.h" // agg_util
#include "Dimension.h" // agg_util

namespace libdap {
    class ConstraintEvaluator;
    class DDS;
    class Marshaller;
}

namespace agg_util {

class ArrayJoinExistingAggregation: public ArrayAggregationBase {
public:

    /**
     * Construct a joinNew Array aggregation given the parameters.
     *
     * @param granuleTemplate the Array to use as a prototype for the UNaggregated Array
     *              It is the object for the aggVar as loaded from memberDatasets[0].
     * @param memberDatasets  list of the member datasets forming the aggregation.
     *                        this list will be copied internally so memberDatasets
     *                        may be destroyed after this call (though the element
     *                        AggMemberDataset objects will be ref()'d in our copy).
     * @param arrayGetter  smart ptr to the algorithm for getting out the constrained
     *                     array from each individual AMDList DataDDS.
     *                     Ownership transferred to this (clearly).
     * @param joinDim the outer dimension we are using (with post-aggregation
     *                cardinality) for the joinExisting
     *
     */
    ArrayJoinExistingAggregation(const libdap::Array& granuleTemplate, const AMDList& memberDatasets,
        std::auto_ptr<ArrayGetterInterface>& arrayGetter, const Dimension& joinDim);

    ArrayJoinExistingAggregation(const ArrayJoinExistingAggregation& rhs);

    virtual ~ArrayJoinExistingAggregation();

    ArrayJoinExistingAggregation& operator=(const ArrayJoinExistingAggregation& rhs);

    /**
     * Virtual Constructor: Make a deep copy (clone) of the object
     * and return it.
     * @return ptr to the cloned object.
     */
    virtual ArrayJoinExistingAggregation* ptr_duplicate();

    virtual bool serialize(libdap::ConstraintEvaluator &eval, libdap::DDS &dds, libdap::Marshaller &m, bool ce_eval);

protected:
    // Subclass Interface

    /** IMPL of subclass hook for read() to copy granule
     * constraints properly (inner dim ones). */
    virtual void transferOutputConstraintsIntoGranuleTemplateHook();

    /* IMPL of virtual hook.
     * Does the internal work to make the aggregation using read
     * and respecting constraints on the outer dimension */
    virtual void readConstrainedGranuleArraysAndAggregateDataHook();

private:
    // helpers

    /** Duplicate just the local (this subclass) data rep */
    void duplicate(const ArrayJoinExistingAggregation& rhs);

    /** Clear any state from this */
    void cleanup() throw ();

    /////////////////////////////////////////////////////////////////////////////
    // Data Rep

    /** The (outer) dimension we will be joining along,
     *  with post-aggregation cardinality. */
    agg_util::Dimension _joinDim;
};

}

#endif /* __AGG_UTIL__ARRAY_JOIN_EXISTING_AGGREGATION_H__ */
