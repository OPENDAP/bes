// This file is part of the hdf5_handler implementing for the CF-compliant
// Copyright (c) 2011-2016 The HDF Group, Inc. and OPeNDAP, Inc.
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
/// \file HDF5CF.h
/// \brief This class specifies the core engineering of mapping HDF5 to DAP by following CF
///
///  It includes 
///  1) retrieving HDF5 metadata info.
///  2) Translatng HDF5 objects into DAP DDS and DAS by following CF conventions.
///  It covers the handling of generic NASA HDF5 products and HDF-EOS5 products.
///
///
/// \author Muqun Yang <myang6@hdfgroup.org>
///
/// Copyright (C) 2011-2016 The HDF Group
///
/// All rights reserved.

#ifndef _HDF5CF_H
#define _HDF5CF_H

#include<sstream>
#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <list>
#include <algorithm>
#include "HDF5CFUtil.h"
//#include "h5cfdaputil.h"
#include "HDF5GCFProduct.h"
#include "HE5Parser.h"

//enum CVType { CV_EXIST,CV_LAT_MISS,CV_LON_MISS,CV_NONLATLON_MISS,CV_FILLINDEX,CV_MODIFY,CV_SPECIAL,CV_UNSUPPORTED};
enum EOS5Type {
    GRID, SWATH, ZA, OTHERVARS
};
enum GMPattern {
    GENERAL_DIMSCALE, GENERAL_LATLON2D, GENERAL_LATLON1D, GENERAL_LATLON_COOR_ATTR, OTHERGMS
};
enum EOS5AuraName {
    OMI, MLS, HIRDLS, TES, NOTAURA
};
static string FILE_ATTR_TABLE_NAME = "HDF5_GLOBAL";

namespace HDF5CF {
class File;
class GMFile;
class EOS5File;

class Exception: public exception {
public:
    /// Constructor
    Exception(const string & msg) :
        message(msg)
    {
    }

    virtual ~ Exception() throw ()
    {
    }

    virtual const char *what() const throw ()
    {
        return this->message.c_str();
    }

    virtual void setException(string except_message)
    {
        this->message = except_message;
    }

protected:
    std::string message;
};
template<typename T, typename U, typename V, typename W, typename X> static void _throw5(const char *fname, int line,
    int numarg, const T & a1, const U & a2, const V & a3, const W & a4, const X & a5)
{
    ostringstream ss;
    ss << fname << ":" << line << ":";
    for (int i = 0; i < numarg; ++i) {
        ss << " ";
        switch (i) {
        case 0:
            ss << a1;
            break;
        case 1:
            ss << a2;
            break;
        case 2:
            ss << a3;
            break;
        case 3:
            ss << a4;
            break;
        case 4:
            ss << a5;
            break;
        default:
            break;
        }
    }
    throw Exception(ss.str());
}

/// The followings are convenient functions to throw exceptions with different
// number of arguments.
/// We assume that the maximum number of arguments is 5.
#define throw1(a1)  _throw5(__FILE__, __LINE__, 1, a1, 0, 0, 0, 0)
#define throw2(a1, a2)  _throw5(__FILE__, __LINE__, 2, a1, a2, 0, 0, 0)
#define throw3(a1, a2, a3)  _throw5(__FILE__, __LINE__, 3, a1, a2, a3, 0, 0)
#define throw4(a1, a2, a3, a4)  _throw5(__FILE__, __LINE__, 4, a1, a2, a3, a4, 0)
#define throw5(a1, a2, a3, a4, a5)  _throw5(__FILE__, __LINE__, 5, a1, a2, a3, a4, a5)

struct delete_elem {
    template<typename T> void operator ()(T * ptr)
    {
        delete ptr;
    }
};

/// This class repersents one dimension of an HDF5 dataset(variable).
// It holds only the size of that dimension.
// Note: currently the unlimited dimension(maxdims) case
// doesn't need to be considered.
class Dimension {
public:
    hsize_t getSize() const
    {
        return this->size;
    }
    const string & getName() const
    {
        return this->name;
    }
    const string & getNewName() const
    {
        return this->newname;
    }

    /// Has unlimited dimensions.
    bool HaveUnlimitedDim() const
    {
        return unlimited_dim;
    }

protected:
    Dimension(hsize_t dimsize) :
        size(dimsize), name(""), newname(""), unlimited_dim(false)
    {
    }

protected:
    hsize_t size;
    string name;
    string newname;
    bool unlimited_dim;

    friend class EOS5File;
    friend class GMFile;
    friend class File;
    friend class Var;
    friend class CVar;
    friend class GMCVar;
    friend class EOS5CVar;
    friend class GMSPVar;
};

/// This class represents  one attribute
class Attribute {

public:
    Attribute() :
        dtype(H5UNSUPTYPE), count(0), fstrsize(0)
    {
    }
    ;
    ~Attribute();

public:
    const string & getName() const
    {
        return this->name;
    }

    const string & getNewName() const
    {
        return this->newname;
    }

    H5DataType getType() const
    {
        return this->dtype;
    }

    hsize_t getCount() const
    {
        return this->count;
    }

    size_t getBufSize() const
    {
        return (this->value).size();
    }

    const std::vector<char>&getValue() const
    {
        return this->value;
    }

    const std::vector<size_t>&getStrSize() const
    {
        return this->strsize;
    }

protected:
    string name;
    string newname;
    H5DataType dtype;
    hsize_t count;
    vector<size_t> strsize;
    size_t fstrsize;
    vector<char> value;

    friend class File;
    friend class GMFile;
    friend class EOS5File;
    friend class Var;
    friend class CVar;
    friend class GMCVar;
    friend class GMSPVar;
    friend class EOS5CVar;
};

/// This class represents one HDF5 dataset(CF variable)
class Var {
public:
    Var() :
        dtype(H5UNSUPTYPE), rank(-1), comp_ratio(1), total_elems(0), unsupported_attr_dtype(false),
        unsupported_attr_dspace(false), unsupported_dspace(false), dimnameflag(false)
    {
    }
    Var(Var*var);
    virtual ~Var();

public:

