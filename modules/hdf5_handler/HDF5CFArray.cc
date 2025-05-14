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

////////////////////////////////////////////////////////////////////////////////
/// \file HDF5CFArray.cc
/// \brief The implementation of  methods to read data array into DAP buffer from an HDF5 dataset for the CF option.
///
/// In the future, this may be merged with the default option.
/// \author Kent Yang <myang6@hdfgroup.org>
///
////////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <BESDebug.h>
#include <sys/stat.h>
#include <libdap/InternalErr.h>

#include <libdap/Str.h>
#include "HDF5RequestHandler.h"
#include "HDF5CFArray.h"
#include "h5cfdaputil.h"
#include "ObjMemCache.h"

using namespace std;
using namespace libdap;


BaseType *HDF5CFArray::ptr_duplicate()
{
    auto HDF5CFArray_unique = make_unique<HDF5CFArray>(*this);
    return HDF5CFArray_unique.release();
}

// Read in an HDF5 Array 
bool HDF5CFArray::read()
{

    BESDEBUG("h5","Coming to HDF5CFArray read "<<endl);
    if(length() == 0)
        return true;

    if((nullptr == HDF5RequestHandler::get_lrdata_mem_cache()) && 
        nullptr == HDF5RequestHandler::get_srdata_mem_cache()){
        read_data_NOT_from_mem_cache(false,nullptr);
        return true;
    }

    // Flag to check if using large raw data cache or small raw data cache.
    short use_cache_flag = 0;

    // The small data cache is checked first to reduce the resources to operate the big data cache.
    if(HDF5RequestHandler::get_srdata_mem_cache() != nullptr) {
        if(((cvtype == CV_EXIST) && (islatlon != true)) || (cvtype == CV_NONLATLON_MISS) 
            || (cvtype == CV_FILLINDEX) ||(cvtype == CV_MODIFY) ||(cvtype == CV_SPECIAL)){

            if(HDF5CFUtil::cf_dap2_support_numeric_type(dtype,is_dap4)==true) 
                use_cache_flag = 1;
        }
    }

    // If this variable doesn't fit the small data cache, let's check if it fits the large data cache.
    if(use_cache_flag !=1) {

        if(HDF5RequestHandler::get_lrdata_mem_cache() != nullptr) {

            // This is the trival case. 
            // If no information is provided in the configuration file of large data cache,
            // just cache the lat/lon variable per file.
            if(HDF5RequestHandler::get_common_cache_dirs() == false) {
		        if(cvtype == CV_LAT_MISS || cvtype == CV_LON_MISS 
                    || (cvtype == CV_EXIST && islatlon == true)) {
#if 0
//cerr<<"coming to use_cache_flag =2 "<<endl;
#endif
                    // Only the data with the numeric datatype DAP2 and CF support are cached.
		            if(HDF5CFUtil::cf_dap2_support_numeric_type(dtype,is_dap4)==true)
		                use_cache_flag = 2;
                }
            }
            else {// Have large data cache configuration info.

                // Need to check if we don't want to cache some CVs, now
                // this only applies to lat/lon CV.
		        if(cvtype == CV_LAT_MISS || cvtype == CV_LON_MISS
                    || (cvtype == CV_EXIST && islatlon == true)) {

                    vector<string> cur_lrd_non_cache_dir_list;
                    HDF5RequestHandler::get_lrd_non_cache_dir_list(cur_lrd_non_cache_dir_list);

                    // Check if this file is included in the non-cache directory
                    if( (cur_lrd_non_cache_dir_list.empty()) ||
                        ("" == check_str_sect_in_list(cur_lrd_non_cache_dir_list,filename,'/'))) {

                        // Only data with the numeric datatype DAP2 and CF support are cached.   
                        if(HDF5CFUtil::cf_dap2_support_numeric_type(dtype,is_dap4)==true) 
                            use_cache_flag = 3;                       
                    }
                }
                // Here we allow all the variable names to be cached. 
                // The file path that includes the variables can also be included.
                vector<string> cur_lrd_var_cache_file_list;
                HDF5RequestHandler::get_lrd_var_cache_file_list(cur_lrd_var_cache_file_list);
                if(cur_lrd_var_cache_file_list.empty() == false){
#if 0
///for(int i =0; i<cur_lrd_var_cache_file_list.size();i++)
//cerr<<"lrd var cache is "<<cur_lrd_var_cache_file_list[i]<<endl;
#endif
                    if(true == check_var_cache_files(cur_lrd_var_cache_file_list,filename,varname)){
#if 0
//cerr<<"varname is "<<varname <<endl;
//cerr<<"have var cached "<<endl;
#endif

                         // Only the data with the numeric datatype DAP2 and CF support are cached. 
                        if(HDF5CFUtil::cf_dap2_support_numeric_type(dtype,is_dap4)==true) 
                            use_cache_flag = 4;
                    }
                }
            }
        }
    }

    if(0 == use_cache_flag) 
        read_data_NOT_from_mem_cache(false,nullptr);
    else {// memory cache cases

        string cache_key;

        // Possibly we have common lat/lon dirs,so check here.
        if( 3 == use_cache_flag){
            vector<string> cur_cache_dlist;
            HDF5RequestHandler::get_lrd_cache_dir_list(cur_cache_dlist);
            string cache_dir = check_str_sect_in_list(cur_cache_dlist,filename,'/');
            if(cache_dir != "") 
                cache_key = cache_dir + varname;
            else {
                cache_key = filename + varname;
                // If this lat/lon is not in the common dir. list, it is still cached as a general lat/lon.
                // Change the flag to 2.
                use_cache_flag = 2;
            }

        }
        else
            cache_key = filename + varname;

        handle_data_with_mem_cache(dtype,total_elems,use_cache_flag,cache_key,is_dap4);

    }

    return true;
}

