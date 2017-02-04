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

#include <BaseType.h>
//#include "MyBaseTypeFactory.h"
#include "NCMLBaseArray.h"
#include "NCMLDebug.h"
#include "Shape.h"

namespace ncml_module {

#if 0
// I blocked this off because it's not being actively used and it's premise (massive data copying) is pretty much a
// problem for the handler if it is used. ndp 8/7/15

/**
 * Make a new NCMLArray<T> from the given proto, using the Array interface.
 * It uses the underlying proto.var() BaseType to figure out the type parameter T
 * for the returned class, hence the return of the generic superclass pointer.
 *
 * All the data values in the proto value buffer are copied into the returned copy.
 * This ASSUMES that the data currently in proto's Vector _buf is valid for the
 * UNCONSTRAINED data case.
 *
 * @note This is used for handling problems with renaming an NCArray prior to read() loading
 * the data, forcing us to call read() on a rename, thus breaking constraints.  To handle this
 * error, we copy the data into our NCMLArray<T> in order to handle the constraints ourselves.
 */
auto_ptr< NCMLBaseArray >
NCMLBaseArray::createFromArray(const libdap::Array& protoC)
{
    // The const in the signature means semantic const.  We promise not to change protoC,
    // but need a non-const reference to make calls to it, unfortunately.
    libdap::Array& proto = const_cast<libdap::Array&>(protoC);

    BESDEBUG("ncml", "NCMLBaseArray::createFromArray(): Converting prototype Array name=" + proto.name() + " into an NCMLArray..." << endl);

    BaseType* pTemplate = proto.var();
    NCML_ASSERT_MSG(pTemplate, "NCMLArray::createFromArray(): got NULL template BaseType var() for proto name=" + proto.name());

    // Factory up and test result
    string ncmlArrayType = "Array<" + pTemplate->type_name() + ">";
    auto_ptr<libdap::BaseType> pNewBT = MyBaseTypeFactory::makeVariable(ncmlArrayType, proto.name());
    VALID_PTR(pNewBT.get());
    auto_ptr< NCMLBaseArray > pNewArray = auto_ptr< NCMLBaseArray > (dynamic_cast< NCMLBaseArray*>(pNewBT.release()));
    VALID_PTR(pNewArray.get());

    // Finally, we should be able to copy the data now.
    pNewArray->copyDataFrom(proto);

    return pNewArray;// relinquish
}
#endif

NCMLBaseArray::NCMLBaseArray() :
    Array("", 0), _noConstraints(0), _currentConstraints(0)
{

}

NCMLBaseArray::NCMLBaseArray(const std::string& name) :
    Array(name, 0), _noConstraints(0), _currentConstraints(0)
{
}

NCMLBaseArray::NCMLBaseArray(const NCMLBaseArray& proto) :
    Array(proto), _noConstraints(0), _currentConstraints(0)
{
    copyLocalRepFrom(proto);
}

NCMLBaseArray::~NCMLBaseArray()
{
    destroy(); // local data
}

NCMLBaseArray&
NCMLBaseArray::operator=(const NCMLBaseArray& rhs)
{
    if (&rhs == this) {
        return *this;
    }

    // Call the super assignment
    Array::operator=(rhs);

    // Copy local private rep
    copyLocalRepFrom(rhs);

    return *this;
}

bool NCMLBaseArray::read_p()
{
    // If we haven't computed constrained buffer yet, or they changed,
    // we must call return false to force read() to be called again.
    return !haveConstraintsChangedSinceLastRead();
}

void NCMLBaseArray::set_read_p(bool /* state */)
{
    // Just drop it on the floor we compute it
    // Array::set_read_p(state);
}

bool NCMLBaseArray::read()
{
    BESDEBUG("ncml", "NCMLArray::read() called!" << endl);

    // If first call, cache the full dataset.  Throw if there's an error with this.
    cacheSuperclassStateIfNeeded();

    // If _currentConstraints is null or different than current Array dimensions,
    // compute the constrained data buffer from the local data cache and the current Array dimensions.
    if (haveConstraintsChangedSinceLastRead()) {
        // Enumerate and set the constrained values into Vector super..
        createAndSetConstrainedValueBuffer();

        // Copy the constraints we used to generate these values
        // so we know if we need to redo this in another call to read() or not.
        cacheCurrentConstraints();
    }
    return true;
}

Shape NCMLBaseArray::getSuperShape() const
{
    // make the Shape for our superclass Array
    return Shape(*this);
}

bool NCMLBaseArray::isConstrained() const
{
    Shape superShape = getSuperShape();
    return superShape.isConstrained();
}

bool NCMLBaseArray::haveConstraintsChangedSinceLastRead() const
{
    // If there's none, then they've changed by definition.
    if (!_currentConstraints) {
        return true;
    }
    else // compare the current values to those currently in our Array slice
    {
        return ((*_currentConstraints) != getSuperShape());
    }
}

void NCMLBaseArray::cacheCurrentConstraints()
{
    // If got some already, blow them away...
    if (_currentConstraints) {
        delete _currentConstraints;
        _currentConstraints = 0;
    }
    _currentConstraints = new Shape(*this);
    //BESDEBUG("ncml", "NCMLBaseArray: Cached current constraints:" << (*_currentConstraints) << endl);
}

void NCMLBaseArray::cacheUnconstrainedDimensions()
{
    // We already got it...
    if (_noConstraints) {
        return;
    }

    // Copy from the super Array's current dimensions and force values to define an unconstrained space.
    _noConstraints = new Shape(*this);
    _noConstraints->setToUnconstrained();

    //BESDEBUG("ncml", "NCMLBaseArray: cached unconstrained shape=" << (*_noConstraints) << endl);
}

void NCMLBaseArray::cacheSuperclassStateIfNeeded()
{
    // We had better have a template or else the width() calls will be wrong.
    NCML_ASSERT(var());

    // First call, make sure we grab unconstrained state.
    if (!_noConstraints) {
        cacheUnconstrainedDimensions();
    }

    // Subclasses will handle this
    cacheValuesIfNeeded();
}

void NCMLBaseArray::copyLocalRepFrom(const NCMLBaseArray& proto)
{
    // Avoid unnecessary finagling
    if (&proto == this) {
        return;
    }

    // Blow away any old data before copying new
    destroy();

    if (proto._noConstraints) {
        _noConstraints = new Shape(*(proto._noConstraints));
    }

    if (proto._currentConstraints) {
        _currentConstraints = new Shape(*(proto._currentConstraints));
    }
}

/** Helper to destroy all the local data to pristine state. */
void NCMLBaseArray::destroy() throw ()
{
    delete _noConstraints;
    _noConstraints = 0;
    delete _currentConstraints;
    _currentConstraints = 0;
}

}
