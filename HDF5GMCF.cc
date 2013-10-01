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
/// \file HDF5GMCF.cc
/// \brief Implementation of the mapping of NASA generic HDF5 products to DAP by following CF
///
///  It includes functions to 
///  1) retrieve HDF5 metadata info.
///  2) translate HDF5 objects into DAP DDS and DAS by following CF conventions.
///
///
/// \author Muqun Yang <myang6@hdfgroup.org>
///
/// Copyright (C) 2011-2013 The HDF Group
///
/// All rights reserved.

#include "HDF5CF.h"
using namespace HDF5CF;

GMCVar::GMCVar(Var*var) {

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
//cerr <<"dim->name "<< (*ird)->name <<endl;
//cerr <<"dim->newname "<< (*ird)->newname <<endl;
        dim->name = (*ird)->name;
        dim->newname = (*ird)->newname;
        dims.push_back(dim);
    }
    product_type = General_Product;

}
GMSPVar::GMSPVar(Var*var) {

    fullpath = var->fullpath;
    rank  = var->rank;
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
    } // for (vector<Attribute*>::iterator ira = var->attrs.begin();

    for (vector<Dimension*>::iterator ird = var->dims.begin();
        ird!=var->dims.end(); ++ird) {
        Dimension *dim = new Dimension((*ird)->size);
        dim->name = (*ird)->name;
        dim->newname = (*ird)->newname;
        dims.push_back(dim);
    }
}


GMFile::GMFile(const char*path, hid_t file_id, H5GCFProduct product_type, GMPattern gproduct_pattern):
File(path,file_id), product_type(product_type),gproduct_pattern(gproduct_pattern),iscoard(false) 
{


}
GMFile::~GMFile() 
{

    if (!this->cvars.empty()){
        for (vector<GMCVar *>:: const_iterator i= this->cvars.begin(); i!=this->cvars.end(); ++i) {
           delete *i;
        }
    }
}

string GMFile::get_CF_string(string s) {

    if ((General_Product == product_type &&  OTHERGMS == gproduct_pattern) || s[0] !='/') 
        return File::get_CF_string(s);
    else {
        s.erase(0,1);
        return File::get_CF_string(s);
    }
}

void GMFile::Retrieve_H5_Info(const char *path,
                              hid_t file_id, bool include_attr) throw (Exception) {

    // MeaSure SeaWiFS needs the attribute info. to build the dimension name list.
    // So set the include_attr to be true.
    if (product_type == Mea_SeaWiFS_L2 || product_type == Mea_SeaWiFS_L3
        || Mea_Ozone == product_type || General_Product == product_type)  
        File::Retrieve_H5_Info(path,file_id,true);
    else 
        File::Retrieve_H5_Info(path,file_id,include_attr);
}

void GMFile::Retrieve_H5_Supported_Attr_Values() throw (Exception) {

    File::Retrieve_H5_Supported_Attr_Values();
    for (vector<GMCVar *>::iterator ircv = this->cvars.begin();
          ircv != this->cvars.end(); ++ircv) {
          
        if ((CV_EXIST == (*ircv)->cvartype ) || (CV_MODIFY == (*ircv)->cvartype) 
            || (CV_FILLINDEX == (*ircv)->cvartype)){
            for (vector<Attribute *>::iterator ira = (*ircv)->attrs.begin();
                 ira != (*ircv)->attrs.end(); ++ira) {
                Retrieve_H5_Attr_Value(*ira,(*ircv)->fullpath);
            }
        }
    }
    for (vector<GMSPVar *>::iterator irspv = this->spvars.begin();
          irspv != this->spvars.end(); ++irspv) {
          
        for (vector<Attribute *>::iterator ira = (*irspv)->attrs.begin();
              ira != (*irspv)->attrs.end(); ++ira) {
            Retrieve_H5_Attr_Value(*ira,(*irspv)->fullpath);
            Adjust_H5_Attr_Value(*ira);
        }
    }
}

void GMFile::Adjust_H5_Attr_Value(Attribute *attr) throw (Exception) {

    if (product_type == ACOS_L2S) {
        if (("Type" == attr->name) && (H5VSTRING == attr->dtype)) {
            string orig_attrvalues(attr->value.begin(),attr->value.end());
            if (orig_attrvalues != "Signed64") return;
            string new_attrvalues = "Signed32";
            // Since the new_attrvalues size is the same as the orig_attrvalues size
            // No need to adjust the strsize and fstrsize. KY 2011-2-1
            attr->value.clear();
            attr->value.resize(new_attrvalues.size());
            copy(new_attrvalues.begin(),new_attrvalues.end(),attr->value.begin()); 
        }
    } // if (product_type == ACOS_L2S)
}

void GMFile:: Handle_Unsupported_Dtype(bool include_attr) throw(Exception) {

    File::Handle_Unsupported_Dtype(include_attr);
    for (vector<GMCVar *>::iterator ircv = this->cvars.begin();
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
        if (false == HDF5CFUtil::cf_strict_support_type(temp_dtype)) {
            
            // This may need to be checked carefully in the future,
            // My current understanding is that the coordinate variable can
            // be ignored if the corresponding variable is ignored. 
            // Currently we don't find any NASA files in this category.
            // KY 2012-5-21
            delete (*ircv);
            this->cvars.erase(ircv);
            ircv--;
        }
    } // for (vector<GMCVar *>::iterator ircv = this->cvars.begin();
    for (vector<GMSPVar *>::iterator ircv = this->spvars.begin();
                ircv != this->spvars.end(); ++ircv) {

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
        if (false == HDF5CFUtil::cf_strict_support_type(temp_dtype)) {
            delete (*ircv);
            this->spvars.erase(ircv);
            ircv--;
        }
    }// for (vector<GMSPVar *>::iterator ircv = this->spvars.begin();
}

void GMFile:: Handle_Unsupported_Dspace() throw(Exception) {

    File::Handle_Unsupported_Dspace();
    
    if(true == this->unsupported_var_dspace) {
        for (vector<GMCVar *>::iterator ircv = this->cvars.begin();
                ircv != this->cvars.end(); ++ircv) {
            if (true  == (*ircv)->unsupported_dspace ) {
            
                // This may need to be checked carefully in the future,
                // My current understanding is that the coordinate variable can
                // be ignored if the corresponding variable is ignored. 
                // Currently we don't find any NASA files in this category.
                // KY 2012-5-21
                delete (*ircv);
                this->cvars.erase(ircv);
                ircv--;
            }
        } // for (vector<GMCVar *>::iterator ircv = this->cvars.begin();

        for (vector<GMSPVar *>::iterator ircv = this->spvars.begin();
                ircv != this->spvars.end(); ++ircv) {

            if (true == (*ircv)->unsupported_dspace) {
                delete (*ircv);
                this->spvars.erase(ircv);
                ircv--;
            }
        }// for (vector<GMSPVar *>::iterator ircv = this->spvars.begin();
    }// if(true == this->unsupported_dspace) 
}

void GMFile::Add_Dim_Name() throw(Exception){
    
    switch(product_type) {
        case Mea_SeaWiFS_L2:
        case Mea_SeaWiFS_L3:
            Add_Dim_Name_Mea_SeaWiFS();
            break;
        case Aqu_L3:
            Add_Dim_Name_Aqu_L3();
            break;
        case SMAP:
            Add_Dim_Name_SMAP();
            break;
        case ACOS_L2S:
            Add_Dim_Name_ACOS_L2S();
            break;
        case Mea_Ozone:
            Add_Dim_Name_Mea_Ozonel3z();
            break;
        case General_Product:
            Add_Dim_Name_General_Product();
            break;
        default:
           throw1("Cannot generate dim. names for unsupported datatype");
    } // switch(product_type)

// Just for debugging
#if 0
for (vector<Var*>::iterator irv2 = this->vars.begin();
    irv2 != this->vars.end(); irv2++) {
    for (vector<Dimension *>::iterator ird = (*irv2)->dims.begin();
         ird !=(*irv2)->dims.end(); ird++) {
        // cerr<<"Dimension name afet Add_Dim_Name "<<(*ird)->newname <<endl;
    }
}
#endif

}

void GMFile::Add_Dim_Name_Mea_SeaWiFS() throw(Exception){

//cerr<<"coming to Add_Dim_Name_Mea_SeaWiFS"<<endl;
    
    pair<set<string>::iterator,bool> setret;
    if (Mea_SeaWiFS_L3 == product_type) 
        iscoard = true;
    for (vector<Var *>::iterator irv = this->vars.begin();
        irv != this->vars.end(); ++irv) {
        Handle_UseDimscale_Var_Dim_Names_Mea_SeaWiFS_Ozone((*irv));
        for (vector<Dimension *>::iterator ird = (*irv)->dims.begin();
            ird !=(*irv)->dims.end();++ird) { 
            setret = dimnamelist.insert((*ird)->name);
            if (true == setret.second) 
                Insert_One_NameSizeMap_Element((*ird)->name,(*ird)->size);
        }
    } // for (vector<Var *>::iterator irv = this->vars.begin();
 
    if (true == dimnamelist.empty()) 
        throw1("This product should have the dimension names, but no dimension names are found");
}    


