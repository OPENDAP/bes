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

/////////////////////////////////////////////////////////////////////////////
/// \file HDF5CF.cc
/// \brief Implementation of the basic mapping of HDF5 to DAP by following CF
///
///  It includes basic functions to 
///  1) retrieve HDF5 metadata info.
///  2) translate HDF5 objects into DAP DDS and DAS by following CF conventions.
///
///
/// \author Muqun Yang <myang6@hdfgroup.org>
///
/// Copyright (C) 2011-2013 The HDF Group
///
/// All rights reserved.

#include <sstream>
#include <algorithm>
#include <functional>
#include <climits>
#include "HDF5CF.h"


using namespace HDF5CF;


File::~File ()
{

    if (this->fileid >= 0)  {
        if (this->rootid >= 0) {
            for_each (this->groups.begin (), this->groups.end (),
                                   delete_elem ());
            for_each (this->vars.begin (), this->vars.end (),
                                   delete_elem ());
            for_each (this->root_attrs.begin (), this->root_attrs.end (),
                                   delete_elem ());
            H5Gclose(rootid);
        } 
        //H5Fclose(fileid);
    } 
}

Group::~Group() 
{
        for_each (this->attrs.begin (), this->attrs.end (),
                                   delete_elem ());
}

Var::~Var() 
{
        for_each (this->dims.begin (), this->dims.end (),
                                   delete_elem ());
        for_each (this->attrs.begin (), this->attrs.end (),
                                   delete_elem ());
}

Attribute::~Attribute() 
{
}

void
File::Retrieve_H5_Info(const char *path, hid_t file_id, bool include_attr)
throw(Exception) {

    // "h5","coming to Retrieve_H5_Info "<<endl;

    if (true == include_attr) {

        // Check if the BES key is set to check the ignored objects
        // We will only use DAS to output these information.
        string check_ignored_objects_key_str="H5.CheckIgnoreObj";
        this->check_ignored = HDF5CFDAPUtil::check_beskeys(check_ignored_objects_key_str);
        if(true == this->check_ignored) 
            this->add_ignored_info_page_header();

    }

    hid_t root_id;
    if ((root_id = H5Gopen(file_id,"/",H5P_DEFAULT))<0){
        throw1 ("Cannot open the HDF5 root group " );
    }
    this->rootid =root_id;
    try {
        this->Retrieve_H5_Obj(root_id,"/",include_attr);
    }
    catch(...) {
        throw;
    }

    if (true == include_attr) {

        // Find the file(root group) attribute

        // Obtain the object type, such as group or dataset. 
        H5O_info_t oinfo;
        int num_attrs = 0;

        if (H5Oget_info(root_id,&oinfo) < 0)
            throw1( "Error obtaining the info for the root group");

        num_attrs = oinfo.num_attrs;
        bool temp_unsup_attr_atype = false;

        for (int j = 0; j < num_attrs; j ++) {
            Attribute * attr = new Attribute();
            try {
                this->Retrieve_H5_Attr_Info(attr,root_id,j, temp_unsup_attr_atype);
            }
            catch(...) {
                delete attr;
                throw;

            }
            this->root_attrs.push_back(attr);
        }
        
        this->unsupported_attr_dtype = temp_unsup_attr_atype;
    }
}

void 
File::Retrieve_H5_Obj(hid_t grp_id,const char*gname, bool include_attr)
throw(Exception) {

    // Iterate through the file to see the members of the group from the root.
    H5G_info_t g_info;
    hsize_t nelems = 0;

    if (H5Gget_info(grp_id,&g_info) <0) 
        throw2 ("Counting hdf5 group elements error for ", gname);
    nelems = g_info.nlinks;

    ssize_t oname_size = 0;
    for (hsize_t i = 0; i < nelems; i++) {

        hid_t cgroup = -1;
        hid_t cdset  = -1;
        //char *oname = NULL;
        Group *group = NULL;
        Var   *var   = NULL;
        Attribute *attr = NULL;

        try {

            size_t dummy_name_len = 1;
            // Query the length of object name.
            oname_size =
                H5Lget_name_by_idx(grp_id,".",H5_INDEX_NAME,H5_ITER_NATIVE,i,NULL,
                dummy_name_len, H5P_DEFAULT);
            if (oname_size <= 0) 
                throw2("Error getting the size of the hdf5 object from the group: ", gname);

            // Obtain the name of the object 
            vector<char> oname;
            oname.resize((size_t)oname_size+1);
            //oname = new char[(size_t) oname_size + 1];
            

            if (H5Lget_name_by_idx(grp_id,".",H5_INDEX_NAME,H5_ITER_NATIVE,i,&oname[0],
                (size_t)(oname_size+1), H5P_DEFAULT) < 0)
                throw2("Error getting the hdf5 object name from the group: ",gname);

            // Check if it is the hard link or the soft link
            H5L_info_t linfo;
            if (H5Lget_info(grp_id,&oname[0],&linfo,H5P_DEFAULT)<0)
                throw2 ("HDF5 link name error from ", gname);

            // We ignore soft link and external links in this release 
            if(H5L_TYPE_SOFT == linfo.type  || H5L_TYPE_EXTERNAL == linfo.type){
                if(true == include_attr && true == check_ignored) {
                    this->add_ignored_info_links_header();
                    string full_path_name;
                    string temp_oname(oname.begin(),oname.end());
                    full_path_name = ((string(gname) != "/") 
                                ?(string(gname)+"/"+temp_oname.substr(0,temp_oname.size()-1)):("/"+temp_oname.substr(0,temp_oname.size()-1)));
                    this->add_ignored_info_links(full_path_name);

                }
                continue;
            }

            // Obtain the object type, such as group or dataset. 
            H5O_info_t oinfo;

            if (H5Oget_info_by_idx(grp_id, ".", H5_INDEX_NAME, H5_ITER_NATIVE,
                              i, &oinfo, H5P_DEFAULT)<0) 
                throw2( "Error obtaining the info for the object ",string(oname.begin(),oname.end()));
            
            H5O_type_t obj_type = oinfo.type;

            switch (obj_type) {

                case H5O_TYPE_GROUP: {

                    // Obtain the full path name
                    string full_path_name;
                    string temp_oname(oname.begin(),oname.end());
                    

                    full_path_name = ((string(gname) != "/") 
                                ?(string(gname)+"/"+temp_oname.substr(0,temp_oname.size()-1)):("/"+temp_oname.substr(0,temp_oname.size()-1)));

                    //  "h5","Group full_path_name " <<full_path_name <<endl;

                    cgroup = H5Gopen(grp_id, full_path_name.c_str(),H5P_DEFAULT);
                    if (cgroup < 0)
                        throw2( "Error opening the group ",full_path_name);

                    group = new Group();
                    group->path = full_path_name;
                    group->newname = full_path_name;


                    if (true == include_attr) {

                        int num_attrs = oinfo.num_attrs;
                        bool temp_unsup_attr_dtype = false;

                        for (int j = 0; j < num_attrs; j ++) {

                            attr = new Attribute();
                            Retrieve_H5_Attr_Info(attr,cgroup,j, temp_unsup_attr_dtype);
                            group->attrs.push_back(attr);
                            attr = NULL;
                       }

                       group->unsupported_attr_dtype = temp_unsup_attr_dtype;
                    }
                    this->groups.push_back(group);
                    Retrieve_H5_Obj(cgroup,full_path_name.c_str(),include_attr);
                    H5Gclose(cgroup);
                }

                    break;
                case H5O_TYPE_DATASET:{

                    // cerr<<"Coming to the dataset full_path " <<endl;
                    // Obtain the absolute path of the HDF5 dataset

                    //  string full_path_name = ((string(gname) != "/") 
                    //                ?(string(gname)+"/"+string(oname)):("/"+string(oname)));
                    // string full_path_name = string(gname) + "/" + string(oname);
                    //cerr<<"dataset full_path "<<full_path_name <<endl;
                    string temp_oname(oname.begin(),oname.end());
                    string full_path_name = ((string(gname) != "/") 
                                ?(string(gname)+"/"+temp_oname.substr(0,temp_oname.size()-1)):("/"+temp_oname.substr(0,temp_oname.size()-1)));


                    var = new Var();
                    var->name = temp_oname.substr(0,temp_oname.size()-1);
                    var->fullpath = full_path_name;
                    var->newname = full_path_name;

                    //cerr<<"variable path" <<var->fullpath <<endl;

                    cdset = H5Dopen(grp_id, full_path_name.c_str(),H5P_DEFAULT);
                    if (cdset < 0){
                        throw2( "Error opening the HDF5 dataset ",full_path_name);
                    }

                    bool temp_unsup_var_dtype = false;
                    Retrieve_H5_VarType(var,cdset,full_path_name,temp_unsup_var_dtype);

                    if (!this->unsupported_var_dtype && temp_unsup_var_dtype) 
                        this->unsupported_var_dtype = true; 

                    bool temp_unsup_var_dspace = false;

                    Retrieve_H5_VarDim(var,cdset,full_path_name,temp_unsup_var_dspace);

                    if (!this->unsupported_var_dspace && temp_unsup_var_dspace)
                        this->unsupported_var_dspace = true;

                    if (true == include_attr) {

                       int num_attrs = oinfo.num_attrs;
                       bool temp_unsup_attr_dtype = false;

                       for (int j = 0; j < num_attrs; j ++) {
                          
                            attr = new Attribute();

                            Retrieve_H5_Attr_Info(attr,cdset,j, temp_unsup_attr_dtype);
                            var->attrs.push_back(attr);
                            attr = NULL;
                       }

                       var->unsupported_attr_dtype = temp_unsup_attr_dtype;
                    }
 
                    this->vars.push_back(var);
                    H5Dclose(cdset);
                }
                    break;

                case H5O_TYPE_NAMED_DATATYPE:{
                    if(true == include_attr && true == check_ignored) {
                        this->add_ignored_info_namedtypes(string(gname),string(oname.begin(),oname.end()));
                    }
                }
                    // ignore the named datatype
                    break;
                default:
                    break;
            }
        } // try
        catch(...) {

            if (attr != NULL) {
                delete attr;
                attr = NULL;
            }

            if (var != NULL) {
                delete var;
                var = NULL;
            }

            if (group != NULL) {
                delete group;
                group = NULL;
            }

            if (cgroup !=-1) 
                H5Gclose(cgroup);

            if (cdset != -1) 
                H5Dclose(cdset);

            throw;

        } // catch
    } // for (hsize_t i = 0; i < nelems; i++)

}

