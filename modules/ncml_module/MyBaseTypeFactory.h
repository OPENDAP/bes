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
#ifndef __NCML_MODULE__MYBASETYPEFACTORY_H__
#define __NCML_MODULE__MYBASETYPEFACTORY_H__

#include <memory>
#include <string>

// libdap
#include "Array.h"
#include "BaseType.h" // need the Type enum...

namespace libdap {
class BaseTypeFactory;
}

namespace ncml_module {
/**
 * @brief Wrapper for the BaseTypeFactory that lets us create by type name.
 *
 * The regular BaseTypeFactory doesn't have a factory method by type name,
 * so this wrapper will add the desired functionality.  It is a static class
 * rather than a subclass.
 *
 * Note that we can create normal libdap::Array by name, but this is deprecated
 * since it fails with constraints.  We have added
 * special functionality for Array<T> to define an NCMLArray<T> as the return type.
 * This allows hyperslab constraints to work.
 */
class MyBaseTypeFactory {
protected:
    // static class for now
    MyBaseTypeFactory();
    virtual ~MyBaseTypeFactory();

private:
    // disallow copies
    MyBaseTypeFactory(const MyBaseTypeFactory& rhs);
    MyBaseTypeFactory& operator=(const MyBaseTypeFactory& rhs);

public:

    static libdap::Type getType(const string& name);

    /** @return whether the typeName refers to a simple (non-container) type. */
    static bool isSimpleType(const string& typeName);

    /** @return whether the desired type is of the form Array<T>
     * for some basic type T.  This is a special case for creating Arrays of
     * subclass NCMLArray<T> so we can handle constraints.
     */
    static bool isArrayTemplate(const string& typeName);

    /** Return a new variable of the given type
     * @param type the DAP type
     * @param name the name to give the new variable
     * */
    static std::auto_ptr<libdap::BaseType> makeVariable(const libdap::Type& type, const string& name);

    /** Return a new variable of the given type name.  Return null if type is not valid.
     * @param type  the DAP type to create.
     * @param name the name to give the new variable
     * */
    static std::auto_ptr<libdap::BaseType> makeVariable(const string& type, const string& name);

    /** Make an Array<T> where T is the DAP simple type for the values in the Array.
     * This creates the proper template class of NCMLArray<T> now rather than Array so we can handle
     * constraints.
     * @param type the parameterized name of the Array type, e.g. "Array<String>",
     *             Array<UInt32>, etc.
     * @param name the name to give the new Array
     * @param addTemplateVar  whether to create and add the template var so that var() is non-null.
     */
    static std::auto_ptr<libdap::Array> makeArrayTemplateVariable(const string& type, const string& name,
        bool addTemplateVar);

private:
    //data rep

    // Singleton factory we will use to create variables by type name
    static libdap::BaseTypeFactory* _spFactory;
};

}

#endif /* __NCML_MODULE__MYBASETYPEFACTORY_H__ */
