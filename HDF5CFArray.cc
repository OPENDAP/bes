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

////////////////////////////////////////////////////////////////////////////////
/// \file HDF5CFArray.cc
/// \brief The implementation of  methods to read data array into DAP buffer from an HDF5 dataset for the CF option.
///
/// In the future, this may be merged with the dddefault option.
/// \author Muqun Yang <myang6@hdfgroup.org>
///
////////////////////////////////////////////////////////////////////////////////

#include "config_hdf5.h"
#include <iostream>
#include <sstream>
#include <cassert>
#include <BESDebug.h>
#include "InternalErr.h"

#include "Str.h"
#include "HDF5RequestHandler.h"
#include "HDF5CFArray.h"
#include "h5cfdaputil.h"
#include "ObjMemCache.h"



BaseType *HDF5CFArray::ptr_duplicate()
{
    return new HDF5CFArray(*this);
}

// Read in an HDF5 Array 
bool HDF5CFArray::read()
{

    BESDEBUG("h5","Coming to HDF5CFArray read "<<endl);
    if(length() == 0)
        return true;

    if((NULL == HDF5RequestHandler::get_lrdata_mem_cache()) && 
        NULL == HDF5RequestHandler::get_srdata_mem_cache()){
        read_data_NOT_from_mem_cache(false,NULL);
        return true;
    }

//cerr<<"varname is "<<varname <<endl;
    // Flag to check if using large raw data cache or small raw data cache.
    short use_cache_flag = 0;

    // The small data cache is checked first to reduce the resources to operate the big data cache.
    if(HDF5RequestHandler::get_srdata_mem_cache() != NULL) {
        if(((cvtype == CV_EXIST) && (islatlon != true)) || (cvtype == CV_NONLATLON_MISS) 
            || (cvtype == CV_FILLINDEX) ||(cvtype == CV_MODIFY) ||(cvtype == CV_SPECIAL)){

            if(HDF5CFUtil::cf_strict_support_type(dtype)==true) 
		use_cache_flag = 1;
        }
    }

    // If this varible doesn't fit the small data cache, let's check if it fits the large data cache.
    if(use_cache_flag !=1) {

        if(HDF5RequestHandler::get_lrdata_mem_cache() != NULL) {

            // This is the trival case. 
            // If no information is provided in the configuration file of large data cache,
            // just cache the lat/lon varible per file.
            if(HDF5RequestHandler::get_common_cache_dirs() == false) {
		if(cvtype == CV_LAT_MISS || cvtype == CV_LON_MISS 
                    || (cvtype == CV_EXIST && islatlon == true)) {
//cerr<<"coming to use_cache_flag =2 "<<endl;

                    // Only the data with the numeric datatype DAP2 and CF support are cached.
		    if(HDF5CFUtil::cf_strict_support_type(dtype)==true)
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
                    if( (cur_lrd_non_cache_dir_list.size() == 0) ||
                        ("" == check_str_sect_in_list(cur_lrd_non_cache_dir_list,filename,'/'))) {

                        // Only data with the numeric datatype DAP2 and CF support are cached.   
                        if(HDF5CFUtil::cf_strict_support_type(dtype)==true) 
                            use_cache_flag = 3;                       
                    }
                }
                // Here we allow all the variable names to be cached. 
                // The file path that includes the variables can also included.
                vector<string> cur_lrd_var_cache_file_list;
                HDF5RequestHandler::get_lrd_var_cache_file_list(cur_lrd_var_cache_file_list);
                if(cur_lrd_var_cache_file_list.size() >0){
///for(int i =0; i<cur_lrd_var_cache_file_list.size();i++)
//cerr<<"lrd var cache is "<<cur_lrd_var_cache_file_list[i]<<endl;
                    if(true == check_var_cache_files(cur_lrd_var_cache_file_list,filename,varname)){
//cerr<<"varname is "<<varname <<endl;
//cerr<<"have var cached "<<endl;

                         // Only the data with the numeric datatype DAP2 and CF support are cached. 
                        if(HDF5CFUtil::cf_strict_support_type(dtype)==true) 
                            use_cache_flag = 4;
                    }
                }
            }
        }
    }

    if(0 == use_cache_flag) 
        read_data_NOT_from_mem_cache(false,NULL);
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

        handle_data_with_mem_cache(dtype,total_elems,use_cache_flag,cache_key);

    }

    return true;
}

