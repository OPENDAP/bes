// This file is part of hdf5_handler an HDF5 file handler for the OPeNDAP
// data server.

// Author: Muqun Yang <myang6@hdfgroup.org> 

// Copyright (c) 2011-2016 The HDF Group, Inc. and OPeNDAP, Inc.
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
// You can contact The HDF Group, Inc. at 1800 South Oak Street,
// Suite 203, Champaign, IL 61820  
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
/// Copyright (c) 2011-2016 The HDF Group
/// 
/// All rights reserved.
///
/// 
////////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <sstream>
#include <cassert>
#include <algorithm>
#include <BESDebug.h>
#include "InternalErr.h"

#include "HDF5BaseArray.h"
#include "HDF5RequestHandler.h"
#include "ObjMemCache.h"

#if 0
BaseType *HDF5BaseArray::ptr_duplicate()
{
    return new HDF5BaseArray(*this);
}

// Always return true. 
// Data will be read from the missing coordinate variable class(HDF5GMCFMissNonLLCVArray etc.)
bool HDF5BaseArray::read()
{
    BESDEBUG("h5","Coming to HDF5BaseArray read "<<endl);
    return true;
}

#endif

// parse constraint expr. and make hdf5 coordinate point location.
// return number of elements to read. 
int
HDF5BaseArray::format_constraint (int *offset, int *step, int *count)
{
    long nels = 1;
    int id = 0;

    Dim_iter p = dim_begin ();

    while (p != dim_end ()) {

        int start = dimension_start (p, true);
        int stride = dimension_stride (p, true);
        int stop = dimension_stop (p, true);

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
    }// while (p != dim_end ())

    return nels;
}

void HDF5BaseArray::write_nature_number_buffer(int rank, int tnumelm) {

    if (rank != 1) 
        throw InternalErr(__FILE__, __LINE__, "Currently the rank of the missing field should be 1");
    
    vector<int>offset;
    vector<int>count;
    vector<int>step;
    offset.resize(rank);
    count.resize(rank);
    step.resize(rank);


    int nelms = format_constraint(&offset[0], &step[0], &count[0]);

    // Since we always assign the the missing Z dimension as 32-bit
    // integer, so no need to check the type. The missing Z-dim is always
    // 1-D with natural number 1,2,3,....
    vector<int>val;
    val.resize(nelms);

    if (nelms == tnumelm) {
        for (int i = 0; i < nelms; i++)
            val[i] = i;
        set_value((dods_int32 *) &val[0], nelms);
    }
    else {
        for (int i = 0; i < count[0]; i++)
            val[i] = offset[0] + step[0] * i;
        set_value((dods_int32 *) &val[0], nelms);
    }
}