void
File:: Retrieve_H5_VarType(Var *var, hid_t dset_id, const string & varname,bool &unsup_var_dtype)
throw(Exception){

    hid_t ty_id = -1;

    // Obtain the data type of the variable. 
    if ((ty_id = H5Dget_type(dset_id)) < 0) 
        throw2("unable to obtain hdf5 datatype for the dataset ",varname);

    // The following datatype class and datatype will not be supported for the CF option.
    //   H5T_TIME,  H5T_BITFIELD
    //   H5T_OPAQUE,  H5T_ENUM
    //   H5T_REFERENCE, H5T_COMPOUND
    //   H5T_VLEN,H5T_ARRAY
    //  64-bit integer

    // Note: H5T_REFERENCE H5T_COMPOUND and H5T_ARRAY can be mapped to DAP2 DDS for the default option.
    // H5T_COMPOUND, H5T_ARRAY can be mapped to DAP2 DAS for the default option.
    // 1-D variable length of string can also be mapped for the CF option..
    // The variable length string class is H5T_STRING rather than H5T_VLEN,
    // We also ignore the mapping of integer 64 bit since DAP2 doesn't
    // support 64-bit integer. In theory, DAP2 doesn't support long double
    // (128-bit or 92-bit floating point type).
    // 

    var->dtype = HDF5CFUtil::H5type_to_H5DAPtype(ty_id);
    if (false == HDF5CFUtil::cf_strict_support_type(var->dtype)) 
        unsup_var_dtype = true;

    H5Tclose(ty_id);
}

void 
File:: Retrieve_H5_VarDim(Var *var, hid_t dset_id, const string & varname, bool &unsup_var_dspace) 
throw(Exception){


    vector<hsize_t> dsize;
    vector<hsize_t> maxsize;

    hid_t dspace_id = -1;
    hid_t ty_id     = -1;
    
    try {
        if ((dspace_id = H5Dget_space(dset_id)) < 0) 
            throw2("Cannot get hdf5 dataspace id for the variable ",varname);    

        H5S_class_t space_class = H5S_NO_CLASS;
        if ((space_class = H5Sget_simple_extent_type(dspace_id)) < 0)
            throw2("Cannot obtain the HDF5 dataspace class for the variable ",varname);

        if (H5S_NULL == space_class)
            unsup_var_dspace = true; 
        else {
            // Note: currently we only support the string scalar space dataset. 
            // In the future, other atomic datatype should be supported. KY 2012-5-21
            if (H5S_SCALAR == space_class) {

                // Obtain the data type of the variable. 
                if ((ty_id = H5Dget_type(dset_id)) < 0) 
                    throw2("unable to obtain the hdf5 datatype for the dataset ",varname);

                if (H5T_STRING != H5Tget_class(ty_id)) 
                    unsup_var_dspace = true;               

                H5Tclose(ty_id);
            }

            if (false == unsup_var_dspace) {
                
                int ndims = H5Sget_simple_extent_ndims(dspace_id);
                if (ndims < 0) 
                    throw2("Cannot get the hdf5 dataspace number of dimension for the variable ",varname);

                var->rank = ndims;
                if (ndims !=0) {
                    //dsize =  new hsize_t[ndims];
                    //maxsize = new hsize_t[ndims];
                    dsize.resize(ndims);
                    maxsize.resize(ndims);
                }

                // DAP applications don't care about the unlimited dimensions 
                // since the applications only care about retrieving the data.
                // So we don't check the maxsize to see if it is the unlimited dimension 
                // variable.
                if (H5Sget_simple_extent_dims(dspace_id, &dsize[0], &maxsize[0])<0) 
                    throw2("Cannot obtain the dim. info for the variable ", varname);

                // dsize can be 0. Currently DAP2 doesn't support this. So ignore now. KY 2012-5-21
                for (int i = 0; i < ndims; i++) {
                    if (0 == dsize[i]) {
                        unsup_var_dspace = true;
                        break;
                    }
                }

                if (false == unsup_var_dspace) {
                    for (int i=0; i<ndims; i++) {
                        Dimension * dim = new Dimension(dsize[i]); 
                        var->dims.push_back(dim);
                    }
                }
            }
        }
        
        var->unsupported_dspace = unsup_var_dspace;

        H5Sclose(dspace_id);
    }

    catch  (...) {

        if (dspace_id != -1) 
            H5Sclose(dspace_id);

        if (ty_id != -1)
            H5Tclose(ty_id);

        throw;
        //throw2("Cannot obtain the dimension information for the dataset ",varname);
    }

}


void 
File:: Retrieve_H5_Attr_Info(Attribute * attr, hid_t obj_id,const int j, bool &unsup_attr_dtype) 
throw(Exception) 

{

    hid_t attrid = -1;
    hid_t ty_id = -1;
    hid_t aspace_id = -1;
    hid_t memtype = -1;


    try {

        // Obtain the attribute ID.
        if ((attrid = H5Aopen_by_idx(obj_id, ".", H5_INDEX_CRT_ORDER, H5_ITER_INC,(hsize_t)j, H5P_DEFAULT, H5P_DEFAULT)) < 0) 
            throw1("Unable to open attribute by index " );


        // Obtain the size of attribute name.
        ssize_t name_size =  H5Aget_name(attrid, 0, NULL);
        if (name_size < 0) 
            throw1("Unable to obtain the size of the hdf5 attribute name  " );

        //attr_name = new char[name_size+1];
        string attr_name;
        attr_name.resize(name_size+1);

        // Obtain the attribute name.    
        if ((H5Aget_name(attrid, name_size+1, &attr_name[0])) < 0) 
            throw1("unable to obtain the hdf5 attribute name  ");

        // Obtain the type of the attribute. 
        if ((ty_id = H5Aget_type(attrid)) < 0) 
            throw2("unable to obtain hdf5 datatype for the attribute ",attr_name);

        // The following datatype class and datatype will not be supported for the CF option.
        //   H5T_TIME,  H5T_BITFIELD
        //   H5T_OPAQUE,  H5T_ENUM
        //   H5T_REFERENCE, H5T_COMPOUND
        //   H5T_VLEN,H5T_ARRAY
        //  64-bit integer

        // Note: H5T_REFERENCE H5T_COMPOUND and H5T_ARRAY can be mapped to DAP2 DDS for the default option.
        // H5T_COMPOUND, H5T_ARRAY can be mapped to DAP2 DAS for the default option.
        // 1-D variable length of string can also be mapped for the CF option..
        // The variable length string class is H5T_STRING rather than H5T_VLEN,
        // We also ignore the mapping of integer 64 bit since DAP2 doesn't
        // support 64-bit integer. In theory, DAP2 doesn't support long double
        // (128-bit or 92-bit floating point type).
        // 
        attr->dtype = HDF5CFUtil::H5type_to_H5DAPtype(ty_id);
        if (false == HDF5CFUtil::cf_strict_support_type(attr->dtype)) 
            unsup_attr_dtype = true;

        if ((aspace_id = H5Aget_space(attrid)) < 0) 
            throw2("Cannot get hdf5 dataspace id for the attribute ",attr_name);

        int ndims = H5Sget_simple_extent_ndims(aspace_id);
        if (ndims < 0) 
            throw2("Cannot get the hdf5 dataspace number of dimension for attribute ",attr_name);

        hsize_t nelmts = 1;
        // if it is a scalar attribute, just define number of elements to be 1.
        if (ndims != 0) {

            vector<hsize_t> asize;
            vector<hsize_t> maxsize;
            asize.resize(ndims);
            maxsize.resize(ndims);

            // DAP applications don't care about the unlimited dimensions 
            // since the applications only care about retrieving the data.
            // So we don't check the maxsize to see if it is the unlimited dimension 
            // attribute.
            if (H5Sget_simple_extent_dims(aspace_id, &asize[0], &maxsize[0])<0) 
                throw2("Cannot obtain the dim. info for the attribute ", attr_name);

            // Return ndims and size[ndims]. 
            for (int j = 0; j < ndims; j++)
                nelmts *= asize[j];
        } // if(ndims != 0)

        size_t ty_size = H5Tget_size(ty_id);
        if (0 == ty_size ) 
            throw2("Cannot obtain the dtype size for the attribute ",attr_name);


        memtype = H5Tget_native_type(ty_id, H5T_DIR_ASCEND);
        if (memtype < 0) 
            throw2("Cannot obtain the memory datatype for the attribute ",attr_name);

        //attr->attr_mtypeid = memtype;
        // Store the name and the count
        string temp_aname(attr_name.begin(),attr_name.end());
        attr->name = temp_aname.substr(0,temp_aname.size()-1);
        attr->newname = attr->name;
        attr->count = nelmts;


        // Release HDF5 resources.
        H5Tclose(ty_id);
        H5Tclose(memtype);
        H5Sclose(aspace_id);
        H5Aclose(attrid);


    } // try
    catch(...) {

        if(ty_id != -1) 
            H5Tclose(ty_id);

        if(memtype != -1)
            H5Tclose(memtype);

        if(aspace_id != -1)
            H5Sclose(aspace_id);

        if(attrid != -1)
            H5Aclose(attrid);

        throw;
    }

}

