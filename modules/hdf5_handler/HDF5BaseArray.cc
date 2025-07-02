// This file is part of hdf5_handler an HDF5 file handler for the OPeNDAP
// data server.

// Author: Kent Yang <myang6@hdfgroup.org> 

// Copyright (c) 2011-2023 The HDF Group, Inc. and OPeNDAP, Inc.
//
// This is free software; you can redistribute it and/or modify it under the
// terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 2.1 of the License, or (at your
// option) any later version.
//
// This software is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
// License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
// You can contact The HDF Group, Inc. at 410 E University Ave,
// Suite 200, Champaign, IL 61820  
///////////////////////////////////////////////////////////////////////////////
/// \file HDF5BaseArray.cc
/// \brief Implementation of a helper class that aims to reduce code redundence for different special CF derived array class
/// For example, format_constraint has been called by different CF derived array class,
/// and write_nature_number_buffer has also be used by missing variables on both 
/// HDF-EOS5 and generic HDF5 products. 
/// 
/// This class converts HDF5 array type into DAP array.
/// 
/// \author Kent Yang       (myang6@hdfgroup.org)
/// Copyright (c) 2011-2023 The HDF Group
/// 
/// All rights reserved.
///
/// 
////////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <sstream>
#include <cassert>
#include <algorithm>
#include <memory>
#include <BESDebug.h>
#include <libdap/InternalErr.h>

#include "HDF5BaseArray.h"
#include "HDF5RequestHandler.h"
#include "ObjMemCache.h"

using namespace std;
using namespace libdap;

// parse constraint expr. and make hdf5 coordinate point location.
// return number of elements to read. 
int64_t
HDF5BaseArray::format_constraint (int64_t *offset, int64_t *step, int64_t *count)
{
    int64_t nels = 1;
    int id = 0;

    Dim_iter p = dim_begin ();

    while (p != dim_end ()) {

        int64_t start = dimension_start_ll (p, true);
        int64_t stride = dimension_stride_ll (p, true);
        int64_t stop = dimension_stop_ll (p, true);

        // Check for illegal  constraint
        if (start > stop) {
            ostringstream oss;
            oss << "Array/Grid hyperslab start point "<< start <<
                   " is greater than stop point " <<  stop <<".";
            throw Error(malformed_expr, oss.str());
        }

        offset[id] = start;
        step[id] = stride;
        count[id] = ((stop - start) / stride) + 1;      // count of elements
        nels *= count[id];              // total number of values for variable

        BESDEBUG ("h5",
                         "=format_constraint():"
                         << "id=" << id << " offset=" << offset[id]
                         << " step=" << step[id]
                         << " count=" << count[id]
                         << endl);

        id++;
        p++;
    }// "while (p != dim_end ())"

    return nels;
}

void HDF5BaseArray::write_nature_number_buffer(int rank, int64_t tnumelm) {

    if (rank != 1) { 
        string msg =  "Currently the rank of the missing field should be 1";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }

    if (tnumelm >DODS_INT_MAX) {
        string msg = "Currently the maximum number for this dimension is less than DODS_INT_MAX";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }
    
    vector<int64_t>offset;
    vector<int64_t>count;
    vector<int64_t>step;
    offset.resize(rank);
    count.resize(rank);
    step.resize(rank);


    int64_t nelms = format_constraint(offset.data(), step.data(), count.data());

    // Since we always assign the missing Z dimension as 32-bit
    // integer, so no need to check the type. The missing Z-dim is always
    // 1-D with natural number 1,2,3,....
    vector<int>val;
    val.resize(nelms);

    if (nelms == tnumelm) {
        for (int64_t i = 0; i < nelms; i++)
            val[i] = (int)i;
        set_value_ll(val.data(), nelms);
    }
    else {
        for (int64_t i = 0; i < count[0]; i++)
            val[i] = (int)(offset[0] + step[0] * i);
        set_value_ll(val.data(), nelms);
    }
}