void GMFile::Handle_UseDimscale_Var_Dim_Names_Mea_SeaWiFS_Ozone(Var* var)
throw(Exception){

    Attribute* dimlistattr = NULL;
    bool has_dimlist = false;
    bool has_class = false;
    bool has_reflist = false;

    for(vector<Attribute *>::iterator ira = var->attrs.begin();
          ira != var->attrs.end();ira++) {
        if ("DIMENSION_LIST" == (*ira)->name) {
           dimlistattr = *ira;
           has_dimlist = true;  
        }
        if ("CLASS" == (*ira)->name) 
            has_class = true;
        if ("REFERENCE_LIST" == (*ira)->name) 
            has_reflist = true;
        
        if (true == has_dimlist) 
            break;
        if (true == has_class && true == has_reflist) 
            break; 
    } // for(vector<Attribute *>::iterator ira = var->attrs.begin(); ...

    if (true == has_dimlist) 
        Add_UseDimscale_Var_Dim_Names_Mea_SeaWiFS_Ozone(var,dimlistattr);

    // Dim name is the same as the variable name for dimscale variable
    else if(true == has_class && true == has_reflist) {
        if (var->dims.size() !=1) 
           throw2("dimension scale dataset must be 1 dimension, this is not true for variable ",
                  var->name);

        // The var name is the object name, however, we would like the dimension name to be full path.
        // so that the dim name can be served as the key for future handling.
        (var->dims)[0]->name = var->fullpath;
        (var->dims)[0]->newname = var->fullpath;
        pair<set<string>::iterator,bool> setret;
        setret = dimnamelist.insert((var->dims)[0]->name);
        if (true == setret.second) 
            Insert_One_NameSizeMap_Element((var->dims)[0]->name,(var->dims)[0]->size);
    }

    // No dimension, add fake dim names, this may never happen for MeaSure
    // but just for coherence and completeness.
    // For Fake dimesnion
    else {

        set<hsize_t> fakedimsize;
        pair<set<hsize_t>::iterator,bool> setsizeret;
        for (vector<Dimension *>::iterator ird= var->dims.begin();
            ird != var->dims.end(); ++ird) {
                Add_One_FakeDim_Name(*ird);
                setsizeret = fakedimsize.insert((*ird)->size);
                if (false == setsizeret.second)   
                    Adjust_Duplicate_FakeDim_Name(*ird);
        }
#if 0
        for (int i = 0; i < var->dims.size(); ++i) {
            Add_One_FakeDim_Name((var->dims)[i]);
            bool gotoMainLoop = false;
                for (int j =i-1; j>=0 && !gotoMainLoop; --j) {
                    if (((var->dims)[i])->size == ((var->dims)[j])->size){
                        Adjust_Duplicate_FakeDim_Name((var->dims)[i]);
                        gotoMainLoop = true;
                    }
                }
        }
#endif
        
    }
}

void GMFile::Add_UseDimscale_Var_Dim_Names_Mea_SeaWiFS_Ozone(Var *var,Attribute*dimlistattr) 
throw (Exception){
    
    ssize_t objnamelen = -1;
    hobj_ref_t rbuf;
    //hvl_t *vlbuf = NULL;
    vector<hvl_t> vlbuf;
    
    hid_t dset_id = -1;
    hid_t attr_id = -1;
    hid_t atype_id = -1;
    hid_t amemtype_id = -1;
    hid_t aspace_id = -1;
    hid_t ref_dset = -1;


    if(NULL == dimlistattr) 
        throw2("Cannot obtain the dimension list attribute for variable ",var->name);

    if (0==var->rank) 
        throw2("The number of dimension should NOT be 0 for the variable ",var->name);
    
    try {

        //vlbuf = new hvl_t[var->rank];
        vlbuf.resize(var->rank);
    
        hid_t dset_id = H5Dopen(this->fileid,(var->fullpath).c_str(),H5P_DEFAULT);
        if (dset_id < 0) 
            throw2("Cannot open the dataset ",var->fullpath);

        attr_id = H5Aopen(dset_id,(dimlistattr->name).c_str(),H5P_DEFAULT);
        if (attr_id <0 ) 
            throw4("Cannot open the attribute ",dimlistattr->name," of HDF5 dataset ",var->fullpath);

        atype_id = H5Aget_type(attr_id);
        if (atype_id <0) 
            throw4("Cannot obtain the datatype of the attribute ",dimlistattr->name," of HDF5 dataset ",var->fullpath);

        amemtype_id = H5Tget_native_type(atype_id, H5T_DIR_ASCEND);

        if (amemtype_id < 0) 
            throw2("Cannot obtain the memory datatype for the attribute ",dimlistattr->name);


        if (H5Aread(attr_id,amemtype_id,&vlbuf[0]) <0)  
            throw2("Cannot obtain the referenced object for the variable ",var->name);
        

        vector<char> objname;
        int vlbuf_index = 0;

        // The dimension names of variables will be the HDF5 dataset names dereferenced from the DIMENSION_LIST attribute.
        for (vector<Dimension *>::iterator ird = var->dims.begin();
                ird != var->dims.end(); ++ird) {

            rbuf =((hobj_ref_t*)vlbuf[vlbuf_index].p)[0];
            if ((ref_dset = H5Rdereference(attr_id, H5R_OBJECT, &rbuf)) < 0) 
                throw2("Cannot dereference from the DIMENSION_LIST attribute  for the variable ",var->name);

            if ((objnamelen= H5Iget_name(ref_dset,NULL,0))<=0) 
                throw2("Cannot obtain the dataset name dereferenced from the DIMENSION_LIST attribute  for the variable ",var->name);
            objname.resize(objnamelen+1);
            if ((objnamelen= H5Iget_name(ref_dset,&objname[0],objnamelen+1))<=0) 
                throw2("Cannot obtain the dataset name dereferenced from the DIMENSION_LIST attribute  for the variable ",var->name);

            string objname_str = string(objname.begin(),objname.end());
            string trim_objname = objname_str.substr(0,objnamelen);
            (*ird)->name = string(trim_objname.begin(),trim_objname.end());

            pair<set<string>::iterator,bool> setret;
            setret = dimnamelist.insert((*ird)->name);
            if (true == setret.second) 
               Insert_One_NameSizeMap_Element((*ird)->name,(*ird)->size);
            (*ird)->newname = (*ird)->name;
            H5Dclose(ref_dset);
            ref_dset = -1;
            objname.clear();
            vlbuf_index++;
        }// for (vector<Dimension *>::iterator ird = var->dims.begin()
        if(vlbuf.size()!= 0) {

            if ((aspace_id = H5Aget_space(attr_id)) < 0)
                throw2("Cannot get hdf5 dataspace id for the attribute ",dimlistattr->name);

            if (H5Dvlen_reclaim(amemtype_id,aspace_id,H5P_DEFAULT,(void*)&vlbuf[0])<0) 
                throw2("Cannot successfully clean up the variable length memory for the variable ",var->name);

            H5Sclose(aspace_id);
           
        }

        H5Tclose(atype_id);
        H5Tclose(amemtype_id);
        H5Aclose(attr_id);
        H5Dclose(dset_id);
    
       // if(vlbuf != NULL)
        //  delete[] vlbuf;
    }

    catch(...) {

        if(atype_id != -1)
            H5Tclose(atype_id);

        if(amemtype_id != -1)
            H5Tclose(amemtype_id);

        if(aspace_id != -1)
            H5Sclose(aspace_id);

        if(attr_id != -1)
            H5Aclose(attr_id);

        if(dset_id != -1)
            H5Dclose(dset_id);

        //if(vlbuf != NULL)
         //   delete[] vlbuf;

        //throw1("Error in method GMFile::Add_UseDimscale_Var_Dim_Names_Mea_SeaWiFS_Ozone"); 
        throw;
    }
 
}

