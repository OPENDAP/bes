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

/////////////////////////////////////////////////////////////////////////////
/// \file HDFEOS5CF.cc
/// \brief Implementation of the mapping of HDF-EOS5 products to DAP by following CF
///
///  It includes functions to 
///  1) retrieve HDF5 metadata info.
///  2) translate HDF5 objects into DAP DDS and DAS by following CF conventions.
///
///
/// \author Muqun Yang <myang6@hdfgroup.org>
///
/// Copyright (C) 2011 The HDF Group
///
/// All rights reserved.

#include "HDF5CF.h"
using namespace HDF5CF;

EOS5CVar::EOS5CVar(Var*var) {

    newname = var->newname;
    name = var->name;
    fullpath = var->fullpath;
    rank  = var->rank;
    dtype = var->dtype;
    unsupported_attr_dtype = var->unsupported_attr_dtype;
    unsupported_dspace = var->unsupported_dspace;

    for (vector<Attribute*>::iterator ira = var->attrs.begin();
        ira!=var->attrs.end(); ++ira) {
        Attribute* attr= new Attribute();
        attr->name = (*ira)->name;
        attr->newname = (*ira)->newname;
        attr->dtype =(*ira)->dtype;
        attr->count =(*ira)->count;
        attr->strsize = (*ira)->strsize;
        attr->fstrsize = (*ira)->fstrsize;
        attr->value =(*ira)->value;
        attrs.push_back(attr);
    }

    for (vector<Dimension*>::iterator ird = var->dims.begin();
        ird!=var->dims.end(); ++ird) {
        Dimension *dim = new Dimension((*ird)->size);
        dim->name = (*ird)->name;
        dim->newname = (*ird)->newname;
        dims.push_back(dim);
    }

}

//This method will effectively remove any dimnames like
// ???/XDim or ???/YDim from the dimension name set. 
// Use this function in caution.
void
EOS5CFGrid::Update_Dimnamelist() {


    // If I put both "XDim" and "YDim" into one for loop, Mac g++ compiler 
    // gives segmentation fault, which doesn't make sense.
    // I simply split them into two loops. It doesn't affect performance much.
    // KY 2012-2-14
    for (set<string>::iterator it = this->vardimnames.begin();
         it != this->vardimnames.end(); ++it) {
         string xydimname_candidate = HDF5CFUtil::obtain_string_after_lastslash(*it);
         if ("XDim" == xydimname_candidate) {
            this->vardimnames.erase(*it);
            break;
         }
    }

    for (set<string>::iterator it = this->vardimnames.begin();
         it != this->vardimnames.end(); ++it) {
        string xydimname_candidate = HDF5CFUtil::obtain_string_after_lastslash(*it);
        if ("YDim" == xydimname_candidate) {
            this->vardimnames.erase(*it);
            break;
        }
    }
 
}
         

EOS5File::~EOS5File()
{
    for (vector<EOS5CVar *>:: const_iterator i= this->cvars.begin(); i!=this->cvars.end(); ++i) 
        delete *i;
        
    for (vector<EOS5CFGrid *>:: const_iterator i= this->eos5cfgrids.begin(); i!=this->eos5cfgrids.end(); ++i) 
        delete *i;
    
    for (vector<EOS5CFSwath *>:: const_iterator i= this->eos5cfswaths.begin(); i!=this->eos5cfswaths.end(); ++i) 
        delete *i;
        
     for (vector<EOS5CFZa *>:: const_iterator i= this->eos5cfzas.begin(); i!=this->eos5cfzas.end(); ++i) 
        delete *i;
        
}

string EOS5File::get_CF_string(string s) {

    if (s[0] !='/') 
        return File::get_CF_string(s);
    else {
        s.erase(0,1);
        return File::get_CF_string(s);
    }
}

void EOS5File::Retrieve_H5_Info(const char *path,
                              hid_t file_id, bool include_attr) throw (Exception) {
    // Since we need to check the attribute info in order to determine if the file is augmented to netCDF-4,
    // we need to retrieve the attribute info also.
    File::Retrieve_H5_Info(path,file_id,true);
}

void EOS5File::Retrieve_H5_Supported_Attr_Values() throw (Exception) {

    File::Retrieve_H5_Supported_Attr_Values();
    for (vector<EOS5CVar *>::iterator ircv = this->cvars.begin();
          ircv != this->cvars.end(); ++ircv) {
        if ((CV_EXIST == (*ircv)->cvartype ) || (CV_MODIFY == (*ircv)->cvartype)){
            for (vector<Attribute *>::iterator ira = (*ircv)->attrs.begin();
                 ira != (*ircv)->attrs.end(); ++ira) 
                Retrieve_H5_Attr_Value(*ira,(*ircv)->fullpath);
                    
        }
    }
}

void EOS5File::Adjust_H5_Attr_Value(Attribute *attr) throw (Exception) {

}

void EOS5File:: Handle_Unsupported_Dtype(bool include_attr) throw(Exception) {

    File::Handle_Unsupported_Dtype(include_attr);
    for (vector<EOS5CVar *>::iterator ircv = this->cvars.begin();
                ircv != this->cvars.end(); ++ircv) {
        if (true == include_attr) {
            for (vector<Attribute *>::iterator ira = (*ircv)->attrs.begin();
                 ira != (*ircv)->attrs.end(); ++ira) {
                H5DataType temp_dtype = (*ira)->getType();
                if (false == HDF5CFUtil::cf_strict_support_type(temp_dtype)) {
                    delete (*ira);
                    (*ircv)->attrs.erase(ira);
                    ira--;
                }
            }
        }

        H5DataType temp_dtype = (*ircv)->getType();
        if (!HDF5CFUtil::cf_strict_support_type(temp_dtype)) {
            delete (*ircv);
            this->cvars.erase(ircv);
            ircv--;
        }

    }
}

void EOS5File:: Handle_Unsupported_Dspace() throw(Exception) {

    File::Handle_Unsupported_Dspace();
 
    if (true == this->unsupported_var_dspace) {
        for (vector<EOS5CVar *>::iterator ircv = this->cvars.begin();
                ircv != this->cvars.end(); ++ircv) {
            if (true == (*ircv)->unsupported_dspace) {
                delete (*ircv);
                this->cvars.erase(ircv);
                ircv--;
            }
        }
    }
}
void EOS5File::Adjust_EOS5Dim_Info(HE5Parser*strmeta_info) throw(Exception) {
    
    // Condense redundant XDim, YDim in the grid/swath/za dimension list
    
    for (unsigned int i = 0; i <strmeta_info->swath_list.size();++i) {
        HE5Swath& he5s = strmeta_info->swath_list.at(i);
        
        Adjust_EOS5Dim_List(he5s.dim_list);
        for (unsigned int j = 0; j<he5s.geo_var_list.size(); ++j) {
            Adjust_EOS5VarDim_Info((he5s.geo_var_list)[j].dim_list,he5s.dim_list);
        }
        for (unsigned int j = 0; j<he5s.data_var_list.size(); ++j) {
            Adjust_EOS5VarDim_Info((he5s.data_var_list)[j].dim_list,he5s.dim_list);
        }
    }

    for (unsigned int i = 0; i <strmeta_info->grid_list.size();++i) {

        HE5Grid& he5g = strmeta_info->grid_list.at(i);

        Adjust_EOS5Dim_List(he5g.dim_list);
        for (unsigned int j = 0; j<he5g.data_var_list.size(); ++j) {
            Adjust_EOS5VarDim_Info((he5g.data_var_list)[j].dim_list,he5g.dim_list);
        }
    }               

    for (unsigned int i = 0; i <strmeta_info->za_list.size();++i) {
        HE5Za& he5z = strmeta_info->za_list.at(i);
        
        Adjust_EOS5Dim_List(he5z.dim_list);
        for (unsigned int j = 0; j<he5z.data_var_list.size(); ++j) {
            Adjust_EOS5VarDim_Info((he5z.data_var_list)[j].dim_list,he5z.dim_list);
        }
    }               
}

void EOS5File::Adjust_EOS5Dim_List(vector<HE5Dim>& groupdimlist) throw(Exception){

    Remove_NegativeSizeDims(groupdimlist);

    // Condense redundant XDim, YDim in the grid/swath/za dimension list
    Condense_EOS5Dim_List(groupdimlist);

}

void EOS5File::Remove_NegativeSizeDims(vector<HE5Dim>& groupdimlist) throw(Exception){

    vector <HE5Dim>:: iterator id;
    // We find one product has dimension with name:  Unlimited, size: -1; this dimension
    // will not be used by any variables. The "Unlimited" dimension is useful for extended
    // datasets when data is written. It is not useful for data accessing as far as I know.
    // So we will remove it from the list. 
    // This algoritm will also remove  any dimension with size <=0. KY 2011-1-14
    for (id = groupdimlist.begin(); id != groupdimlist.end(); ++id) {
        if ((*id).size <= 0) {
            groupdimlist.erase(id);
            id --;
        }
    }
}

void EOS5File::Condense_EOS5Dim_List(vector<HE5Dim>& groupdimlist) throw(Exception){

    set<int>xdimsizes;
    set<int>ydimsizes;
    pair<set<int>::iterator,bool> setret;
    vector <HE5Dim>:: iterator id;

    for (id = groupdimlist.begin(); id != groupdimlist.end(); ++id) {
        if ("XDim" == (*id).name || "Xdim" == (*id).name) {
            setret = xdimsizes.insert((*id).size);
            if (false == setret.second) {
                groupdimlist.erase(id);
                id--;
            }
            else if ("Xdim" == (*id).name) (*id).name = "XDim";
                
        }
    }
  
    for (id = groupdimlist.begin(); id != groupdimlist.end(); ++id) {
        if ("YDim" == (*id).name || "Ydim" ==(*id).name) {
            setret = ydimsizes.insert((*id).size);
            if (false == setret.second) {
                groupdimlist.erase(id);
                id--;
            }
            else if ("Ydim" == (*id).name) 
                (*id).name = "YDim";
        }
    }
}
        

void EOS5File::Adjust_EOS5VarDim_Info(vector<HE5Dim>& vardimlist, vector<HE5Dim>& groupdimlist) throw(Exception){

    set<string> dimnamelist;
    pair<set<string>::iterator,bool> setret;
    
    // For EOS5 Grids: Dimension names XDim and YDim are predefined. 
    // Even the data producers make a mistake to define "xdim", "ydim" etc in the grid
    // dimension name list, the variable will still pick up "XDim" and "YDim" as their
    // dimension names So we assume that 'xdim", "ydim" etc will never appear in the
    // variable name list.  
    for (unsigned int i = 0; i <vardimlist.size(); ++i) {

        HE5Dim& he5d = vardimlist.at(i);
        bool dim_in_groupdimlist = false;
        for (unsigned int j = 0; j <groupdimlist.size();++j) {
            HE5Dim he5gd = groupdimlist.at(j);
            if (he5gd.name == he5d.name) {
                he5d.size = he5gd.size;
                dim_in_groupdimlist = true;
                break;
            }
        }

        if (false == dim_in_groupdimlist) 
            throw2("The EOS5 group dimension name list doesn't include the dimension ",he5d.name);

        // Some variables have data like float foo[nlevel= 10][nlevel= 10],need to make the dimname unique
        // to ensure the coordinate variables to be generated correctly.
        // 
        setret = dimnamelist.insert(he5d.name);
        if (false == setret.second) {
            int clash_index =1;
            string temp_clashname=he5d.name+'_';
            HDF5CFUtil::gen_unique_name(temp_clashname,dimnamelist,clash_index);
            he5d.name = temp_clashname;
            // We have to add this dim. to this  dim. list if this dim doesn't exist in the dim. list.
            bool dim_exist = false;
            for (unsigned int j = 0; j <groupdimlist.size();++j) {
                if (he5d.name == groupdimlist[j].name && 
                    he5d.size == groupdimlist[j].size ) {
                    dim_exist = true;
                    break;
                }
            }
                    
            if (false == dim_exist) 
                groupdimlist.push_back(he5d);

            // cerr<<"groupdimlist size= "<<groupdimlist.size() <<endl;
        }//if(false == setret.second)
    }// for (unsigned int i = 0; i <vardimlist.size(); ++i)

}
                
