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
        H5Fclose(fileid);
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

    // cerr <<"coming to Retrieve_H5_Info "<<endl;
    hid_t root_id;
    if ((root_id = H5Gopen(file_id,"/",H5P_DEFAULT))<0){
        throw1 ("Cannot open the HDF5 root group " );
    }
    this->Retrieve_H5_Obj(root_id,"/",include_attr);
    this->rootid =root_id;

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
            this->Retrieve_H5_Attr_Info(attr,root_id,j, temp_unsup_attr_atype);
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
            if(H5L_TYPE_SOFT == linfo.type  || H5L_TYPE_EXTERNAL == linfo.type)
                continue;

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

                    //  cerr <<"Group full_path_name " <<full_path_name <<endl;


                    group = new Group();
                    group->path = full_path_name;
                    group->newname = full_path_name;

                    cgroup = H5Gopen(grp_id, full_path_name.c_str(),H5P_DEFAULT);
                    if (cgroup < 0)
                        throw2( "Error opening the group ",full_path_name);

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

//                    string full_path_name = ((string(gname) != "/") 
 //                                   ?(string(gname)+"/"+string(oname)):("/"+string(oname)));
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
                    if (cdset < 0)
                        throw2( "Error opening the HDF5 dataset ",full_path_name);

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

                case H5O_TYPE_NAMED_DATATYPE:
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

    hid_t ty_id;

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

    //hsize_t*  dsize = NULL;
    //hsize_t* maxsize = NULL;

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

        // Memory allocation exceptions could have been thrown when
        // creating these, so check if these are not null before deleting.
        //if( dsize != NULL ) 
         //   delete[] dsize;
        //if( maxsize!= NULL ) 
         //   delete[] maxsize;

        throw;
        //throw2("Cannot obtain the dimension information for the dataset ",varname);
    }

}


void 
File:: Retrieve_H5_Attr_Info(Attribute * attr, hid_t obj_id,const int j, bool &unsup_attr_dtype) 
throw(Exception) 