// Reading data not from memory cache: The data can be read from the disk cache or can be read via the HDF5 APIs
void HDF5CFArray::read_data_NOT_from_mem_cache(bool add_mem_cache,void*buf) {

    vector<int64_t>offset;
    vector<int64_t>count;
    vector<int64_t>step;
    vector<hsize_t> hoffset;
    vector<hsize_t>hcount;
    vector<hsize_t>hstep;
    int64_t nelms = 1;

    if (rank <= 0) 
        throw InternalErr (__FILE__, __LINE__,
                          "The number of dimension of the variable is <=0 for an array.");
    else {

        offset.resize(rank);
        count.resize(rank);
        step.resize(rank);
        hoffset.resize(rank);
        hcount.resize(rank);
        hstep.resize(rank);
        nelms = format_constraint (offset.data(), step.data(), count.data());
        for (int i = 0; i <rank; i++) {
            hoffset[i] = (hsize_t) offset[i];
            hcount[i] = (hsize_t) count[i];
            hstep[i] = (hsize_t) step[i];
        }
    }

    BESDEBUG("h5","after format_constraint "<<endl);
    hid_t dsetid = -1;
    hid_t dspace = -1;
    hid_t mspace = -1;
    hid_t dtypeid = -1;
    hid_t memtype = -1;

    bool data_from_disk_cache = false;
    bool data_to_disk_cache = false;

    // Check if the disk cache can be applied.
    bool use_disk_cache = valid_disk_cache();
 
    string cache_fpath;

    if(true == use_disk_cache) {
        BESDEBUG("h5","Coming to use disk cache "<<endl);


        unsigned long long disk_cache_size = HDF5RequestHandler::get_disk_cache_size();
        string diskcache_dir = HDF5RequestHandler::get_disk_cache_dir();
        string diskcache_prefix = HDF5RequestHandler::get_disk_cachefile_prefix();

        string cache_fname=HDF5CFUtil::obtain_cache_fname(diskcache_prefix,filename,varname);
        cache_fpath = diskcache_dir + "/"+ cache_fname;
        
        int64_t temp_total_elems = 1;
        for (const auto &dimsize:dimsizes)
            temp_total_elems = temp_total_elems*dimsize;

        short dtype_size = HDF5CFUtil::H5_numeric_atomic_type_size(dtype);
        // CHECK: I think when signed 8-bit needs to be converted to int16, dtype_size should also change.
        if(is_dap4 == false && dtype==H5CHAR) 
            dtype_size = 2;

        int64_t expected_file_size = dtype_size *temp_total_elems;
        int fd = 0;
        HDF5DiskCache *disk_cache = HDF5DiskCache::get_instance(disk_cache_size,diskcache_dir,diskcache_prefix);
        if( true == disk_cache->get_data_from_cache(cache_fpath, expected_file_size,fd)) {

            vector<size_t> offset_size_t;
            offset_size_t.resize(rank);
            for(int i = 0; i <rank;i++)
                offset_size_t[i] = (size_t)offset[i];
            size_t offset_1st = INDEX_nD_TO_1D(dimsizes,offset_size_t);          
            vector<size_t>end;
            end.resize(rank);
            for (int i = 0; i < rank; i++)
                end[i] = offset[i] +(count[i]-1)*step[i];
            size_t offset_last = INDEX_nD_TO_1D(dimsizes,end);
#if 0
//cerr<<"offset_1d is "<<offset_1st <<endl;
//cerr<<"offset_last is "<<offset_last <<endl;
#endif
            size_t total_read = dtype_size*(offset_last-offset_1st+1);
            
            off_t fpos = lseek(fd,dtype_size*offset_1st,SEEK_SET);
            if (-1 == fpos) {
                disk_cache->unlock_and_close(cache_fpath);
                disk_cache->purge_file(cache_fpath);
            }
 
            /// Read the data from the cache
            else 
                data_from_disk_cache = obtain_cached_data(disk_cache,cache_fpath,fd, step,count,total_read,dtype_size);

        }

        if(true == data_from_disk_cache)
            return;
        else 
            data_to_disk_cache = true;

    }


// END CACHE
   
    bool pass_fileid = HDF5RequestHandler::get_pass_fileid();
    if(false == pass_fileid) {
        if ((fileid = H5Fopen(filename.c_str(),H5F_ACC_RDONLY,H5P_DEFAULT))<0) {
            ostringstream eherr;
            eherr << "HDF5 File " << filename 
                  << " cannot be opened. "<<endl;
            throw InternalErr (__FILE__, __LINE__, eherr.str ());
        }
    }

    if ((dsetid = H5Dopen(fileid,varname.c_str(),H5P_DEFAULT))<0) {
        HDF5CFUtil::close_fileid(fileid,pass_fileid);
        ostringstream eherr;
        eherr << "HDF5 dataset " << varname
              << " cannot be opened. "<<endl;
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }

    if ((dspace = H5Dget_space(dsetid))<0) {

        H5Dclose(dsetid);
        HDF5CFUtil::close_fileid(fileid,pass_fileid);
        ostringstream eherr;
        eherr << "Space id of the HDF5 dataset " << varname
              << " cannot be obtained. "<<endl;
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }

    if (H5Sselect_hyperslab(dspace, H5S_SELECT_SET,
                               hoffset.data(), hstep.data(),
                               hcount.data(), nullptr) < 0) {

            H5Sclose(dspace);
            H5Dclose(dsetid);
            HDF5CFUtil::close_fileid(fileid,pass_fileid);
            ostringstream eherr;
            eherr << "The selection of hyperslab of the HDF5 dataset " << varname
                  << " fails. "<<endl;
            throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }

    mspace = H5Screate_simple(rank, hcount.data(),nullptr);
    if (mspace < 0) {
            H5Sclose(dspace);
            H5Dclose(dsetid); 
            HDF5CFUtil::close_fileid(fileid,pass_fileid);
            ostringstream eherr;
            eherr << "The creation of the memory space of the  HDF5 dataset " << varname
                  << " fails. "<<endl;
            throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }


    if ((dtypeid = H5Dget_type(dsetid)) < 0) {
            
        H5Sclose(mspace);
        H5Sclose(dspace);
        H5Dclose(dsetid);
        HDF5CFUtil::close_fileid(fileid,pass_fileid);
        ostringstream eherr;
        eherr << "Obtaining the datatype of the HDF5 dataset " << varname
              << " fails. "<<endl;
        throw InternalErr (__FILE__, __LINE__, eherr.str ());

    }

    if ((memtype = H5Tget_native_type(dtypeid, H5T_DIR_ASCEND))<0) {

        H5Sclose(mspace);
        H5Tclose(dtypeid);
        H5Sclose(dspace);
        H5Dclose(dsetid);
        HDF5CFUtil::close_fileid(fileid,pass_fileid);
        ostringstream eherr;
        eherr << "Obtaining the memory type of the HDF5 dataset " << varname
              << " fails. "<<endl;
        throw InternalErr (__FILE__, __LINE__, eherr.str ());

    }

    hid_t read_ret = -1;

    // Before reading the data, we will check if the memory cache is turned on, 
    // The add_mem_cache  is only true when the data memory cache keys are on and  used. 
    if(true == add_mem_cache) {
        if(buf== nullptr) {
            H5Sclose(mspace);
            H5Tclose(dtypeid);
            H5Sclose(dspace);
            H5Dclose(dsetid);
            HDF5CFUtil::close_fileid(fileid,pass_fileid);
            throw InternalErr(__FILE__,__LINE__,"The memory data cache buffer needs to be set");
        }
        read_ret= H5Dread(dsetid,memtype,H5S_ALL,H5S_ALL,H5P_DEFAULT,buf);
        if(read_ret <0){ 
            H5Sclose(mspace);
            H5Tclose(dtypeid);
            H5Sclose(dspace);
            H5Dclose(dsetid);
            HDF5CFUtil::close_fileid(fileid,pass_fileid);
            throw InternalErr(__FILE__,__LINE__,"Cannot read the data to the buffer.");
        }
    }

    
    // Now reading the data, note dtype is not dtypeid.
    // dtype is an enum  defined by the handler.
     
if (true == data_to_disk_cache) 
cerr<<"writing data to disk cache for "<<varname <<endl;
    switch (dtype) {

        case H5CHAR:
        {

            vector<char> val;
            val.resize(nelms);

            read_ret = H5Dread(dsetid,memtype,mspace,dspace,H5P_DEFAULT,val.data());
            if (read_ret < 0) {

                H5Sclose(mspace);
                H5Tclose(memtype);
                H5Tclose(dtypeid);
                H5Sclose(dspace);
                H5Dclose(dsetid);
                HDF5CFUtil::close_fileid(fileid,pass_fileid);
                ostringstream eherr;
                eherr << "Cannot read the HDF5 dataset " << varname
                      << " with the type of H5T_NATIVE_CHAR "<<endl;
                throw InternalErr (__FILE__, __LINE__, eherr.str ());

            }
 
            if(is_dap4 == true) 
                set_value_ll((dods_int8 *)val.data(),nelms);
            else {

                vector<short>newval;
                newval.resize(nelms);
    
                for (int64_t counter = 0; counter < nelms; counter++)
                    newval[counter] = (short) (val[counter]);
    
                set_value_ll(newval.data(), nelms);
            }

            if(true == data_to_disk_cache) {
                try {
                    BESDEBUG("h5","writing data to disk cache "<<endl);
                    write_data_to_cache(dsetid,dspace,mspace,memtype,cache_fpath,2,val,nelms);
                }
                catch(...) {
                    H5Sclose(mspace);
                    H5Tclose(memtype);
                    H5Tclose(dtypeid);
                    H5Sclose(dspace);
                    H5Dclose(dsetid);
                    HDF5CFUtil::close_fileid(fileid,pass_fileid);
                    ostringstream eherr;
                    eherr << "write data to cache failed.";
                    throw InternalErr (__FILE__, __LINE__, eherr.str ());

                }
            }

        } // case H5CHAR
           break;

        // Note: for DAP2, H5INT64,H5UINT64 will be ignored.
        case H5UCHAR:
        case H5UINT16:
        case H5INT16:
        case H5INT32:
        case H5UINT32:
        case H5INT64:
        case H5UINT64:
        case H5FLOAT32:
        case H5FLOAT64:
        
                
        {
            size_t dtype_size = HDF5CFUtil::H5_numeric_atomic_type_size(dtype);
            vector<char> val;
            val.resize(nelms*dtype_size);
            
            read_ret = H5Dread(dsetid,memtype,mspace,dspace,H5P_DEFAULT,val.data());
            if (read_ret < 0) {
                H5Sclose(mspace);
                H5Tclose(memtype);
                H5Tclose(dtypeid);
                H5Sclose(dspace);
                H5Dclose(dsetid);
                HDF5CFUtil::close_fileid(fileid,pass_fileid);
                ostringstream eherr;
                eherr << "Cannot read the HDF5 dataset " << varname
                      << " with the type of H5T_NATIVE_UCHAR "<<endl;
                throw InternalErr (__FILE__, __LINE__, eherr.str ());

            }
            // Not sure if "set_value ((dods_byte *) val.data(), nelms);" works.
            BESDEBUG("h5","after H5Dread "<<endl);
            val2buf(val.data());
            BESDEBUG("h5","after val2buf "<<endl);
            set_read_p(true);

            if(true == data_to_disk_cache) {
                BESDEBUG("h5","writing data to disk cache "<<endl);
                try {
                    write_data_to_cache(dsetid,dspace,mspace,memtype,cache_fpath,dtype_size,val,nelms);
                }
                catch(...) {
                    H5Sclose(mspace);
                    H5Tclose(memtype);
                    H5Tclose(dtypeid);
                    H5Sclose(dspace);
                    H5Dclose(dsetid);
                    HDF5CFUtil::close_fileid(fileid,pass_fileid);
                    ostringstream eherr;
                    eherr << "Write data to cache failed." 
                          << "It is very possible the error is caused by the server failure"
                          << " such as filled disk partition at the server rather than Hyrax. Please contact "
                          << " the corresponding data center first. If the issue is not due to "
                          << " the server,"; 
                    throw InternalErr (__FILE__, __LINE__, eherr.str ());

                }

            }
        } // case H5UCHAR...
            break;



#if 0
        case H5INT16:
        {
            vector<short>val;
            val.resize(nelms);
                
            read_ret = H5Dread(dsetid,memtype,mspace,dspace,H5P_DEFAULT,val.data());
            if (read_ret < 0) {

                H5Sclose(mspace);
                H5Tclose(memtype);
                H5Tclose(dtypeid);
                H5Sclose(dspace);
                H5Dclose(dsetid);
                HDF5CFUtil::close_fileid(fileid,pass_fileid);
                //H5Fclose(fileid);
                ostringstream eherr;
                eherr << "Cannot read the HDF5 dataset " << varname
                      << " with the type of H5T_NATIVE_SHORT "<<endl;
                throw InternalErr (__FILE__, __LINE__, eherr.str ());

            }
            set_value ((dods_int16 *) val.data(), nelms);
        }// H5INT16
            break;


        case H5UINT16:
            {
                vector<unsigned short> val;
                val.resize(nelms);
                read_ret = H5Dread(dsetid,memtype,mspace,dspace,H5P_DEFAULT,val.data());
                if (read_ret < 0) {

                    H5Sclose(mspace);
                    H5Tclose(memtype);
                    H5Tclose(dtypeid);
                    H5Sclose(dspace);
                    H5Dclose(dsetid);
                    HDF5CFUtil::close_fileid(fileid,pass_fileid);
                    ostringstream eherr;
                    eherr << "Cannot read the HDF5 dataset " << varname
                        << " with the type of H5T_NATIVE_USHORT "<<endl;
                    throw InternalErr (__FILE__, __LINE__, eherr.str ());

                }
                set_value ((dods_uint16 *) val.data(), nelms);
            } // H5UINT16
            break;


        case H5INT32:
        {
            vector<int>val;
            val.resize(nelms);
            read_ret = H5Dread(dsetid,memtype,mspace,dspace,H5P_DEFAULT,val.data());
            if (read_ret < 0) {
                H5Sclose(mspace);
                H5Tclose(memtype);
                H5Tclose(dtypeid);
                H5Sclose(dspace);
                H5Dclose(dsetid);
                HDF5CFUtil::close_fileid(fileid,pass_fileid);
                ostringstream eherr;
                eherr << "Cannot read the HDF5 dataset " << varname
                      << " with the type of H5T_NATIVE_INT "<<endl;
                throw InternalErr (__FILE__, __LINE__, eherr.str ());

            }
            set_value ((dods_int32 *) val.data(), nelms);
        } // case H5INT32
            break;

        case H5UINT32:
        {
            vector<unsigned int>val;
            val.resize(nelms);
            read_ret = H5Dread(dsetid,memtype,mspace,dspace,H5P_DEFAULT,val.data());
            if (read_ret < 0) {
                H5Sclose(mspace);
                H5Tclose(memtype);
                H5Tclose(dtypeid);
                H5Sclose(dspace);
                H5Dclose(dsetid);
                HDF5CFUtil::close_fileid(fileid,pass_fileid);
                ostringstream eherr;
                eherr << "Cannot read the HDF5 dataset " << varname
                      << " with the type of H5T_NATIVE_UINT "<<endl;
                throw InternalErr (__FILE__, __LINE__, eherr.str ());

            }
            set_value ((dods_uint32 *) val.data(), nelms);
        }
            break;

        case H5FLOAT32:
        {

            vector<float>val;
            val.resize(nelms);

            read_ret = H5Dread(dsetid,memtype,mspace,dspace,H5P_DEFAULT,val.data());
            if (read_ret < 0) {
                H5Sclose(mspace);
                H5Tclose(memtype);
                H5Tclose(dtypeid);
                H5Sclose(dspace);
                H5Dclose(dsetid);
                HDF5CFUtil::close_fileid(fileid,pass_fileid);
                ostringstream eherr;
                eherr << "Cannot read the HDF5 dataset " << varname
                      << " with the type of H5T_NATIVE_FLOAT "<<endl;
                throw InternalErr (__FILE__, __LINE__, eherr.str ());

            }
            set_value ((dods_float32 *) val.data(), nelms);
        }
            break;


        case H5FLOAT64:
        {

            vector<double>val;
            val.resize(nelms);
            read_ret = H5Dread(dsetid,memtype,mspace,dspace,H5P_DEFAULT,val.data());

            if (read_ret < 0) {
                H5Sclose(mspace);
                H5Tclose(memtype);
                H5Tclose(dtypeid);
                H5Sclose(dspace);
                H5Dclose(dsetid);
                HDF5CFUtil::close_fileid(fileid,pass_fileid);
                ostringstream eherr;
                eherr << "Cannot read the HDF5 dataset " << varname
                      << " with the type of H5T_NATIVE_DOUBLE "<<endl;
                throw InternalErr (__FILE__, __LINE__, eherr.str ());

            }
            set_value ((dods_float64 *) val.data(), nelms);
        } // case H5FLOAT64
            break;

#endif

        case H5FSTRING:
        {
            size_t ty_size = H5Tget_size(dtypeid);
            if (ty_size == 0) {
                H5Sclose(mspace);
                H5Tclose(memtype);
                H5Tclose(dtypeid);
                H5Sclose(dspace);
                H5Dclose(dsetid);
                HDF5CFUtil::close_fileid(fileid,pass_fileid);
                ostringstream eherr;
                eherr << "Cannot obtain the size of the fixed size HDF5 string of the dataset " 
                      << varname <<endl;
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }

            vector <char> strval;
            strval.resize(nelms*ty_size);
            read_ret = H5Dread(dsetid,memtype,mspace,dspace,H5P_DEFAULT,(void*)strval.data());

            if (read_ret < 0) {
                H5Sclose(mspace);
                H5Tclose(memtype);
                H5Tclose(dtypeid);
                H5Sclose(dspace);
                H5Dclose(dsetid);
                HDF5CFUtil::close_fileid(fileid,pass_fileid);
                ostringstream eherr;
                eherr << "Cannot read the HDF5 dataset " << varname
                      << " with the type of the fixed size HDF5 string "<<endl;
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }

            string total_string(strval.begin(),strval.end());
            strval.clear(); // May not be necessary
            vector <string> finstrval;
            finstrval.resize(nelms);
            for (int64_t i = 0; i<nelms; i++) 
                  finstrval[i] = total_string.substr(i*ty_size,ty_size);
            
            // Check if we should drop the long string

            // If the size of an individual element is longer than the current netCDF JAVA
            // string and the "EnableDropLongString" key is turned on,
            // No string is generated.
            if ((true == HDF5RequestHandler::get_drop_long_string()) &&
                ty_size > NC_JAVA_STR_SIZE_LIMIT) {
                for (int64_t i = 0; i<nelms; i++)
                    finstrval[i] = "";
            }
            set_value_ll(finstrval,nelms);
            total_string.clear();
        }
            break;


        case H5VSTRING:
        {
            size_t ty_size = H5Tget_size(memtype);
            if (ty_size == 0) {
                H5Sclose(mspace);
                H5Tclose(memtype);
                H5Tclose(dtypeid);
                H5Sclose(dspace);
                H5Dclose(dsetid);
                HDF5CFUtil::close_fileid(fileid,pass_fileid);
                ostringstream eherr;
                eherr << "Cannot obtain the size of the fixed size HDF5 string of the dataset "
                      << varname <<endl;
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }
            vector <char> strval;
            strval.resize(nelms*ty_size);
            read_ret = H5Dread(dsetid,memtype,mspace,dspace,H5P_DEFAULT,(void*)strval.data());

            if (read_ret < 0) {
                H5Sclose(mspace);
                H5Tclose(memtype);
                H5Tclose(dtypeid);
                H5Sclose(dspace);
                H5Dclose(dsetid);
                HDF5CFUtil::close_fileid(fileid,pass_fileid);
                ostringstream eherr;
                eherr << "Cannot read the HDF5 dataset " << varname
                      << " with the type of the HDF5 variable length string "<<endl;
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }

            vector<string>finstrval;
            finstrval.resize(nelms);
            char*temp_bp = strval.data();
            char*onestring = nullptr;
            for (int64_t i =0;i<nelms;i++) {
                onestring = *(char**)temp_bp;
                if(onestring!=nullptr ) 
                    finstrval[i] =string(onestring);
                
                else // We will add a nullptr if onestring is nullptr.
                    finstrval[i]="";
                temp_bp +=ty_size;
            }

            if (false == strval.empty()) {
                herr_t ret_vlen_claim;
                ret_vlen_claim = H5Dvlen_reclaim(memtype,mspace,H5P_DEFAULT,(void*)strval.data());
                if (ret_vlen_claim < 0){
                    H5Sclose(mspace);
                    H5Tclose(memtype);
                    H5Tclose(dtypeid);
                    H5Sclose(dspace);
                    H5Dclose(dsetid);
                    HDF5CFUtil::close_fileid(fileid,pass_fileid);
                    ostringstream eherr;
                    eherr << "Cannot reclaim the memory buffer of the HDF5 variable length string of the dataset "
                          << varname <<endl;
                    throw InternalErr (__FILE__, __LINE__, eherr.str ());
 
                }
            }

            // If the size of one string element is longer than the current netCDF JAVA
            // string and the "EnableDropLongString" key is turned on,
            // No string is generated.
            if (true == HDF5RequestHandler::get_drop_long_string()) {
                bool drop_long_str = false;
                for (int64_t i =0;i<nelms;i++) {
                    if(finstrval[i].size() >NC_JAVA_STR_SIZE_LIMIT){
                        drop_long_str = true;
                        break;
                    }
                }
                if (drop_long_str == true) {
                    for (int64_t i =0;i<nelms;i++)
                        finstrval[i] = "";
                }
            }
            set_value_ll(finstrval,nelms);

        }
            break;

        default:
        {
                H5Tclose(memtype);
                H5Tclose(dtypeid);
                H5Sclose(mspace);
                H5Sclose(dspace);
                H5Dclose(dsetid);
                HDF5CFUtil::close_fileid(fileid,pass_fileid);
                ostringstream eherr;
                eherr << "Cannot read the HDF5 dataset " << varname
                        << " with the unsupported HDF5 datatype"<<endl;
                    throw InternalErr (__FILE__, __LINE__, eherr.str ());
        }
    }

    H5Tclose(memtype);
    H5Tclose(dtypeid);
    H5Sclose(mspace);
    H5Sclose(dspace);
    H5Dclose(dsetid);
    HDF5CFUtil::close_fileid(fileid,pass_fileid);

}

