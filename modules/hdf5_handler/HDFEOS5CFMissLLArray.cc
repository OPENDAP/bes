// This file is part of the hdf5_handler implementing for the CF-compliant
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

/////////////////////////////////////////////////////////////////////////////
/// \file HDFEOS5CFMissLLArray.cc
/// \brief The implementation of the retrieval of the missing lat/lon values for HDF-EOS5 products
///
/// \author Kent Yang <myang6@hdfgroup.org>
///
/// Copyright (C) 2011-2023 The HDF Group
///
/// All rights reserved.

#include <sys/stat.h>
#include <memory>
#include <iostream>
#include <BESDebug.h>
#include <libdap/InternalErr.h>
#include <BESInternalError.h>

#include "HDFEOS5CFMissLLArray.h"
#include "HDF5RequestHandler.h"

using namespace std;
using namespace libdap;

BaseType *HDFEOS5CFMissLLArray::ptr_duplicate()
{
    auto HDFEOS5CFMissLLArray_unique = make_unique<HDFEOS5CFMissLLArray>(*this);
    return HDFEOS5CFMissLLArray_unique.release();
}

bool HDFEOS5CFMissLLArray::read()
{

    BESDEBUG("h5","Coming to HDFEOS5CFMissLLArray read "<<endl);
    if(nullptr == HDF5RequestHandler::get_lrdata_mem_cache())
        read_data_NOT_from_mem_cache(false,nullptr);
    else {
       	vector<string> cur_lrd_non_cache_dir_list;                                      
        HDF5RequestHandler::get_lrd_non_cache_dir_list(cur_lrd_non_cache_dir_list);     
                                                                                                    
        string cache_key;
        // Check if this file is included in the non-cache directory                    
        if( (cur_lrd_non_cache_dir_list.empty()) ||                                 
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
                    handle_data_with_mem_cache(H5FLOAT32,(size_t)ydimsize,cache_flag,cache_key,false);
                else 
                    handle_data_with_mem_cache(H5FLOAT32,(size_t)xdimsize,cache_flag,cache_key,false);
        }
        else 
	         read_data_NOT_from_mem_cache(false,nullptr);
    }
    return true;
}

