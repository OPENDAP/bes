
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2002,2003 OPeNDAP, Inc.
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

// (c) COPYRIGHT URI/MIT 1999
// Please read the full copyright statement in the file COPYRIGHT_URI.
//
// Authors:
//      jhrg,jimg       James Gallagher <jgallagher@gso.uri.edu>

// Declarations for CE functions.
//
// 1/15/99 jhrg

#ifndef _ce_functions_h
#define _ce_functions_h

#include <libdap/BaseType.h>
#include <libdap/Array.h>
#include <libdap/Error.h>
#include <libdap/ConstraintEvaluator.h>
#include <libdap/Sequence.h>
#include <libdap/Structure.h>
#include <libdap/ServerFunction.h>

//#include "GeoGridFunction.h"

namespace libdap
{

#if 0
void function_geogrid(int argc, BaseType *argv[], DDS &dds, BaseType **btpp) ;
class GeoGridFunction: public libdap::ServerFunction {
public:
	GeoGridFunction()
    {
		setName("geogrid");
		setDescriptionString("Subsets a grid by the values of it's geo-located map variables.");
		setUsageString("geogrid(...)");
		setRole("http://services.opendap.org/dap4/server-side-function/geogrid");
		setDocUrl("http://docs.opendap.org/index.php/Server_Side_Processing_Functions#geogrid");
		setFunction(libdap::function_geogrid);
		setVersion("1.2");
    }
    virtual ~GeoGridFunction()
    {
    }

    bool canOperateOn(DDS &dds);

};
#endif

#if 0
void function_grid(int argc, BaseType *argv[], DDS &dds, BaseType **btpp) ;
class GridFunction: public libdap::ServerFunction {
public:
	GridFunction()
    {
		setName("grid");
		setDescriptionString("Subsets a grid by the values of it's geo-located map variables.");
		setUsageString("grid(...)");
		setRole("http://services.opendap.org/dap4/server-side-function/grid");
		setDocUrl("http://docs.opendap.org/index.php/Server_Side_Processing_Functions#grid");
		setFunction(libdap::function_grid);
		setVersion("1.0");
 }
    virtual ~GridFunction()
    {
    }

    bool canOperateOn(DDS &dds);

};
#endif

#if 0
/**
 * The linear_scale() function applies the familiar y = mx + b equation to data.
 */
void function_linear_scale(int argc, BaseType *argv[], DDS &dds, BaseType **btpp) ;

/**
 * The LinearScaleFunction class encapsulates the linear_scale function 'function_linear_scale'
 * along with additional meta-data regarding its use and applicability.
 */
class LinearScaleFunction: public libdap::ServerFunction {
public:
	LinearScaleFunction()
    {
		setName("linear_scale");
		setDescriptionString("The linear_scale() function applies the familiar y = mx + b equation to data.");
		setUsageString("linear_scale(var) | linear_scale(var,scale_factor,add_offset) | linear_scale(var,scale_factor,add_offset,missing_value)");
		setRole("http://services.opendap.org/dap4/server-side-function/linear-scale");
		setDocUrl("http://docs.opendap.org/index.php/Server_Side_Processing_Functions#linear_scale");
		setFunction(libdap::function_linear_scale);
		setVersion("1.0b1");
    }
    virtual ~LinearScaleFunction()
    {
    }

};
#endif


#if 0
void function_version(int argc, BaseType *argv[], DDS &dds, BaseType **btpp) ;
class VersionFunction: public libdap::ServerFunction {
public:
	VersionFunction()
    {
		setName("version");
		setDescriptionString("The version function provides a list of the server-side processing functions available on a given server along with their versions.");
		setUsageString("version()");
		setRole("http://services.opendap.org/dap4/server-side-function/version");
		setDocUrl("http://docs.opendap.org/index.php/Server_Side_Processing_Functions#version");
		setFunction(libdap::function_version);
		setVersion("1.0");
    }
    virtual ~VersionFunction()
    {
    }

};
#endif

} // namespace libdap

#endif // _ce_functions_h