void 
File::Retrieve_H5_Supported_Attr_Values() throw(Exception) 
{

    for (vector<Attribute *>::iterator ira = this->root_attrs.begin();
                ira != this->root_attrs.end(); ++ira) 
          Retrieve_H5_Attr_Value(*ira,"/");

    for (vector<Group *>::iterator irg = this->groups.begin();
                irg != this->groups.end(); ++irg) {
        for (vector<Attribute *>::iterator ira = (*irg)->attrs.begin();
             ira != (*irg)->attrs.end(); ++ira) {
            Retrieve_H5_Attr_Value(*ira,(*irg)->path);
        }
    }

    for (vector<Var *>::iterator irv = this->vars.begin();
                irv != this->vars.end(); ++irv) {
        for (vector<Attribute *>::iterator ira = (*irv)->attrs.begin();
             ira != (*irv)->attrs.end(); ++ira) {
            Retrieve_H5_Attr_Value(*ira,(*irv)->fullpath);
        }
    }
}


void 
File::Retrieve_H5_Attr_Value( Attribute *attr,string obj_name) 
throw(Exception) 
{

    // Define HDF5 object Ids.
    hid_t obj_id = -1;
    hid_t attr_id = -1;
    hid_t ty_id  = -1;
    hid_t memtype_id = -1;
    hid_t aspace_id = -1;


    try {

        // Open the object that hold this attribute
        obj_id = H5Oopen(this->fileid,obj_name.c_str(),H5P_DEFAULT);
        if (obj_id < 0) 
            throw2("Cannot open the object ",obj_name);

        attr_id = H5Aopen(obj_id,(attr->name).c_str(),H5P_DEFAULT);
        if (attr_id <0 ) 
            throw4("Cannot open the attribute ",attr->name," of object ",obj_name);

        ty_id   = H5Aget_type(attr_id);
        if (ty_id <0) 
            throw4("Cannot obtain the datatype of  the attribute ",attr->name," of object ",obj_name);

        memtype_id = H5Tget_native_type(ty_id, H5T_DIR_ASCEND);

        if (memtype_id < 0)
            throw2("Cannot obtain the memory datatype for the attribute ",attr->name);


        size_t ty_size = H5Tget_size(memtype_id);
        if (0 == ty_size )
            throw4("Cannot obtain the dtype size for the attribute ",attr->name, " of object ",obj_name);

        size_t total_bytes = attr->count * ty_size;

        // We have to handle variable length string differently. 
        if (H5VSTRING == attr->dtype) {

            // Variable length string attribute values only store pointers of the actual string value.
            vector<char> temp_buf;
            temp_buf.resize(total_bytes);

            if (H5Aread(attr_id, memtype_id, &temp_buf[0]) < 0) 
                throw4("Cannot obtain the value of the attribute ",attr->name, " of object ",obj_name);

            char *temp_bp = NULL;
            temp_bp = &temp_buf[0];
            char* onestring = NULL;
	    string total_vstring ="";

            attr->strsize.resize(attr->count);

            for (unsigned int temp_i = 0; temp_i <attr->count; temp_i++) {

                // This line will assure that we get the real variable length string value.
                onestring =*(char **)temp_bp;
                if(onestring!= NULL) {
                    total_vstring +=string(onestring);
                    attr->strsize[temp_i] = (string(onestring)).size();
                }
                else attr->strsize[temp_i] = 0;

                // going to the next value.
                temp_bp +=ty_size;
            }
            if (&temp_buf[0] != NULL) {

                aspace_id = H5Aget_space(attr_id);
                if (aspace_id < 0) 
                    throw4("Cannot obtain space id for ",attr->name, " of object ",obj_name);

                // Reclaim any VL memory if necessary.
                if (H5Dvlen_reclaim(memtype_id,aspace_id,H5P_DEFAULT,&temp_buf[0]) < 0) 
                    throw4("Cannot reclaim VL memory for ",attr->name, " of object ",obj_name);

                H5Sclose(aspace_id);
            }

            if (HDF5CFUtil::H5type_to_H5DAPtype(ty_id) !=H5VSTRING)  
                throw4("Error to obtain the VL string type for attribute ",attr->name, " of object ",obj_name);

            attr->value.resize(total_vstring.size());

            copy(total_vstring.begin(),total_vstring.end(),attr->value.begin());

        }
        else {

            if (attr->dtype == H5FSTRING) { 
                attr->fstrsize = ty_size;
            }
            
            attr->value.resize(total_bytes);

            // Read HDF5 attribute data.
            if (H5Aread(attr_id, memtype_id, (void *) &attr->value[0]) < 0) 
                throw4("Cannot obtain the dtype size for the attribute ",attr->name, " of object ",obj_name);

            if (attr->dtype == H5FSTRING) {

                size_t sect_size = ty_size;
                int num_sect = (total_bytes%sect_size==0)?(total_bytes/sect_size)
                                                     :(total_bytes/sect_size+1);
                vector<size_t>sect_newsize;
                sect_newsize.resize(num_sect);

                string total_fstring = string(attr->value.begin(),attr->value.end());
                
                string new_total_fstring = HDF5CFUtil::trim_string(memtype_id,total_fstring,
                                           num_sect,sect_size,sect_newsize);
                // "h5","The first new sect size is "<<sect_newsize[0] <<endl; 
                attr->value.resize(new_total_fstring.size());
                copy(new_total_fstring.begin(),new_total_fstring.end(),attr->value.begin()); 
                attr->strsize.resize(num_sect);
                for (int temp_i = 0; temp_i <num_sect; temp_i ++) 
                    attr->strsize[temp_i] = sect_newsize[temp_i];

                // "h5","new string value " <<string(attr->value.begin(), attr->value.end()) <<endl;
#if 0
for (int temp_i = 0; temp_i <num_sect; temp_i ++)
     "h5","string new section size = " << attr->strsize[temp_i] <<endl;
#endif
            }
        }

        H5Tclose(memtype_id);
        H5Tclose(ty_id);
        H5Aclose(attr_id); 
        H5Oclose(obj_id);

   }

    catch(...) {

        if (memtype_id !=-1)
            H5Tclose(memtype_id);

        if (ty_id != -1)
            H5Tclose(ty_id);

        if (aspace_id != -1)
            H5Sclose(aspace_id);

        if (attr_id != -1)
            H5Aclose(attr_id);

        if (obj_id != -1)
            H5Oclose(obj_id);


        //throw1("Error in method File::Retrieve_H5_Attr_Value");
        throw;
    }

}
 
void File::Handle_Unsupported_Dtype(bool include_attr) throw(Exception) {

    if (true == include_attr) {
        Handle_Group_Unsupported_Dtype();
        Handle_VarAttr_Unsupported_Dtype();
    }

    Handle_Var_Unsupported_Dtype();

#if 0
    // First the root attributes
    if (true == include_attr) {
        if  (false == this->root_attrs.empty()) {
            if (true == this->unsupported_attr_dtype) {
                for (vector<Attribute *>::iterator ira = this->root_attrs.begin();
                    ira != this->root_attrs.end(); ++ira) {
                    H5DataType temp_dtype = (*ira)->getType();
                    if (false == HDF5CFUtil::cf_strict_support_type(temp_dtype)) {
                        delete (*ira);
		        this->root_attrs.erase(ira);
		        ira--;
		    }
                }
            }
        }

        // Then the group attributes
        if (false == this->groups.empty()) {
            for (vector<Group *>::iterator irg = this->groups.begin();
                    irg != this->groups.end(); ++irg) {
                if (false == (*irg)->attrs.empty()) {
                    if (true == (*irg)->unsupported_attr_dtype) {
                        for (vector<Attribute *>::iterator ira = (*irg)->attrs.begin();
                            ira != (*irg)->attrs.end(); ++ira) {
                            H5DataType temp_dtype = (*ira)->getType();
                            if (false == HDF5CFUtil::cf_strict_support_type(temp_dtype)) {
                                delete (*ira);
                                (*irg)->attrs.erase(ira);
                                ira--;
                            }
                        }
                    }
                }
            }
        }
    }

    // Then the variable(HDF5 dataset) and the correponding attributes.
    if (false == this->vars.empty()) {
        if (true == include_attr) {
            for (vector<Var *>::iterator irv = this->vars.begin();
                 irv != this->vars.end(); ++irv) {
                if (false == (*irv)->attrs.empty()) {
                    if (true == (*irv)->unsupported_attr_dtype) {
                        for (vector<Attribute *>::iterator ira = (*irv)->attrs.begin();
                            ira != (*irv)->attrs.end(); ++ira) {
                            H5DataType temp_dtype = (*ira)->getType();
                            if (false == HDF5CFUtil::cf_strict_support_type(temp_dtype)) {
                                delete (*ira);
                                (*irv)->attrs.erase(ira);
                                ira--;
                            }
                        }
                    }
                }
           }
       }
       if (true == this->unsupported_var_dtype) {
           // "h5","having unsupported variable datatype" <<endl;
            for (vector<Var *>::iterator irv = this->vars.begin();
                irv != this->vars.end(); ++irv) {
                H5DataType temp_dtype = (*irv)->getType();
                if (false == HDF5CFUtil::cf_strict_support_type(temp_dtype)) {
                    delete (*irv);
                    this->vars.erase(irv);
                    irv--;
                }
            }
        }
    }
#endif
}

