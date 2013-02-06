
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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

#include "BaseType.h"
#include "Array.h"
#include "Error.h"
#include "ConstraintEvaluator.h"
#include "AbstractFunction.h"

namespace libdap
{
#if 0
// These functions are use by the code in GeoConstraint
string extract_string_argument(BaseType *arg) ;
double extract_double_value(BaseType *arg) ;
double *extract_double_array(Array *a) ;
void set_array_using_double(Array *dest, double *src, int src_len) ;
#endif
#if 0
void function_geoarray(int argc, BaseType *argv[], DDS &dds, BaseType **btpp) ;
void function_ugrid_demo(int argc, BaseType * argv[], DDS &dds, BaseType **btpp) ;

// Projection function used to pass DAP version information
void function_dap(int argc, BaseType *argv[], DDS &dds, ConstraintEvaluator &ce);

void register_functions(ConstraintEvaluator &ce);
#endif



void function_geogrid(int argc, BaseType *argv[], DDS &dds, BaseType **btpp) ;
class GeoGridFunction: public AbstractFunction {
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

};



void function_grid(int argc, BaseType *argv[], DDS &dds, BaseType **btpp) ;
class GridFunction: public AbstractFunction {
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

};




void function_linear_scale(int argc, BaseType *argv[], DDS &dds, BaseType **btpp) ;
class LinearScaleFunction: public AbstractFunction {
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




void function_version(int argc, BaseType *argv[], DDS &dds, BaseType **btpp) ;
class VersionFunction: public AbstractFunction {
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


} // namespace libdap

#endif // _ce_functions_h