bool HDF5CFArray::valid_disk_cache() const {

    bool ret_value = false;
    if(true == HDF5RequestHandler::get_use_disk_cache()) {

        BESDEBUG("h5","Coming to disk cache "<<endl);
        // Check if this is a valid numeric datatype we want to support
        if(dtype == H5CHAR || dtype ==H5UCHAR || dtype==H5INT16 || dtype ==H5UINT16 ||
           dtype == H5INT32 || dtype ==H5UINT32 || dtype ==H5FLOAT32 || dtype==H5FLOAT64 ||
           dtype == H5INT64 || dtype ==H5UINT64){

            BESDEBUG("h5","Coming to disk cache datatype block"<<endl);

            string diskcache_dir = HDF5RequestHandler::get_disk_cache_dir();
            string diskcache_prefix = HDF5RequestHandler::get_disk_cachefile_prefix();
            long diskcache_size = HDF5RequestHandler::get_disk_cache_size();

            if(("" == diskcache_dir)||(""==diskcache_prefix)||(diskcache_size <=0))
                throw InternalErr (__FILE__, __LINE__, "Either the cached dir is empty or the prefix is nullptr or the cache size is not set.");
            else {
                struct stat sb;
                if(stat(diskcache_dir.c_str(),&sb) !=0) {
                    string err_mesg="The cached directory " + diskcache_dir;
                    err_mesg = err_mesg + " doesn't exist.  ";
                    throw InternalErr(__FILE__,__LINE__,err_mesg);
                }
                else { 
                    if(true == S_ISDIR(sb.st_mode)) {
                        if(access(diskcache_dir.c_str(),R_OK|W_OK|X_OK) == -1) {
                            string err_mesg="The cached directory " + diskcache_dir;
                            err_mesg = err_mesg + " can NOT be read,written or executable.";
                            throw InternalErr(__FILE__,__LINE__,err_mesg);
                        }
                    }
                    else {
                        string err_mesg="The cached directory " + diskcache_dir;
                        err_mesg = err_mesg + " is not a directory.";
                        throw InternalErr(__FILE__,__LINE__,err_mesg);
                    }
                }
            }

            short dtype_size = HDF5CFUtil::H5_numeric_atomic_type_size(dtype);
            // Check if we only need to cache the specific compressed data
            if(true == HDF5RequestHandler::get_disk_cache_comp_data()){ 
                BESDEBUG("h5","Compression disk cache key is true"<<endl);
                ret_value = valid_disk_cache_for_compressed_data(dtype_size);
                BESDEBUG("h5","variable disk cache passes the compression parameter check"<<endl);
            }
            else {
                BESDEBUG("h5","Compression disk cache key is NOT set, disk cache key is true."<<endl);
                ret_value = true;
            }

        }

    }
    return ret_value;
}

