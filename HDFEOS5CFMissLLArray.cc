// This file is part of the hdf5_handler implementing for the CF-compliant
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

/////////////////////////////////////////////////////////////////////////////
/// \file HDFEOS5CFMissLLArray.cc
/// \brief The implementation of the retrieval of the missing lat/lon values for HDF-EOS5 products
///
/// \author Kent Yang <myang6@hdfgroup.org>
///
/// Copyright (C) 2011-2016 The HDF Group
///
/// All rights reserved.

#include "config_hdf5.h"
#include <iostream>
#include <sstream>
#include <cassert>
#include <BESDebug.h>
#include "InternalErr.h"
//#include <Array.h>

#include "HDFEOS5CFMissLLArray.h"
#include "HDF5RequestHandler.h"


BaseType *HDFEOS5CFMissLLArray::ptr_duplicate()
{
    return new HDFEOS5CFMissLLArray(*this);
}

bool HDFEOS5CFMissLLArray::read()
{

    BESDEBUG("h5","Coming to HDFEOS5CFMissLLArray read "<<endl);
    if(NULL == HDF5RequestHandler::get_lrdata_mem_cache())
        read_data_NOT_from_mem_cache(false,NULL);
    else {
	vector<string> cur_lrd_non_cache_dir_list;                                      
        HDF5RequestHandler::get_lrd_non_cache_dir_list(cur_lrd_non_cache_dir_list);     
                                                                                                    
        string cache_key;
        // Check if this file is included in the non-cache directory                    
        if( (cur_lrd_non_cache_dir_list.size() == 0) ||                                 
    	    ("" == check_str_sect_in_list(cur_lrd_non_cache_dir_list,filename,'/'))) {  
                short cache_flag = 2;
		vector<string> cur_cache_dlist;                                                         
		HDF5RequestHandler::get_lrd_cache_dir_list(cur_cache_dlist);                            
		string cache_dir = check_str_sect_in_list(cur_cache_dlist,filename,'/');                
		if(cache_dir != ""){                                                                     
		    cache_key = cache_dir + varname;                                                    
                    cache_flag = 3;
                }
		else                                                                                    
		    cache_key = filename + varname;     

                // Need to obtain the total number of elements.
                // Currently only trivial geographic projection is supported.
                // So the total number of elements for LAT is ydimsize,
                //    the total number of elements for LON is xdimsize.
                if(cvartype == CV_LAT_MISS)
                    handle_data_with_mem_cache(H5FLOAT32,(size_t)ydimsize,cache_flag,cache_key);
                else 
                    handle_data_with_mem_cache(H5FLOAT32,(size_t)xdimsize,cache_flag,cache_key);
        }
        else 
	    read_data_NOT_from_mem_cache(false,NULL);
    }
                                                                                                    
                                                   
    return true;
}

