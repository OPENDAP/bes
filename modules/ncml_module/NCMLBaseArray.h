/////////////////////////////////////////////////////////////////////////////
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
#ifndef __NCML_MODULE__NCMLBASEARRAY_H__
#define __NCML_MODULE__NCMLBASEARRAY_H__

#include <Array.h>
#include <memory>
#include "Shape.h"

/**
 * An abstract superclass for NCMLArray<T> that handles the non-parameterized
 * functionality and allows us to treat subclasses of any type T polymorphically.
 *
 */
namespace ncml_module {

class NCMLBaseArray: public libdap::Array {
public:
    // class methods

#if 0
    /**
     * Make a new NCMLArray<T> from the given proto, using the Array interface.
     * It uses the underlying proto.var() BaseType to figure out the type parameter T
     * for the returned class, hence the return of the generic superclass pointer.
     *
     * All the data values in the proto value buffer are copied into the returned copy.
     * This ASSUMES that the data currently in proto's Vector _buf is valid for the
     * UNCONSTRAINED data case.
     *
     * It also copies the template arg, attribute table, and shape of the proto.
     *
     * @note This is used for handling problems with renaming an NCArray prior to read() loading
     * the data, forcing us to call read() on a rename, thus breaking constraints.  To handle this
     * error, we copy the data into our NCMLArray<T> in order to handle the constraints ourselves.
     */
    static auto_ptr< NCMLBaseArray > createFromArray(const libdap::Array& proto);
#endif

public:
    // Instance methods
    NCMLBaseArray();
    explicit NCMLBaseArray(const std::string& name);
    explicit NCMLBaseArray(const NCMLBaseArray& proto);

    virtual ~NCMLBaseArray();

    NCMLBaseArray& operator=(const NCMLBaseArray& rhs);

    /** Override to return false if we have uncomputed constraints
     * and only true if the current constraints match the Vector value buffer.
     */
    virtual bool read_p();

    /** Override to disable setting of this flag.  We will leave it false
     * unless the constraints match the Vector value buffer.
     */
    virtual void set_read_p(bool state);

    /**
     * If there are no constraints and this is the first call to read(),
     * we will do nothing, assuming the sueprclasses have everything under control.
     *
     * If there are constraints, this function will create the correct
     * buffer in Vector with the constrained data, generated from cached
     * local values gathered to be from the unconstrained state.
     *
     * The first call to read() will assume the CURRENT Vector buffer
     * has ALL values (unconstrained) and store a local copy before
     * generating a Vector buffer for the current constraints.
     *
     * After this call, the caller can be assured that the Vector's data buffer
     * has properly constrained data matching the current super Array's constraints.
     * Subsequent calls to read() will see if the constraints used to create the
     * Vector data buffer have changed and if so recompute a new Vector buffer from
     * the locally cached values.
     */
    virtual bool read();

    /** Get the current dimensions of our superclass Array as a Shape object. */
    virtual Shape getSuperShape() const;

    /**
     * Return whether the superclass Array has been constrained along
     * any dimensions.
     */
    virtual bool isConstrained() const;

    /**
     * Return whether the constraints used to create Vector._buf for the last read()
     * have changed, meaning we need to recompute Vector._buf using the new values.
     */
    virtual bool haveConstraintsChangedSinceLastRead() const;

    /** Store the current super Array shape as the current constraints so we remember */
    virtual void cacheCurrentConstraints();

    virtual void cacheUnconstrainedDimensions();

    /** Required by subclasses to copy the original data values locally */
    virtual void cacheSuperclassStateIfNeeded();

    /** Must copy the unconstrained current values of the proper type within Vector into the local instance. */
    virtual void cacheValuesIfNeeded() = 0;

    /** Copy the data values from the given array, assuming the type
     * matches the template type T of the subclass.  If not,
     * then exception.
     */
    virtual void copyDataFrom(libdap::Array& from)=0;

    /** @return whether we have the unconstrained data cached yet. */
    virtual bool isDataCached() const = 0;

    /**
     * Given the current Shape of the Array, generate the constrained value buffer
     * and set it into the Vector superclass rep for proper serialize().
     */
    virtual void createAndSetConstrainedValueBuffer() = 0;

private:

    /** Copy just the variables introduced in this class */
    void copyLocalRepFrom(const NCMLBaseArray& proto);

    /** Destroy the data local to this class */
    void destroy() throw ();

protected:
    // Data rep
    // The Shape for the case of NO constraints on the data, or null if not set yet.
    Shape* _noConstraints;

    // The Shape for the CURRENT dimensions in super Array, used to calculate the transmission buffer
    // for read() and also to check if haveConstraintsChangedSinceLastRead().  Null if not set yet.
    Shape* _currentConstraints;
};

}

#endif /* __NCML_MODULE__NCMLBASEARRAY_H__ */
