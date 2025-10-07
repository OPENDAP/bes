/////////////////////////////////////////////////////////////////////////////
//  This file is part of the hdf5 data handler for the OPeNDAP data server.
//
// Author:   Hyo-Kyung Lee <hyoklee@hdfgroup.org>
//           Kent Yang <myang6@hdfgroup.org>
//
// Copyright (c) 2007-2023 The HDF Group, Inc. and OPeNDAP, Inc.
///
/// All rights reserved.
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
/// \file HE5Checker.cc
/// \brief A class for parsing NASA HDF-EOS5 StructMetadata.
///
/// This class contains functions that parse NASA HDF-EOS5 StructMetadata
/// and prepares the Vector structure that other functions reference.
///
/// \author Hyo-Kyung Lee <hyoklee@hdfgroup.org>
/// \author Kent Yang <myang6@hdfgroup.org>
#include <map>
#include <math.h>
#include <BESDebug.h>
#include "HE5Checker.h"
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <BESInternalError.h>

using namespace std;

// We found some HDF-EOS5 files that have unlimited dimensions don't provide the current dimension size.
// So the current dimension size has to be retrieved through the variables.
void
HE5Checker::update_unlimited_dim_sizes(HE5Parser* p, hid_t fileid) 
{
    //Swath
    for (auto &g:p->swath_list) 
        update_unlimited_dim_sizes_internal(fileid, g.name,g.dim_list,g.data_var_list, g.geo_var_list);

    vector<HE5Var> geo_var_list;
    // Grid
    for (auto &g:p->grid_list) 
        update_unlimited_dim_sizes_internal(fileid, g.name,g.dim_list,g.data_var_list, geo_var_list);

    // ZA
    for (auto &g:p->za_list) 
        update_unlimited_dim_sizes_internal(fileid, g.name,g.dim_list,g.data_var_list, geo_var_list);

}