void HDFEOS5CFMissLLArray::read_data_NOT_from_mem_cache(bool add_cache,void*buf){

    BESDEBUG("h5","Coming to read_data_NOT_from_mem_cache "<<endl);

    // First handle geographic projection. No GCTP is needed.
    if(eos5_projcode == HE5_GCTP_GEO) {
        read_data_NOT_from_mem_cache_geo(add_cache,buf);
        return;
    }

    int nelms = -1;
    vector<int>offset;
    vector<int>count;
    vector<int>step;


    if (rank <=  0) 
       throw InternalErr (__FILE__, __LINE__,
                          "The number of dimension of this variable should be greater than 0");
    else {

         offset.resize(rank);
         count.resize(rank);
         step.resize(rank);
         nelms = format_constraint (&offset[0], &step[0], &count[0]);
    }

    if (nelms <= 0) 
       throw InternalErr (__FILE__, __LINE__,
                          "The number of elments is negative.");

    double upleft[2];
    double lowright[2];
    vector<int>rows;
    vector<int>cols;
    vector<double>lon;
    vector<double>lat;
    rows.resize(xdimsize*ydimsize);
    cols.resize(xdimsize*ydimsize);
    lon.resize(xdimsize*ydimsize);
    lat.resize(xdimsize*ydimsize);

    upleft[0] = point_left;
    upleft[1] = point_upper;
    lowright[0] = point_right;
    lowright[1] = point_lower;


    int i = 0, j = 0, k = 0;

    int r = -1;

    for (k = j = 0; j < ydimsize; ++j) {
        for (i = 0; i < xdimsize; ++i) {
            rows[k] = j;
            cols[k] = i;
            ++k;
        }
    }

    BESDEBUG("h5", " Before calling GDij2ll, check all projection parameters. "  << endl);
    BESDEBUG("h5", " eos5_zone is "  << eos5_zone <<endl);
    BESDEBUG("h5", " eos5_params[0] is "  << eos5_params[0] <<endl);
    BESDEBUG("h5", " eos5_params[1] is "  << eos5_params[1] <<endl);
    BESDEBUG("h5", " eos5_sphere is "  << eos5_sphere <<endl);
    BESDEBUG("h5", " xdimsize is "  << xdimsize <<endl);
    BESDEBUG("h5", " ydimsize is "  << ydimsize <<endl);
    BESDEBUG("h5", " eos5_pixelreg is "  << eos5_pixelreg <<endl);
    BESDEBUG("h5", " eos5_origin is "  << eos5_origin <<endl);
    BESDEBUG("h5", " upleft[0] is "  << upleft[0] <<endl);
    BESDEBUG("h5", " upleft[1] is "  << upleft[1] <<endl);
    BESDEBUG("h5", " lowright[0] is "  << lowright[0] <<endl);
    BESDEBUG("h5", " lowright[1] is "  << lowright[1] <<endl);
    
    cerr<< " eos5_params[0] is "  << eos5_params[0] <<endl;
    cerr<< " eos5_params[1] is "  << eos5_params[1] <<endl;
    cerr<< " eos5_sphere is "  << eos5_sphere <<endl;
    cerr<<  " eos5_zone is "  << eos5_zone <<endl;
#if 0
    cerr<< " Before calling GDij2ll, check all projection parameters. "  << endl;
    cerr<<  " eos5_zone is "  << eos5_zone <<endl;
    cerr<< " eos5_params[0] is "  << eos5_params[0] <<endl;
    cerr<< " eos5_params[1] is "  << eos5_params[1] <<endl;
    BESDEBUG("h5", " xdimsize is "  << xdimsize <<endl);
    BESDEBUG("h5", " ydimsize is "  << ydimsize <<endl);
    BESDEBUG("h5", " eos5_pixelreg is "  << eos5_pixelreg <<endl);
    BESDEBUG("h5", " eos5_origin is "  << eos5_origin <<endl);
    BESDEBUG("h5", " upleft[0] is "  << upleft[0] <<endl);
    BESDEBUG("h5", " upleft[1] is "  << upleft[1] <<endl);
    BESDEBUG("h5", " lowright[0] is "  << lowright[0] <<endl);
    BESDEBUG("h5", " lowright[1] is "  << lowright[1] <<endl);
#endif
 
    r = GDij2ll (eos5_projcode, eos5_zone, &eos5_params[0], eos5_sphere, xdimsize, ydimsize, upleft, lowright,
                 xdimsize * ydimsize, &rows[0], &cols[0], &lon[0], &lat[0], eos5_pixelreg, eos5_origin);
    if (r != 0) {
        ostringstream eherr;
        eherr << "cannot calculate grid latitude and longitude";
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }

    BESDEBUG("h5", " The first value of lon is "  << lon[0] <<endl);
    BESDEBUG("h5", " The first value of lat is "  << lat[0] <<endl);

    vector<size_t>pos(rank,0);
    for (int i = 0; i< rank; i++)
        pos[i] = offset[i];

    vector<size_t>dimsizes;
    dimsizes.push_back(ydimsize);
    dimsizes.push_back(xdimsize);
    int total_elms = xdimsize*ydimsize;

    
    if(CV_LON_MISS == cvartype) {
        if(total_elms == nelms)
            set_value((dods_float64 *)&lon[0],total_elms);
        else {
            vector<double>val;
            subset<double>(
                           &lon[0],
                           rank,
                           dimsizes,
                           &offset[0],
                           &step[0],
                           &count[0],
                           &val,
                           pos,
                           0);
            set_value((dods_float64 *)&val[0],nelms);              
        }
    }
    else if(CV_LAT_MISS == cvartype) {

        if(total_elms == nelms)
            set_value((dods_float64 *)&lat[0],total_elms);
        else {
            vector<double>val;
            subset<double>(
                           &lat[0],
                           rank,
                           dimsizes,
                           &offset[0],
                           &step[0],
                           &count[0],
                           &val,
                           pos,
                           0);
            set_value((dods_float64 *)&val[0],nelms);              
        }
    }
}