    /// Get the original name of this variable
    const string & getName() const
    {
        return this->name;
    }

    /// Get the new name of this variable
    const string & getNewName() const
    {
        return this->newname;
    }

    /// Get the full path of this variable
    const string & getFullPath() const
    {
        return this->fullpath;
    }

    size_t getTotalElems() const
    {
        return this->total_elems;

    }

    /// Get the dimension rank of this variable
    int getRank() const
    {
        return this->rank;
    }

    /// Get the data type of this variable(Not HDF5 datatype id)
    H5DataType getType() const
    {
        return this->dtype;
    }

    const vector<Attribute *>&getAttributes() const
    {
        return this->attrs;
    }

    /// Get the list of the dimensions
    const vector<Dimension *>&getDimensions() const
    {
        return this->dims;
    }

    /// Get the compression ratio of this dataset
    int getCompRatio() const
    {
        return this->comp_ratio;
    }

protected:

    std::string newname;
    std::string name;
    std::string fullpath;
    H5DataType dtype;
    int rank;
    int comp_ratio;
    size_t total_elems;
    bool unsupported_attr_dtype;
    bool unsupported_attr_dspace;
    bool unsupported_dspace;
    bool dimnameflag;

    vector<Attribute *> attrs;
    vector<Dimension *> dims;

    friend class CVar;
    friend class GMCVar;
    friend class GMSPVar;
    friend class EOS5CVar;
    friend class File;
    friend class GMFile;
    friend class EOS5File;
};

/// This class is a derived class of Var. It represents a coordinate variable.
class CVar: public Var {
public:
    CVar() :
        cvartype(CV_UNSUPPORTED)
    {
    }
    ~CVar()
    {
    }
    /// Get the coordinate variable type of this variable
    CVType getCVType() const
    {
        return this->cvartype;
    }

    bool isLatLon() const;

private:
    // Each coordinate variable has and only has one dimension
    // This assumption is based on the exact match between
    // variables and dimensions
    string cfdimname;
    CVType cvartype;

    friend class File;
    friend class GMFile;
    friend class EOS5File;
};

/// This class is a derived class of Var. It represents a special general HDF5 product(currently ACOS and OCO-2)
class GMSPVar: public Var {
public:
    GMSPVar() :
        otype(H5UNSUPTYPE), sdbit(-1), numofdbits(-1)
    {
    }
    GMSPVar(Var *var);
    ~GMSPVar()
    {
    }
    H5DataType getOriginalType() const
    {
        return this->otype;
    }

    int getStartBit() const
    {
        return this->sdbit;
    }

    int getBitNum() const
    {
        return this->numofdbits;
    }

private:
    H5DataType otype;
    int sdbit;
    int numofdbits;

    friend class File;
    friend class GMFile;
};

/// This class is a derived class of CVar. It represents a coordinate variable for general HDF5 files.
class GMCVar: public CVar {
public:
    GMCVar() :
        product_type(General_Product)
    {
    }
    GMCVar(Var*var);
    ~GMCVar()
    {
    }

    /// Get the data type of this variable
    H5GCFProduct getPtType() const
    {
        return this->product_type;
    }

private:
    H5GCFProduct product_type;
    friend class GMFile;
};

/// This class is a derived class of CVar. It represents a coordinate variable for HDF-EOS5 files.
class EOS5CVar: public CVar {
public:
    EOS5CVar() :
        eos_type(OTHERVARS), is_2dlatlon(false), point_lower(0.0), point_upper(0.0), point_left(0.0), point_right(0.0), xdimsize(
            0), ydimsize(0), eos5_pixelreg(HE5_HDFE_CENTER),            // may change later
        eos5_origin(HE5_HDFE_GD_UL), // may change later
        eos5_projcode(HE5_GCTP_GEO), //may change later
        zone(-1), sphere(0)
    {
        std::fill_n(param, 13, 0);
    }
    ;
    EOS5CVar(Var *);

    ~EOS5CVar()
    {
    }

    EOS5Type getEos5Type() const
    {
        return this->eos_type;
    }
    float getPointLower() const
    {
        return this->point_lower;
    }
    float getPointUpper() const
    {
        return this->point_upper;
    }

    float getPointLeft() const
    {
        return this->point_left;
    }
    float getPointRight() const
    {
        return this->point_right;
    }

    EOS5GridPRType getPixelReg() const
    {
        return this->eos5_pixelreg;
    }
    EOS5GridOriginType getOrigin() const
    {
        return this->eos5_origin;
    }

    EOS5GridPCType getProjCode() const
    {
        return this->eos5_projcode;
    }

    int getXDimSize() const
    {
        return this->xdimsize;
    }

    int getYDimSize() const
    {
        return this->ydimsize;
    }

    std::vector<double> getParams() const
    {
        std::vector<double> ret_params;
        for (int i = 0; i < 13; i++)
            ret_params.push_back(param[i]);
        return ret_params;
    }

    int getZone() const
    {
        return this->zone;
    }

    int getSphere() const
    {
        return this->sphere;
    }

private:
    EOS5Type eos_type;
    bool is_2dlatlon;
    float point_lower;
    float point_upper;
    float point_left;
    float point_right;
    int xdimsize;
    int ydimsize;
    EOS5GridPRType eos5_pixelreg;
    EOS5GridOriginType eos5_origin;
    EOS5GridPCType eos5_projcode;

    double param[13];
    int zone;
    int sphere;
    friend class EOS5File;
};

/// This class represents an HDF5 group. The group will be flattened according to the CF conventions.
class Group {
public:
    Group() :
        unsupported_attr_dtype(false), unsupported_attr_dspace(false)
    {
    }
    ~Group();

public:

    /// Get the original path of this group
    const string & getPath() const
    {
        return this->path;
    }