void HE5Checker::update_unlimited_dim_sizes_internal(hid_t fileid, const string& eos5_name, vector<HE5Dim>&dim_list, const vector<HE5Var>&data_var_list, const vector<HE5Var>&geo_var_list) {

    unordered_set<string> wrong_size_dim_names;
    unordered_map<string, string> wrong_size_dim_var_names;
    unordered_map<string, int> wrong_size_dim_name_to_var_dim_index;
    unordered_map<string, int> corrected_dim_name_size;
    
    // Identify the Unlimited dimension in both EOS5 object dimension list and the variable dimension list.
    for (const auto &md:dim_list) {
        if (md.name == "Unlimited") {
            for (const auto &v:data_var_list) {
                if (v.dim_list.size() == v.max_dim_list.size()) {
                    for (unsigned i = 0; i < v.dim_list.size(); i++) {
                        if ((v.max_dim_list[i]).name == "Unlimited") {
                            if (wrong_size_dim_names.find((v.dim_list[i]).name) ==wrong_size_dim_names.end()) {
                                wrong_size_dim_names.insert((v.dim_list[i]).name);
                                wrong_size_dim_var_names[(v.dim_list[i]).name] = v.name;
                                wrong_size_dim_name_to_var_dim_index[(v.dim_list[i]).name] = i;
                            }
                        }
                    }
                }
            }
            break;
        }
    }

    // Here we have to find the dimension size from the variable. This size is surely to be right.
    if (wrong_size_dim_names.empty() == false) {

        for (const auto &wdv:wrong_size_dim_var_names) {

            string var_name = wdv.second;
            string var_path = "/HDFEOS/SWATHS/" + eos5_name + "/Data Fields/" + var_name;
            int var_dim_index = -1;
            if (wrong_size_dim_name_to_var_dim_index.find(wdv.first) != wrong_size_dim_name_to_var_dim_index.end())
                var_dim_index = wrong_size_dim_name_to_var_dim_index[wdv.first];
            else  {
                string msg = "Cannot find the dimension index to search the dimension size for the variable " + var_path;
                BESInternalError(msg,__FILE__,__LINE__);
            }
            int correct_dim_size = obtain_correct_dim_size(fileid,var_path,var_dim_index);
            if (corrected_dim_name_size.find(wdv.first)==corrected_dim_name_size.end())
                corrected_dim_name_size[wdv.first] = correct_dim_size;
            else {
                string msg = "Cannot find the dimension name to correct the size for the variable " + var_path;
                BESInternalError(msg,__FILE__,__LINE__);
            }
        }

        for (auto &d:dim_list) {
            for (const auto& cdns: corrected_dim_name_size) {
                if (d.name == cdns.first) {
                    d.size = cdns.second;
                    break;
                }
            }
        }
    }

    // For swath, we have to search the geolocation fields.
    if(geo_var_list.empty() == false) {

        unordered_set<string> wrong_geo_size_dim_names;
        unordered_map<string, string> wrong_geo_size_dim_var_names;
        unordered_map<string, int> wrong_geo_size_dim_name_to_var_dim_index;
        unordered_map<string, int> corrected_geo_dim_name_size;
    
        for (const auto &md:dim_list) {
            if (md.name == "Unlimited") {
                for (const auto &v:geo_var_list) {
                    if (v.dim_list.size() == v.max_dim_list.size()) {
                        for (unsigned i = 0; i < v.dim_list.size(); i++) {
                            if ((v.max_dim_list[i]).name == "Unlimited") {
                                if (wrong_size_dim_names.find((v.dim_list[i]).name) ==wrong_size_dim_names.end() && wrong_geo_size_dim_names.find((v.dim_list[i]).name) ==wrong_geo_size_dim_names.end()) {
                                    wrong_geo_size_dim_names.insert((v.dim_list[i]).name);
                                    wrong_geo_size_dim_var_names[(v.dim_list[i]).name] = v.name;
                                    wrong_geo_size_dim_name_to_var_dim_index[(v.dim_list[i]).name] = i;
                                }
                            }
                        }
                    }
                }
                break;
            }
        }
        if (wrong_geo_size_dim_names.empty() == false) {
    
            for (const auto &wdv:wrong_geo_size_dim_var_names) {
                string var_name = wdv.second;
                string var_path = "/HDFEOS/SWATHS/" + eos5_name + "/Geolocation Fields/" + var_name;
                int var_dim_index = -1;
                if (wrong_geo_size_dim_name_to_var_dim_index.find(wdv.first) != wrong_geo_size_dim_name_to_var_dim_index.end())
                    var_dim_index = wrong_geo_size_dim_name_to_var_dim_index[wdv.first];
                else  {
                    string msg = "Cannot find the dimension index to search the dimension size for the variable " + var_path;
                    BESInternalError(msg,__FILE__,__LINE__);
                }
                int correct_dim_size = obtain_correct_dim_size(fileid,var_path,var_dim_index);
                if (corrected_geo_dim_name_size.find(wdv.first)==corrected_geo_dim_name_size.end())
                    corrected_geo_dim_name_size[wdv.first] = correct_dim_size;
                else {
                    string msg = "Cannot find the dimension name to correct the size for the variable " + var_path;
                    BESInternalError(msg,__FILE__,__LINE__);
                }
            }
    
            for (auto &d:dim_list) {
                for (const auto& cdns: corrected_geo_dim_name_size) {
                    if (d.name == cdns.first) {
                        d.size = cdns.second;
                        break;
                    }
                }
            }
        }
    }

}
 
int HE5Checker::obtain_correct_dim_size(hid_t file_id, const string &var_path, int var_dim_index) const{

    hid_t dset_id = H5Dopen2(file_id,var_path.c_str(),H5P_DEFAULT);
    if (dset_id <0) {
        string msg = "Cannot open the HDF5 dataset " + var_path + ".";
        BESInternalError(msg,__FILE__,__LINE__);
    }

    hid_t space_id = H5Dget_space(dset_id);
    if (space_id <0) {
        string msg = "Cannot get the space id of the HDF5 dataset " + var_path + ".";
        H5Dclose(dset_id);
        BESInternalError(msg,__FILE__,__LINE__);
    }

    int ndims = H5Sget_simple_extent_ndims(space_id);
    if (ndims < 0) {
        string msg = "cannot get the number of dimension for the HDF5 dataset " + var_path +".";
        H5Dclose(dset_id);
        H5Sclose(space_id);
        throw BESInternalError(msg,__FILE__, __LINE__);
    }

    if (var_dim_index >= ndims || var_dim_index <0) {
        string msg = "the requested dimension index is invalid for the HDF5 dataset " + var_path +".";
        H5Dclose(dset_id);
        H5Sclose(space_id);
        throw BESInternalError(msg,__FILE__, __LINE__);
    }
    vector<hsize_t> dim_sizes(ndims);
    if (H5Sget_simple_extent_dims(space_id, dim_sizes.data(),nullptr) <0) {
        string msg = "cannot get the number of dimension for the HDF5 dataset " + var_path +".";
        H5Dclose(dset_id);
        H5Sclose(space_id);
        throw BESInternalError(msg,__FILE__, __LINE__);
    }

    H5Sclose(space_id);
    H5Dclose(dset_id);
    return dim_sizes[var_dim_index];

}

