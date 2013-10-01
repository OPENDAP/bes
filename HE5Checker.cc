/////////////////////////////////////////////////////////////////////////////
//  This file is part of the hdf5 data handler for the OPeNDAP data server.
//
// Author:   Hyo-Kyung Lee <hyoklee@hdfgroup.org>
//           MuQun Yang <myang6@hdfgroup.org>
//
// Copyright (c) 2007-2013 The HDF Group, Inc. and OPeNDAP, Inc.
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
// You can contact The HDF Group, Inc. at 1800 South Oak Street,
// Suite 203, Champaign, IL 61820  
/// \file HE5Checker.cc
/// \brief A class for parsing NASA HDF-EOS5 StructMetadata.
///
/// This class contains functions that parse NASA HDF-EOS5 StructMetadata
/// and prepares the Vector structure that other functions reference.
///
/// \author Hyo-Kyung Lee <hyoklee@hdfgroup.org>
/// \author MuQun Yang <myang6@hdfgroup.org>
#include <map>
#include <math.h>
#include "HE5Checker.h"

using namespace std;


bool
HE5Checker::check_grids_missing_projcode(HE5Parser* p) 
{
    bool flag = false;
    for (unsigned int i = 0; i <p->grid_list.size(); i++) {
        HE5Grid g = p->grid_list.at(i);
        if (HE5_GCTP_MISSING == g.projection) {
            flag = true; 
            break;
        }
    }
    return flag;
}
    
bool
HE5Checker::check_grids_unknown_parameters(HE5Parser* p) 
{
    HE5Grid g;
    bool flag = false;
    for (unsigned int i = 0; i <p->grid_list.size(); i++) {
        g = p->grid_list.at(i);
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
HE5Checker::set_grids_missing_pixreg_orig(HE5Parser* p)
{
    unsigned int i = 0;
#if 0
 cerr << "HE5Checker::set_missing_values(Grid Size=" 
         << p->grid_list.size() << ")" << endl;
#endif
    for(i=0; i < p->grid_list.size(); i++) {
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
            cerr << "Grid projection is not set or the projection code is wrong. Name=" << g.name
                 << endl;
        }
#endif

        if (HE5_HDFE_MISSING == (p->grid_list)[i].pixelregistration) 
            (p->grid_list)[i].pixelregistration = HE5_HDFE_CENTER;

        if (HE5_HDFE_GD_MISSING == (p->grid_list)[i].gridorigin)
            (p->grid_list)[i].gridorigin = HE5_HDFE_GD_UL;
    
    }
}

bool 
HE5Checker::check_grids_multi_latlon_coord_vars(HE5Parser* p)
{

    // No need to check for the file that only has one grid or no grid.
    if (1 == p->grid_list.size() ||
        0 == p->grid_list.size() ) return false;

    unsigned int i = 0;
    // Store name size pair.
    typedef map<string, int> Dimmap;
    Dimmap dim_map;
    bool flag = false;

    for(i=0; i < p->grid_list.size(); i++) {
        HE5Grid g = p->grid_list.at(i);
        unsigned int j = 0;
        for(j=0; j < g.dim_list.size(); j++) {
            HE5Dim d = g.dim_list.at(j);
            Dimmap::iterator iter = dim_map.find(d.name);
            if(iter != dim_map.end()){
                if(d.size != iter->second){
#if 0
                    cerr << "Dimension size mismatch is found." 
                         << " Name=" << d.name 
                         << " Size=" << d.size << endl;
#endif 
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

   
    if (false == flag) {

        HE5Grid g = p->grid_list.at(0);
        EOS5GridPCType projcode = g.projection;
        EOS5GridPRType pixelreg = g.pixelregistration;      
        EOS5GridOriginType pixelorig =  g.gridorigin;

        float lowercoor = g.point_lower;
        float uppercoor = g.point_upper;
        float leftcoor =g.point_left;
        float rightcoor= g.point_right;



        for(i=1; i < p->grid_list.size(); i++) {
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

bool HE5Checker::check_grids_support_projcode(HE5Parser*p) {

    bool flag = false;
    for (unsigned int i = 0; i <p->grid_list.size(); i++) {
        HE5Grid g = p->grid_list.at(i);
        if (g.projection != HE5_GCTP_GEO) {
            flag = true;
            break;
        }
    }
    return flag;


}