    /// Get the new name of this group(flattened,name clashed checked)
    const string & getNewName() const
    {
        return this->newname;
    }

    const vector<Attribute *>&getAttributes() const
    {
        return this->attrs;
    }

protected:

    string newname;
    string path;

    vector<Attribute *> attrs;
    bool unsupported_attr_dtype;
    bool unsupported_attr_dspace;

    friend class File;
    friend class GMFile;
    friend class EOS5File;
};

/// This class retrieves all information from an HDF5 file.
class File {
public:

    /// Retrieve DDS information from the HDF5 file.
    /// The reason to separate reading DDS from DAS is:
    /// DAP needs to hold all values of attributes in the memory,
    /// building DDS doesn't need the attributes. So to reduce
    /// huge memory allocation for some HDF5 files, we separate
    /// the access of DAS from DDS although internally they
    /// still share common routines.
    virtual void Retrieve_H5_Info(const char *path, hid_t file_id, bool) throw (Exception);

    /// Retrieve attribute values for the supported HDF5 datatypes.
    virtual void Retrieve_H5_Supported_Attr_Values() throw (Exception);

    /// Retrieve coordinate variable attributes.
    virtual void Retrieve_H5_CVar_Supported_Attr_Values() = 0;

    /// Handle unsupported HDF5 datatypes
    virtual void Handle_Unsupported_Dtype(bool) throw (Exception);

    /// Handle unsupported HDF5 dataspaces for datasets
    virtual void Handle_Unsupported_Dspace(bool) throw (Exception);

    /// Handle other unmapped objects/attributes
    virtual void Handle_Unsupported_Others(bool) throw (Exception);

    /// Flatten the object name
    virtual void Flatten_Obj_Name(bool) throw (Exception);

    /// Add supplemental attributes such as fullpath and original name
    virtual void Add_Supplement_Attrs(bool) throw (Exception);

    /// Check if having Grid Mapping Attrs
    virtual bool Have_Grid_Mapping_Attrs();

    /// Handle Grid Mapping Vars
    virtual void Handle_Grid_Mapping_Vars();

    /// Handle "coordinates" attributes
    virtual void Handle_Coor_Attr() = 0;

    /// Handle coordinate variables
    virtual void Handle_CVar() = 0;

    /// Handle special variables
    virtual void Handle_SpVar() = 0;

    /// Handle special variable attributes
    virtual void Handle_SpVar_Attr() = 0;

    /// Adjust object names based on different products
    virtual void Adjust_Obj_Name() = 0;

    /// Adjust dimension names based on different products
    virtual void Adjust_Dim_Name() = 0;

    /// Handle dimension name clashing. Since COARDS requires the change of cv names,
    /// So we need to handle dimension name clashing specially.
    virtual void Handle_DimNameClashing() = 0;

    /// Obtain the HDF5 file ID
    hid_t getFileID() const
    {
        return this->fileid;
    }

    /// Obtain the path of the file
    const string & getPath() const
    {
        return this->path;
    }

    /// Public interface to obtain information of all variables.
    const vector<Var *>&getVars() const
    {
        return this->vars;
    }

    /// Public interface to obtain information of all attributes under the root group.
    const vector<Attribute *>&getAttributes() const
    {
        return this->root_attrs;
    }

    /// Public interface to obtain all the group info.
    const vector<Group *>&getGroups() const
    {
        return this->groups;
    }

    /// Has unlimited dimensions.
    bool HaveUnlimitedDim() const
    {
        return have_udim;
    }

    /// Obtain the flag to see if ignored objects should be generated.
    virtual bool Get_IgnoredInfo_Flag() = 0;

    /// Obtain the message that contains the ignored object info.
    virtual const string & Get_Ignored_Msg() = 0;

    virtual ~File();

protected:

    void Retrieve_H5_Obj(hid_t grp_id, const char*gname, bool include_attr) throw (Exception);
    void Retrieve_H5_Attr_Info(Attribute *, hid_t obj_id, const int j, bool& unsup_attr_dtype, bool & unsup_attr_dspace)
        throw (Exception);
    void Retrieve_H5_Attr_Value(Attribute *attr, string) throw (Exception);

    void Retrieve_H5_VarType(Var*, hid_t dset_id, const string& varname, bool &unsup_var_dtype) throw (Exception);
    void Retrieve_H5_VarDim(Var*, hid_t dset_id, const string &varname, bool & unsup_var_dspace) throw (Exception);

    float Retrieve_H5_VarCompRatio(Var*, hid_t) throw (Exception);

    void Handle_Group_Unsupported_Dtype() throw (Exception);
    void Handle_Var_Unsupported_Dtype() throw (Exception);
    void Handle_VarAttr_Unsupported_Dtype() throw (Exception);

    void Handle_GroupAttr_Unsupported_Dspace() throw (Exception);
    void Handle_VarAttr_Unsupported_Dspace() throw (Exception);

    void Gen_Group_Unsupported_Dtype_Info() throw (Exception);
    void Gen_Var_Unsupported_Dtype_Info() throw (Exception);
    virtual void Gen_VarAttr_Unsupported_Dtype_Info() throw (Exception);

    void Handle_GeneralObj_NameClashing(bool, set<string> &objnameset) throw (Exception);
    void Handle_Var_NameClashing(set<string> &objnameset) throw (Exception);
    void Handle_Group_NameClashing(set<string> &objnameset) throw (Exception);
    void Handle_Obj_AttrNameClashing() throw (Exception);
    template<typename T> void Handle_General_NameClashing(set<string>&objnameset, vector<T*>& objvec) throw (Exception);

    void Add_One_FakeDim_Name(Dimension *dim) throw (Exception);
    void Adjust_Duplicate_FakeDim_Name(Dimension * dim) throw (Exception);
    void Insert_One_NameSizeMap_Element(string name, hsize_t size, bool unlimited) throw (Exception);
    void Insert_One_NameSizeMap_Element2(map<string, hsize_t> &, map<string, bool>&, string name, hsize_t size,
        bool unlimited) throw (Exception);
    //void Replace_Dim_Name_All(const string orig_dim_name,const string new_dim_name) throw(Exception);