//#if 0
void HDF5BaseArray::read_data_from_mem_cache(H5DataType h5type, const vector<size_t> &h5_dimsizes,void* buf) {

    BESDEBUG("h5", "Coming to read_data_from_mem_cache"<<endl);
    vector<int>offset;
    vector<int>count;
    vector<int>step;

    int ndims = h5_dimsizes.size();
    if(ndims == 0)
        throw InternalErr(__FILE__, __LINE__, "Currently we only support array numeric data in the cache, the number of dimension for this file is 0");
    

    offset.resize(ndims);
    count.resize(ndims);
    step.resize(ndims);
    int nelms = format_constraint (&offset[0], &step[0], &count[0]);

    // set the original position to the starting point
    vector<size_t>pos(ndims,0);
    for (int i = 0; i< ndims; i++)
        pos[i] = offset[i];


    switch (h5type) {

        case H5UCHAR:
                
        {
            vector<unsigned char> val;
            subset<unsigned char>(
                                      buf,
                                      ndims,
                                      h5_dimsizes,
                                      &offset[0],
                                      &step[0],
                                      &count[0],
                                      &val,
                                      pos,
                                      0
                                     );

            set_value ((dods_byte *) &val[0], nelms);
        } // case H5UCHAR
            break;

        case H5CHAR:
        {

            vector<char>val;
            subset<char>(
                                      buf,
                                      ndims,
                                      h5_dimsizes,
                                      &offset[0],
                                      &step[0],
                                      &count[0],
                                      &val,
                                      pos,
                                      0
                                     );

            vector<short>newval;
            newval.resize(nelms);

            for (int counter = 0; counter < nelms; counter++)
                newval[counter] = (short) (val[counter]);
            set_value ((dods_int16 *) &val[0], nelms);

        } // case H5CHAR
           break;

        case H5INT16:
        {
            vector<short> val;
            subset<short>(
                                      buf,
                                      ndims,
                                      h5_dimsizes,
                                      &offset[0],
                                      &step[0],
                                      &count[0],
                                      &val,
                                      pos,
                                      0
                                     );


            set_value ((dods_int16 *) &val[0], nelms);
        }// H5INT16
            break;


        case H5UINT16:
        {
    	    vector<unsigned short> val;
	    subset<unsigned short>(
				    buf,
				    ndims,
                                    h5_dimsizes,
                                    &offset[0],
                                    &step[0],
                                    &count[0],
                                    &val,
                                    pos,
                                    0
                                  );

               
            set_value ((dods_uint16 *) &val[0], nelms);
        } // H5UINT16
            break;

        case H5INT32:
        {
            vector<int>val;
            subset<int>(
			buf,
			ndims,
			h5_dimsizes,
			&offset[0],
			&step[0],
			&count[0],
			&val,
			pos,
			0
			);

            set_value ((dods_int32 *) &val[0], nelms);
        } // case H5INT32
            break;

        case H5UINT32:
        {
            vector<unsigned int>val;
            subset<unsigned int>(
				buf,
				ndims,
				h5_dimsizes,
				&offset[0],
				&step[0],
				&count[0],
				&val,
				pos,
				0
				);

            set_value ((dods_uint32 *) &val[0], nelms);
        }
            break;

        case H5FLOAT32:
        {
            vector<float>val;
            subset<float>(
	    		  buf,
			  ndims,
			  h5_dimsizes,
			  &offset[0],
			  &step[0],
			  &count[0],
			  &val,
			  pos,
			  0
			  );
            set_value ((dods_float32 *) &val[0], nelms);
        }
            break;


        case H5FLOAT64:
        {

            vector<double>val;
            subset<double>(
		    	    buf,
	    		    ndims,
    			    h5_dimsizes,
			    &offset[0],
    			    &step[0],
    			    &count[0],
			    &val,
			    pos,
			    0
			    );
            set_value ((dods_float64 *) &val[0], nelms);
        } // case H5FLOAT64
            break;

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
    int start[],
    int stride[],
    int edge[],
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
            //poutput->push_back(input[HDF5CFUtil::INDEX_nD_TO_1D( dim, pos)]);
        }
    } // end of for
    return 0;
} // end of template<typename T> static int subset

