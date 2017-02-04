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
#ifndef __AGG_UTIL__RCOBJECT_INTERFACE_H__
#define __AGG_UTIL__RCOBJECT_INTERFACE_H__

#include <string>

namespace agg_util {

/**
 * Interface class for a reference counted object.
 */
class RCObjectInterface {
public:

    virtual ~RCObjectInterface() = 0;

    /** Increase the reference count by one.
     * const since we do not consider the ref count part of the semantic constness of the rep */
    virtual int ref() const = 0;

    /** Decrease the reference count by one.  If it goes from 1 to 0,
     * delete this and this is no longer valid.
     * @return the new ref count.  If it is 0, the called knows the
     * object was deleted.
     *
     * It is illegal to unref() an object with a count of 0.  We don't
     * throw to allow use in dtors, so the caller is to not do it!
     *
     * const since the reference count is not part of the semantic constness of the rep
     */
    virtual int unref() const = 0;

    /** Get the current reference count */
    virtual int getRefCount() const = 0;

    /** @return a string with the refcount and memory address  */
    virtual std::string toString() const = 0;

    /** If the object is in an auto-delete pool,
     * remove it from the pool and force it to only delete
     * when it's ref count goes to 0.
     * Useful when we desire a particular object stay around
     * outside of the pool's lifetime.
     */
    virtual void removeFromPool() const = 0;

};
// class RCObjectInterface

}// namespace agg_util

#endif /* __AGG_UTIL__RCOBJECT_INTERFACE_H__ */
