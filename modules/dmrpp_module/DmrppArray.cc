
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

#include "config.h"

#include <string>
#include <sstream>

#include <BESError.h>
#include <BESDebug.h>

#include "Odometer.h"
#include "DmrppArray.h"
#include "DmrppUtil.h"

using namespace dmrpp;
using namespace libdap;
using namespace std;

void
DmrppArray::_duplicate(const DmrppArray &)
{
}

DmrppArray::DmrppArray(const string &n, BaseType *v) : Array(n, v, true /*is dap4*/), DmrppCommon()
{
}

DmrppArray::DmrppArray(const string &n, const string &d, BaseType *v) : Array(n, d, v, true), DmrppCommon()
{
}

BaseType *
DmrppArray::ptr_duplicate()
{
    return new DmrppArray(*this);
}

DmrppArray::DmrppArray(const DmrppArray &rhs) : Array(rhs), DmrppCommon(rhs)
{
    _duplicate(rhs);
}

DmrppArray &
DmrppArray::operator=(const DmrppArray &rhs)
{
    if (this == &rhs)
    return *this;

    dynamic_cast<Array &>(*this) = rhs; // run Constructor=

    _duplicate(rhs);
    DmrppCommon::_duplicate(rhs);

    return *this;
}

/**
 * @brief Is this Array subset?
 * @return True if the array has a projection expression, false otherwise
 */
bool
DmrppArray::is_projected()
{
    for (Dim_iter p = dim_begin(), e = dim_end(); p != e; ++p)
        if (dimension_size(p, true) != dimension_size(p, true))
            return true;

    return false;
}





void
DmrppArray::read_constrained(Odometer &odometer, Dim_iter p, unsigned long &target_index, Odometer::shape &subsetAddress){

	Dim_iter myDim = p;

	int start = this->dimension_start(myDim,true);
	int stop = this->dimension_stop(myDim,true);
	int stride = this->dimension_stride(myDim,true);

    p++;

    if(p==dim_end() && stride==1){
    	subsetAddress.push_back(start);
		unsigned int start_index = odometer.set_indices(subsetAddress);
    	subsetAddress.pop_back();

    	subsetAddress.push_back(stop);
		unsigned int stop_index = odometer.set_indices(subsetAddress);
    	subsetAddress.pop_back();

    	//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    	// @TODO Copy data block from start_index to stop_index
    	//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    	target_index += stop-start;
    }
    else {
    	for(int myDimIndex=start; myDimIndex<=stop ;myDimIndex+=stride){
    		// Is it the last dimension?
    		if(p!=dim_end()){
    			// Nope!
    			// then we recurse to the last dimension to read stuff
    			subsetAddress.push_back(myDimIndex);
    			read_constrained(odometer,p,target_index,subsetAddress);
    			subsetAddress.pop_back();
    		}
    		else {
				// We are at the last (inner most) dimension.
				// So it's time to copy values.
				subsetAddress.push_back(myDimIndex);
				unsigned int sourceIndex = odometer.set_indices(subsetAddress);

		    	//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
				// @TODO Copy value here.
		    	//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

				target_index++;
				subsetAddress.pop_back();

    		}
    	}

    }



}

// FIXME This version of read() should work for unconstrained accesses where
// we don't have to think about chunking. jhrg 11/23/16
bool
DmrppArray::read()
{
    BESDEBUG("dmrpp", "Entering " <<__PRETTY_FUNCTION__ << " for " << name() << endl);

    if (read_p())
        return true;

    // First cut at subsetting; read the whole thing and then subset that.
    unsigned long long array_nbytes = get_size(); //width();

    rbuf_size(array_nbytes);

    ostringstream range;   // range-get needs a string arg for the range
    range << get_offset() << "-" << get_offset() + get_size() - 1;

    BESDEBUG("dmrpp", "Reading  " << get_data_url() << ": " << range.str() << endl);

    curl_read_bytes(get_data_url(), range.str(), dynamic_cast<DmrppCommon*>(this));

    // Could use get_rbuf_size() in place of sizeof() for a more generic version.
    if (array_nbytes != get_bytes_read()) {
        ostringstream oss;
        oss << "DmrppArray: Wrong number of bytes read for '" << name() << "'; expected " << array_nbytes
            << " but found " << get_bytes_read() << endl;
        throw BESError(oss.str(), BES_INTERNAL_ERROR, __FILE__, __LINE__);
    }


    if (!is_projected()) {      // if there is no projection constraint
        val2buf(get_rbuf());    // yes, it's not type-safe
    }
    // else
    // Get the start, stop values for each dimension
    // Determine the size of the destination buffer (should match length() / width())
    // Allocate the dest buffer in the array
    // Use odometer code to copy data out of the rbuf and into the dest buffer of the array
    else {
        Odometer::shape full_shape, subset;
        for(Dim_iter dim=dim_begin(); dim!=dim_end(); dim++){
        	full_shape.push_back(dimension_size(dim,false));
        }
        Odometer odometer(full_shape);
        Dim_iter dimension = dim_begin();
        unsigned long target_index = 0;
        read_constrained(odometer, dimension, target_index, subset);
    }

    set_read_p(true);

    return true;
}






void DmrppArray::dump(ostream & strm) const
{
    strm << DapIndent::LMarg << "DmrppArray::dump - (" << (void *) this << ")" << endl;
    DapIndent::Indent();
    strm << DapIndent::LMarg << "offset: " << get_offset() << endl;
    strm << DapIndent::LMarg << "size:   " << get_size() << endl;
    strm << DapIndent::LMarg << "md5:    " << get_md5() << endl;
    strm << DapIndent::LMarg << "uuid:   " << get_uuid() << endl;
    Array::dump(strm);
    strm << DapIndent::LMarg << "value: " << "----" << /*d_buf <<*/ endl;
    DapIndent::UnIndent();
}
