// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES

// Copyright (c) 2016 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

#include "config.h"

#include <string>

#include <BESError.h>
#include <BESDebug.h>

#include "DmrppByte.h"

#include "DmrppInt8.h"

#include "DmrppInt16.h"
#include "DmrppUInt16.h"
#include "DmrppInt32.h"
#include "DmrppUInt32.h"

#include "DmrppInt64.h"
#include "DmrppUInt64.h"

#include "DmrppFloat32.h"
#include "DmrppFloat64.h"

#include "DmrppStr.h"
#include "DmrppUrl.h"

#include "DmrppD4Enum.h"

#include "DmrppD4Opaque.h"

#include "DmrppArray.h"
#include "DmrppStructure.h"

#include "DmrppD4Sequence.h"
#include "DmrppD4Group.h"

#include "DmrppTypeFactory.h"

using namespace libdap;
using namespace std;

BaseType *DmrppTypeFactory::NewVariable(Type t, const string &name) const
{
	switch (t) {
	case dods_byte_c:
		return NewByte(name);
	case dods_char_c:
		return NewChar(name);

	case dods_uint8_c:
		return NewUInt8(name);
	case dods_int8_c:
		return NewInt8(name);

	case dods_int16_c:
		return NewInt16(name);
	case dods_uint16_c:
		return NewUInt16(name);
	case dods_int32_c:
		return NewInt32(name);
	case dods_uint32_c:
		return NewUInt32(name);

	case dods_int64_c:
		return NewInt64(name);
	case dods_uint64_c:
		return NewUInt64(name);

	case dods_float32_c:
		return NewFloat32(name);
	case dods_float64_c:
		return NewFloat64(name);

	case dods_str_c:
		return NewStr(name);
	case dods_url_c:
		return NewURL(name);

	case dods_enum_c:
		return NewEnum(name);

	case dods_opaque_c:
		return NewOpaque(name);

	case dods_array_c:
		return NewArray(name);

	case dods_structure_c:
		return NewStructure(name);

	case dods_sequence_c:
		return NewD4Sequence(name);

	case dods_group_c:
		return NewGroup(name);

	default:
		throw BESError("Unimplemented type in DAP4.", BES_INTERNAL_ERROR, __FILE__, __LINE__);
	}
}

Byte *
DmrppTypeFactory::NewByte(const string &n) const
{
	return new DmrppByte(n);
}

Byte *
DmrppTypeFactory::NewChar(const string &n) const
{
	Byte *b = new DmrppByte(n);
	b->set_type(dods_char_c);
	return b;
}

Byte *
DmrppTypeFactory::NewUInt8(const string &n) const
{
	Byte *b = new DmrppByte(n);
	b->set_type(dods_uint8_c);
	return b;
}

Int8 *
DmrppTypeFactory::NewInt8(const string &n) const
{
	return new DmrppInt8(n);
}

Int16 *
DmrppTypeFactory::NewInt16(const string &n) const
{
	return new DmrppInt16(n);
}

UInt16 *
DmrppTypeFactory::NewUInt16(const string &n) const
{
	return new DmrppUInt16(n);
}

Int32 *
DmrppTypeFactory::NewInt32(const string &n) const
{
	BESDEBUG("dmrpp", "Inside DAP4BaseTypeFactory::NewInt32" << endl);
	return new DmrppInt32(n);
}

UInt32 *
DmrppTypeFactory::NewUInt32(const string &n) const
{
	return new DmrppUInt32(n);
}

Int64 *
DmrppTypeFactory::NewInt64(const string &n) const
{
    BESDEBUG("dmrpp", "Inside DAP4BaseTypeFactory::NewInt64" << endl);
	return new DmrppInt64(n);
}

UInt64 *
DmrppTypeFactory::NewUInt64(const string &n) const
{
	return new DmrppUInt64(n);
}

Float32 *
DmrppTypeFactory::NewFloat32(const string &n) const
{
	return new DmrppFloat32(n);
}

Float64 *
DmrppTypeFactory::NewFloat64(const string &n) const
{
	return new DmrppFloat64(n);
}

Str *
DmrppTypeFactory::NewStr(const string &n) const
{
	return new DmrppStr(n);
}

Url *
DmrppTypeFactory::NewUrl(const string &n) const
{
	return new DmrppUrl(n);
}

/** Note that this method is called NewURL - URL in caps.
 */
Url *
DmrppTypeFactory::NewURL(const string &n) const
{
	return NewUrl(n);
}

D4Opaque *
DmrppTypeFactory::NewOpaque(const string &n) const
{
	return new DmrppD4Opaque(n);
}

D4Enum *
DmrppTypeFactory::NewEnum(const string &name, Type type) const
{
	return new DmrppD4Enum(name, type);
}

Array *
DmrppTypeFactory::NewArray(const string &n, BaseType *v) const
{
	return new DmrppArray(n, v);
}

Structure *
DmrppTypeFactory::NewStructure(const string &n) const
{
	return new DmrppStructure(n);
}

D4Sequence *
DmrppTypeFactory::NewD4Sequence(const string &n) const
{
	return new DmrppD4Sequence(n);
}

D4Group *
DmrppTypeFactory::NewGroup(const string &n) const
{
	return new DmrppD4Group(n);
}