void HDFEOS5CFMissLLArray::read_data_NOT_from_mem_cache(bool add_cache,void*buf){

    BESDEBUG("h5","Coming to read_data_NOT_from_mem_cache "<<endl);

    // First handle geographic projection. No GCTP is needed.Since the calculation
    // of lat/lon is really simple for this case. No need to provide the disk cache
    // unless the calculation takes too long. We will see.
    if(eos5_projcode == HE5_GCTP_GEO) {
        read_data_NOT_from_mem_cache_geo(add_cache,buf);
        return;
    }

    int64_t nelms = -1;
    vector<int64_t>offset;
    vector<int64_t>count;
    vector<int64_t>step;

    if (rank <=  0) {
        string msg = "The number of dimension of this variable should be greater than 0.";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }
    else {
         offset.resize(rank);
         count.resize(rank);
         step.resize(rank);
         nelms = format_constraint (offset.data(), step.data(), count.data());
    }

    if (nelms <= 0) { 
        string msg = "The number of elments is negative.";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }

    int64_t total_elms = xdimsize*ydimsize;
    if (total_elms > DODS_INT_MAX) {
        string msg = "Currently we cannot calculate lat/lon that is greater than 2G for HDF-EOS5.";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }


    vector<size_t>pos(rank,0);
    for (int i = 0; i< rank; i++)
        pos[i] = offset[i];

    vector<size_t>dimsizes;
    dimsizes.push_back(ydimsize);
    dimsizes.push_back(xdimsize);

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


    int j = 0;
    int r = -1;

    for (int k = j = 0; j < ydimsize; ++j) {
        for (int i = 0; i < xdimsize; ++i) {
            rows[k] = j;
            cols[k] = i;
            ++k;
        }
    }

    BESDEBUG("h5", " Before calling GDij2ll, check all projection parameters. "  << endl);
    BESDEBUG("h5", " eos5_projcode is "  << eos5_projcode <<endl);
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
    
#if 0
    cerr<< " eos5_params[0] is "  << eos5_params[0] <<endl;
    cerr<< " eos5_params[1] is "  << eos5_params[1] <<endl;
    cerr<< " eos5_sphere is "  << eos5_sphere <<endl;
    cerr<<  " eos5_zone is "  << eos5_zone <<endl;
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
 
    // boolean to mark if the value can be read from the cache. 
    // Not necessary from programming point of view. Use it to
    // make the code flow clear.
    bool ll_read_from_cache = true;

    // Check if geo-cache is turned on.
    bool use_latlon_cache = HDF5RequestHandler::get_use_eosgeo_cachefile();


    // Adding more code to read latlon from cache here.
    if(use_latlon_cache == true) {

        // Cache name is made in a special way to make sure the same lat/lon 
        // fall into the same cache file.
        string cache_fname = obtain_ll_cache_name();

        // Obtain eos-geo disk cache size, dir and prefix.
        long ll_disk_cache_size = HDF5RequestHandler::get_latlon_disk_cache_size();
        string ll_disk_cache_dir = HDF5RequestHandler::get_latlon_disk_cache_dir();
        string ll_disk_cache_prefix = HDF5RequestHandler::get_latlon_disk_cachefile_prefix();

        // Expected cache file size
        int expected_file_size = 2*xdimsize*ydimsize*sizeof(double);     
        HDF5DiskCache *ll_cache = HDF5DiskCache::get_instance(ll_disk_cache_size,ll_disk_cache_dir,ll_disk_cache_prefix);
        int fd = 0;
        ll_read_from_cache = ll_cache->get_data_from_cache(cache_fname, expected_file_size,fd);

        if(ll_read_from_cache == true) {

            BESDEBUG("h5", " Read latitude and longitude from a disk cache. "  <<endl);
            size_t var_offset = 0;
            // Longitude is stored after latitude.
            if(CV_LON_MISS == cvartype) 
                var_offset = xdimsize*ydimsize*sizeof(double);

            vector<double> var_value;
            var_value.resize(xdimsize*ydimsize);

#if 0
            //Latitude starts from 0, longitude starts from xdimsize*ydimsize*sizeof(double);
#endif
            off_t fpos = lseek(fd,var_offset,SEEK_SET);
            if(fpos == -1) {
                string msg = "Cannot seek the cached file offset.";
                throw BESInternalError(msg,__FILE__,__LINE__);
            }
            ssize_t ret_val = HDF5CFUtil::read_buffer_from_file(fd,(void*)var_value.data(),var_value.size()*sizeof(double));
            ll_cache->unlock_and_close(cache_fname);

            // If the cached file is not read correctly, we should purge the file.
            if((-1 == ret_val) || ((size_t)ret_val != (xdimsize*ydimsize*sizeof(double)))) {
                ll_cache->purge_file(cache_fname);
                ll_read_from_cache = false; 
            }
            else {
                // short-cut, no need to do subset.
                if(total_elms == nelms)
                    set_value_ll(var_value.data(),total_elms);
                else {
                    vector<double>val;
                    subset<double>(
                           var_value.data(),
                           rank,
                           dimsizes,
                           offset.data(),
                           step.data(),
                           count.data(),
                           &val,
                           pos,
                           0);
                    set_value_ll(val.data(),nelms);
                }
                return;
            }
        }
        else 
            ll_read_from_cache = false;
    }
    
    // Calculate Lat/lon by using GCTP
    r = GDij2ll (eos5_projcode, eos5_zone, eos5_params.data(), eos5_sphere, xdimsize, ydimsize, upleft, lowright,
                 xdimsize * ydimsize, rows.data(), cols.data(), lon.data(), lat.data(), eos5_pixelreg, eos5_origin);
    if (r != 0) {
        string msg = "Cannot calculate grid latitude and longitude.";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }

    // ll_read_from_cache may be redundant. It is good to remind that we will generate the cache file
    // when reading from the cache fails.
    // We are using cache and need to write data to the cache.
    if(use_latlon_cache == true && ll_read_from_cache == false) {
        string cache_fname = obtain_ll_cache_name();
        long ll_disk_cache_size = HDF5RequestHandler::get_latlon_disk_cache_size();
        string ll_disk_cache_dir = HDF5RequestHandler::get_latlon_disk_cache_dir();
        string ll_disk_cache_prefix = HDF5RequestHandler::get_latlon_disk_cachefile_prefix();
        
        BESDEBUG("h5", " Write EOS5 grid latitude and longitude to a disk cache. "  <<endl);

        HDF5DiskCache *ll_cache = HDF5DiskCache::get_instance(ll_disk_cache_size,ll_disk_cache_dir,ll_disk_cache_prefix);
        
        // Merge vector lat and lon. lat first.
        vector <double>latlon;
        latlon.reserve(xdimsize*ydimsize*2);
        latlon.insert(latlon.end(),lat.begin(),lat.end());
        latlon.insert(latlon.end(),lon.begin(),lon.end());
        ll_cache->write_cached_data(cache_fname,2*xdimsize*ydimsize*sizeof(double),latlon);
     
    }
    BESDEBUG("h5", " The first value of lon is "  << lon[0] <<endl);
    BESDEBUG("h5", " The first value of lat is "  << lat[0] <<endl);

#if 0
    vector<size_t>pos(rank,0);
    for (int i = 0; i< rank; i++)
        pos[i] = offset[i];

    vector<size_t>dimsizes;
    dimsizes.push_back(ydimsize);
    dimsizes.push_back(xdimsize);
    int total_elms = xdimsize*ydimsize;
#endif

    
    if(CV_LON_MISS == cvartype) {
        if(total_elms == nelms)
            set_value_ll(lon.data(),total_elms);
        else {
            vector<double>val;
            subset<double>(
                           lon.data(),
                           rank,
                           dimsizes,
                           offset.data(),
                           step.data(),
                           count.data(),
                           &val,
                           pos,
                           0);
            set_value_ll(val.data(),nelms);
        }
       
    }
    else if(CV_LAT_MISS == cvartype) {

        if(total_elms == nelms)
            set_value_ll(lat.data(),total_elms);
        else {
            vector<double>val;
            subset<double>(
                           lat.data(),
                           rank,
                           dimsizes,
                           offset.data(),
                           step.data(),
                           count.data(),
                           &val,
                           pos,
                           0);
            set_value_ll(val.data(),nelms);
        }
    }
    return;

}

