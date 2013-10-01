// This file is part of the hdf5_handler implementing for the CF-compliant
// Copyright (c) 2011-2013 The HDF Group, Inc. and OPeNDAP, Inc.
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
/// \file HDF5CFStr.cc
/// \brief The implementation of mapping  HDF5 String to DAP String for the CF option
///
/// In the future, this may be merged with the default option.
/// \author Muqun Yang <myang6@hdfgroup.org>
///
////////////////////////////////////////////////////////////////////////////////


#include "config_hdf5.h"

#include <iostream>
#include <sstream>

#include "InternalErr.h"
#include "h5cfdaputil.h"
#include "HDF5CFStr.h"
#include <hdf5.h>

HDF5CFStr::HDF5CFStr(const string &n, const string &d,const string &varname) 
      : Str(n, d), varname(varname)
{
}

HDF5CFStr::~HDF5CFStr()
{
}
BaseType *HDF5CFStr::ptr_duplicate()
{
    return new HDF5CFStr(*this);
}

bool HDF5CFStr::read()
{
    hid_t fileid = -1;
    hid_t dsetid = -1;
    hid_t dspace = -1;
    hid_t dtypeid = -1;
    hid_t memtype = -1;

    if ((fileid = H5Fopen(dataset().c_str(),H5F_ACC_RDONLY,H5P_DEFAULT))<0) {
        ostringstream eherr;
        eherr << "HDF5 File " << dataset()  
              << " cannot be opened. "<<endl;
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }

    if ((dsetid = H5Dopen(fileid,varname.c_str(),H5P_DEFAULT))<0) {
        H5Fclose(fileid);
        ostringstream eherr;
        eherr << "HDF5 dataset " << name()
              << " cannot be opened. "<<endl;
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }

    if ((dspace = H5Dget_space(dsetid))<0) {

        H5Dclose(dsetid);
        H5Fclose(fileid);
        ostringstream eherr;
        eherr << "Space id of the HDF5 dataset " << name()
              << " cannot be obtained. "<<endl;
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }

    if (H5S_SCALAR != H5Sget_simple_extent_type(dspace)) {

        H5Dclose(dsetid);
        H5Fclose(fileid);
        ostringstream eherr;
        eherr << " The HDF5 dataset " << name()
              << " is not scalar. "<<endl;
        throw InternalErr (__FILE__, __LINE__, eherr.str ());

    }


    if ((dtypeid = H5Dget_type(dsetid)) < 0) {
            
        H5Sclose(dspace);
        H5Dclose(dsetid);
        H5Fclose(fileid);
        ostringstream eherr;
        eherr << "Obtaining the datatype of the HDF5 dataset " << name()
              << " fails. "<<endl;
        throw InternalErr (__FILE__, __LINE__, eherr.str ());

    }

    if ((memtype = H5Tget_native_type(dtypeid, H5T_DIR_ASCEND))<0) {

        H5Tclose(dtypeid);
        H5Sclose(dspace);
        H5Dclose(dsetid);
        H5Fclose(fileid);
        ostringstream eherr;
        eherr << "Obtaining the memory type of the HDF5 dataset " << name()
              << " fails. "<<endl;
        throw InternalErr (__FILE__, __LINE__, eherr.str ());

    }

    // Check if we should drop the long string
    string check_droplongstr_key ="H5.EnableDropLongString";
    bool is_droplongstr = false;
    is_droplongstr = HDF5CFDAPUtil::check_beskeys(check_droplongstr_key);

    htri_t is_vlen_str = H5Tis_variable_str(dtypeid);
    if (is_vlen_str > 0) {
        size_t ty_size = H5Tget_size(memtype);
        if (ty_size == 0) {
            H5Tclose(memtype);
            H5Tclose(dtypeid);
            H5Sclose(dspace);
            H5Dclose(dsetid);
            H5Fclose(fileid);
            ostringstream eherr;
            eherr << "Cannot obtain the size of the fixed size HDF5 string of the dataset "
                  << name() <<endl;
            throw InternalErr (__FILE__, __LINE__, eherr.str ());
        }
        vector <char> strval;
        strval.resize(ty_size);
        hid_t read_ret = -1;
        read_ret = H5Dread(dsetid,memtype,H5S_ALL,H5S_ALL,H5P_DEFAULT,(void*)&strval[0]);

        if (read_ret < 0) {
            H5Tclose(memtype);
            H5Tclose(dtypeid);
            H5Sclose(dspace);
            H5Dclose(dsetid);
            H5Fclose(fileid);
            ostringstream eherr;
            eherr << "Cannot read the HDF5 dataset " << name()
                  << " with the type of the HDF5 variable length string "<<endl;
            throw InternalErr (__FILE__, __LINE__, eherr.str ());
        }

        char*temp_bp = &strval[0];
        char*onestring = NULL;
        string final_str ="";
               
        onestring = *(char**)temp_bp;
        if(onestring!=NULL ) 
            final_str =string(onestring);
                 
        else // We will add a NULL is onestring is NULL.
            final_str="";

        if (""!=final_str) {
            hsize_t ret_vlen_claim = 0;
            ret_vlen_claim = H5Dvlen_reclaim(memtype,dspace,H5P_DEFAULT,(void*)&strval[0]);
            if (ret_vlen_claim < 0){
                H5Tclose(memtype);
                H5Tclose(dtypeid);
                H5Sclose(dspace);
                H5Dclose(dsetid);
                H5Fclose(fileid);
                ostringstream eherr;
                eherr << "Cannot reclaim the memory buffer of the HDF5 variable length string of the dataset "
                      << name() <<endl;
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
 
            }
        }

        // If the string size is longer than the current netCDF JAVA
        // string and the "EnableDropLongString" key is turned on,
        // No string is generated.
        if (true == is_droplongstr) {
            if( final_str.size() > NC_JAVA_STR_SIZE_LIMIT) 
                final_str = "";
        }
        set_value(final_str);
    }     

    else if (0 == is_vlen_str) {
        size_t ty_size = H5Tget_size(dtypeid);
        if (ty_size == 0) {
            H5Tclose(memtype);
            H5Tclose(dtypeid);
            H5Sclose(dspace);
            H5Dclose(dsetid);
            H5Fclose(fileid);
            ostringstream eherr;
            eherr << "Cannot obtain the size of the fixed size HDF5 string of the dataset " 
                  << name() <<endl;
            throw InternalErr (__FILE__, __LINE__, eherr.str ());
        }

        vector <char> strval;
        strval.resize(1+ty_size);
        hid_t read_ret = -1;
        read_ret = H5Dread(dsetid,memtype,H5S_ALL,H5S_ALL,H5P_DEFAULT,(void*)&strval[0]);

        if (read_ret < 0) {
            H5Tclose(memtype);
            H5Tclose(dtypeid);
            H5Sclose(dspace);
            H5Dclose(dsetid);
            H5Fclose(fileid);
            ostringstream eherr;
            eherr << "Cannot read the HDF5 dataset " << name()
                  << " with the type of the fixed size HDF5 string "<<endl;
            throw InternalErr (__FILE__, __LINE__, eherr.str ());
        }

        string total_string(strval.begin(),strval.end());
        strval.clear();//release some memory;

        // Need to trim the null parameters 
        size_t temp_pos;
        if (H5Tget_strpad(dtypeid) == H5T_STR_NULLTERM)
            temp_pos = total_string.find_first_of('\0');
        else if (H5Tget_strpad(dtypeid) == H5T_STR_SPACEPAD)
            temp_pos = total_string.find_last_not_of(' ')+1;
        else 
            temp_pos = total_string.find_last_not_of('0')+1;

        string trim_string = total_string.substr(0,temp_pos);

        // If the string size is longer than the current netCDF JAVA
        // string and the "EnableDropLongString" key is turned on,
        // No string is generated.
        if (true == is_droplongstr) {
            if( trim_string.size() > NC_JAVA_STR_SIZE_LIMIT) 
                trim_string = "";
        }
        set_value(trim_string);
    }
    else {

        H5Tclose(memtype);
        H5Tclose(dtypeid);
        H5Sclose(dspace);
        H5Dclose(dsetid);
        H5Fclose(fileid);

        throw InternalErr (__FILE__, __LINE__, "H5Tis_variable_str returns negative value" );
    } 

    return false;
}