//#if 0
void HDF5BaseArray::read_data_from_mem_cache(H5DataType h5type, const vector<size_t> &h5_dimsizes,void* buf,const bool is_dap4){

    BESDEBUG("h5", "Coming to read_data_from_mem_cache"<<endl);
    vector<int64_t>offset;
    vector<int64_t>count;
    vector<int64_t>step;

    auto ndims = (int)(h5_dimsizes.size());
    if(ndims == 0) {
        string msg = "Currently we only support array numeric data in the cache, the number of dimension for this file is 0.";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }
    

    offset.resize(ndims);
    count.resize(ndims);
    step.resize(ndims);
    int64_t nelms = format_constraint (offset.data(), step.data(), count.data());

    // set the original position to the starting point
    vector<size_t>pos(ndims,0);
    for (int64_t i = 0; i< ndims; i++)
        pos[i] = offset[i];


    switch (h5type) {

        case H5UCHAR:
                
        {
            vector<unsigned char> val;
            subset<unsigned char>(
                                      buf,
                                      ndims,
                                      h5_dimsizes,
                                      offset.data(),
                                      step.data(),
                                      count.data(),
                                      &val,
                                      pos,
                                      0
                                     );

            set_value_ll ((dods_byte *) val.data(), nelms);
        } // case H5UCHAR
            break;

        case H5CHAR:
        {

            vector<char>val;
            subset<char>(
                                      buf,
                                      ndims,
                                      h5_dimsizes,
                                      offset.data(),
                                      step.data(),
                                      count.data(),
                                      &val,
                                      pos,
                                      0
                                     );

            if(false == is_dap4) {

                vector<short>newval;
                newval.resize(nelms);

                for (int64_t counter = 0; counter < nelms; counter++)
                    newval[counter] = (short) (val[counter]);
                set_value_ll ((dods_int16 *) val.data(), nelms);
            }
            else 
                set_value_ll ((dods_int8 *) val.data(), nelms);


        } // case H5CHAR
           break;

        case H5INT16:
        {
            vector<short> val;
            subset<short>(
                                      buf,
                                      ndims,
                                      h5_dimsizes,
                                      offset.data(),
                                      step.data(),
                                      count.data(),
                                      &val,
                                      pos,
                                      0
                                     );


            set_value_ll (val.data(), nelms);
        }// H5INT16
            break;


        case H5UINT16:
        {
    	    vector<unsigned short> val;
	    subset<unsigned short>(
				    buf,
				    ndims,
                                    h5_dimsizes,
                                    offset.data(),
                                    step.data(),
                                    count.data(),
                                    &val,
                                    pos,
                                    0
                                  );

               
            set_value_ll (val.data(), nelms);
        } // H5UINT16
            break;

        case H5INT32:
        {
            vector<int>val;
            subset<int>(
			buf,
			ndims,
			h5_dimsizes,
			offset.data(),
			step.data(),
			count.data(),
			&val,
			pos,
			0
			);

            set_value_ll (val.data(), nelms);
        } // case H5INT32
            break;

        case H5UINT32:
        {
            vector<unsigned int>val;
            subset<unsigned int>(
				buf,
				ndims,
				h5_dimsizes,
				offset.data(),
				step.data(),
				count.data(),
				&val,
				pos,
				0
				);

            set_value_ll (val.data(), nelms);
        }
            break;
        // Add the code for the CF option DAP4 support
        // For the CF option DAP2 support, the code will
        // not come here since 64-integer will be ignored 
        // in DAP2.
        case H5INT64:
        {
            vector<long long>val;
            subset<long long>(
			buf,
			ndims,
			h5_dimsizes,
			offset.data(),
			step.data(),
			count.data(),
			&val,
			pos,
			0
			);

            set_value_ll ((dods_int64 *) val.data(), nelms);
        } // case H5INT64
            break;

        case H5UINT64:
        {
            vector<unsigned long long>val;
            subset<unsigned long long>(
				buf,
				ndims,
				h5_dimsizes,
				offset.data(),
				step.data(),
				count.data(),
				&val,
				pos,
				0
				);

            set_value_ll ((dods_uint64 *) val.data(), nelms);
        }
            break;


        case H5FLOAT32:
        {
            vector<float>val;
            subset<float>(
	    		  buf,
			  ndims,
			  h5_dimsizes,
			  offset.data(),
			  step.data(),
			  count.data(),
			  &val,
			  pos,
			  0
			  );
            set_value_ll (val.data(), nelms);
        }
            break;


        case H5FLOAT64:
        {

            vector<double>val;
            subset<double>(
		    	    buf,
	    		    ndims,
    			    h5_dimsizes,
			    offset.data(),
    			    step.data(),
    			    count.data(),
			    &val,
			    pos,
			    0
			    );
            set_value_ll (val.data(), nelms);
        } // case H5FLOAT64
            break;

        default: {
           string msg = "Non-supported datatype for the variable " + name() + ".";
           throw InternalErr(__FILE__,__LINE__, msg);
        }
    }
}