void GMFile::Add_Dim_Name_Mea_Ozonel3z() throw(Exception){

    iscoard = true;
    bool use_dimscale = false;

    for (vector<Group *>::iterator irg = this->groups.begin();
        irg != this->groups.end(); ++ irg) {
        if ("/Dimensions" == (*irg)->path) {
            use_dimscale = true;
            break;
        }
    }

    if (false == use_dimscale) {

        bool has_dimlist = false;
        bool has_class = false;
        bool has_reflist = false;

        for (vector<Var *>::iterator irv = this->vars.begin();
                irv != this->vars.end(); irv++) {

            for(vector<Attribute *>::iterator ira = (*irv)->attrs.begin();
                ira != (*irv)->attrs.end();ira++) {
                if ("DIMENSION_LIST" == (*ira)->name) 
                    has_dimlist = true;  
            }
            if (true == has_dimlist) 
                break;
        }

        if (true == has_dimlist) {
            for (vector<Var *>::iterator irv = this->vars.begin();
                irv != this->vars.end(); irv++) {

                for(vector<Attribute *>::iterator ira = (*irv)->attrs.begin();
                    ira != (*irv)->attrs.end();ira++) {
                    if ("CLASS" == (*ira)->name) 
                        has_class = true;
                    if ("REFERENCE_LIST" == (*ira)->name) 
                        has_reflist = true;
                    if (true == has_class && true == has_reflist) 
                        break; 
                } 

                if (true == has_class && 
                    true == has_reflist) 
                    break;
            
            } 
            if (true == has_class && true == has_reflist) 
                use_dimscale = true;
        } // if (true == has_dimlist)
    } // if (false == use_dimscale)

    if (true == use_dimscale) {

        pair<set<string>::iterator,bool> setret;
        for (vector<Var *>::iterator irv = this->vars.begin();
                irv != this->vars.end(); ++irv) {
            Handle_UseDimscale_Var_Dim_Names_Mea_SeaWiFS_Ozone((*irv));
            for (vector<Dimension *>::iterator ird = (*irv)->dims.begin();
                ird !=(*irv)->dims.end();++ird) { 
                setret = dimnamelist.insert((*ird)->name);
                if(true == setret.second) 
                    Insert_One_NameSizeMap_Element((*ird)->name,(*ird)->size);
            }
        }
 
        if (true == dimnamelist.empty()) 
            throw1("This product should have the dimension names, but no dimension names are found");
    } // if (true == use_dimscale)    

    else {

        // Since the dim. size of each dimension of 2D lat/lon may be the same, so use multimap.
        multimap<hsize_t,string> ozonedimsize_to_dimname;
        pair<multimap<hsize_t,string>::iterator,multimap<hsize_t,string>::iterator> mm_er_ret;
        multimap<hsize_t,string>::iterator irmm;
 
        for (vector<Var *>::iterator irv = this->vars.begin();
                irv != this->vars.end(); ++irv) {
            bool is_cv = check_cv((*irv)->name);
            if (true == is_cv) {
//cerr<<"find CV "<<(*irv)->name <<endl;
                if ((*irv)->dims.size() != 1)
                    throw3("The coordinate variable", (*irv)->name," must be one dimension for the zonal average product");
                ozonedimsize_to_dimname.insert(pair<hsize_t,string>(((*irv)->dims)[0]->size,(*irv)->fullpath));
#if 0
            ((*irv)->dims[0])->name = (*irv)->name;
            ((*irv)->dims[0])->newname = (*irv)->name;
            pair<set<string>::iterator,bool> setret;
            setret = dimnamelist.insert(((*irv)->dims[0])->name);
            if (setret.second) 
                Insert_One_NameSizeMap_Element(((*irv)->dims[0])->name,((*irv)->dims[0])->size);
#endif
            }
        }// for (vector<Var *>::iterator irv = this->vars.begin(); ...

        set<hsize_t> fakedimsize;
        pair<set<hsize_t>::iterator,bool> setsizeret;
        pair<set<string>::iterator,bool> setret;
        pair<set<string>::iterator,bool> tempsetret;
        set<string> tempdimnamelist;
        bool fakedimflag = false;

        for (vector<Var *>::iterator irv = this->vars.begin();
                irv != this->vars.end(); ++irv) {

            for (vector<Dimension *>::iterator ird = (*irv)->dims.begin();
                ird != (*irv)->dims.end(); ++ird) {

                fakedimflag = true;
                mm_er_ret = ozonedimsize_to_dimname.equal_range((*ird)->size);
                for (irmm = mm_er_ret.first; irmm!=mm_er_ret.second;irmm++) {
                    setret = tempdimnamelist.insert(irmm->second);
                    if (true == setret.second) {
                       (*ird)->name = irmm->second;
                       (*ird)->newname = (*ird)->name;
                       setret = dimnamelist.insert((*ird)->name);
                       if(setret.second) Insert_One_NameSizeMap_Element((*ird)->name,(*ird)->size);
                       fakedimflag = false;
                       break;
                    }
                }
                      
                if (true == fakedimflag) {
                     Add_One_FakeDim_Name(*ird);
                     setsizeret = fakedimsize.insert((*ird)->size);
                     if (false == setsizeret.second)  
                        Adjust_Duplicate_FakeDim_Name(*ird);
                }
            
            } // for (vector<Dimension *>::iterator ird = (*irv)->dims.begin();
            tempdimnamelist.clear();
            fakedimsize.clear();
        } // for (vector<Var *>::iterator irv = this->vars.begin();
    } // else
}

bool GMFile::check_cv(string & varname) throw(Exception) {

     const string lat_name ="Latitude";
     const string time_name ="Time";
     const string ratio_pressure_name ="MixingRatioPressureLevels";
     const string profile_pressure_name ="ProfilePressureLevels";
     const string wave_length_name ="Wavelength";

     if (lat_name == varname) 
        return true;
     else if (time_name == varname) 
        return true;
     else if (ratio_pressure_name == varname) 
        return true;
     else if (profile_pressure_name == varname) 
        return true;
     else if (wave_length_name == varname)
        return true;
     else 
        return false;
}
     
void GMFile::Add_Dim_Name_Aqu_L3()throw(Exception)
{
    for (vector<Var *>::iterator irv = this->vars.begin();
                irv != this->vars.end(); irv++) {
        if ("l3m_data" == (*irv)->name) {
           ((*irv)->dims)[0]->name = "lat";
           ((*irv)->dims)[0]->newname = "lat";
           ((*irv)->dims)[1]->name = "lon";
           ((*irv)->dims)[1]->newname = "lon";
           break;
        }
       
// For the time being, don't assign dimension names to palette,
// we will see if tools can pick up l3m and then make decisions.
#if 0
        if ("palette" == (*irv)->name) {
//cerr <<"coming to palette" <<endl;
          ((*irv)->dims)[0]->name = "paldim0";
           ((*irv)->dims)[0]->newname = "paldim0";
           ((*irv)->dims)[1]->name = "paldim1";
           ((*irv)->dims)[1]->newname = "paldim1";
        }
#endif

    }
}
void GMFile::Add_Dim_Name_SMAP()throw(Exception){

    string tempvarname ="";
    string key = "_lat";
    string smapdim0 ="YDim";
    string smapdim1 ="XDim";

    // Since the dim. size of each dimension of 2D lat/lon may be the same, so use multimap.
    multimap<hsize_t,string> smapdimsize_to_dimname;
    pair<multimap<hsize_t,string>::iterator,multimap<hsize_t,string>::iterator> mm_er_ret;
    multimap<hsize_t,string>::iterator irmm; 

    // Generate dimension names based on the size of "???_lat"(one coordinate variable) 
    for (vector<Var *>::iterator irv = this->vars.begin();
                irv != this->vars.end(); ++irv) {
        tempvarname = (*irv)->name;
        if ((tempvarname.size() > key.size())&& 
            (key == tempvarname.substr(tempvarname.size()-key.size(),key.size()))){
//cerr<<"tempvarname " <<tempvarname <<endl;
            if ((*irv)->dims.size() !=2) throw1("Currently only 2D lat/lon is supported for SMAP");
            smapdimsize_to_dimname.insert(pair<hsize_t,string>(((*irv)->dims)[0]->size,smapdim0));
            smapdimsize_to_dimname.insert(pair<hsize_t,string>(((*irv)->dims)[1]->size,smapdim1));
            break;
        }
    }

    set<hsize_t> fakedimsize;
    pair<set<hsize_t>::iterator,bool> setsizeret;
    pair<set<string>::iterator,bool> setret;
    pair<set<string>::iterator,bool> tempsetret;
    set<string> tempdimnamelist;
    bool fakedimflag = false;


    for (vector<Var *>::iterator irv = this->vars.begin();
                irv != this->vars.end(); ++irv) {

        for (vector<Dimension *>::iterator ird = (*irv)->dims.begin();
            ird != (*irv)->dims.end(); ++ird) {

            fakedimflag = true;
            mm_er_ret = smapdimsize_to_dimname.equal_range((*ird)->size);
            for (irmm = mm_er_ret.first; irmm!=mm_er_ret.second;irmm++) {
                setret = tempdimnamelist.insert(irmm->second);
                if (setret.second) {
                   (*ird)->name = irmm->second;
                   (*ird)->newname = (*ird)->name;
                   setret = dimnamelist.insert((*ird)->name);
                   if(setret.second) Insert_One_NameSizeMap_Element((*ird)->name,(*ird)->size);
                   fakedimflag = false;
                   break;
                }
            }
                      
            if (true == fakedimflag) {
                 Add_One_FakeDim_Name(*ird);
                 setsizeret = fakedimsize.insert((*ird)->size);
                 if (!setsizeret.second)  
                    Adjust_Duplicate_FakeDim_Name(*ird);
            }
        } // for (vector<Dimension *>::iterator ird = (*irv)->dims.begin();
        tempdimnamelist.clear();
        fakedimsize.clear();
    } // for (vector<Var *>::iterator irv = this->vars.begin();
}