    virtual string get_CF_string(string);
    virtual void Replace_Var_Info(Var* src, Var *target);
    virtual void Replace_Var_Attrs(Var *src, Var*target);

    void Add_Str_Attr(Attribute* attr, const string &attrname, const string& strvalue) throw (Exception);
    //bool Var_Has_Attr(Var*var,const string &attrname);
    string Retrieve_Str_Attr_Value(Attribute *attr, const string var_path);
    bool Is_Str_Attr(Attribute* attr, string varfullpath, const string &attrname, const string& strvalue);
    void Add_One_Float_Attr(Attribute* attr, const string &attrname, float float_value) throw (Exception);
    void Replace_Var_Str_Attr(Var* var, const string &attr_name, const string& strvalue);
    void Change_Attr_One_Str_to_Others(Attribute *attr, Var *var) throw (Exception);

    // Check if having variable latitude by variable names (containing ???latitude/Latitude/lat)
    bool Is_geolatlon(const string &var_name, bool is_lat);
    bool has_latlon_cf_units(Attribute*attr, const string &varfullpath, bool is_lat);

    // Check if a variable with a var name is under a specific group with groupname
    // note: the variable's size at each dimension is also returned. The user must allocate the
    // memory for the dimension sizes(an array(vector is perferred).
    bool is_var_under_group(const string &varname, const string &grpname, const int var_rank, vector<size_t> &var_size);

    virtual void Gen_Unsupported_Dtype_Info(bool) = 0;
    virtual void Gen_Unsupported_Dspace_Info() throw (Exception);
    void Gen_DimScale_VarAttr_Unsupported_Dtype_Info() throw (Exception);
    void add_ignored_info_page_header();
    void add_ignored_info_obj_header();
    // void add_ignored_info_obj_dtype_header();
    //void add_ignored_info_obj_dspace_header();
    void add_ignored_info_links_header();
    void add_ignored_info_links(const string& link_name);
    void add_ignored_info_namedtypes(const string&, const string&);
    void add_ignored_info_attrs(bool is_grp, const string & obj_path, const string &attr_name);
    void add_ignored_info_objs(bool is_dim_related, const string & obj_path);
    void add_no_ignored_info();
    bool ignored_dimscale_ref_list(Var *var);
    bool Check_DropLongStr(Var *var, Attribute *attr) throw (Exception);
    void add_ignored_var_longstr_info(Var*var, Attribute *attr) throw (Exception);
    void add_ignored_grp_longstr_info(const string& grp_path, const string& attr_name);
    void add_ignored_droplongstr_hdr();
    bool Check_VarDropLongStr(const string &varpath, const vector<Dimension *>&, H5DataType) throw (Exception);

    void release_standalone_var_vector(vector<Var*>&vars);

    // Handle grid_mapping attributes
    string Check_Grid_Mapping_VarName(const string& attr_value,const string& var_full_path);
    string Check_Grid_Mapping_FullPath(const string& attr_value);

protected:
    File(const char *h5_path, hid_t file_id) :
        path(string(h5_path)), fileid(file_id), rootid(-1), unsupported_var_dtype(false), unsupported_attr_dtype(false), unsupported_var_dspace(
            false), unsupported_attr_dspace(false), unsupported_var_attr_dspace(false), addeddimindex(0), check_ignored(
            false), have_ignored(false), have_udim(false)
    {
    }

protected:

    string path;
    hid_t fileid;
    hid_t rootid;

    /// Var vectors.
    vector<Var *> vars;

    /// Root attribute vectors.
    vector<Attribute *> root_attrs;

    /// Non-root group vectors.
    vector<Group*> groups;

    bool unsupported_var_dtype;
    bool unsupported_attr_dtype;

    bool unsupported_var_dspace;
    bool unsupported_attr_dspace;
    bool unsupported_var_attr_dspace;

    set<string> dimnamelist;
    //set<string>unlimited_dimnamelist;
    map<string, hsize_t> dimname_to_dimsize;

    // Unlimited dim. info
    map<string, bool> dimname_to_unlimited;

    /// Handle added dimension names
    map<hsize_t, string> dimsize_to_fakedimname;
    int addeddimindex;

    bool check_ignored;
    bool have_ignored;
    bool have_udim;
    string ignored_msg;

};

/// This class is a derived class of File. It includes methods applied to general HDF5 files only.
class GMFile: public File {
public:
    GMFile(const char*path, hid_t file_id, H5GCFProduct product, GMPattern gproduct_pattern);
    virtual ~GMFile();
public:
    H5GCFProduct getProductType() const
    {
        return product_type;
    }

    const vector<GMCVar *>&getCVars() const
    {
        return this->cvars;
    }

    const vector<GMSPVar *>&getSPVars() const
    {
        return this->spvars;
    }

    /// Retrieve DDS information from the HDF5 file; real implementation for general HDF5 products.
    void Retrieve_H5_Info(const char *path, hid_t file_id, bool include_attr) throw (Exception);

    /// Retrieve attribute values for the supported HDF5 datatypes for general HDF5 products.
    void Retrieve_H5_Supported_Attr_Values() throw (Exception);

    void Retrieve_H5_CVar_Supported_Attr_Values();

    /// Adjust attribute values for general HDF5 products.
    void Adjust_H5_Attr_Value(Attribute *attr) throw (Exception);

    /// Handle unsupported HDF5 datatypes for general HDF5 products.
    void Handle_Unsupported_Dtype(bool) throw (Exception);

    /// Handle unsupported HDF5 dataspaces for general HDF5 products
    void Handle_Unsupported_Dspace(bool) throw (Exception);