//! Getting a subset of a variable
//
//      \param input Input variable
//       \param dim dimension info of the input
//       \param start start indexes of each dim
//       \param stride stride of each dim
//       \param edge count of each dim
//       \param poutput output variable
// 	\parrm index dimension index
//       \return 0 if successful. -1 otherwise.
//
template<typename T>  
int HDF5BaseArray::subset(
    void* input,
    int rank,
    const vector<size_t> & dim,
    int64_t start[],
    int64_t stride[],
    int64_t edge[],
    vector<T> *poutput,
    vector<size_t>& pos,
    int index)
{
    for(int k=0; k<edge[index]; k++) 
    {	
        pos[index] = start[index] + k*stride[index];
        if(index+1<rank)
            subset(input, rank, dim, start, stride, edge, poutput,pos,index+1);			
        if(index==rank-1)
        {
            size_t cur_pos = INDEX_nD_TO_1D( dim, pos);
            void* tempbuf = (void*)((char*)input+cur_pos*sizeof(T));
            poutput->push_back(*(static_cast<T*>(tempbuf)));
            //"poutput->push_back(input[HDF5CFUtil::INDEX_nD_TO_1D( dim, pos)]);"
        }
    } // end of for
    return 0;
} // end of template<typename T> static int subset

size_t HDF5BaseArray::INDEX_nD_TO_1D (const std::vector < size_t > &dims,
                                 const std::vector < size_t > &pos) const {
    //
    //  "int a[10][20][30]  // & a[1][2][3] == a + (20*30+1 + 30*2 + 1 *3)"
    //  "int b[10][2] // &b[1][1] == b + (2*1 + 1)"
    // 
    if(dims.size () != pos.size ()) {
        string msg = "dimension error in INDEX_nD_TO_1D routine.";
        throw InternalErr(__FILE__,__LINE__, msg);
    }
    size_t sum = 0;
    size_t  start = 1;

    for (size_t p = 0; p < pos.size (); p++) {
        size_t m = 1;

        for (size_t j = start; j < dims.size (); j++)
            m *= dims[j];
        sum += m * pos[p];
        start++;
    }
    return sum;
}

// This routine will check if any section(separated by sep) of string cur_str is inside the vector str_list. 
// The first found string will be returned or empty string will return if not found in the whole cur_str.
string HDF5BaseArray::
check_str_sect_in_list(const vector<string>&str_list, const string &cur_str,const char sep) const {

    string ret_str;
    string::size_type start = 0;
    string::size_type end = 0;
    // Obtain the ret_str value
    // The cur_str will be chopped into tokens separated by sep.
    while ((end = cur_str.find(sep, start)) != string::npos) {
        if(std::find(str_list.begin(),str_list.end(),cur_str.substr(start,end-start))!=
           str_list.end()) {
           ret_str = cur_str.substr(start,end-start);
           break;
        }
        start = end + 1;
    }

    // We will not include the last sect (rightmost sect) of cur_str.
#if 0
    //if(ret_str != "") {
    //    if(ret_str == cur_str.substr(cur_str.find_last_of(sep)+1))  
    //        ret_str ="";
    //}
    //
#endif

    return ret_str;

}

// This routine will check if there is any sub-string of the fullpath(fname+varname) that is exactly the subset of the fullpath with the same ending 
// of the fullpath is contained in the slist.
// Examples: slist contains { /foo1/foovar foovar2 } fname is /temp/myfile/foo1/ varname is foovar. The rotuine will return true. 
//                                                   fname is /myfile/foo2/  varname is foovar. The routine will return false.
bool HDF5BaseArray::
check_var_cache_files(const vector<string>&slist, const string &fname,const string &varname) const {

    bool ret_value = false;
    if(fname=="" || varname=="")
        return ret_value;

    string fullpath;

    if(fname[fname.size()-1] == '/') {
        if(varname[0]!='/')
            fullpath = fname+varname;
        else
            fullpath = fname.substr(0,fname.size()-1)+varname;
    }
    else {
        if(varname[0]!='/')
            fullpath = fname+'/'+varname;
        else
            fullpath = fname+varname;
    }
        

    for(unsigned int i = 0; i<slist.size();i++) {
#if 0
//cerr<<"fullpath is "<<fullpath <<endl;
//cerr<<"slist[i] is "<<slist[i] <<endl;
//cerr<<"fullpath - slist size"<<fullpath.size() -slist[i].size()<<endl;
//cerr<<"fullpath.rfind(slist[i] is "<<fullpath.rfind(slist[i]) <<endl;
#endif
        if(fullpath.rfind(slist[i])==(fullpath.size()-slist[i].size())){
            ret_value = true;
            break;
        }
    }
    return ret_value;
}