bool
HE5Checker::check_grids_missing_projcode(const HE5Parser* p) const
{
    bool flag = false;
    for (const auto &g:p->grid_list) {
        if (HE5_GCTP_MISSING == g.projection) {
            flag = true; 
            break;
        }
    }
    return flag;
}
    
bool
HE5Checker::check_grids_unknown_parameters(const HE5Parser* p) const
{
    bool flag = false;
    for (const auto &g:p->grid_list) {
        if (HE5_GCTP_UNKNOWN == g.projection ||
            HE5_HDFE_UNKNOWN == g.pixelregistration ||
            HE5_HDFE_GD_UNKNOWN == g.gridorigin) {
            flag = true; 
            break;
        }
    }
    return flag;
}
 
void
HE5Checker::set_grids_missing_pixreg_orig(HE5Parser* p) const
{
    BESDEBUG("h5", "HE5Checker::set_missing_values(Grid Size=" 
         << p->grid_list.size() << ")" << endl);
    for(auto &g: p->grid_list) {
#if 0 
// Unnecessary 
        unsigned int j = 0;
        for(j=0; j < g.dim_list.size(); j++) {
            HE5Dim d = g.dim_list.at(j);
            cout << "Grid Dim Name=" << d.name;
            cout << " Size=" << d.size << endl;
        }
        for(j=0; j < g.data_var_list.size(); j++) {
            HE5Var v = g.data_var_list.at(j);
            unsigned int k = 0;
            for(k=0; k < v.dim_list.size(); k++) {
                HE5Dim d = v.dim_list.at(k);
                cout << "Grid Var Dim Name=" << d.name << endl;
            }
        }
        if(g.projection == -1){
            flag = true;
            "h5", "Grid projection is not set or the projection code is wrong. Name=" << g.name
                 << endl;
        }
#endif

        if (HE5_HDFE_MISSING == g.pixelregistration) 
            g.pixelregistration = HE5_HDFE_CENTER;

        if (HE5_HDFE_GD_MISSING == g.gridorigin)
            g.gridorigin = HE5_HDFE_GD_UL;
    
    }
}

bool 
HE5Checker::check_grids_multi_latlon_coord_vars(HE5Parser* p) const
{

    // No need to check for the file that only has one grid or no grid.
    if (1 == p->grid_list.size() ||
        p->grid_list.empty() ) return false;

    // Store name size pair.
    typedef map<string, int> Dimmap;
    Dimmap dim_map;
    bool flag = false;

    // Pick up the same dimension name with different sizes
    // This is not unusual since typically for grid since XDim and YDim are default for any EOS5 grid.
    for (const auto &g:p->grid_list) {
        for (const auto &d:g.dim_list) {
            Dimmap::iterator iter = dim_map.find(d.name);
            if(iter != dim_map.end()){
                if(d.size != iter->second){
                    flag = true;
                    break;
                }
            }
            else{
                dim_map[d.name] = d.size;
            }
        }
        if (true == flag) break;

    }

    // Even if the {name,size} is the same for different grids,
    // we still need to check their projection parameters to
    // make sure they are matched.  
    if (false == flag) {

        HE5Grid g = p->grid_list.at(0);
        EOS5GridPCType projcode = g.projection;
        EOS5GridPRType pixelreg = g.pixelregistration;      
        EOS5GridOriginType pixelorig =  g.gridorigin;

        auto lowercoor = (float)(g.point_lower);
        auto uppercoor = (float)(g.point_upper);
        auto leftcoor = (float)(g.point_left);
        auto rightcoor= (float)(g.point_right);

        for(unsigned int i=1; i < p->grid_list.size(); i++) {
            g = p->grid_list.at(i);
            if (projcode != g.projection ||
                pixelreg != g.pixelregistration   ||
                pixelorig!= g.gridorigin ||
                fabs(lowercoor-g.point_lower) >0.001 ||
                fabs(uppercoor-g.point_upper) >0.001 ||
                fabs(leftcoor-g.point_left) >0.001 ||
                fabs(rightcoor-g.point_right) >0.001 ){
                flag = true;
                break;
            }
        }
    }
      
    return flag;
}

bool HE5Checker::check_grids_support_projcode(const HE5Parser*p) const{

    bool flag = false;
    for (const auto &g:p->grid_list) {
        if (g.projection != HE5_GCTP_GEO && g.projection != HE5_GCTP_SNSOID
            && g.projection != HE5_GCTP_LAMAZ && g.projection != HE5_GCTP_PS) {
            flag = true;
            break;
        }
    }
    return flag;

}