    /// Handle other unmapped objects/attributes for general HDF5 products
    void Handle_Unsupported_Others(bool) throw (Exception);

    /// Add dimension name
    void Add_Dim_Name() throw (Exception);

    /// Handle coordinate variables for general NASA HDF5 products
    void Handle_CVar() throw (Exception);

    /// Handle special variables for general NASA HDF5 products
    void Handle_SpVar() throw (Exception);

    /// Handle special variable attributes for general NASA HDF5 products
    void Handle_SpVar_Attr() throw (Exception);

    /// Adjust object names based on different general NASA HDF5 products
    void Adjust_Obj_Name() throw (Exception);

    /// Flatten the object name for general NASA HDF5 products
    void Flatten_Obj_Name(bool include_attr) throw (Exception);

    /// Handle object name clashing for general NASA HDF5 products
    void Handle_Obj_NameClashing(bool) throw (Exception);

    /// Adjust dimension name for general NASA HDF5 products
    void Adjust_Dim_Name() throw (Exception);

    void Handle_DimNameClashing() throw (Exception);

    /// Add supplemental attributes such as fullpath and original name for general NASA HDF5 products
    void Add_Supplement_Attrs(bool) throw (Exception);

    /// Check if having Grid Mapping Attrs
    bool Have_Grid_Mapping_Attrs();
    
    /// Handle Grid Mapping Vars
    void Handle_Grid_Mapping_Vars();

    //
    bool Is_Hybrid_EOS5();
    void Handle_Hybrid_EOS5();
 
    /// Handle "coordinates" attributes for general HDF5 products
    void Handle_Coor_Attr();

    /// Unsupported datatype array may generate FakeDim. Remove them.
    void Remove_Unused_FakeDimVars();

    /// Update "product type" attributes for general HDF5 products
    void Update_Product_Type() throw (Exception);

    /// Obtain ignored info. flag
    bool Get_IgnoredInfo_Flag()
    {
        return check_ignored;
    }

    /// Get the message that contains the ignored obj. info
    const string& Get_Ignored_Msg()
    {
        return ignored_msg;
    }

protected:
    void Add_Dim_Name_GPM() throw (Exception);
    void Add_Dim_Name_Mea_SeaWiFS() throw (Exception);
    void Handle_UseDimscale_Var_Dim_Names_Mea_SeaWiFS_Ozone(Var*) throw (Exception);
    void Add_UseDimscale_Var_Dim_Names_Mea_SeaWiFS_Ozone(Var *, Attribute*) throw (Exception);

    void Add_Dim_Name_Mea_Ozonel3z() throw (Exception);
    bool check_cv(string & varname) throw (Exception);

    void Add_Dim_Name_Aqu_L3() throw (Exception);
    void Add_Dim_Name_OBPG_L3() throw (Exception);
    void Add_Dim_Name_OSMAPL2S() throw (Exception);
    void Add_Dim_Name_ACOS_L2S_OCO2_L1B() throw (Exception);

    void Add_Dim_Name_General_Product() throw (Exception);
    void Check_General_Product_Pattern() throw (Exception);
    bool Check_Dimscale_General_Product_Pattern() throw (Exception);
    bool Check_LatLon2D_General_Product_Pattern() throw (Exception);
    bool Check_LatLon2D_General_Product_Pattern_Name_Size(const string& latname, const string& lonname)
        throw (Exception);
    bool Check_LatLon1D_General_Product_Pattern() throw (Exception);
    bool Check_LatLon1D_General_Product_Pattern_Name_Size(const string& latname, const string& lonname)
        throw (Exception);

    bool Check_LatLon_With_Coordinate_Attr_General_Product_Pattern() throw (Exception);
    void Build_lat1D_latlon_candidate(Var*, const vector<Var*>&);
    void Build_latg1D_latlon_candidate(Var*, const vector<Var*>&);
    void Build_unique_latlon_candidate();
    //bool Check_LatLonName_General_Product(int latlon_rank) throw(Exception);
    void Add_Dim_Name_LatLon1D_Or_CoordAttr_General_Product() throw (Exception);
    void Add_Dim_Name_LatLon2D_General_Product() throw (Exception);
    void Add_Dim_Name_Dimscale_General_Product() throw (Exception);
    void Handle_UseDimscale_Var_Dim_Names_General_Product(Var*) throw (Exception);
    void Add_UseDimscale_Var_Dim_Names_General_Product(Var*, Attribute*) throw (Exception);

    //bool Check_2DLatLon_Dimscale(string &latname, string &lonname) throw(Exception);
    //void Update_2DLatLon_Dimscale_CV(const string & latname, const string & lonname) throw(Exception);

    // Check if we have 2-D lat/lon CVs, and if yes, add those to the CV list.
    void Update_M2DLatLon_Dimscale_CVs() throw (Exception);
    bool Check_1DGeolocation_Dimscale() throw (Exception);
    void Obtain_1DLatLon_CVs(vector<GMCVar*> &cvar_1dlat, vector<GMCVar*> &cvar_1dlon);
    void Obtain_2DLatLon_Vars(vector<Var*> &var_2dlat, vector<Var*> &var_2dlon,
        map<string, int>&latlon2d_path_to_index);
    void Obtain_2DLLVars_With_Dims_not_1DLLCVars(vector<Var*> &var_2dlat, vector<Var*> &var_2dlon,
        vector<GMCVar*> &cvar_1dlat, vector<GMCVar*> &cvar_1dlon, map<string, int>&latlon2d_path_to_index);
    void Obtain_2DLLCVar_Candidate(vector<Var*> &var_2dlat, vector<Var*> &var_2dlon,
        map<string, int>&latlon2d_path_to_index) throw (Exception);
    void Obtain_unique_2dCV(vector<Var*>&, map<string, int>&);
    void Remove_2DLLCVar_Final_Candidate_from_Vars(vector<int>&) throw (Exception);

