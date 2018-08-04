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
#ifndef __NCML_MODULE__NCMLARRAY_H__
#define __NCML_MODULE__NCMLARRAY_H__

#include <dods-datatypes.h>
#include <Array.h>
#include <BaseType.h>
#include <Vector.h>
#include <memory>
// #include "MyBaseTypeFactory.h"
#include "NCMLBaseArray.h"
#include "NCMLDebug.h"
#include "Shape.h"
#include <sstream>
#include <string>
#include <typeinfo>
#include <vector>

using libdap::Array;
using libdap::dods_byte;
using libdap::dods_int16;
using libdap::dods_uint16;
using libdap::dods_int32;
using libdap::dods_uint32;
using libdap::dods_float32;
using libdap::dods_float64;

using std::string;
using std::vector;

namespace ncml_module {
class Shape;

/**
 * @brief A parameterized subclass of libdap::Array that allows us to apply constraints on
 * NcML-specified data prior to serialization.  All the code is in the .h, so no .cc is defined.
 *
 * This class caches the full set of data values of basic type T for the unconstrained
 * super Array shape.  It forces read_p() to ALWAYS be false to force a call of this->read()
 * prior to any serialization of super Vector's data buffer.
 *
 *
 * In a read() call, if constraints have been applied to the Array superclass, this class
 * will effect the Vector superclass data buffer (Vector._buf) in order to calculate
 * and store the constrained data into Vector._buf so serialize() passes the constrained
 * data properly.  It maintains a copy of the full set of data, however, in case constraints
 * are later removed or changed.  It maintains a copy of the constrained shape used to generate
 * the current Vector._buf so that on subsequent read() calls it can check to see if the constraints
 * have changed and if so recompute Vector._buf.
 *
 * We use a template on the underlying data type stored, such as dods_byte, dods_int32, etc.
 * Note that this can ALSO be std::string, in which case Vector does special processing.  We
 * need to be aware of this in processing data.
 *
 * NOTE: I examined the way this class is used and I think that, in fact, it is only currently used
 * for new variable arrays (in which the data is provided in the source ncml file) and NEVER for
 * existing arrays (which would mean massive copying of data). Now I may well be wrong about this
 * assessment, but at the moment that what it looks like to me. ndp - 8/7/15
 *
 */
template<typename T>
class NCMLArray: public NCMLBaseArray {
public:
    // Class methods

public:
    // Instance methods
    NCMLArray() :
        NCMLBaseArray(""), _allValues(0)
    {
    }

    explicit NCMLArray(const string& name) :
        NCMLBaseArray(name), _allValues(0)
    {
    }

    explicit NCMLArray(const NCMLArray<T>& proto) :
        NCMLBaseArray(proto), _allValues(0)
    {
        copyLocalRepFrom(proto);
    }

    /** Destroy any locally cached data */
    virtual ~NCMLArray()
    {
        destroy(); // helper
    }

    NCMLArray<T>&
    operator=(const NCMLArray<T>& rhs)
    {
        if (&rhs == this) {
            return *this;
        }

        // Call the super assignment
        NCMLBaseArray::operator=(rhs);

        // Copy local private rep
        copyLocalRepFrom(rhs);

        return *this;
    }

    /** Override virtual constructor, deep clone */
    virtual NCMLArray<T>* ptr_duplicate()
    {
        return new NCMLArray(*this);
    }

    /** Copy the rep from the given Array.
     * This includes:
     *  - Shape
     *  - Attribute table
     *  - Template BaseType var
     *  - Data values
     */
    virtual void copyDataFrom(libdap::Array& from)
    {
        // We both better have a valid template class.
        VALID_PTR(from.var());

        // clear out any current local values
        destroy();

        // OK, now that we have it, we need to copy the attribute table and the prototype variable
        set_attr_table(from.get_attr_table());
        // switched to add_var_ncopy() to avoid memory leak. jhrg 8/3/18
        add_var_nocopy(from.var()->ptr_duplicate()); // This may leak memory. Fixed.

        // add all the dimensions
        Array::Dim_iter endIt = from.dim_end();
        Array::Dim_iter it;
        for (it = from.dim_begin(); it != endIt; ++it) {
            Array::dimension& dim = *it;
            append_dim(dim.size, dim.name);
        }

        // Finally, copy the data.
        // Initialize with length() values so the storage and size of _allValues is correct.
        _allValues = new std::vector<T>(from.length());
        NCML_ASSERT(_allValues->size() == static_cast<unsigned int>(from.length()));

        // Copy the values in from._buf into our cache!
        T* pFirst = &((*_allValues)[0]);
        from.buf2val(reinterpret_cast<void**>(&pFirst));
    }

    virtual bool isDataCached() const
    {
        return _allValues;
    }

    /////////////////////////////////////////////////////////////
    // We have to override these to make a copy in this subclass as well since constraints added before read().
    // TODO Consider instead allowing add_constraint() in Array to be virtual so we can catch it and cache at that point rather than
    // all the time!!