void GMFile::Add_Dim_Name_ACOS_L2S()throw(Exception){

    for (vector<Var *>::iterator irv = this->vars.begin();
        irv != this->vars.end(); ++irv) {

        set<hsize_t> fakedimsize;
        pair<set<hsize_t>::iterator,bool> setsizeret;
        for (vector<Dimension *>::iterator ird= (*irv)->dims.begin();
            ird != (*irv)->dims.end(); ++ird) {
                Add_One_FakeDim_Name(*ird);
                setsizeret = fakedimsize.insert((*ird)->size);
                if (false == setsizeret.second)   
                    Adjust_Duplicate_FakeDim_Name(*ird);
        }
    } // for (vector<Var *>::iterator irv = this->vars.begin();
}
void GMFile::Add_Dim_Name_General_Product()throw(Exception){

    // Check attributes 
    Check_General_Product_Pattern();

    // This general product should follow the HDF5 dimension scale model. 
    if (GENERAL_DIMSCALE == this->gproduct_pattern)
        Add_Dim_Name_Dimscale_General_Product();

}

void GMFile::Check_General_Product_Pattern() throw(Exception) {

    bool has_dimlist = false;
    bool has_dimscalelist  = false;

    // Check if containing the "DIMENSION_LIST" attribute;
    for (vector<Var *>::iterator irv = this->vars.begin();
        irv != this->vars.end(); ++irv) {
        for(vector<Attribute *>::iterator ira = (*irv)->attrs.begin();
          ira != (*irv)->attrs.end();ira++) {
           if ("DIMENSION_LIST" == (*ira)->name) {
                has_dimlist = true;
                break;
           }
        }
        if (true == has_dimlist)
            break;
    }

    // Check if containing both the attribute "CLASS" and the attribute "REFERENCE_LIST" for the same variable.
    // This is the dimension scale. 
    // Actually "REFERENCE_LIST" is not necessary for a dimension scale dataset. If a dimension scale doesn't
    // have a "REFERENCE_LIST", it is still valid. But no other variables use this dimension scale. We found
    // such a case in a matched_airs_aqua product. KY 2012-12-03
    for (vector<Var *>::iterator irv = this->vars.begin();
        irv != this->vars.end(); ++irv) {


        for(vector<Attribute *>::iterator ira = (*irv)->attrs.begin();
          ira != (*irv)->attrs.end();ira++) {
            if ("CLASS" == (*ira)->name) {

                Retrieve_H5_Attr_Value(*ira,(*irv)->fullpath);
                string class_value;
                class_value.resize((*ira)->value.size());
                copy((*ira)->value.begin(),(*ira)->value.end(),class_value.begin());

                // Compare the attribute "CLASS" value with "DIMENSION_SCALE". We only compare the string with the size of
                // "DIMENSION_SCALE", which is 15.
                if (0 == class_value.compare(0,15,"DIMENSION_SCALE")) {
                    has_dimscalelist = true;
                    break;
                }
            }
        }

        if (true == has_dimscalelist)
            break;
        
    }

    if (true == has_dimlist && true == has_dimscalelist)
        this->gproduct_pattern = GENERAL_DIMSCALE;

}

void GMFile::Add_Dim_Name_Dimscale_General_Product() throw(Exception) {

    //cerr<<"coming to Add_Dim_Name_Dimscale_General_Product"<<endl;
    pair<set<string>::iterator,bool> setret;
    this->iscoard = true;

    for (vector<Var *>::iterator irv = this->vars.begin();
        irv != this->vars.end(); ++irv) {
        Handle_UseDimscale_Var_Dim_Names_General_Product((*irv));
        for (vector<Dimension *>::iterator ird = (*irv)->dims.begin();
            ird !=(*irv)->dims.end();++ird) { 
            setret = dimnamelist.insert((*ird)->name);
            if (true == setret.second) 
                Insert_One_NameSizeMap_Element((*ird)->name,(*ird)->size);
        }
    } // for (vector<Var *>::iterator irv = this->vars.begin();
 
    if (true == dimnamelist.empty()) 
        throw1("This product should have the dimension names, but no dimension names are found");

}

void GMFile::Handle_UseDimscale_Var_Dim_Names_General_Product(Var *var) throw(Exception) {

    Attribute* dimlistattr = NULL;
    bool has_dimlist = false;
    bool has_dimclass   = false;

    for(vector<Attribute *>::iterator ira = var->attrs.begin();
          ira != var->attrs.end();ira++) {
        if ("DIMENSION_LIST" == (*ira)->name) {
            dimlistattr = *ira;
            has_dimlist = true;  
        }
        if ("CLASS" == (*ira)->name) {

            Retrieve_H5_Attr_Value(*ira,var->fullpath);
            string class_value;
            class_value.resize((*ira)->value.size());
            copy((*ira)->value.begin(),(*ira)->value.end(),class_value.begin());

            // Compare the attribute "CLASS" value with "DIMENSION_SCALE". We only compare the string with the size of
            // "DIMENSION_SCALE", which is 15.
            if (0 == class_value.compare(0,15,"DIMENSION_SCALE")) {
                has_dimclass = true;
                break;
            }
        }

    } // for(vector<Attribute *>::iterator ira = var->attrs.begin(); ...

    // This is a general variable, we need to find the corresponding coordinate variables.
    if (true == has_dimlist) 
        Add_UseDimscale_Var_Dim_Names_General_Product(var,dimlistattr);

    // Dim name is the same as the variable name for dimscale variable
    else if(true == has_dimclass) {
        if (var->dims.size() !=1) 
           throw2("Currently dimension scale dataset must be 1 dimension, this is not true for the dataset ",
                  var->name);

        // The var name is the object name, however, we would like the dimension name to be the full path.
        // so that the dim name can be served as the key for future handling.
        (var->dims)[0]->name = var->fullpath;
        (var->dims)[0]->newname = var->fullpath;
        pair<set<string>::iterator,bool> setret;
        setret = dimnamelist.insert((var->dims)[0]->name);
        if (true == setret.second) 
            Insert_One_NameSizeMap_Element((var->dims)[0]->name,(var->dims)[0]->size);
    }

    // No dimension, add fake dim names, this will rarely happen.
    else {

        set<hsize_t> fakedimsize;
        pair<set<hsize_t>::iterator,bool> setsizeret;
        for (vector<Dimension *>::iterator ird= var->dims.begin();
            ird != var->dims.end(); ++ird) {
                Add_One_FakeDim_Name(*ird);
                setsizeret = fakedimsize.insert((*ird)->size);
                // Avoid the same size dimension sharing the same dimension name.
                if (false == setsizeret.second)   
                    Adjust_Duplicate_FakeDim_Name(*ird);
        }
    }

}

void GMFile::Add_UseDimscale_Var_Dim_Names_General_Product(Var *var,Attribute*dimlistattr) 
throw (Exception){
    
    ssize_t objnamelen = -1;
    hobj_ref_t rbuf;
    //hvl_t *vlbuf = NULL;
    vector<hvl_t> vlbuf;
    
    hid_t dset_id = -1;
    hid_t attr_id = -1;
    hid_t atype_id = -1;
    hid_t amemtype_id = -1;
    hid_t aspace_id = -1;
    hid_t ref_dset = -1;


    if(NULL == dimlistattr) 
        throw2("Cannot obtain the dimension list attribute for variable ",var->name);

    if (0==var->rank) 
        throw2("The number of dimension should NOT be 0 for the variable ",var->name);
    
    try {

        //vlbuf = new hvl_t[var->rank];
        vlbuf.resize(var->rank);
    
        hid_t dset_id = H5Dopen(this->fileid,(var->fullpath).c_str(),H5P_DEFAULT);
        if (dset_id < 0) 
            throw2("Cannot open the dataset ",var->fullpath);

        attr_id = H5Aopen(dset_id,(dimlistattr->name).c_str(),H5P_DEFAULT);
        if (attr_id <0 ) 
            throw4("Cannot open the attribute ",dimlistattr->name," of HDF5 dataset ",var->fullpath);

        atype_id = H5Aget_type(attr_id);
        if (atype_id <0) 
            throw4("Cannot obtain the datatype of the attribute ",dimlistattr->name," of HDF5 dataset ",var->fullpath);

        amemtype_id = H5Tget_native_type(atype_id, H5T_DIR_ASCEND);

        if (amemtype_id < 0) 
            throw2("Cannot obtain the memory datatype for the attribute ",dimlistattr->name);


        if (H5Aread(attr_id,amemtype_id,&vlbuf[0]) <0)  
            throw2("Cannot obtain the referenced object for the variable ",var->name);
        

        vector<char> objname;
        int vlbuf_index = 0;

        // The dimension names of variables will be the HDF5 dataset names dereferenced from the DIMENSION_LIST attribute.
        for (vector<Dimension *>::iterator ird = var->dims.begin();
                ird != var->dims.end(); ++ird) {

            rbuf =((hobj_ref_t*)vlbuf[vlbuf_index].p)[0];
            if ((ref_dset = H5Rdereference(attr_id, H5R_OBJECT, &rbuf)) < 0) 
                throw2("Cannot dereference from the DIMENSION_LIST attribute  for the variable ",var->name);

            if ((objnamelen= H5Iget_name(ref_dset,NULL,0))<=0) 
                throw2("Cannot obtain the dataset name dereferenced from the DIMENSION_LIST attribute  for the variable ",var->name);
            objname.resize(objnamelen+1);
            if ((objnamelen= H5Iget_name(ref_dset,&objname[0],objnamelen+1))<=0) 
                throw2("Cannot obtain the dataset name dereferenced from the DIMENSION_LIST attribute  for the variable ",var->name);

            string objname_str = string(objname.begin(),objname.end());
            string trim_objname = objname_str.substr(0,objnamelen);
            (*ird)->name = string(trim_objname.begin(),trim_objname.end());

            pair<set<string>::iterator,bool> setret;
            setret = dimnamelist.insert((*ird)->name);
            if (true == setret.second) 
               Insert_One_NameSizeMap_Element((*ird)->name,(*ird)->size);
            (*ird)->newname = (*ird)->name;
            H5Dclose(ref_dset);
            ref_dset = -1;
            objname.clear();
            vlbuf_index++;
        }// for (vector<Dimension *>::iterator ird = var->dims.begin()
        if(vlbuf.size()!= 0) {

            if ((aspace_id = H5Aget_space(attr_id)) < 0)
                throw2("Cannot get hdf5 dataspace id for the attribute ",dimlistattr->name);

            if (H5Dvlen_reclaim(amemtype_id,aspace_id,H5P_DEFAULT,(void*)&vlbuf[0])<0) 
                throw2("Cannot successfully clean up the variable length memory for the variable ",var->name);

            H5Sclose(aspace_id);
           
        }

        H5Tclose(atype_id);
        H5Tclose(amemtype_id);
        H5Aclose(attr_id);
        H5Dclose(dset_id);
    
       // if(vlbuf != NULL)
        //  delete[] vlbuf;
    }

    catch(...) {

        if(atype_id != -1)
            H5Tclose(atype_id);

        if(amemtype_id != -1)
            H5Tclose(amemtype_id);

        if(aspace_id != -1)
            H5Sclose(aspace_id);

        if(attr_id != -1)
            H5Aclose(attr_id);

        if(dset_id != -1)
            H5Dclose(dset_id);

        //if(vlbuf != NULL)
         //   delete[] vlbuf;

        //throw1("Error in method GMFile::Add_UseDimscale_Var_Dim_Names_Mea_SeaWiFS_Ozone"); 
        throw;
    }
 
}