void EOS5File::Add_EOS5File_Info(HE5Parser * strmeta_info, bool grids_mllcv) throw(Exception) {

   string fslash_str="/";
   string grid_str ="/GRIDS/";
   string swath_str="/SWATHS/";
   string za_str = "/ZAS/";
   // cerr<<"coming to Add_EOSFile_Info "<<endl;

   // Assign the original number of grids. These number will be useful
   // to generate the final DAP object names for grids/swaths/zas that don't have coordinate 
   // variables. For example, OMI level 2G product has latitude and longitude with 3-D arrays.
   // There is no way to make the lat/lon become CF coordinate variables.  To still follow the
   // HDF-EOS5 object name conventions, the original number of grid is expected. 
   // Since this happens only for grids, we just keep the original number for grids now.
   this->orig_num_grids = strmeta_info->grid_list.size();

   // 
   for(unsigned int i=0; i < strmeta_info->grid_list.size(); i++) {
        HE5Grid he5g = strmeta_info->grid_list.at(i);
        EOS5CFGrid * eos5grid = new EOS5CFGrid();
        eos5grid->name = he5g.name;
        eos5grid->dimnames.resize(he5g.dim_list.size());
        
        for (unsigned int j=0; j <he5g.dim_list.size(); j++) {

            HE5Dim he5d = he5g.dim_list.at(j);
            if ("XDim" == he5d.name) 
                eos5grid->xdimsize = he5d.size;
            if ("YDim" == he5d.name) 
                eos5grid->ydimsize = he5d.size;

            // Here we add the grid name connecting with "/" to
            // adjust the dim names to assure the uniqueness of
            // the dimension names for multiple grids.
            // For single grid, we don't have to do that.
            // However, considering the rare case that one
            // can have one grid, one swath and one za, the dimnames
            // without using the group names may cause the name clashings.
            // so still add the group path.
            string unique_dimname=
                grid_str +he5g.name + fslash_str + he5d.name;

            (eos5grid->dimnames)[j] = unique_dimname;

            pair<map<hsize_t,string>::iterator,bool>mapret1;
            mapret1 = eos5grid->dimsizes_to_dimnames.insert(pair<hsize_t,string>((hsize_t)he5d.size,unique_dimname));

            // Create the dimname to dimsize map. This will be used to create the missing coordinate
            // variables. Based on our understanding, dimension names should be unique for 
            // grid/swath/zonal average. We will throw an error if we find the same dimension name used.
            pair<map<string,hsize_t>::iterator,bool>mapret2;
            mapret2 = eos5grid->dimnames_to_dimsizes.insert(pair<string,hsize_t>(unique_dimname,(hsize_t)he5d.size));
           if (false == mapret2.second) 
                throw5("The dimension name ",unique_dimname, " with the dimension size ", he5d.size, "is not unique");
        } // for (int j=0; j <he5g.dim_list.size(); j++)

        // Check if having  Latitude/Longitude.
        EOS5SwathGrid_Set_LatLon_Flags(eos5grid,he5g.data_var_list);


        // Using map for possible the third-D CVs.
        map<string,string> dnames_to_1dvnames;
        EOS5Handle_nonlatlon_dimcvars(he5g.data_var_list,GRID,he5g.name,dnames_to_1dvnames);
        eos5grid->dnames_to_1dvnames = dnames_to_1dvnames;
        eos5grid->point_lower = he5g.point_lower;
        eos5grid->point_upper = he5g.point_upper;
        eos5grid->point_left = he5g.point_left;
        eos5grid->point_right = he5g.point_right;
         
        eos5grid->eos5_pixelreg = he5g.pixelregistration;
        eos5grid->eos5_origin    = he5g.gridorigin;
        eos5grid->eos5_projcode    = he5g.projection;
        this->eos5cfgrids.push_back(eos5grid);

   }// for(int i=0; i < strmeta_info->grid_list.size(); i++) 

   this->grids_multi_latloncvs = grids_mllcv;

   // Second Swath
   for (unsigned int i=0; i < strmeta_info->swath_list.size(); i++) {

        HE5Swath he5s = strmeta_info->swath_list.at(i);
        EOS5CFSwath * eos5swath = new EOS5CFSwath();
        eos5swath->name = he5s.name;
        eos5swath->dimnames.resize(he5s.dim_list.size());
        
        for (unsigned int j=0; j <he5s.dim_list.size(); j++) {

            HE5Dim he5d = he5s.dim_list.at(j);

            // Here we add the swath name connecting with "/" to
            // adjust the dim names to assure the uniqueness of
            // the dimension names for multiple swaths.
            // For single swath, we don't have to do that.
            // However, considering the rare case that one
            // can have one grid, one swath and one za, the dimnames
            // without using the group names may cause the name clashings.
            // so still add the group path.
            string unique_dimname=
                swath_str + he5s.name + fslash_str + he5d.name;
            (eos5swath->dimnames)[j] = unique_dimname;

            // Create the dimsize to dimname map for those variables missing dimension names.
            // Note: For different dimnames sharing the same dimsizes, we only pick up the first one.
            pair<map<hsize_t,string>::iterator,bool>mapret1;
            mapret1 = eos5swath->dimsizes_to_dimnames.insert(pair<hsize_t,string>((hsize_t)he5d.size,unique_dimname));

            // Create the dimname to dimsize map. This will be used to create the missing coordinate
            // variables. Based on our understanding, dimension names should be unique for 
            // grid/swath/zonal average. We will throw an error if we find the same dimension name used.
            pair<map<string,hsize_t>::iterator,bool>mapret2;
            mapret2 = eos5swath->dimnames_to_dimsizes.insert(pair<string,hsize_t>(unique_dimname,(hsize_t)he5d.size));
            if (false == mapret2.second) 
                throw5("The dimension name ",unique_dimname, " with the dimension size ", he5d.size, "is not unique");
           
        } // for (int j=0; j <he5s.dim_list.size(); j++)

        // Check if having  Latitude/Longitude.
        EOS5SwathGrid_Set_LatLon_Flags(eos5swath,he5s.geo_var_list);

        // Using map for possible the third-D CVs.
        map<string,string> dnames_to_geo1dvnames;
        EOS5Handle_nonlatlon_dimcvars(he5s.geo_var_list,SWATH,he5s.name,dnames_to_geo1dvnames);
        eos5swath->dnames_to_geo1dvnames = dnames_to_geo1dvnames;
        this->eos5cfswaths.push_back(eos5swath);
   } // for (int i=0; i < strmeta_info->swath_list.size(); i++)

   // Third Zonal average
   for(unsigned int i=0; i < strmeta_info->za_list.size(); i++) {

        HE5Za he5z = strmeta_info->za_list.at(i);

        EOS5CFZa * eos5za = new EOS5CFZa();
        eos5za->name = he5z.name;
        eos5za->dimnames.resize(he5z.dim_list.size());

        for (unsigned int j=0; j <he5z.dim_list.size(); j++) {

            HE5Dim he5d = he5z.dim_list.at(j);

            // Here we add the grid name connecting with "/" to
            // adjust the dim names to assure the uniqueness of
            // the dimension names for multiple grids.
            // For single grid, we don't have to do that.
            string unique_dimname =
                za_str + he5z.name + fslash_str + he5d.name;
            (eos5za->dimnames)[j] = unique_dimname;
            pair<map<hsize_t,string>::iterator,bool>mapret1;
            mapret1 = eos5za->dimsizes_to_dimnames.insert(pair<hsize_t,string>((hsize_t)he5d.size,unique_dimname));

            // Create the dimname to dimsize map. This will be used to create the missing coordinate
            // variables. Based on our understanding, dimension names should be unique for 
            // grid/swath/zonal average. We will throw an error if we find the same dimension name used.
            pair<map<string,hsize_t>::iterator,bool>mapret2;
            mapret2 = eos5za->dimnames_to_dimsizes.insert(pair<string,hsize_t>(unique_dimname,(hsize_t)he5d.size));
            if (false == mapret2.second) 
                throw5("The dimension name ",unique_dimname, " with the dimension size ", he5d.size, "is not unique");

        } // for (int j=0; j <he5z.dim_list.size(); j++) 

        // Using map for possible the third-D CVs.
        map<string,string> dnames_to_1dvnames;
        EOS5Handle_nonlatlon_dimcvars(he5z.data_var_list,ZA,he5z.name,dnames_to_1dvnames);
        eos5za->dnames_to_1dvnames = dnames_to_1dvnames;
        this->eos5cfzas.push_back(eos5za);
   } // for(int i=0; i < strmeta_info->za_list.size(); i++) 

// Debugging info
#if 0
for (vector<EOS5CFGrid *>::iterator irg = this->eos5cfgrids.begin();
                irg != this->eos5cfgrids.end(); ++irg) {

cerr<<"grid name "<<(*irg)->name <<endl;
cerr<<"eos5_pixelreg"<<(*irg)->eos5_pixelreg <<endl;
cerr<<"eos5_origin"<<(*irg)->eos5_pixelreg <<endl;
cerr<<"point_lower "<<(*irg)->point_lower <<endl;
cerr<<"xdimsize "<<(*irg)->xdimsize <<endl;

if((*irg)->has_g2dlatlon) cerr<<"has g2dlatlon"<<endl;
if((*irg)->has_2dlatlon) cerr<<"has 2dlatlon"<<endl;
if((*irg)->has_1dlatlon) cerr<<"has 1dlatlon"<<endl;
if((*irg)->has_nolatlon) cerr<<"has no latlon" <<endl;
if(this->grids_multi_latloncvs) cerr<<"having multiple lat/lon from structmeta" <<endl;
else cerr<<"no multiple lat/lon from structmeta" <<endl;


// Dimension names
cerr <<"number of dimensions "<<(*irg)->dimnames.size() <<endl;
for (vector<string>::iterator irv = (*irg)->dimnames.begin();
                irv != (*irg)->dimnames.end(); ++irv) 
     cerr<<"dim names" <<*irv <<endl;

// mapping size to name
for (map<hsize_t,string>::iterator im1 = (*irg)->dimsizes_to_dimnames.begin();
        im1 !=(*irg)->dimsizes_to_dimnames.end();++im1) {
     cerr<<"size to name "<< (int)((*im1).first) <<"=> "<<(*im1).second <<endl;
}
   
// mapping dime names to 1d varname
for (map<string,string>::iterator im2 = (*irg)->dnames_to_1dvnames.begin();
        im2 !=(*irg)->dnames_to_1dvnames.end();++im2) {
cerr<<"dimanme to 1d var name "<< (*im2).first <<"=> "<<(*im2).second <<endl;
}
}

//Swath
for (vector<EOS5CFSwath *>::iterator irg = this->eos5cfswaths.begin();
                irg != this->eos5cfswaths.end(); ++irg) {

cerr<<"swath name "<<(*irg)->name <<endl;
if((*irg)->has_nolatlon) cerr<<"has no latlon" <<endl;
if((*irg)->has_1dlatlon) cerr<<"has 1dlatlon"<<endl;
if((*irg)->has_2dlatlon) cerr<<"has 2dlatlon"<<endl;

// Dimension names
for (vector<string>::iterator irv = (*irg)->dimnames.begin();
                irv != (*irg)->dimnames.end(); ++irv) 
cerr<<"dim names" <<*irv <<endl;

// mapping size to name
for (map<hsize_t,string>::iterator im1 = (*irg)->dimsizes_to_dimnames.begin();
        im1 !=(*irg)->dimsizes_to_dimnames.end();++im1) {
cerr<<"size to name "<< (int)((*im1).first) <<"=> "<<(*im1).second <<endl;
}
   
// mapping dime names to 1d varname
for (map<string,string>::iterator im2 = (*irg)->dnames_to_geo1dvnames.begin();
        im2 !=(*irg)->dnames_to_geo1dvnames.end();++im2) {
cerr<<"dimname to 1d varname "<< (*im2).first <<"=> "<<(*im2).second <<endl;
}
}

for (vector<EOS5CFZa *>::iterator irg = this->eos5cfzas.begin();
                irg != this->eos5cfzas.end(); ++irg) {

cerr<<"za name now"<<(*irg)->name <<endl;

// Dimension names
for (vector<string>::iterator irv = (*irg)->dimnames.begin();
                irv != (*irg)->dimnames.end(); ++irv) 
cerr<<"dim names" <<*irv <<endl;

// mapping size to name
for (map<hsize_t,string>::iterator im1 = (*irg)->dimsizes_to_dimnames.begin();
        im1 !=(*irg)->dimsizes_to_dimnames.end();++im1) {
cerr<<"size to name "<< (int)((*im1).first) <<"=> "<<(*im1).second <<endl;
}
   
// mapping dime names to 1d varname
for (map<string,string>::iterator im2 = (*irg)->dnames_to_1dvnames.begin();
        im2 !=(*irg)->dnames_to_1dvnames.end();++im2) {
cerr<<"dimname to 1d varname "<< (*im2).first <<"=> "<<(*im2).second <<endl;
}
}
#endif 

}

template<class T> 
void EOS5File::EOS5SwathGrid_Set_LatLon_Flags(T* eos5gridswath,vector<HE5Var> &eos5varlist) throw (Exception){

    bool find_lat = false;
    bool find_lon = false;
    bool has_1dlat = false;
    bool has_1dlon = false;
    bool has_2dlat = false;
    string lat_xdimname;
    string lat_ydimname;
    string lon_xdimname; 
    string lon_ydimname;
    bool has_2dlon = false;
    bool has_g2dlat = false;
    bool has_g2dlon = false;

    for (unsigned int i = 0; i < eos5varlist.size(); ++i) {
        HE5Var he5v = eos5varlist.at(i);
        if ("Latitude" == he5v.name) { 
            find_lat = true;
            int num_dims = he5v.dim_list.size();
            if ( 1 == num_dims) 
                has_1dlat = true;
            else if (2 == num_dims) {
                  lat_ydimname = (he5v.dim_list)[0].name;
                  lat_xdimname = (he5v.dim_list)[1].name;
                  has_2dlat = true;
            }
            else if (num_dims >2) 
                has_g2dlat = true;
            else 
                throw1("The number of dimension should not be 0 for grids or swaths");
        }// if ("Latitude" == he5v.name)
        
        if ("Longitude" == he5v.name) {
            find_lon = true;
            int num_dims =  he5v.dim_list.size();
            if ( 1 == num_dims) 
                has_1dlon = true;
            else if (2 == num_dims) {
                lon_ydimname = (he5v.dim_list)[0].name;
                lon_xdimname = (he5v.dim_list)[1].name;
                has_2dlon = true;
            }
            else if (num_dims >2) 
                has_g2dlon = true;
            else 
                throw1("The number of dimension should not be 0 for grids or swaths");
        } // if ("Longitude" == he5v.name)

        if (true == find_lat && true == find_lon) {
            if (true == has_1dlat && true == has_1dlon) 
                eos5gridswath->has_1dlatlon = true;

            // Make sure we have lat[YDIM][XDIM] and lon[YDIM][XDIM]
            if (true == has_2dlat && true == has_2dlon && 
                lat_ydimname == lon_ydimname &&
                lat_xdimname == lon_xdimname) 
                eos5gridswath->has_2dlatlon = true;
                
            if (true == has_g2dlat && true == has_g2dlon) 
                eos5gridswath->has_g2dlatlon = true;

            eos5gridswath->has_nolatlon = false;
            break;
        } // if (true == find_lat && true == find_lon) 
    }// for (unsigned int i = 0; i < eos5varlist.size(); ++i)
}
            

void EOS5File::EOS5Handle_nonlatlon_dimcvars(vector<HE5Var> & eos5varlist,
                                             EOS5Type eos5type, 
                                             string groupname,
                                             map<string,string>& dnamesgeo1dvnames) throw(Exception){

    set<string> nocvdimnames;
    string grid_str = "/GRIDS/";
    string xdim_str ="XDim";
    string ydim_str ="YDim";
    string fslash_str ="/";
    string eos5typestr;

    if (GRID==eos5type) {
        string xdimname = grid_str + groupname + fslash_str + xdim_str;
        nocvdimnames.insert(xdimname);
        string ydimname = grid_str + groupname + fslash_str + ydim_str;
        nocvdimnames.insert(ydimname);
        eos5typestr = "/GRIDS/";
    }
    else if (SWATH == eos5type) 
        eos5typestr = "/SWATHS/";
    else if (ZA    == eos5type) 
        eos5typestr = "/ZAS/";
    else 
        throw1("Unsupported HDF-EOS5 type, this type is not swath, grid or zonal average");

    pair<map<string,string>::iterator,bool>mapret;
    for(unsigned int i=0; i < eos5varlist.size(); ++i) {
        HE5Var he5v = eos5varlist.at(i);
        if (1 == he5v.dim_list.size()) {
            HE5Dim he5d = he5v.dim_list.at(0);
            string dimname;
            dimname =  eos5typestr  + groupname + fslash_str + he5d.name;
            string varname; // using the new var name format
            varname =  eos5typestr  + groupname + fslash_str + he5v.name;
            mapret = dnamesgeo1dvnames.insert(pair<string,string>(dimname,varname));

            // If another geo field already shares the same dimname, we need to
            // disqualify this geofield as the coordinate variable since it is not
            // unique anymore.
            if (false == mapret.second ) 
                nocvdimnames.insert(dimname);
        }
    }

    // Manage the coordinate variables. We only want to leave fields that uniquely hold
    // the dimension name to be the possible cv candidate.
    set<string>:: iterator itset;
    for ( itset = nocvdimnames.begin(); itset!=nocvdimnames.end();++itset) 
        dnamesgeo1dvnames.erase(*itset);
}

void EOS5File::Adjust_Var_NewName_After_Parsing() throw(Exception) {

    for (vector<Var *>::iterator irv = this->vars.begin();
                irv != this->vars.end(); ++irv) {
        Obtain_Var_NewName(*irv);
    }
}

void EOS5File::Obtain_Var_NewName(Var *var) throw(Exception) {

    string fslash_str="/";
    string eos5typestr="";

    EOS5Type vartype = Get_Var_EOS5_Type(var);

    // Actually the newname is used to check if the we have the existing
    // third dimension coordinate variable. To avoid the check of
    // fullpath again. We will make newname to have the path and remove
     // the grid and grid name before passing to DDS downstream. KY 2012-1-20
    switch (vartype) {
        case GRID:  
        {
            eos5typestr = "/GRIDS/";
            string eos5_groupname = Obtain_Var_EOS5Type_GroupName(var,vartype); 
//            var->newname = ((1 == num_grids)?var->name:
//                          eos5typestr + eos5_groupname + fslash_str + var->name);
              var->newname = eos5typestr + eos5_groupname + fslash_str + var->name;
        }
        break;
        
        case SWATH:  
        {
            eos5typestr = "/SWATHS/";
            string eos5_groupname = Obtain_Var_EOS5Type_GroupName(var,vartype); 
//            var->newname = ((1 == num_swaths)?var->name:
  //                         eos5typestr + eos5_groupname + fslash_str + var->name);
              var->newname = eos5typestr + eos5_groupname + fslash_str + var->name;
        }
        break;
        case ZA:  
        {
            eos5typestr = "/ZAS/";
            string eos5_groupname = Obtain_Var_EOS5Type_GroupName(var,vartype); 
   //         var->newname = ((1 == num_zas)?var->name:
    //                       eos5typestr + eos5_groupname + fslash_str + var->name);
              var->newname = eos5typestr + eos5_groupname + fslash_str + var->name;
        }
        break;
        case OTHERVARS: {
            string eos5infopath = "/HDFEOS INFORMATION";
            if (var->fullpath.size() > eos5infopath.size()){
                if (eos5infopath == var->fullpath.substr(0,eos5infopath.size()))
                    var->newname = var->name;
            }
            else var->newname = var->fullpath;
        }
        break;
        default:
            throw1("Non-supported EOS type");
    } // switch(vartype)
}

EOS5Type EOS5File::Get_Var_EOS5_Type(Var* var) throw(Exception) {

    string EOS5GRIDPATH ="/HDFEOS/GRIDS";
    string EOS5SWATHPATH ="/HDFEOS/SWATHS";
    string EOS5ZAPATH ="/HDFEOS/ZAS";

    if (var->fullpath.size() >= EOS5GRIDPATH.size()) {
        if (EOS5GRIDPATH == 
            var->fullpath.substr(0,EOS5GRIDPATH.size())) 
            return GRID; 
    }
    if (var->fullpath.size() >=EOS5SWATHPATH.size()) {
        if (EOS5SWATHPATH ==
            var->fullpath.substr(0,EOS5SWATHPATH.size())) 
            return SWATH;
    }
    if (var->fullpath.size() >=EOS5ZAPATH.size()) {
        if (EOS5ZAPATH ==
            var->fullpath.substr(0,EOS5ZAPATH.size())) 
            return ZA;
    }
    return OTHERVARS;

}


void EOS5File::Add_Dim_Name( HE5Parser *strmeta_info) throw(Exception){

    for (vector<Var *>::iterator irv = this->vars.begin();
                irv != this->vars.end(); ++irv) {
        Obtain_Var_Dims(*irv,strmeta_info); 
#if 0
for (vector<Dimension *>::iterator ird = (*irv)->dims.begin();
    ird != (*irv)->dims.end();++ird) {
cerr<<"dim name right after change "<<(*ird)->newname <<endl;
}
#endif
            
    }
}