// Handle data when memory cache is turned on.
void HDF5BaseArray::
handle_data_with_mem_cache(H5DataType h5_dtype, size_t total_elems,const short cache_flag, const string & cache_key, const bool is_dap4) {     

    // 
    ObjMemCache * mem_data_cache= nullptr;
    if(1 == cache_flag) 
        mem_data_cache = HDF5RequestHandler::get_srdata_mem_cache();
    else if(cache_flag > 1) {
        mem_data_cache = HDF5RequestHandler::get_lrdata_mem_cache();

#if 0
//cerr<<"coming to the large metadata cache "<<endl;
//cerr<<"The cache key is "<<cache_key <<endl;

// dump the values in the cache,keep this line to check if memory cache works.
//mem_data_cache->dump(cerr);
#endif

    }


    if (mem_data_cache == nullptr) {
        string msg = "The memory data cache should NOT be nullptr.";
        throw InternalErr(__FILE__,__LINE__, msg);
    }

    auto mem_cache_ptr = static_cast<HDF5DataMemCache*>(mem_data_cache->get(cache_key));
    if(mem_cache_ptr) {
        
        BESDEBUG("h5","Cache flag: 1 small data cache, 2 large data cache general"
                 <<" 3 large data cache common dir, 4 large data cache real var" <<endl);
       
        BESDEBUG("h5","Data Memory Cache hit, the variable name is "<<name() <<". The cache flag is "<< cache_flag<<endl);

#if 0
        //const string var_name = mem_cache_ptr->get_varname();
#endif

        // Obtain the buffer and do subsetting
        const size_t var_size = mem_cache_ptr->get_var_buf_size();
        if(!var_size) { 
            string msg = "The cached data buffer size is 0.";
            throw InternalErr(__FILE__,__LINE__, msg);
        }
        else {

            void *buf = mem_cache_ptr->get_var_buf();

            // Obtain dimension size info.
    	    vector<size_t> dim_sizes;
	    Dim_iter i_dim = dim_begin();
	    Dim_iter i_enddim = dim_end();
	    while (i_dim != i_enddim) {
	 	dim_sizes.push_back(dimension_size_ll(i_dim));
		++i_dim;
	    }
            // read data from the memory cache
     	    read_data_from_mem_cache(h5_dtype,dim_sizes,buf,is_dap4);
	}
    }
    else{ 

        BESDEBUG("h5","Cache flag: 1 small data cache, 2 large data cache genenral"
                 <<" 3 large data cache common dir, 4 large data cache real var" <<endl);
       
        BESDEBUG("h5","Data Memory added to the cache, the variable name is "<<name() <<". The cache flag is "<< cache_flag<<endl);

        vector <char> buf;
        if (total_elems == 0) {
            string msg = "The total number of elements is 0.";
            throw InternalErr(__FILE__,__LINE__, msg);
        }

        buf.resize(total_elems*HDF5CFUtil::H5_numeric_atomic_type_size(h5_dtype));

        // This routine will read the data, send it to the DAP and save the buf to the cache.
        read_data_NOT_from_mem_cache(true,buf.data());
            
        // Create a new cache element.

        auto new_mem_cache_ele_unique = make_unique<HDF5DataMemCache>();
        auto new_mem_cache_ele = new_mem_cache_ele_unique.release();
       	new_mem_cache_ele->set_databuf(buf);

        // Add this entry to the cache list
       	mem_data_cache->add(new_mem_cache_ele, cache_key);
    }

}

BaseType* HDF5BaseArray::h5cfdims_transform_to_dap4(D4Group *grp) {

    if(grp == nullptr)
        return nullptr;
    Array *dest = static_cast<HDF5BaseArray*>(ptr_duplicate());

    // If there is just a size, don't make
    // a D4Dimension (In DAP4 you cannot share a dimension unless it has
    // a name). jhrg 3/18/14

    D4Dimensions *grp_dims = grp->dims();
    for (Array::Dim_iter dap2_dim = dest->dim_begin(), e = dest->dim_end(); dap2_dim != e; ++dap2_dim) {
        if (!(*dap2_dim).name.empty()) {

            // If a D4Dimension with the name already exists, use it.
            D4Dimension *d4_dim = grp_dims->find_dim((*dap2_dim).name);
            if (!d4_dim) {
                auto d4_dim_unique = make_unique<D4Dimension>((*dap2_dim).name, (*dap2_dim).size);
                d4_dim = d4_dim_unique.release();
                grp_dims->add_dim_nocopy(d4_dim);
            }
            // At this point d4_dim's name and size == those of (*d) so just set
            // the D4Dimension pointer, so it matches the one in the D4Group.
            (*dap2_dim).dim = d4_dim;
        }
    }

    return dest;

}