bool HDF5CFArray:: valid_disk_cache_for_compressed_data(short dtype_size) const {

    bool ret_value = false;
    // The compression ratio should be smaller than the threshold(hard to compress)
    // and the total var size should be bigger than the defined size(bigger)
#if 0
    size_t total_byte = total_elems*dtype_size;
#endif
    if((comp_ratio < HDF5RequestHandler::get_disk_comp_threshold()) 
       && (total_elems*dtype_size >= HDF5RequestHandler::get_disk_var_size())) {
cerr<<"var name: "<<varname <<endl;
cerr<<"var size in bytes: "<<total_elems*dtype_size <<endl;
        if( true == HDF5RequestHandler::get_disk_cache_float_only_comp()) {
            if(dtype==H5FLOAT32 || dtype == H5FLOAT64) 
                ret_value = true;
        }
        else 
            ret_value = true;
    }
    return ret_value;

}

bool HDF5CFArray::obtain_cached_data(HDF5DiskCache *disk_cache,const string & cache_fpath, int fd,vector<int64_t> &cd_step, vector<int64_t>&cd_count,size_t total_read,short dtype_size) {

    ssize_t ret_read_val = -1;
    vector<char>buf;

    buf.resize(total_read);
    ret_read_val = HDF5CFUtil::read_buffer_from_file(fd,(void*)buf.data(),total_read);
    disk_cache->unlock_and_close(cache_fpath);
    if((-1 == ret_read_val) || (ret_read_val != (ssize_t)total_read)) {
        disk_cache->purge_file(cache_fpath);
        return false;
    }
    else {    
        size_t nele_to_read = 1;
        for(int i = 0; i<rank;i++) 
            nele_to_read *=cd_count[i];

        if(nele_to_read == (total_read/dtype_size)) {
            val2buf(buf.data());
            set_read_p(true);
        }
        else { //  Need to re-assemble the buffer according to different datatype

            vector<int64_t>cd_start(rank,0);
            vector<size_t>cd_pos(rank,0);
            int64_t nelms_to_send = 1;
            for(int i = 0; i <rank; i++)
                nelms_to_send = nelms_to_send*cd_count[i];

            switch (dtype) {

                case H5CHAR:
                {
#if 0
                     vector<int>total_val;
                     total_val.resize(total_read/dtype_size);
                     memcpy(total_val.data(),(void*)buf.data(),total_read);

                     vector<int>final_val;
                     subset<int>(
                                      total_val.data(),
                                      rank,
                                      dimsizes,
                                      cd_start.data(),
                                      cd_step.data(),
                                      cd_count.data(),
                                      &final_val,
                                      cd_pos,
                                      0
                                     );

#endif

                    if(is_dap4 == false) {
                        vector<short>final_val;
                        subset<short>(
                                      buf.data(),
                                      rank,
                                      dimsizes,
                                      cd_start.data(),
                                      cd_step.data(),
                                      cd_count.data(),
                                      &final_val,
                                      cd_pos,
                                      0
                                     );
                        set_value_ll((dods_int16*)final_val.data(),nelms_to_send);
                    }
                    else {
                        vector<char>final_val;
                        subset<char>(
                                      buf.data(),
                                      rank,
                                      dimsizes,
                                      cd_start.data(),
                                      cd_step.data(),
                                      cd_count.data(),
                                      &final_val,
                                      cd_pos,
                                      0
                                     );
                        set_value_ll((dods_int8*)final_val.data(),nelms_to_send);
                    }
 
                }

                    break;
                case H5UCHAR:
                {
#if 0
                    vector<unsigned char>total_val;
                    total_val.resize(total_read/dtype_size);
                    memcpy(total_val.data(),(void*)buf.data(),total_read);

                    vector<unsigned char>final_val;
                    subset<unsigned char>(
                                      total_val.data(),
                                      rank,
                                      dimsizes,
                                      cd_start.data(),
                                      cd_step.data(),
                                      cd_count.data(),
                                      &final_val,
                                      cd_pos,
                                      0
                                     );

#endif
                    vector<unsigned char>final_val;
                    subset<unsigned char>(
                                      buf.data(),
                                      rank,
                                      dimsizes,
                                      cd_start.data(),
                                      cd_step.data(),
                                      cd_count.data(),
                                      &final_val,
                                      cd_pos,
                                      0
                                     );

                    set_value_ll (final_val.data(), nelms_to_send);
                }
                    break;

                case H5INT16:
                {
#if 0
                    vector<short>total_val;
                    total_val.resize(total_read/dtype_size);
                    memcpy(total_val.data(),(void*)buf.data(),total_read);

                    vector<short>final_val;
                    subset<short>(
                                      total_val.data(),
                                      rank,
                                      dimsizes,
                                      cd_start.data(),
                                      cd_step.data(),
                                      cd_count.data(),
                                      &final_val,
                                      cd_pos,
                                      0
                                     );
#endif

                    vector<short>final_val;
                    subset<short>(
                                      buf.data(),
                                      rank,
                                      dimsizes,
                                      cd_start.data(),
                                      cd_step.data(),
                                      cd_count.data(),
                                      &final_val,
                                      cd_pos,
                                      0
                                     );

                    set_value_ll (final_val.data(), nelms_to_send);
                }
                    break;

                case H5UINT16:
                {
#if 0
                    vector<unsigned short>total_val;
                    total_val.resize(total_read/dtype_size);
                    memcpy(total_val.data(),(void*)buf.data(),total_read);

                    vector<unsigned short>final_val;
                    subset<unsigned short>(
                                      total_val.data(),
                                      rank,
                                      dimsizes,
                                      cd_start.data(),
                                      cd_step.data(),
                                      cd_count.data(),
                                      &final_val,
                                      cd_pos,
                                      0
                                     );
#endif

                    vector<unsigned short>final_val;
                    subset<unsigned short>(
                                      buf.data(),
                                      rank,
                                      dimsizes,
                                      cd_start.data(),
                                      cd_step.data(),
                                      cd_count.data(),
                                      &final_val,
                                      cd_pos,
                                      0
                                     );

                    set_value_ll (final_val.data(), nelms_to_send);
                }
                    break;

                case H5INT32:
                {
#if 0
                    vector<int>total_val;
                    total_val.resize(total_read/dtype_size);
                    memcpy(total_val.data(),(void*)buf.data(),total_read);

                    vector<int>final_val;
                    subset<int>(
                                      total_val.data(),
                                      rank,
                                      dimsizes,
                                      cd_start.data(),
                                      cd_step.data(),
                                      cd_count.data(),
                                      &final_val,
                                      cd_pos,
                                      0
                                     );

#endif

                    vector<int>final_val;
                    subset<int>(
                                      buf.data(),
                                      rank,
                                      dimsizes,
                                      cd_start.data(),
                                      cd_step.data(),
                                      cd_count.data(),
                                      &final_val,
                                      cd_pos,
                                      0
                                     );


                    set_value_ll (final_val.data(), nelms_to_send);
                }
                    break;

                case H5UINT32:
                {
#if 0
                    vector<unsigned int>total_val;
                    total_val.resize(total_read/dtype_size);
                    memcpy(total_val.data(),(void*)buf.data(),total_read);

                    vector<unsigned int>final_val;
                    subset<unsigned int>(
                                      total_val.data(),
                                      rank,
                                      dimsizes,
                                      cd_start.data(),
                                      cd_step.data(),
                                      cd_count.data(),
                                      &final_val,
                                      cd_pos,
                                      0
                                     );
#endif

                    vector<unsigned int>final_val;
                    subset<unsigned int>(
                                      buf.data(),
                                      rank,
                                      dimsizes,
                                      cd_start.data(),
                                      cd_step.data(),
                                      cd_count.data(),
                                      &final_val,
                                      cd_pos,
                                      0
                                     );

                    set_value_ll (final_val.data(), nelms_to_send);
                }
                    break;

                case H5INT64: // Only for DAP4 CF
                {
#if 0
                    vector<unsigned int>total_val;
                    total_val.resize(total_read/dtype_size);
                    memcpy(total_val.data(),(void*)buf.data(),total_read);

                    vector<unsigned int>final_val;
                    subset<unsigned int>(
                                      total_val.data(),
                                      rank,
                                      dimsizes,
                                      cd_start.data(),
                                      cd_step.data(),
                                      cd_count.data(),
                                      &final_val,
                                      cd_pos,
                                      0
                                     );
#endif

                    vector<long long >final_val;
                    subset<long long >(
                                      buf.data(),
                                      rank,
                                      dimsizes,
                                      cd_start.data(),
                                      cd_step.data(),
                                      cd_count.data(),
                                      &final_val,
                                      cd_pos,
                                      0
                                     );

                    set_value_ll ((dods_int64*)final_val.data(), nelms_to_send);
                }
                    break;



                case H5UINT64: // Only for DAP4 CF
                {
#if 0
                    vector<unsigned int>total_val;
                    total_val.resize(total_read/dtype_size);
                    memcpy(total_val.data(),(void*)buf.data(),total_read);

                    vector<unsigned int>final_val;
                    subset<unsigned int>(
                                      total_val.data(),
                                      rank,
                                      dimsizes,
                                      cd_start.data(),
                                      cd_step.data(),
                                      cd_count.data(),
                                      &final_val,
                                      cd_pos,
                                      0
                                     );
#endif

                    vector<unsigned long long >final_val;
                    subset<unsigned long long >(
                                      buf.data(),
                                      rank,
                                      dimsizes,
                                      cd_start.data(),
                                      cd_step.data(),
                                      cd_count.data(),
                                      &final_val,
                                      cd_pos,
                                      0
                                     );

                    set_value_ll ((dods_uint64*)final_val.data(), nelms_to_send);
                }
                    break;


                case H5FLOAT32:
                {
#if 0
                    vector<float>total_val;
                    total_val.resize(total_read/dtype_size);
                    memcpy(total_val.data(),(void*)buf.data(),total_read);

                    vector<float>final_val;
                    subset<float>(
                                      total_val.data(),
                                      rank,
                                      dimsizes,
                                      cd_start.data(),
                                      cd_step.data(),
                                      cd_count.data(),
                                      &final_val,
                                      cd_pos,
                                      0
                                     );
#endif
                    
                    vector<float>final_val;
                    subset<float>(
                                      buf.data(),
                                      rank,
                                      dimsizes,
                                      cd_start.data(),
                                      cd_step.data(),
                                      cd_count.data(),
                                      &final_val,
                                      cd_pos,
                                      0
                                     );


                    set_value_ll (final_val.data(), nelms_to_send);
                }
                    break;
                case H5FLOAT64:
                {
#if 0
                    vector<double>total_val;
                    total_val.resize(total_read/dtype_size);
                    memcpy(total_val.data(),(void*)buf.data(),total_read);

                    vector<double>final_val;
                    subset<double>(
                                      total_val.data(),
                                      rank,
                                      dimsizes,
                                      cd_start.data(),
                                      cd_step.data(),
                                      cd_count.data(),
                                      &final_val,
                                      cd_pos,
                                      0
                                     );
#endif
                    vector<double>final_val;
                    subset<double>(
                                      buf.data(),
                                      rank,
                                      dimsizes,
                                      cd_start.data(),
                                      cd_step.data(),
                                      cd_count.data(),
                                      &final_val,
                                      cd_pos,
                                      0
                                     );

                    set_value_ll (final_val.data(), nelms_to_send);
                }
                    break;
                default:
                    throw InternalErr (__FILE__, __LINE__, "unsupported data type.");

            }// "end switch(dtype)"
        }// "end else (stride is not 1)"
        return true;
    }// "end else(full_read = true)"
}