size_t HDF5BaseArray::INDEX_nD_TO_1D (const std::vector < size_t > &dims,
                                 const std::vector < size_t > &pos){
    //
    //  int a[10][20][30];  // & a[1][2][3] == a + (20*30+1 + 30*2 + 1 *3);
    //  int b[10][2]; // &b[1][1] == b + (2*1 + 1);
    // 
    if(dims.size () != pos.size ())
        throw InternalErr(__FILE__,__LINE__,"dimension error in INDEX_nD_TO_1D routine.");
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
check_str_sect_in_list(const vector<string>&str_list, const string &cur_str,const char sep) {

    string ret_str;
    string::size_type start = 0, end = 0;
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
    //if(ret_str != "") {
    //    if(ret_str == cur_str.substr(cur_str.find_last_of(sep)+1))  
    //        ret_str ="";
    //}

    return ret_str;

}

// This routine will check if there is any sub-string of the fullpath(fname+varname) that is exactly the subset of the fullpath with the same ending 
// of the fullpath is contained in the slist.
// Examples: slist contains { /foo1/foovar foovar2 } fname is /temp/myfile/foo1/ varname is foovar. The rotuine will return true. 
//                                                   fname is /myfile/foo2/  varname is foovar. The routine will return false.
bool HDF5BaseArray::
check_var_cache_files(const vector<string>&slist, const string &fname,const string &varname) {

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
        

    for(int i = 0; i<slist.size();i++) {
//cerr<<"fullpath is "<<fullpath <<endl;
//cerr<<"slist[i] is "<<slist[i] <<endl;
//cerr<<"fullpath - slist size"<<fullpath.size() -slist[i].size()<<endl;
//cerr<<"fullpath.rfind(slist[i] is "<<fullpath.rfind(slist[i]) <<endl;
        if(fullpath.rfind(slist[i])==(fullpath.size()-slist[i].size())){
//cerr<<"find the list "<<endl;
            ret_value = true;
            break;
        }
    }
    return ret_value;
}

// Handle data when memory cache is turned on.
void HDF5BaseArray::
handle_data_with_mem_cache(H5DataType h5_dtype, size_t total_elems,const short cache_flag, const string & cache_key) {     

    // 
    ObjMemCache * mem_data_cache= NULL;
    if(1 == cache_flag) 
        mem_data_cache = HDF5RequestHandler::get_srdata_mem_cache();
    else if(cache_flag > 1) {
        mem_data_cache = HDF5RequestHandler::get_lrdata_mem_cache();
//cerr<<"coming to the large metadata cache "<<endl;
//cerr<<"The cache key is "<<cache_key <<endl;

// dump the values in the cache
//mem_data_cache->dump(cerr);
    }


    if(mem_data_cache == NULL)
        throw InternalErr(__FILE__,__LINE__,"The memory data cache should NOT be NULL.");

    HDF5DataMemCache* mem_cache_ptr = static_cast<HDF5DataMemCache*>(mem_data_cache->get(cache_key));
    if(mem_cache_ptr) {
//cerr<<"coming to the cache hit"<<endl;
        
        BESDEBUG("h5","Cache flag: 1 small data cache, 2 large data cache genenral"
                 <<" 3 large data cache common dir, 4 large data cache real var" <<endl);
       
        BESDEBUG("h5","Data Memory Cache hit, the variable name is "<<name() <<". The cache flag is "<< cache_flag<<endl);

        //const string var_name = mem_cache_ptr->get_varname();

        // Obtain the buffer and do subsetting
        const size_t var_size = mem_cache_ptr->get_var_buf_size();
        if(!var_size) 
            throw InternalErr(__FILE__,__LINE__,"The cached data buffer size is 0.");
        else {

            void *buf = mem_cache_ptr->get_var_buf();

            // Obtain dimension size info.
    	    vector<size_t> dim_sizes;
	    Dim_iter i_dim = dim_begin();
	    Dim_iter i_enddim = dim_end();
	    while (i_dim != i_enddim) {
	 	dim_sizes.push_back(dimension_size(i_dim));
		++i_dim;
	    }
            // read data from the memory cache
     	    read_data_from_mem_cache(h5_dtype,dim_sizes,buf);
	}
    }
    else{ 
//cerr<<"coming to add the cache  "<<endl;

        BESDEBUG("h5","Cache flag: 1 small data cache, 2 large data cache genenral"
                 <<" 3 large data cache common dir, 4 large data cache real var" <<endl);
       
        BESDEBUG("h5","Data Memory added to the cache, the variable name is "<<name() <<". The cache flag is "<< cache_flag<<endl);

 	vector <char> buf;
 	if(total_elems == 0)
     	    throw InternalErr(__FILE__,__LINE__,"The total number of elements is 0.");

       	buf.resize(total_elems*HDF5CFUtil::H5_numeric_atomic_type_size(h5_dtype));

        // This routine will read the data, send it to the DAP and save the buf to the cache.
  	read_data_NOT_from_mem_cache(true,&buf[0]);
            
        // Create a new cache element.
    	//HDF5DataMemCache* new_mem_cache = new HDF5DataMemCache(varname);
    	HDF5DataMemCache* new_mem_cache_ele = new HDF5DataMemCache();
       	new_mem_cache_ele->set_databuf(buf);

        // Add this entry to the cache list
       	mem_data_cache->add(new_mem_cache_ele, cache_key);
    }

    return;
}



