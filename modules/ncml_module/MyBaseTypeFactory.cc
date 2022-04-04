///////////////////////////////////////////////////////////////////////////////
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

#include "config.h"

#include "MyBaseTypeFactory.h"

#include <libdap/BaseType.h>
#include <libdap/BaseTypeFactory.h>

#include <libdap/Array.h>
#include <libdap/Byte.h>
#include <libdap/Float32.h>
#include <libdap/Float64.h>
#include <libdap/Grid.h>
#include <libdap/Int16.h>
#include <libdap/Int32.h>
#include "NCMLArray.h"
#include <libdap/Sequence.h>
#include <libdap/Str.h>
#include <libdap/Structure.h>
#include <libdap/UInt16.h>
#include <libdap/UInt32.h>
#include <libdap/Url.h>

using namespace libdap;
using namespace std;

namespace ncml_module {

/* static */
libdap::BaseTypeFactory* MyBaseTypeFactory::_spFactory = new BaseTypeFactory();

#if 0
MyBaseTypeFactory::MyBaseTypeFactory()
{
}

MyBaseTypeFactory::~MyBaseTypeFactory()
{
}
#endif

unique_ptr<libdap::BaseType> MyBaseTypeFactory::makeVariable(const libdap::Type& t, const string &name)
{
    switch (t) {
    case dods_byte_c:
        return unique_ptr<BaseType>(_spFactory->NewByte(name));
        break;

    case dods_int16_c:
        return unique_ptr<BaseType>(_spFactory->NewInt16(name));
        break;

    case dods_uint16_c:
        return unique_ptr<BaseType>(_spFactory->NewUInt16(name));
        break;

    case dods_int32_c:
        return unique_ptr<BaseType>(_spFactory->NewInt32(name));
        break;

    case dods_uint32_c:
        return unique_ptr<BaseType>(_spFactory->NewUInt32(name));
        break;

    case dods_float32_c:
        return unique_ptr<BaseType>(_spFactory->NewFloat32(name));
        break;

    case dods_float64_c:
        return unique_ptr<BaseType>(_spFactory->NewFloat64(name));
        break;

    case dods_str_c:
        return unique_ptr<BaseType>(_spFactory->NewStr(name));
        break;

    case dods_url_c:
        return unique_ptr<BaseType>(_spFactory->NewUrl(name));
        break;

    case dods_array_c:
        THROW_NCML_INTERNAL_ERROR("MyBaseTypeFactory::makeVariable(): no longer can make Array, instead use Array<T> form!");
        break;

    case dods_structure_c:
        return unique_ptr<BaseType>(_spFactory->NewStructure(name));
        break;

    case dods_sequence_c:
        return unique_ptr<BaseType>(_spFactory->NewSequence(name));
        break;

    case dods_grid_c:
        return unique_ptr<BaseType>(_spFactory->NewGrid(name));
        break;

    default:
        THROW_NCML_INTERNAL_ERROR("MyBaseTypeFactory::makeVariable(): request to make an unknown variable type.");
#if 0
            return unique_ptr<BaseType>(0);
#endif
    }
}

unique_ptr<libdap::BaseType> MyBaseTypeFactory::makeVariable(const string& type, const std::string& name)
{
    if (isArrayTemplate(type)) {
        // create the template var by default... if the caller reads it, this one will
        // be deleted.  Better safe than bus error.
        return unique_ptr<BaseType>(makeArrayTemplateVariable(type, name, true).release());
    }
    else {
        return makeVariable(getType(type), name);
    }
}

/** Get the Type enumeration value which matches the given name. */
libdap::Type MyBaseTypeFactory::getType(const string& name)
{
    if (name == "Byte") {
        return dods_byte_c;
    }
    else if (name == "Int16") {
        return dods_int16_c;
    }
    else if (name == "UInt16") {
        return dods_uint16_c;
    }

    else if (name == "Int32") {
        return dods_int32_c;
    }

    else if (name == "UInt32") {
        return dods_uint32_c;
    }

    else if (name == "Float32") {
        return dods_float32_c;
    }

    else if (name == "Float64") {
        return dods_float64_c;
    }

    else if (name == "String" || name == "string") {
        return dods_str_c;
    }

    else if (name == "URL") {
        return dods_url_c;
    }

    else if (name == "Array") {
        return dods_array_c;
    }

    else if (name == "Structure") {
        return dods_structure_c;
    }

    else if (name == "Sequence") {
        return dods_sequence_c;
    }

    else if (name == "Grid") {
        return dods_grid_c;
    }

    else {
        return dods_null_c;
    }
}

bool MyBaseTypeFactory::isSimpleType(const string& name)
{
    Type t = getType(name);
    switch (t) {
    case dods_byte_c:
    case dods_int16_c:
    case dods_uint16_c:
    case dods_int32_c:
    case dods_uint32_c:
    case dods_float32_c:
    case dods_float64_c:
    case dods_str_c:
    case dods_url_c:
        return true;
    default:
        return false;
    }
}

bool MyBaseTypeFactory::isArrayTemplate(const string& typeName)
{
    // Just check for the form. We won't typecheck the template arg here since we'll just match strings.
    return (typeName.find("Array<") == 0 && (typeName.at(typeName.size() - 1) == '>'));
}

std::unique_ptr<libdap::Array>
MyBaseTypeFactory::makeArrayTemplateVariable(const string& type, const string& name, bool makeTemplateVar)
{
    Array* pNew = nullptr;
    if (type == "Array<Byte>") {
        pNew = new NCMLArray<dods_byte>(name);
        if (makeTemplateVar) {
            pNew->add_var_nocopy(makeVariable("Byte", name).release());
        }
    }
    else if (type == "Array<Int16>") {
        pNew = new NCMLArray<dods_int16>(name);
        if (makeTemplateVar) {
            pNew->add_var_nocopy(makeVariable("Int16", name).release());
        }
    }
    else if (type == "Array<UInt16>") {
        pNew = new NCMLArray<dods_uint16>(name);
        if (makeTemplateVar) {
            pNew->add_var_nocopy(makeVariable("UInt16", name).release());
        }
    }
    else if (type == "Array<Int32>") {
        pNew = new NCMLArray<dods_int32>(name);
        if (makeTemplateVar) {
            pNew->add_var_nocopy(makeVariable("Int32", name).release());
        }
    }
    else if (type == "Array<UInt32>") {
        pNew = new NCMLArray<dods_uint32>(name);
        if (makeTemplateVar) {
            pNew->add_var_nocopy(makeVariable("UInt32", name).release());
        }
    }
    else if (type == "Array<Float32>") {
        pNew = new NCMLArray<dods_float32>(name);
        if (makeTemplateVar) {
            pNew->add_var_nocopy(makeVariable("Float32", name).release());
        }
    }
    else if (type == "Array<Float64>") {
        pNew = new NCMLArray<dods_float64>(name);
        if (makeTemplateVar) {
            pNew->add_var_nocopy(makeVariable("Float64", name).release());
        }
    }
    else if (type == "Array<String>" || type == "Array<Str>") {
        pNew = new NCMLArray<std::string>(name);
        if (makeTemplateVar) {
            pNew->add_var_nocopy(makeVariable("String", name).release());
        }
    }
    else if (type == "Array<URL>" || type == "Array<Url>") {
        pNew = new NCMLArray<std::string>(name);
        if (makeTemplateVar) {
            pNew->add_var_nocopy(makeVariable("URL", name).release());
        }
    }
    else {
        THROW_NCML_INTERNAL_ERROR("MyBaseTypeFactory::makeArrayTemplateVariable(): can't create type=" + type);
    }

    // OOM condition...
    if (!pNew) {
        THROW_NCML_INTERNAL_ERROR(
            "MyBaseTypeFactory::makeArrayTemplateVariable(): failed to allocate memory for type=" + type);
    }

    return unique_ptr<Array>(pNew);
}
} // namespace ncml_module