void GMFile::Handle_CVar() throw(Exception){

    // No support for coordinate variables for general HDF5 products
    // and ACOS_L2S
    if (General_Product == this->product_type ||
        ACOS_L2S == this->product_type) {
        if (GENERAL_DIMSCALE == this->gproduct_pattern)
            Handle_CVar_Dimscale_General_Product();
        else
            return;
    } 

    if (Mea_SeaWiFS_L2 == this->product_type ||
        Mea_SeaWiFS_L3 == this->product_type) 
        Handle_CVar_Mea_SeaWiFS();

    if (Aqu_L3 == this->product_type) 
        Handle_CVar_Aqu_L3(); 
    if (SMAP == this->product_type) 
        Handle_CVar_SMAP();
    if (Mea_Ozone == this->product_type) 
        Handle_CVar_Mea_Ozone();
}

void GMFile::Handle_CVar_Mea_SeaWiFS() throw(Exception){

    pair<set<string>::iterator,bool> setret;
    set<string>tempdimnamelist = dimnamelist;

    for (set<string>::iterator irs = dimnamelist.begin();
            irs != dimnamelist.end();++irs) {
        for (vector<Var *>::iterator irv = this->vars.begin();
                irv != this->vars.end(); ++irv) {
            if ((*irs)== (*irv)->fullpath) {

                if (!iscoard && (("/natrack" == (*irs)) 
                                 || "/nxtrack" == (*irs)))
                    continue;

                if((*irv)->dims.size()!=1) 
                    throw3("Coard coordinate variable",(*irv)->name, "is not 1D");
                   // Create Coordinate variables.
                   tempdimnamelist.erase(*irs);
                   GMCVar* GMcvar = new GMCVar(*irv);
                   GMcvar->cfdimname = *irs;
                   GMcvar->cvartype = CV_EXIST;
                   GMcvar->product_type = product_type;
                   this->cvars.push_back(GMcvar); 
                   delete(*irv);
                   this->vars.erase(irv);
                   irv--;
            } // if ((*irs)== (*irv)->fullpath)
            else if(false == iscoard) { 
            // 2-D lat/lon, natrack maps to lat, nxtrack maps to lon.
                 
                if ((((*irs) =="/natrack") && ((*irv)->fullpath == "/latitude"))
                  ||(((*irs) =="/nxtrack") && ((*irv)->fullpath == "/longitude"))) {
                    tempdimnamelist.erase(*irs);
                    GMCVar* GMcvar = new GMCVar(*irv);
                    GMcvar->cfdimname = *irs;    
                    GMcvar->cvartype = CV_EXIST;
                    GMcvar->product_type = product_type;
                    this->cvars.push_back(GMcvar);
                    delete(*irv);
                    this->vars.erase(irv);
                    irv--;
                }
            }// else if(false == iscoard)
        } // for (vector<Var *>::iterator irv = this->vars.begin() ... 
    } // for (set<string>::iterator irs = dimnamelist.begin() ...

    // Creating the missing "third-dimension" according to the dimension names.
    // This may never happen for the current MeaSure SeaWiFS, but put it here for code coherence and completeness.
    // KY 12-30-2011
    for (set<string>::iterator irs = tempdimnamelist.begin();
        irs != tempdimnamelist.end();++irs) {
        GMCVar*GMcvar = new GMCVar();
        Create_Missing_CV(GMcvar,*irs);
        this->cvars.push_back(GMcvar);
    }
}

void GMFile::Handle_CVar_SMAP() throw(Exception) {

    pair<set<string>::iterator,bool> setret;
    set<string>tempdimnamelist = dimnamelist;
    string tempvarname;
    string key0 = "_lat";
    string key1 = "_lon";
    string smapdim0 ="YDim";
    string smapdim1 ="XDim";

    bool foundkey0 = false;
    bool foundkey1 = false;

    set<string> itset;

    for (vector<Var *>::iterator irv = this->vars.begin();
                irv != this->vars.end(); ++irv) {

        tempvarname = (*irv)->name;

        if ((tempvarname.size() > key0.size())&&
                (key0 == tempvarname.substr(tempvarname.size()-key0.size(),key0.size()))){
                foundkey0 = true;
            if (dimnamelist.find(smapdim0)== dimnamelist.end()) 
                throw5("variable ",tempvarname," must have dimension ",smapdim0," , but not found ");

                tempdimnamelist.erase(smapdim0);
                GMCVar* GMcvar = new GMCVar(*irv);
                GMcvar->newname = GMcvar->name; // Remove the path, just use the variable name
                GMcvar->cfdimname = smapdim0;    
                GMcvar->cvartype = CV_EXIST;
                GMcvar->product_type = product_type;
                this->cvars.push_back(GMcvar);
                delete(*irv);
                this->vars.erase(irv);
                irv--;
        }// if ((tempvarname.size() > key0.size())&& ...
                    
        else if ((tempvarname.size() > key1.size())&& 
                (key1 == tempvarname.substr(tempvarname.size()-key1.size(),key1.size()))){
                foundkey1 = true;
            if (dimnamelist.find(smapdim1)== dimnamelist.end()) 
                throw5("variable ",tempvarname," must have dimension ",smapdim1," , but not found ");

                tempdimnamelist.erase(smapdim1);

                GMCVar* GMcvar = new GMCVar(*irv);
                GMcvar->newname = GMcvar->name;
                GMcvar->cfdimname = smapdim1;    
                GMcvar->cvartype = CV_EXIST;
                GMcvar->product_type = product_type;
                this->cvars.push_back(GMcvar);
                delete(*irv);
                this->vars.erase(irv);
                irv--;
        }// else if ((tempvarname.size() > key1.size())&& ...
        if (true == foundkey0 && true == foundkey1) 
            break;
            
    } // for (vector<Var *>::iterator irv = this->vars.begin(); ...

    for (set<string>::iterator irs = tempdimnamelist.begin();
        irs != tempdimnamelist.end();++irs) {

        GMCVar*GMcvar = new GMCVar();
        Create_Missing_CV(GMcvar,*irs);
        this->cvars.push_back(GMcvar);
    }

}

