/////////////////////////////////////////////////////////////////////////////
//  This file is part of the hdf5 data handler for the OPeNDAP data server.
//
// Author:   Hyo-Kyung Lee <hyoklee@hdfgroup.org>
//
// Copyright (c) 2007-2023 The HDF Group, Inc. and OPeNDAP, Inc.
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

#include "HE5Parser.h"
#include <libdap/InternalErr.h>
#include <HDF5CFUtil.h>
#include <stdlib.h>

using namespace std;

HE5Parser::HE5Parser() = default;

HE5Parser::~HE5Parser() = default;


void HE5Parser::print() const
{

    if(err_msg != ""){
        cerr<< "Parse error:" << err_msg << endl;
    }
    cout << "ZA Size=" << za_list.size() << endl;
    for (const auto &z:za_list) {
        cout << "ZA Name=" << z.name << endl;
        cout << "ZA Dim Size=" << z.dim_list.size() << endl;
        for (const auto &d:z.dim_list) {
            cout << "ZA Dim Name=" << d.name;
            cout << " Size=" << d.size << endl;
        }

        cout << "ZA Var Size=" << z.data_var_list.size() 
             << endl;
        for (const auto &v:z.data_var_list) {
            cout << "ZA Var Name=" << v.name << endl;
            cout << "ZA Var Dim Size=" << v.dim_list.size() << endl;
            for (const auto &d:v.dim_list) 
                cout << "ZA Var Dim Name=" << d.name << endl;
        }
    }

    cout << "Swath Size=" << swath_list.size() << endl;
    for (const auto &s:swath_list) {
        cout << "Swath Name=" << s.name << endl;
        cout << "Swath Dim Size=" << s.dim_list.size() << endl;
        for (const auto &d:s.dim_list) {
            cout << "Swath Dim Name=" << d.name;
            cout << " Size=" << d.size << endl;
        }

        cout << "Swath Geo Var Size=" << s.geo_var_list.size() 
             << endl;
        for (const auto &v:s.geo_var_list) {
            cout << "Swath Geo Var Name=" << v.name << endl;
            cout << "Swath Geo Var Dim Size=" << v.dim_list.size() << endl;
            for (const auto &d:v.dim_list) {
                cout << "Swath Geo Var Dim Name=" << d.name;
                cout << " Size=" << d.size << endl;
            }
        }

        cout << "Swath Data Var Size=" << s.data_var_list.size() 
             << endl;
        for (const auto &v:s.data_var_list) {
            cout << "Swath Data Var Name=" << v.name << endl;
            cout << "Swath Data Var Number Dim =" << v.dim_list.size() << endl;
            for (const auto &d:v.dim_list) {
                cout << "Swath Data Var Dim Name=" << d.name << endl;
                cout <<"Swath Data Var Dim Size= "<< d.size<<endl;
            }
            for (const auto &d:v.max_dim_list) {
                cout << "Swath Data Var Max Dim Name=" << d.name << endl;
                cout <<"Swath Data Var Dim Size= "<< d.size<<endl;
            }
        }
    }

    cout << "Grid Size=" << grid_list.size() << endl;
    for (const auto &g:grid_list) {
        cout << "Grid Name=" << g.name << endl;

        cout << "Grid point_lower=" << g.point_lower << endl;
        cout << "Grid point_upper=" << g.point_upper << endl;
        cout << "Grid point_left="  << g.point_left  << endl;
        cout << "Grid point_right=" << g.point_right << endl;
        cout << "Grid Sphere code =" <<g.sphere <<endl;

        cout << "Grid Dim Size=" << g.dim_list.size() << endl;
        for (const auto &d:g.dim_list) {
            cout << "Grid Dim Name=" << d.name;
            cout << " Size=" << d.size << endl;
        }

        cout << "Grid Var Size=" << g.data_var_list.size() 
             << endl;
        for(const auto &v:g.data_var_list) {
            cout << "Grid Var Name=" << v.name << endl;
            cout << "Grid Var Dim Size=" << v.dim_list.size() << endl;
            for (const auto &d:v.dim_list) 
                cout << "Grid Var Dim Name=" << d.name << endl;
#if 0
            for(k=0; k < v.max_dim_list.size(); k++) {
                HE5Dim d = v.max_dim_list.at(k);
                cout << "Grid Var Max Dim Name=" << d.name << endl;
            }
#endif
        }
        cout << "Grid pixelregistration=" << 
            g.pixelregistration
             << endl;
        cout << "Grid origin=" << 
            g.gridorigin
             << endl;
        cout << "Grid projection=" << 
            g.projection
             << endl;

        cout <<"Grid zone= "<< g.zone<<endl;
        cout <<"Grid sphere= "<<g.sphere<<endl;

        cout<<"Grid projection parameters are "<<endl;
        for(const auto &gp:g.param) 
            cout<<gp<<endl;
    }
    
}

void HE5Parser::add_projparams(const string & st_str) {

    string projparms = "ProjParams=(";
    char parms_end_marker = ')';
    size_t parms_spos = st_str.find(projparms);
    int grid_index = 0;
    while(parms_spos!=string::npos) {
        size_t parms_epos = st_str.find(parms_end_marker,parms_spos);
        if(parms_epos == string::npos) {
            string msg = "HDF-EOS5 Grid ProjParms syntax error: ProjParams doesn't end with ')'. ";
            throw libdap::InternalErr(__FILE__,__LINE__, msg);
        }
        string projparms_raw_values = st_str.substr(parms_spos+projparms.size(),parms_epos-parms_spos-projparms.size());
        vector<string> projparms_values;
        HDF5CFUtil::Split(projparms_raw_values.c_str(),',',projparms_values);

        for(unsigned int i = 0; i<projparms_values.size();i++) {
            grid_list[grid_index].param[i] = strtod(projparms_values[i].c_str(),nullptr);
        }
#if 0
for(vector<string>::iterator istr=projparms_values.begin();istr!=projparms_values.end();++istr)
cerr<<"projparms value is "<<*istr<<endl;
#endif
        parms_spos = st_str.find(projparms,parms_epos);
        grid_index++;
    }

}