    void Handle_CVar_GPM_L1() throw (Exception);
    void Handle_CVar_GPM_L3() throw (Exception);
    void Handle_CVar_Mea_SeaWiFS() throw (Exception);
    void Handle_CVar_Aqu_L3() throw (Exception);
    void Handle_CVar_OBPG_L3() throw (Exception);
    void Handle_CVar_OSMAPL2S() throw (Exception);
    void Handle_CVar_Mea_Ozone() throw (Exception);
    void Handle_SpVar_ACOS_OCO2() throw (Exception);
    void Handle_CVar_Dimscale_General_Product() throw (Exception);
    void Handle_CVar_LatLon2D_General_Product() throw (Exception);
    void Handle_CVar_LatLon1D_General_Product() throw (Exception);
    void Handle_CVar_LatLon_General_Product() throw (Exception);

    void Adjust_Mea_Ozone_Obj_Name() throw (Exception);
    void Adjust_GPM_L3_Obj_Name() throw (Exception);

    void Handle_GMCVar_NameClashing(set<string> &) throw (Exception);
    void Handle_GMCVar_AttrNameClashing() throw (Exception);
    void Handle_GMSPVar_NameClashing(set<string> &) throw (Exception);
    void Handle_GMSPVar_AttrNameClashing() throw (Exception);
    template<typename T> void GMHandle_General_NameClashing(set<string>&objnameset, vector<T*>& objvec)
        throw (Exception);

    string get_CF_string(string s);

    // The following routines are for generating coordinates attributes for netCDF-4 like 2D-latlon cases.
    bool Check_Var_2D_CVars(Var*) throw (Exception);
    bool Flatten_VarPath_In_Coordinates_Attr(Var*) throw (Exception);
    void Handle_LatLon_With_CoordinateAttr_Coor_Attr() throw (Exception);
    bool Coord_Match_LatLon_NameSize(const string & coord_values) throw (Exception);
    bool Coord_Match_LatLon_NameSize_Same_Group(const string & coord_values, const string &var_path) throw (Exception);
    void Add_VarPath_In_Coordinates_Attr(Var*, const string &);

    // The following three routines handle the GPM CF-related attributes
    void Handle_GPM_l1_Coor_Attr() throw (Exception);
    void Correct_GPM_L1_LatLon_units(Var *var, const string unit_value) throw (Exception);
    void Add_GPM_Attrs() throw (Exception);

    void Add_Aqu_Attrs() throw (Exception);
    void Add_SeaWiFS_Attrs() throw (Exception);
    void Create_Missing_CV(GMCVar*, const string &) throw (Exception);

    bool Is_netCDF_Dimension(Var *var) throw (Exception);

    void Gen_Unsupported_Dtype_Info(bool);
    void Gen_VarAttr_Unsupported_Dtype_Info() throw (Exception);
    void Gen_GM_VarAttr_Unsupported_Dtype_Info();
    void Gen_Unsupported_Dspace_Info() throw (Exception);
    void Handle_GM_Unsupported_Dtype(bool) throw (Exception);
    void Handle_GM_Unsupported_Dspace(bool) throw (Exception);

    bool Remove_EOS5_Strings(string &);
    bool Remove_EOS5_Strings_NonEOS_Fields (string &);
    void release_standalone_GMCVar_vector(vector<GMCVar*> &tempgc_vars);

private:
    H5GCFProduct product_type;
    GMPattern gproduct_pattern;
    vector<GMCVar *> cvars;
    vector<GMSPVar *> spvars;
    string gp_latname;
    string gp_lonname;
    set<string> grp_cv_paths;
    vector<struct Name_Size_2Pairs> latloncv_candidate_pairs;
    //map<string,string>dimcvars_2dlatlon;
    bool iscoard;
    bool ll2d_no_cv;

};

/// This class simulates an HDF-EOS5 Grid. Currently only geographic projection is supported.
class EOS5CFGrid {
public:
    EOS5CFGrid() :
        point_lower(0.0), point_upper(0.0), point_left(0.0), point_right(0.0),
        eos5_pixelreg(HE5_HDFE_CENTER),            // may change later
        eos5_origin(HE5_HDFE_GD_UL), // may change later
        eos5_projcode(HE5_GCTP_GEO), //may change later
        zone(-1), sphere(0),
        addeddimindex(0),
        xdimsize(0), ydimsize(0),
        has_nolatlon(true), has_1dlatlon(false), has_2dlatlon(false), has_g2dlatlon(false)
    {
        fill_n(param, 13, 0);
    }

    ~EOS5CFGrid()
    {
    }

protected:
    void Update_Dimnamelist();

private:
    float point_lower;
    float point_upper;
    float point_left;
    float point_right;
    EOS5GridPRType eos5_pixelreg;
    EOS5GridOriginType eos5_origin;
    EOS5GridPCType eos5_projcode;

    double param[13];
    int zone;
    int sphere;

    vector<string> dimnames;
    set<string> vardimnames;
    map<string, hsize_t> dimnames_to_dimsizes;

    // Unlimited dim. info
    map<string, bool> dimnames_to_unlimited;

    map<hsize_t, string> dimsizes_to_dimnames;
    int addeddimindex;

    map<string, string> dnames_to_1dvnames;
    string name;
    int xdimsize;
    int ydimsize;
    //bool has_nodimnames_vars;
    bool has_nolatlon;
    bool has_1dlatlon;
    bool has_2dlatlon;
    bool has_g2dlatlon;

    friend class EOS5File;
};

/// This class simulates an HDF-EOS5 Swath.
class EOS5CFSwath {
public:
    EOS5CFSwath() :
        addeddimindex(0), has_nolatlon(true), has_1dlatlon(false), has_2dlatlon(false), has_g2dlatlon(false)
    {
    }

    ~EOS5CFSwath()
    {
    }

private:

    vector<string> dimnames;
    set<string> vardimnames;
    map<string, hsize_t> dimnames_to_dimsizes;

