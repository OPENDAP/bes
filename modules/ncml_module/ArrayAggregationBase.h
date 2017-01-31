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
#ifndef __AGG_UTIL__ARRAY_AGGREGATION_BASE_H__
#define __AGG_UTIL__ARRAY_AGGREGATION_BASE_H__

#include "AggMemberDataset.h" // agg_util
#include "AggregationUtil.h" // agg_util
#include <Array.h> // libdap
#include <memory> // std

namespace libdap {
    class ConstraintEvaluator;
    class DDS;
    class Marshaller;
}

namespace agg_util
{
  /**
   * Base class for subclasses of libdap::Array which
   * perform aggregation on a list of AggMemberDatasets when asked.
   */
  class ArrayAggregationBase : public libdap::Array
  {
  public:
    /**
     * Construct the base class using the given parameters.
     * Used for join aggregation concrete subclasses.
     *
     * @param granuleProto  template describing the data array of
     *                      for a granule (member) of the join agg.
     *                      Note: for joinExisting the array size
     *                      may not be correct!
     * @param memberDatasets the granules to use in the agg
     * @param arrayGetter auto_ptr to the data array getter for the variable.
     *                    Note the auto_ptr ref is sunk by the ctor so don't
     *                    delete/release from the caller.
     * @return
     */
    ArrayAggregationBase(
        const libdap::Array& granuleProto, // prototype for granule
        const AMDList& memberDatasets, // granule descs to use for the agg
        std::auto_ptr<ArrayGetterInterface>& arrayGetter // way to get the data array
        );

    ArrayAggregationBase(const ArrayAggregationBase& rhs);

    virtual ~ArrayAggregationBase();

    ArrayAggregationBase& operator=(const ArrayAggregationBase& rhs);

    /** virtual constructor i.e. clone */
    virtual ArrayAggregationBase* ptr_duplicate();

    /**
     * Base implementation that works for both joinNew and joinExisting.
     * Sets ups constraints and things and then calls the subclass helper
     * readAndAggregateGranules() for the specialized subclass behaviors.
     *
     * @note This implementation of read() will read all of the data
     * for an aggregated variable. Child classes of this class can use
     * specialized versions of serialize() to implement a different
     * behavior so that data that are read in parts (e.g. slices of an
     * Array) can also be written in parts. This can reduce access latency.
     *
     * @throw Can throw BESError, minimally
     *
     * @return whether it works
     */
    virtual bool read();

    /**
    * Get the list of AggMemberDataset's that comprise this aggregation
    */
    const AMDList& getDatasetList() const;

  protected:


    /** Print out the constraints on fromArray to the debug channel */
    void printConstraints(const Array& fromArray);

    /** Accessor for subclasses
     * Note this is protected, so not const!
     * Subclasses may mutate the return hence this,
     * but should not delete it, hence the reference. */
    libdap::Array& getGranuleTemplateArray();

    /** Accessor for subclasses
    * Note this is protected, so not const!
    * Subclasses may mutate the return hence this,
    * but should not delete it, hence the reference. */
    const ArrayGetterInterface& getArrayGetterInterface() const;

  protected: // Subclass Interface

    /** subclass hook from read() to setup constraints on inner dims correctly */
    virtual void transferOutputConstraintsIntoGranuleTemplateHook();

    /**
     * The meat of the subclass impl of read().
     * Called from read() once this base class state
     * is ready for the granule data.
     */
    virtual void readConstrainedGranuleArraysAndAggregateDataHook();

  private:

    /** Assign the state from rhs into this */
    void duplicate(const ArrayAggregationBase& rhs);

    /** Clean up any local state */
    void cleanup() throw();

    ////////////////////////////////////////////////////////////////////
    /// Data Rep

    /** A template for the unaggregated (sub) Array
     *  It will be used to constrain and validate other
     *  dataset aggVar's as they are loaded.
     */
    std::auto_ptr<libdap::Array> _pSubArrayProto;

    /**Gets the constrained, read data out
    for aggregating into this object. */
    std::auto_ptr<ArrayGetterInterface> _pArrayGetter;

    /**
     * Entries contain information on loading the individual datasets
     * in a lazy evaluation as needed if they are in the output.
     * The elements are ref counted, so need not be destroyed.
     */
    AMDList _datasetDescs;

  };

}

#endif /* __AGG_UTIL__ARRAY_AGGREGATION_BASE_H__ */