void GMFile::Handle_CVar_Aqu_L3() throw(Exception) {

    iscoard = true;
    for (vector<Var *>::iterator irv = this->vars.begin();
                irv != this->vars.end(); ++irv) {

        if ( "l3m_data" == (*irv)->name) { 
            for (vector<Dimension *>::iterator ird = (*irv)->dims.begin();
                        ird != (*irv)->dims.end(); ++ird) {
                GMCVar*GMcvar = new GMCVar();
                GMcvar->name = (*ird)->name;
                GMcvar->newname = GMcvar->name;
                GMcvar->fullpath = GMcvar->name;
                GMcvar->rank = 1;
                GMcvar->dtype = H5FLOAT32; 
                Dimension* gmcvar_dim = new Dimension((*ird)->size);
                gmcvar_dim->name = GMcvar->name;
                gmcvar_dim->newname = gmcvar_dim->name;
                GMcvar->dims.push_back(gmcvar_dim); 
                GMcvar->cfdimname = gmcvar_dim->name;
                if ("lat" ==GMcvar->name ) GMcvar->cvartype = CV_LAT_MISS;
                if ("lon" == GMcvar->name ) GMcvar->cvartype = CV_LON_MISS;
                GMcvar->product_type = product_type;
                this->cvars.push_back(GMcvar);
            } // for (vector<Dimension *>::iterator ird = (*irv)->dims.begin(); ...
        } // if ( "l3m_data" == (*irv)->name) 
    }//for (vector<Var *>::iterator irv = this->vars.begin(); ...
 
}

void GMFile::Handle_CVar_Mea_Ozone() throw(Exception){

    pair<set<string>::iterator,bool> setret;
    set<string>tempdimnamelist = dimnamelist;

    if(false == iscoard) 
        throw1("Measure Ozone level 3 zonal average product must follow COARDS conventions");

    for (set<string>::iterator irs = dimnamelist.begin();
            irs != dimnamelist.end();++irs) {
        for (vector<Var *>::iterator irv = this->vars.begin();
                irv != this->vars.end(); ++irv) {
            if ((*irs)== (*irv)->fullpath) {

                if((*irv)->dims.size()!=1) 
                    throw3("Coard coordinate variable",(*irv)->name, "is not 1D");

                   // Create Coordinate variables.
                   tempdimnamelist.erase(*irs);
                   GMCVar* GMcvar = new GMCVar(*irv);
                   GMcvar->cfdimname = *irs;
                   GMcvar->cvartype = CV_EXIST;
                   GMcvar->product_type = product_type;
                   this->cvars.push_back(GMcvar); 
                   delete(*irv);
                   this->vars.erase(irv);
                   irv--;
            } // if ((*irs)== (*irv)->fullpath)
       } // for (vector<Var *>::iterator irv = this->vars.begin();
    } // for (set<string>::iterator irs = dimnamelist.begin();

    // Wait for the final product to see if the following statement is true. Now comment out.
    //if(false == tempdimnamelist.empty()) throw1("Measure Ozone level 3 new data shouldnot have missing dimensions");
    for (set<string>::iterator irs = tempdimnamelist.begin();
        irs != tempdimnamelist.end();irs++) {

        GMCVar*GMcvar = new GMCVar();
        Create_Missing_CV(GMcvar,*irs);
        this->cvars.push_back(GMcvar);
    }
}

void GMFile::Handle_CVar_Dimscale_General_Product() throw(Exception) {

    pair<set<string>::iterator,bool> setret;
    set<string>tempdimnamelist = dimnamelist;

    if(false == iscoard) 
        throw1("Currently products that use HDF5 dimension scales  must follow COARDS conventions");

    for (set<string>::iterator irs = dimnamelist.begin();
            irs != dimnamelist.end();++irs) {
        for (vector<Var *>::iterator irv = this->vars.begin();
                irv != this->vars.end(); ++irv) {

            // This is the dimension scale dataset; it should be changed to a coordinate variable.
            if ((*irs)== (*irv)->fullpath) {

                if((*irv)->dims.size()!=1) 
                    throw3("COARDS coordinate variable",(*irv)->name, "is not 1D");

                // Create Coordinate variables.
                tempdimnamelist.erase(*irs);
                GMCVar* GMcvar = new GMCVar(*irv);
                GMcvar->cfdimname = *irs;

                // Check if this is just a netCDF-4 dimension. 
                bool is_netcdf_dimension = Is_netCDF_Dimension(*irv);
                if (true == is_netcdf_dimension)
                   GMcvar->cvartype = CV_FILLINDEX;
                else 
                    GMcvar->cvartype = CV_EXIST;

                GMcvar->product_type = product_type;
                this->cvars.push_back(GMcvar); 
                delete(*irv);
                this->vars.erase(irv);
                irv--;
            } // if ((*irs)== (*irv)->fullpath)
       } // for (vector<Var *>::iterator irv = this->vars.begin();
    } // for (set<string>::iterator irs = dimnamelist.begin();

    // Wait for the final product to see if the following statement is true. Now comment out.
    //if(false == tempdimnamelist.empty()) throw1("Measure Ozone level 3 new data shouldnot have missing dimensions");
    for (set<string>::iterator irs = tempdimnamelist.begin();
        irs != tempdimnamelist.end();irs++) {

        GMCVar*GMcvar = new GMCVar();
        Create_Missing_CV(GMcvar,*irs);
        this->cvars.push_back(GMcvar);
    }

}

void GMFile::Handle_SpVar() throw(Exception){
    if (ACOS_L2S == product_type) 
        Handle_SpVar_ACOS();
}

void GMFile::Handle_SpVar_ACOS() throw(Exception) {

    //The ACOS only have 64-bit variables. So we will not handle attributes yet.
    for (vector<Var *>::iterator irv = this->vars.begin();
                irv != this->vars.end(); ++irv) {
        if (H5INT64 == (*irv)->getType()) {
            
            // First: Time Part of soundingid
            GMSPVar * spvar = new GMSPVar(*irv);
            spvar->name = (*irv)->name +"_Time";
            spvar->newname = (*irv)->newname+"_Time";
            spvar->dtype = H5INT32;
            spvar->otype = (*irv)->getType();
            spvar->sdbit = 1;

            // 2 digit hour, 2 digit min, 2 digit seconds
            spvar->numofdbits = 6;
            this->spvars.push_back(spvar);

            // Second: Date Part of soundingid
            GMSPVar * spvar2 = new GMSPVar(*irv);
            spvar2->name = (*irv)->name +"_Date";
            spvar2->newname = (*irv)->newname+"_Date";
            spvar2->dtype = H5INT32;
            spvar2->otype = (*irv)->getType();
            spvar2->sdbit = 7;

            // 4 digit year, 2 digit month, 2 digit day
            spvar2->numofdbits = 8;
            this->spvars.push_back(spvar2);

            delete(*irv);
            this->vars.erase(irv);
            irv--;
        } // if (H5INT64 == (*irv)->getType())
    } // for (vector<Var *>::iterator irv = this->vars.begin(); ...
}

            
void GMFile::Adjust_Obj_Name() throw(Exception) {

    if(Mea_Ozone == product_type) 
        Adjust_Mea_Ozone_Obj_Name();

}

void GMFile:: Adjust_Mea_Ozone_Obj_Name() throw(Exception) {

    string objnewname;
    for (vector<Var *>::iterator irv = this->vars.begin();
                   irv != this->vars.end(); ++irv) {
        objnewname =  HDF5CFUtil::obtain_string_after_lastslash((*irv)->newname);
        if (objnewname !="") 
           (*irv)->newname = objnewname;

#if 0
//Just for debugging
for (vector<Dimension *>::iterator ird = (*irv)->dims.begin();
                ird !=(*irv)->dims.end();++ird) {
 cerr<<"Ozone dim. name "<<(*ird)->name <<endl;
 cerr<<"Ozone dim. new name "<<(*ird)->newname <<endl;
}
#endif

    }

    for (vector<GMCVar *>::iterator irv = this->cvars.begin();
                irv != this->cvars.end(); ++irv) {
        objnewname =  HDF5CFUtil::obtain_string_after_lastslash((*irv)->newname);
        if (objnewname !="")
           (*irv)->newname = objnewname;
#if 0
 //Just for debugging
for (vector<Dimension *>::iterator ird = (*irv)->dims.begin();
                ird !=(*irv)->dims.end();++ird) {
 cerr<<"Ozone CV dim. name "<<(*ird)->name <<endl;
 cerr<<"Ozone CV dim. new name "<<(*ird)->newname <<endl;
}   
#endif
    }
}
void GMFile::Flatten_Obj_Name(bool include_attr) throw(Exception){
     
    File::Flatten_Obj_Name(include_attr);

    for (vector<GMCVar *>::iterator irv = this->cvars.begin();
             irv != this->cvars.end(); ++irv) {
        (*irv)->newname = get_CF_string((*irv)->newname);

        for (vector<Dimension *>::iterator ird = (*irv)->dims.begin();
                        ird != (*irv)->dims.end(); ++ird) { 
            (*ird)->newname = get_CF_string((*ird)->newname);
        }

        

        if (true == include_attr) {
            for (vector<Attribute *>::iterator ira = (*irv)->attrs.begin();
                        ira != (*irv)->attrs.end(); ++ira) 
                (*ira)->newname = get_CF_string((*ira)->newname);
                
        }

    }

    for (vector<GMSPVar *>::iterator irv = this->spvars.begin();
                irv != this->spvars.end(); ++irv) {
        (*irv)->newname = get_CF_string((*irv)->newname);

        for (vector<Dimension *>::iterator ird = (*irv)->dims.begin();
                     ird != (*irv)->dims.end(); ++ird) 
            (*ird)->newname = get_CF_string((*ird)->newname);

        if (true == include_attr) {
            for (vector<Attribute *>::iterator ira = (*irv)->attrs.begin();
                        ira != (*irv)->attrs.end(); ++ira) 
                  (*ira)->newname = get_CF_string((*ira)->newname);
                
        }
    }
}