void 
HDF5CFArray::write_data_to_cache(hid_t dset_id, hid_t /*dspace_id*/, hid_t /*mspace_id*/, hid_t memtype,
    const string& cache_fpath, short dtype_size, const vector<char> &buf, int64_t nelms) {

    unsigned long long disk_cache_size = HDF5RequestHandler::get_disk_cache_size();
    string disk_cache_dir = HDF5RequestHandler::get_disk_cache_dir();
    string disk_cache_prefix = HDF5RequestHandler::get_disk_cachefile_prefix();
    HDF5DiskCache *disk_cache = HDF5DiskCache::get_instance(disk_cache_size,disk_cache_dir,disk_cache_prefix);
    int64_t total_nelem = 1;
    for(int i = 0; i <rank; i++)
        total_nelem = total_nelem*dimsizes[i];

    vector<char>val;

    if(H5CHAR == dtype && is_dap4 == false) {
 
        vector<short>newval;
        newval.resize(total_nelem);
        if(total_nelem == nelms) {
            for (int64_t i = 0; i < total_nelem;i++)
                newval[i] = (short)buf[i];
            disk_cache->write_cached_data2(cache_fpath,sizeof(short)*total_nelem,(const void*)newval.data());
        }
        else {
            vector<char>val2;
            val2.resize(total_nelem);
            if(H5Dread(dset_id, memtype, H5S_ALL, H5S_ALL,H5P_DEFAULT, val2.data())<0)
                throw InternalErr (__FILE__, __LINE__, "Cannot read the whole HDF5 dataset for the disk cache.");
            for (int64_t i = 0; i < total_nelem;i++)
                newval[i] = (short)val2[i];
            disk_cache->write_cached_data2(cache_fpath,sizeof(short)*total_nelem,(const void*)newval.data());
       }
    }
    else {
        if(total_nelem == nelms) {
            disk_cache->write_cached_data2(cache_fpath,dtype_size*total_nelem,(const void*)buf.data());
        }
        else {
            val.resize(dtype_size*total_nelem);
            if(H5Dread(dset_id, memtype, H5S_ALL, H5S_ALL,H5P_DEFAULT, val.data())<0)
                throw InternalErr (__FILE__, __LINE__, "Cannot read the whole SDS for cache.");

            disk_cache->write_cached_data2(cache_fpath,dtype_size*total_nelem,(const void*)val.data());
        }
    }
}



