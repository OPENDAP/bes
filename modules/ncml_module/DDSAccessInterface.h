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
#ifndef __AGG_UTIL__DDS_ACCESS_INTERFACE_H__
#define __AGG_UTIL__DDS_ACCESS_INTERFACE_H__

#include "RCObjectInterface.h"

namespace libdap {
class DDS;
}

namespace agg_util {
/**
 * Interface class for any object that can contains a DDS.
 * Useful for avoiding module back-dependencies
 * since we do not want agg_util dependencies back to
 * ncml_module, e.g.
 */
class DDSAccessInterface {
public:

    virtual ~DDSAccessInterface() = 0;

    /**
     * Accessor for a contained DDS.
     * The returned object is to be considered an alias,
     * and should NOT be deleted or stored outside the lifetime of this!
     * If the object doesn't have a valid DDS currently, NULL is returned.
     * @return alias to the DDS that the object is containing, or NULL if none.
     */
    virtual const libdap::DDS* getDDS() const = 0;
};

/** Mixture interface for when we a
 * reference-counted DDS container */
class DDSAccessRCInterface: public virtual RCObjectInterface, public virtual DDSAccessInterface {
public:
    virtual ~DDSAccessRCInterface() = 0;
};

}

#endif /* __AGG_UTIL__DDS_ACCESS_INTERFACE_H__ */