// CHECK if finding the same variables from the parser.
bool EOS5File::Obtain_Var_Dims(Var *var,HE5Parser * strmeta_info) throw(Exception){

    string varname_from_parser ="";
    EOS5Type vartype = Get_Var_EOS5_Type(var);

    if (GRID == vartype) {
        int num_grids = strmeta_info->grid_list.size();
        for (int i = 0; i < num_grids; ++i) {   
            HE5Grid he5g = strmeta_info->grid_list.at(i);
            if (he5g.name == Obtain_Var_EOS5Type_GroupName(var,vartype)){  
                EOS5CFGrid *eos5cfgrid = (this->eos5cfgrids)[i];
                bool var_is_parsed = Set_Var_Dims(eos5cfgrid,var,he5g.data_var_list,he5g.name,num_grids,GRID);
                if (false == var_is_parsed){
                    map<hsize_t,string> dimsizes_to_dimnames = eos5cfgrid->dimsizes_to_dimnames;
                    // Check if this swath includes data fields(variables) that don't have any dimension names.
                    // This rarely happens. But we do find one NASA Aura product that has this problem. Although
                    // this has been fixed, we should anticipiate that the similar problem may happen in the future. 
                    // So check here to avoid the potential problems. KY 2012-1-9
                    Set_NonParse_Var_Dims(eos5cfgrid,var,dimsizes_to_dimnames,num_grids,vartype);
                }
            }
        }// for (unsigned int i = 0; i < num_grids; ++i)
    }// if (GRID == vartype)
    else if (SWATH == vartype) {
        int num_swaths = strmeta_info->swath_list.size();
        for (int i = 0; i < num_swaths; ++i) {

            HE5Swath he5s = strmeta_info->swath_list.at(i);

            if (he5s.name == Obtain_Var_EOS5Type_GroupName(var,vartype)){

                EOS5CFSwath *eos5cfswath = (this->eos5cfswaths)[i];

                bool var_is_parsed = true;
                int swath_fieldtype_flag = Check_EOS5Swath_FieldType(var);
                if (1 == swath_fieldtype_flag) 
                    var_is_parsed = Set_Var_Dims(eos5cfswath,var,he5s.geo_var_list,he5s.name,num_swaths,SWATH);
                else if ( 0 == swath_fieldtype_flag)
                    var_is_parsed = Set_Var_Dims(eos5cfswath,var,he5s.data_var_list,he5s.name,num_swaths,SWATH);
                else // Neither Geo nor Data(For example, added by the augmentation tool)
                    var_is_parsed = false;
                   
                if (false == var_is_parsed){
                    map<hsize_t,string> dimsizes_to_dimnames = eos5cfswath->dimsizes_to_dimnames;
                    Set_NonParse_Var_Dims(eos5cfswath,var,dimsizes_to_dimnames,num_swaths,vartype);
                }
            } // if (he5s.name == Obtain_Var_EOS5Type_GroupName(var,vartype))
        }// for (unsigned int i = 0; i < num_swaths; ++i)
    } // else if (SWATH == vartype)

    else if (ZA == vartype) {

        int num_zas = strmeta_info->za_list.size();
        for ( int i = 0; i < num_zas; ++i) {
            HE5Za he5z = strmeta_info->za_list.at(i);
            if (he5z.name == Obtain_Var_EOS5Type_GroupName(var,vartype)){
                EOS5CFZa *eos5cfza = (this->eos5cfzas)[i];
                bool var_is_parsed = Set_Var_Dims(eos5cfza,var,he5z.data_var_list,he5z.name,num_zas,ZA);
                if (false == var_is_parsed){
                    map<hsize_t,string> dimsizes_to_dimnames = eos5cfza->dimsizes_to_dimnames;
                    Set_NonParse_Var_Dims(eos5cfza,var,dimsizes_to_dimnames,num_zas,vartype);
                }
            }
        }
    }
    return false;
}
               
template <class T> 
bool EOS5File::Set_Var_Dims(T* eos5data, Var *var, vector<HE5Var> &he5var,
                            const string& groupname,int num_groups, EOS5Type eos5type) throw(Exception) {

    bool is_parsed = false;
    string eos5typestr = "";
    string fslash_str="/";

//cerr<<"coming to Set_Var_Dims "<<endl;

    if (GRID == eos5type) 
        eos5typestr = "/GRIDS/"; 
    else if (SWATH == eos5type) 
        eos5typestr = "/SWATHS/";
    else if (ZA    == eos5type) 
        eos5typestr = "/ZAS/";
    else 
        throw1("Unsupported HDF-EOS5 type, this type is not swath, grid or zonal average");

    for (unsigned int i = 0; i < he5var.size(); i++) {

        HE5Var he5v = he5var.at(i);

        if (he5v.name == var->name) {

            if (he5v.dim_list.size() != var->dims.size())
                throw2("Number of dimensions don't match with the structmetadata for variable ",
                        var->name);
            is_parsed = true;

            // Some variables have the same dim. names shared. For examples, we
            // see variables that have int foo[nlevels][nlevels]. To generate the CVs,
            // we have to make the dimension name unique for one variable. So we will
            // change the dimension names. The variable for the same example will be 
            // int foo[nlevels][nlevels_1]. Note this is not required by CF conventions.
            // This is simply due to the mising of the third coordinate variable for some 
            // NASA products. Another way is to totally ignore this kind of variables which
            // we will wait for users' responses.

            // Here is the killer, if different dim. names share the same size,
            // Currently there are no ways to know which dimension name is corresponding to
            // which size. HDF-EOS model gives too much freedom to users. The DimList in
            // the StructMetadata doesn't reflect the order at all. See two example files
            // CH4 in TES-Aura_L3-CH4_r0000010410_F01_07.he5 and NO2DayColumn in
            // HIRDLS-Aura_L3SCOL_v06-00-00-c02_2005d022-2008d077.he5.
            // Fortunately it seems that it doesn't matter for us to make the mapping from
            // dimension names to coordinate variables.
            // KY 2012-1-10

            // Dimension list of some OMI level 2 products doesn't include all dimension name and size
            // pairs. For example, Latitude[1644][60]. We have no way to find the dimension name of
            // the dimension with the size of 1644. The dimension name list of the variable also
            // includes the wrong dimension name. In this case, a dimension with the dimension size =1 
            // is allocated in the latitude's dimension list. The latest version still has this bug.
            // To serve this kind of files, we create a fakedim name for the unmatched size.
            // KY 2012-1-13

            set<hsize_t> dimsize_have_name_set;
            pair<set<hsize_t>::iterator,bool> setret1;
            set<string>  thisvar_dimname_set;
            pair<set<string>::iterator,bool> setret2;
          
            for (unsigned int j=0; j<he5v.dim_list.size();j++) {
                HE5Dim he5d = he5v.dim_list.at(j);
                for (vector<Dimension *>::iterator ird = var->dims.begin();
                        ird != var->dims.end(); ++ird) {
                    if ((hsize_t)(he5d.size) == (*ird)->size) {
                        // This will assure that the same size dims be assigned to different dims
                        if (""==(*ird)->name) { 
                            string dimname_candidate = eos5typestr  + groupname + fslash_str + he5d.name;
                            setret2 = thisvar_dimname_set.insert(dimname_candidate);
                            if (true == setret2.second) {
                                (*ird)->name =  dimname_candidate;
                                // Should check in the future if the newname may cause potential inconsistency. KY-2012-3-9
                                (*ird)->newname = (num_groups == 1)? he5d.name :(*ird)->name;
                                eos5data->vardimnames.insert((*ird)->name);
                            }
                        }
                    }
                }
            } // for (unsigned int j=0; j<he5v.dim_list.size();j++)

            // We have to go through the dimension list of this variable again to assure that every dimension has a name.
            for (vector<Dimension *>::iterator ird = var->dims.begin();
                        ird != var->dims.end(); ++ird) {
                if ("" == (*ird)->name) 
                    Create_Unique_DimName(eos5data, thisvar_dimname_set,*ird, num_groups, eos5type);
            }
        } // if (he5v.name == var->name) 
    } // for (unsigned int i = 0; i < he5var.size(); i++)
    return is_parsed;
} 

template<class T>
void EOS5File::Create_Unique_DimName(T*eos5data,set<string>& thisvar_dimname_set, 
                                     Dimension *dim,int num_groups, EOS5Type eos5type) throw(Exception){

//cerr <<"NO DIMNAME dim size = "<<(int)(dim->size) <<endl;

    map<hsize_t,string>:: iterator itmap1;
    map<string,hsize_t>:: iterator itmap2;
    pair<set<string>::iterator,bool> setret2;
    itmap1 = (eos5data->dimsizes_to_dimnames).find(dim->size);

    // Even if we find this dimension matches the dimsizes_to_dimnames map, we have to check if the dimension
    // name has been used for this size. This is to make sure each dimension has a unique name in a variable.
    // For example, float foo[100][100] can be float foo[nlevels = 100][nlevels_1 = 100].
    // Step 1: Check if there is a dimension name that matches the size

    if (itmap1 != (eos5data->dimsizes_to_dimnames).end()) {
        string dimname_candidate = (eos5data->dimsizes_to_dimnames)[dim->size];

        // First check local var dimname set
        setret2 = thisvar_dimname_set.insert(dimname_candidate);

        if (false == setret2.second) {
 
            // Will see if other dimension names have this size
            bool match_some_dimname = Check_All_DimNames(eos5data,dimname_candidate,dim->size);

            if (false == match_some_dimname) { 

                // dimname_candidate is updated.
                Get_Unique_Name(eos5data->vardimnames,dimname_candidate);
                thisvar_dimname_set.insert(dimname_candidate);

                // Finally generate a new dimension(new dim. name with a size);Update all information
                Insert_One_NameSizeMap_Element2(eos5data->dimnames_to_dimsizes,dimname_candidate,dim->size);
                eos5data->dimsizes_to_dimnames.insert(pair<hsize_t,string>(dim->size,dimname_candidate));
                eos5data->dimnames.push_back(dimname_candidate);
            }
        }   

        // The final dimname_candidate(may be updated) should be assigned to the name of this dimension
        dim->name = dimname_candidate;
        if (num_groups > 1) 
            dim->newname = dim->name;
        else { 
            string dname = HDF5CFUtil::obtain_string_after_lastslash(dim->name);
            if (""==dname)
                throw3("The dimension name ", dim->name, " of the variable  is not right");
                            else dim->newname = dname;
        }
    }

    else { // No dimension names match or close to march this dimension name, we will create a fakedim. 
           // Check Add_One_FakeDim_Name in HDF5CF.cc Fakedimname must be as a string reference.
        // cerr <<"NO DIMNAME dim size = "<<(int)(dim->size) <<endl;
        string Fakedimname = Create_Unique_FakeDimName(eos5data,eos5type);
        thisvar_dimname_set.insert(Fakedimname);

        // Finally generate a new dimension(new dim. name with a size);Update all information
        Insert_One_NameSizeMap_Element2(eos5data->dimnames_to_dimsizes,Fakedimname,dim->size);
        eos5data->dimsizes_to_dimnames.insert(pair<hsize_t,string>(dim->size,Fakedimname));
        eos5data->dimnames.push_back(Fakedimname);
        dim->name = Fakedimname;
        if (num_groups > 1) 
            dim->newname = dim->name;
        else {
            string dname = HDF5CFUtil::obtain_string_after_lastslash(dim->name);
            if (""==dname)
                throw3("The dimension name ", dim->name, " of the variable  is not right");
            else 
                dim->newname = dname;
        }
    }
}



template<class T>
bool EOS5File::Check_All_DimNames(T* eos5data,string& dimname,hsize_t dimsize) {

    bool ret_flag = false;
    for (map<string,hsize_t>::iterator im =eos5data->dimnames_to_dimsizes.begin();
        im !=eos5data->dimnames_to_dimsizes.end();++im) {
        // cerr<<"name to size"<< (*im).first <<"=> "<<(int)((*im).second) <<endl;
        // dimname must not be the same one since the same one is rejected.
        if (dimsize == (*im).second && dimname !=(*im).first) {
            dimname = (*im).first;
            ret_flag =  true;
            break;
        }
    }
    return ret_flag;
}       


void EOS5File::Get_Unique_Name(set<string> & nameset,string& dimname_candidate) throw(Exception){

    int clash_index =1;
    string temp_clashname=dimname_candidate+'_';
    HDF5CFUtil::gen_unique_name(temp_clashname,nameset,clash_index);
    dimname_candidate = temp_clashname;
}

template<class T>
string EOS5File::Create_Unique_FakeDimName(T*eos5data, EOS5Type eos5type) throw(Exception) {

    string fslash_str = "/";
    string eos5typestr;
    if (GRID==eos5type) 
        eos5typestr = "/GRIDS/";
    else if (SWATH == eos5type) 
        eos5typestr = "/SWATHS/";
    else if (ZA    == eos5type) 
        eos5typestr = "/ZAS/";
    else 
        throw1("Unsupported HDF-EOS5 type, this type is not swath, grid or zonal average");

    stringstream sfakedimindex;
    sfakedimindex << eos5data->addeddimindex;
    string fakedimstr = "FakeDim";
    string added_dimname = eos5typestr + eos5data->name + fslash_str + fakedimstr + sfakedimindex.str();

    pair<set<string>::iterator,bool> setret;
    setret = eos5data->vardimnames.insert(added_dimname);
    if (false == setret.second) 
        Get_Unique_Name(eos5data->vardimnames,added_dimname);
    eos5data->addeddimindex = eos5data->addeddimindex + 1;
    return added_dimname;
}


string EOS5File::Obtain_Var_EOS5Type_GroupName(Var*var,EOS5Type eos5type) throw(Exception) {

    string EOS5GRIDPATH ="/HDFEOS/GRIDS";
    string EOS5SWATHPATH ="/HDFEOS/SWATHS";
    string EOS5ZAPATH ="/HDFEOS/ZAS";
    size_t eostypename_start_pos;
    size_t eostypename_end_pos;
    string groupname;

    // The fullpath is like "HDFEOS/GRIDS/Temp/Data Fields/etc
    // To get "Temp", we obtain the position of "T" and the position of "p"
    // and then generate a substr.

    if (GRID == eos5type) 
        eostypename_start_pos = EOS5GRIDPATH.size()+1;
    else if (SWATH == eos5type)
        eostypename_start_pos = EOS5SWATHPATH.size()+1;
    else if (ZA == eos5type)
        eostypename_start_pos = EOS5ZAPATH.size()+1;
    else 
        throw2("Non supported eos5 type for var ",var->fullpath);
   
    eostypename_end_pos = var->fullpath.find('/',eostypename_start_pos)-1;
    groupname = var->fullpath.substr(eostypename_start_pos,eostypename_end_pos-eostypename_start_pos+1);

    return groupname;
}

int EOS5File::Check_EOS5Swath_FieldType(Var*var) throw(Exception) {

    string geofield_relative_path = "/Geolocation Fields/"+var->name;
    string datafield_relative_path = "/Data Fields/" + var->name;

    int tempflag = -1;
    
    if (var->fullpath.size() > datafield_relative_path.size()) {
        size_t field_pos_in_full_path = var->fullpath.size()-datafield_relative_path.size();
        if (var->fullpath.rfind(datafield_relative_path,field_pos_in_full_path) !=string::npos) 
            tempflag = 0;
    }
    
    if (tempflag != 0 && (var->fullpath.size() > geofield_relative_path.size())) {
        size_t field_pos_in_full_path = var->fullpath.size()-geofield_relative_path.size();
//cerr<<"field_pos_in_full_path= "<<(int)field_pos_in_full_path <<endl;
        if (var->fullpath.rfind(geofield_relative_path,field_pos_in_full_path) !=string::npos) 
            tempflag = 1;
    }
    return tempflag;
}
     

  
// An error will be thrown if we find a dimension size that doesn't match any dimension name
// in this EOS5 group.
template<class T> 
void EOS5File::Set_NonParse_Var_Dims(T*eos5data, Var* var,  
                                     map<hsize_t,string>& dimsizes_to_dimnames,
                                     int num_groups, EOS5Type eos5type) throw(Exception){

    map<hsize_t,string>::iterator itmap;
    set<string> thisvar_dimname_set;

    for (vector<Dimension *>::iterator ird = var->dims.begin();
        ird != var->dims.end(); ++ird) {
        if ("" == (*ird)->name) 
                    Create_Unique_DimName(eos5data, thisvar_dimname_set,*ird, num_groups, eos5type);
        else  
            throw5("The dimension name ", (*ird)->name, " of the variable ",var->name," is not right");
    }
}


void EOS5File::Check_Aura_Product_Status() throw(Exception) {

    // Aura files will put an attribute called InStrumentName under /HDFEOS/ADDITIONAL/FILE_ATTRIBUTES
    // We just need to check that attribute.
    string eos5_fattr_group_name ="/HDFEOS/ADDITIONAL/FILE_ATTRIBUTES";
    string instrument_attr_name = "InstrumentName";

    // Check if this file is an aura file
    for (vector<Group *>::iterator irg = this->groups.begin();
                irg != this->groups.end(); ++irg) {
        if (eos5_fattr_group_name ==(*irg)->path){
            for (vector<Attribute *>::iterator ira = (*irg)->attrs.begin();
                ira != (*irg)->attrs.end(); ++ira) {
                if (instrument_attr_name == (*ira)->name) {
                    Retrieve_H5_Attr_Value(*ira,(*irg)->path);
                    string attr_value((*ira)->value.begin(),(*ira)->value.end());
                    if ("OMI" == attr_value) {
                        this->isaura = true;
                        this->aura_name = OMI;
                    }
                    else if ("MLS Aura" == attr_value) {
                        this->isaura = true;
                        this->aura_name = MLS;
                    }
                    else if ("TES" == attr_value) {
                        this->isaura = true;
                        this->aura_name = TES;
                    }
                    else if ("HIRDLS" == attr_value) {
                        this->isaura = true;
                        this->aura_name = HIRDLS;
                    }
                    break;
                }
            }
        }
    }

    // Assign EOS5 to CF MAP values for Aura files
    if (true == this->isaura) {
        eos5_to_cf_attr_map["FillValue"] = "_FillValue";
        eos5_to_cf_attr_map["MissingValue"] = "missing_value";
        eos5_to_cf_attr_map["Units"] = "units";
        eos5_to_cf_attr_map["Offset"] = "add_offset";
        eos5_to_cf_attr_map["ScaleFactor"] = "scale_factor";
        eos5_to_cf_attr_map["ValidRange"] = "valid_range";
        eos5_to_cf_attr_map["Title"] = "title";
    }

}