    // Helper macros to avoid a bunch of cut & paste

#define NCMLARRAY_CHECK_ARRAY_TYPE_THEN_CALL_SUPER(arrayValue, sz)  \
  if (typeid(arrayValue) != typeid(T*)) \
  { \
    THROW_NCML_INTERNAL_ERROR("NCMLArray<T>::set_value(): got wrong type of value array, doesn't match type T!"); \
  } \
  bool ret = Vector::set_value((arrayValue), (sz)); \
  cacheSuperclassStateIfNeeded(); \
  return ret;

#define NCMLARRAY_CHECK_VECTOR_TYPE_THEN_CALL_SUPER(vecValue, sz) \
  if (typeid(vecValue) != typeid(vector<T>&)) \
  { \
    THROW_NCML_INTERNAL_ERROR("NCMLArray<T>::setValue(): got wrong type of value vector, doesn't match type T!"); \
  } \
  bool ret = Vector::set_value((vecValue), (sz)); \
  cacheSuperclassStateIfNeeded(); \
  return ret;

    virtual bool set_value(dods_byte *val, int sz)
    {
        NCMLARRAY_CHECK_ARRAY_TYPE_THEN_CALL_SUPER(val, sz);
    }

    virtual bool set_value(vector<dods_byte> &val, int sz)
    {
        NCMLARRAY_CHECK_VECTOR_TYPE_THEN_CALL_SUPER(val, sz);
    }

    virtual bool set_value(dods_int16 *val, int sz)
    {
        NCMLARRAY_CHECK_ARRAY_TYPE_THEN_CALL_SUPER(val, sz);
    }

    virtual bool set_value(vector<dods_int16> &val, int sz)
    {
        NCMLARRAY_CHECK_VECTOR_TYPE_THEN_CALL_SUPER(val, sz);
    }

    virtual bool set_value(dods_uint16 *val, int sz)
    {
        NCMLARRAY_CHECK_ARRAY_TYPE_THEN_CALL_SUPER(val, sz);
    }

    virtual bool set_value(vector<dods_uint16> &val, int sz)
    {
        NCMLARRAY_CHECK_VECTOR_TYPE_THEN_CALL_SUPER(val, sz);
    }

    virtual bool set_value(dods_int32 *val, int sz)
    {
        NCMLARRAY_CHECK_ARRAY_TYPE_THEN_CALL_SUPER(val, sz);
    }

    virtual bool set_value(vector<dods_int32> &val, int sz)
    {
        NCMLARRAY_CHECK_VECTOR_TYPE_THEN_CALL_SUPER(val, sz);
    }

    virtual bool set_value(dods_uint32 *val, int sz)
    {
        NCMLARRAY_CHECK_ARRAY_TYPE_THEN_CALL_SUPER(val, sz);
    }

    virtual bool set_value(vector<dods_uint32> &val, int sz)
    {
        NCMLARRAY_CHECK_VECTOR_TYPE_THEN_CALL_SUPER(val, sz);
    }

    virtual bool set_value(dods_float32 *val, int sz)
    {
        NCMLARRAY_CHECK_ARRAY_TYPE_THEN_CALL_SUPER(val, sz);
    }

    virtual bool set_value(vector<dods_float32> &val, int sz)
    {
        NCMLARRAY_CHECK_VECTOR_TYPE_THEN_CALL_SUPER(val, sz);
    }

    virtual bool set_value(dods_float64 *val, int sz)
    {
        NCMLARRAY_CHECK_ARRAY_TYPE_THEN_CALL_SUPER(val, sz);
    }

    virtual bool set_value(vector<dods_float64> &val, int sz)
    {
        NCMLARRAY_CHECK_VECTOR_TYPE_THEN_CALL_SUPER(val, sz);
    }

    virtual bool set_value(string *val, int sz)
    {
        NCMLARRAY_CHECK_ARRAY_TYPE_THEN_CALL_SUPER(val, sz);
    }

    virtual bool set_value(vector<string> &val, int sz)
    {
        NCMLARRAY_CHECK_VECTOR_TYPE_THEN_CALL_SUPER(val, sz);
    }

#undef NCMLARRAY_CHECK_ARRAY_TYPE_THEN_CALL_SUPER
#undef NCMLARRAY_CHECK_VECTOR_TYPE_THEN_CALL_SUPER

protected:

    /**
     * The first time we get a read(), we want to grab the current state of the superclass Array and Vector
     * and cache them locally before we apply any needed constraints.
     * ASSUME: that the current value of the _buf in Vector super is the FULL set of values for the UNCONSTRAINED Array.
     * We also grab the dimensions from the super Array, but realize that these may be already constrained.
     * For this reason, we cache the UNCONSTRAINED shape (i.e. the d.size() for all dimensions) as well
     * as the full set of current dimensions, constrained or not.  The latter tells of if constraints have
     * been applied and if we need to compute a new _buf in read().
     */
    virtual void cacheValuesIfNeeded()
    {
        // If the super Vector has no capacity, it's not set up correctly, so don't call this or we get exception.
        if (get_value_capacity() == 0) {
            BESDEBUG("ncml", "cacheValuesIfNeeded: the superclass Vector has no data so not copying...");
        }

        // If we haven't gotten this yet, go get it,
        // assuming the super Vector contains all values
        if (!_allValues) {
            BESDEBUG("ncml",
                "NCMLArray<T>:: we don't have unconstrained values cached, caching from Vector now..." << endl);
            unsigned int spaceSize = _noConstraints->getUnconstrainedSpaceSize();

#if 0
            ostringstream oss;
            oss <<"NCMLArray expected superclass Vector length() to be the same as unconstrained space size, but it wasn't!";
            oss << "length(): " << length() << "' spaceSize: " << spaceSize;
            NCML_ASSERT_MSG(static_cast<unsigned int>(length()) == spaceSize, oss.str());
#else
            NCML_ASSERT_MSG(static_cast<unsigned int>(length()) == spaceSize,
                "NCMLArray expected superclass Vector length() to be the same as unconstrained space size, but it wasn't!");
#endif
            // Make new default storage with enough space for all the data.
            _allValues = new vector<T>(spaceSize);
            NCML_ASSERT(_allValues->size() == spaceSize); // the values should all be default for T().
            // Grab the address of the start of the vector memory block.
            // This is safe since vector memory is required to be contiguous
            T* pFirstElt = &((*_allValues)[0]);
            // Now overwrite the defaults in from the buffer.
            unsigned int stored = buf2val(reinterpret_cast<void**>(&pFirstElt));

            NCML_ASSERT((stored / sizeof(T)) == spaceSize); // make sure it did what it was supposed to do.
            // OK, we have our copy now!
        }

        // We ignore the current constraints since we don't worry about them until later in read().
    }

    /**
     * If we have constraints that are not the same as the constraints
     * on the last read() call (or if this is first read() call), use
     * the super Array's current constrained dimension values to set Vector->val2buf()
     * with the proper constrained data.
     * ASSUMES: cacheSuperclassStateIfNeeded() has already been called once so
     * that this instance's state contains all the unconstrained data values and shape.
     */
    virtual void createAndSetConstrainedValueBuffer()
    {
        BESDEBUG("ncml", "NCMLArray<T>::createAndSetConstrainedValueBuffer() called!" << endl);

        // Whether to validate, depending on debug build or not.
#ifdef NDEBUG
        const bool validateBounds = false;
#else
        const bool validateBounds = true;
#endif

        // These need to exist or caller goofed.
        VALID_PTR(_noConstraints);
        VALID_PTR(_allValues);

        // This reflects the current constraints, so is what we want.
        unsigned int numVals = length();
        vector<T> values; // Exceptions may wind through and I want this storage cleaned, so vector<T> rather than T*.
        values.reserve(numVals);

        // Our current space, with constraints
        const Shape shape = getSuperShape();
        Shape::IndexIterator endIt = shape.endSpaceEnumeration();
        Shape::IndexIterator it;
        unsigned int count = 0;  // just a counter for number of points for sanity checking
        for (it = shape.beginSpaceEnumeration(); it != endIt; ++it, ++count) {
            // Take the current point in constrained space, look it up in cached 3values, set it as next elt in output
            values.push_back((*_allValues)[_noConstraints->getRowMajorIndex(*it, validateBounds)]);
        }

        // Sanity check the number of points we added.  They need to match or something is wrong.
        if (count != static_cast<unsigned int>(length())) {
            stringstream msg;
            msg << "While adding points to hyperslab buffer we got differing number of points "
                "from Shape space enumeration as expected from the constraints! "
                "Shape::IndexIterator produced " << count << " points but we expected " << length();
            THROW_NCML_INTERNAL_ERROR(msg.str());
        }

        if (count != shape.getConstrainedSpaceSize()) {
            stringstream msg;
            msg << "While adding points to hyperslab buffer we got differing number of points "
                "from Shape space enumeration as expected from the shape.getConstrainedSpaceSize()! "
                "Shape::IndexIterator produced " << count << " points but we expected "
                << shape.getConstrainedSpaceSize();
            THROW_NCML_INTERNAL_ERROR(msg.str());
        }

        // Otherwise, we're good, so blast the values into the valuebuffer.
        // tell it to reuse the current buffer since by definition is has enough room since it holds all points to start.
        val2buf(static_cast<void*>(&(values[0])), true);
    }

private:
    // This class ONLY methods

    /** Copy in this the local data (private rep) in proto
     * Used by ptr_duplicate() and copy ctor */
    void copyLocalRepFrom(const NCMLArray<T>& proto)
    {
        // Avoid unnecessary finagling
        if (&proto == this) {
            return;
        }

        // Blow away any old data before copying new
        destroy();

        // If there are values, make a copy of the vector.
        if (proto._allValues) {
            _allValues = new vector<T>(*(proto._allValues));
        }
    }

    /** Helper to destroy all the local data to pristine state. */
    void destroy() throw ()
    {
        delete _allValues;
        _allValues = 0;
    }

private:

    // The unconstrained data set, cached from super in first call to cacheSuperclassStateIfNeeded()
    // if it is null.
    std::vector<T>* _allValues;

};
// class NCMLArray<T>

}// namespace ncml_module

#endif /* __NCML_MODULE__NCMLARRAY_H__ */