void GMFile::Handle_Obj_NameClashing(bool include_attr) throw(Exception) {

    // objnameset will be filled with all object names that we are going to check the name clashing.
    // For example, we want to see if there are any name clashings for all variable names in this file.
    // objnameset will include all variable names. If a name clashing occurs, we can figure out from the set operation immediately.

    set<string>objnameset;
    Handle_GMCVar_NameClashing(objnameset);
    Handle_GMSPVar_NameClashing(objnameset);
    File::Handle_GeneralObj_NameClashing(include_attr,objnameset);
    if (true == include_attr) {
        Handle_GMCVar_AttrNameClashing();
        Handle_GMSPVar_AttrNameClashing();
    }
    Handle_DimNameClashing();
}

void GMFile::Handle_GMCVar_NameClashing(set<string> &objnameset ) throw(Exception) {

    GMHandle_General_NameClashing(objnameset,this->cvars);
}

void GMFile::Handle_GMSPVar_NameClashing(set<string> &objnameset ) throw(Exception) {

    GMHandle_General_NameClashing(objnameset,this->spvars);
}


void GMFile::Handle_GMCVar_AttrNameClashing() throw(Exception) {

    set<string> objnameset;

    for (vector<GMCVar *>::iterator irv = this->cvars.begin();
        irv != this->cvars.end(); ++irv) {
        Handle_General_NameClashing(objnameset,(*irv)->attrs);
        objnameset.clear();
    }
}
void GMFile::Handle_GMSPVar_AttrNameClashing() throw(Exception) {

    set<string> objnameset;

    for (vector<GMSPVar *>::iterator irv = this->spvars.begin();
        irv != this->spvars.end(); ++irv) {
        Handle_General_NameClashing(objnameset,(*irv)->attrs);
        objnameset.clear();
    }
}