void File::Handle_Group_Unsupported_Dtype() throw(Exception) {

        // First root
        if  (false == this->root_attrs.empty()) {
            if (true == this->unsupported_attr_dtype) {
                for (vector<Attribute *>::iterator ira = this->root_attrs.begin();
                    ira != this->root_attrs.end(); ++ira) {
                    H5DataType temp_dtype = (*ira)->getType();
                    if (false == HDF5CFUtil::cf_strict_support_type(temp_dtype)) {
                        delete (*ira);
		        this->root_attrs.erase(ira);
		        ira--;
		    }
                }
            }
        }

        // Then the group attributes
        if (false == this->groups.empty()) {
            for (vector<Group *>::iterator irg = this->groups.begin();
                    irg != this->groups.end(); ++irg) {
                if (false == (*irg)->attrs.empty()) {
                    if (true == (*irg)->unsupported_attr_dtype) {
                        for (vector<Attribute *>::iterator ira = (*irg)->attrs.begin();
                            ira != (*irg)->attrs.end(); ++ira) {
                            H5DataType temp_dtype = (*ira)->getType();
                            if (false == HDF5CFUtil::cf_strict_support_type(temp_dtype)) {
                                delete (*ira);
                                (*irg)->attrs.erase(ira);
                                ira--;
                            }
                        }
                    }
                }
            }
        }
 
}

void File:: Gen_Group_Unsupported_Dtype_Info() throw(Exception) {

        // First root
        if  (false == this->root_attrs.empty()) {
            if (true == this->unsupported_attr_dtype) {
                for (vector<Attribute *>::iterator ira = this->root_attrs.begin();
                    ira != this->root_attrs.end(); ++ira) {
                    H5DataType temp_dtype = (*ira)->getType();
                    if (false == HDF5CFUtil::cf_strict_support_type(temp_dtype)) {
                        this->add_ignored_info_attrs(true,"/",(*ira)->name);
		    }
                }
            }
        }

        // Then the group attributes
        if (false == this->groups.empty()) {
            for (vector<Group *>::iterator irg = this->groups.begin();
                    irg != this->groups.end(); ++irg) {
                if (false == (*irg)->attrs.empty()) {
                    if (true == (*irg)->unsupported_attr_dtype) {
                        for (vector<Attribute *>::iterator ira = (*irg)->attrs.begin();
                            ira != (*irg)->attrs.end(); ++ira) {
                            H5DataType temp_dtype = (*ira)->getType();
                            if (false == HDF5CFUtil::cf_strict_support_type(temp_dtype)) {
                                this->add_ignored_info_attrs(true,(*irg)->path,(*ira)->name);
                            }
                        }
                    }
                }
            }
        }
 
}

void File:: Handle_Var_Unsupported_Dtype() throw(Exception) {
    if (false == this->vars.empty()) {
       if (true == this->unsupported_var_dtype) {
           // "h5","having unsupported variable datatype" <<endl;
            for (vector<Var *>::iterator irv = this->vars.begin();
                irv != this->vars.end(); ++irv) {
                H5DataType temp_dtype = (*irv)->getType();
                if (false == HDF5CFUtil::cf_strict_support_type(temp_dtype)) {
                    delete (*irv);
                    this->vars.erase(irv);
                    irv--;
                }
            }
        }
    }

}


void File:: Gen_Var_Unsupported_Dtype_Info() throw(Exception) {

    if (false == this->vars.empty()) {
       if (true == this->unsupported_var_dtype) {
           // "h5","having unsupported variable datatype" <<endl;
            for (vector<Var *>::iterator irv = this->vars.begin();
                irv != this->vars.end(); ++irv) {
                H5DataType temp_dtype = (*irv)->getType();
                if (false == HDF5CFUtil::cf_strict_support_type(temp_dtype)) {
                    this->add_ignored_info_objs(false,(*irv)->fullpath);
                }
            }
        }
    }

}

// The variable(HDF5 dataset) and the correponding attributes.
void File::Handle_VarAttr_Unsupported_Dtype() throw(Exception) {
    if (false == this->vars.empty()) {
            for (vector<Var *>::iterator irv = this->vars.begin();
                 irv != this->vars.end(); ++irv) {
                if (false == (*irv)->attrs.empty()) {
                    if (true == (*irv)->unsupported_attr_dtype) {
                        for (vector<Attribute *>::iterator ira = (*irv)->attrs.begin();
                            ira != (*irv)->attrs.end(); ++ira) {
                            H5DataType temp_dtype = (*ira)->getType();
                            if (false == HDF5CFUtil::cf_strict_support_type(temp_dtype)) {
                                delete (*ira);
                                (*irv)->attrs.erase(ira);
                                ira--;
                            }
                        }
                    }
                }
           }
       
    }
 

}

void File:: Gen_VarAttr_Unsupported_Dtype_Info() throw(Exception) {

    if (false == this->vars.empty()) {
            for (vector<Var *>::iterator irv = this->vars.begin();
                 irv != this->vars.end(); ++irv) {
                if (false == (*irv)->attrs.empty()) {
                    if (true == (*irv)->unsupported_attr_dtype) {
                        for (vector<Attribute *>::iterator ira = (*irv)->attrs.begin();
                            ira != (*irv)->attrs.end(); ++ira) {
                            H5DataType temp_dtype = (*ira)->getType();
                            if (false == HDF5CFUtil::cf_strict_support_type(temp_dtype)) {
                                this->add_ignored_info_attrs(false,(*irv)->fullpath,(*ira)->name);
                            }
                        }
                    }
                }
           }
       
    }
 
}

void File:: Gen_DimScale_VarAttr_Unsupported_Dtype_Info() throw(Exception) {

    for (vector<Var *>::iterator irv = this->vars.begin();
                 irv != this->vars.end(); ++irv) {
                // If the attribute REFERENCE_LIST comes with the attribut CLASS, the
                // attribute REFERENCE_LIST is okay to ignore. No need to report.
                bool is_ignored = ignored_dimscale_ref_list((*irv));
                if (false == (*irv)->attrs.empty()) {
                    if (true == (*irv)->unsupported_attr_dtype) {
                        for (vector<Attribute *>::iterator ira = (*irv)->attrs.begin();
                            ira != (*irv)->attrs.end(); ++ira) {
                            H5DataType temp_dtype = (*ira)->getType();
                            if (false == HDF5CFUtil::cf_strict_support_type(temp_dtype)) {
                                // "DIMENSION_LIST" is okay to ignore and "REFERENCE_LIST"
                                // is okay to ignore if the variable has another attribute
                                // CLASS="DIMENSION_SCALE"
                                if (("DIMENSION_LIST" !=(*ira)->name) &&
                                    (("REFERENCE_LIST" != (*ira)->name || true == is_ignored)))
                                    this->add_ignored_info_attrs(false,(*irv)->fullpath,(*ira)->name);
                            }
                        }
                    }
                }
            }


}
void File::Handle_Unsupported_Dspace() throw(Exception) {

    // The unsupported data space 
    if (false == this->vars.empty()) {
        if (true == this->unsupported_var_dspace) {
            for (vector<Var *>::iterator irv = this->vars.begin();
                 irv != this->vars.end(); ++irv) {
                if (true  == (*irv)->unsupported_dspace) {
                    delete (*irv);
                    this->vars.erase(irv);
                    irv--;
                }
            }
        }
    }
}


void File:: Gen_Unsupported_Dspace_Info() throw(Exception) {

    if (false == this->vars.empty()) {
        if (true == this->unsupported_var_dspace) {
            for (vector<Var *>::iterator irv = this->vars.begin();
                 irv != this->vars.end(); ++irv) {
                if (true  == (*irv)->unsupported_dspace) {
                    this->add_ignored_info_objs(true,(*irv)->fullpath);
                }
            }
        }
    }
 
}
void File:: Handle_Unsupported_Others(bool include_attr) throw(Exception) {

    if(true == this->check_ignored && true == include_attr) {

        // Check the drop long string feature.
        string check_droplongstr_key ="H5.EnableDropLongString";
        bool is_droplongstr = false;
        is_droplongstr = HDF5CFDAPUtil::check_beskeys(check_droplongstr_key);
        if(true == is_droplongstr){

            for (vector<Attribute *>::iterator ira = this->root_attrs.begin();
                ira != this->root_attrs.end(); ++ira) {
                if(H5FSTRING == (*ira)->dtype || H5VSTRING == (*ira)->dtype) {
                    if((*ira)->getBufSize() > NC_JAVA_STR_SIZE_LIMIT) {
                        this->add_ignored_droplongstr_hdr();
                        this->add_ignored_grp_longstr_info("/",(*ira)->name);
                    }
                }
            }

            for (vector<Group *>::iterator irg = this->groups.begin();
                        irg != this->groups.end(); ++irg) {
                for (vector<Attribute *>::iterator ira = (*irg)->attrs.begin();
                     ira != (*irg)->attrs.end(); ++ira) {
                    if(H5FSTRING == (*ira)->dtype || H5VSTRING == (*ira)->dtype) {
                       if((*ira)->getBufSize() > NC_JAVA_STR_SIZE_LIMIT) {
                           this->add_ignored_droplongstr_hdr();
                           this->add_ignored_grp_longstr_info((*irg)->path,(*ira)->name);
                       }
                   }
 
                }
            }
            for (vector<Var *>::iterator irv = this->vars.begin();
                 irv != this->vars.end(); ++irv) {
                if(true == Check_DropLongStr((*irv),NULL)) {
                    this->add_ignored_droplongstr_hdr();
                    this->add_ignored_var_longstr_info((*irv),NULL);
                }
                for(vector<Attribute *>::iterator ira = (*irv)->attrs.begin();
                    ira != (*irv)->attrs.end();++ira) {
                    if(true == Check_DropLongStr((*irv),(*ira))) {
                        this->add_ignored_droplongstr_hdr();
                        this->add_ignored_var_longstr_info((*irv),(*ira));
                    }
                }
            }
        }
    }

}

