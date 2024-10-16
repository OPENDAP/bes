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
/// \file HDF5GCFProduct.cc
/// \brief The implementation of functions to identify different NASA HDF5 products.
/// Current supported products include MEaSUREs SeaWiFS, OZone, Aquarius level 3
/// Old SMAP Level 2 Simulation files and ACOS level 2S(OCO level1B).
///
/// \author Kent Yang <myang6@hdfgroup.org>
///
////////////////////////////////////////////////////////////////////////////////


#include <libdap/InternalErr.h>
#include <assert.h>
#include "HDF5GCFProduct.h"
#include "h5apicompatible.h"

using namespace std;
using namespace libdap;


// The knowledge on how to distinguish each HDF5 product is all buried here
// All product attribute names and values are defined at the header files. 
H5GCFProduct check_product(hid_t file_id) {

    hid_t root_id = -1;
    H5GCFProduct product_type = General_Product;

    // Open the root group.
    if ((root_id = H5Gopen(file_id,ROOT_NAME,H5P_DEFAULT))<0){
        string msg = "cannot open the HDF5 root group  ";
        msg += string(ROOT_NAME);
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    // Check if the product is MEaSUREs SeaWiFS
    int s_level = -1;

    // Also set Aquarius level, although we only support
    // Aquarius level 3, in the future we may support Aquarius
    // level 2.
    int a_level = -1;


    // Check if the product is GPM level 1, if yes, just return.
    if (true == check_gpm_l1(root_id)){     
        product_type = GPM_L1;
    }

    // Check if the product is GPM level 3, if yes, just return.
    else if (true == check_gpms_l3(root_id)){     
        product_type = GPMS_L3;
    }

    else if (true == check_gpmm_l3(root_id)) {
        product_type = GPMM_L3;

    }

    else if (true == check_measure_seawifs(root_id,s_level)) {
        if (2 == s_level) product_type =  Mea_SeaWiFS_L2;
        if (3 == s_level) product_type =  Mea_SeaWiFS_L3;
    }

    else if (true == check_aquarius(root_id,a_level)){
        if (3 == a_level) product_type =  Aqu_L3;
    }
    else if (true == check_obpg(root_id,a_level)){
        if (3 == a_level) product_type =  OBPG_L3;
    }

    else if (true == check_measure_ozone(root_id)) {
        product_type = Mea_Ozone;
    }
    else { 
        int osmapl2s_flag = 1; // This is OSMAPL2S 
        if (true == check_osmapl2s_acosl2s_oco2l1b(root_id,osmapl2s_flag)) 
            product_type =  OSMAPL2S;

        if (General_Product == product_type) {

            int acosl2s_oco2l1b_flag = 2; // This is ACOSL2S_OR_OCO2L1B
            if (true == check_osmapl2s_acosl2s_oco2l1b(root_id,acosl2s_oco2l1b_flag)) 
                product_type =  ACOS_L2S_OR_OCO2_L1B;
        }
       
    }

    H5Gclose(root_id);
    return product_type;
}

// Function to check if the product is GPM level 1
bool check_gpm_l1(hid_t s_root_id) {

    htri_t has_gpm_l1_attr1 = -1;
    bool   ret_flag = false;


    // Here we check the existence of attribute 1 first 
    has_gpm_l1_attr1 = H5Aexists(s_root_id,GPM_ATTR1_NAME);

    if(has_gpm_l1_attr1 >0) {

        H5G_info_t g_info;
        hsize_t nelms = 0;

        if(H5Gget_info(s_root_id,&g_info) <0) {
            H5Gclose(s_root_id);
            throw InternalErr(__FILE__,__LINE__,"Cannot get the HDF5 object info. successfully");
        }

        nelms = g_info.nlinks;

        hid_t cgroup = -1;
        hid_t attrid = -1;

        for (unsigned int i = 0; i<nelms; i++) {

            try {

                size_t dummy_name_len = 1;

                // Query the length of object name.
                ssize_t oname_size = 
                          H5Lget_name_by_idx(s_root_id,".",H5_INDEX_NAME,H5_ITER_NATIVE,i,nullptr,
                                             dummy_name_len, H5P_DEFAULT);
                if (oname_size <= 0)
                    throw InternalErr(__FILE__,__LINE__,"Error getting the size of the hdf5 object from the root group. ");

                // Obtain the name of the object 
                vector<char> oname;
                oname.resize((size_t)oname_size+1);

                if (H5Lget_name_by_idx(s_root_id,".",H5_INDEX_NAME,H5_ITER_NATIVE,i,oname.data(),
                    (size_t)(oname_size+1), H5P_DEFAULT) < 0)
                    throw InternalErr(__FILE__,__LINE__,"Error getting the hdf5 object name from the root group. ");

                // Check if it is the hard link or the soft link
                H5L_info_t linfo;
                if (H5Lget_info(s_root_id,oname.data(),&linfo,H5P_DEFAULT)<0)
                    throw InternalErr (__FILE__,__LINE__,"HDF5 link name error from the root group. ");

                // Ignore soft links and external links  
                if(H5L_TYPE_SOFT == linfo.type  || H5L_TYPE_EXTERNAL == linfo.type)
                    continue;

                // Obtain the object type, such as group or dataset. 
                H5O_info_t soinfo;
                if(H5OGET_INFO_BY_IDX(s_root_id,".",H5_INDEX_NAME,H5_ITER_NATIVE, (hsize_t)i,&soinfo,H5P_DEFAULT)<0) 
                    throw InternalErr(__FILE__,__LINE__,"Cannot get the HDF5 object info. successfully. ");

                H5O_type_t obj_type = soinfo.type;

                // We only need to check the group attribute.
                if(obj_type == H5O_TYPE_GROUP) {

                    // Check the attribute name of that group
                    cgroup = H5Gopen(s_root_id,oname.data(),H5P_DEFAULT);
                    if(cgroup < 0) 
                        throw InternalErr(__FILE__,__LINE__,"Cannot open the group.");

                    int num_attrs = soinfo.num_attrs;

                    // Loop through all the attributes to see if the GPM level 1 swath header exists.
                    for (int j = 0; j < num_attrs; j++) {

                        // Obtain the attribute ID.
                        if ((attrid = H5Aopen_by_idx(cgroup, ".", H5_INDEX_CRT_ORDER, H5_ITER_INC,(hsize_t)j, H5P_DEFAULT, H5P_DEFAULT)) < 0)
                            throw InternalErr(__FILE__,__LINE__,"Unable to open attribute by index " );

                        // Obtain the size of the attribute name.
                        ssize_t name_size =  H5Aget_name(attrid, 0, nullptr);
                        if (name_size < 0)
                            throw InternalErr(__FILE__,__LINE__,"Unable to obtain the size of the hdf5 attribute name  " );

                        string attr_name;
                        attr_name.resize(name_size+1);

                        // Obtain the attribute name.    
                        if ((H5Aget_name(attrid, name_size+1, &attr_name[0])) < 0)
                            throw InternalErr(__FILE__,__LINE__,"unable to obtain the hdf5 attribute name  ");

                        string swathheader(GPM_SWATH_ATTR2_NAME);
                        if(attr_name.rfind(swathheader) !=string::npos) {
                            H5Aclose(attrid);
                            ret_flag = true;
                            break;
                        }
                        H5Aclose(attrid);
                    }

                    if(true == ret_flag){
                        H5Gclose(cgroup);
                        break;
                    }
                    H5Gclose(cgroup);
                }
            }
            catch(...) {
                if(s_root_id != -1)
                    H5Gclose(s_root_id);
                if(cgroup != -1)
                    H5Gclose(cgroup);
                if(attrid != -1)
                    H5Aclose(attrid);
                throw;
            }
        }
 
    }

    return ret_flag;

}


// Function to check if the product is GPM level 3
bool check_gpms_l3(hid_t s_root_id) {

    htri_t has_gpm_l3_attr1 = -1;
    bool   ret_flag = false;

    // Here we check the existence of attribute 1 first 
    has_gpm_l3_attr1 = H5Aexists(s_root_id,GPM_ATTR1_NAME);

    if(has_gpm_l3_attr1 >0) {

        htri_t has_gpm_grid_group = -1;

        has_gpm_grid_group = H5Lexists(s_root_id,GPM_GRID_GROUP_NAME1,H5P_DEFAULT);

        hid_t s_group_id = -1;
        if (has_gpm_grid_group >0){

            // Open the group
            if ((s_group_id = H5Gopen(s_root_id, GPM_GRID_GROUP_NAME1,H5P_DEFAULT))<0) {
                string msg = "Cannot open the HDF5 Group  ";
                msg += string(GPM_GRID_GROUP_NAME1);
                H5Gclose(s_root_id);
                throw InternalErr(__FILE__, __LINE__, msg);
            }
        }
        else {

            if(H5Lexists(s_root_id,GPM_GRID_GROUP_NAME2,H5P_DEFAULT) >0) {
                // Open the group
                if ((s_group_id = H5Gopen(s_root_id, GPM_GRID_GROUP_NAME2,H5P_DEFAULT))<0) {
                    string msg = "Cannot open the HDF5 Group  ";
                    msg += string(GPM_GRID_GROUP_NAME2);
                    H5Gclose(s_root_id);
                    throw InternalErr(__FILE__, __LINE__, msg);
                }
 
            }

        }
        if (s_group_id >0) {
            htri_t has_gpm_l3_attr2 = -1;
            has_gpm_l3_attr2 = H5Aexists(s_group_id,GPM_ATTR2_NAME);
            if (has_gpm_l3_attr2 >0) 
                ret_flag = true;

            H5Gclose(s_group_id);
        }


    }

    return ret_flag;

}


bool check_gpmm_l3(hid_t s_root_id) {

    htri_t has_gpm_l3_attr1 = -1;
    bool   ret_flag = false;


    // Here we check the existence of attribute 1 first 
    has_gpm_l3_attr1 = H5Aexists(s_root_id,GPM_ATTR1_NAME);

    if(has_gpm_l3_attr1 >0) {

        if(H5Lexists(s_root_id,GPM_GRID_MULTI_GROUP_NAME,H5P_DEFAULT) >0) {
            hid_t cgroup_id = -1;
            // Open the group
            if ((cgroup_id = H5Gopen(s_root_id, GPM_GRID_MULTI_GROUP_NAME,H5P_DEFAULT))<0) {
                string msg = "Cannot open the HDF5 Group  ";
                msg += string(GPM_GRID_MULTI_GROUP_NAME);
                H5Gclose(s_root_id);
                throw InternalErr(__FILE__, __LINE__, msg);
            }

            H5G_info_t g_info;
            hsize_t nelms = 0;

            if(H5Gget_info(cgroup_id,&g_info) <0) {
                H5Gclose(cgroup_id);
                H5Gclose(s_root_id);
                throw InternalErr(__FILE__,__LINE__,"Cannot get the HDF5 object info. successfully");
            }

            nelms = g_info.nlinks;

            hid_t cgroup2_id = -1;
            hid_t attrid = -1;

            for (unsigned int i = 0; i<nelms; i++) {

                try {

                    size_t dummy_name_len = 1;

                    // Query the length of object name.
                    ssize_t oname_size = 
                    H5Lget_name_by_idx(cgroup_id,".",H5_INDEX_NAME,H5_ITER_NATIVE,i,nullptr,
                                                     dummy_name_len, H5P_DEFAULT);
                    if (oname_size <= 0)
                        throw InternalErr(__FILE__,__LINE__,"Error getting the size of the hdf5 object from the grid group. ");

                    // Obtain the name of the object 
                    vector<char> oname;
                    oname.resize((size_t)oname_size+1);

                    if (H5Lget_name_by_idx(cgroup_id,".",H5_INDEX_NAME,H5_ITER_NATIVE,i,oname.data(),
                                    (size_t)(oname_size+1), H5P_DEFAULT) < 0)
                        throw InternalErr(__FILE__,__LINE__,"Error getting the hdf5 object name from the root group. ");

                    // Check if it is the hard link or the soft link
                    H5L_info_t linfo;
                    if (H5Lget_info(cgroup_id,oname.data(),&linfo,H5P_DEFAULT)<0)
                        throw InternalErr (__FILE__,__LINE__,"HDF5 link name error from the root group. ");

                    // Ignore soft links and external links  
                    if(H5L_TYPE_SOFT == linfo.type  || H5L_TYPE_EXTERNAL == linfo.type)
                        continue;

                    // Obtain the object type, such as group or dataset. 
                    H5O_info_t soinfo;
                    if(H5OGET_INFO_BY_IDX(cgroup_id,".",H5_INDEX_NAME,H5_ITER_NATIVE, (hsize_t)i,&soinfo,H5P_DEFAULT)<0) 
                        throw InternalErr(__FILE__,__LINE__,"Cannot get the HDF5 object info. successfully. ");

                    H5O_type_t obj_type = soinfo.type;

                    // We only need to check the group attribute.
                    if(obj_type == H5O_TYPE_GROUP) {

                        // Check the attribute name of that group
                        cgroup2_id = H5Gopen(cgroup_id,oname.data(),H5P_DEFAULT);
                        if(cgroup2_id < 0)
                            throw InternalErr(__FILE__,__LINE__,"Cannot open the group.");

                        htri_t has_gpm_l3_attr2;
                        has_gpm_l3_attr2 = H5Aexists(cgroup2_id,GPM_ATTR2_NAME);
                        if (has_gpm_l3_attr2 >0) { 
                            ret_flag = true;
                            H5Gclose(cgroup2_id);
                            break;
                        }
                        else {

                            auto num_attrs = (int)(soinfo.num_attrs);

                            // Loop through all the attributes to see if the GPM level 1 swath header exists.
                            for (int j = 0; j < num_attrs; j++) {

                                // Obtain the attribute ID.
                                if ((attrid = H5Aopen_by_idx(cgroup2_id, ".", H5_INDEX_CRT_ORDER, H5_ITER_INC,(hsize_t)j, H5P_DEFAULT, H5P_DEFAULT)) < 0)
                                    throw InternalErr(__FILE__,__LINE__,"Unable to open attribute by index " );

                                // Obtain the size of the attribute name.
                                ssize_t name_size =  H5Aget_name(attrid, 0, nullptr);
                                if (name_size < 0)
                                    throw InternalErr(__FILE__,__LINE__,"Unable to obtain the size of the hdf5 attribute name  " );

                                string attr_name;
                                attr_name.resize(name_size+1);

                                // Obtain the attribute name.    
                                if ((H5Aget_name(attrid, name_size+1, &attr_name[0])) < 0)
                                    throw InternalErr(__FILE__,__LINE__,"unable to obtain the hdf5 attribute name  ");

                                string gridheader(GPM_ATTR2_NAME);
                                if(attr_name.find(gridheader) !=string::npos) {
                                    ret_flag = true;
                                    break;
                                }
                            }

                            if(true == ret_flag)
                                break;
 
                        }

                        H5Gclose(cgroup2_id);
 
                    }
                }
                catch(...) {
                    if(s_root_id != -1)
                        H5Gclose(s_root_id);
                    if(cgroup_id != -1)
                        H5Gclose(cgroup_id);
                    if(cgroup2_id != -1)
                        H5Gclose(cgroup2_id);
                    throw;
                }
            }
            H5Gclose(cgroup_id);
        }
    } 
    return ret_flag;
}

// Function to check if the product is MeaSure seaWiFS
bool check_measure_seawifs(hid_t s_root_id,int & s_lflag) {

    htri_t has_seawifs_attr1 = -1;
    bool   ret_flag = false;

    // Here we check the existence of attribute 1 first since
    // attribute 1 will tell if the product is SeaWIFS or not. 
    // attribute 2 and 3 will distinguish if it is level 2 or level 3.
    has_seawifs_attr1 = H5Aexists(s_root_id,SeaWiFS_ATTR1_NAME);

    if (has_seawifs_attr1 >0) {

        string attr1_value="";
        obtain_gm_attr_value(s_root_id, SeaWiFS_ATTR1_NAME, attr1_value);
        if (0 == attr1_value.compare(SeaWiFS_ATTR1_VALUE)) {
            htri_t has_seawifs_attr2 = -1;
            htri_t has_seawifs_attr3 = -1;
            has_seawifs_attr2 = H5Aexists(s_root_id,SeaWiFS_ATTR2_NAME);
            has_seawifs_attr3 = H5Aexists(s_root_id,SeaWiFS_ATTR3_NAME);

            if ((has_seawifs_attr2 >0) && (has_seawifs_attr3 > 0)){
                string attr2_value ="";
                string  attr3_value ="";
                obtain_gm_attr_value(s_root_id,SeaWiFS_ATTR2_NAME, attr2_value);
                obtain_gm_attr_value(s_root_id,SeaWiFS_ATTR3_NAME, attr3_value);

                // The first part of the <long_name> should be "SeaWiFS"
                // "Level 2" or "Level 3" should also be inside <long_name>.
                // OR the short_name should start with "SWDB_L2" and "SWDB_L3".
                 
                if (((0 == attr2_value.find(SeaWiFS_ATTR2_FPVALUE)) &&
                     (attr2_value.find(SeaWiFS_ATTR2_L2PVALUE)!=string::npos))
                   ||(0 == attr3_value.find(SeaWiFS_ATTR3_L2FPVALUE))) { 
                    // "h5","coming to seaWiFS level 2" <<endl
                    s_lflag = 2;
                    ret_flag = true; 
                }
                else if (((0 == attr2_value.find(SeaWiFS_ATTR2_FPVALUE)) &&
                        (attr2_value.find(SeaWiFS_ATTR2_L3PVALUE)!=string::npos))
                        ||(0 == attr3_value.find(SeaWiFS_ATTR3_L3FPVALUE))) { 
                    s_lflag = 3;
                    ret_flag = true; 
                }
            }
            // else if long_name or short_name don't exist, not what we supported.
            else if ((0 == has_seawifs_attr2 ) || (0 == has_seawifs_attr3))
                ; // no op
            else {
                string msg = "Fail to determine if the HDF5 attribute  ";
                msg += string(SeaWiFS_ATTR2_NAME);
                msg += "or the HDF5 attribute ";
                msg += string(SeaWiFS_ATTR3_NAME);
                msg +=" exists ";
                H5Gclose(s_root_id);
                throw InternalErr(__FILE__, __LINE__, msg);
            }
        }
    }
    else if (0 == has_seawifs_attr1) 
            ;//no op
    else {
        string msg = "Fail to determine if the HDF5 attribute  ";
        msg += string(SeaWiFS_ATTR1_NAME);
        msg +=" exists ";
        H5Gclose(s_root_id);
        throw InternalErr(__FILE__, __LINE__, msg);
    }
    return ret_flag;
}

 // Function to check if the product is MeaSure seaWiFS
bool check_measure_ozone(hid_t s_root_id) {

    htri_t has_ozone_attr1 = -1;
    bool  ret_flag = false;

    // Here we check the existence of attribute 1 first since
    // attribute 1 will tell if the product is SeaWIFS or not. 
    // attribute 2 and 3 will distinguish if it is level 2 or level 3.
    has_ozone_attr1 = H5Aexists(s_root_id,Ozone_ATTR1_NAME);

    if (has_ozone_attr1 >0) {
        string attr1_value = "";
        obtain_gm_attr_value(s_root_id, Ozone_ATTR1_NAME, attr1_value);
        if ((0 == attr1_value.compare(Ozone_ATTR1_VALUE1)) ||
            (0 == attr1_value.compare(Ozone_ATTR1_VALUE2))) {
            htri_t has_ozone_attr2 = -1;
            has_ozone_attr2 = H5Aexists(s_root_id,Ozone_ATTR2_NAME);
            if (has_ozone_attr2 >0) {
                string attr2_value = "";
                obtain_gm_attr_value(s_root_id,Ozone_ATTR2_NAME, attr2_value);
                if(0 == attr2_value.compare(Ozone_ATTR2_VALUE)) 
                    ret_flag = true;
            }
            // else if "ParameterName" attributes don't exist, not what we supported.
            else if (0 == has_ozone_attr2)
                     ;// no op
            else {
                string msg = "Fail to determine if the HDF5 attribute  ";
                msg += string(Ozone_ATTR2_NAME);
                msg +=" exists ";
                H5Gclose(s_root_id);
                throw InternalErr(__FILE__, __LINE__, msg);
            }
        }
    }
    else if (0 == has_ozone_attr1 )
        ; // no op 
    else {
        string msg = "Fail to determine if the HDF5 attribute  ";
        msg += string(Ozone_ATTR1_NAME);
        msg +=" exists ";
        H5Gclose(s_root_id);
        throw InternalErr(__FILE__, __LINE__, msg);
    }
    return ret_flag;
}       
// Function to check if the product is Aquarius
// We still leave a flag to indicate the level although
// we just support the special arrangement of level 3 now. 
// Possibly level 2 can be added 
// in the future.
bool check_aquarius(hid_t s_root_id,int & s_level) {

    htri_t has_aquarius_attr1 = -1;
    bool ret_flag = false;

    // Here we check the existence of attribute 1 first since
    // attribute 1 will tell if the product is Aquarius or not. 
    // attribute 2 will tell its level.
    has_aquarius_attr1 = H5Aexists(s_root_id,Aquarius_ATTR1_NAME);
    if (has_aquarius_attr1 >0) {
        string attr1_value = "";
        obtain_gm_attr_value(s_root_id, Aquarius_ATTR1_NAME, attr1_value);
        if (0 == attr1_value.compare(Aquarius_ATTR1_VALUE)) {
            htri_t has_aquarius_attr2 = -1;
            has_aquarius_attr2 = H5Aexists(s_root_id,Aquarius_ATTR2_NAME);
            if (has_aquarius_attr2 >0) {
                string attr2_value ="";
                obtain_gm_attr_value(s_root_id,Aquarius_ATTR2_NAME, attr2_value);

                // The "Title" of Aquarius should include "Level-3".
                if (attr2_value.find(Aquarius_ATTR2_PVALUE)!=string::npos){
                    // Set it to level 3
                    s_level = 3;
                    ret_flag = true; 
                }
            }
            // else if long_name or short_name don't exist, not what we supported.
            else if (0 == has_aquarius_attr2)
                     ; // no op
            else {
                string msg = "Fail to determine if the HDF5 attribute  ";
                msg += string(Aquarius_ATTR2_NAME);
                msg +=" exists ";
                H5Gclose(s_root_id);
                throw InternalErr(__FILE__, __LINE__, msg);
            }
        }
    }
    else if (0 == has_aquarius_attr1) {
        htri_t has_aquarius_attr1_2 = H5Aexists(s_root_id,Aquarius_ATTR1_NAME2);
        if (has_aquarius_attr1_2 >0) {
            string attr1_value = "";
            obtain_gm_attr_value(s_root_id, Aquarius_ATTR1_NAME2, attr1_value);
            if (0 == attr1_value.compare(Aquarius_ATTR1_VALUE)) {
                htri_t has_aquarius_attr2 = -1;
                has_aquarius_attr2 = H5Aexists(s_root_id,Aquarius_ATTR2_NAME2);
                if (has_aquarius_attr2 >0) {
                    string attr2_value ="";
                    obtain_gm_attr_value(s_root_id,Aquarius_ATTR2_NAME2, attr2_value);
    
                    // The "Title" of Aquarius should include "Level-3".
                    if (attr2_value.find(Aquarius_ATTR2_PVALUE)!=string::npos){
                        // Set it to level 3
                        s_level = 3;
                        ret_flag = true; 
                    }
                }
                // else if long_name or short_name don't exist, not what we supported.
                else if (0 == has_aquarius_attr2)
                         ; // no op
                else {
                    string msg = "Fail to determine if the HDF5 attribute  ";
                    msg += string(Aquarius_ATTR2_NAME2);
                    msg +=" exists ";
                    H5Gclose(s_root_id);
                    throw InternalErr(__FILE__, __LINE__, msg);
                }
            }
    
        }
        else if (0 == has_aquarius_attr1_2)
                     ; // no op
        else {
            string msg = "Fail to determine if the HDF5 attribute  ";
            msg += string(Aquarius_ATTR1_NAME2);
            msg +=" exists ";
            H5Gclose(s_root_id);
            throw InternalErr(__FILE__, __LINE__, msg);
        }
    }
    else {
        string msg = "Fail to determine if the HDF5 attribute  ";
        msg += string(Aquarius_ATTR1_NAME);
        msg +=" exists ";
        H5Gclose(s_root_id);
        throw InternalErr(__FILE__, __LINE__, msg);
    }
    return ret_flag;
}       

// Function to check if the product is OBPG level 3.
// We leave a flag to indicate if this product is level 3 for 
// possible level 2 support later.
bool check_obpg(hid_t s_root_id,int & s_level) {

    htri_t has_obpg_attr1 = -1;
    bool ret_flag = false;

    // Here we check the existence of attribute 1 first since
    // attribute 1 will tell if the product is OBPG or not. 
    // attribute 2 will tell its level.
    has_obpg_attr1 = H5Aexists(s_root_id,Obpgl3_ATTR1_NAME);
    if (has_obpg_attr1 >0) {
        string attr1_value = "";
        obtain_gm_attr_value(s_root_id, Obpgl3_ATTR1_NAME, attr1_value);
        htri_t has_obpg_attr2 = -1;
        has_obpg_attr2 = H5Aexists(s_root_id,Obpgl3_ATTR2_NAME);
        if (has_obpg_attr2 >0) {
            string attr2_value ="";
            obtain_gm_attr_value(s_root_id,Obpgl3_ATTR2_NAME, attr2_value);
            if ((0 == attr1_value.compare(Obpgl3_ATTR1_VALUE)) &&
                (0 == attr2_value.compare(Obpgl3_ATTR2_VALUE))) {
                    // Set it to level 3
                    s_level = 3;
                    ret_flag = true; 
            }
        }
        // else if long_name or short_name don't exist, not what we supported.
        else if (0 == has_obpg_attr2)
            ; // no op
        else {
                string msg = "Fail to determine if the HDF5 attribute  ";
                msg += string(Obpgl3_ATTR2_NAME);
                msg +=" exists ";
                H5Gclose(s_root_id);
                throw InternalErr(__FILE__, __LINE__, msg);
        }
    }
    else if (0 == has_obpg_attr1) 
        ;// no op
    else {
        string msg = "Fail to determine if the HDF5 attribute  ";
        msg += string(Obpgl3_ATTR1_NAME);
        msg +=" exists ";
        H5Gclose(s_root_id);
        throw InternalErr(__FILE__, __LINE__, msg);
    }
    return ret_flag;
}
// Function to check if the product is ACOS Level 2 or OSMAPL2S.
bool check_osmapl2s_acosl2s_oco2l1b(hid_t s_root_id, int which_pro) {

    htri_t has_smac_group;
    bool return_flag = false;
    has_smac_group = H5Lexists(s_root_id,SMAC2S_META_GROUP_NAME,H5P_DEFAULT);

    if (has_smac_group >0){
        hid_t s_group_id = -1;

        // Open the group
        if ((s_group_id = H5Gopen(s_root_id, SMAC2S_META_GROUP_NAME,H5P_DEFAULT))<0) {
           string msg = "Cannot open the HDF5 Group  ";
           msg += string(SMAC2S_META_GROUP_NAME);
           H5Gclose(s_root_id);
           throw InternalErr(__FILE__, __LINE__, msg);
        }

        // OSMAPL2S 
        if (1 == which_pro) {
        
            htri_t has_osmapl2s_attr = -1;
            // OSMAPL2S will have an attribute called ProjectID
            has_osmapl2s_attr = H5Aexists(s_group_id,OSMAPL2S_ATTR_NAME);
            if (has_osmapl2s_attr >0) {
                string attr_value = "";
                obtain_gm_attr_value(s_group_id, OSMAPL2S_ATTR_NAME, attr_value);
                if (attr_value.compare(OSMAPL2S_ATTR_VALUE) == 0) 
                    return_flag = true;
                else 
                    return_flag = false;
                H5Gclose(s_group_id);
            }
            else if (0 == has_osmapl2s_attr) {
                H5Gclose(s_group_id);
                return_flag = false;
            }
            else {
                string msg = "Fail to determine if the HDF5 link  ";
                msg += string(OSMAPL2S_ATTR_NAME);
                msg +="  exists ";
                H5Gclose(s_group_id);
                H5Gclose(s_root_id);
                throw InternalErr(__FILE__, __LINE__, msg);
            }
        }
        else if (2 == which_pro) {

            htri_t has_acos_dset = -1;

            // ACOSL2S(OCO2L1B) will have a dataset called ProjectId
            has_acos_dset = H5Lexists(s_group_id,ACOS_L2S_OCO2_L1B_DSET_NAME,H5P_DEFAULT);
            if (has_acos_dset > 0) {
                // Obtain the dataset ID
                hid_t s_dset_id = -1;
                if ((s_dset_id = H5Dopen(s_group_id, ACOS_L2S_OCO2_L1B_DSET_NAME,H5P_DEFAULT)) < 0) {
                    string msg = "cannot open the HDF5 dataset  ";
                    msg += string(ACOS_L2S_OCO2_L1B_DSET_NAME);
                    H5Gclose(s_group_id);
                    H5Gclose(s_root_id);
                    throw InternalErr(__FILE__, __LINE__, msg);
                }

                // Obtain the datatype ID
                hid_t dtype = -1;
                if ((dtype = H5Dget_type(s_dset_id)) < 0) {
                    H5Dclose(s_dset_id);
                    H5Gclose(s_group_id);
                    H5Gclose(s_root_id);
                    string msg = "cannot get the datatype of HDF5 dataset  ";
                    msg += string(ACOS_L2S_OCO2_L1B_DSET_NAME);
                    throw InternalErr(__FILE__, __LINE__, msg);
                }

                // Obtain the datatype class 
                H5T_class_t ty_class = H5Tget_class(dtype);
                if (ty_class < 0) {
                    H5Tclose(dtype);
                    H5Dclose(s_dset_id);
                    H5Gclose(s_group_id);
                    H5Gclose(s_root_id);
                    string msg = "cannot get the datatype class of HDF5 dataset  ";
                    msg += string(ACOS_L2S_OCO2_L1B_DSET_NAME);
                    throw InternalErr(__FILE__, __LINE__, msg);
                }

                if (ty_class != H5T_STRING) {
                    H5Tclose(dtype);
		            H5Dclose(s_dset_id);
                    H5Gclose(s_group_id);
                    H5Gclose(s_root_id);
                    string msg = "This dataset must be a H5T_STRING class  ";
                    msg += string(ACOS_L2S_OCO2_L1B_DSET_NAME);
                    throw InternalErr(__FILE__, __LINE__, msg);
                }


                hid_t dspace = -1;
                if ((dspace = H5Dget_space(s_dset_id)) < 0) {
                    H5Tclose(dtype);
                    H5Dclose(s_dset_id);
                    H5Gclose(s_group_id);
                    H5Gclose(s_root_id);
                    string msg = "cannot get the the dataspace of HDF5 dataset  ";
                    msg += string(ACOS_L2S_OCO2_L1B_DSET_NAME);
                    throw InternalErr(__FILE__, __LINE__, msg);
                }

                hssize_t num_elem = 0;
                if ((num_elem = H5Sget_simple_extent_npoints(dspace))<=0) {
                    H5Tclose(dtype);
                    H5Sclose(dspace);
                    H5Dclose(s_dset_id);
                    H5Gclose(s_group_id);
                    H5Gclose(s_root_id);
                    string msg = "cannot get the the number of points of HDF5 dataset  ";
                    msg += string(ACOS_L2S_OCO2_L1B_DSET_NAME);
                    throw InternalErr(__FILE__, __LINE__, msg);
                }

                size_t dtype_size = H5Tget_size(dtype);
                if (dtype_size <= 0) {
                    H5Tclose(dtype);
                    H5Dclose(s_dset_id);
                    H5Sclose(dspace);
                    H5Gclose(s_group_id);
                    H5Gclose(s_root_id);
                    string msg = "cannot get the the dataspace of HDF5 dataset  ";
                    msg += string(ACOS_L2S_OCO2_L1B_DSET_NAME);
                    throw InternalErr(__FILE__, __LINE__, msg);
                }
 
                size_t total_data_size = num_elem * H5Tget_size(dtype);

                if (H5Tis_variable_str(dtype)) {

                    vector<char>temp_buf;
                    temp_buf.resize(total_data_size);

                    if (H5Dread(s_dset_id,dtype,H5S_ALL,H5S_ALL,H5P_DEFAULT, temp_buf.data())<0){
                        H5Tclose(dtype);
                        H5Dclose(s_dset_id);
                        H5Sclose(dspace);
                        H5Gclose(s_group_id);
                        H5Gclose(s_root_id);
                        string msg = "cannot get the the dataspace of HDF5 dataset  ";
                        msg += string(ACOS_L2S_OCO2_L1B_DSET_NAME);
                        throw InternalErr(__FILE__, __LINE__, msg);
                    }

                    char *temp_bp = temp_buf.data();
                    char *onestring = nullptr;
                    string total_string="";
                        
                    for (int temp_i = 0; temp_i <num_elem; temp_i++) {

                        // This line will assure that we get the real variable length string value.
                        onestring =*(char **)temp_bp;

                        // Change the C-style string to C++ STD string just for easy handling.
                        if (onestring !=nullptr) {
                            string tempstring(onestring);
                            total_string+=tempstring;
                        }
                        // going to the next value.
                        temp_bp += dtype_size;
                    }
                        
                    // Reclaim any VL memory if necessary.
                    herr_t ret_vlen_claim;
                    ret_vlen_claim = H5Dvlen_reclaim(dtype,dspace,H5P_DEFAULT,temp_buf.data());
                    if(ret_vlen_claim < 0) {
                        H5Sclose(dspace);
                        H5Tclose(dtype);
                        H5Dclose(s_dset_id);
                        H5Gclose(s_group_id);
                        throw InternalErr(__FILE__, __LINE__, "Cannot reclaim the memory buffer of the HDF5 variable length string.");
                    }
                    H5Sclose(dspace);
                    H5Tclose(dtype);
                    H5Dclose(s_dset_id);
                    H5Gclose(s_group_id);
 
                    if (total_string.compare(ACOS_L2S_ATTR_VALUE) ==0 ||
                        total_string.compare(OCO2_L1B_ATTR_VALUE) ==0 ||
                        total_string.compare(OCO2_L1B_ATTR_VALUE2)==0) 
                        return_flag = true;
                }
                else {
                    vector<char> temp_buf(total_data_size+1);
                    if (H5Dread(s_dset_id,dtype,H5S_ALL,H5S_ALL,H5P_DEFAULT, temp_buf.data())<0){
                           H5Tclose(dtype);
                           H5Dclose(s_dset_id);
                           H5Sclose(dspace);
                           H5Gclose(s_group_id);
                           H5Gclose(s_root_id);
                           string msg = "cannot data of HDF5 dataset  ";
                           msg += string(ACOS_L2S_OCO2_L1B_DSET_NAME);
                           throw InternalErr(__FILE__, __LINE__, msg);
                    }

                    string total_string(temp_buf.begin(),temp_buf.end()-1);
                    H5Sclose(dspace);
                    H5Tclose(dtype);
                    H5Dclose(s_dset_id);
                    H5Gclose(s_group_id);

                    if (0 == total_string.compare(ACOS_L2S_ATTR_VALUE) || 
                        0 == total_string.compare(OCO2_L1B_ATTR_VALUE)) 
                        return_flag = true;
                    else 
                        return_flag = false;
                } 
            }
            else if (0 == has_acos_dset) {
                H5Gclose(s_group_id);
                return_flag = false;
            }
            else {
                string msg = "Fail to determine if the HDF5 link  "; 
                msg += string(ACOS_L2S_OCO2_L1B_DSET_NAME);
                msg +="  exists ";
                H5Gclose(s_group_id);
                H5Gclose(s_root_id);
                throw InternalErr(__FILE__, __LINE__, msg);
            }
         }
         // Other product, don't do anything.
    }
    else if (0 == has_smac_group) 
        return_flag = false;
    else {
        string msg = "Fail to determine if the link  ";
        msg += string(SMAC2S_META_GROUP_NAME);
        msg +=" exists or not ";
        H5Gclose(s_root_id);
        throw InternalErr(__FILE__, __LINE__, msg);
    }
    return return_flag;
}
        
void obtain_gm_attr_value(hid_t s_root_id, const char* s_attr_name, string & s_attr_value) {

    hid_t s_attr_id = -1;
    if ((s_attr_id = H5Aopen_by_name(s_root_id,".",s_attr_name,
                     H5P_DEFAULT, H5P_DEFAULT)) <0) {
        string msg = "Cannot open the HDF5 attribute  ";
        msg += string(s_attr_name);
        H5Gclose(s_root_id);
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    hid_t attr_type = -1;
    if ((attr_type = H5Aget_type(s_attr_id)) < 0) {
        string msg = "cannot get the attribute datatype for the attribute  ";
        msg += string(s_attr_name);
        H5Aclose(s_attr_id);
        H5Gclose(s_root_id);
    }
               
    hid_t attr_space = -1;
    if ((attr_space = H5Aget_space(s_attr_id)) < 0) {
        string msg = "cannot get the hdf5 dataspace id for the attribute ";
        msg += string(s_attr_name);
        H5Tclose(attr_type);
        H5Aclose(s_attr_id);
        H5Gclose(s_root_id);
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    auto num_elm = (int)(H5Sget_simple_extent_npoints(attr_space));
    if (0 == num_elm) {
        string msg = "cannot get the number for the attribute ";
        msg += string(s_attr_name);
        H5Tclose(attr_type);
        H5Aclose(s_attr_id);
        H5Sclose(attr_space);
        H5Gclose(s_root_id);
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    size_t atype_size = H5Tget_size(attr_type);
    if (atype_size <= 0) {
        string msg = "cannot obtain the datatype size of the attribute ";
        msg += string(s_attr_name);
        H5Tclose(attr_type);
        H5Aclose(s_attr_id);
        H5Sclose(attr_space);
        H5Gclose(s_root_id);
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    if(H5Tis_variable_str(attr_type)) {
        
        vector<char> temp_buf;
        // Variable length string attribute values only store pointers of the actual string value.
        temp_buf.resize(atype_size*num_elm);
        if (H5Aread(s_attr_id, attr_type, temp_buf.data()) < 0) {
            string msg = "cannot retrieve the value of  the attribute ";
            msg += string(s_attr_name);
            H5Tclose(attr_type);
            H5Aclose(s_attr_id);
            H5Sclose(attr_space);
            H5Gclose(s_root_id);
            throw InternalErr(__FILE__, __LINE__, msg);
        }

        char *temp_bp;
        temp_bp = temp_buf.data();
        char* onestring;
        for (int temp_i = 0; temp_i <num_elm; temp_i++) {

            // This line will assure that we get the real variable length string value.
            onestring =*(char **)temp_bp;

            // Change the C-style string to C++ STD string just for easy appending the attributes in DAP.
            if (onestring !=nullptr) 
                string tempstring(onestring);
        }

    
        if (temp_buf.empty() != true) {

            // Reclaim any VL memory if necessary.
            herr_t ret_vlen_claim;
            ret_vlen_claim = H5Dvlen_reclaim(attr_type,attr_space,H5P_DEFAULT,temp_buf.data());
            if(ret_vlen_claim < 0){
                H5Tclose(attr_type);
                H5Aclose(s_attr_id);
                H5Sclose(attr_space);
                throw InternalErr(__FILE__, __LINE__, "Cannot reclaim the memory buffer of the HDF5 variable length string.");
            }
                 
            temp_buf.clear();
        }
    }
    else {
        vector<char> temp_buf(atype_size*num_elm+1);
        if (H5Aread(s_attr_id,attr_type, temp_buf.data())<0){
            string msg = "cannot retrieve the value of  the attribute ";
            msg += string(s_attr_name);
            H5Tclose(attr_type);
            H5Aclose(s_attr_id);
            H5Sclose(attr_space);
            H5Gclose(s_root_id);
            throw InternalErr(__FILE__, __LINE__, msg);

        }

        string temp_attr_value(temp_buf.begin(),temp_buf.end());
        size_t temp_null_pos = temp_attr_value.find_first_of('\0');
        s_attr_value = temp_attr_value.substr(0,temp_null_pos);
    }
    H5Tclose(attr_type);
    H5Sclose(attr_space);
    H5Aclose(s_attr_id);
    
}