    // Unlimited dim. info
    map<string, bool> dimnames_to_unlimited;

    map<hsize_t, string> dimsizes_to_dimnames;
    int addeddimindex;

    map<string, string> dnames_to_geo1dvnames;
    string name;
    bool has_nolatlon;
    bool has_1dlatlon;
    bool has_2dlatlon;
    bool has_g2dlatlon;

    friend class EOS5File;
};

/// This class simulates an HDF-EOS5 Zonal average object.
class EOS5CFZa {
public:
    EOS5CFZa() :
        addeddimindex(0)
    {
    }

    ~EOS5CFZa()
    {
    }

private:

    vector<string> dimnames;
    set<string> vardimnames;
    map<string, hsize_t> dimnames_to_dimsizes;
    // Unlimited dim. info
    map<string, bool> dimnames_to_unlimited;

    map<hsize_t, string> dimsizes_to_dimnames;
    int addeddimindex;

    map<string, string> dnames_to_1dvnames;
    string name;

    friend class EOS5File;
};

/// This class is a derived class of File. It includes methods applied to HDF-EOS5 files only.
class EOS5File: public File {
public:
    EOS5File(const char*he5_path, hid_t file_id) :
        File(he5_path, file_id), iscoard(false), grids_multi_latloncvs(false), isaura(false), aura_name(NOTAURA),
        orig_num_grids(0)
    {
    }

    virtual ~EOS5File();
public:

    /// Obtain coordinate variables for HDF-EOS5 products
    const vector<EOS5CVar *>&getCVars() const
    {
        return this->cvars;
    }

    /// Retrieve DDS information from the HDF5 file; a real implementation for HDF-EOS5 products
    void Retrieve_H5_Info(const char *path, hid_t file_id, bool include_attr) throw (Exception);

    /// Retrieve attribute values for the supported HDF5 datatypes for HDF-EOS5 products.
    void Retrieve_H5_Supported_Attr_Values() throw (Exception);

    /// Retrieve coordinate variable attributes.
    void Retrieve_H5_CVar_Supported_Attr_Values();

    /// Handle unsupported HDF5 datatypes for HDF-EOS5 products.
    void Handle_Unsupported_Dtype(bool) throw (Exception);

    /// Handle unsupported HDF5 dataspaces for HDF-EOS5 products.
    void Handle_Unsupported_Dspace(bool) throw (Exception);

    /// Handle other unmapped objects/attributes for HDF-EOS5 products
    void Handle_Unsupported_Others(bool) throw (Exception);

    /// Adjust HDF-EOS5 dimension information
    void Adjust_EOS5Dim_Info(HE5Parser*strmeta_info) throw (Exception);

    /// Add HDF-EOS5 dimension and coordinate variable related info. to EOS5Grid,EOS5Swath etc.
    void Add_EOS5File_Info(HE5Parser*, bool) throw (Exception);

    /// Adjust variable names for HDF-EOS5 files
    void Adjust_Var_NewName_After_Parsing() throw (Exception);

    /// This method is a no-op operation. Leave here since the method in the base class is pure virtual.
    void Adjust_Obj_Name() throw (Exception);

    /// Add the dimension name for HDF-EOS5 files
    void Add_Dim_Name(HE5Parser *) throw (Exception);

    /// Check if the HDF-EOS5 file is an Aura file. Special CF operations need to be used.
    void Check_Aura_Product_Status() throw (Exception);

    /// Handle coordinate variable for HDF-EOS5 files
    void Handle_CVar() throw (Exception);

    /// Handle special variables for HDF-EOS5 files
    void Handle_SpVar() throw (Exception);

    /// Handle special variables for HDF-EOS5 files
    void Handle_SpVar_Attr() throw (Exception);

    /// Adjust variable dimension names before the flattening for HDF-EOS5 files.
    void Adjust_Var_Dim_NewName_Before_Flattening() throw (Exception);

    /// Flatten the object name for HDF-EOS5 files
    void Flatten_Obj_Name(bool include_attr) throw (Exception);

    /// Set COARDS flag
    void Set_COARDS_Status() throw (Exception);

    /// Adjust the attribute info for HDF-EOS5 products
    void Adjust_Attr_Info() throw (Exception);

//        void Adjust_Special_EOS5CVar_Name() throw(Exception);

    /// Handle the object name clashing for HDF-EOS5 products
    void Handle_Obj_NameClashing(bool) throw (Exception);

    /// Add the supplemental attributes for HDF-EOS5 products
    void Add_Supplement_Attrs(bool) throw (Exception);

    /// Handle the coordinates attribute for HDF-EOS5 products
    void Handle_Coor_Attr();

    /// Adjust the dimension name for HDF-EOS5 products
    void Adjust_Dim_Name() throw (Exception);

    void Handle_DimNameClashing() throw (Exception);

    /// Check if having Grid Mapping Attrs
    bool Have_Grid_Mapping_Attrs();
    
    /// Handle Grid Mapping Vars
    void Handle_Grid_Mapping_Vars();


    bool Get_IgnoredInfo_Flag()
    {
        return check_ignored;
    }

    const string& Get_Ignored_Msg()
    {
        return ignored_msg;
    }

protected:
    void Adjust_H5_Attr_Value(Attribute *attr) throw (Exception);

    void Adjust_EOS5Dim_List(vector<HE5Dim>&) throw (Exception);
    void Condense_EOS5Dim_List(vector<HE5Dim>&) throw (Exception);
    void Remove_NegativeSizeDims(vector<HE5Dim>&) throw (Exception);
    void Adjust_EOS5VarDim_Info(vector<HE5Dim>&, vector<HE5Dim>&, const string &, EOS5Type) throw (Exception);

    void EOS5Handle_nonlatlon_dimcvars(vector<HE5Var> & eos5varlist, EOS5Type, string groupname,
        map<string, string>& dnamesgeo1dvnames) throw (Exception);
    template<class T> void EOS5SwathGrid_Set_LatLon_Flags(T* eos5gridswath, vector<HE5Var>& eos5varlist)
        throw (Exception);