void File::Flatten_Obj_Name(bool include_attr) throw(Exception) {

    for (vector<Var *>::iterator irv = this->vars.begin();
         irv != this->vars.end(); ++irv) {
        (*irv)->newname = get_CF_string((*irv)->newname);

//cerr<<"CF variable new name "<< (*irv)->newname <<endl;

        for (vector<Dimension *>::iterator ird = (*irv)->dims.begin();
             ird != (*irv)->dims.end(); ++ird) {
            (*ird)->newname = get_CF_string((*ird)->newname);
        }
    }

    if (true == include_attr) {

        for (vector<Attribute *>::iterator ira = this->root_attrs.begin();
             ira != this->root_attrs.end(); ++ira){ 
            (*ira)->newname = get_CF_string((*ira)->newname);
        }

        for (vector<Group *>::iterator irg = this->groups.begin();
              irg != this->groups.end(); ++irg) {
            (*irg)->newname = get_CF_string((*irg)->newname);
            for (vector<Attribute *>::iterator ira = (*irg)->attrs.begin();
                        ira != (*irg)->attrs.end(); ++ira) {
                (*ira)->newname = get_CF_string((*ira)->newname);
            }
        }

        for (vector<Var *>::iterator irv = this->vars.begin();
            irv != this->vars.end(); ++irv) {
            for (vector<Attribute *>::iterator ira = (*irv)->attrs.begin();
                  ira != (*irv)->attrs.end(); ++ira) {
                (*ira)->newname = get_CF_string((*ira)->newname);
            }
        }
    } // if (true == include_attr)
}

void File::Handle_Var_NameClashing(set<string>&objnameset ) throw(Exception) {

    Handle_General_NameClashing(objnameset,this->vars);
}

void File::Handle_Group_NameClashing(set<string> &objnameset ) throw(Exception) {


    pair<set<string>::iterator,bool> setret;


    // Now for DAS, we need to handle name clashings for
    // DAS tables. Namely we need to make sure the global attribute
    // table(HDF5_GLOBAL) and the attribute tables mapped from 
    // HDF5 groups will not have name clashings with the variable name
    // lists. If having the name clashings, the global attribute table and the
    // the attribute tables generated from the groups will be changed.
    // The file attribute name clashing

    setret = objnameset.insert(FILE_ATTR_TABLE_NAME);
    if (false == setret.second) {

        int clash_index = 1;
        string fa_clash_name = FILE_ATTR_TABLE_NAME;
        HDF5CFUtil::gen_unique_name(fa_clash_name,objnameset,clash_index);
        FILE_ATTR_TABLE_NAME = fa_clash_name;
    }

    // The group attribute name clashing
    Handle_General_NameClashing(objnameset,this->groups);

}

void File::Handle_Obj_AttrNameClashing() throw(Exception){

     // Now handling the possible name clashings for attributes
    // For attribute clashings, we only need to resolve the name clashings 
    // for attributes within each variable, file attributes and attributes
    // within each group. The name clashings for attributes should be very rare.
    // Potentially the checking and the correcting may be  costly.
    // This is another reason for special products, we may not even need to check
    // the name clashings. KY 2011-12-24

    set<string>objnameset;

    // For root attributes   
    Handle_General_NameClashing(objnameset,this->root_attrs);

    // For group attributes
    for (vector<Group *>::iterator irg = this->groups.begin();
        irg != this->groups.end(); ++irg) {
        objnameset.clear();
        Handle_General_NameClashing(objnameset,(*irg)->attrs);
    }

    // For variable attributes
    for (vector<Var *>::iterator irv = this->vars.begin();
        irv != this->vars.end(); ++irv) { 
        objnameset.clear();
        Handle_General_NameClashing(objnameset,(*irv)->attrs);
    }
}

//class T must have member string newname
template<class T> void
File::Handle_General_NameClashing(set <string>&objnameset, vector<T*>& objvec) throw(Exception){


//    set<string> objnameset;
    pair<set<string>::iterator,bool> setret;
    set<string>::iterator iss;

    vector<string> clashnamelist;
    vector<string>::iterator ivs;

    map<int,int> cl_to_ol;
    int ol_index = 0;
    int cl_index = 0;

    /*class*/
    typename vector<T*>::iterator irv;

	for (irv = objvec.begin(); irv != objvec.end(); ++irv) {
		setret = objnameset.insert((*irv)->newname);
		if (false == setret.second) {
			clashnamelist.insert(clashnamelist.end(), (*irv)->newname);
			cl_to_ol[cl_index] = ol_index;
			cl_index++;
		}
		ol_index++;
	}

    // Now change the clashed elements to unique elements; 
    // Generate the set which has the same size as the original vector.
    for (ivs=clashnamelist.begin(); ivs!=clashnamelist.end(); ivs++) {
        int clash_index = 1;
        string temp_clashname = *ivs +'_';
        HDF5CFUtil::gen_unique_name(temp_clashname,objnameset,clash_index);
        *ivs = temp_clashname;
    }

    // Now go back to the original vector, make it unique.
    for (unsigned int i =0; i <clashnamelist.size(); i++)
        objvec[cl_to_ol[i]]->newname = clashnamelist[i];

}




void File::Handle_GeneralObj_NameClashing(bool include_attr,set<string>& objnameset ) throw(Exception) {

    Handle_Var_NameClashing(objnameset);
    if (true == include_attr) {
        Handle_Group_NameClashing(objnameset );
        Handle_Obj_AttrNameClashing();
    }
}

string
File::get_CF_string(string s)
{

    if(""==s) return s;
    string insertString(1,'_');

    // Always start with _ if the first character is not a letter
    if (true == isdigit(s[0]))
        s.insert(0,insertString);

    for(unsigned int i=0; i < s.length(); i++)
        if((false == isalnum(s[i])) &&  (s[i]!='_'))
            s[i]='_';

    return s;

}
void
File:: Insert_One_NameSizeMap_Element( string name,hsize_t size) throw(Exception) 
{
    pair<map<string,hsize_t>::iterator,bool>mapret;
    mapret = dimname_to_dimsize.insert(pair<string,hsize_t>(name,size));
    if (false == mapret.second) 
        throw4("The dimension name ",name," should map to ",size);

}

void
File:: Insert_One_NameSizeMap_Element2(map<string,hsize_t>& name_to_size, string name,hsize_t size) throw(Exception) 
{
    pair<map<string,hsize_t>::iterator,bool>mapret;
    mapret = name_to_size.insert(pair<string,hsize_t>(name,size));
    if (false == mapret.second) 
        throw4("The dimension name ",name," should map to ",size);

}

// For dimension names added by the handlers, by default,
// Each dimension will have a unique dimension name. For example,
// Int foo[100][200] will be Int foo[Fakedim1][Fakedim2]
// If you have many variables, the dimension names may be too many.
// To reduce numbers, we ASSUME that the dimension having the same
// size shares the same dimension. In this way, the number of dimension names
// will be reduced.
// For example, Int foo2[100][300] will be Int foo2[Fakedim1][Fakedim3]
// instead of foo2[Fakedim3][Fakedim4]. However, that may impose
// another problem. Suppose Int Foosame[100][100] becomes
// Int Foosame[FakeDim1][FakeDim1]. This doesn't make sense for some
// applications. The fuction Adjust_Duplicate_FakeDim_Name will make sure 
// this case will not happen. 
void
File:: Add_One_FakeDim_Name(Dimension *dim) throw(Exception){

    stringstream sfakedimindex;
    string fakedimstr = "FakeDim";
    pair<set<string>::iterator,bool> setret;
    map<hsize_t,string>::iterator im;
    pair<map<hsize_t,string>::iterator,bool>mapret;
    
    sfakedimindex << addeddimindex;
    string added_dimname = fakedimstr + sfakedimindex.str();

    // Build up the size to fakedim map.
    mapret = dimsize_to_fakedimname.insert(pair<hsize_t,string>(dim->size,added_dimname));
    if (false == mapret.second) { //The dim size exists, use the corresponding name.
        dim->name = dimsize_to_fakedimname[dim->size];
        dim->newname = dim->name;
    }
    else { // Insert this (dimsize,dimname) pair to dimsize_to_fakedimname map successfully.
        //First make sure this new dim name doesn't have name clashing
        // with previous dim names, after the checking, inserting to the
        // dimname list set.
        // dimnamelist is a private memeber of File.
        setret = dimnamelist.insert(added_dimname);
        if (false == setret.second) {
            int clash_index =1;
            string temp_clashname=added_dimname+'_';
            HDF5CFUtil::gen_unique_name(temp_clashname,dimnamelist,clash_index);
            dim->name = temp_clashname;
            dim->newname = dim->name;
            setret = dimnamelist.insert(dim->name);
            if(false == setret.second) 
                throw2("Fail to insert the unique dimsizede name ", dim->name);

           // We have to adjust the dim. name of the dimsize_to_fakedimname map, since the
           // dimname has been updated for this size.
           dimsize_to_fakedimname.erase(dim->size);
	   mapret = dimsize_to_fakedimname.insert(pair<hsize_t,string>(dim->size,dim->name));
           if (false == mapret.second) 
                throw4("The dimension size ",dim->size," should map to ",dim->name);
        }// if(false == setret.second)

        // New dim name is inserted successfully, update the dimname_to_dimsize map.
        dim->name = added_dimname;
        dim->newname = dim->name;
        Insert_One_NameSizeMap_Element(dim->name,dim->size);

        // Increase the dimindex since the new dimname has been inserted.
        addeddimindex++;
    }// else
}

    
void
File:: Adjust_Duplicate_FakeDim_Name(Dimension * dim) throw(Exception){

    // No need to adjust the dimsize_to_fakedimname map, only create a new Fakedim
    // The simplest way is to increase the dim index and resolve any name clashings with other dim names.
    // Note: No need to update the dimsize_to_dimname map since the original "FakeDim??" of this size
    // can be used as a dimension name of other variables. But we need to update the dimname_to_dimsize map
    // since this is a new dim name.
    stringstream sfakedimindex;
    pair<set<string>::iterator,bool> setret;
    
    addeddimindex++;
    sfakedimindex << addeddimindex;
    string added_dimname = "FakeDim" + sfakedimindex.str();
    setret = dimnamelist.insert(added_dimname);
    if (false == setret.second) {
        int clash_index =1;
        string temp_clashname=added_dimname+'_';
        HDF5CFUtil::gen_unique_name(temp_clashname,dimnamelist,clash_index);
        dim->name = temp_clashname;
        dim->newname = dim->name;
        setret = dimnamelist.insert(dim->name);
        if(false == setret.second) 
            throw2("Fail to insert the unique dimsizede name ", dim->name);
    }
    dim->name = added_dimname;
    dim->newname = dim->name;
    Insert_One_NameSizeMap_Element(dim->name,dim->size);

    // Need to prepare for the next unique FakeDim. 
    addeddimindex++;
}

