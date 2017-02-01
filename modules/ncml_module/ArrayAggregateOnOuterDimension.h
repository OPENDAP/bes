//////////////////////////////////////////////////////////////////////////////
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

#ifndef __AGG_UTIL__ARRAY_AGGREGATE_ON_OUTER_DIMENSION_H__
#define __AGG_UTIL__ARRAY_AGGREGATE_ON_OUTER_DIMENSION_H__

#include "ArrayAggregationBase.h" // agg_util
#include "Dimension.h" // agg_util

using std::string;
using std::vector;
using libdap::Array;

namespace libdap {
    class ConstraintEvaluator;
    class DDS;
    class Marshaller;
}

namespace agg_util {
/**
 * class ArrayAggregateOnOuterDimension
 *
 * Array variable which contains information for performing a
 * joinNew (new outer dimension) aggregation of an Array variable using
 * samples of this variable in a specified list of member datasets.
 *
 * The list is specified as a list of RCPtr<AggMemberDataset>,
 * i.e. reference-counted AggMemberDataset's (AMD).  The AMD is in
 * charge of lazy-loading it's contained DataDDS as needed
 * as well as for only loading the required data for a read() call.
 * In other words, read() on this subclass will respect the constraints
 * given to the superclass Array.
 *
 * @note This class is designed to lazy-load the member
 * datasets _only if there are needed for the actual serialization_
 * at read() call time. Also note that this class specializes the
 * BaseType::serialize() method such that data reads (from datasets
 * and writes (to the network) are interleaved, reducing latency. In
 * addition, the data for the response is not stored in the object;
 * only the parts about to be serialized are even held in memory and
 * then only until they are sent to the client. Calls to the read()
 * method still read all of the data in to the variable's local storage
 * so that code that depends on that behavior will still work. If all
 * of the data are in memory, the overloaded serialize() method will
 * call libdap::Array::serialize(), preserving the expected behavior
 * in that case.
 *
 * @note The member datasets might be external files or might be
 * wrapped virtual datasets (specified in NcML) or nested
 * aggregation's.
 */
class ArrayAggregateOnOuterDimension: public ArrayAggregationBase {
public:
    /**
     * Construct a joinNew Array aggregation given the parameters.
     *
     * @param proto the Array to use as a prototype for the UNaggregated Array
     *              (ie module the new dimension).
     *              It is the object for the aggVar as loaded from memberDatasets[0].
     * @param memberDatasets  list of the member datasets forming the aggregation.
     *                        this list will be copied internally so memberDatasets
     *                        may be destroyed after this call (though the element
     *                        AggMemberDataset objects will be ref()'d in our copy).
     * @param arrayGetter  smart ptr to the algorithm for getting out the constrained
     *                     array from each individual AMDList DataDDS.
     *                     Ownership transferred to this (clearly).
     * @param newDim the new outer dimension this instance will add to the proto Array template
     *
     */
    ArrayAggregateOnOuterDimension(const libdap::Array& proto, const AMDList& memberDatasets,
        std::auto_ptr<ArrayGetterInterface>& arrayGetter, const Dimension& newDim);

    /** Construct from a copy */
    ArrayAggregateOnOuterDimension(const ArrayAggregateOnOuterDimension& proto);

    /** Destroy any local memory */
    virtual ~ArrayAggregateOnOuterDimension();

    /**
     * Virtual Constructor: Make a deep copy (clone) of the object
     * and return it.
     * @return ptr to the cloned object.
     */
    virtual ArrayAggregateOnOuterDimension* ptr_duplicate();

    /**
     * Assign this from rhs object.
     * @param rhs the object to copy from
     * @return *this
     */
    ArrayAggregateOnOuterDimension& operator=(const ArrayAggregateOnOuterDimension& rhs);

    virtual bool serialize(libdap::ConstraintEvaluator &eval, libdap::DDS &dds, libdap::Marshaller &m, bool ce_eval);

protected:
    // Subclass Interface
    /** Subclass hook for read() to copy granule constraints properly (inner dim ones). */
    virtual void transferOutputConstraintsIntoGranuleTemplateHook();

    /** Actually go through the constraints and stream the correctly
     * constrained data into the superclass's output buffer for
     * serializing out.
     */
    virtual void readConstrainedGranuleArraysAndAggregateDataHook();

private:
    // Helper interface

    /** Duplicate just the local (this subclass) data rep */
    void duplicate(const ArrayAggregateOnOuterDimension& rhs);

    /** Clear out any used memory */
    void cleanup() throw ();

private:
    // Data rep

    // The new outer dimension description
    Dimension _newDim;

};
// class ArrayAggregateOnOuterDimension

}

#endif /* __AGG_UTIL__ARRAY_AGGREGATE_ON_OUTER_DIMENSION_H__ */
