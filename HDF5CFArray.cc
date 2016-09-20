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

void HDF5CFArray::read_data_from_mem_cache(void*buf) {

    // Just see if it works.
    val2buf(buf);
    set_read_p(true);
    return;
}
// Read in an HDF5 Array 
bool HDF5CFArray::read()
{

    BESDEBUG("h5","Coming to HDF5CFArray read "<<endl);
    if(length() == 0)
        return true;

    
#if 0
    if(HDF5RequestHandler::check_dds_cache())
        BESDEBUG("h5","have DDS cache "<<endl);
    else 
        BESDEBUG("h5","Dont' have DDS cache "<<endl);
#endif

    // Check if using the memory cache
    if((NULL == HDF5RequestHandler::get_data_mem_cache()) || (false == HDF5CFUtil::use_data_mem_cache(dtype,cvtype,varname))){ 
cerr<<"no mem cache "<<endl;
        read_data_from_file(false,NULL);
    }
    else {// Using the memory cache
        
// UNCOMMENT LATER
//#if 0
    ObjMemCache* my_data_cache = HDF5RequestHandler::get_data_mem_cache();
    //HDF5DataMemCache * cached_h5data_mem_cache_ptr = 0;
    if(my_data_cache) {
        // Cache key needs to be filename+varname.
        HDF5DataMemCache* cached_h5data_mem_cache_ptr = static_cast<HDF5DataMemCache*>((HDF5RequestHandler::get_data_mem_cache())->get(filename+varname));
        if(cached_h5data_mem_cache_ptr) {
            BESDEBUG("h5","Data Memory Cache hit "<<endl);
 cerr<<"Cache hit: varname is "<<varname <<endl;
            const string var_name = cached_h5data_mem_cache_ptr->get_varname();
 cerr<<"my variable name is "<<var_name <<endl;

            // Obtain the buffer and do subsetting
            const size_t var_size = cached_h5data_mem_cache_ptr->get_var_buf_size();
cerr<<"var buf size is "<<var_size <<endl;
            if(!var_size) 
                throw InternalErr(__FILE__,__LINE__,"The cached data buffer size is 0.");
            else {
                vector<char> buf;
                buf.resize(var_size);
                cached_h5data_mem_cache_ptr->get_var_buf(buf);
                read_data_from_mem_cache(&buf[0]);
            }
        }
        else{ 
            BESDEBUG("h5","Data memory added to the cache "<<endl);
//cerr<<"coming to add data memory cache "<<endl;
cerr<<"Cache added: varname is "<<varname <<endl;
            vector <char> buf;
            if(total_elems == 0)
                throw InternalErr(__FILE__,__LINE__,"The total number of elements is 0.");
            buf.resize(total_elems*HDF5CFUtil::H5_numeric_atomic_type_size(dtype));
            read_data_from_file(true,&buf[0]);
            // Add the buffer 
            
            HDF5DataMemCache* new_mem_cache = new HDF5DataMemCache(varname);
            new_mem_cache->set_databuf(buf);
            my_data_cache->add(new_mem_cache, filename+varname);
        }
    }
    }
//#endif
// END OF UNCOMMENT

    return true;
}

void HDF5CFArray::read_data_from_file(bool add_cache,void*buf) {

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
    //H5Fclose(fileid);
    
    return ;
}


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