void EOS5File::Handle_CVar() throw(Exception){

    bool is_augmented = Check_Augmentation_Status();

#if 0
if(is_augmented) cerr <<"The file is augmented "<<endl;
else cerr<<"The file is not augmented "<<endl;
#endif

    if (this->eos5cfgrids.size() > 0) 
        Handle_Grid_CVar(is_augmented);
    if (this->eos5cfswaths.size() >0) 
        Handle_Swath_CVar(is_augmented);
    if (this->eos5cfzas.size() >0) 
        Handle_Za_CVar(is_augmented);

#if 0
for (vector<EOS5CVar *>::iterator irv = this->cvars.begin();
                irv != this->cvars.end(); irv++) {
cerr <<"EOS5CVar name "<<(*irv)->name <<endl;
cerr <<"EOS5CVar dimension name "<< (*irv)->cfdimname <<endl;
cerr <<"EOS5CVar new name "<<(*irv)->newname <<endl;
}
#endif
    
}
void EOS5File::Handle_Grid_CVar(bool is_augmented) throw(Exception){


    if (true == is_augmented) {
//cerr<<"IS Augmented "<<endl;
        // Create latitude/longitude based on the first XDim and YDim
        Handle_Augmented_Grid_CVar();
    }
    else {
        // cerr<<"before this->eos5cfgrids.size() = " << this->eos5cfgrids.size() << endl;
        Remove_MultiDim_LatLon_EOS5CFGrid();
        // cerr<<"this->eos5cfgrids.size() = " << this->eos5cfgrids.size() << endl;
        // If the grid size is 0, it must be a Grid file that cannot be handled
        // with the CF option, simply return with handling any coordinate variables.
        if (0 == this->eos5cfgrids.size()) return;
        if (1 == this->eos5cfgrids.size()) 
            Handle_Single_Nonaugment_Grid_CVar((this->eos5cfgrids)[0]);
        else 
            Handle_Multi_Nonaugment_Grid_CVar();
    }
}

bool EOS5File::Check_Augmentation_Status() throw(Exception) {

    bool aug_status = false;
    int num_aug_eos5grp = 0;

    for (vector <EOS5CFGrid *>::iterator irg = this->eos5cfgrids.begin();
                irg != this->eos5cfgrids.end(); ++irg) {
        for (vector<Var *>::iterator irv = this->vars.begin();
                irv != this->vars.end(); ++irv) {
            bool is_augmented = Check_Augmented_Var_Candidate(*irg,*irv,GRID);
            if (true == is_augmented) {
                num_aug_eos5grp ++;
                break;
            }
        }
    }

    for (vector <EOS5CFSwath *>::iterator irg = this->eos5cfswaths.begin();
                irg != this->eos5cfswaths.end(); ++irg) {
        for (vector<Var *>::iterator irv = this->vars.begin();
                irv != this->vars.end(); ++irv) {
            bool is_augmented = Check_Augmented_Var_Candidate(*irg,*irv,SWATH);
            if (true == is_augmented) {
                num_aug_eos5grp ++;
                break;
            }

        }
    }

    for (vector <EOS5CFZa *>::iterator irg = this->eos5cfzas.begin();
                irg != this->eos5cfzas.end(); ++irg) {
        for (vector<Var *>::iterator irv = this->vars.begin();
                irv != this->vars.end(); ++irv) {
            bool is_augmented = Check_Augmented_Var_Candidate(*irg,*irv,ZA);
            if (true == is_augmented) {
                num_aug_eos5grp ++;
                break;
            }
        }
    }

    int total_num_eos5grp = this->eos5cfgrids.size() + this->eos5cfswaths.size() + this->eos5cfzas.size();
//cerr<< "total_num_eos5grp "<<total_num_eos5grp <<endl;
//cerr <<"num_aug_eos5grp "<< num_aug_eos5grp <<endl;

    if (num_aug_eos5grp == total_num_eos5grp) 
        aug_status = true;
    return aug_status;

}

// This method is not used. Still keep it now since it may be useful in the future. KY 2012-3-09
bool EOS5File::Check_Augmented_Var_Attrs(Var *var) throw(Exception) {

    // We will check whether the attribute "CLASS" and the attribute "REFERENCE_LIST" exist.
    // For the attribute "CLASS", we would like to check if the value is "DIMENSION_SCALE".
    bool has_dimscale_class = false;
    bool has_reflist = false;
    for (vector<Attribute *>::iterator ira = var->attrs.begin();
                        ira != var->attrs.end(); ++ira) {
        if ("CLASS" == (*ira)->name) {
            Retrieve_H5_Attr_Value(*ira,var->fullpath);
            string class_value((*ira)->value.begin(),(*ira)->value.end());
            if ("DIMENSION_SCALE"==class_value) 
                has_dimscale_class = true;
        }
        
        if ("REFERENCE_LIST" == (*ira)->name) 
            has_reflist = true;
        if (true == has_reflist && true == has_dimscale_class) 
            break;
    }

    if (true == has_reflist && true == has_dimscale_class) 
        return true;
    else 
        return false;

}

template <class T>
bool EOS5File::Check_Augmented_Var_Candidate(T *eos5data, Var *var, EOS5Type eos5type) throw(Exception) {

    bool  augmented_var = false;

    string EOS5DATAPATH = "";
    if (GRID == eos5type) 
        EOS5DATAPATH = "/HDFEOS/GRIDS/";
    else if (ZA   == eos5type) 
        EOS5DATAPATH = "/HDFEOS/ZAS/";
    else if (SWATH == eos5type) 
        EOS5DATAPATH ="/HDFEOS/SWATHS/";
    else throw1("Non supported EOS5 type");

    string fslash_str ="/";
    string THIS_EOS5DATAPATH = EOS5DATAPATH + eos5data->name + fslash_str;

    // Match the EOS5 type
    if (eos5type == Get_Var_EOS5_Type(var)) {
        string var_eos5data_name = Obtain_Var_EOS5Type_GroupName(var,eos5type);
        // Match the EOS5 group name
        if (var_eos5data_name == eos5data->name) {
            if (var->fullpath.size() > THIS_EOS5DATAPATH.size()){
                // Obtain the var name from the full path
                string var_path_after_eos5dataname = var->fullpath.substr(THIS_EOS5DATAPATH.size());
                // Match the variable name
                if (var_path_after_eos5dataname == var->name) 
                    augmented_var = true;
            }
        }
    }

    return augmented_var;

}
    

void EOS5File::Handle_Augmented_Grid_CVar() throw(Exception) {
    for (vector <EOS5CFGrid *>::iterator irv = this->eos5cfgrids.begin();
        irv !=this->eos5cfgrids.end();++irv) 
        Handle_Single_Augment_CVar(*irv,GRID);
}


template <class T> 
void EOS5File::Handle_Single_Augment_CVar(T* cfeos5data, EOS5Type eos5type) throw(Exception) {
     
    set<string> tempvardimnamelist;
    tempvardimnamelist = cfeos5data->vardimnames;
    set <string>::iterator its;
//cerr <<"coming to Single Augment Grid "<<endl;
    for (its = tempvardimnamelist.begin(); its != tempvardimnamelist.end(); ++its) {
//cerr<<"DIMENSION NAME List "<<*its <<endl;
        for (vector<Var *>::iterator irv = this->vars.begin();
                irv != this->vars.end(); ++irv) {

            bool is_augmented = Check_Augmented_Var_Candidate(cfeos5data,*irv,eos5type);

            if (true == is_augmented) {

                // Since we have already checked if this file is augmented or not, we can safely
                // compare the dimension name with the var name now.
//cerr<<"dimension name "<<*its <<endl;
                string tempdimname = HDF5CFUtil::obtain_string_after_lastslash(*its);
//cerr<<"tempdimname "<<tempdimname <<endl;
                if (tempdimname == (*irv)->name) { 

                    //Find it, create a coordinate variable.
                    EOS5CVar *EOS5cvar = new EOS5CVar(*irv);

                    // Still keep the original dimension name to avoid the nameclashing when
                    // one grid and one swath and one za occur in the same file
                    EOS5cvar->cfdimname = *its;
                    EOS5cvar->cvartype = CV_EXIST;
                    EOS5cvar->eos_type = eos5type;

                    // Save this cv to the cv vector
                    this->cvars.push_back(EOS5cvar);

                    // Remove this var from the var vector since it becomes a cv.
                    delete(*irv);
                    this->vars.erase(irv);
                    irv--;
                }
            } // if (true == is_augmented)
        } // for (vector<Var *>::iterator irv = this->vars.begin();....
    } // for (its = tempvardimnamelist.begin(); its != tempvardimnamelist.end(); ++its)

    for (vector<EOS5CVar *>::iterator irv = this->cvars.begin();
                irv != this->cvars.end(); ++irv) {
        its = tempvardimnamelist.find((*irv)->cfdimname);
        if (its != tempvardimnamelist.end()) 
            tempvardimnamelist.erase(its);
    }

    if (false == tempvardimnamelist.empty()) 
        throw1("Augmented files still need to provide more coordinate variables");
}


void EOS5File::Remove_MultiDim_LatLon_EOS5CFGrid() throw(Exception)         {

    for (vector <EOS5CFGrid *>::iterator irg = this->eos5cfgrids.begin();
         irg != this->eos5cfgrids.end(); ++irg) {

        // If number of dimension latitude/longitude is >=2, no cooridnate variables will be generated.
        // We will simply remove this grid from the vector eos5cfgrids.
        // In the future, we may consider supporting 2D latlon. KY 2012-1-17
        // I just find that new OMI level 3 data provide 2D lat/lon for geographic projection data.
        // The 2D lat/lon can be condensed to 1D lat/lon, which is the same calculated by the calculation of
        // the projection. So I don't remove this OMI grid from the grid list. KY 2012-2-9
        // However, I do remove the "Longitude" and "Latitude" fields since "Latitude" and "Longitude"
        // can be calculated. 

        if (true  == (*irg)->has_2dlatlon) {

            if ((true == this->isaura) && (OMI == this->aura_name) &&
                (HE5_GCTP_GEO == (*irg)->eos5_projcode))

            {// We need to remove the redundant latitude and longitude fields

                string EOS5GRIDPATH ="/HDFEOS/GRIDS/";
                string fslash_str ="/";
                string THIS_EOS5GRIDPATH = EOS5GRIDPATH + (*irg)->name + fslash_str;
                int catch_latlon = 0;

                for (vector<Var *>::iterator irv = this->vars.begin();
                    (irv != this->vars.end()) && (catch_latlon!=2); ++irv) {
                    if (GRID == Get_Var_EOS5_Type(*irv) &&
                            ((*irv)->fullpath.size() > THIS_EOS5GRIDPATH.size())) {

                       string var_grid_name = Obtain_Var_EOS5Type_GroupName(*irv,GRID);
                       if ((var_grid_name == (*irg)->name)) {
                           if(("Longitude" == (*irv)->name)  || ("Latitude" == (*irv)->name)) {
                                catch_latlon++;
                                // Remove this var from the var vector since it becomes a cv.
                                delete(*irv);
                                this->vars.erase(irv);
                                irv--;
                           }
                       }
                    }
                } //  for (vector<Var *>::iterator irv = this->vars.begin() ...
                if (2 == catch_latlon) {
                    // cerr <<"coming to the catch "<<endl;
                    (*irg)->has_nolatlon = true;
                    (*irg)->has_2dlatlon = false;
                }
            } // if ((true == this->isaura) ...
            else {// remove this grid from the eos5cfgrids list.
                delete (*irg);
                this->eos5cfgrids.erase(irg);
                irg--;
            }
        } // if (true  == (*irg) ...
            
        if (true == (*irg)->has_g2dlatlon)  {
            delete (*irg);
            this->eos5cfgrids.erase(irg);
            irg--;
        }
    } // for (vector <EOS5CFGrid *>::iterator irg = this->eos5cfgrids.begin() ...
}

void EOS5File::Handle_Single_Nonaugment_Grid_CVar(EOS5CFGrid* cfgrid) throw(Exception){

    set<string> tempvardimnamelist;
    tempvardimnamelist = cfgrid->vardimnames;

     // Handle Latitude and longitude
    bool use_own_latlon = false;
    if (true == cfgrid->has_1dlatlon) 
        use_own_latlon = Handle_Single_Nonaugment_Grid_CVar_OwnLatLon(cfgrid, tempvardimnamelist);
#if 0
if(use_own_latlon) cerr <<"using 1D latlon"<<endl;
else cerr <<"use_own_latlon is false "<<endl;
#endif

    if (false == use_own_latlon) {
        bool use_eos5_latlon = false;
        use_eos5_latlon = Handle_Single_Nonaugment_Grid_CVar_EOS5LatLon(cfgrid,tempvardimnamelist);

        // If we cannot obtain lat/lon from the HDF-EOS5 library, no need to create other CVs. Simply return.
        if (false == use_eos5_latlon) 
            return;
    }

     // Else handling non-latlon grids
//cerr <<"tempvardim set size "<<tempvardimnamelist.size() <<endl;
    Handle_NonLatLon_Grid_CVar(cfgrid,tempvardimnamelist);

}


bool EOS5File::Handle_Single_Nonaugment_Grid_CVar_OwnLatLon(EOS5CFGrid *cfgrid, set<string>& tempvardimnamelist) throw(Exception) {

    set <string>::iterator its;
    string EOS5GRIDPATH ="/HDFEOS/GRIDS/";
    string fslash_str ="/";
    string THIS_EOS5GRIDPATH = EOS5GRIDPATH + cfgrid->name + fslash_str;
//cerr<<"coming to Handle_Single_Nonaugment_Grid_CVar_OwnLatLon " <<endl;

    // Handle latitude and longitude
    bool find_latydim = false;
    bool find_lonxdim = false;

    for (vector<Var *>::iterator irv = this->vars.begin();
            irv != this->vars.end(); ++irv) {
        if (GRID == Get_Var_EOS5_Type(*irv) &&
            ((*irv)->fullpath.size() > THIS_EOS5GRIDPATH.size())) {

            string var_grid_name = Obtain_Var_EOS5Type_GroupName(*irv,GRID);
            if ((var_grid_name == cfgrid->name) && ((*irv)->name == "Latitude")) {
//cerr<<"coming to latitiude "<<endl;
                string tempdimname = (((*irv)->dims)[0])->name;
                if ("YDim" == HDF5CFUtil::obtain_string_after_lastslash(tempdimname)) {
//cerr<<"coming to YDim "<<endl;
                        //Find it, create a coordinate variable.
                    EOS5CVar *EOS5cvar = new EOS5CVar(*irv);

                    // Still keep the original dimension name to avoid the nameclashing when
                    // one grid and one swath and one za occur in the same file
                    EOS5cvar->cfdimname = tempdimname;
                    EOS5cvar->cvartype = CV_EXIST;
                    EOS5cvar->eos_type = GRID;

                    // Save this cv to the cv vector
                    this->cvars.push_back(EOS5cvar);

                    // Remove this var from the var vector since it becomes a cv.
                    delete(*irv);
                    this->vars.erase(irv);

                    // No need to remove back the iterator since it will go out of the loop.
                    find_latydim = true;
                    break;
                } // if ("YDim" == HDF5CFUtil::obtain_string_after_lastslash(tempdimname))
            } // if ((var_grid_name == cfgrid->name) && ((*irv)->name == "Latitude"))
        } // if (GRID == Get_Var_EOS5_Type(*irv) ...
    } // for (vector<Var *>::iterator irv = this->vars.begin() ...

    for (vector<Var *>::iterator irv = this->vars.begin();
               irv != this->vars.end(); ++irv) {

        if (GRID == Get_Var_EOS5_Type(*irv) &&
                  ((*irv)->fullpath.size() > THIS_EOS5GRIDPATH.size())) {

            string var_grid_name = Obtain_Var_EOS5Type_GroupName(*irv,GRID);
                      
            if ((var_grid_name == cfgrid->name) && ((*irv)->name == "Longitude")) {
//cerr<<"coming to longitiude "<<endl;
                string tempdimname = (((*irv)->dims)[0])->name;

                if ("XDim" == HDF5CFUtil::obtain_string_after_lastslash(tempdimname)) {
                    //Find it, create a coordinate variable.
                    EOS5CVar *EOS5cvar = new EOS5CVar(*irv);

                    // Still keep the original dimension name to avoid the nameclashing when
                    // one grid and one swath and one za occur in the same file
                    EOS5cvar->cfdimname = tempdimname;
                    EOS5cvar->cvartype = CV_EXIST;
                    EOS5cvar->eos_type = GRID;

                    // Save this cv to the cv vector
                    this->cvars.push_back(EOS5cvar);

                    // Remove this var from the var vector since it becomes a cv.
                    delete(*irv);
                    this->vars.erase(irv);
                    find_lonxdim = true;
                    break;
                } // if ("XDim" == HDF5CFUtil::obtain_string_after_lastslash(tempdimname))
            } // if ((var_grid_name == cfgrid->name) && ((*irv)->name == "Longitude"))
        } // if (GRID == Get_Var_EOS5_Type(*irv) ...
    }// for (vector<Var *>::iterator irv = this->vars.begin(); ...

    

    for (vector<EOS5CVar *>::iterator irv = this->cvars.begin();
                irv != this->cvars.end(); ++irv) {
        its = tempvardimnamelist.find((*irv)->cfdimname);
        if (its != tempvardimnamelist.end()) tempvardimnamelist.erase(its);

    }
     
#if 0
for (its = tempvardimnamelist.begin(); its != tempvardimnamelist.end(); ++its)
        cerr <<"tempvardim "<<*its <<endl;
#endif

    return(find_latydim == true && find_lonxdim == true);   
}