string HDFEOS5CFMissLLArray::obtain_ll_cache_name() {

    BESDEBUG("h5","Coming to obtain_ll_cache_name "<<endl);


    // Here we have a sanity check for the cached parameters:Cached directory,file prefix and cached directory size.
    // Supposedly Hyrax BES cache feature should check this and the code exists. However, the
    string bescachedir = HDF5RequestHandler::get_latlon_disk_cache_dir();
    string bescacheprefix = HDF5RequestHandler::get_latlon_disk_cachefile_prefix();
    long cachesize = HDF5RequestHandler::get_latlon_disk_cache_size();

    if(("" == bescachedir)||(""==bescacheprefix)||(cachesize <=0)){
        string msg = "Either the cached dir is empty or the prefix is nullptr or the cache size is not set.";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }
    else {
        struct stat sb;
        if(stat(bescachedir.c_str(),&sb) !=0) {
            string msg="The cached directory " + bescachedir;
            msg += " doesn't exist.  ";
            throw InternalErr(__FILE__,__LINE__,msg);
                    
        }
        else { 
            if(true == S_ISDIR(sb.st_mode)) {
                if(access(bescachedir.c_str(),R_OK|W_OK|X_OK) == -1) {
                    string msg="The cached directory " + bescachedir;
                    msg += " can NOT be read,written or executable.";
                    throw InternalErr(__FILE__,__LINE__,msg);
                }
            }
            else {
                string msg="The cached directory " + bescachedir;
                msg = msg + " is not a directory.";
                throw InternalErr(__FILE__,__LINE__,msg);
            }
        }
    }

    string cache_fname=HDF5RequestHandler::get_latlon_disk_cachefile_prefix();

    // Projection code,zone,sphere,pix,origin
    cache_fname +=HDF5CFUtil::get_int_str(eos5_projcode);
    cache_fname +=HDF5CFUtil::get_int_str(eos5_zone);
    cache_fname +=HDF5CFUtil::get_int_str(eos5_sphere);
    cache_fname +=HDF5CFUtil::get_int_str(eos5_pixelreg);
    cache_fname +=HDF5CFUtil::get_int_str(eos5_origin);

    cache_fname +=HDF5CFUtil::get_int_str(ydimsize);
    cache_fname +=HDF5CFUtil::get_int_str(xdimsize);
 
    // upleft,lowright
    // HDF-EOS upleft,lowright,params use DDDDMMMSSS.6 digits. So choose %17.6f.
    cache_fname +=HDF5CFUtil::get_double_str(point_left,17,6);
    cache_fname +=HDF5CFUtil::get_double_str(point_upper,17,6);
    cache_fname +=HDF5CFUtil::get_double_str(point_right,17,6);
    cache_fname +=HDF5CFUtil::get_double_str(point_lower,17,6);

    // According to HDF-EOS2 document, only 13 parameters are used.
    for(int ipar = 0; ipar<13;ipar++) {
        cache_fname+=HDF5CFUtil::get_double_str(eos5_params[ipar],17,6);
    }
            
    string cache_fpath = bescachedir + "/"+ cache_fname;
    return cache_fpath;

}
void HDFEOS5CFMissLLArray::read_data_NOT_from_mem_cache_geo(bool add_cache,void*buf){

    BESDEBUG("h5","Coming to read_data_NOT_from_mem_cache_geo "<<endl);
    int64_t nelms = -1;
    vector<int64_t>offset;
    vector<int64_t>count;
    vector<int64_t>step;


    if (rank <=  0) {
       string msg = "The number of dimension of this variable should be greater than 0.";
       msg += " The variable name is " + varname + ".";
       throw BESInternalError(msg,__FILE__,__LINE__);
    }
    else {
         offset.resize(rank);
         count.resize(rank);
         step.resize(rank);
         nelms = format_constraint (offset.data(), step.data(), count.data());
    }

    if (nelms <= 0 || nelms >DODS_INT_MAX) {
       string msg = "The number of elments for geographic lat/lon is negative or greater than 2G.";
       msg += " The variable name is " + varname + ".";
       throw BESInternalError(msg,__FILE__,__LINE__);
    }

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

	if (ydimsize <=0) {
            string msg = "The number of elments should be greater than 0.";
            msg += " The variable name is " + varname + ".";
	    throw BESInternalError(msg,__FILE__,__LINE__);
        }
           
	float lat_step = (end - start) /ydimsize;

	// Now offset,step and val will always be valid. line 74 and 85 assure this.
	if ( HE5_HDFE_CENTER == eos5_pixelreg ) {
	    for (int i = 0; i < nelms; i++)
		val[i] = ((offset[0]+i*step[0] + 0.5F) * lat_step + start) / 1000000.0F;

            // If the memory cache is turned on, we have to save all values to the buf
            if(add_cache == true) {
            	vector<float>total_val;
		total_val.resize(ydimsize);
                for (int total_i = 0; total_i < ydimsize; total_i++)
		    total_val[total_i] = ((total_i + 0.5F) * lat_step + start) / 1000000.0F;
                // Note: the float is size 4 
                memcpy(buf,total_val.data(),4*ydimsize);
            }

	}
	else { // HE5_HDFE_CORNER 
	    for (int i = 0; i < nelms; i++)
		val[i] = ((float)(offset[0]+i * step[0])*lat_step + start) / 1000000.0F;

            // If the memory cache is turned on, we have to save all values to the buf
            if(add_cache == true) {
            	vector<float>total_val;
		total_val.resize(ydimsize);
                for (int total_i = 0; total_i < ydimsize; total_i++)
		    total_val[total_i] = ((float)(total_i) * lat_step + start) / 1000000.0F;
                // Note: the float is size 4 
                memcpy(buf,total_val.data(),4*ydimsize);
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

	if (xdimsize <=0) {
            string msg = "The number of elments should be greater than 0.";
            msg += " The variable name is " + varname + ".";
        }
	float lon_step = (end - start) /xdimsize;

	if (HE5_HDFE_CENTER == eos5_pixelreg) {
	    for (int i = 0; i < nelms; i++)
		val[i] = ((offset[0] + i *step[0] + 0.5F) * lon_step + start ) / 1000000.0F;

            // If the memory cache is turned on, we have to save all values to the buf
            if(add_cache == true) {
            	vector<float>total_val;
		total_val.resize(xdimsize);
                for (int total_i = 0; total_i < xdimsize; total_i++)
		    total_val[total_i] = ((total_i+0.5F) * lon_step + start) / 1000000.0F;
                // Note: the float is size 4 
                memcpy(buf,total_val.data(),4*xdimsize);
            }

	}
	else { // HE5_HDFE_CORNER 
	    for (int i = 0; i < nelms; i++)
		val[i] = ((float)(offset[0]+i*step[0]) * lon_step + start) / 1000000.0F;

            // If the memory cache is turned on, we have to save all values to the buf
            if(add_cache == true) {
            	vector<float>total_val;
		total_val.resize(xdimsize);
                for (int total_i = 0; total_i < xdimsize; total_i++)
		    total_val[total_i] = ((float)(total_i) * lon_step + start) / 1000000.0F;
                // Note: the float is size 4 
                memcpy(buf,total_val.data(),4*xdimsize);
            }
	}
    }

    set_value_ll(val.data(), nelms);
 
    return;
}
