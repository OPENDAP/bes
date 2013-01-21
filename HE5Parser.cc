/////////////////////////////////////////////////////////////////////////////
//  This file is part of the hdf5 data handler for the OPeNDAP data server.
//
// Author:   Hyo-Kyung Lee <hyoklee@hdfgroup.org>
//
// Copyright (c) 2007-2012 The HDF Group, Inc. and OPeNDAP, Inc.
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

#include "HE5Parser.h"

using namespace std;

HE5Parser::HE5Parser()
{
    structure_state = -1;       // Za/Swath/Grid state 
    parser_state = 0;           // Parser's current state 
    err_msg = "";               // error message.
}

HE5Parser::~HE5Parser()
{
}


void HE5Parser::print()
{
    unsigned int i = 0;

    if(err_msg != ""){
        cerr << "Parse error:" << err_msg << endl;
    }
    cout << "ZA Size=" << za_list.size() << endl;
    for(i=0; i < za_list.size(); i++) {
        HE5Za z = za_list.at(i);
        cout << "ZA Name=" << z.name << endl;
        cout << "ZA Dim Size=" << z.dim_list.size() << endl;
        unsigned int j = 0;
        for(j=0; j < z.dim_list.size(); j++) {
            HE5Dim d = z.dim_list.at(j);
            cout << "ZA Dim Name=" << d.name;
            cout << " Size=" << d.size << endl;
        }

        cout << "ZA Var Size=" << z.data_var_list.size() 
             << endl;
        for(j=0; j < z.data_var_list.size(); j++) {
            HE5Var v = z.data_var_list.at(j);
            cout << "ZA Var Name=" << v.name << endl;
            cout << "ZA Var Dim Size=" << v.dim_list.size() << endl;
            unsigned int k = 0;
            for(k=0; k < v.dim_list.size(); k++) {
                HE5Dim d = v.dim_list.at(k);
                cout << "ZA Var Dim Name=" << d.name << endl;
            }
        }
    }

    cout << "Swath Size=" << swath_list.size() << endl;
    for(i=0; i < swath_list.size(); i++) {
        HE5Swath s = swath_list.at(i);
        cout << "Swath Name=" << s.name << endl;
        cout << "Swath Dim Size=" << s.dim_list.size() << endl;
        unsigned int j = 0;
        for(j=0; j < s.dim_list.size(); j++) {
            HE5Dim d = s.dim_list.at(j);
            cout << "Swath Dim Name=" << d.name;
            cout << " Size=" << d.size << endl;
        }

        cout << "Swath Geo Var Size=" << s.geo_var_list.size() 
             << endl;
        for(j=0; j < s.geo_var_list.size(); j++) {
            HE5Var v = s.geo_var_list.at(j);
            cout << "Swath Geo Var Name=" << v.name << endl;
            cout << "Swath Geo Var Dim Size=" << v.dim_list.size() << endl;
            unsigned int k = 0;
            for(k=0; k < v.dim_list.size(); k++) {
                HE5Dim d = v.dim_list.at(k);
                cout << "Swath Geo Var Dim Name=" << d.name;
                cout << " Size=" << d.size << endl;
            }
        }

        cout << "Swath Data Var Size=" << s.data_var_list.size() 
             << endl;
        for(j=0; j < s.data_var_list.size(); j++) {
            HE5Var v = s.data_var_list.at(j);
            cout << "Swath Data Var Name=" << v.name << endl;
            cout << "Swath Data Var Number Dim =" << v.dim_list.size() << endl;
            unsigned int k = 0;
            for(k=0; k < v.dim_list.size(); k++) {
                HE5Dim d = v.dim_list.at(k);
                cout << "Swath Data Var Dim Name=" << d.name << endl;
                cout <<"Swath Data Var Dim Size= "<< d.size<<endl;
            }
        }
    }

    cout << "Grid Size=" << grid_list.size() << endl;
    for(i=0; i < grid_list.size(); i++) {
        HE5Grid g = grid_list.at(i);
        cout << "Grid Name=" << g.name << endl;

        cout << "Grid point_lower=" << g.point_lower << endl;
        cout << "Grid point_upper=" << g.point_upper << endl;
        cout << "Grid point_left="  << g.point_left  << endl;
        cout << "Grid point_right=" << g.point_right << endl;

        cout << "Grid Dim Size=" << g.dim_list.size() << endl;
        unsigned int j = 0;
        for(j=0; j < g.dim_list.size(); j++) {
            HE5Dim d = g.dim_list.at(j);
            cout << "Grid Dim Name=" << d.name;
            cout << " Size=" << d.size << endl;
        }

        cout << "Grid Var Size=" << g.data_var_list.size() 
             << endl;
        for(j=0; j < g.data_var_list.size(); j++) {
            HE5Var v = g.data_var_list.at(j);
            cout << "Grid Var Name=" << v.name << endl;
            cout << "Grid Var Dim Size=" << v.dim_list.size() << endl;
            unsigned int k = 0;
            for(k=0; k < v.dim_list.size(); k++) {
                HE5Dim d = v.dim_list.at(k);
                cout << "Grid Var Dim Name=" << d.name << endl;
            }
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
    }
    
}
