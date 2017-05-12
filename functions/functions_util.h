
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of bes, A C++ implementation of the OPeNDAP
// Hyrax data server

// Copyright (c) 2015 OPeNDAP, Inc.
// Authors: James Gallagher <jgallagher@opendap.org>
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

#ifndef FUNCTIONS_FUNCTIONS_UTIL_H_
#define FUNCTIONS_FUNCTIONS_UTIL_H_

namespace libdap {
class BaseType;
class Array;
}

// These functions are part of libdap. Use #include <util.h>. jhrg 4/28/15
//
//bool libdap::double_eq(double lhs, double rhs, double epsilon /* = 1.0e-5 */);

//string libdap::extract_string_argument(BaseType *arg) ;
//double libdap::extract_double_value(BaseType *arg) ;
//double *libdap::extract_double_array(Array *a) ;
//void libdap::extract_double_array(Array *a, vector<double> &dest) ;
//void libdap::set_array_using_double(Array *dest, double *src, int src_len) ;

namespace functions {
// These functions are defined here in the bes 'functions' module. jhrg 4/28/15

std::vector<int> parse_dims(const std::string &shape);

void check_number_type_array(libdap::BaseType *btp, unsigned int rank = 0);

unsigned int extract_uint_value(libdap::BaseType *arg);

#if 0
/// We might move these into functions_util over time if they become generally
/// useful. jhrg 5/1/15

// in LinearScaleFunction.cc
static double string_to_double(const char *val);
static double get_attribute_double_value(BaseType *var, const string &attribute);
static double get_attribute_double_value(BaseType *var, vector<string> &attributes);
static double get_missing_value(BaseType *var);
static double get_slope(BaseType *var);
static double get_y_intercept(BaseType *var);

// in TabularFunction.cc (these are methods)
typedef std::vector<unsigned long> Shape;
static Shape array_shape(Array *a);
static bool shape_matches(Array *a, const Shape &shape);
static bool dep_indep_match(const Shape &dep_shape, const Shape &indep_shape);

static unsigned long number_of_values(const Shape &shape);

static void build_columns(unsigned long n, BaseType *btp, std::vector<Array*> &arrays, Shape &shape);

static void read_values(const std::vector<Array*> &arrays);

static void build_sequence_values(const std::vector<Array*> &arrays, SequenceValues &sv);
static void combine_sequence_values(SequenceValues &dep, const SequenceValues &indep);
static void add_index_column(const Shape &indep_shape, const Shape &dep_shape, std::vector<Array*> &dep_vars);
#endif

} //namespace functions

#endif /* FUNCTIONS_FUNCTIONS_UTIL_H_ */
