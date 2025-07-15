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
/// \author Kent Yang <myang6@hdfgroup.org>
///
/// Copyright (C) 2011-2022 The HDF Group
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
#include "HDF5GCFProduct.h"
#include "HE5Parser.h"

// "enum CVType: CV_EXIST,CV_LAT_MISS,CV_LON_MISS,CV_NONLATLON_MISS,CV_FILLINDEX,CV_MODIFY,CV_SPECIAL,CV_UNSUPPORTED"
enum EOS5Type {
    GRID, SWATH, ZA, OTHERVARS
};
enum GMPattern {
    GENERAL_DIMSCALE, GENERAL_LATLON2D, GENERAL_LATLON1D, GENERAL_LATLON_COOR_ATTR, OTHERGMS
};
enum EOS5AuraName {
    OMI, MLS, HIRDLS, TES, NOTAURA
};
static std::string FILE_ATTR_TABLE_NAME = "HDF5_GLOBAL";

namespace HDF5CF {
class File;
class GMFile;
class EOS5File;

class Exception: public std::exception {
public:
    /// Constructor
    explicit Exception(const std::string & msg) :
        message(msg)
    {
    }

    ~ Exception() throw () override = default;

    const char *what() const throw () override
    {
        return this->message.c_str();
    }

    virtual void setException(const std::string& except_message)
    {
        this->message = except_message;
    }

private:
    std::string message;
};
template<typename T, typename U, typename V, typename W, typename X> static void _throw5(const char *fname, int line,
    int numarg, const T & a1, const U & a2, const V & a3, const W & a4, const X & a5)
{
    std::ostringstream ss;
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
    const std::string & getName() const
    {
        return this->name;
    }
    const std::string & getNewName() const
    {
        return this->newname;
    }

    /// Has unlimited dimensions.
    bool HaveUnlimitedDim() const
    {
        return unlimited_dim;
    }

    explicit Dimension(hsize_t dimsize) :
        size(dimsize)
    {
    }

private:
    hsize_t size;
    std::string name ="";
    std::string newname = "";
    bool unlimited_dim = false;

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
    Attribute() = default;

    ~Attribute() = default;

    const std::string & getName() const
    {
        return this->name;
    }

    const std::string & getNewName() const
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

    bool getCsetType() const {
        return this->is_cset_ascii;
    }

private:
    std::string name;
    std::string newname;
    H5DataType dtype = H5UNSUPTYPE;
    hsize_t count = 0;
    std::vector<size_t> strsize;
    size_t fstrsize = 0;
    std::vector<char> value;
    bool is_cset_ascii = true;

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
    Var() = default;
    explicit Var(const Var*var);
    virtual ~Var();


    /// Get the original name of this variable
    const std::string & getName() const
    {
        return this->name;
    }

    /// Get the new name of this variable
    const std::string & getNewName() const
    {
        return this->newname;
    }

    /// Get the full path of this variable
    const std::string & getFullPath() const
    {
        return this->fullpath;
    }

    size_t getTotalElems() const
    {
        return this->total_elems;

    }

    bool getZeroStorageSize() const
    {
        return this->zero_storage_size;
    }

    bool getCoorAttrAddPath() const
    {
        return this->coord_attr_add_path;
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

    const std::vector<Attribute *>&getAttributes() const
    {
        return this->attrs;
    }

    /// Get the list of the dimensions
    const std::vector<Dimension *>&getDimensions() const
    {
        return this->dims;
    }

    /// Get the compression ratio of this dataset
    float getCompRatio() const
    {
        return this->comp_ratio;
    }

private:

    std::string newname;
    std::string name;
    std::string fullpath;
    H5DataType dtype = H5UNSUPTYPE;
    int rank = -1;
    float comp_ratio = 1.0;
    size_t total_elems = 0;
    bool zero_storage_size = false;
    bool unsupported_attr_dtype = false;
    bool unsupported_attr_dspace = false;
    bool unsupported_dspace = false;
    bool dimnameflag = false;
    bool coord_attr_add_path = true;

    std::vector<Attribute *> attrs;
    std::vector<Dimension *> dims;

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

    CVar() = default;
    ~CVar() override = default;

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
    std::string cfdimname;
    CVType cvartype = CV_UNSUPPORTED;

    friend class File;
    friend class GMFile;
    friend class EOS5File;
};

/// This class is a derived class of Var. It represents a special general HDF5 product(currently ACOS and OCO-2)
class GMSPVar: public Var {
public:
    GMSPVar() = default;
    explicit GMSPVar(const Var *var);
    ~GMSPVar() override = default;

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
    H5DataType otype = H5UNSUPTYPE;
    int sdbit = -1;
    int numofdbits = -1;

    friend class File;
    friend class GMFile;
};

/// This class is a derived class of CVar. It represents a coordinate variable for general HDF5 files.
class GMCVar: public CVar {
public:
    GMCVar()  = default;
    explicit GMCVar(const Var*var);
    ~GMCVar() override = default;

    /// Get the data type of this variable
    H5GCFProduct getPtType() const
    {
        return this->product_type;
    }

private:
    H5GCFProduct product_type = General_Product;
    friend class GMFile;
};

/// This class is a derived class of CVar. It represents a coordinate variable for HDF-EOS5 files.
class EOS5CVar: public CVar {
public:
    EOS5CVar() 
    {
        std::fill_n(param, 13, 0);
    }
    ;
    explicit EOS5CVar(const Var *);

    ~EOS5CVar() override = default;

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
        for (const auto &param_ele:param)
            ret_params.push_back(param_ele);
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
    EOS5Type eos_type = OTHERVARS;
    bool is_2dlatlon = false;
    float point_lower = 0.0;
    float point_upper = 0.0;
    float point_left = 0.0;
    float point_right = 0.0;
    int xdimsize = 0;
    int ydimsize = 0;
    EOS5GridPRType eos5_pixelreg = HE5_HDFE_CENTER; // May change later
    EOS5GridOriginType eos5_origin = HE5_HDFE_GD_UL; // May change later
    EOS5GridPCType eos5_projcode = HE5_GCTP_GEO; // May change later
    int zone = -1;
    int sphere = 0;
    double param[13];
    friend class EOS5File;
};

/// This class represents an HDF5 group. The group will be flattened according to the CF conventions.
class Group {
public:
    Group() = default;
    ~Group();


    /// Get the original path of this group
    const std::string & getPath() const
    {
        return this->path;
    }

    /// Get the new name of this group(flattened,name clashed checked)
    const std::string & getNewName() const
    {
        return this->newname;
    }

    const std::vector<Attribute *>&getAttributes() const
    {
        return this->attrs;
    }

private:

    std::string newname;
    std::string path;

    std::vector<Attribute *> attrs;
    bool unsupported_attr_dtype = false;
    bool unsupported_attr_dspace = false;

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
    virtual void Retrieve_H5_Info(const char *path, hid_t file_id, bool);

    /// Retrieve attribute values for the supported HDF5 datatypes.
    virtual void Retrieve_H5_Supported_Attr_Values();

    /// Retrieve attribute values for a variable
    virtual void Retrieve_H5_Var_Attr_Values(Var *var);

    /// Retrieve coordinate variable attributes.
    virtual void Retrieve_H5_CVar_Supported_Attr_Values() = 0;

    /// Handle unsupported HDF5 datatypes
    virtual void Handle_Unsupported_Dtype(bool);

    /// Handle unsupported HDF5 dataspaces for datasets
    virtual void Handle_Unsupported_Dspace(bool);

    /// Handle other unmapped objects/attributes
    virtual void Handle_Unsupported_Others(bool) ;

    /// Flatten the object name
    virtual void Flatten_Obj_Name(bool) ;

    /// Add supplemental attributes such as fullpath and original name
    virtual void Add_Supplement_Attrs(bool) ;

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

    /// Handle Special variable and attributes for DMR
    virtual void Handle_SpVar_DMR() = 0;

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
    const std::string & getPath() const
    {
        return this->path;
    }

    /// Public interface to obtain information of all variables.
    const std::vector<Var *>&getVars() const
    {
        return this->vars;
    }

    /// Public interface to obtain information of all attributes under the root group.
    const std::vector<Attribute *>&getAttributes() const
    {
        return this->root_attrs;
    }

    /// Public interface to obtain all the group info.
    const std::vector<Group *>&getGroups() const
    {
        return this->groups;
    }

    /// Has unlimited dimensions.
    bool HaveUnlimitedDim() const
    {
        return have_udim;
    }

    void setDap4(bool is_dap4)  
    {
        _is_dap4 = is_dap4;
    }
    bool getDap4() const 
    {
        return _is_dap4;
    }

    bool getIsCOARD() const 
    {
        return iscoard;
    }

  
    
    /// Obtain the flag to see if ignored objects should be generated.
    virtual bool Get_IgnoredInfo_Flag() = 0;

    /// Obtain the message that contains the ignored object info.
    virtual const std::string & Get_Ignored_Msg() = 0;

    virtual ~File();

protected:

    void Retrieve_H5_Obj(hid_t grp_id, const char*gname, bool include_attr);
    void Retrieve_H5_Attr_Info(Attribute *, hid_t obj_id, const int j, bool& unsup_attr_dtype, bool & unsup_attr_dspace) const;
        
    void Retrieve_H5_Attr_Value(Attribute *attr, const std::string &) const;

    void Retrieve_H5_VarType(Var*, hid_t dset_id, const std::string& varname, bool &unsup_var_dtype) const; 
    void Retrieve_H5_VarDim(Var*, hid_t dset_id, const std::string &varname, bool & unsup_var_dspace);

    float Retrieve_H5_VarCompRatio(const Var*, const hid_t) const;

    void Handle_Group_Unsupported_Dtype() ;
    void Handle_Var_Unsupported_Dtype() ;
    void Handle_VarAttr_Unsupported_Dtype() ;

    void Handle_GroupAttr_Unsupported_Dspace() ;
    void Handle_VarAttr_Unsupported_Dspace() ;

    void Gen_Group_Unsupported_Dtype_Info() ;
    void Gen_Var_Unsupported_Dtype_Info() ;
    virtual void Gen_VarAttr_Unsupported_Dtype_Info() ;

    void Handle_GeneralObj_NameClashing(bool, std::set<std::string> &objnameset) ;
    void Handle_Var_NameClashing(std::set<std::string> &objnameset) ;
    void Handle_Group_NameClashing(std::set<std::string> &objnameset) ;
    void Handle_Obj_AttrNameClashing() ;
    template<typename T> void Handle_General_NameClashing(std::set<std::string>&objnameset, std::vector<T*>& objvec) ;

    void Add_One_FakeDim_Name(Dimension *dim) ;
    void Adjust_Duplicate_FakeDim_Name(Dimension * dim) ;
    void Adjust_Duplicate_FakeDim_Name2(Dimension * dim,int dup_dim_size_index) ;
    void Insert_One_NameSizeMap_Element(const std::string & name, hsize_t size, bool unlimited) ;
    void Insert_One_NameSizeMap_Element2(std::map<std::string, hsize_t> &, std::map<std::string, bool>&, const std::string &name, hsize_t size,
        bool unlimited) const;

    virtual std::string get_CF_string(std::string);
    void Replace_Var_Info(const Var* src, Var *target);
    void Replace_Var_Attrs(const Var *src, Var*target);

    void Add_Str_Attr(Attribute* attr, const std::string &attrname, const std::string& strvalue) const;
    std::string Retrieve_Str_Attr_Value(Attribute *attr, const std::string& var_path) const;
    bool Is_Str_Attr(Attribute* attr, const std::string & varfullpath, const std::string &attrname, const std::string& strvalue);
    void Add_One_Float_Attr(Attribute* attr, const std::string &attrname, float float_value) const;
    void Replace_Var_Str_Attr(Var* var, const std::string &attr_name, const std::string& strvalue);
    void Change_Attr_One_Str_to_Others(Attribute *attr, const Var *var) const;

    // Check if having variable latitude by variable names (containing ???latitude/Latitude/lat)
    bool Is_geolatlon(const std::string &var_name, bool is_lat) const;
    bool has_latlon_cf_units(Attribute*attr, const std::string &varfullpath, bool is_lat);

    // Check if a variable with a var name is under a specific group with groupname
    // note: the variable's size at each dimension is also returned. The user must allocate the
    // memory for the dimension sizes(an array(vector is perferred).
    bool is_var_under_group(const std::string &varname, const std::string &grpname, const int var_rank, std::vector<size_t> &var_size) const;

    // Remove netCDF internal attributes. Right now, only CLASS=DIMENSION_SCALE and NAME=Var_name and NAME=pure netCDF dimesnion are handled.
    void remove_netCDF_internal_attributes(bool include_attr);

    virtual void Gen_Unsupported_Dtype_Info(bool) = 0;
    virtual void Gen_Unsupported_Dspace_Info() ;
    void Gen_DimScale_VarAttr_Unsupported_Dtype_Info() ;
    void add_ignored_info_page_header();
    void add_ignored_info_obj_header();
#if 0
    // void add_ignored_info_obj_dtype_header();
    //void add_ignored_info_obj_dspace_header();
#endif
    void add_ignored_info_links_header();
    void add_ignored_info_links(const std::string& link_name);
    void add_ignored_info_namedtypes(const std::string&, const std::string&);
    void add_ignored_info_attrs(bool is_grp, const std::string & obj_path, const std::string &attr_name);
    void add_ignored_info_objs(bool is_dim_related, const std::string & obj_path);
    void add_no_ignored_info();
    bool ignored_dimscale_ref_list(const Var *var) const;
    bool Check_DropLongStr(const Var *var, const Attribute *attr) const;
    void add_ignored_var_longstr_info(const Var*var, const Attribute *attr) ;
    void add_ignored_grp_longstr_info(const std::string& grp_path, const std::string& attr_name);
    void add_ignored_droplongstr_hdr();
    bool Check_VarDropLongStr(const std::string &varpath, const std::vector<Dimension *>&, H5DataType) const;

    void release_standalone_var_vector(std::vector<Var*>&vars);

    // Handle grid_mapping attributes
    std::string Check_Grid_Mapping_VarName(const std::string& attr_value,const std::string& var_full_path) const;
    std::string Check_Grid_Mapping_FullPath(const std::string& attr_value) const;

protected:
    File(const char *h5_path, hid_t file_id) :
        path(std::string(h5_path)), fileid(file_id)
    {
    }

    // TODO: Will see if having time to make the following memembers private. See ESDIS-560
    std::string path;
    hid_t fileid;
    hid_t rootid = -1;

    /// Var vectors.
    std::vector<Var *> vars;

    /// Root attribute vectors.
    std::vector<Attribute *> root_attrs;

    /// Non-root group vectors.
    std::vector<Group*> groups;

    bool unsupported_var_dtype = false;
    bool unsupported_attr_dtype = false;

    bool unsupported_var_dspace = false;
    bool unsupported_attr_dspace = false;
    bool unsupported_var_attr_dspace = false;

    std::set<std::string> dimnamelist;
    //"set<string>unlimited_dimnamelist "
    std::map<std::string, hsize_t> dimname_to_dimsize;

    // Unlimited dim. info
    std::map<std::string, bool> dimname_to_unlimited;

    /// Handle added dimension names
    std::map<hsize_t, std::string> dimsize_to_fakedimname;
    int addeddimindex = 0;
    std::vector<std::pair<hsize_t, std::string>>dup_dimsize_dimname;

    bool check_ignored = false;
    bool have_ignored = false;
    bool have_udim = false;
    bool _is_dap4 = false;
    bool iscoard = false;
    std::string ignored_msg;

};

/// This class is a derived class of File. It includes methods applied to general HDF5 files only.
class GMFile: public File {
public:
    GMFile(const char*path, hid_t file_id, H5GCFProduct product, GMPattern gproduct_pattern);
    ~GMFile() override;

    H5GCFProduct getProductType() const
    {
        return product_type;
    }

    const std::vector<GMCVar *>&getCVars() const
    {
        return this->cvars;
    }

    const std::vector<GMSPVar *>&getSPVars() const
    {
        return this->spvars;
    }

    bool is_special_gpm_l3() const
    {
        return this->special_gpm_l3;
    }

    /// Retrieve DDS information from the HDF5 file; real implementation for general HDF5 products.
    void Retrieve_H5_Info(const char *path, hid_t file_id, bool include_attr) override;

    /// Retrieve attribute values for the supported HDF5 datatypes for general HDF5 products.
    void Retrieve_H5_Supported_Attr_Values() override;

    void Retrieve_H5_CVar_Supported_Attr_Values() override;

    /// Adjust attribute values for general HDF5 products.
    void Adjust_H5_Attr_Value(Attribute *attr) const;

    /// Handle unsupported HDF5 datatypes for general HDF5 products.
    void Handle_Unsupported_Dtype(bool) override;

    /// Handle unsupported HDF5 dataspaces for general HDF5 products
    void Handle_Unsupported_Dspace(bool) override;

    /// Handle other unmapped objects/attributes for general HDF5 products
    void Handle_Unsupported_Others(bool) override;

    /// Remove unneeded objects 
    void Remove_Unneeded_Objects();

    /// Add dimension name
    void Add_Dim_Name() ;

    /// Handle coordinate variables for general NASA HDF5 products
    void Handle_CVar() override;

    /// Handle special variables for general NASA HDF5 products
    void Handle_SpVar() override;

    /// Handle special variable attributes for general NASA HDF5 products
    void Handle_SpVar_Attr() override;

    /// Handle special variables and attributes for general NASA HDF5 files(for DMR)
    /// Note this function is not used.
    void Handle_SpVar_DMR() override { };

    /// Adjust object names based on different general NASA HDF5 products
    void Adjust_Obj_Name() override;

    /// Flatten the object name for general NASA HDF5 products
    void Flatten_Obj_Name(bool include_attr) override;

    /// Handle object name clashing for general NASA HDF5 products
    void Handle_Obj_NameClashing(bool) ;

    /// Adjust dimension name for general NASA HDF5 products
    void Adjust_Dim_Name() override;

    void Handle_DimNameClashing() override;

    /// Add supplemental attributes such as fullpath and original name for general NASA HDF5 products
    void Add_Supplement_Attrs(bool) override;

    /// Check if having Grid Mapping Attrs
    bool Have_Grid_Mapping_Attrs()override;
    
    /// Handle Grid Mapping Vars
    void Handle_Grid_Mapping_Vars()override;

    //
    bool Is_Hybrid_EOS5() const;
    void Handle_Hybrid_EOS5();
 
    /// Handle "coordinates" attributes for general HDF5 products
    void Handle_Coor_Attr()override;

    /// Unsupported datatype array may generate FakeDim. Remove them.
    void Remove_Unused_FakeDimVars();

    /// Remove the _nc4_non_coord from the variable new names
    void Rename_NC4_NonCoordVars() const;

    /// Update "product type" attributes for general HDF5 products
    void Update_Product_Type() ;

    /// Update the coordinate attribute to include path and also flatten
    void Add_Path_Coord_Attr();

    /// Update the Bounds attribute to follow the CF conventions
    void Update_Bounds_Attr();
   
    /// Update the netCDF-4 pure dimension size when the pure dimension is an unlimited dimension
    void Update_NC4_PureDimSize() const;

    /// Obtain ignored info. flag
    bool Get_IgnoredInfo_Flag() override
    {
        return check_ignored;
    }

    /// Get the message that contains the ignored obj. info
    const std::string& Get_Ignored_Msg() override
    {
        return ignored_msg;
    }

protected:
    bool Check_And_Update_New_GPM_L3();
    void Remove_OMPSNPP_InputPointers();
    void Add_Dim_Name_GPM() ;
    void Add_Dim_Name_Mea_SeaWiFS() ;
    void Handle_UseDimscale_Var_Dim_Names_Mea_SeaWiFS_Ozone(Var*) ;
    void Add_UseDimscale_Var_Dim_Names_Mea_SeaWiFS_Ozone(const Var *, const Attribute*) ;

    void Add_Dim_Name_Mea_Ozonel3z() ;
    bool check_cv(const std::string & varname) const;

    void Add_Dim_Name_Aqu_L3() const;
    void Add_Dim_Name_OBPG_L3() ;
    void Add_Dim_Name_OSMAPL2S() ;
    void Add_Dim_Name_ACOS_L2S_OCO2_L1B() ;

    void Add_Dim_Name_General_Product() ;
    void Check_General_Product_Pattern() ;
    bool Check_Dimscale_General_Product_Pattern() ;
    bool Check_LatLon2D_General_Product_Pattern() ;
    bool Check_LatLon2D_General_Product_Pattern_Name_Size(const std::string& latname, const std::string& lonname)
        ;
    bool Check_LatLon1D_General_Product_Pattern() ;
    bool Check_LatLon1D_General_Product_Pattern_Name_Size(const std::string& latname, const std::string& lonname)
        ;

    bool Check_LatLon_With_Coordinate_Attr_General_Product_Pattern() ;
    void Build_lat1D_latlon_candidate(const Var*, const std::vector<Var*>&);
    void Build_latg1D_latlon_candidate(const Var*, const std::vector<Var*>&);
    void Build_unique_latlon_candidate();
    void Add_Dim_Name_LatLon1D_Or_CoordAttr_General_Product() ;
    void Add_Dim_Name_LatLon2D_General_Product() ;
    void Add_Dim_Name_Dimscale_General_Product() ;
    void Handle_UseDimscale_Var_Dim_Names_General_Product(Var*) ;
    void Add_UseDimscale_Var_Dim_Names_General_Product(const Var*, const Attribute*) ;

    // Check if we have 2-D lat/lon CVs, and if yes, add those to the CV list.
    void Update_M2DLatLon_Dimscale_CVs() ;
    bool Check_1DGeolocation_Dimscale() ;
    void Obtain_1DLatLon_CVs(std::vector<GMCVar*> &cvar_1dlat, std::vector<GMCVar*> &cvar_1dlon);
    void Obtain_2DLatLon_Vars(std::vector<Var*> &var_2dlat, std::vector<Var*> &var_2dlon,
        std::map<std::string, int>&latlon2d_path_to_index);
    void Obtain_2DLLVars_With_Dims_not_1DLLCVars(std::vector<Var*> &var_2dlat, std::vector<Var*> &var_2dlon,
        const std::vector<GMCVar*> &cvar_1dlat, const std::vector<GMCVar*> &cvar_1dlon, std::map<std::string, int>&latlon2d_path_to_index);
    void Obtain_2DLLCVar_Candidate(std::vector<Var*> &var_2dlat, std::vector<Var*> &var_2dlon,
        std::map<std::string, int>&latlon2d_path_to_index) ;
    void Obtain_unique_2dCV(std::vector<Var*>&, std::map<std::string, int>&);
    void Remove_2DLLCVar_Final_Candidate_from_Vars(std::vector<int>&) ;

    void Handle_CVar_GPM_L1() ;
    void Handle_CVar_GPM_L3() ;
    void Handle_CVar_Mea_SeaWiFS() ;
    void Handle_CVar_Aqu_L3() ;
    void Handle_CVar_OBPG_L3() ;
    void Handle_CVar_OSMAPL2S() ;
    void Handle_CVar_Mea_Ozone() ;
    void Handle_SpVar_ACOS_OCO2() ;
    void Handle_CVar_Dimscale_General_Product() ;
    void Handle_CVar_LatLon2D_General_Product() ;
    void Handle_CVar_LatLon1D_General_Product() ;
    void Handle_CVar_LatLon_General_Product() ;

    void Adjust_Mea_Ozone_Obj_Name() const;
    void Adjust_GPM_L3_Obj_Name() const;


    void Handle_GMCVar_NameClashing(std::set<std::string> &) ;
    void Handle_GMCVar_AttrNameClashing() ;
    void Handle_GMSPVar_NameClashing(std::set<std::string> &) ;
    void Handle_GMSPVar_AttrNameClashing() ;
    template<typename T> void GMHandle_General_NameClashing(std::set<std::string>&objnameset, std::vector<T*>& objvec)
        ;

    std::string get_CF_string(std::string s) override;

    // The following routines are for generating coordinates attributes for netCDF-4 like 2D-latlon cases.
    bool Check_Var_2D_CVars(Var*) const;
    bool Flatten_VarPath_In_Coordinates_Attr(Var*) ;
#if 0
    //bool Flatten_VarPath_In_Coordinates_Attr_EOS5(Var*) ;
#endif
    void Handle_LatLon_With_CoordinateAttr_Coor_Attr() ;
    bool Coord_Match_LatLon_NameSize(const std::string & coord_values) ;
    bool Coord_Match_LatLon_NameSize_Same_Group(const std::string & coord_values, const std::string &var_path) ;
    void Add_VarPath_In_Coordinates_Attr(Var*, const std::string &);

    // The following three routines handle the GPM CF-related attributes
    void Handle_GPM_l1_Coor_Attr() const;
    void Correct_GPM_L1_LatLon_units(Var *var, const std::string & unit_value) ;
    void Add_GPM_Attrs() ;

    void Add_Aqu_Attrs() ;
    void Add_SeaWiFS_Attrs() const;
    void Create_Missing_CV(GMCVar*, const std::string &) ;

    bool Is_netCDF_Dimension(const Var *var) const;

    void Gen_Unsupported_Dtype_Info(bool) override;
    void Gen_VarAttr_Unsupported_Dtype_Info()  override;
    void Gen_GM_VarAttr_Unsupported_Dtype_Info();
    void Gen_Unsupported_Dspace_Info()  override;
    void Handle_GM_Unsupported_Dtype(bool) ;
    void Handle_GM_Unsupported_Dspace(bool) ;

    bool Remove_EOS5_Strings(std::string &) const;
    bool Remove_EOS5_Strings_NonEOS_Fields (std::string &) const;
    void release_standalone_GMCVar_vector(std::vector<GMCVar*> &tempgc_vars);

private:
    H5GCFProduct product_type;
    GMPattern gproduct_pattern;
    std::vector<GMCVar *> cvars;
    std::vector<GMSPVar *> spvars;
    std::string gp_latname;
    std::string gp_lonname;
    std::set<std::string> grp_cv_paths;
    std::set<std::string> nc4_sdimv_dv_path;
    std::vector<struct Name_Size_2Pairs> latloncv_candidate_pairs;
    //"map<string,string>dimcvars_2dlatlon"
#if 0
    bool ll2d_no_cv;
#endif
    bool have_nc4_non_coord = false;
    bool special_gpm_l3 = false;

};

/// This class simulates an HDF-EOS5 Grid. Currently only geographic projection is supported.
class EOS5CFGrid {
public:
    EOS5CFGrid() 
    {
        std::fill_n(param, 13, 0);
    }

    ~EOS5CFGrid() = default;

protected:
    void Update_Dimnamelist();

private:
    float point_lower = 0.0;
    float point_upper = 0.0;
    float point_left = 0.0;
    float point_right = 0.0;
    EOS5GridPRType eos5_pixelreg = HE5_HDFE_CENTER; // may change later
    EOS5GridOriginType eos5_origin = HE5_HDFE_GD_UL; // may change later
    EOS5GridPCType eos5_projcode = HE5_GCTP_GEO; // may change later

    double param[13];
    int zone = -1;
    int sphere = 0;

    std::vector<std::string> dimnames;
    std::set<std::string> vardimnames;
    std::map<std::string, hsize_t> dimnames_to_dimsizes;

    // Unlimited dim. info
    std::map<std::string, bool> dimnames_to_unlimited;

    std::map<hsize_t, std::string> dimsizes_to_dimnames;
    int addeddimindex = 0;

    std::map<std::string, std::string> dnames_to_1dvnames;
    std::string name;
    int xdimsize = 0;
    int ydimsize = 0;
    bool has_nolatlon = true;
    bool has_1dlatlon = false;
    bool has_2dlatlon = false;
    bool has_g2dlatlon = false;

    friend class EOS5File;
};

/// This class simulates an HDF-EOS5 Swath.
class EOS5CFSwath {
public:
    EOS5CFSwath() = default;
    ~EOS5CFSwath() = default;

private:

    std::vector<std::string> dimnames;
    std::set<std::string> vardimnames;
    std::map<std::string, hsize_t> dimnames_to_dimsizes;

    // Unlimited dim. info
    std::map<std::string, bool> dimnames_to_unlimited;

    std::map<hsize_t, std::string> dimsizes_to_dimnames;
    int addeddimindex = 0;

    std::map<std::string, std::string> dnames_to_geo1dvnames;
    std::string name;
    bool has_nolatlon = true;
    bool has_1dlatlon = false;
    bool has_2dlatlon = false;
    bool has_g2dlatlon = false;

    friend class EOS5File;
};

/// This class simulates an HDF-EOS5 Zonal average object.
class EOS5CFZa {
public:
    EOS5CFZa() = default;

    ~EOS5CFZa() = default;

private:

    std::vector<std::string> dimnames;
    std::set<std::string> vardimnames;
    std::map<std::string, hsize_t> dimnames_to_dimsizes;
    // Unlimited dim. info
    std::map<std::string, bool> dimnames_to_unlimited;

    std::map<hsize_t, std::string> dimsizes_to_dimnames;
    int addeddimindex = 0;

    std::map<std::string, std::string> dnames_to_1dvnames;
    std::string name;

    friend class EOS5File;
};

/// This class is a derived class of File. It includes methods applied to HDF-EOS5 files only.
class EOS5File: public File {

public:
    EOS5File(const char*he5_path, hid_t file_id) :
        File(he5_path, file_id)
    {
    }

    ~EOS5File() override;

    /// Obtain coordinate variables for HDF-EOS5 products
    const std::vector<EOS5CVar *>&getCVars() const
    {
        return this->cvars;
    }

    /// Retrieve DDS information from the HDF5 file; a real implementation for HDF-EOS5 products
    void Retrieve_H5_Info(const char *path, hid_t file_id, bool include_attr) override;

    /// Retrieve attribute values for the supported HDF5 datatypes for HDF-EOS5 products.
    void Retrieve_H5_Supported_Attr_Values()  override;

    /// Retrieve coordinate variable attributes.
    void Retrieve_H5_CVar_Supported_Attr_Values() override;

    /// Handle unsupported HDF5 datatypes for HDF-EOS5 products.
    void Handle_Unsupported_Dtype(bool) override ;

    /// Handle unsupported HDF5 dataspaces for HDF-EOS5 products.
    void Handle_Unsupported_Dspace(bool) override;

    /// Handle other unmapped objects/attributes for HDF-EOS5 products
    void Handle_Unsupported_Others(bool)  override;

    /// Adjust HDF-EOS5 dimension information
    void Adjust_EOS5Dim_Info(HE5Parser*strmeta_info) ;

    /// Add HDF-EOS5 dimension and coordinate variable related info. to EOS5Grid,EOS5Swath etc.
    void Add_EOS5File_Info(HE5Parser*, bool) ;

    /// Adjust variable names for HDF-EOS5 files
    void Adjust_Var_NewName_After_Parsing() const;

    /// This method is a no-op operation. Leave here since the method in the base class is pure virtual.
    void Adjust_Obj_Name()  override;

    /// Add the dimension name for HDF-EOS5 files
    void Add_Dim_Name(HE5Parser *) ;

    /// Check if the HDF-EOS5 file is an Aura file. Special CF operations need to be used.
    void Check_Aura_Product_Status() ;

    /// Handle coordinate variable for HDF-EOS5 files
    void Handle_CVar()  override;

    /// Handle special variables for HDF-EOS5 files
    void Handle_SpVar()  override;

    /// Handle special variables for HDF-EOS5 files
    void Handle_SpVar_Attr()  override;

    /// Handle special variables and attributes for HDF-EOS5 files(for DMR)
    void Handle_SpVar_DMR()  override;
     
    /// Adjust variable dimension names before the flattening for HDF-EOS5 files.
    void Adjust_Var_Dim_NewName_Before_Flattening() const;

    /// Flatten the object name for HDF-EOS5 files
    void Flatten_Obj_Name(bool include_attr) override ;

    /// Set COARDS flag
    void Set_COARDS_Status() ;

    /// Adjust the attribute info for HDF-EOS5 products
    void Adjust_Attr_Info() ;

    /// Handle the object name clashing for HDF-EOS5 products
    void Handle_Obj_NameClashing(bool) ;

    /// Add the supplemental attributes for HDF-EOS5 products
    void Add_Supplement_Attrs(bool)  override;

    /// Handle the coordinates attribute for HDF-EOS5 products
    void Handle_Coor_Attr() override;

    /// Adjust the dimension name for HDF-EOS5 products
    void Adjust_Dim_Name()  override;

    void Handle_DimNameClashing()  override;

    /// Check if having Grid Mapping Attrs
    bool Have_Grid_Mapping_Attrs() override;
    
    /// Handle Grid Mapping Vars
    void Handle_Grid_Mapping_Vars() override;


    bool Have_EOS5_Grids() const {
        return !(this->eos5cfgrids.empty());
    }
    bool Get_IgnoredInfo_Flag() override
    {
        return check_ignored;
    }

    const std::string& Get_Ignored_Msg() override
    {
        return ignored_msg;
    }

protected:
    void Adjust_H5_Attr_Value(const Attribute *attr) const;

    void Adjust_EOS5Dim_List(std::vector<HE5Dim>&) const;
    void Condense_EOS5Dim_List(std::vector<HE5Dim>&) const;
    void Remove_NegativeSizeDims(std::vector<HE5Dim>&) const;
    void Adjust_EOS5DimSize_List(std::vector<HE5Dim>&,const std::vector<HE5Var>&, const EOS5Type,const std::string & eos5objname) const;
    void Adjust_EOS5VarDim_Info(std::vector<HE5Dim>&, std::vector<HE5Dim>&, const std::string &, EOS5Type) ;

    void EOS5Handle_nonlatlon_dimcvars(std::vector<HE5Var> & eos5varlist, EOS5Type, const std::string &groupname,
        std::map<std::string, std::string>& dnamesgeo1dvnames) const;
    template<class T> void EOS5SwathGrid_Set_LatLon_Flags(T* eos5gridswath, std::vector<HE5Var>& eos5varlist) const;

    void Obtain_Var_NewName(Var*) const;
    EOS5Type Get_Var_EOS5_Type(const Var*) const;

    bool Obtain_Var_Dims(const Var*, HE5Parser*) ;
    template<class T> bool Set_Var_Dims(T*, const Var*, std::vector<HE5Var>&, const std::string&, int, EOS5Type) ;
    template<class T> void Create_Unique_DimName(T*, std::set<std::string>&, Dimension *, int, EOS5Type) ;

    template<class T> bool Check_All_DimNames(T*, std::string &, hsize_t) const;
    std::string Obtain_Var_EOS5Type_GroupName(const Var*, EOS5Type) const;
    int Check_EOS5Swath_FieldType(const Var*) const;
    void Get_Unique_Name(std::set<std::string>&, std::string&) const;

    template<class T> std::string Create_Unique_FakeDimName(T*, EOS5Type) const;
    template<class T> void Set_NonParse_Var_Dims(T*, const Var*, const std::map<hsize_t, std::string>&, int, EOS5Type) ;

    void Handle_Grid_CVar(bool) ;
    void Handle_Augmented_Grid_CVar() ;
    template<class T> void Handle_Single_Augment_CVar(T*, EOS5Type) ;

    void Handle_Multi_Nonaugment_Grid_CVar() ;
    void Handle_Single_Nonaugment_Grid_CVar(EOS5CFGrid*) ;
    bool Handle_Single_Nonaugment_Grid_CVar_OwnLatLon(const EOS5CFGrid *, std::set<std::string>&) ;
    bool Handle_Single_Nonaugment_Grid_CVar_EOS5LatLon(const EOS5CFGrid *, std::set<std::string>&) ;
    void Handle_NonLatLon_Grid_CVar(EOS5CFGrid *, std::set<std::string>&) ;
    void Remove_MultiDim_LatLon_EOS5CFGrid() ;
    void Adjust_EOS5GridDimNames(const EOS5CFGrid *) const;

    void Handle_Swath_CVar(bool) ;
    void Handle_Single_1DLatLon_Swath_CVar(EOS5CFSwath *cfswath, bool is_augmented) ;
    void Handle_Single_2DLatLon_Swath_CVar(EOS5CFSwath *cfswath, bool is_augmented) ;
    void Handle_NonLatLon_Swath_CVar(EOS5CFSwath *cfswath, std::set<std::string>& tempvardimnamelist) ;
    void Handle_Special_NonLatLon_Swath_CVar(EOS5CFSwath *cfswath, const std::set<std::string>&tempvardimnamelist) ;

    void Handle_Za_CVar(bool) ;

    bool Check_Augmentation_Status() const;
    // Don't remove the following commented if 0 line!
#if 0
    //bool Check_Augmented_Var_Attrs(Var *var) throw(Exception);
#endif
    template<class T> bool Check_Augmented_Var_Candidate(T*, const Var*, EOS5Type) const;

    template<class T> void Adjust_Per_Var_Dim_NewName_Before_Flattening(T*, bool, int, int, int) const;
    void Adjust_SharedLatLon_Grid_Var_Dim_Name() const;

    void Adjust_Aura_Attr_Name() ;
    void Adjust_Aura_Attr_Value() const;
    void Handle_EOS5CVar_Unit_Attr() const;
    void Add_EOS5_Grid_CF_Attr() ;
    void Handle_Aura_Special_Attr() const;

    std::string get_CF_string(std::string s) override;
    void Replace_Var_Info_EOS(const EOS5CVar *src, EOS5CVar *target);
    void Replace_Var_Attrs_EOS(const EOS5CVar *src, EOS5CVar *target);
    void Handle_EOS5CVar_NameClashing(std::set<std::string> &) ;
    void Handle_EOS5CVar_AttrNameClashing() ;
    template<typename T> void EOS5Handle_General_NameClashing(std::set<std::string>&objnameset, std::vector<T*>& objvec)
        ;
    template<typename T> void Create_Missing_CV(T*, EOS5CVar*, const std::string &, EOS5Type, int) const;
    void Create_Added_Var_NewName_FullPath(EOS5Type, const std::string&, const std::string&, std::string &, std::string &) const;

    void Handle_EOS5_Unsupported_Dtype(bool) ;
    void Handle_EOS5_Unsupported_Dspace(bool) ;

    void Gen_Unsupported_Dtype_Info(bool) override;
    void Gen_VarAttr_Unsupported_Dtype_Info()  override;
    void Gen_EOS5_VarAttr_Unsupported_Dtype_Info() ;

    void Gen_Unsupported_Dspace_Info()  override;

private:
    std::vector<EOS5CVar *> cvars;
    std::vector<EOS5CFGrid *> eos5cfgrids;
    std::vector<EOS5CFSwath *> eos5cfswaths;
    std::vector<EOS5CFZa *> eos5cfzas;
    std::map<std::string, std::string> eos5_to_cf_attr_map;
    bool grids_multi_latloncvs = false;
    bool isaura = false;
    EOS5AuraName aura_name = NOTAURA;
    int orig_num_grids = 0;
    std::multimap<std::string, std::string> dimname_to_dupdimnamelist;
};

}
#endif