void HDF5CFArray::read_data_NOT_from_mem_cache(bool add_cache,void*buf) {

    vector<int>offset;
    vector<int>count;
    vector<int>step;
    vector<hsize_t> hoffset;
    vector<hsize_t>hcount;
    vector<hsize_t>hstep;
    int nelms = 1;

    if (rank < 0) 
        throw InternalErr (__FILE__, __LINE__,
                          "The number of dimension of the variable is negative.");
    else if (rank == 0) 
        nelms = 1;
    else {

        offset.resize(rank);
        count.resize(rank);
        step.resize(rank);
        hoffset.resize(rank);
        hcount.resize(rank);
        hstep.resize(rank);
        nelms = format_constraint (&offset[0], &step[0], &count[0]);
        for (int i = 0; i <rank; i++) {
            hoffset[i] = (hsize_t) offset[i];
            hcount[i] = (hsize_t) count[i];
            hstep[i] = (hsize_t) step[i];
        }
    }

    hid_t dsetid = -1;
    hid_t dspace = -1;
    hid_t mspace = -1;
    hid_t dtypeid = -1;
    hid_t memtype = -1;

   
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

    if (rank > 0) {
        if (H5Sselect_hyperslab(dspace, H5S_SELECT_SET,
                               &hoffset[0], &hstep[0],
                               &hcount[0], NULL) < 0) {

            H5Sclose(dspace);
            H5Dclose(dsetid);
            HDF5CFUtil::close_fileid(fileid,pass_fileid);
            ostringstream eherr;
            eherr << "The selection of hyperslab of the HDF5 dataset " << varname
                  << " fails. "<<endl;
            throw InternalErr (__FILE__, __LINE__, eherr.str ());
        }

        mspace = H5Screate_simple(rank, &hcount[0],NULL);
        if (mspace < 0) {
            H5Sclose(dspace);
            H5Dclose(dsetid); 
            HDF5CFUtil::close_fileid(fileid,pass_fileid);
            //H5Fclose(fileid);
            ostringstream eherr;
            eherr << "The creation of the memory space of the  HDF5 dataset " << varname
                  << " fails. "<<endl;
            throw InternalErr (__FILE__, __LINE__, eherr.str ());
        }
    }


    if ((dtypeid = H5Dget_type(dsetid)) < 0) {
            
        if (rank >0) 
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

        if (rank >0) 
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
    if(true == add_cache) {
        read_ret= H5Dread(dsetid,memtype,H5S_ALL,H5S_ALL,H5P_DEFAULT,buf);
        if(read_ret <0){ 
            if (rank >0) 
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
     
    switch (dtype) {

        case H5UCHAR:
                
        {
            vector<unsigned char> val;
            val.resize(nelms);
            
            if (0 == rank) 
                read_ret = H5Dread(dsetid,memtype,H5S_ALL,H5S_ALL,H5P_DEFAULT,&val[0]);
            else 
                read_ret = H5Dread(dsetid,memtype,mspace,dspace,H5P_DEFAULT,&val[0]);

            if (read_ret < 0) {
                if (rank > 0) 
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
            set_value ((dods_byte *) &val[0], nelms);
        } // case H5UCHAR
            break;


        case H5CHAR:
        {

            vector<char> val;
            val.resize(nelms);

            if (0 == rank) 
                read_ret = H5Dread(dsetid,memtype,H5S_ALL,H5S_ALL,H5P_DEFAULT,&val[0]);
            else 
                read_ret = H5Dread(dsetid,memtype,mspace,dspace,H5P_DEFAULT,&val[0]);

            if (read_ret < 0) {

                if (rank > 0) 
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

            vector<short>newval;
            newval.resize(nelms);

            for (int counter = 0; counter < nelms; counter++)
                newval[counter] = (short) (val[counter]);

            set_value ((dods_int16 *) &newval[0], nelms);
        } // case H5CHAR
           break;


        case H5INT16:
        {
            vector<short>val;
            val.resize(nelms);
                
            if (0 == rank) 
                read_ret = H5Dread(dsetid,memtype,H5S_ALL,H5S_ALL,H5P_DEFAULT,&val[0]);
            else 
                read_ret = H5Dread(dsetid,memtype,mspace,dspace,H5P_DEFAULT,&val[0]);

            if (read_ret < 0) {

                if (rank > 0) 
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
            set_value ((dods_int16 *) &val[0], nelms);
        }// H5INT16
            break;


        case H5UINT16:
            {
                vector<unsigned short> val;
                val.resize(nelms);
                if (0 == rank) 
                   read_ret = H5Dread(dsetid,memtype,H5S_ALL,H5S_ALL,H5P_DEFAULT,&val[0]);
                else 
                   read_ret = H5Dread(dsetid,memtype,mspace,dspace,H5P_DEFAULT,&val[0]);

                if (read_ret < 0) {

                    if (rank > 0) H5Sclose(mspace);
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
                set_value ((dods_uint16 *) &val[0], nelms);
            } // H5UINT16
            break;


        case H5INT32:
        {
            vector<int>val;
            val.resize(nelms);
            if (0 == rank) 
                read_ret = H5Dread(dsetid,memtype,H5S_ALL,H5S_ALL,H5P_DEFAULT,&val[0]);
            else 
                read_ret = H5Dread(dsetid,memtype,mspace,dspace,H5P_DEFAULT,&val[0]);

            if (read_ret < 0) {
                if (rank > 0) 
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
            set_value ((dods_int32 *) &val[0], nelms);
        } // case H5INT32
            break;

        case H5UINT32:
        {
            vector<unsigned int>val;
            val.resize(nelms);
            if (0 == rank) 
                read_ret = H5Dread(dsetid,memtype,H5S_ALL,H5S_ALL,H5P_DEFAULT,&val[0]);
            else 
                read_ret = H5Dread(dsetid,memtype,mspace,dspace,H5P_DEFAULT,&val[0]);

            if (read_ret < 0) {

                if (rank > 0) 
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
            set_value ((dods_uint32 *) &val[0], nelms);
        }
            break;

        case H5FLOAT32:
        {

            vector<float>val;
            val.resize(nelms);

            if (0 == rank) 
                read_ret = H5Dread(dsetid,memtype,H5S_ALL,H5S_ALL,H5P_DEFAULT,&val[0]);
            else 
                read_ret = H5Dread(dsetid,memtype,mspace,dspace,H5P_DEFAULT,&val[0]);

            if (read_ret < 0) {
                if (rank > 0) 
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
            set_value ((dods_float32 *) &val[0], nelms);
        }
            break;


        case H5FLOAT64:
        {

            vector<double>val;
            val.resize(nelms);
            if (0 == rank) 
                read_ret = H5Dread(dsetid,memtype,H5S_ALL,H5S_ALL,H5P_DEFAULT,&val[0]);
            else 
                read_ret = H5Dread(dsetid,memtype,mspace,dspace,H5P_DEFAULT,&val[0]);

            if (read_ret < 0) {
                if (rank > 0) 
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
            set_value ((dods_float64 *) &val[0], nelms);
        } // case H5FLOAT64
            break;


        case H5FSTRING:
        {
            size_t ty_size = H5Tget_size(dtypeid);
            if (ty_size == 0) {
                if (rank >0) 
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
            if (0 == rank) 
                read_ret = H5Dread(dsetid,memtype,H5S_ALL,H5S_ALL,H5P_DEFAULT,(void*)&strval[0]);
            else 
                read_ret = H5Dread(dsetid,memtype,mspace,dspace,H5P_DEFAULT,(void*)&strval[0]);

            if (read_ret < 0) {
                if (rank > 0) 
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
            strval.clear();//save some memory;
            vector <string> finstrval;
            finstrval.resize(nelms);
            for (int i = 0; i<nelms; i++) 
                  finstrval[i] = total_string.substr(i*ty_size,ty_size);
            
            // Check if we should drop the long string

            // If the string size is longer than the current netCDF JAVA
            // string and the "EnableDropLongString" key is turned on,
            // No string is generated.
            if ((true == HDF5RequestHandler::get_drop_long_string()) &&
                total_string.size() > NC_JAVA_STR_SIZE_LIMIT) {
                for (int i = 0; i<nelms; i++)
                    finstrval[i] = "";
            }
            set_value(finstrval,nelms);
            total_string.clear();
        }
            break;


        case H5VSTRING:
        {
            size_t ty_size = H5Tget_size(memtype);
            if (ty_size == 0) {
                if (rank >0) 
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
            if (0 == rank) 
                read_ret = H5Dread(dsetid,memtype,H5S_ALL,H5S_ALL,H5P_DEFAULT,(void*)&strval[0]);
            else 
                read_ret = H5Dread(dsetid,memtype,mspace,dspace,H5P_DEFAULT,(void*)&strval[0]);

            if (read_ret < 0) {
                if (rank > 0) 
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
            char*temp_bp = &strval[0];
            char*onestring = NULL;
            for (int i =0;i<nelms;i++) {
                onestring = *(char**)temp_bp;
                if(onestring!=NULL ) 
                    finstrval[i] =string(onestring);
                
                else // We will add a NULL if onestring is NULL.
                    finstrval[i]="";
                temp_bp +=ty_size;
            }

            if (false == strval.empty()) {
                herr_t ret_vlen_claim;
                if (0 == rank) 
                    ret_vlen_claim = H5Dvlen_reclaim(memtype,dspace,H5P_DEFAULT,(void*)&strval[0]);
                else 
                    ret_vlen_claim = H5Dvlen_reclaim(memtype,mspace,H5P_DEFAULT,(void*)&strval[0]);
                if (ret_vlen_claim < 0){
                    if (rank >0) 
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

            // If the string size is longer than the current netCDF JAVA
            // string and the "EnableDropLongString" key is turned on,
            // No string is generated.
            if (true == HDF5RequestHandler::get_drop_long_string()) {
                size_t total_str_size = 0;
                for (int i =0;i<nelms;i++) 
                    total_str_size += finstrval[i].size();
                if (total_str_size  > NC_JAVA_STR_SIZE_LIMIT) {
                    for (int i =0;i<nelms;i++)
                        finstrval[i] = "";
                }
            }
            set_value(finstrval,nelms);

        }
            break;

        default:
        {
                H5Tclose(memtype);
                H5Tclose(dtypeid);
                if (0 == rank) 
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
    if (rank == 0)
        H5Sclose(mspace);
    H5Sclose(dspace);
    H5Dclose(dsetid);
    HDF5CFUtil::close_fileid(fileid,pass_fileid);
    
    return;
}

#if 0
void HDF5CFArray::read_data_from_mem_cache(void*buf) {

    vector<int>offset;
    vector<int>count;
    vector<int>step;
    int nelms = format_constraint (&offset[0], &step[0], &count[0]);
    // set the original position to the starting point
    vector<int>at_pos(at_ndims,0);
    for (int i = 0; i< rank; i++)
        at_pos[i] = at_offset[i];


    switch (dtype) {

        case H5UCHAR:
                
        {
            vector<unsigned char> val;
            val.resize(nelms);
            subset<unsigned char>(
                                      &total_val[0],
                                      rank,
                                      dimsizes,
                                      offset,
                                      step,
                                      count,
                                      &final_val,
                                      at_pos,
                                      0
                                     );
                

            set_value ((dods_byte *) &val[0], nelms);
        } // case H5UCHAR
            break;


        case H5CHAR:
        {

            vector<char> val;
            val.resize(nelms);

            if (0 == rank) 
                read_ret = H5Dread(dsetid,memtype,H5S_ALL,H5S_ALL,H5P_DEFAULT,&val[0]);
            else 
                read_ret = H5Dread(dsetid,memtype,mspace,dspace,H5P_DEFAULT,&val[0]);

            if (read_ret < 0) {

                if (rank > 0) 
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

            vector<short>newval;
            newval.resize(nelms);

            for (int counter = 0; counter < nelms; counter++)
                newval[counter] = (short) (val[counter]);

            set_value ((dods_int16 *) &newval[0], nelms);
        } // case H5CHAR
           break;


        case H5INT16:
        {
            vector<short>val;
            val.resize(nelms);
                
            if (0 == rank) 
                read_ret = H5Dread(dsetid,memtype,H5S_ALL,H5S_ALL,H5P_DEFAULT,&val[0]);
            else 
                read_ret = H5Dread(dsetid,memtype,mspace,dspace,H5P_DEFAULT,&val[0]);

            if (read_ret < 0) {

                if (rank > 0) 
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
            set_value ((dods_int16 *) &val[0], nelms);
        }// H5INT16
            break;


        case H5UINT16:
            {
                vector<unsigned short> val;
                val.resize(nelms);
                if (0 == rank) 
                   read_ret = H5Dread(dsetid,memtype,H5S_ALL,H5S_ALL,H5P_DEFAULT,&val[0]);
                else 
                   read_ret = H5Dread(dsetid,memtype,mspace,dspace,H5P_DEFAULT,&val[0]);

                if (read_ret < 0) {

                    if (rank > 0) H5Sclose(mspace);
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
                set_value ((dods_uint16 *) &val[0], nelms);
            } // H5UINT16
            break;


        case H5INT32:
        {
            vector<int>val;
            val.resize(nelms);
            if (0 == rank) 
                read_ret = H5Dread(dsetid,memtype,H5S_ALL,H5S_ALL,H5P_DEFAULT,&val[0]);
            else 
                read_ret = H5Dread(dsetid,memtype,mspace,dspace,H5P_DEFAULT,&val[0]);

            if (read_ret < 0) {
                if (rank > 0) 
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
            set_value ((dods_int32 *) &val[0], nelms);
        } // case H5INT32
            break;

        case H5UINT32:
        {
            vector<unsigned int>val;
            val.resize(nelms);
            if (0 == rank) 
                read_ret = H5Dread(dsetid,memtype,H5S_ALL,H5S_ALL,H5P_DEFAULT,&val[0]);
            else 
                read_ret = H5Dread(dsetid,memtype,mspace,dspace,H5P_DEFAULT,&val[0]);

            if (read_ret < 0) {

                if (rank > 0) 
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
            set_value ((dods_uint32 *) &val[0], nelms);
        }
            break;

        case H5FLOAT32:
        {

            vector<float>val;
            val.resize(nelms);

            if (0 == rank) 
                read_ret = H5Dread(dsetid,memtype,H5S_ALL,H5S_ALL,H5P_DEFAULT,&val[0]);
            else 
                read_ret = H5Dread(dsetid,memtype,mspace,dspace,H5P_DEFAULT,&val[0]);

            if (read_ret < 0) {
                if (rank > 0) 
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
            set_value ((dods_float32 *) &val[0], nelms);
        }
            break;


        case H5FLOAT64:
        {

            vector<double>val;
            val.resize(nelms);
            if (0 == rank) 
                read_ret = H5Dread(dsetid,memtype,H5S_ALL,H5S_ALL,H5P_DEFAULT,&val[0]);
            else 
                read_ret = H5Dread(dsetid,memtype,mspace,dspace,H5P_DEFAULT,&val[0]);

            if (read_ret < 0) {
                if (rank > 0) 
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
            set_value ((dods_float64 *) &val[0], nelms);
        } // case H5FLOAT64
            break;



    // Just see if it works.
    val2buf(buf);
    set_read_p(true);
    return;
}
#endif

#if 0
// parse constraint expr. and make hdf5 coordinate point location.
// return number of elements to read. 
int
HDF5CFArray::format_constraint (int *offset, int *step, int *count)
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
