// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2015 OPeNDAP, Inc.
// Authors: Dan Holloway <dholloway@opendap.org>
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

#include "config.h"

#include <cassert>

#include <sstream>
#include <vector>

#include <Type.h>
#include <BaseType.h>
#include <Byte.h>
#include <Int16.h>
#include <UInt16.h>
#include <Int32.h>
#include <UInt32.h>
#include <Float32.h>
#include <Float64.h>
#include <Str.h>
#include <Url.h>
#include <Array.h>
#include <Grid.h>
#include <Error.h>
#include <DDS.h>

#include <DMR.h>
#include <D4Group.h>
#include <D4RValue.h>

#include <debug.h>
#include <util.h>

#include <BaseTypeFactory.h>

#include <BESDebug.h>

#include "MakeMaskFunction.h"
#include "functions_util.h"

using namespace libdap;

namespace functions {

string make_mask_info =
        string("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n")
                + "<function name=\"make_array\" version=\"1.0\" href=\"http://docs.opendap.org/index.php/Server_Side_Processing_Functions#make_mask\">\n"
                + "</function>";


template <typename T>
void make_mask_helper(const vector<Array*>dims, Array *tuples, vector<dods_byte> mask)
{

    vector< vector<double> > dim_value_vecs(dims.size());

    int i = 0;  // index the dim_value_vecs vector of vectors;
    for (vector<Array*>::iterator d = dims.begin(), e = dims.end(); d != e; ++d) {
	dim_value_vecs.at(i++) = extract_double_array(*d);
    }

    // Copy the 'tuple' data to a simple vector<T>
    vector<T> data(tuples->length());
    tuples->value(&data[0]);

    // Iterate over the tuples, searching the dimensions for their values
    int nDims = dims.size();
    int nTuples = data.size() / nDims;

    bool theyAllMatched;
    vector<int> indices(nDims);
    T *tVal = &data[0];

    for (int outer = 0; outer < nTuples; outer++ ) {
	for (int inner = 0; inner < nDims; inner++) {

	    theyAllMatched = true;  // Assume all 'tuple' dimension values will match.

	    for (int i = 0; i < dim_value_vecs[inner].size(); i++) {
		if ( double_eq( (double)*tVal, dim_value_vecs[inner][i] )) {
		    indices[inner] = i;
		}
		else {
		    theyAllMatched = false;  // If any don't, then don't mask these 'tuple' locations.
	    }
	}

	if ( theyAllMatched ) {
	    offset = calculateOffset(indices);  // calculate the offset into the mask for the [i][j][...] indices
	    mask[offset] = 1;
	}
    }
}

/** Given a ...

 @param argc A count of the arguments
 @param argv An array of pointers to each argument, wrapped in a child of BaseType
 @param btpp A pointer to the return value; caller must delete.

 @return The mask variable, represented using Byte
 @exception Error Thrown if target variable is not a DAP2 Grid
**/
void function_dap2_make_mask(int argc, BaseType * argv[], DDS &, BaseType **btpp)
{
    if (argc == 0) {
        Str *response = new Str("info");
        response->set_value(make_mask_info);
        *btpp = response;
        return;
    }

    // Check for two args or more. The first two must be strings.
    DBG(cerr << "argc = " << argc << endl);
    if (argc < 4) throw Error(malformed_expr, "make_mask(target,nDims,[dim1,...],$TYPE(dim1_value0,dim2_value0,...)) requires at least four arguments.");

    string requestedTargetName = extract_string_argument(argv[0]);
    BESDEBUG("functions", "Requested target variable: " << requestedTargetName << endl);

    BaseType *btp = argv[0];

    if ( btp->type() != dods_grid_c ) {
	throw Error(malformed_expr, "make_mask(first argument must point to a DAP2 Grid variable.");
    }
    
    Grid *g = static_cast<Grid*>(btp);
    Array *a = g->get_array();

    // Create the 'mask' array using the shape of the target grid variable's array.

    vector<MaskDIM> maskDims;
    vector<string> dimNames;
    int nGridDims = 1;
    Array *mask = new Array("mask", new Byte("mask"));

    for (Array::Dim_iter i = a->dim_begin(); i != a->dim_end(); ++i) {
	mask->append_dim(a->dimension_size(i), a->dimension_name(i));
	MaskDIM *mDim = new MaskDIM;;
	mDim->size = a->dimension_size(i);
	mDim->name = a->dimension_name(i);
	mDim->offset = 0;
	//maskDims.push_back(mDim);
	nGridDims++;
    }

    vector<int> offsets;
    offsets.reserve(nGridDims);

    std::vector<int>::size_type idx = nGridDims - 1;
    int currOffset = 0;
    int sumOfPrevOffsets = 0;

    bool atBeginning = true;

    for (idx = nGridDims; idx>0; idx--) {
	
	MaskDIM *mDim = &(maskDims[idx-1]);
	
	if ( atBeginning ) {
	    currOffset = sizeof(dods_byte);
	    mDim->offset = currOffset;
	    atBeginning = false;
	}
	else {
	    currOffset = mDim->size * sumOfPrevOffsets;
	    mDim->offset = currOffset;
	}

	sumOfPrevOffsets = currOffset;
    }

    // read argv[1], the number[N] of dimension variables represented in tuples

    unsigned int nDims = extract_uint_value(argv[1]);

    // read argv[2] -> argv[2+numberOfDims]; the grid dimensions comprising mask tuples
 
    vector<Array*> dims;

    for (unsigned int i=0; i < nDims; i++) {
	
	btp = argv[2+i];
	if ( btp->type() != dods_array_c ) {
	    throw Error(malformed_expr, "make_mask(dimension-name arguments must point to a DAP2 Grid variable dimensions.");
	}
	
	int dSize; 
	Array *a = static_cast<Array*>(btpgi);
	for (Array::Dim_iter itr = a->dim_begin(); itr != a->dim_end(); ++itr) {
	    dSize = a->dimension_size(itr);
	    cerr << "dim[" << i << "] = " << a->name() << " size=" << dSize << endl;
       	}

	dims.push_back(a);
	
    }

    BESDEBUG("functions", "number of dimensions: " << dims.size() << endl);

    btp = argv[argc-1];

    if ( btp->type() != dods_array_c ) {
	throw Error(malformed_expr, "make_mask(last argument must be a special-form array..");
    }

    check_number_type_array (btp);  // Throws an exception if not a numeric type.
	
    Array *tuples = static_cast<Array*>(btp);

    switch (tuples->var()->type()) {
    // All mask values are stored in Byte DAP variables by the stock argument parser
    // except values too large; those are stored in a UInt32
    case dods_byte_c:
	//make_mask_helper<dods_byte>(dims, tuples, mask);
        cerr << "read_mask_values<dods_byte, Byte>(tuples, dims, mask);" << endl;
        break;

    case dods_int16_c:
	//make_mask_helper<dods_int16>(dims, tuples, mask);
        cerr << "read_mask_values<dods_int16, Byte>(tuples, dims, mask);" << endl;
        break;

    case dods_uint16_c:
	//make_mask_helper<dods_uint16>(dims, tuples, mask);
        cerr << "read_mask_values<dods_uint16, Byte>(tuples, dims, mask);" << endl;
        break;

    case dods_int32_c:
	//make_mask_helper<dods_int32>(dims, tuples, mask);
        cerr << "read_mask_values<dods_int32, Byte>(tuples, dims, mask);" << endl;
        break;

    case dods_uint32_c:
        // FIXME Should be UInt32 but the parser uses Int32 unless a value is too big.
	//make_mask_helper<dods_uint32>(dims, tuples, mask);
        cerr << "read_mask_values<dods_uint32, Byte>(tuples, dims, mask);" << endl;
        break;

    case dods_float32_c:
	//make_mask_helper<dods_float32>(dims, tuples, mask);
        cerr << "read_mask_values<dods_float32, Byte>(tuples, dims, mask);" << endl;
        break;

    case dods_float64_c:
	//make_mask_helper<dods_float64>(dims, tuples, mask);
        cerr << " read_mask_values<dods_float64, Byte>(tuples, dims, mask);" << endl;
        break;

    case dods_str_c:
	//make_mask_helper<dods_str>(dims, tuples, mask);
        cerr << "read_mask_values<string, Byte>(tuples, dims, mask);" << endl;
        break;

    case dods_url_c:
	//make_mask_helper<dods_url>(dims, tuples, mask);
        cerr << "read_mask_values<string, Byte>(tuples, dims, mask);" << endl;
        break;

    default:
        throw InternalErr(__FILE__, __LINE__, "Unknown type error");
    }


    unsigned int tSz = tuples->dimension_size(tuples->dim_begin());

    // Calculate the number of Tuples passed to the function.
    unsigned int numberOfTuples = tSz / nDims; 

    Array *idx = &tuples[0];
    vector<int> indices(nDims);
    int index;
 
    for (int outer = 0; outer < numberOfTuples; outer++) {

	// Foreach tuple member search the respective dimension
	//  if FOUND, return indice, if NOT_FOUND return -1;

	for (int inner = 0; inner < nDims; inner++) {
	    index = dimSearch( dims[inner], idx->value());
	    indices[inner] = index;
	}

	// Check to ensure each tuple member passed search test.

	searchFailed = false;
	for (int inner = 0; inner < nDims; inner++) {
	    if ( indices[inner] == -1 ) {
		searchFailed = true;
	    }
	}

	if ( searchFailed ) {
	    // do nothing
	}
	else {
	    indices.push_back( indice.offset );
	}
    }

    cerr << "nDims:" << nDims << " nTuples:" << numberOfTuples << endl;

    BESDEBUG("function",
	     "function_dap2_make_mask() -target " <<  requestedTargetName << " -nDims " << nDims << endl);

    //*btpp = function_linear_scale_worker(argv[0], m, b, missing, use_missing);

}

} // namespace functions