bool EOS5File::Handle_Single_Nonaugment_Grid_CVar_EOS5LatLon(EOS5CFGrid *cfgrid, set<string>& tempvardimnamelist) throw(Exception) {

    // Handle latitude and longitude
    bool find_ydim = false;
    bool find_xdim = false;
    set <string>::iterator its;


#if 0
for (its = tempvardimnamelist.begin(); its != tempvardimnamelist.end(); ++its) 
cerr<<"dim names "<<(*its) <<endl;
#endif

    for (its = tempvardimnamelist.begin(); its != tempvardimnamelist.end(); ++its) {

        if ("YDim" == HDF5CFUtil::obtain_string_after_lastslash(*its)) {
//cerr<<"coming to YDim "<<endl;

            // Create EOS5 Latitude CV
            EOS5CVar *EOS5cvar = new EOS5CVar();
            EOS5cvar->name = "lat";
            Create_Added_Var_NewName_FullPath(GRID,cfgrid->name,EOS5cvar->name,EOS5cvar->newname,EOS5cvar->fullpath);
            EOS5cvar->rank = 1;
            EOS5cvar->dtype = H5FLOAT32;
            Dimension* eos5cvar_dim = new Dimension((hsize_t)cfgrid->ydimsize);
            eos5cvar_dim->name = *its;
            eos5cvar_dim->newname = (this->eos5cfgrids.size() == 1) ? "YDim":(*its);
            EOS5cvar->dims.push_back(eos5cvar_dim);
            EOS5cvar->cfdimname = eos5cvar_dim->name;
            EOS5cvar->cvartype = CV_LAT_MISS;
            EOS5cvar->eos_type = GRID;
            EOS5cvar->xdimsize = cfgrid->xdimsize;
            EOS5cvar->ydimsize = cfgrid->ydimsize;

            //Special parameters for EOS5 Grid
            EOS5cvar->point_lower = cfgrid->point_lower;
            EOS5cvar->point_upper = cfgrid->point_upper;
            EOS5cvar->point_left = cfgrid->point_left;
            EOS5cvar->point_right = cfgrid->point_right;
            EOS5cvar->eos5_pixelreg = cfgrid->eos5_pixelreg;
            EOS5cvar->eos5_origin = cfgrid->eos5_origin;
            EOS5cvar->eos5_projcode = cfgrid->eos5_projcode;

            // Save this cv to the cv vector
            this->cvars.push_back(EOS5cvar);
            // erase the dimension name from the dimension name set
            tempvardimnamelist.erase(its);
            find_ydim = true;
// cerr<<"end of YDim " <<endl;
         
        } // if ("YDim" == HDF5CFUtil::obtain_string_after_lastslash(*its))

        else if ("XDim" == HDF5CFUtil::obtain_string_after_lastslash(*its)) {
// cerr<<"coming to XDim" <<endl;

            // Create EOS5 Latitude CV
            EOS5CVar *EOS5cvar = new EOS5CVar();
            EOS5cvar->name = "lon";
            Create_Added_Var_NewName_FullPath(GRID,cfgrid->name,EOS5cvar->name,EOS5cvar->newname,EOS5cvar->fullpath);
            //EOS5cvar->newname = EOS5cvar->name;
            //EOS5cvar->fullpath = EOS5cvar->name;
            EOS5cvar->rank = 1;
            EOS5cvar->dtype = H5FLOAT32;
            Dimension* eos5cvar_dim = new Dimension((hsize_t)cfgrid->xdimsize);
            //eos5cvar_dim->name = EOS5cvar->name;
            eos5cvar_dim->name = *its;
            eos5cvar_dim->newname = (this->eos5cfgrids.size() == 1) ? "XDim":(*its);
            EOS5cvar->dims.push_back(eos5cvar_dim);
            EOS5cvar->cfdimname = eos5cvar_dim->name;
            EOS5cvar->cvartype = CV_LON_MISS;
            EOS5cvar->eos_type = GRID;
            EOS5cvar->xdimsize = cfgrid->xdimsize;
            EOS5cvar->ydimsize = cfgrid->ydimsize;


            //Special parameters for EOS5 Grid
            EOS5cvar->point_lower = cfgrid->point_lower;
            EOS5cvar->point_upper = cfgrid->point_upper;
            EOS5cvar->point_left = cfgrid->point_left;
            EOS5cvar->point_right = cfgrid->point_right;
            EOS5cvar->eos5_pixelreg = cfgrid->eos5_pixelreg;
            EOS5cvar->eos5_origin = cfgrid->eos5_origin;
            EOS5cvar->eos5_projcode = cfgrid->eos5_projcode;

            // Save this cv to the cv vector
            this->cvars.push_back(EOS5cvar);
            // erase the dimension name from the dimension name set
            tempvardimnamelist.erase(its);
            find_xdim = true;
// cerr<<"end of XDim" <<endl;
         
        } // else if ("XDim" == HDF5CFUtil::obtain_string_after_lastslash(*its))
        if (true == find_xdim && true == find_ydim) 
            break;
    } // for (its = tempvardimnamelist.begin(); its != tempvardimnamelist.end(); ++its)
    
    return ( true == find_xdim && true == find_ydim);
}

void EOS5File::Handle_NonLatLon_Grid_CVar(EOS5CFGrid *cfgrid, set<string>& tempvardimnamelist) throw(Exception) {

    // First check if we have existing coordinate variable
    set <string>::iterator its;
    int num_dimnames = tempvardimnamelist.size();
    bool has_dimnames = true;

//cerr<<"num_dimnames "<<num_dimnames <<endl;
    for (its = tempvardimnamelist.begin(); its != tempvardimnamelist.end(); ++its) {
        if (cfgrid->dnames_to_1dvnames.find(*its) !=cfgrid->dnames_to_1dvnames.end()){
            for (vector<Var *>::iterator irv = this->vars.begin();
                    has_dimnames && (irv != this->vars.end()); ++irv) {
                // We need to check if this var is a grid and use "newname"
                // of var to check the dnames_to_1dvnames since it is 
                // possible to have name clashings for the "name" of a var.
                if (GRID == Get_Var_EOS5_Type(*irv) &&
                    (*irv)->newname == (cfgrid->dnames_to_1dvnames)[*its]) {

                    //Find it, create a coordinate variable.
                    EOS5CVar *EOS5cvar = new EOS5CVar(*irv);
    
                    // Still keep the original dimension name to avoid the nameclashing when
                    // one grid and one swath and one za occur in the same file
                    EOS5cvar->cfdimname = *its;
                    EOS5cvar->cvartype = CV_EXIST;
                    EOS5cvar->eos_type = GRID;

                    // Save this cv to the cv vector
                    this->cvars.push_back(EOS5cvar);
        
                    // Remove this var from the var vector since it becomes a cv.
                    delete(*irv);
                    this->vars.erase(irv);
                    irv--;
                    num_dimnames--;
                    if (0 == num_dimnames) 
                        has_dimnames = false;
                } // if (GRID == Get_Var_EOS5_Type(*irv) ...
            } // for (vector<Var *>::iterator irv = this->vars.begin(); ...
        } // if (cfgrid->dnames_to_1dvnames.find(*its) !=cfgrid->dnames_to_1dvnames.end())
    } // for (its = tempvardimnamelist.begin(); its != tempvardimnamelist.end(); ++its)

// cerr<<"before EOS5CVar in handle_nonlatlon "<<endl;
    // Remove the dimension name that finds the cooresponding variables from the tempvardimlist.
    for (vector<EOS5CVar *>::iterator irv = this->cvars.begin();
             irv != this->cvars.end(); ++irv) {
        its = tempvardimnamelist.find((*irv)->cfdimname);
        if (its != tempvardimnamelist.end()) 
            tempvardimnamelist.erase(its);
    }

    // Second: Some dimension names still need to find CVs, create the missing CVs
    for (its = tempvardimnamelist.begin(); its != tempvardimnamelist.end(); ++its) {

        EOS5CVar *EOS5cvar = new EOS5CVar();
        Create_Missing_CV(cfgrid,EOS5cvar,*its,GRID,this->eos5cfgrids.size());
        this->cvars.push_back(EOS5cvar);

    }
// cerr<<" end of handle_nonlatlon "<<endl;
}


void EOS5File::Handle_Multi_Nonaugment_Grid_CVar() throw(Exception){

//cerr <<"coming to Handle_Multi_nonaugment_Grid_CVar "<<endl;

    // If the multiple grids don't share the same lat/lon according to the parameters
    // We then assume that each single grid has its own lat/lon, just loop through each grid.
    if (true == this->grids_multi_latloncvs) {
        for (vector <EOS5CFGrid *>::iterator irv = this->eos5cfgrids.begin();
            irv !=this->eos5cfgrids.end();++irv)
            Handle_Single_Nonaugment_Grid_CVar(*irv);
    }

    // We would like to check if lat/lon pairs provide for all grids
    // If lat/lon pairs are provided for all grids, then we ASSUME that
    // all grids share the same lat/lon values. This is what happened with
    // Aura grids. We only apply this to Aura files.They provide a lat/lon pair for each grid. We will observe
    // if this assumption is true for the future products.
    // If lat/lon pairs are  not provided for all grids, we assume that each grid
    // may still have its unique lat/lon.
    else {
        int num_1dlatlon_pairs = 0;
        for (vector <EOS5CFGrid *>::iterator irv = this->eos5cfgrids.begin();
            irv !=this->eos5cfgrids.end() ;++irv) 
            if (true == (*irv)->has_1dlatlon) 
                num_1dlatlon_pairs ++;

        bool use_eos5_latlon = false;
        if (( 0 == num_1dlatlon_pairs) || 
            ((num_1dlatlon_pairs == (int)(this->eos5cfgrids.size())) && (true == this->isaura))) {
            set<string> tempvardimnamelist = ((this->eos5cfgrids)[0])->vardimnames;
            if ( 0 == num_1dlatlon_pairs) { 
                use_eos5_latlon = 
                Handle_Single_Nonaugment_Grid_CVar_EOS5LatLon((this->eos5cfgrids)[0],tempvardimnamelist);

                if (false == use_eos5_latlon) 
                    return;
            }
       
            else {
//cerr<<"coming to one lat/lon for all grids" <<endl;
                // One lat/lon for all grids
                bool use_own_latlon = false;
                use_own_latlon = 
                Handle_Single_Nonaugment_Grid_CVar_OwnLatLon((this->eos5cfgrids)[0],tempvardimnamelist);
                if (false == use_own_latlon) {
                    use_eos5_latlon = Handle_Single_Nonaugment_Grid_CVar_EOS5LatLon((this->eos5cfgrids)[0],tempvardimnamelist);
                    if (false == use_eos5_latlon) 
                        return;
                }
            }

//cerr <<"before handling nonlatlon grid"<<endl;
            // We need to handle the first grid differently since it will include "XDim" and "YDim".
            Handle_NonLatLon_Grid_CVar((this->eos5cfgrids)[0],tempvardimnamelist);

//cerr <<"after handling nonlatlon grid"<<endl;
            // Updating the dimension name sets for other grids
            for (unsigned j = 1; j < this->eos5cfgrids.size(); j++ ) 
                (this->eos5cfgrids)[j]->Update_Dimnamelist();

//cerr <<"after updating dimension name list"<<endl;
            // Adjusting the dimension names for all vars under these EOS5 Grids
            Adjust_EOS5GridDimNames((this->eos5cfgrids)[0]);

            // Now we can safely handle the rest grids
            for (unsigned j = 1; j < this->eos5cfgrids.size(); j++ ) {
                tempvardimnamelist = (this->eos5cfgrids)[j]->vardimnames;
                Handle_NonLatLon_Grid_CVar((this->eos5cfgrids)[j],tempvardimnamelist);
                tempvardimnamelist.clear();
            }
//cerr <<"end of NONLATLON "<<endl;
        }// if (( 0 == num_1dlatlon_pairs) || .....
        // No unique lat/lon, just loop through. 
        else {
            
            this->grids_multi_latloncvs = true;
            for (vector <EOS5CFGrid *>::iterator irv = this->eos5cfgrids.begin();
                irv !=this->eos5cfgrids.end();++irv)
                Handle_Single_Nonaugment_Grid_CVar(*irv);
        }
    }
}

void EOS5File::Adjust_EOS5GridDimNames(EOS5CFGrid *cfgrid) throw(Exception) {

    string xdimname;
    string ydimname;
    bool find_xdim = false;
    bool find_ydim = false;
    
    for (set<string>::iterator it = cfgrid->vardimnames.begin();
         it != cfgrid->vardimnames.end(); ++it) {
        string xydimname_candidate = HDF5CFUtil::obtain_string_after_lastslash(*it);
        if ("XDim" == xydimname_candidate) {
            find_xdim = true;
            xdimname = *it;
        }
        else if ("YDim" == xydimname_candidate) {
            find_ydim = true;
            ydimname = *it;
        }
        if (find_xdim && find_ydim) break;
    } // for (set<string>::iterator it = cfgrid->vardimnames.begin() ...

    if (false == find_xdim || false == find_ydim) 
        throw2("Cannot find Dimension name that includes XDim or YDim in the grid ",cfgrid->name);

    for (vector <Var *>::iterator irv = this->vars.begin();
                irv !=this->vars.end();++irv) {
        if (GRID == Get_Var_EOS5_Type(*irv)){
            for (vector <Dimension *>::iterator id = (*irv)->dims.begin();
                id != (*irv)->dims.end(); ++id) {
                string xydimname_candidate = HDF5CFUtil::obtain_string_after_lastslash((*id)->name);
                if ("XDim" == xydimname_candidate) (*id)->name = xdimname;
                else if("YDim" == xydimname_candidate) (*id)->name = ydimname;
//cerr<<"dim name before the final "<<(*id)->name <<endl;
            }
        }
    }
}

void EOS5File::Handle_Swath_CVar(bool isaugmented) throw(Exception){

    // In this version, we will not use the augmented option for coordinate variables of swath
    // since MLS products don't use the recent version of the augmentation tool to allocate their 
    // coordinate variables.
    for (vector <EOS5CFSwath *>::iterator irs = this->eos5cfswaths.begin();
            irs !=this->eos5cfswaths.end();++irs) {
        if ((*irs)->has_1dlatlon) 
            Handle_Single_1DLatLon_Swath_CVar(*irs,isaugmented);

        else if((*irs)->has_2dlatlon) 
            Handle_Single_2DLatLon_Swath_CVar(*irs,isaugmented);

        // If number of dimension latitude/longitude is >2 or no lat/lon, 
        // no cooridnate variables will be generated.
        // We will simply remove this swath from the vector eos5cfswaths.
        // In the future, we may consider supporting non "Latitude", "Longitude" naming swaths. 
        // KY 2011-1-20
        else {
            delete (*irs);
            this->eos5cfswaths.erase(irs);
            irs--;
        }
    } // for (vector <EOS5CFSwath *>::iterator irs = this->eos5cfswaths.begin();
}

void EOS5File::Handle_Single_1DLatLon_Swath_CVar(EOS5CFSwath *cfswath, bool is_augmented) throw(Exception){

    // For 1DLatLon, we will use latitude as the coordinate variable
    set <string>::iterator its;
    set <string> tempvardimnamelist = cfswath->vardimnames;
    string EOS5SWATHPATH ="/HDFEOS/SWATHS/";
    string fslash_str ="/";
    string THIS_EOS5SWATHPATH = EOS5SWATHPATH + cfswath->name + fslash_str;
    bool find_lat = false;
#if 0
for (its = tempvardimnamelist.begin(); its != tempvardimnamelist.end(); ++its)
cerr<<"Dimension name befor latitude " << *its << endl;
#endif

    // Find latitude and assign to the coordinate variable
    for (vector<Var *>::iterator irv = this->vars.begin();
         irv != this->vars.end(); ++irv) {
        if (SWATH == Get_Var_EOS5_Type(*irv) &&
           ((*irv)->fullpath.size() > THIS_EOS5SWATHPATH.size())) {

            string var_swath_name = Obtain_Var_EOS5Type_GroupName(*irv,SWATH);
            if ((var_swath_name == cfswath->name) && ((*irv)->name == "Latitude")) {

                //Find it, create a coordinate variable.
                EOS5CVar *EOS5cvar = new EOS5CVar(*irv);

                // Still keep the original dimension name to avoid the nameclashing when
                // one grid and one swath and one za occur in the same file
                EOS5cvar->cfdimname = ((*irv)->dims)[0]->name;
                EOS5cvar->cvartype = CV_EXIST;
                EOS5cvar->eos_type = SWATH;

                // Save this cv to the cv vector
                this->cvars.push_back(EOS5cvar);

                // Remove this var from the var vector since it becomes a cv.
                delete(*irv);
                this->vars.erase(irv);
                irv--;
                find_lat = true;
                break;
            } // if ((var_swath_name == cfswath->name) && ...
        } // if (SWATH == Get_Var_EOS5_Type(*irv) &&
    } // for (vector<Var *>::iterator irv = this->vars.begin() ...
     
    for (its = tempvardimnamelist.begin(); its != tempvardimnamelist.end(); ++its) {
        for (vector<EOS5CVar *>::iterator irv = this->cvars.begin();
                irv != this->cvars.end(); ++irv) {
            if (((*irv)->name == "Latitude") && (*irv)->cfdimname == (*its)) {
                tempvardimnamelist.erase(its);
                break;
            }
        }
    }

#if 0
for (its = tempvardimnamelist.begin(); its != tempvardimnamelist.end(); ++its) 
cerr<<"Dimension name afte latitude " << *its << endl;
#endif

    Handle_NonLatLon_Swath_CVar(cfswath,tempvardimnamelist);

    // Remove the added variables during the augmentation process
    if (true == is_augmented) {
        for (vector<Var *>::iterator irv = this->vars.begin();
                irv != this->vars.end(); ++irv) {

            if (SWATH == Get_Var_EOS5_Type(*irv) &&
                        ((*irv)->fullpath.size() > THIS_EOS5SWATHPATH.size())) {
                string var_path_after_swathname = (*irv)->fullpath.substr(THIS_EOS5SWATHPATH.size());
//cerr<<"var_path_after_swathname "<<var_path_after_swathname <<endl;
                if (var_path_after_swathname == (*irv)->name) {
                    delete(*irv);
                    this->vars.erase(irv);
                    irv--;
                }
            }
        }
    } // if (true == is_augmented)
}