void File::Replace_Dim_Name_All(const string orig_dim_name, const string new_dim_name) throw(Exception) {

    // The newname of the original dimension should also be replaced by new_dim_name
    for (vector<Var *>::iterator irv = this->vars.begin();
        irv != this->vars.end(); ++irv) {
        for (vector<Dimension *>::iterator ird= (*irv)->dims.begin();
            ird != (*irv)->dims.end(); ++ird) {
            if((*ird)->name == orig_dim_name) {
                (*ird)->name = new_dim_name;
                (*ird)->newname = new_dim_name;
            }

        }
    }
}

#if 0
void File::Use_Dim_Name_With_Size_All(const string dim_name, const size_t dim_size) throw(Exception) {

    // The newname of the original dimension should also be replaced by new_dim_name
    for (vector<Var *>::iterator irv = this->vars.begin();
        irv != this->vars.end(); ++irv) {
        for (vector<Dimension *>::iterator ird= (*irv)->dims.begin();
            ird != (*irv)->dims.end(); ++ird) {
            if((*ird)->size == orig_dim_name) {
                (*ird)->name = new_dim_name;
                (*ird)->newname = new_dim_name;
            }

        }
    }
}
#endif
void 
File:: Add_Str_Attr(Attribute* attr,const string &attrname, const string& strvalue) throw(Exception){

    attr->name    = attrname;
    attr->newname = attr->name;
    attr->dtype   = H5FSTRING;
    attr->count   = 1;
    attr->fstrsize = strvalue.size();
    attr->strsize.resize(1);
    attr->strsize[0] = attr->fstrsize;
    attr->value.resize(strvalue.size());
    copy(strvalue.begin(),strvalue.end(),attr->value.begin());
}

bool 
File:: Is_Str_Attr(Attribute* attr,string varfullpath,const string &attrname, const string& strvalue) {
    bool ret_value = false;
    if(attrname== get_CF_string(attr->newname)) {
       Retrieve_H5_Attr_Value(attr,varfullpath);
        string attr_value(attr->value.begin(),attr->value.end());
        if(attr_value == strvalue) 
            ret_value= true;
    }
    return ret_value;
}

void
File:: Add_One_Float_Attr(Attribute* attr,const string &attrname, float float_value) throw(Exception){
    attr->name = attrname;
    attr->newname = attr->name;
    attr->dtype   = H5FLOAT32;
    attr->count   = 1;
    attr->value.resize(sizeof(float));
    memcpy(&(attr->value[0]),(void*)(&float_value),sizeof(float));
}

void
File::Change_Attr_One_Str_to_Others(Attribute* attr, Var*var) throw(Exception) {

    
    char *pEnd;
    // string to long int number.
    long int num_sli = 0;
    if(attr->dtype != H5FSTRING) 
        throw2("Currently we only convert fixed-size string to other datatypes. ", attr->name);
    if(attr->count != 1)
        throw4("The fixed-size string count must be 1 and the current count is ",attr->count, " for the attribute ",attr->name);

    Retrieve_H5_Attr_Value(attr,var->fullpath);
    string attr_value;
    attr_value.resize(attr->value.size());
    copy(attr->value.begin(),attr->value.end(),attr_value.begin());
 
    switch(var->dtype) {

        case H5UCHAR:
        {
            num_sli = strtol(&(attr->value[0]),&pEnd,10);
            if(num_sli <0 || num_sli >UCHAR_MAX) 
                throw5("Attribute type is unsigned char, the current attribute ",attr->name, " has the value ",num_sli, ". It is overflowed. ");
            else {
                unsigned char num_suc =(unsigned char)num_sli;
                attr->dtype = H5UCHAR; 
                 //attr->count = 1;
                attr->value.resize(sizeof(unsigned char));
                memcpy(&(attr->value[0]),(void*)(&num_suc),sizeof(unsigned char));
            }

        }
            break;
        case H5CHAR:
        {
            num_sli = strtol(&(attr->value[0]),&pEnd,10);
            if(num_sli <SCHAR_MIN || num_sli >SCHAR_MAX) 
                throw5("Attribute type is signed char, the current attribute ",attr->name, " has the value ",num_sli, ". It is overflowed. ");
            else {
                char num_sc = (char)num_sli;
                attr->dtype = H5CHAR; 
                 //attr->count = 1;
                attr->value.resize(sizeof(char));
                memcpy(&(attr->value[0]),(void*)(&num_sc),sizeof(char));
            }

        }
            break;
        case H5INT16:
        {
            num_sli = strtol(&(attr->value[0]),&pEnd,10);
            if(num_sli <SHRT_MIN || num_sli >SHRT_MAX) 
                throw5("Attribute type is 16-bit integer, the current attribute ",attr->name, " has the value ",num_sli, ". It is overflowed. ");
            else {
                short num_ss = (short)num_sli;
                attr->dtype = H5INT16; 
                //attr->count = 1;
                attr->value.resize(sizeof(short));
                memcpy(&(attr->value[0]),(void*)(&num_ss),sizeof(short));
            }


        }
            break;
        case H5UINT16:
        {
            num_sli = strtol(&(attr->value[0]),&pEnd,10);
            if(num_sli <0 || num_sli >USHRT_MAX) 
                throw5("Attribute type is unsigned 16-bit integer, the current attribute ",attr->name, " has the value ",num_sli, ". It is overflowed. ");
            else {
                unsigned short num_uss = (unsigned short)num_sli;
                attr->dtype = H5UINT16; 
                //attr->count = 1;
                attr->value.resize(sizeof(unsigned short));
                memcpy(&(attr->value[0]),(void*)(&num_uss),sizeof(unsigned short));
            }
        }
            break;
        case H5INT32:
        {
            num_sli = strtol(&(attr->value[0]),&pEnd,10);
            if(num_sli <LONG_MIN || num_sli >LONG_MAX) 
                throw5("Attribute type is 32-bit integer, the current attribute ",attr->name, " has the value ",num_sli, ". It is overflowed. ");
            else {
                attr->dtype = H5INT32; 
                //attr->count = 1;
                attr->value.resize(sizeof(long int));
                memcpy(&(attr->value[0]),(void*)(&num_sli),sizeof(long int));
            }

        }
            break;
        case H5UINT32:
        {
            unsigned long int num_suli = strtoul(&(attr->value[0]),&pEnd,10);
            if(num_suli >ULONG_MAX) 
                throw5("Attribute type is 32-bit unsigned integer, the current attribute ",attr->name, " has the value ",num_suli, ". It is overflowed. ");
            else {
                attr->dtype = H5UINT32; 
                //attr->count = 1;
                attr->value.resize(sizeof(unsigned long int));
                memcpy(&(attr->value[0]),(void*)(&num_suli),sizeof(unsigned long int));
            }
        }
            break;
        case H5FLOAT32:
        {
            float num_sf = strtof(&(attr->value[0]),NULL);
            // Don't think it is necessary to check if floating-point is oveflowed for this routine. ignore it now. KY 2014-09-22
            attr->dtype = H5FLOAT32;
            //attr->count = 1;
            attr->value.resize(sizeof(float));
            memcpy(&(attr->value[0]),(void*)(&num_sf),sizeof(float));
        }
            break;
        case H5FLOAT64:
        {
            double num_sd = strtod(&(attr->value[0]),NULL);
            // Don't think it is necessary to check if floating-point is oveflowed for this routine. ignore it now. KY 2014-09-22
            attr->dtype = H5FLOAT64;
            //attr->count = 1;
            attr->value.resize(sizeof(double));
            memcpy(&(attr->value[0]),(void*)(&num_sd),sizeof(double));
        }
            break;

        default:
            throw4("Unsupported HDF5 datatype that the string is converted to for the attribute ",attr->name, " of the variable ",var->fullpath);
    } // switch(var->dtype)


}

void 
File:: Replace_Var_Str_Attr(Var* var ,const string &attr_name, const string& strvalue) {

    bool rep_attr = true;
    bool rem_attr = false;
    for(vector<Attribute *>::iterator ira = var->attrs.begin();
        ira != var->attrs.end();ira++) {
        if((*ira)->name == attr_name){
            if(true == Is_Str_Attr(*ira,var->fullpath,attr_name,strvalue)) 
                rep_attr = false; 
            else 
                rem_attr = true;
            break;
        }
    }

    // Remove the attribute if the attribute value is not strvalue
    if( true == rem_attr) {
       for(vector<Attribute *>::iterator ira = var->attrs.begin();
                            ira != var->attrs.end();ira++) {
           if((*ira)->name == attr_name){
               delete(*ira);
               var->attrs.erase(ira);
               break;
           }
       }
   }

   // Add the attribute with strvalue
   if(true == rep_attr) {
       Attribute * attr = new Attribute();
       Add_Str_Attr(attr,attr_name,strvalue);
       var->attrs.push_back(attr);
   }
}