{

//cerr <<"Coming to Retrieve_H5_Attr_Info "<<endl;

    hid_t attrid = -1;
    hid_t ty_id = -1;
    hid_t aspace_id = -1;
    hid_t memtype = -1;

    //char* attr_name = NULL;
    
    //hsize_t* asize  = NULL;
    //hsize_t* maxsize = NULL;

 //   vector<hsize_t> asize;
  //  vector<hsize_t> maxsize;

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

 //           hsize_t* asize =  new hsize_t[ndims];
 //           hsize_t* maxsize = new hsize_t[ndims];
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
//ty_size = 0;
        if (0 == ty_size ) 
            throw2("Cannot obtain the dtype size for the attribute ",attr_name);


        memtype = H5Tget_native_type(ty_id, H5T_DIR_ASCEND);
//memtype = -1;

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

       // if(attr_name != NULL) 
        //   delete []attr_name;
#if 0
        if(asize != NULL) 
           delete []asize;
        if(maxsize != NULL) 
           delete []maxsize;
#endif

      

        // cerr <<"attribute name "<<attr->name <<endl;
        // cerr <<"attribute count "<<attr->count <<endl;
        // cerr <<"attribute type size "<<(int)ty_size <<endl;
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

#if 0
        if(attr_name != NULL) 
            delete []attr_name;
        if( asize != NULL) 
            delete []asize;

        if(maxsize != NULL) 
            delete []maxsize;
#endif

        //throw1("Error in method File::Retrieve_h5_Attr_Info");
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
    // cerr<<"coming to retrieve HDF5 attribute value "<<endl;

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
                throw4("Cannot obtain the dtype size for the attribute ",attr->name, " of object ",obj_name);

            char *temp_bp = NULL;
            temp_bp = &temp_buf[0];
            char* onestring = NULL;
	    string total_vstring ="";

            attr->strsize.resize(attr->count);
            // cerr<<"attr->count "<< attr->count <<endl;

            for (unsigned int temp_i = 0; temp_i <attr->count; temp_i++) {

                // This line will assure that we get the real variable length string value.
                onestring =*(char **)temp_bp;
                if(onestring!= NULL) {
                    // cerr <<"one string "<<onestring <<endl;
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
            // cerr <<"total_string in vlen "<< total_vstring <<endl;

        }
        else {

            if (attr->dtype == H5FSTRING) { 
                attr->fstrsize = ty_size;
            }
            
            attr->value.resize(total_bytes);

            // Read HDF5 attribute data.
            if (H5Aread(attr_id, memtype_id, (void *) &attr->value[0]) < 0) 
                throw4("Cannot obtain the dtype size for the attribute ",attr->name, " of object ",obj_name);

// cerr <<"attr name "<< attr->name <<endl;
// cerr <<"total_bytes " << total_bytes <<endl;
#if 0
if(attr->dtype == H5FSTRING) {
    cerr <<"string fixed size = " << attr->fstrsize <<endl;
    cerr <<"string attr value = " << string(attr->value.begin(), attr->value.end()) <<endl;

}         
#endif
            if (attr->dtype == H5FSTRING) {

                size_t sect_size = ty_size;
                int num_sect = (total_bytes%sect_size==0)?(total_bytes/sect_size)
                                                     :(total_bytes/sect_size+1);
                vector<size_t>sect_newsize;
                sect_newsize.resize(num_sect);

                string total_fstring = string(attr->value.begin(),attr->value.end());
                
                string new_total_fstring = HDF5CFUtil::trim_string(memtype_id,total_fstring,
                                           num_sect,sect_size,sect_newsize);
                // cerr <<"The first new sect size is "<<sect_newsize[0] <<endl; 
                attr->value.resize(new_total_fstring.size());
                copy(new_total_fstring.begin(),new_total_fstring.end(),attr->value.begin()); 
                attr->strsize.resize(num_sect);
                for (int temp_i = 0; temp_i <num_sect; temp_i ++) 
                    attr->strsize[temp_i] = sect_newsize[temp_i];
            // cerr <<"new string value " <<string(attr->value.begin(), attr->value.end()) <<endl;
#if 0
for (int temp_i = 0; temp_i <num_sect; temp_i ++)
     cerr <<"string new section size = " << attr->strsize[temp_i] <<endl;
#endif
            }
        }

        H5Tclose(memtype_id);
        H5Tclose(ty_id);
        H5Aclose(attr_id); 
        H5Oclose(obj_id);
        //if (temp_buf != NULL)
         //   delete []temp_buf;

        //if (sect_newsize != NULL) 
         //   delete [] sect_newsize;

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

        //if (temp_buf != NULL)
         //   delete[]temp_buf;

        //if (sect_newsize !=NULL)
         //   delete[]sect_newsize;

        //throw1("Error in method File::Retrieve_H5_Attr_Value");
        throw;
    }

}
 


void File::Handle_Unsupported_Dtype(bool include_attr) throw(Exception) {

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
           // cerr <<"having unsupported variable datatype" <<endl;
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

void File::Handle_Unsupported_Dspace() throw(Exception) {

    // Then the variable(HDF5 dataset) and the correponding attributes.
    if (false == this->vars.empty()) {
        if (true == this->unsupported_var_dspace) {
            for (vector<Var *>::iterator irv = this->vars.begin();
                 irv != this->vars.end(); ++irv) {
//cerr <<"having unsupported variable datatype" <<endl;
                if (true  == (*irv)->unsupported_dspace) {
                    delete (*irv);
                    this->vars.erase(irv);
                    irv--;
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
            // cerr <<"new name before "<<(*ira)->newname <<endl;
            (*ira)->newname = get_CF_string((*ira)->newname);
            // cerr <<"new name after "<<(*ira)->newname <<endl;
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

void File::Handle_RootGroup_NameClashing(set<string> &objnameset ) throw(Exception) {


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
        Handle_RootGroup_NameClashing(objnameset );
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
    if (false == mapret.second) throw4("The dimension name ",name," should map to ",
                                      size);

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
            if(false == setret.second) throw2("Fail to insert the unique dimsizede name ", dim->name);

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
File:: Add_Supplement_Attrs(bool add_path) throw(Exception) {

    if (false == add_path) return;

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

               
