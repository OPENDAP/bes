// This file is part of the hdf5_handler implementing for the CF-compliant
// Copyright (c) 2011 The HDF Group, Inc. and OPeNDAP, Inc.
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
#include <debug.h>
#include "InternalErr.h"
#include <BESDebug.h>

#include "Str.h"
#include "HDF5CFArray.h"



BaseType *HDF5CFArray::ptr_duplicate()
{
    return new HDF5CFArray(*this);
}

// Read in an HDF5 Array 
bool HDF5CFArray::read()
{

    int* offset = NULL;
    int* count = NULL ;
    int* step = NULL;
    hsize_t* hoffset = NULL;
    hsize_t* hcount = NULL;
    hsize_t* hstep = NULL;
    int nelms = 0;

#if 0
cerr<<"coming to read function"<<endl;
cerr<<"file name " <<filename <<endl;
cerr <<"var name "<<varname <<endl;
#endif

    if (rank < 0) 
        throw InternalErr (__FILE__, __LINE__,
                          "The number of dimension of the variable is negative.");

    else if (rank == 0) 
            nelms = 1;
    else {
        try {
            offset = new int[rank];
            count = new int[rank];
            step = new int[rank];
            hoffset = new hsize_t[rank];
            hcount = new hsize_t[rank];
            hstep = new hsize_t[rank];
        }

        catch (...) {
            HDF5CFUtil::ClearMem(offset,count,step,hoffset,hcount,hstep);
            throw InternalErr (__FILE__, __LINE__,
                          "Cannot allocate the memory for offset,count and setp.");
        }

        nelms = format_constraint (offset, step, count);

        for (int i = 0; i <rank; i++) {
            hoffset[i] = (hsize_t) offset[i];
            hcount[i] = (hsize_t) count[i];
            hstep[i] = (hsize_t) step[i];
        }

    }

    hid_t fileid = -1;
    hid_t dsetid = -1;
    hid_t dspace = -1;
    hid_t mspace = -1;
    hid_t dtypeid = -1;
    hid_t memtype = -1;

    if ((fileid = H5Fopen(filename.c_str(),H5F_ACC_RDONLY,H5P_DEFAULT))<0) {

        HDF5CFUtil::ClearMem(offset,count,step,hoffset,hcount,hstep);
        ostringstream eherr;
        eherr << "HDF5 File " << filename 
              << " cannot be opened. "<<endl;
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }

    if ((dsetid = H5Dopen(fileid,varname.c_str(),H5P_DEFAULT))<0) {

        H5Fclose(fileid);
        HDF5CFUtil::ClearMem(offset,count,step,hoffset,hcount,hstep);
        ostringstream eherr;
        eherr << "HDF5 dataset " << varname
              << " cannot be opened. "<<endl;
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }

    if ((dspace = H5Dget_space(dsetid))<0) {

        H5Dclose(dsetid);
        H5Fclose(fileid);
        HDF5CFUtil::ClearMem(offset,count,step,hoffset,hcount,hstep);
        ostringstream eherr;
        eherr << "Space id of the HDF5 dataset " << varname
              << " cannot be obtained. "<<endl;
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }

    if (rank > 0) {
        if (H5Sselect_hyperslab(dspace, H5S_SELECT_SET,
                               hoffset, hstep,
                               hcount, NULL) < 0) {

            H5Sclose(dspace);
            H5Dclose(dsetid);
            H5Fclose(fileid);
            HDF5CFUtil::ClearMem(offset,count,step,hoffset,hcount,hstep);
            ostringstream eherr;
            eherr << "The selection of hyperslab of the HDF5 dataset " << varname
                  << " fails. "<<endl;
            throw InternalErr (__FILE__, __LINE__, eherr.str ());
        }

        mspace = H5Screate_simple(rank, hcount,NULL);
        if (mspace < 0) {
            H5Sclose(dspace);
            H5Dclose(dsetid); 
            H5Fclose(fileid);
            HDF5CFUtil::ClearMem(offset,count,step,hoffset,hcount,hstep);
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
        H5Fclose(fileid);
        HDF5CFUtil::ClearMem(offset,count,step,hoffset,hcount,hstep);
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
        H5Fclose(fileid);
        HDF5CFUtil::ClearMem(offset,count,step,hoffset,hcount,hstep);
        ostringstream eherr;
        eherr << "Obtaining the memory type of the HDF5 dataset " << varname
              << " fails. "<<endl;
        throw InternalErr (__FILE__, __LINE__, eherr.str ());

    }

    // Now reading the data, note dtype is not dtypeid.
    // dtype is an enum  defined by the handler.
     
    void *val = NULL;
    hid_t read_ret = -1;

    switch (dtype) {

        case H5UCHAR:
                
        {
            try {
                val = new unsigned char[nelms];
            }
            catch (...) {
                if (rank >0) 
                    H5Sclose(mspace);
                H5Tclose(memtype);
                H5Tclose(dtypeid);
                H5Sclose(dspace);
                H5Dclose(dsetid);
                H5Fclose(fileid);
                HDF5CFUtil::ClearMem(offset,count,step,hoffset,hcount,hstep);
                ostringstream eherr;
                eherr << "Cannot allocate memory for dataset " << varname
                      << " with the type of H5T_NATIVE_UCHAR "<<endl;
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }

            
            if (0 == rank) 
                read_ret = H5Dread(dsetid,memtype,H5S_ALL,H5S_ALL,H5P_DEFAULT,val);
            else 
                read_ret = H5Dread(dsetid,memtype,mspace,dspace,H5P_DEFAULT,val);

            if (read_ret < 0) {
                if (rank > 0) 
                    H5Sclose(mspace);
                H5Tclose(memtype);
                H5Tclose(dtypeid);
                H5Sclose(dspace);
                H5Dclose(dsetid);
                H5Fclose(fileid);
                HDF5CFUtil::ClearMem(offset,count,step,hoffset,hcount,hstep);
                ostringstream eherr;
                eherr << "Cannot read the HDF5 dataset " << varname
                      << " with the type of H5T_NATIVE_UCHAR "<<endl;
                throw InternalErr (__FILE__, __LINE__, eherr.str ());

            }
            set_value ((dods_byte *) val, nelms);
            if (val != NULL) 
                delete[] (unsigned char*)val; 
        } // case H5UCHAR
            break;


        case H5CHAR:
        {
            try {
                val = new char[nelms];
            }
            catch (...) {
                    
                if (rank >0) 
                    H5Sclose(mspace);
                H5Tclose(memtype);
                H5Tclose(dtypeid);
                H5Sclose(dspace);
                H5Dclose(dsetid);
                H5Fclose(fileid);
                HDF5CFUtil::ClearMem(offset,count,step,hoffset,hcount,hstep);
                ostringstream eherr;
                eherr << "Cannot allocate memory for dataset " << varname
                      << " with the type of H5T_NATIVE_UCHAR "<<endl;
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }

            if (0 == rank) 
                read_ret = H5Dread(dsetid,memtype,H5S_ALL,H5S_ALL,H5P_DEFAULT,val);
            else 
                read_ret = H5Dread(dsetid,memtype,mspace,dspace,H5P_DEFAULT,val);

            if (read_ret < 0) {

                if (rank > 0) 
                    H5Sclose(mspace);
                H5Tclose(memtype);
                H5Tclose(dtypeid);
                H5Sclose(dspace);
                H5Dclose(dsetid);
                H5Fclose(fileid);
                HDF5CFUtil::ClearMem(offset,count,step,hoffset,hcount,hstep);
                ostringstream eherr;
                eherr << "Cannot read the HDF5 dataset " << varname
                      << " with the type of H5T_NATIVE_CHAR "<<endl;
                throw InternalErr (__FILE__, __LINE__, eherr.str ());

            }

            short* newval = NULL;
            char* newvalchar = NULL;

            try {
                newval = new short[nelms];
            }
            catch (...) {
                if (rank >0) 
                    H5Sclose(mspace);
                H5Tclose(memtype);
                H5Tclose(dtypeid);
                H5Sclose(dspace);
                H5Dclose(dsetid);
                H5Fclose(fileid);
                HDF5CFUtil::ClearMem(offset,count,step,hoffset,hcount,hstep);
                if(val != NULL) 
                    delete[] (char *)val;
                ostringstream eherr;
                eherr << "Cannot allocate memory for dataset " << varname
                      << " with the type of H5T_NATIVE_CHAR "<<endl;
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }

            newvalchar = (char*)val;

            for (int counter = 0; counter < nelms; counter++)
                newval[counter] = (short) (newvalchar[counter]);

            set_value ((dods_int16 *) newval, nelms);
            if (val != NULL) 
                delete[] (char*)val; 
            if (newval != NULL) 
                delete[]newval;
        } // case H5CHAR
           break;


        case H5INT16:
        {
            try {
                val = new short[nelms];
            }
            catch (...) {
                if (rank >0) 
                    H5Sclose(mspace);
                H5Tclose(memtype);
                H5Tclose(dtypeid);
                H5Sclose(dspace);
                H5Dclose(dsetid);
                H5Fclose(fileid);
                HDF5CFUtil::ClearMem(offset,count,step,hoffset,hcount,hstep);
                ostringstream eherr;
                eherr << "Cannot allocate memory for dataset " << varname
                      << " with the type of H5T_NATIVE_SHORT "<<endl;
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }

                
            if (0 == rank) 
                read_ret = H5Dread(dsetid,memtype,H5S_ALL,H5S_ALL,H5P_DEFAULT,val);
            else 
                read_ret = H5Dread(dsetid,memtype,mspace,dspace,H5P_DEFAULT,val);

            if (read_ret < 0) {

                if (rank > 0) 
                    H5Sclose(mspace);
                H5Tclose(memtype);
                H5Tclose(dtypeid);
                H5Sclose(dspace);
                H5Dclose(dsetid);
                H5Fclose(fileid);
                HDF5CFUtil::ClearMem(offset,count,step,hoffset,hcount,hstep);
                ostringstream eherr;
                eherr << "Cannot read the HDF5 dataset " << varname
                      << " with the type of H5T_NATIVE_SHORT "<<endl;
                throw InternalErr (__FILE__, __LINE__, eherr.str ());

            }
            set_value ((dods_int16 *) val, nelms);
            if (val != NULL) 
                delete[] (short*)val; 
        }// H5INT16
            break;


        case H5UINT16:
            {
                try {
                    val = new unsigned short[nelms];
                }
                catch (...) {
                    if (rank >0) H5Sclose(mspace);
                    H5Tclose(memtype);
                    H5Tclose(dtypeid);
                    H5Sclose(dspace);
                    H5Dclose(dsetid);
                    H5Fclose(fileid);
                    HDF5CFUtil::ClearMem(offset,count,step,hoffset,hcount,hstep);
                    ostringstream eherr;
                    eherr << "Cannot allocate memory for dataset " << varname
                        << " with the type of H5T_NATIVE_USHORT "<<endl;
                    throw InternalErr (__FILE__, __LINE__, eherr.str ());
                }
                if (0 == rank) read_ret = H5Dread(dsetid,memtype,H5S_ALL,H5S_ALL,H5P_DEFAULT,val);
                else read_ret = H5Dread(dsetid,memtype,mspace,dspace,H5P_DEFAULT,val);

                if (read_ret < 0) {

                    if (rank > 0) H5Sclose(mspace);
                    H5Tclose(memtype);
                    H5Tclose(dtypeid);
                    H5Sclose(dspace);
                    H5Dclose(dsetid);
                    H5Fclose(fileid);
                    HDF5CFUtil::ClearMem(offset,count,step,hoffset,hcount,hstep);
                    ostringstream eherr;
                    eherr << "Cannot read the HDF5 dataset " << varname
                        << " with the type of H5T_NATIVE_USHORT "<<endl;
                    throw InternalErr (__FILE__, __LINE__, eherr.str ());

                }
                set_value ((dods_uint16 *) val, nelms);
                if (val != NULL) 
                    delete[] (unsigned short*)val;        
            } // H5UINT16
            break;


        case H5INT32:
        {
            try {
                val = new int[nelms];
            }
            catch (...) {

                if (rank >0) 
                    H5Sclose(mspace);
                H5Tclose(memtype);
                H5Tclose(dtypeid);
                H5Sclose(dspace);
                H5Dclose(dsetid);
                H5Fclose(fileid);
                HDF5CFUtil::ClearMem(offset,count,step,hoffset,hcount,hstep);
                ostringstream eherr;
                eherr << "Cannot allocate memory for dataset " << varname
                      << " with the type of H5T_NATIVE_INT "<<endl;
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }

            if (0 == rank) 
                read_ret = H5Dread(dsetid,memtype,H5S_ALL,H5S_ALL,H5P_DEFAULT,val);
            else 
                read_ret = H5Dread(dsetid,memtype,mspace,dspace,H5P_DEFAULT,val);

            if (read_ret < 0) {
                if (rank > 0) 
                    H5Sclose(mspace);
                H5Tclose(memtype);
                H5Tclose(dtypeid);
                H5Sclose(dspace);
                H5Dclose(dsetid);
                H5Fclose(fileid);
                HDF5CFUtil::ClearMem(offset,count,step,hoffset,hcount,hstep);
                ostringstream eherr;
                eherr << "Cannot read the HDF5 dataset " << varname
                      << " with the type of H5T_NATIVE_INT "<<endl;
                throw InternalErr (__FILE__, __LINE__, eherr.str ());

            }
            set_value ((dods_int32 *) val, nelms);
            if (val!=NULL) 
                delete[] (int*)val;        
        } // case H5INT32
            break;

        case H5UINT32:
        {
            try {
                val = new unsigned int[nelms];
            }
            catch (...) {
                if (rank >0) 
                    H5Sclose(mspace);
                H5Tclose(memtype);
                H5Tclose(dtypeid);
                H5Sclose(dspace);
                H5Dclose(dsetid);
                H5Fclose(fileid);
                HDF5CFUtil::ClearMem(offset,count,step,hoffset,hcount,hstep);
                ostringstream eherr;
                eherr << "Cannot allocate memory for dataset " << varname
                      << " with the type of H5T_NATIVE_UINT "<<endl;
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }
            if (0 == rank) 
                read_ret = H5Dread(dsetid,memtype,H5S_ALL,H5S_ALL,H5P_DEFAULT,val);
            else 
                read_ret = H5Dread(dsetid,memtype,mspace,dspace,H5P_DEFAULT,val);

            if (read_ret < 0) {

                if (rank > 0) 
                    H5Sclose(mspace);
                H5Tclose(memtype);
                H5Tclose(dtypeid);
                H5Sclose(dspace);
                H5Dclose(dsetid);
                H5Fclose(fileid);
                HDF5CFUtil::ClearMem(offset,count,step,hoffset,hcount,hstep);
                ostringstream eherr;
                eherr << "Cannot read the HDF5 dataset " << varname
                      << " with the type of H5T_NATIVE_UINT "<<endl;
                throw InternalErr (__FILE__, __LINE__, eherr.str ());

            }
            set_value ((dods_uint32 *) val, nelms);
            if (val != NULL) 
                delete[] (unsigned int*)val;        
        }
            break;

        case H5FLOAT32:
        {
            try {
                val = new float[nelms];
            }
            catch (...) {
                if (rank >0) 
                    H5Sclose(mspace);
                H5Tclose(memtype);
                H5Tclose(dtypeid);
                H5Sclose(dspace);
                H5Dclose(dsetid);
                H5Fclose(fileid);
                HDF5CFUtil::ClearMem(offset,count,step,hoffset,hcount,hstep);
                ostringstream eherr;
                eherr << "Cannot allocate memory for dataset " << varname
                      << " with the type of H5T_NATIVE_FLOAT "<<endl;
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }

            if (0 == rank) 
                read_ret = H5Dread(dsetid,memtype,H5S_ALL,H5S_ALL,H5P_DEFAULT,val);
            else 
                read_ret = H5Dread(dsetid,memtype,mspace,dspace,H5P_DEFAULT,val);

            if (read_ret < 0) {
                if (rank > 0) 
                    H5Sclose(mspace);
                H5Tclose(memtype);
                H5Tclose(dtypeid);
                H5Sclose(dspace);
                H5Dclose(dsetid);
                H5Fclose(fileid);
                HDF5CFUtil::ClearMem(offset,count,step,hoffset,hcount,hstep);
                ostringstream eherr;
                eherr << "Cannot read the HDF5 dataset " << varname
                      << " with the type of H5T_NATIVE_FLOAT "<<endl;
                throw InternalErr (__FILE__, __LINE__, eherr.str ());

            }
            set_value ((dods_float32 *) val, nelms);
            if (val != NULL) 
                delete[] (float*)val;        
        }
            break;


        case H5FLOAT64:
        {
            try {
                val = new double[nelms];
            }
            catch (...) {
                if (rank > 0) 
                    H5Sclose(mspace);
                H5Tclose(memtype);
                H5Tclose(dtypeid);
                H5Sclose(dspace);
                H5Dclose(dsetid);
                H5Fclose(fileid);
                HDF5CFUtil::ClearMem(offset,count,step,hoffset,hcount,hstep);
                ostringstream eherr;
                eherr << "Cannot allocate memory for dataset " << varname
                      << " with the type of H5T_NATIVE_DOUBLE "<<endl;
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }
            if (0 == rank) 
                read_ret = H5Dread(dsetid,memtype,H5S_ALL,H5S_ALL,H5P_DEFAULT,val);
            else 
                read_ret = H5Dread(dsetid,memtype,mspace,dspace,H5P_DEFAULT,val);

            if (read_ret < 0) {
                if (rank > 0) 
                    H5Sclose(mspace);
                H5Tclose(memtype);
                H5Tclose(dtypeid);
                H5Sclose(dspace);
                H5Dclose(dsetid);
                H5Fclose(fileid);
                HDF5CFUtil::ClearMem(offset,count,step,hoffset,hcount,hstep);
                ostringstream eherr;
                eherr << "Cannot read the HDF5 dataset " << varname
                      << " with the type of H5T_NATIVE_DOUBLE "<<endl;
                throw InternalErr (__FILE__, __LINE__, eherr.str ());

            }
            set_value ((dods_float64 *) val, nelms);
            if (val != NULL) 
                delete[] (double*)val;        
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
                H5Fclose(fileid);
                HDF5CFUtil::ClearMem(offset,count,step,hoffset,hcount,hstep);
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
                H5Fclose(fileid);
                HDF5CFUtil::ClearMem(offset,count,step,hoffset,hcount,hstep);
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
            string check_droplongstr_key ="H5.EnableDropLongString";
            bool is_droplongstr = false;
            is_droplongstr = 
                    HDF5CFDAPUtil::check_beskeys(check_droplongstr_key);

            // If the string size is longer than the current netCDF JAVA
            // string and the "EnableDropLongString" key is turned on,
            // No string is generated.
            if (true == is_droplongstr &&
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
                H5Fclose(fileid);
                HDF5CFUtil::ClearMem(offset,count,step,hoffset,hcount,hstep);
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
                H5Fclose(fileid);
                HDF5CFUtil::ClearMem(offset,count,step,hoffset,hcount,hstep);
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
                hsize_t ret_vlen_claim;
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
                    H5Fclose(fileid);
                    HDF5CFUtil::ClearMem(offset,count,step,hoffset,hcount,hstep);
                    ostringstream eherr;
                    eherr << "Cannot reclaim the memory buffer of the HDF5 variable length string of the dataset "
                          << varname <<endl;
                    throw InternalErr (__FILE__, __LINE__, eherr.str ());
 
                }
            }

            // Check if we should drop the long string
            string check_droplongstr_key ="H5.EnableDropLongString";
            bool is_droplongstr = false;
            is_droplongstr = 
                    HDF5CFDAPUtil::check_beskeys(check_droplongstr_key);

            // If the string size is longer than the current netCDF JAVA
            // string and the "EnableDropLongString" key is turned on,
            // No string is generated.
            if (true == is_droplongstr) {
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
                H5Fclose(fileid);
                HDF5CFUtil::ClearMem(offset,count,step,hoffset,hcount,hstep);
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
    H5Fclose(fileid);
    HDF5CFUtil::ClearMem(offset,count,step,hoffset,hcount,hstep);
    
    return false;
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


                // Check for illegical  constraint
                if (stride < 0 || start < 0 || stop < 0 || start > stop) {
                        ostringstream oss;

                        oss << "Array/Grid hyperslab indices are bad: [" << start <<
                                ":" << stride << ":" << stop << "]";
                        throw Error (malformed_expr, oss.str ());
                }

                // Check for an empty constraint and use the whole dimension if so.
                if (start == 0 && stop == 0 && stride == 0) {
                        start = dimension_start (p, false);
                        stride = dimension_stride (p, false);
                        stop = dimension_stop (p, false);
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