// We don't inherit libdap Array Class's transform_to_dap4 method since CF option is still using it.
// This function is used for 64-bit integer mapping to DAP4 for the CF option. largely borrowed from
// DAP4 code.
BaseType* HDF5CFArray::h5cfdims_transform_to_dap4_int64(D4Group *grp) {

    if(grp == nullptr)
        return nullptr;
    Array *dest = dynamic_cast<HDF5CFArray*>(ptr_duplicate());

    // If there is just a size, don't make
    // a D4Dimension (In DAP4 you cannot share a dimension unless it has
    // a name). jhrg 3/18/14

    for (Array::Dim_iter d = dest->dim_begin(), e = dest->dim_end(); d != e; ++d) {
        if (false == (*d).name.empty()) {

            D4Group *temp_grp   = grp;
            D4Dimension *d4_dim = nullptr;
            while(temp_grp) {

                D4Dimensions *temp_dims = temp_grp->dims();

                // Check if the dimension is defined in this group
                d4_dim = temp_dims->find_dim((*d).name);
                if (d4_dim) {
                  (*d).dim = d4_dim;
                  break;
                }

                if (temp_grp->get_parent())
                    temp_grp = static_cast<D4Group*>(temp_grp->get_parent());
                else 
                    temp_grp = nullptr;

            }

            // Not find this dimension in any of the ancestor groups, add it to this group.
            // The following block is fine, but to avoid the complaint from sonarcloud.
            // Use a bool.
            bool d4_dim_null = ((d4_dim==nullptr)?true:false);
            // Not find this dimension in any of the ancestor groups, add it to this group.
            if (d4_dim_null == true) {

                auto d4_dim_unique = make_unique<D4Dimension>((*d).name, (*d).size);
                D4Dimensions * dims = grp->dims();
                d4_dim = d4_dim_unique.release();
                dims->add_dim_nocopy(d4_dim);
                (*d).dim = d4_dim;
            }
        }
    }

    dest->set_is_dap4(true);

    return dest;

}