void EOS5File::Handle_Single_2DLatLon_Swath_CVar(EOS5CFSwath *cfswath, bool is_augmented) throw(Exception){

    // For 2DLatLon, we will use both latitude and longitude as the coordinate variables
    set <string>::iterator its;
    set <string> tempvardimnamelist = cfswath->vardimnames;
    string EOS5SWATHPATH ="/HDFEOS/SWATHS/";
    string fslash_str ="/";
    string THIS_EOS5SWATHPATH = EOS5SWATHPATH + cfswath->name + fslash_str;
    bool find_lat = false;
    bool find_lon = false;

#if 0
for (its = tempvardimnamelist.begin(); its != tempvardimnamelist.end(); ++its)
cerr<<"Dimension name befor latitude " << *its << endl;
#endif

    // Find latitude and assign to the coordinate variable
    for (vector<Var *>::iterator irv = this->vars.begin();
                    irv != this->vars.end(); ++irv) {
        if (SWATH == Get_Var_EOS5_Type(*irv) &&
                  ((*irv)->fullpath.size() > THIS_EOS5SWATHPATH.size())) {
            string var_swath_name = Obtain_Var_EOS5Type_GroupName(*irv,SWATH);
            if ( (var_swath_name == cfswath->name) && ((*irv)->name == "Latitude")) {

                //Find it, create a coordinate variable.
                EOS5CVar *EOS5cvar = new EOS5CVar(*irv);

                // Still keep the original dimension name to avoid the nameclashing when
                // one grid and one swath and one za occur in the same file
                EOS5cvar->cfdimname = ((*irv)->dims)[0]->name;
                EOS5cvar->cvartype = CV_EXIST;
                EOS5cvar->eos_type = SWATH;
                EOS5cvar->is_2dlatlon = true;

                // Save this cv to the cv vector
                this->cvars.push_back(EOS5cvar);

                // Remove this var from the var vector since it becomes a cv.
                delete(*irv);
                this->vars.erase(irv);
                irv--;
                find_lat = true;
            } // if ( (var_swath_name == cfswath->name) && ...
            else if ( (var_swath_name == cfswath->name) && ((*irv)->name == "Longitude")) {

                //Find it, create a coordinate variable.
                EOS5CVar *EOS5cvar = new EOS5CVar(*irv);

                // Still keep the original dimension name to avoid the nameclashing when
                // one grid and one swath and one za occur in the same file
                EOS5cvar->cfdimname = ((*irv)->dims)[1]->name;
                EOS5cvar->cvartype = CV_EXIST;
                EOS5cvar->eos_type = SWATH;
                EOS5cvar->is_2dlatlon = true;

                // Save this cv to the cv vector
                this->cvars.push_back(EOS5cvar);

                // Remove this var from the var vector since it becomes a cv.
                delete(*irv);
                this->vars.erase(irv);
                irv--;
                find_lon = true;

            } // else if ( (var_swath_name == cfswath->name) && ...
        } // if (SWATH == Get_Var_EOS5_Type(*irv) && ...

        if (true == find_lat && true == find_lon) break;
    } // for (vector<Var *>::iterator irv = this->vars.begin();

     
    for (its = tempvardimnamelist.begin(); its != tempvardimnamelist.end(); ++its) {
         for (vector<EOS5CVar *>::iterator irv = this->cvars.begin();
                irv != this->cvars.end(); ++irv) {
            if (((*irv)->name == "Latitude") && (*irv)->cfdimname == (*its)) {
                tempvardimnamelist.erase(its);
                break;
            }

            if (((*irv)->name == "Longitude") && (*irv)->cfdimname == (*its)) {
                tempvardimnamelist.erase(its);
                break;
            }
        }
    }

#if 0
for (its = tempvardimnamelist.begin(); its != tempvardimnamelist.end(); ++its) 
cerr<<"Dimension name afte latitude " << *its << endl;
#endif

    Handle_NonLatLon_Swath_CVar(cfswath,tempvardimnamelist);

    // Remove the added variables during the augmentation process
    // For Swath, we don't want to keep the augmented files. This is because 
    // some aura files assign the dimensional scale as zero.
    // We will actively check the new NASA HDF-EOS5 products and will 
    // revise the following section as needed. KY 2012-03-09
    if (true == is_augmented) {
        for (vector<Var *>::iterator irv = this->vars.begin();
                irv != this->vars.end(); ++irv) {

            if (SWATH == Get_Var_EOS5_Type(*irv) &&
                        ((*irv)->fullpath.size() > THIS_EOS5SWATHPATH.size())) {
                string var_path_after_swathname = (*irv)->fullpath.substr(THIS_EOS5SWATHPATH.size());
                if (var_path_after_swathname == (*irv)->name) {
                    delete(*irv);
                    this->vars.erase(irv);
                    irv--;
                }
            }
        }
    }
}

void EOS5File::Handle_NonLatLon_Swath_CVar(EOS5CFSwath *cfswath, set<string>& tempvardimnamelist) throw(Exception) {

    // First check if we have existing coordinate variable
    set <string>::iterator its;
    int num_dimnames = tempvardimnamelist.size();
    bool has_dimnames = true;
    for (its = tempvardimnamelist.begin(); its != tempvardimnamelist.end(); ++its) {
        if (cfswath->dnames_to_geo1dvnames.find(*its) !=cfswath->dnames_to_geo1dvnames.end()){
            for (vector<Var *>::iterator irv = this->vars.begin();
                    has_dimnames && (irv != this->vars.end()); ++irv) {

                // We need to check if this var is a swath and use "newname"
                // of var to check the dnames_to_1dvnames since it is 
                // possible to have name clashings for the "name" of a var.
                if (SWATH == Get_Var_EOS5_Type(*irv) &&
                    (*irv)->newname == (cfswath->dnames_to_geo1dvnames)[*its]) {

                    //Find it, create a coordinate variable.
                    EOS5CVar *EOS5cvar = new EOS5CVar(*irv);
    
                    // Still keep the original dimension name to avoid the nameclashing when
                    // one grid and one swath and one za occur in the same file
                    EOS5cvar->cfdimname = *its;
                    EOS5cvar->cvartype = CV_EXIST;
                    EOS5cvar->eos_type = SWATH;

                    // Save this cv to the cv vector
                    this->cvars.push_back(EOS5cvar);
        
                    // Remove this var from the var vector since it becomes a cv.
                    delete(*irv);
                    this->vars.erase(irv);
                    irv--;
                    num_dimnames--;
                    if (0 == num_dimnames) 
                        has_dimnames = false;
                } // if (SWATH == Get_Var_EOS5_Type(*irv) && ...)
            } // for (vector<Var *>::iterator irv = this->vars.begin(); ...
        } // if (cfswath->dnames_to_geo1dvnames.find(*its) ....
    } // for (its = tempvardimnamelist.begin()...

    // Remove the dimension name that finds the cooresponding variables from the tempvardimlist.
    for (vector<EOS5CVar *>::iterator irv = this->cvars.begin();
                irv != this->cvars.end(); ++irv) {
         its = tempvardimnamelist.find((*irv)->cfdimname);
         if (its != tempvardimnamelist.end()) tempvardimnamelist.erase(its);
    }

    // Check if some attributes have CV information for some special products
    // Currently TES needs to be handled carefully
    Handle_Special_NonLatLon_Swath_CVar(cfswath, tempvardimnamelist);

    // Remove the dimension name that finds the cooresponding variables from the tempvardimlist.
    for (vector<EOS5CVar *>::iterator irv = this->cvars.begin();
                irv != this->cvars.end(); ++irv) {
         its = tempvardimnamelist.find((*irv)->cfdimname);
         if (its != tempvardimnamelist.end()) 
            tempvardimnamelist.erase(its);
    }

    // Second: Some dimension names still need to find CVs, create the missing CVs
    for (its = tempvardimnamelist.begin(); its != tempvardimnamelist.end(); ++its) {

        EOS5CVar *EOS5cvar = new EOS5CVar();
        Create_Missing_CV(cfswath,EOS5cvar,*its,SWATH,this->eos5cfswaths.size());
        this->cvars.push_back(EOS5cvar);

    }
}


void EOS5File::Handle_Special_NonLatLon_Swath_CVar(EOS5CFSwath *cfswath, set<string>& tempvardimnamelist) throw(Exception) {

    // We have no choice but hard-code this one. 
    // TES swath puts "Pressure" as the VerticalCoordinate but doesn't provide "Pressure" values.
    // Moreover, the number of pressure level(66) is one less than the total number of corresponding dimension size(67)
    // most probably due to the missing pressure level on the ground. To make the handler visualize some
    // TES variables and to follow the general physical sense. We have to add a pressure level by linear interpolation.
    // KY 2012-1-27
    if (true == this->isaura && TES == this->aura_name) {

        string eos5_swath_group_name ="/HDFEOS/SWATHS/" + cfswath->name;
        string eos5_vc_attr_name = "VerticalCoordinate";
        string eos5_pre_attr_name = "Pressure";
        bool has_vc_attr = false;
        Group *vc_group = NULL;

        // 1. Check if having the "VerticalCoordinate" attribute in this swath and the attribute is "Pressure".
        for (vector<Group *>::iterator irg = this->groups.begin();
                irg != this->groups.end(); ++irg) {
            if (eos5_swath_group_name ==(*irg)->path){
                for (vector<Attribute *>::iterator ira = (*irg)->attrs.begin();
                    ira != (*irg)->attrs.end(); ++ira) {
                    if (eos5_vc_attr_name == (*ira)->name) {
                        Retrieve_H5_Attr_Value(*ira,(*irg)->path);
                        string attr_value((*ira)->value.begin(),(*ira)->value.end());
                        if (eos5_pre_attr_name == attr_value) {
                            has_vc_attr = true;
                            vc_group = *irg;
                            break;
                        }
                    }
                } // for (vector<Attribute *>:: iterator ira =(*irg)->attrs.begin(); ...
                if (true == has_vc_attr) 
                    break;
            } // if (eos5_swath_group_name ==(*irg)->path)
        } // for (vector<Group *>::iterator irg = this->groups.begin(); ...

                
        // 2. Check if having the "Pressure" attribute and if the attribute size is 1 less than
        // the dimension size of "nLevels". If yes,
        // add one pressure value by using the nearest neighbor value. This value should be the first value
        // of the "Pressure" attribute.
        // Another special part of the TES file is that dimension name nLevels is used twice in some variables
        // float foo[...][nLevels][nLevels]. To make the variable visualized by tools, the dimension name
        // needs to be changed and the coordinate variable needs to separately created. Note this is not
        // against CF conventions. However, the popular tools are not happy with the duplicate dimension names 
        // in a variable.
        // Though may not cover 100% cases, searching the string after the last forward slash and see if
        // it contains nLevels should catch 99% memebers of  the "nLevels" family. We will then create the
        // corresponding coordinate variables.

        // 2.1. Check if we have the dimension name called "nLevels" for this swath
        if (true == has_vc_attr) {
            string dimname_candidate = "/SWATHS/"+cfswath->name + "/nLevels";
            set<string>:: iterator it;
            for (it = tempvardimnamelist.begin(); it!=tempvardimnamelist.end();++it) {
                if ((*it).find(dimname_candidate) != string::npos) {
                    hsize_t dimsize_candidate = 0;
                    if ((cfswath->dimnames_to_dimsizes).find(*it) !=
                        (cfswath->dimnames_to_dimsizes).end())  
                        dimsize_candidate = cfswath->dimnames_to_dimsizes[*it];
                    else throw2("Cannot find the dimension size of the dimension name ",*it);

                    // Note: we don't have to use two loops to create the coordinate variables.
                    // However, there are only 3-4 attributes for this group and so far TES has only 
                    // one additional nLevels.
                    // So essentially the following loop doesn't hurt the performance.
                    // KY 2012-2-1
                    for (vector<Attribute *>::iterator ira = vc_group->attrs.begin();
                        ira != vc_group->attrs.end(); ++ira) {
                        if ((eos5_pre_attr_name == (*ira)->name) && ((*ira)->count == (dimsize_candidate-1))) {

                            // Should change the attr_value from char type to float type when reading the data
                            // Here just adding a coordinate variable by using this name.
                            EOS5CVar *EOS5cvar = new EOS5CVar();
                            string reduced_dimname = HDF5CFUtil::obtain_string_after_lastslash(*it);
                            string orig_dimname = "nLevels";
                            if ("nLevels" == reduced_dimname) 
                                EOS5cvar->name      = eos5_pre_attr_name + "_CV";
                            else // the dimname will be ..._CV_1 etc.
                                EOS5cvar->name = eos5_pre_attr_name +"_CV"+ reduced_dimname.substr(orig_dimname.size());
                            Create_Added_Var_NewName_FullPath(SWATH,cfswath->name,EOS5cvar->name,EOS5cvar->newname,EOS5cvar->fullpath);
                            EOS5cvar->rank = 1;
                            EOS5cvar->dtype = (*ira)->dtype;
                            Dimension *eos5cvar_dim = new Dimension(dimsize_candidate);
                            eos5cvar_dim->name = *it;
                            if (1 == this->eos5cfswaths.size()) 
                                eos5cvar_dim->newname = reduced_dimname;
                            else 
                                eos5cvar_dim->newname = eos5cvar_dim->name;
                            
                            EOS5cvar->dims.push_back(eos5cvar_dim);
                            EOS5cvar->cvartype = CV_SPECIAL;
                            EOS5cvar->cfdimname = eos5cvar_dim->name;
                            EOS5cvar->eos_type = SWATH;

                            // Save this cv to the cv vector
                            this->cvars.push_back(EOS5cvar);
                        }// if ((eos5_pre_attr_name == (*ira)->name) && ...
                    }// for (vector<Attribute *>::iterator ira = vc_group->attrs.begin();
                } // if ((*it).find(dimname_candidate) != string::npos)
            } // for (it = tempvardimnamelist.begin(); ...
        } // if (true == has_vc_attr) ...
    } // if (true == this->isaura && ...
}
   

    
void EOS5File::Handle_Za_CVar(bool isaugmented) throw(Exception){

    // We are not supporting non-augmented zonal average HDF-EOS5 product now. KY-2012-1-20
    if(false == isaugmented) 
        return;

    for (vector <EOS5CFZa *>::iterator irv = this->eos5cfzas.begin();
            irv !=this->eos5cfzas.end();++irv)
        Handle_Single_Augment_CVar(*irv,ZA);
    
}

void EOS5File::Adjust_Var_Dim_NewName_Before_Flattening() throw(Exception) {

    int num_grids =this->eos5cfgrids.size();
    int num_swaths = this->eos5cfswaths.size();
    int num_zas = this->eos5cfzas.size();

    bool mixed_eos5typefile = false;

    // Check if this file mixes grid,swath and zonal average
    if (((num_grids > 0) && (num_swaths > 0)) ||
        ((num_grids > 0) && (num_zas > 0)) ||
        ((num_swaths >0) && (num_zas > 0)))
        mixed_eos5typefile = true;

    // This file doesn't mix swath, grid and zonal average
    for (vector<Var *>::iterator irv = this->vars.begin();
                irv != this->vars.end(); ++irv) 
        Adjust_Per_Var_Dim_NewName_Before_Flattening(*irv,mixed_eos5typefile,num_grids,num_swaths,num_zas);


    for (vector<EOS5CVar *>::iterator irv = this->cvars.begin();
                irv != this->cvars.end(); ++irv) 
        Adjust_Per_Var_Dim_NewName_Before_Flattening(*irv,mixed_eos5typefile,num_grids,num_swaths,num_zas);
#if 0
for (vector<EOS5CVar *>::iterator irv = this->cvars.begin();
                irv != this->cvars.end(); ++irv) {
    for (vector<Dimension *>::iterator ird = (*irv)->dims.begin();
        ird !=(*irv)->dims.end(); ++ird) {
cerr<<"eos5svar dimension new name "<<(*ird)->newname <<endl;
    }
}
#endif

    Adjust_SharedLatLon_Grid_Var_Dim_Name();
    
}