void 
File:: Add_Supplement_Attrs(bool add_path) throw(Exception) {

    if (false == add_path) 
        return;

    // Adding variable original name(origname) and full path(fullpath)
    for (vector<Var *>::iterator irv = this->vars.begin();
                irv != this->vars.end(); ++irv) {
         Attribute * attr = new Attribute();
         const string varname = (*irv)->name; 
         const string attrname = "origname";
         Add_Str_Attr(attr,attrname,varname);
         (*irv)->attrs.push_back(attr);
    }

    for (vector<Var *>::iterator irv = this->vars.begin();
                irv != this->vars.end(); ++irv) {
         Attribute * attr = new Attribute();
         const string varname = (*irv)->fullpath; 
         const string attrname = "fullnamepath";
         Add_Str_Attr(attr,attrname,varname);
         (*irv)->attrs.push_back(attr);
    }

    // Adding group path
    for (vector<Group *>::iterator irg = this->groups.begin();
                irg != this->groups.end(); ++irg) {
        // Only when this group has attributes, the original path of the group has some values. So add it.
        if (false == (*irg)->attrs.empty()) {

            Attribute * attr = new Attribute();
            const string varname = (*irg)->path;
            const string attrname = "fullnamepath";
            Add_Str_Attr(attr,attrname,varname);
            (*irg)->attrs.push_back(attr);
        }
    }

}

// Variable target will not be deleted, but rather its contents are replaced.
// We may make this as an operator = in the future.
void
File:: Replace_Var_Info(Var *src, Var *target) {


#if 0
    for_each (target->dims.begin (), target->dims.end (),
                                     delete_elem ());
    for_each (target->attrs.begin (), target->attrs.end (),
                                     delete_elem ());
#endif

    target->newname = src->newname;
    target->name = src->name;
    target->fullpath = src->fullpath;
    target->rank  = src->rank;
    target->dtype = src->dtype;
    target->unsupported_attr_dtype = src->unsupported_attr_dtype;
    target->unsupported_dspace = src->unsupported_dspace;
#if 0
    for (vector<Attribute*>::iterator ira = target->attrs.begin();
        ira!=target->attrs.end(); ++ira) {
        delete (*ira);
        target->attrs.erase(ira);
        ira--;
    }
#endif
    for (vector<Dimension*>::iterator ird = target->dims.begin();
        ird!=target->dims.end(); ++ird) {
        delete (*ird);
        target->dims.erase(ird);
        ird--;
    }
 
    // Somehow attributes cannot be replaced. 
#if 0
    for (vector<Attribute*>::iterator ira = src->attrs.begin();
        ira!=src->attrs.end(); ++ira) {
        Attribute* attr= new Attribute();
        attr->name = (*ira)->name;
        attr->newname = (*ira)->newname;
        attr->dtype =(*ira)->dtype;
        attr->count =(*ira)->count;
        attr->strsize = (*ira)->strsize;
        attr->fstrsize = (*ira)->fstrsize;
        attr->value =(*ira)->value;
        target->attrs.push_back(attr);
    }
#endif

    for (vector<Dimension*>::iterator ird = src->dims.begin();
        ird!=src->dims.end(); ++ird) {
        Dimension *dim = new Dimension((*ird)->size);
        dim->name = (*ird)->name;
        dim->newname = (*ird)->newname;
        target->dims.push_back(dim);
    }

}

 void
File:: Replace_Var_Attrs(Var *src, Var *target) {


#if 0
    for_each (target->dims.begin (), target->dims.end (),
                                     delete_elem ());
    for_each (target->attrs.begin (), target->attrs.end (),
                                     delete_elem ());
#endif

    for (vector<Attribute*>::iterator ira = target->attrs.begin();
        ira!=target->attrs.end(); ++ira) {
        delete (*ira);
        target->attrs.erase(ira);
        ira--;
    }
    for (vector<Attribute*>::iterator ira = src->attrs.begin();
        ira!=src->attrs.end(); ++ira) {
        Attribute* attr= new Attribute();
        attr->name = (*ira)->name;
        attr->newname = (*ira)->newname;
        attr->dtype =(*ira)->dtype;
        attr->count =(*ira)->count;
        attr->strsize = (*ira)->strsize;
        attr->fstrsize = (*ira)->fstrsize;
        attr->value =(*ira)->value;
        target->attrs.push_back(attr);
    }

}              

void 
File:: add_ignored_info_page_header() {
    ignored_msg = " \n This page is for HDF5 CF hyrax data providers or distributors to check if any HDF5 object or attribute information are ignored during the mapping. \n\n";
}

void 
File:: add_ignored_info_obj_header() {

    ignored_msg += " Some HDF5 objects or the object information are ignored when mapping to DAP2 by the HDF5 OPeNDAP";
    ignored_msg += " handler due to the restrictions of DAP2, CF conventions or CF tools.";
    ignored_msg += " Please use HDF5 tools(h5dump or HDFView) to check carefully and make sure that these objects";
    ignored_msg += " are OK to ignore for your service. For questions or requests to find a way to handle the ignored objects, please";
    ignored_msg += " contact the HDF5 OPeNDAP handler developer or send an email to help@hdfgroup.org.\n";

    ignored_msg += " \n In general, ignored HDF5 objects include HDF5 soft links, external links and named datatype.\n";
    ignored_msg += " \n The HDF5 datasets(variables in the CF term) and attributes that have the following datatypes are ignored: \n";
    ignored_msg += " Signed and unsigned 64-bit integers, HDF5 compound, HDF5 variable length(excluding variable length string),";
    ignored_msg += " HDF5 reference, HDF5 enum, HDF5 opaque , HDF5 bitfield, HDF5 Array and HDF5 Time datatypes.\n";

    ignored_msg += " \n The HDF5 datasets(variables in the CF term) and attributes associated with the following dimensions are ignored: \n";
    ignored_msg += " 1) non-string datatype scalar variables\n";
    ignored_msg += " 2) variables that have HDF5 NULL dataspace(H5S_NULL)(rarely occurred)\n";
    ignored_msg += " 3) variables that have any zero size dimensions\n\n";

}

void 
File:: add_ignored_info_links_header() {

    if(false == this->have_ignored) {
        add_ignored_info_obj_header();
        have_ignored = true;
    }
    // Add ignored datatype header.
    string lh_msg =  "******WARNING******\n";
    lh_msg +=  "IGNORED soft links or external links are: ";
    if(ignored_msg.rfind(lh_msg) == string::npos)
        ignored_msg += lh_msg + "\n";

}
#if 0
void 
File:: add_ignored_info_obj_dtype_header() {

    // Add ignored datatype header.
    ignored_msg += " \n Variables and attributes ignored due to the unsupported datatypes. \n";
    ignored_msg += " In general, the unsupported datatypes include: \n";
    ignored_msg += " Signed and unsigned 64-bit integers, HDF5 compound, HDF5 variable length(excluding variable length string),";
    ignored_msg += " HDF5 reference, HDF5 enum, HDF5 opaque , HDF5 bitfield, HDF5 Array and HDF5 Time datatypes.\n";

}

void 
File:: add_ignored_info_obj_dspace_header() {

    // Add ignored dataspace header.
    ignored_msg += " \n Variables and attributes ignored due to the unsupported dimensions. \n";
    ignored_msg += " In general, the unsupported dimensions include: \n";
    ignored_msg += " 1) non-string datatype scalar variables\n";
    ignored_msg += " 2) variables that have HDF5 NULL dataspace(H5S_NULL)(rarely occurred)\n";
    ignored_msg += " 3) variables that have any zero size dimensions\n";


}
#endif
void 
File:: add_ignored_info_links(const string & link_path) {
    if(ignored_msg.find("Link paths: ")== string::npos) 
        ignored_msg += " Link paths: " + link_path ;
    else 
        ignored_msg += " "+ link_path;
}

void 
File:: add_ignored_info_namedtypes(const string& grp_name, const string& named_dtype_name) {

    if(false == this->have_ignored) {
        add_ignored_info_obj_header();
        have_ignored = true;
    }

    string ignored_HDF5_named_dtype_hdr = "\n******WARNING******";
    ignored_HDF5_named_dtype_hdr += "\n IGNORED HDF5 named datatype objects:\n";
    string ignored_HDF5_named_dtype_msg = " Group name: " + grp_name + "  HDF5 named datatype name: " +named_dtype_name + "\n";
    if(ignored_msg.find(ignored_HDF5_named_dtype_hdr) == string::npos) 
        ignored_msg += ignored_HDF5_named_dtype_hdr + ignored_HDF5_named_dtype_msg;   
    else 
        ignored_msg += ignored_HDF5_named_dtype_msg;
    
}

void 
File:: add_ignored_info_attrs(bool is_grp,const string & obj_path, const string & attr_name) {

    if(false == this->have_ignored) {
        add_ignored_info_obj_header();
        have_ignored = true;
    }

 //   string ignored_dtype_header_substr = "unsupported datatypes include: ";
    
    string ignored_warning_str = "\n******WARNING******";
    string ignored_HDF5_grp_hdr = ignored_warning_str +"\n Ignored attributes under root and groups:\n";
    string ignored_HDF5_grp_msg = " Group path: " + obj_path + "  Attribute names: " +attr_name + "\n";
    string ignored_HDF5_var_hdr = ignored_warning_str +"\n Ignored attributes for variables:\n";
    string ignored_HDF5_var_msg = " Variable path: " + obj_path + "  Attribute names: " +attr_name + "\n";


//    if(ignored_msg.find(ignored_dtype_header_substr) == string::npos) 
 //       add_ignored_info_obj_dtype_header();
    
    if( true == is_grp) {
        if(ignored_msg.find(ignored_HDF5_grp_hdr) == string::npos) 
            ignored_msg += ignored_HDF5_grp_hdr + ignored_HDF5_grp_msg;   
        else 
            ignored_msg += ignored_HDF5_grp_msg;
    }
    else {
        if(ignored_msg.find(ignored_HDF5_var_hdr) == string::npos) 
            ignored_msg += ignored_HDF5_var_hdr + ignored_HDF5_var_msg;   
        else 
            ignored_msg += ignored_HDF5_var_msg;
    }
    
}