void HDFEOS5CFMissLLArray::read_data_NOT_from_mem_cache_geo(bool add_cache,void*buf){

    BESDEBUG("h5","Coming to read_data_NOT_from_mem_cache_geo "<<endl);
    int nelms = -1;
    vector<int>offset;
    vector<int>count;
    vector<int>step;


    if (rank <=  0) 
       throw InternalErr (__FILE__, __LINE__,
                          "The number of dimension of this variable should be greater than 0");
    else {

         offset.resize(rank);
         count.resize(rank);
         step.resize(rank);
         nelms = format_constraint (&offset[0], &step[0], &count[0]);
    }

    if (nelms <= 0) 
       throw InternalErr (__FILE__, __LINE__,
                          "The number of elments is negative.");

    float start = 0.0;
    float end   = 0.0;

    vector<float>val;
    val.resize(nelms);
    

    if (CV_LAT_MISS == cvartype) {
        
	if (HE5_HDFE_GD_UL == eos5_origin || HE5_HDFE_GD_UR == eos5_origin) {

	    start = point_upper;
	    end   = point_lower; 

	}
	else {// (gridorigin == HE5_HDFE_GD_LL || gridorigin == HE5_HDFE_GD_LR)
        
	    start = point_lower;
	    end = point_upper;
	}

	if(ydimsize <=0) 
	    throw InternalErr (__FILE__, __LINE__,
			    "The number of elments should be greater than 0.");
           
	float lat_step = (end - start) /ydimsize;

	// Now offset,step and val will always be valid. line 74 and 85 assure this.
	if ( HE5_HDFE_CENTER == eos5_pixelreg ) {
	    for (int i = 0; i < nelms; i++)
		val[i] = ((float)(offset[0]+i*step[0] + 0.5f) * lat_step + start) / 1000000.0;

            // If the memory cache is turned on, we have to save all values to the buf
            if(add_cache == true) {
            	vector<float>total_val;
		total_val.resize(ydimsize);
                for (int total_i = 0; total_i < ydimsize; total_i++)
		    total_val[total_i] = ((float)(total_i + 0.5f) * lat_step + start) / 1000000.0;
                // Note: the float is size 4 
                memcpy(buf,&total_val[0],4*ydimsize);
            }

	}
	else { // HE5_HDFE_CORNER 
	    for (int i = 0; i < nelms; i++)
		val[i] = ((float)(offset[0]+i * step[0])*lat_step + start) / 1000000.0;

            // If the memory cache is turned on, we have to save all values to the buf
            if(add_cache == true) {
            	vector<float>total_val;
		total_val.resize(ydimsize);
                for (int total_i = 0; total_i < ydimsize; total_i++)
		    total_val[total_i] = ((float)(total_i) * lat_step + start) / 1000000.0;
                // Note: the float is size 4 
                memcpy(buf,&total_val[0],4*ydimsize);
            }

	}
    }

    if (CV_LON_MISS == cvartype) {

	if (HE5_HDFE_GD_UL == eos5_origin || HE5_HDFE_GD_LL == eos5_origin) {

	    start = point_left;
	    end   = point_right; 

	}
	else {// (gridorigin == HE5_HDFE_GD_UR || gridorigin == HE5_HDFE_GD_LR)
        
	    start = point_right;
	    end = point_left;
	}

	if(xdimsize <=0) 
	    throw InternalErr (__FILE__, __LINE__,
			"The number of elments should be greater than 0.");
	float lon_step = (end - start) /xdimsize;

	if (HE5_HDFE_CENTER == eos5_pixelreg) {
	    for (int i = 0; i < nelms; i++)
		val[i] = ((float)(offset[0] + i *step[0] + 0.5f) * lon_step + start ) / 1000000.0;

            // If the memory cache is turned on, we have to save all values to the buf
            if(add_cache == true) {
            	vector<float>total_val;
		total_val.resize(xdimsize);
                for (int total_i = 0; total_i < xdimsize; total_i++)
		    total_val[total_i] = ((float)(total_i+0.5f) * lon_step + start) / 1000000.0;
                // Note: the float is size 4 
                memcpy(buf,&total_val[0],4*xdimsize);
            }

	}
	else { // HE5_HDFE_CORNER 
	    for (int i = 0; i < nelms; i++)
		val[i] = ((float)(offset[0]+i*step[0]) * lon_step + start) / 1000000.0;

            // If the memory cache is turned on, we have to save all values to the buf
            if(add_cache == true) {
            	vector<float>total_val;
		total_val.resize(xdimsize);
                for (int total_i = 0; total_i < xdimsize; total_i++)
		    total_val[total_i] = ((float)(total_i) * lon_step + start) / 1000000.0;
                // Note: the float is size 4 
                memcpy(buf,&total_val[0],4*xdimsize);
            }
	}
    }

#if 0
for (int i =0; i <nelms; i++) 
"h5","final data val "<< i <<" is " << val[i] <<endl;
#endif

    set_value ((dods_float32 *) &val[0], nelms);
    
 
    return;
}
#if 0
// parse constraint expr. and make hdf5 coordinate point location.
// return number of elements to read. 
int
HDFEOS5CFMissLLArray::format_constraint (int *offset, int *step, int *count)
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
        }

        return nels;
}

#endif