template <class T>
void EOS5File::Adjust_Per_Var_Dim_NewName_Before_Flattening(T* var,bool mixed_eos5type,int num_grids,int num_swaths,int num_zas)
throw(Exception) {

    string eos5typestr;
    EOS5Type vartype = Get_Var_EOS5_Type(var);
    switch (vartype) {
            
        case GRID:
        {
            eos5typestr = "/GRIDS/";
            if (false == mixed_eos5type) {
                if (0 == num_grids) 
                    var->newname = ((1 == this->orig_num_grids)?var->name:
                                    var->newname.substr(eos5typestr.size()));
                else 
                    var->newname = ((1 == num_grids)?var->name:
                                    var->newname.substr(eos5typestr.size()));
                // Dimension newname is unlike Var newname, when num_grids is equal to 1, the
                // newname is Dimension name already. So we don't need to do anything with
                // the dimension newname when the num_grids is 1. The main reason we handle
                // the var newname and the dimension newname differently is that the variable name is
                // more critical for users to pick up the meanings of that variable. So we would like
                // to work hard to keep the original form. However, the dimension name is mainly used to
                // generate the coordinate variables. So the different usage makes us relax the dimension
                // name a little bit. This is an example of end-user priority driven implementation.
                // KY 2012-1-24
                // Just receive a user request: the dimension name is also very important.
                // So a bunch of code has been updated. For number of grid/swath/za = 1, I still maintain
                // the newname to be the same as the last part of the dim name. Hopefully this 
                // will handle the current HDF-EOS5 products. Improvement for complicate HDF-EOS5 products
                // will be supported as demanded in the future. KY 2012-1-26
                if (num_grids > 1) {
                    for (vector<Dimension *> ::iterator ird = var->dims.begin();
                         ird !=var->dims.end();ird++) {
                        if((*ird)->newname.size() <= eos5typestr.size())
                            throw5("The size of the dimension new name ",(*ird)->newname, "of variable ",var->newname,
                                      " is too small");
                        (*ird)->newname = (*ird)->newname.substr(eos5typestr.size());
                    }
                }
            } // if(false == mixed_eos5type)
            else {
                // No need to set the dimension newname for the reason listed above.
                var->newname = ((1 == num_grids)?(eos5typestr + var->name):
                                    var->newname);
            }
        }
        break;
           
        case SWATH:
        {
            eos5typestr = "/SWATHS/";
            if (false == mixed_eos5type) {
                var->newname = ((1 == num_swaths)?var->name:
                                    var->newname.substr(eos5typestr.size()));
                if (num_swaths > 1) {
                    for (vector<Dimension *> ::iterator ird = var->dims.begin();
                            ird !=var->dims.end();ird++) {
                        if((*ird)->newname.size() <= eos5typestr.size())
                               throw5("The size of the dimension new name ",(*ird)->newname, "of variable ",var->newname,
                                      " is too small");
                        (*ird)->newname = (*ird)->newname.substr(eos5typestr.size());
                    }
                }
            }
            else {
                var->newname = ((1 == num_swaths)?(eos5typestr + var->name):
                                    var->newname);
            }
            }
            break;
                  
        case ZA:
        {
            eos5typestr = "/ZAS/";
            if (false == mixed_eos5type) {
                var->newname = ((1 == num_zas)?var->name:
                                var->newname.substr(eos5typestr.size()));
                if (num_zas > 1) {
                    for (vector<Dimension *> ::iterator ird = var->dims.begin();
                        ird !=var->dims.end();ird++) {
                        if((*ird)->newname.size() <= eos5typestr.size())
                            throw5("The size of the dimension new name ",(*ird)->newname, "of variable ",var->newname,
                                      " is too small");
                        (*ird)->newname = (*ird)->newname.substr(eos5typestr.size());
                    }
                }
            }
            else {
                var->newname = ((1 == num_zas)?(eos5typestr + var->name):
                                    var->newname);
            }
        }
            break;
        case OTHERVARS: 
            break;
        default:
            throw1("Non-supported EOS type");
        } // switch(vartype)
    
}

void EOS5File::Adjust_SharedLatLon_Grid_Var_Dim_Name() throw(Exception) {

    // Remove the EOS5 type string("GRIDS") and the GRID Name from 
    // the variable newname and the dimension newname
    // This case won't happen for the current version, but may occur
    // if curviliner grid exists in the file. KY 2012-1-26
    if ((this->eos5cfgrids.size() > 1) &&    
        (0 == this->eos5cfswaths.size()) &&
        (0 == this->eos5cfzas.size()) &&    
        (false == this->grids_multi_latloncvs)){

        // We would like to condense the dimension name and the coordinate variable name for lat/lon.
        string lat_dimname;
        string lat_dimnewname;
        string lon_dimname;
        string lon_dimnewname;
        for (vector<EOS5CVar *>::iterator irv = this->cvars.begin();
             irv !=this->cvars.end(); ++irv) {
            if ("lat" == (*irv)->name || "Latitude" == (*irv)->name) {
                (*irv)->newname = (*irv)->name; 
                lat_dimnewname = (((*irv)->dims)[0])->newname;
                lat_dimnewname = HDF5CFUtil::obtain_string_after_lastslash(lat_dimnewname);
                if (""== lat_dimnewname) 
                     throw2("/ is not included in the dimension new name ",(((*irv)->dims)[0])->newname);
                (((*irv)->dims)[0])->newname = lat_dimnewname;
                lat_dimname = (*irv)->cfdimname;
            }
            else if ("lon" == (*irv)->name || "Longitude" == (*irv)->name) {
                (*irv)->newname = (*irv)->name; 
                string lon_dimnewname = (((*irv)->dims)[0])->newname;
                lon_dimnewname = HDF5CFUtil::obtain_string_after_lastslash(lon_dimnewname);
                if (""== lon_dimnewname) 
                   throw2("/ is not included in the dimension new name ",(((*irv)->dims)[0])->newname);
                (((*irv)->dims)[0])->newname = lon_dimnewname;
                lon_dimname = (*irv)->cfdimname;
            }   
        } // for (vector<EOS5CVar *>::iterator irv = this->cvars.begin(); ...
                    
        for (vector<Var *>::iterator irv = this->vars.begin();
             irv !=this->vars.end(); ++irv) {
            for (vector<Dimension *>::iterator ird = (*irv)->dims.begin();
                ird !=(*irv)->dims.end();++ird) {           
                if ((*ird)->name == lat_dimname) 
                    (*ird)->newname = lat_dimnewname;
                else if((*ird)->name == lon_dimname)
                    (*ird)->newname = lon_dimnewname;
            }
        }
    } // if ((this->eos5cfgrids.size() > 1) && ...
}

void EOS5File::Flatten_Obj_Name(bool include_attr) throw(Exception){

    File::Flatten_Obj_Name(include_attr);

    for (vector<EOS5CVar *>::iterator irv = this->cvars.begin();
                irv != this->cvars.end(); ++irv) {
        (*irv)->newname = get_CF_string((*irv)->newname);

        for (vector<Dimension *>::iterator ird = (*irv)->dims.begin();
                   ird != (*irv)->dims.end(); ++ird) {
            (*ird)->newname = get_CF_string((*ird)->newname);
        }

        if (true == include_attr) {
            for (vector<Attribute *>::iterator ira = (*irv)->attrs.begin();
                        ira != (*irv)->attrs.end(); ++ira) {
                (*ira)->newname = get_CF_string((*ira)->newname);
            }
        }
    } // for (vector<EOS5CVar *>::iterator irv = this->cvars.begin(); ...
}

void EOS5File::Handle_Obj_NameClashing(bool include_attr) throw(Exception) {

    // objnameset will be filled with all object names that we are going to check the name clashing.
    // For example, we want to see if there are any name clashings for all variable names in this file.
    // objnameset will include all variable names. If a name clashing occurs, we can figure out from the set operation immediately.
    set<string>objnameset;
    Handle_EOS5CVar_NameClashing(objnameset);
    File::Handle_GeneralObj_NameClashing(include_attr,objnameset);
    if (true == include_attr) {
        Handle_EOS5CVar_AttrNameClashing();
    }

    if (this->cvars.size() >0) 
       Handle_DimNameClashing();
}

void EOS5File::Handle_EOS5CVar_NameClashing(set<string> &objnameset ) throw(Exception) {

    EOS5Handle_General_NameClashing(objnameset,this->cvars);
}

void EOS5File::Handle_EOS5CVar_AttrNameClashing() throw(Exception) {

    set<string> objnameset;

    for (vector<EOS5CVar *>::iterator irv = this->cvars.begin();
        irv != this->cvars.end(); ++irv) {
        Handle_General_NameClashing(objnameset,(*irv)->attrs);
        objnameset.clear();
    }
}

//class T must have member string newname
template<class T> void
EOS5File::EOS5Handle_General_NameClashing(set <string>&objnameset, vector<T*>& objvec) throw(Exception){

    pair<set<string>::iterator,bool> setret;
    set<string>::iterator iss;

    vector<string> clashnamelist;
    vector<string>::iterator ivs;

    map<int,int> cl_to_ol;
    int ol_index = 0;
    int cl_index = 0;

    class vector<T*>::iterator irv;

    for (irv = objvec.begin();
                irv != objvec.end(); ++irv) {

         setret = objnameset.insert((*irv)->newname);
         if (!setret.second ) {
            clashnamelist.insert(clashnamelist.end(),(*irv)->newname);
            cl_to_ol[cl_index] = ol_index;
            cl_index++;
         }
         ol_index++;
    }

    // Now change the clashed elements to unique elements; 
    // Generate the set which has the same size as the original vector.
    for (ivs=clashnamelist.begin(); ivs!=clashnamelist.end(); ++ivs) {
        int clash_index = 1;
        string temp_clashname = *ivs +'_';
        HDF5CFUtil::gen_unique_name(temp_clashname,objnameset,clash_index);
        *ivs = temp_clashname;
    }

    // Now go back to the original vector, make it unique.
    for (unsigned int i =0; i <clashnamelist.size(); i++)
        objvec[cl_to_ol[i]]->newname = clashnamelist[i];

}

void EOS5File::Handle_DimNameClashing() throw(Exception){

    map<string,string>dimname_to_dimnewname;
    pair<map<string,string>::iterator,bool>mapret;
    set<string> dimnameset;
    vector<Dimension*>vdims;
    set<string> dimnewnameset;
    pair<set<string>::iterator,bool> setret;

    // First: Generate the dimset/dimvar based on coordinate variables.
    for (vector<EOS5CVar *>::iterator irv = this->cvars.begin();
             irv !=this->cvars.end(); ++irv) {
        for (vector <Dimension *>:: iterator ird = (*irv)->dims.begin();
            ird !=(*irv)->dims.end();++ird) {
            setret = dimnameset.insert((*ird)->newname);
            if (setret.second) vdims.push_back(*ird);
        }
    }

    // For some cases, dimension names are provided but there are no corresponding coordinate
    // variables. For now, we will assume no such cases.
    EOS5Handle_General_NameClashing(dimnewnameset,vdims);

    // Third: Make dimname_to_dimnewname map
    for (vector<Dimension*>::iterator ird = vdims.begin();ird!=vdims.end();++ird) {
        mapret = dimname_to_dimnewname.insert(pair<string,string>((*ird)->name,(*ird)->newname));
        if (false == mapret.second) 
            throw4("The dimension name ",(*ird)->name," should map to ",
                                      (*ird)->newname);
    }

    // Fourth: Change the original dimension new names to the unique dimension new names
    for (vector<EOS5CVar *>::iterator irv = this->cvars.begin();
        irv !=this->cvars.end(); ++irv)
        for (vector <Dimension *>:: iterator ird = (*irv)->dims.begin();
            ird!=(*irv)->dims.end();++ird)
           (*ird)->newname = dimname_to_dimnewname[(*ird)->name];

    for (vector<Var *>::iterator irv = this->vars.begin();
         irv != this->vars.end(); ++irv)
        for (vector <Dimension *>:: iterator ird = (*irv)->dims.begin();
            ird !=(*irv)->dims.end();++ird)
           (*ird)->newname = dimname_to_dimnewname[(*ird)->name];

}

void EOS5File::Set_COARDS_Status() throw(Exception) {

    iscoard = true;
    for (vector <EOS5CFGrid *>::iterator irg = this->eos5cfgrids.begin();
                irg != this->eos5cfgrids.end(); ++irg) {
        if (false == (*irg)->has_1dlatlon) {
            if (false == (*irg)->has_nolatlon || (HE5_GCTP_GEO != (*irg)->eos5_projcode))
                iscoard = false;
            break;
        }
    }
        
    if (true == iscoard) {
        for (vector <EOS5CFSwath *>::iterator irg = this->eos5cfswaths.begin();
                irg != this->eos5cfswaths.end(); ++irg) {
            if (false == (*irg)->has_1dlatlon) {
                iscoard = false;
                break;
            }
        }
    }
}

void EOS5File::Adjust_Attr_Info() throw(Exception) {

    if (true == this->isaura){
        Adjust_Attr_Name();
        Adjust_Attr_Value();
    }
    else 
        Handle_EOS5CVar_Unit_Attr();
    

}
void EOS5File::Adjust_Attr_Name() throw(Exception) {

    for (vector<Var*>::iterator irv = this->vars.begin();
        irv !=this->vars.end(); ++irv) {
        for(vector <Attribute*>::iterator ira = (*irv)->attrs.begin();
                    ira != (*irv)->attrs.end(); ++ira) {
            if (eos5_to_cf_attr_map.find((*ira)->name) != eos5_to_cf_attr_map.end())
                (*ira)->newname = eos5_to_cf_attr_map[(*ira)->name];
            
        }
    }

    for (vector<EOS5CVar *>::iterator irv = this->cvars.begin();
        irv !=this->cvars.end(); ++irv) {
        for(vector <Attribute*>::iterator ira = (*irv)->attrs.begin();
                    ira != (*irv)->attrs.end(); ++ira) {
            if (eos5_to_cf_attr_map.find((*ira)->name) != eos5_to_cf_attr_map.end()) 
                (*ira)->newname = eos5_to_cf_attr_map[(*ira)->name];
            
        }
    }
}

void EOS5File::Adjust_Attr_Value() throw(Exception) {

    // Handle Units
    Handle_EOS5CVar_Unit_Attr();
    Handle_EOS5CVar_Special_Attr();
    
    // Handle Time. This is just for Aura files. 
    string time_cf_units_value = "seconds since 1993-01-01";
    for (vector<Var*>::iterator irv = this->vars.begin();
        irv !=this->vars.end(); irv++) {
        if(((*irv)->name == "Time") || ((*irv)->name == "nTimes")) {
            for(vector <Attribute*>::iterator ira = (*irv)->attrs.begin();
                ira != (*irv)->attrs.end(); ira++) {
                if ("units" == (*ira)->name){
                    Retrieve_H5_Attr_Value(*ira,(*irv)->fullpath);
                    string units_value((*ira)->value.begin(),(*ira)->value.end());
                    if (time_cf_units_value != units_value) {

                        units_value = time_cf_units_value;
                        (*ira)->value.resize(units_value.size());
                        if (H5FSTRING == (*ira)->dtype)
                            (*ira)->fstrsize = units_value.size();
                             // strsize is used by both fixed and variable length strings.
                        (*ira)->strsize.resize(1);
                        (*ira)->strsize[0] = units_value.size();

                        copy(units_value.begin(),units_value.end(),(*ira)->value.begin());
                    }
                    break;
                } // if ("units" == (*ira)->name)
            } // for(vector <Attribute*>::iterator ira = (*irv)->attrs.begin();
        } // if(((*irv)->name == "Time") || ((*irv)->name == "nTimes"))
    } // for (vector<Var*>::iterator irv = this->vars.begin()...
}