void 
File:: add_ignored_info_objs(bool is_dim_related,const string & obj_path) {

    if(false == this->have_ignored) {
        add_ignored_info_obj_header();
        have_ignored = true;
    }

 //   string ignored_dtype_header_substr = "unsupported datatypes include";
 //   string ignored_dspace_header_substr = "unsupported dimensions include";
    string ignored_warning_str = "\n******WARNING******";
    string ignored_HDF5_dtype_var_hdr = ignored_warning_str +"\n IGNORED variables due to unsupported datatypes:\n";
    string ignored_HDF5_dspace_var_hdr = ignored_warning_str + "\n IGNORED variables due to unsupported dimensions:\n";
    string ignored_HDF5_var_msg = " Variable path: " + obj_path  + "\n";


    if(true == is_dim_related) {
  //      if(ignored_msg.find(ignored_dspace_header_substr) == string::npos) 
   //         add_ignored_info_obj_dspace_header();
        if(ignored_msg.find(ignored_HDF5_dspace_var_hdr) == string::npos) 
            ignored_msg += ignored_HDF5_dspace_var_hdr + ignored_HDF5_var_msg;   
        else 
            ignored_msg += ignored_HDF5_var_msg;
 
    }
    else {
    //    if(ignored_msg.find(ignored_dtype_header_substr) == string::npos) 
    //        add_ignored_info_obj_dtype_header();
        if(ignored_msg.find(ignored_HDF5_dtype_var_hdr) == string::npos) 
            ignored_msg += ignored_HDF5_dtype_var_hdr + ignored_HDF5_var_msg;   
        else 
            ignored_msg += ignored_HDF5_var_msg;
    }
 
}

void 
File:: add_no_ignored_info() {

   ignored_msg += "There are no ignored HDF5 objects or attributes.";

}

// This function should only be used when the HDF5 file is following the netCDF data model.
bool
File::ignored_dimscale_ref_list(Var *var) {

    bool ignored_dimscale = true;
    //if(General_Product == this->product_type && GENERAL_DIMSCALE== this->gproduct_pattern) {

        bool has_dimscale = false;
        bool has_reference_list = false;
        for(vector<Attribute *>::iterator ira = var->attrs.begin();
                     ira != var->attrs.end();ira++) {
             if((*ira)->name == "REFERENCE_LIST" && 
                false == HDF5CFUtil::cf_strict_support_type((*ira)->getType()))
                has_reference_list = true;
             if((*ira)->name == "CLASS") {
                Retrieve_H5_Attr_Value(*ira,var->fullpath);
                string class_value;
                class_value.resize((*ira)->value.size());
                copy((*ira)->value.begin(),(*ira)->value.end(),class_value.begin());

                // Compare the attribute "CLASS" value with "DIMENSION_SCALE". We only compare the string with the size of
                // "DIMENSION_SCALE", which is 15.
                if (0 == class_value.compare(0,15,"DIMENSION_SCALE")) {
                    has_dimscale = true;
                }
            }
 
            if(true == has_dimscale && true == has_reference_list) {
                ignored_dimscale= false;
                break;
            }
           
        }
    //}
    return ignored_dimscale;
}

bool File::Check_DropLongStr(Var *var,Attribute * attr) throw(Exception) {

    bool drop_longstr = false;
    if(NULL == attr) {
        if(H5FSTRING == var->dtype || H5VSTRING == var->dtype) {
            try {
                drop_longstr = Check_VarDropLongStr(var->fullpath,var->dims,var->dtype);
            }
            catch(...) {
                throw1("Check_VarDropLongStr fails ");

            }
        }
        
    }
    else {
        if(H5FSTRING == attr->dtype || H5VSTRING == attr->dtype) {
            if(attr->getBufSize() > NC_JAVA_STR_SIZE_LIMIT) {
               drop_longstr = true;
}
        }

    }
    return drop_longstr;
}

bool File::Check_VarDropLongStr(const string & varpath, const vector<Dimension *>& dims, H5DataType dtype) throw(Exception) {

    bool drop_longstr = false;

    unsigned long long total_elms = 1;
    if(dims.size() !=0) {
        for(int i = 0; i <dims.size(); i++)
            total_elms = total_elms *((dims[i])->size);
        
    }

    if(total_elms > NC_JAVA_STR_SIZE_LIMIT) 
        drop_longstr = true;

    else {

        hid_t dset_id = H5Dopen2(this->fileid,varpath.c_str(),H5P_DEFAULT);
        if(dset_id < 0) 
            throw2 ("Cannot open the dataset  ", varpath);

        hid_t dtype_id = -1;
        if ((dtype_id = H5Dget_type(dset_id)) < 0) {
            H5Dclose(dset_id);
            throw2 ("Cannot obtain the datatype of the dataset  ", varpath);
        }
        
        size_t ty_size =  H5Tget_size(dtype_id);
        if (ty_size == 0) {
            H5Tclose(dtype_id);
            H5Dclose(dset_id);
            throw2 ("Cannot obtain the datatype size of the dataset  ", varpath);
        }

        if(H5FSTRING == dtype) {
            if((ty_size * total_elms) > NC_JAVA_STR_SIZE_LIMIT)
                drop_longstr = true;

        }
 
    

    else if(H5VSTRING == dtype) {

#if 0
        hid_t dset_id = H5Dopen2(this->fileid,varpath.c_str(),H5P_DEFAULT);
        if(dset_id < 0) 
            throw2 ("Cannot open the dataset  ", varpath);

        hid_t dtype_id = -1;
        if ((dtype_id = H5Dget_type(dset_id)) < 0) {
            H5Dclose(dset_id);
            throw2 ("Cannot obtain the datatype of the dataset  ", varpath);
        }
        
        size_t ty_size =  H5Tget_size(dtype_id);
        if (ty_size == 0) {
            H5Tclose(dtype_id);
            H5Dclose(dset_id);
            throw2 ("Cannot obtain the datatype size of the dataset  ", varpath);
        }
#endif
        vector <char> strval;
        strval.resize(total_elms*ty_size);
        hid_t read_ret = H5Dread(dset_id,dtype_id,H5S_ALL,H5S_ALL,H5P_DEFAULT,(void*)&strval[0]);
        if(read_ret < 0) {
            H5Tclose(dtype_id);
            H5Dclose(dset_id);
            throw2 ("Cannot read the data of the dataset  ", varpath);
        }

        
        vector<string>finstrval;
        finstrval.resize(total_elms);
        char*temp_bp = &strval[0];
        char*onestring = NULL;
        for (unsigned long long  i =0;i<total_elms;i++) {
            onestring = *(char**)temp_bp;
            if(onestring!=NULL )
                finstrval[i] =string(onestring);
            else // We will add a NULL if onestring is NULL.
                finstrval[i]="";
            temp_bp +=ty_size;
        }

        if (false == strval.empty()) {
            herr_t ret_vlen_claim;
            hid_t dspace_id = H5Dget_space(dset_id);
            if(dspace_id < 0) {
                H5Tclose(dtype_id);
                H5Dclose(dset_id);
               throw2("Cannot obtain the dataspace id.",varpath);
            }
            ret_vlen_claim = H5Dvlen_reclaim(dtype_id,dspace_id,H5P_DEFAULT,(void*)&strval[0]);
            if (ret_vlen_claim < 0){
                H5Tclose(dtype_id);
                H5Sclose(dspace_id);
                H5Dclose(dset_id);
                throw2 ("Cannot reclaim the vlen space  ", varpath);
            }
            H5Sclose(dspace_id);
        }
        unsigned long long  total_str_size = 0;
        for (unsigned long long  i =0;i<total_elms;i++) {
            total_str_size += finstrval[i].size();
            if (total_str_size  > NC_JAVA_STR_SIZE_LIMIT) {
                drop_longstr = true;
                break;
            } 
        }
         
    }
        H5Tclose(dtype_id);
        H5Dclose(dset_id);
    }
    return drop_longstr;
}

void File::add_ignored_grp_longstr_info(const string& grp_path,const string & attr_name) {

    ignored_msg +="The HDF5 group: " + grp_path + " has an empty-set string attribute: "+ attr_name +"\n";
 
    return;
}

void File::add_ignored_var_longstr_info(Var *var,Attribute *attr) throw(Exception) {

    if(NULL == attr) 
        ignored_msg +="String variable: " + var->fullpath +" value is set to empty.\n";
    else {
        ignored_msg +="The variable: " + var->fullpath + " has an empty-set string attribute: "+ attr->name +"\n";
 
    }
    return;
}
void File::add_ignored_droplongstr_hdr( ) {

    if(false == this->have_ignored)
       this->have_ignored = true;
    string hdr = "\n\n The value of the following string variables or attributes" ;
    hdr += " are set to empty because the string size exceeds netCDF Java string limit(32767 bytes).\n";
    hdr += " Note: for string datasets, if the DAP subset feature is applied and the total subsetted";
    hdr += " string doesn't exceed the netCDF Java string limit, the string value should still return.\n";
    hdr +="To obtain the string value, change the BES key H5.EnableDropLongString=true at the handler BES";
    hdr +=" configuration file(h5.conf)\nto H5.EnableDropLongString=false.\n\n";

    if(ignored_msg.rfind(hdr) == string::npos)
        ignored_msg +=hdr;

}