    void Obtain_Var_NewName(Var*) throw (Exception);
    EOS5Type Get_Var_EOS5_Type(Var*) throw (Exception);

    bool Obtain_Var_Dims(Var*, HE5Parser*) throw (Exception);
    template<class T> bool Set_Var_Dims(T*, Var*, vector<HE5Var>&, const string&, int, EOS5Type) throw (Exception);
    template<class T> void Create_Unique_DimName(T*, set<string>&, Dimension *, int, EOS5Type) throw (Exception);

    template<class T> bool Check_All_DimNames(T*, string &, hsize_t);
    string Obtain_Var_EOS5Type_GroupName(Var*, EOS5Type) throw (Exception);
    int Check_EOS5Swath_FieldType(Var*) throw (Exception);
    void Get_Unique_Name(set<string>&, string&) throw (Exception);

    template<class T> string Create_Unique_FakeDimName(T*, EOS5Type) throw (Exception);
    template<class T> void Set_NonParse_Var_Dims(T*, Var*, map<hsize_t, string>&, int, EOS5Type) throw (Exception);

    void Handle_Grid_CVar(bool) throw (Exception);
    void Handle_Augmented_Grid_CVar() throw (Exception);
    template<class T> void Handle_Single_Augment_CVar(T*, EOS5Type) throw (Exception);

    void Handle_Multi_Nonaugment_Grid_CVar() throw (Exception);
    void Handle_Single_Nonaugment_Grid_CVar(EOS5CFGrid*) throw (Exception);
    bool Handle_Single_Nonaugment_Grid_CVar_OwnLatLon(EOS5CFGrid *, set<string>&) throw (Exception);
    bool Handle_Single_Nonaugment_Grid_CVar_EOS5LatLon(EOS5CFGrid *, set<string>&) throw (Exception);
    void Handle_NonLatLon_Grid_CVar(EOS5CFGrid *, set<string>&) throw (Exception);
    void Remove_MultiDim_LatLon_EOS5CFGrid() throw (Exception);
    void Adjust_EOS5GridDimNames(EOS5CFGrid *) throw (Exception);

    void Handle_Swath_CVar(bool) throw (Exception);
    void Handle_Single_1DLatLon_Swath_CVar(EOS5CFSwath *cfswath, bool is_augmented) throw (Exception);
    void Handle_Single_2DLatLon_Swath_CVar(EOS5CFSwath *cfswath, bool is_augmented) throw (Exception);
    void Handle_NonLatLon_Swath_CVar(EOS5CFSwath *cfswath, set<string>& tempvardimnamelist) throw (Exception);
    void Handle_Special_NonLatLon_Swath_CVar(EOS5CFSwath *cfswath, set<string>&tempvardimnamelist) throw (Exception);

    void Handle_Za_CVar(bool) throw (Exception);

    bool Check_Augmentation_Status() throw (Exception);
    // Don't remove the following commented line!
    //bool Check_Augmented_Var_Attrs(Var *var) throw(Exception);
    template<class T> bool Check_Augmented_Var_Candidate(T*, Var*, EOS5Type) throw (Exception);

    template<class T> void Adjust_Per_Var_Dim_NewName_Before_Flattening(T*, bool, int, int, int) throw (Exception);
    void Adjust_SharedLatLon_Grid_Var_Dim_Name() throw (Exception);

    void Adjust_Aura_Attr_Name() throw (Exception);
    void Adjust_Aura_Attr_Value() throw (Exception);
    void Handle_EOS5CVar_Unit_Attr() throw (Exception);
    void Add_EOS5_Grid_CF_Attr() throw (Exception);
    void Handle_Aura_Special_Attr() throw (Exception);

    string get_CF_string(string s);
    void Replace_Var_Info(EOS5CVar *src, EOS5CVar *target);
    void Replace_Var_Attrs(EOS5CVar *src, EOS5CVar *target);
    void Handle_EOS5CVar_NameClashing(set<string> &) throw (Exception);
    void Handle_EOS5CVar_AttrNameClashing() throw (Exception);
    template<typename T> void EOS5Handle_General_NameClashing(set<string>&objnameset, vector<T*>& objvec)
        throw (Exception);
    //void Adjust_CF_attr() throw (Exception);
    template<typename T> void Create_Missing_CV(T*, EOS5CVar*, const string &, EOS5Type, int) throw (Exception);
    void Create_Added_Var_NewName_FullPath(EOS5Type, const string&, const string&, string &, string &) throw (Exception);


    //void add_ignored_info_attrs(bool is_grp,bool is_first);
    //void add_ignored_info_objs(bool is_dim_related, bool is_first);

    //bool ignored_var_transformed();
    //bool ignored_var_attr_transformed();

    void Handle_EOS5_Unsupported_Dtype(bool) throw (Exception);
    void Handle_EOS5_Unsupported_Dspace(bool) throw (Exception);

    void Gen_Unsupported_Dtype_Info(bool);
    void Gen_VarAttr_Unsupported_Dtype_Info() throw (Exception);
    void Gen_EOS5_VarAttr_Unsupported_Dtype_Info() throw (Exception);

    //void Gen_DimScale_VarAttr_Unsupported_Dtype_Info() throw(Exception);
    void Gen_Unsupported_Dspace_Info() throw (Exception);

private:
    vector<EOS5CVar *> cvars;
    vector<EOS5CFGrid *> eos5cfgrids;
    vector<EOS5CFSwath *> eos5cfswaths;
    vector<EOS5CFZa *> eos5cfzas;
    map<string, string> eos5_to_cf_attr_map;
    bool iscoard;
    EOS5AuraName aura_name;
    bool grids_multi_latloncvs;
    bool isaura;
    int orig_num_grids;
    multimap<string, string> dimname_to_dupdimnamelist;
};

}
#endif