void EOS5File::Handle_EOS5CVar_Special_Attr() throw(Exception) {

    if (true == this->isaura && MLS == this->aura_name) { 

        const string File_attr_group_path = "/HDFEOS/ADDITIONAL/FILE_ATTRIBUTES";
        const string PCF1_attr_name = "PCF1";
        bool find_group = false;
        bool find_attr = false;
        for (vector<Group*>::iterator it_g = this->groups.begin();
                it_g != this->groups.end(); ++it_g) {
            if (File_attr_group_path == (*it_g)->path) {
                find_group = true;
                for (vector<Attribute *>::iterator ira = (*it_g)->attrs.begin();
                        ira != (*it_g)->attrs.end(); ++ira) {
                    if (PCF1_attr_name == (*ira)->name) {
                        Retrieve_H5_Attr_Value(*ira,(*it_g)->path);
                        string pcf_value((*ira)->value.begin(),(*ira)->value.end());
                        HDF5CFDAPUtil::replace_double_quote(pcf_value);
                        (*ira)->value.resize(pcf_value.size());
                        if (H5FSTRING == (*ira)->dtype)
                            (*ira)->fstrsize = pcf_value.size();
                        // strsize is used by both fixed and variable length strings.
                        (*ira)->strsize.resize(1);
                        (*ira)->strsize[0] = pcf_value.size();

                        copy(pcf_value.begin(),pcf_value.end(),(*ira)->value.begin());
                        find_attr = true;
                        break;
                    } // if (PCF1_attr_name == (*ira)->name)  
                }// for (vector<Attribute *>::iterator ira = (*it_g)->attrs.begin()
            } // if (File_attr_group_path == (*it_g)->path)
            if(true == find_group && true == find_attr) 
                break;
        }// for (vector<Group*>::iterator it_g = this->groups.begin() ... 
    } // if (true == this->isaura && MLS == this->aura_name)
}

     
void EOS5File::Handle_EOS5CVar_Unit_Attr() throw(Exception) {

    string unit_attrname = "units";
    string nonll_cf_level_attrvalue ="level";
    string lat_cf_unit_attrvalue ="degrees_north";
    string lon_cf_unit_attrvalue ="degrees_east";
    string tes_cf_pre_attrvalue ="hPa";


    for (vector<EOS5CVar *>::iterator irv = this->cvars.begin();
        irv !=this->cvars.end(); ++irv) {
        switch((*irv)->cvartype) {
            case CV_EXIST:
            case CV_MODIFY:
            {
                for(vector <Attribute*>::iterator ira = (*irv)->attrs.begin();
                    ira != (*irv)->attrs.end(); ++ira) {
                    if ((*ira)->newname ==unit_attrname) {
                        Retrieve_H5_Attr_Value(*ira,(*irv)->fullpath);
                        string units_value((*ira)->value.begin(),(*ira)->value.end());
                        if ((lat_cf_unit_attrvalue !=units_value) && 
                            (((*irv)->name == "Latitude") ||
                             ((this->eos5cfzas.size() > 0) && ((*irv)->name == "nLats")))) {
                             units_value = lat_cf_unit_attrvalue;
                             (*ira)->value.resize(units_value.size());
                             if (H5FSTRING == (*ira)->dtype) 
                                 (*ira)->fstrsize = units_value.size();
                             // strsize is used by both fixed and variable length strings.
                             (*ira)->strsize.resize(1);
                             (*ira)->strsize[0] = units_value.size();
                             copy(units_value.begin(),units_value.end(),(*ira)->value.begin());
                             string tempstring((*ira)->value.begin(),(*ira)->value.end());
                        }
                        else if ((lon_cf_unit_attrvalue !=units_value) && (*irv)->name == "Longitude"){
                             units_value = lon_cf_unit_attrvalue;
                             (*ira)->value.resize(units_value.size());
                             if (H5FSTRING == (*ira)->dtype)
                                 (*ira)->fstrsize = units_value.size();
                             // strsize is used by both fixed and variable length strings.
                                 (*ira)->strsize.resize(1);
                                 (*ira)->strsize[0] = units_value.size();

                             copy(units_value.begin(),units_value.end(),(*ira)->value.begin());
                        }
                        break;
                    } // if ((*ira)->newname ==unit_attrname)
                }
            }
            break;

            case CV_LAT_MISS:
            {
                Attribute * attr = new Attribute();
                Add_Str_Attr(attr,unit_attrname,lat_cf_unit_attrvalue);
                (*irv)->attrs.push_back(attr);
            }
            break;
      
            case CV_LON_MISS:
            {
                Attribute * attr = new Attribute();
                Add_Str_Attr(attr,unit_attrname,lon_cf_unit_attrvalue);
                (*irv)->attrs.push_back(attr);
            }
            break;

            case CV_NONLATLON_MISS:
            {
                Attribute * attr = new Attribute();
                Add_Str_Attr(attr,unit_attrname,nonll_cf_level_attrvalue);
                (*irv)->attrs.push_back(attr);
            }
            break;
            case CV_SPECIAL:
            {
                if (true == this->isaura && TES == this->aura_name) {
                    Attribute * attr = new Attribute();
                    Add_Str_Attr(attr,unit_attrname,tes_cf_pre_attrvalue);
                    (*irv)->attrs.push_back(attr);
                }
            }
            break;
            default:
                throw1("Non-supported Coordinate Variable Type.");
        } // switch((*irv)->cvartype)
    } // for (vector<EOS5CVar *>::iterator irv = this->cvars.begin() ...
}
 

void EOS5File::Adjust_Dim_Name() throw(Exception){

    if (false == this->iscoard) 
        return;
    else {
        for (vector<EOS5CVar *>::iterator irv = this->cvars.begin();
             irv !=this->cvars.end(); irv++) {
            if ((*irv)->dims.size()!=1) 
                throw3("Coard coordinate variable ",(*irv)->name, "is not 1D");
            if ((*irv)->newname != (((*irv)->dims)[0]->newname)) {
                ((*irv)->dims)[0]->newname = (*irv)->newname;

                // For all variables that have this dimension,the dimension newname should also change.
                for (vector<Var*>::iterator irv2 = this->vars.begin();
                    irv2 != this->vars.end(); irv2++) {
                    for (vector<Dimension *>::iterator ird = (*irv2)->dims.begin();
                        ird !=(*irv2)->dims.end(); ird++) {
                        // This is the key, the dimension name of this dimension 
                        // should be equal to the dimension name of the coordinate variable.
                        // Then the dimension name matches and the dimension name should be changed to
                        // the new dimension name.
                        if ((*ird)->name == ((*irv)->dims)[0]->name)
                            (*ird)->newname = ((*irv)->dims)[0]->newname;
                    }
                }
            }// if ((*irv)->newname != (((*irv)->dims)[0]->newname))
        } // for (vector<EOS5CVar *>::iterator irv = this->cvars.begin();
   }// else
}


void
EOS5File:: Add_Supplement_Attrs(bool add_path) throw(Exception) {

    if(true == add_path) {

        File::Add_Supplement_Attrs(add_path);

         // Adding variable original name(origname) and full path(fullpath)
        for (vector<EOS5CVar *>::iterator irv = this->cvars.begin();
                irv != this->cvars.end(); ++irv) {
            if (((*irv)->cvartype == CV_EXIST) || ((*irv)->cvartype == CV_MODIFY)) {
                Attribute * attr = new Attribute();
                const string varname = (*irv)->name;
                const string attrname = "origname";
                Add_Str_Attr(attr,attrname,varname);
                (*irv)->attrs.push_back(attr);
            }
        }

        for (vector<EOS5CVar *>::iterator irv = this->cvars.begin();
                irv != this->cvars.end(); ++irv) {
            if (((*irv)->cvartype == CV_EXIST) || ((*irv)->cvartype == CV_MODIFY)) {
                Attribute * attr = new Attribute();
                const string varname = (*irv)->fullpath;
                const string attrname = "fullnamepath";
                Add_Str_Attr(attr,attrname,varname);
                (*irv)->attrs.push_back(attr);
            }
        }
    } // if(true == add_path)

    if(true == this->iscoard ) {
        for (vector<EOS5CVar *>::iterator irv = this->cvars.begin();
                irv != this->cvars.end(); ++irv) {
            if (((*irv)->cvartype == CV_EXIST) || ((*irv)->cvartype == CV_MODIFY)) {
                Attribute * attr = new Attribute();
                const string attrname = "orig_dimname";
                string orig_dimname = (((*irv)->dims)[0])->name;
                orig_dimname = HDF5CFUtil::obtain_string_after_lastslash(orig_dimname);
                if ("" == orig_dimname) 
                    throw2("wrong dimension name ",orig_dimname);
                if(orig_dimname.find("FakeDim") != string::npos) 
                    orig_dimname = "";
                Add_Str_Attr(attr,attrname,orig_dimname);
                (*irv)->attrs.push_back(attr);
            }
        } // for (vector<EOS5CVar *>::iterator irv = this->cvars.begin()

        for (vector<Var *>::iterator irv = this->vars.begin();
                irv != this->vars.end(); ++irv) {
         
            if ((*irv)->dims.size() >0 ) {
                Attribute * attr = new Attribute();
                if (1 == (*irv)->dims.size()) { 
                    const string attrname = "orig_dimname";
                    string orig_dimname = (((*irv)->dims)[0])->name;
                    if (""==orig_dimname) orig_dimname="NoDimName";
                    else orig_dimname = HDF5CFUtil::obtain_string_after_lastslash(orig_dimname);
                    if(orig_dimname.find("FakeDim") != string::npos) orig_dimname = "NoDimName";
                    Add_Str_Attr(attr,attrname,orig_dimname);
                }
                else {
                    const string attrname ="orig_dimname_list";
                    string orig_dimname_list;
                    for (vector<Dimension *>::iterator ird = (*irv)->dims.begin();
                        ird != (*irv)->dims.end(); ++ird) {
                        string orig_dimname =(*ird)->name;
                        if (""==orig_dimname) orig_dimname="NoDimName";
                        else orig_dimname = HDF5CFUtil::obtain_string_after_lastslash((*ird)->name);
                        if(orig_dimname.find("FakeDim") != string::npos) orig_dimname = "NoDimName";
                        orig_dimname_list = orig_dimname + " ";
                    }
                    Add_Str_Attr(attr,attrname,orig_dimname_list);
                }
                (*irv)->attrs.push_back(attr);
            } //  if ((*irv)->dims.size() >0 ) 
        } // for (vector<Var *>::iterator irv = this->vars.begin();
    } // if(true == this->iscoard )

}

void EOS5File:: Handle_Coor_Attr() {

    string co_attrname = "coordinates";
    string co_attrvalue="";

    if (iscoard) 
        return;

    for (vector<Var *>::iterator irv = this->vars.begin();
                irv != this->vars.end(); ++irv) {

        for (vector<Dimension *>::iterator ird = (*irv)->dims.begin();
                ird != (*irv)->dims.end(); ++ ird) {
            for (vector<EOS5CVar *>::iterator ircv = this->cvars.begin();
                ircv != this->cvars.end(); ++ircv) {
                if ((*ird)->name == (*ircv)->cfdimname)
                    co_attrvalue = (co_attrvalue.empty())?(*ircv)->newname:co_attrvalue + " "+(*ircv)->newname;
            }
        }
        if (false == co_attrvalue.empty()) {
                Attribute * attr = new Attribute();
                Add_Str_Attr(attr,co_attrname,co_attrvalue);
                (*irv)->attrs.push_back(attr);
        }
        co_attrvalue.clear();
    } // for (vector<Var *>::iterator irv = this->vars.begin(); ...

    // We will check if 2dlatlon coordinate variables exist
    bool has_2dlatlon_cv = false;
    for (vector<EOS5CVar *>::iterator ircv = this->cvars.begin();
                ircv != this->cvars.end(); ++ircv) {
        if (true == (*ircv)->is_2dlatlon) {
            has_2dlatlon_cv = true;
            break;
        }
    }

    if (true == has_2dlatlon_cv) {

        string dimname1, dimname2;
        for (vector<EOS5CVar *>::iterator ircv = this->cvars.begin();
                ircv != this->cvars.end(); ++ircv) {
            if (true == (*ircv)->is_2dlatlon) {
                dimname1 = (((*ircv)->dims)[0])->name;
                dimname2 = (((*ircv)->dims)[1])->name;    
                break;
            }
        }

        int num_latlondims = 0;

        for (vector<Var *>::iterator irv = this->vars.begin();
                irv != this->vars.end(); ++irv) {
            for (vector<Dimension *>::iterator ird = (*irv)->dims.begin();
                ird != (*irv)->dims.end(); ++ ird) {
                if (dimname1 == (*ird)->name) num_latlondims++;
                if (dimname2 == (*ird)->name) num_latlondims++;
            }
            if ((num_latlondims !=0) && (num_latlondims !=2)) {
               // need to remove the coordinates attribute.
                for (vector<Attribute *>::iterator ira = (*irv)->attrs.begin();
                    ira != (*irv)->attrs.end(); ++ira) {
                    if (co_attrname == (*ira)->name) {
                        delete(*ira);
                        (*irv)->attrs.erase(ira);
                         break;
                    }
                }
            }
            num_latlondims = 0;
        } // for (vector<Var *>::iterator irv = this->vars.begin();
    } //  if (true == has_2dlatlon_cv)
}

// This function is from the original requirement of NASA, then
// NASA changes the requirment. Still leave it here for future usage.
#if 0
void EOS5File::Adjust_Special_EOS5CVar_Name() throw(Exception) {

    int num_grids =this->eos5cfgrids.size();
    int num_swaths = this->eos5cfswaths.size();
    int num_zas = this->eos5cfzas.size();

    bool mixed_eos5typefile = false;

    // Check if this file mixes grid,swath and zonal average
    if (((num_grids > 0) && (num_swaths > 0)) ||
        ((num_grids > 0) && (num_zas > 0)) ||
        ((num_swaths >0) && (num_zas > 0)))
        mixed_eos5typefile = true;


    if (false == mixed_eos5typefile) {

        // Grid is very special since all grids may share the same lat/lon.
        // so we also consider this case.

        if ((1 == num_swaths) || ( 1 == num_zas) ||
            (1 == num_grids) || ((num_grids >1) && (this->grids_multi_latloncvs))) {  

            string unit_attrname = "units";
            string nonll_cf_level_attralue ="level";
            string lat_cf_unit_attrvalue ="degrees_north";
            string lon_cf_unit_attrvalue ="degrees_east";
       
            for (vector<EOS5CVar *>::iterator irv = this->cvars.begin();
                irv != this->cvars.end(); irv++){
                switch((*irv)->eos_type) {
                    case CV_EXIST:
                    case CV_MODIFY:
                    case CV_LAT_MISS:
                    case CV_LON_MISS:
                    {
                        for(vector <Attribute*>::iterator ira = (*irv)->attrs.begin();
                            ira != (*irv)->attrs.end(); ira++) {
                            if ((*ira)->name ==unit_attrname) {
                                if ((*ira)->value.size() > 0) {
                                    string units_value((*ira)->value.begin(),(*ira)->value.end());                                 
                                    if (lat_cf_unit_attrvalue ==units_value) (*irv)->newname = "lat";
                                    if (lon_cf_unit_attrvalue ==units_value) (*irv)->newname = "lon";
                                }
                            }
                        }
                    }
                    break;
                    case CV_NONLATLON_MISS:
                    {
                        for(vector <Attribute*>::iterator ira = (*irv)->attrs.begin();
                            ira != (*irv)->attrs.end(); ira++) {
                            if ((*ira)->name ==unit_attrname) {
                                if ((*ira)->value.size() > 0) {
                                    string units_value((*ira)->value.begin(),(*ira)->value.end());                      
                                    if (nonll_cf_level_attralue ==units_value) {
                                        (*irv)->newname = "lev";
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    break;
                    default:
                          throw1("Non-supported coordinate variable type");
               }
            }
        }
    }
}
#endif


template<class T> 
void EOS5File:: Create_Missing_CV(T* eos5data,EOS5CVar *EOS5cvar, const string& dimname, 
                                  EOS5Type eos5type,int num_eos5data) throw(Exception) {

    string reduced_dimname = HDF5CFUtil::obtain_string_after_lastslash(dimname); 
    if ("" == reduced_dimname) throw2("wrong dimension name ",dimname);
    EOS5cvar->name = reduced_dimname;
    Create_Added_Var_NewName_FullPath(eos5type,eos5data->name,EOS5cvar->name,EOS5cvar->newname,EOS5cvar->fullpath);
    EOS5cvar->rank = 1;
    EOS5cvar->dtype = H5INT32;
    hsize_t eos5cvar_dimsize = (eos5data->dimnames_to_dimsizes)[dimname];
    Dimension* eos5cvar_dim = new Dimension(eos5cvar_dimsize);
    eos5cvar_dim->name = dimname;
    if (1 == num_eos5data) 
        eos5cvar_dim->newname = reduced_dimname;
    else eos5cvar_dim->newname = dimname; 

    EOS5cvar->dims.push_back(eos5cvar_dim);
    EOS5cvar->cfdimname = dimname;
    EOS5cvar->cvartype = CV_NONLATLON_MISS;
    EOS5cvar->eos_type = eos5type;
}

void EOS5File::Create_Added_Var_NewName_FullPath(EOS5Type eos5type, const string& eos5_groupname, 
                                                 const string& varname, string &var_newname, 
                                                 string &var_fullpath)  throw(Exception) {

    string fslash_str="/";
    string eos5typestr="";
    string top_eos5_groupname ="/HDFEOS";

    switch(eos5type) {
        case GRID:
        {
            eos5typestr ="/GRIDS/";
            var_newname  = eos5typestr + eos5_groupname + fslash_str +varname;
            var_fullpath = top_eos5_groupname + eos5typestr +eos5_groupname + fslash_str + varname;
        }
        break;

        case SWATH:
        {
            eos5typestr ="/SWATHS/";
            var_newname  = eos5typestr + eos5_groupname + fslash_str +varname;
            var_fullpath = top_eos5_groupname + eos5typestr +eos5_groupname + fslash_str + varname;
 
        }
        break;

        case ZA:
        {
            eos5typestr ="/ZAS/";
            var_newname  = eos5typestr + eos5_groupname + fslash_str +varname;
            var_fullpath = top_eos5_groupname + eos5typestr +eos5_groupname + fslash_str + varname;
 
        }
        break;
        case OTHERVARS:
        default:
            throw1("Non-supported EOS type");
    }
}

void EOS5File:: Handle_SpVar() throw(Exception) {

    if (true == this->isaura && TES == this->aura_name) {
        const string ProHist_full_path = "/HDFEOS/ADDITIONAL/FILE_ATTRIBUTES/ProductionHistory";
        for (vector<Var *>::iterator irv = this->vars.begin();
                irv != this->vars.end(); ++irv) {
            if (ProHist_full_path == (*irv)->fullpath) {
               delete (*irv);
               this->vars.erase(irv);
                break;
            }
        }
    }
}

void EOS5File::Adjust_Obj_Name() throw(Exception) {

}