//class T must have member string newname
template<class T> void
GMFile::GMHandle_General_NameClashing(set <string>&objnameset, vector<T*>& objvec) throw(Exception){

    pair<set<string>::iterator,bool> setret;
    set<string>::iterator iss;

    vector<string> clashnamelist;
    vector<string>::iterator ivs;

    map<int,int> cl_to_ol;
    int ol_index = 0;
    int cl_index = 0;

    typename vector<T*>::iterator irv;
    //for (vector<T*>::iterator irv = objvec.begin();

    for (irv = objvec.begin();
                irv != objvec.end(); ++irv) {

        setret = objnameset.insert((*irv)->newname);
        if (false == setret.second ) {
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

void GMFile::Handle_DimNameClashing() throw(Exception){

    // ACOS L2S product doesn't need the dimension name clashing check based on our current understanding. KY 2012-5-16
    if (ACOS_L2S == product_type) return;

    map<string,string>dimname_to_dimnewname;
    pair<map<string,string>::iterator,bool>mapret;
    set<string> dimnameset;
    vector<Dimension*>vdims;
    set<string> dimnewnameset;
    pair<set<string>::iterator,bool> setret;

    // First: Generate the dimset/dimvar based on coordinate variables.
    for (vector<GMCVar *>::iterator irv = this->cvars.begin();
             irv !=this->cvars.end(); ++irv) {
        for (vector <Dimension *>:: iterator ird = (*irv)->dims.begin();
            ird !=(*irv)->dims.end();++ird) {
            setret = dimnameset.insert((*ird)->newname);
            if (true == setret.second) 
                vdims.push_back(*ird); 
        }
    }

    // For some cases, dimension names are provided but there are no corresponding coordinate
    // variables. For now, we will assume no such cases.
    GMHandle_General_NameClashing(dimnewnameset,vdims);
   
    // Third: Make dimname_to_dimnewname map
    for (vector<Dimension*>::iterator ird = vdims.begin();ird!=vdims.end();++ird) {
        mapret = dimname_to_dimnewname.insert(pair<string,string>((*ird)->name,(*ird)->newname));
        if (false == mapret.second) 
            throw4("The dimension name ",(*ird)->name," should map to ",
                                      (*ird)->newname);
    }

    // Fourth: Change the original dimension new names to the unique dimension new names
    for (vector<GMCVar *>::iterator irv = this->cvars.begin();
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
    

void GMFile::Adjust_Dim_Name() throw(Exception){

#if 0
    // Just for debugging
for (vector<Var*>::iterator irv2 = this->vars.begin();
       irv2 != this->vars.end(); irv2++) {
    for (vector<Dimension *>::iterator ird = (*irv2)->dims.begin();
           ird !=(*irv2)->dims.end(); ird++) {
        // cerr<<"Dimension new name "<<(*ird)->newname <<endl;
       }
}
#endif

    // Only need for COARD conventions.
    if( true == iscoard) {
        for (vector<GMCVar *>::iterator irv = this->cvars.begin();
             irv !=this->cvars.end(); ++irv) {
            if ((*irv)->dims.size()!=1) 
                throw3("Coard coordinate variable ",(*irv)->name, "is not 1D");
            if ((*irv)->newname != (((*irv)->dims)[0]->newname)) {
                ((*irv)->dims)[0]->newname = (*irv)->newname;

                // For all variables that have this dimension,the dimension newname should also change.
                for (vector<Var*>::iterator irv2 = this->vars.begin();
                    irv2 != this->vars.end(); ++irv2) {
                    for (vector<Dimension *>::iterator ird = (*irv2)->dims.begin();
                        ird !=(*irv2)->dims.end(); ++ird) {
                        // This is the key, the dimension name of this dimension 
                        // should be equal to the dimension name of the coordinate variable.
                        // Then the dimension name matches and the dimension name should be changed to
                        // the new dimension name.
                        if ((*ird)->name == ((*irv)->dims)[0]->name) 
                            (*ird)->newname = ((*irv)->dims)[0]->newname;
                    }
                }
            } // if ((*irv)->newname != (((*irv)->dims)[0]->newname))
        }// for (vector<GMCVar *>::iterator irv = this->cvars.begin(); ...
   } // if( true == iscoard) 
}

void 
GMFile:: Add_Supplement_Attrs(bool add_path) throw(Exception) {

    if (General_Product == product_type || true == add_path) {
        File::Add_Supplement_Attrs(add_path);   

         // Adding variable original name(origname) and full path(fullpath)
        for (vector<GMCVar *>::iterator irv = this->cvars.begin();
                irv != this->cvars.end(); ++irv) {
            if (((*irv)->cvartype == CV_EXIST) || ((*irv)->cvartype == CV_MODIFY)) {
                Attribute * attr = new Attribute();
                const string varname = (*irv)->name;
                const string attrname = "origname";
                Add_Str_Attr(attr,attrname,varname);
                (*irv)->attrs.push_back(attr);
            }
        }

        for (vector<GMCVar *>::iterator irv = this->cvars.begin();
                irv != this->cvars.end(); ++irv) {
            if (((*irv)->cvartype == CV_EXIST) || ((*irv)->cvartype == CV_MODIFY)) {
                Attribute * attr = new Attribute();
                const string varname = (*irv)->fullpath;
                const string attrname = "fullnamepath";
                Add_Str_Attr(attr,attrname,varname);
                (*irv)->attrs.push_back(attr);
            }
        }

        for (vector<GMSPVar *>::iterator irv = this->spvars.begin();
                irv != this->spvars.end(); ++irv) {
            Attribute * attr = new Attribute();
            const string varname = (*irv)->name;
            const string attrname = "origname";
            Add_Str_Attr(attr,attrname,varname);
            (*irv)->attrs.push_back(attr);
        }

        for (vector<GMSPVar *>::iterator irv = this->spvars.begin();
                irv != this->spvars.end(); ++irv) {
            Attribute * attr = new Attribute();
            const string varname = (*irv)->fullpath;
            const string attrname = "fullnamepath";
            Add_Str_Attr(attr,attrname,varname);
            (*irv)->attrs.push_back(attr);
        }
    } // if (General_Product == product_type || true == add_path)

    if (Aqu_L3 == product_type) 
        Add_Aqu_Attrs();
    if (Mea_SeaWiFS_L2 == product_type || Mea_SeaWiFS_L3 == product_type) 
        Add_SeaWiFS_Attrs();
        
}

void 
GMFile:: Add_Aqu_Attrs() throw(Exception) {

    vector<HDF5CF::Var *>::const_iterator it_v;
    vector<HDF5CF::Attribute *>::const_iterator ira;

    const string orig_longname_attr_name = "Parameter";
    const string longname_attr_name ="long_name";
    string longname_value;
    

    const string orig_units_attr_name = "Units";
    const string units_attr_name = "units";
    string units_value;
    
    //    const string orig_valid_min_attr_name = "Data Minimum";
    const string orig_valid_min_attr_name = "Data Minimum";
    const string valid_min_attr_name = "valid_min";
    float valid_min_value;

    const string orig_valid_max_attr_name = "Data Maximum";
    const string valid_max_attr_name = "valid_max";
    float valid_max_value;

    // The fill value is -32767.0. However, No _FillValue attribute is added.
    // So add it here. KY 2012-2-16
 
    const string fill_value_attr_name = "_FillValue";
    float _FillValue = -32767.0;

    

    for (ira = this->root_attrs.begin(); ira != this->root_attrs.end(); ++ira) {
        if (orig_longname_attr_name == (*ira)->name) {
            Retrieve_H5_Attr_Value(*ira,"/");
            longname_value.resize((*ira)->value.size());
            copy((*ira)->value.begin(),(*ira)->value.end(),longname_value.begin());

        }
        else if (orig_units_attr_name == (*ira)->name) {
            Retrieve_H5_Attr_Value(*ira,"/");
            units_value.resize((*ira)->value.size());
            copy((*ira)->value.begin(),(*ira)->value.end(),units_value.begin());

        }
        else if (orig_valid_min_attr_name == (*ira)->name) {
            Retrieve_H5_Attr_Value(*ira,"/");
            memcpy(&valid_min_value,(void*)(&((*ira)->value[0])),(*ira)->value.size());
        }

        else if (orig_valid_max_attr_name == (*ira)->name) {
            Retrieve_H5_Attr_Value(*ira,"/");
            memcpy(&valid_max_value,(void*)(&((*ira)->value[0])),(*ira)->value.size());
        }
        
    }// for (ira = this->root_attrs.begin(); ira != this->root_attrs.end(); ++ira)

    // Level 3 variable name is l3m_data
    for (it_v = vars.begin();
                it_v != vars.end(); ++it_v) {
        if ("l3m_data" == (*it_v)->name) {

            // 1. Add the long_name attribute
            Attribute * attr = new Attribute();
            Add_Str_Attr(attr,longname_attr_name,longname_value);
            (*it_v)->attrs.push_back(attr);

            // 2. Add the units attribute
            attr = new Attribute();
            Add_Str_Attr(attr,units_attr_name,units_value);
            (*it_v)->attrs.push_back(attr);

            // 3. Add the valid_min attribute
            attr = new Attribute();
            Add_One_Float_Attr(attr,valid_min_attr_name,valid_min_value);
            (*it_v)->attrs.push_back(attr);

            // 4. Add the valid_max attribute
            attr = new Attribute();
            Add_One_Float_Attr(attr,valid_max_attr_name,valid_max_value);
            (*it_v)->attrs.push_back(attr);

            // 5. Add the _FillValue attribute
            attr = new Attribute();
            Add_One_Float_Attr(attr,fill_value_attr_name,_FillValue);
            (*it_v)->attrs.push_back(attr);

            break;
        }
    } // for (it_v = vars.begin(); ...
}

void 
GMFile:: Add_SeaWiFS_Attrs() throw(Exception) {

    // The fill value is -999.0. However, No _FillValue attribute is added.
    // So add it here. KY 2012-2-16
    const string fill_value_attr_name = "_FillValue";
    float _FillValue = -999.0;
    const string valid_range_attr_name = "valid_range";
    vector<HDF5CF::Var *>::const_iterator it_v;
    vector<HDF5CF::Attribute *>::const_iterator ira;


    for (it_v = vars.begin();
                it_v != vars.end(); ++it_v) {
        if (H5FLOAT32 == (*it_v)->dtype) {
            bool has_fillvalue = false;
            bool has_validrange = false;
            for(ira = (*it_v)->attrs.begin(); ira!= (*it_v)->attrs.end();ira++) {
                if (fill_value_attr_name == (*ira)->name){
                    has_fillvalue = true;
                    break;
                }

                else if(valid_range_attr_name == (*ira)->name) {
                    has_validrange = true;
                    break;
                }

            }
            // Add the fill value
            if (has_fillvalue != true && has_validrange != true ) {
                Attribute* attr = new Attribute();
                Add_One_Float_Attr(attr,fill_value_attr_name,_FillValue);
                (*it_v)->attrs.push_back(attr);
            }
        }// if (H5FLOAT32 == (*it_v)->dtype)
    }// for (it_v = vars.begin(); ...
}

void GMFile:: Handle_Coor_Attr() {

    string co_attrname = "coordinates";
    string co_attrvalue="";
    string unit_attrname = "units";
    string nonll_unit_attrvalue ="level";
    string lat_unit_attrvalue ="degrees_north";
    string lon_unit_attrvalue ="degrees_east";

    for (vector<GMCVar *>::iterator ircv = this->cvars.begin();
        ircv != this->cvars.end(); ++ircv) {

        if ((*ircv)->cvartype == CV_NONLATLON_MISS) {
           Attribute * attr = new Attribute();
           Add_Str_Attr(attr,unit_attrname,nonll_unit_attrvalue);
           (*ircv)->attrs.push_back(attr);
        }

        else if ((*ircv)->cvartype == CV_LAT_MISS) {
           Attribute * attr = new Attribute();
           Add_Str_Attr(attr,unit_attrname,lat_unit_attrvalue);
           (*ircv)->attrs.push_back(attr);
        }

        else if ((*ircv)->cvartype == CV_LON_MISS) {
           Attribute * attr = new Attribute();
           Add_Str_Attr(attr,unit_attrname,lon_unit_attrvalue);
           (*ircv)->attrs.push_back(attr);
        }
    } // for (vector<GMCVar *>::iterator ircv = this->cvars.begin(); ...
   
    if(product_type == Mea_SeaWiFS_L2) 
        return; 
    if (true == iscoard) 
        return;
   
    for (vector<Var *>::iterator irv = this->vars.begin();
                irv != this->vars.end(); ++irv) {
        bool coor_attr_keep_exist = false;

        for (vector<Attribute *>:: iterator ira =(*irv)->attrs.begin();
            ira !=(*irv)->attrs.end();++ira) {
            if (((*ira)->newname == "coordinates")) {
                if (product_type == SMAP) {
                    coor_attr_keep_exist = true;
                    break;
                }
                else {
                    delete (*ira);
                    (*irv)->attrs.erase(ira);
                    ira --;
                }
            }
        }// for (vector<Attribute *>:: iterator ira =(*irv)->attrs.begin(); ...

        if (true == coor_attr_keep_exist) 
            continue;
                
        for (vector<Dimension *>::iterator ird = (*irv)->dims.begin();
                ird != (*irv)->dims.end(); ++ ird) {
            for (vector<GMCVar *>::iterator ircv = this->cvars.begin();
                ircv != this->cvars.end(); ++ircv) {
                if ((*ird)->name == (*ircv)->cfdimname) 
                    co_attrvalue = (co_attrvalue.empty())
                                    ?(*ircv)->newname:co_attrvalue + " "+(*ircv)->newname;
            }
        }

        if (false == co_attrvalue.empty()) {
            Attribute * attr = new Attribute();
            Add_Str_Attr(attr,co_attrname,co_attrvalue);
            (*irv)->attrs.push_back(attr);
        }

        co_attrvalue.clear();
    } // for (vector<Var *>::iterator irv = this->vars.begin(); ...
}

void GMFile:: Create_Missing_CV(GMCVar *GMcvar, const string& dimname) throw(Exception) {

    GMcvar->name = dimname;
    GMcvar->newname = GMcvar->name;
    GMcvar->fullpath = GMcvar->name;
    GMcvar->rank = 1;
    GMcvar->dtype = H5INT32;
    hsize_t gmcvar_dimsize = dimname_to_dimsize[dimname];
    Dimension* gmcvar_dim = new Dimension(gmcvar_dimsize);
    gmcvar_dim->name = dimname;
    gmcvar_dim->newname = dimname;
    GMcvar->dims.push_back(gmcvar_dim);
    GMcvar->cfdimname = dimname;
    GMcvar->cvartype = CV_NONLATLON_MISS;
    GMcvar->product_type = product_type;
}

 // Check if this is just a netCDF-4 dimension. We need to check the dimension scale dataset attribute "NAME",
 // the value should start with "This is a netCDF dimension but not a netCDF variable".
bool GMFile::Is_netCDF_Dimension(Var *var) throw(Exception) {
    
    string netcdf_dim_mark = "This is a netCDF dimension but not a netCDF variable";

    bool is_only_dimension = false;

    for(vector<Attribute *>::iterator ira = var->attrs.begin();
          ira != var->attrs.end();ira++) {

        if ("NAME" == (*ira)->name) {

             Retrieve_H5_Attr_Value(*ira,var->fullpath);
             string name_value;
             name_value.resize((*ira)->value.size());
             copy((*ira)->value.begin(),(*ira)->value.end(),name_value.begin());

             // Compare the attribute "NAME" value with the string netcdf_dim_mark. We only compare the string with the size of netcdf_dim_mark
             if (0 == name_value.compare(0,netcdf_dim_mark.size(),netcdf_dim_mark))
                is_only_dimension = true;
           
            break;
        }
    } // for(vector<Attribute *>::iterator ira = var->attrs.begin(); ...

    return is_only_dimension;
}
    







