// This file is part of hdf5_handler a HDF5 file handler for the OPeNDAP
// data server.

// Copyright (c) 2007-2015 The HDF Group, Inc. and OPeNDAP, Inc.
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

///////////////////////////////////////////////////////////////////////////////
/// \file h5dds.cc
/// \brief DDS/DODS request processing source
///
/// This file is part of h5_dap_handler, a C++ implementation of the DAP
/// handler for HDF5 data.
///
/// This file contains functions which use depth-first search to walk through
/// an HDF5 file and build the in-memory DDS.
///
/// \author Hyo-Kyung Lee <hyoklee@hdfgroup.org>
/// \author Kent Yang    <myang6@hdfgroup.org>
///

#include<memory>
#include <libdap/InternalErr.h>
#include <libdap/mime_util.h>

#include <BESDebug.h>
#include <BESInternalError.h>

#include "hdf5_handler.h"
#include "HDF5Int32.h"
#include "HDF5UInt32.h"
#include "HDF5UInt16.h"
#include "HDF5Int16.h"
#include "HDF5Byte.h"
#include "HDF5Array.h"
#include "HDF5Float32.h"
#include "HDF5Float64.h"
#include "HDF5Url.h"
#include "HDF5Structure.h"

#include "HDF5CFUtil.h"


using namespace std;
using namespace libdap;

/// An instance of DS_t structure defined in hdf5_handler.h.
static DS_t dt_inst; 

///////////////////////////////////////////////////////////////////////////////
/// \fn depth_first(hid_t pid, const char *gname, DDS & dds, const char *fname)
/// will fill DDS table.
///
/// This function will walk through hdf5 \a gname group
/// using the depth-first approach to obtain data information
/// (data type and data pattern) of all hdf5 datasets and then
/// put them into ithe dds table.
///
/// \param pid group id
/// \param gname group name (the absolute path from the root group)
/// \param dds reference of DDS object
/// \param fname the HDF5 file name
///
/// \return 0, if failed.
/// \return 1, if succeeded.
///
/// \remarks hard link is treated as a dataset.
/// \remarks will return error message to the DAP interface.
/// \see depth_first(hid_t pid, char *gname, DAS & das, const char *fname) in h5das.cc
///////////////////////////////////////////////////////////////////////////////
bool depth_first(hid_t pid,const  char *gname, DDS & dds, const char *fname)
{
    BESDEBUG("h5",
        ">depth_first()" 
        << " pid: " << pid
        << " gname: " << gname
        << " fname: " << fname
        << endl);
    
    // Iterate through the file to see the members of the group from the root.
    H5G_info_t g_info; 
    hsize_t nelems = 0;
    if(H5Gget_info(pid,&g_info) <0) {
      string msg =
            "h5_dds handler: counting hdf5 group elements error for ";
        msg += gname;
        throw BESInternalError(msg,__FILE__, __LINE__);
    }

    nelems = g_info.nlinks;
        
    ssize_t oname_size;
    for (hsize_t i = 0; i < nelems; i++) {

        vector <char>oname;

        // Query the length of object name.
        oname_size =
 	    H5Lget_name_by_idx(pid,".",H5_INDEX_NAME,H5_ITER_NATIVE,i,nullptr,
                (size_t)DODS_NAMELEN, H5P_DEFAULT);
        if (oname_size <= 0) {
            string msg = "h5_dds handler: Error getting the size of the hdf5 object from the group: ";
            msg += gname;
            throw BESInternalError(msg,__FILE__, __LINE__);
        }

        // Obtain the name of the object
        oname.resize((size_t) oname_size + 1);

        if (H5Lget_name_by_idx(pid,".",H5_INDEX_NAME,H5_ITER_NATIVE,i,oname.data(),
            (size_t)(oname_size+1), H5P_DEFAULT) < 0){
            string msg =
                    "h5_dds handler: Error getting the hdf5 object name from the group: ";
             msg += gname;
                throw BESInternalError(msg,__FILE__, __LINE__);
        }

        // Check if it is the hard link or the soft link
        H5L_info_t linfo;
        if (H5Lget_info(pid,oname.data(),&linfo,H5P_DEFAULT)<0) {
            string msg = "hdf5 link name error from: ";
            msg += gname;
            throw BESInternalError(msg,__FILE__, __LINE__);
        }

        // External links are not supported in this release
        if(linfo.type == H5L_TYPE_EXTERNAL)
            continue;

        // Remember the information of soft links in DAS, not in DDS
        if(linfo.type == H5L_TYPE_SOFT)
            continue;

        // Obtain the object type, such as group or dataset. 
        H5O_info_t oinfo;

        if (H5OGET_INFO_BY_IDX(pid, ".", H5_INDEX_NAME, H5_ITER_NATIVE,
                              i, &oinfo, H5P_DEFAULT)<0) {
            string msg = "h5_dds handler: Error obtaining the info for the object";
            msg += string(oname.begin(),oname.end());
            throw BESInternalError(msg,__FILE__, __LINE__);
        }

        H5O_type_t obj_type = oinfo.type;
        switch (obj_type) {  

            case H5O_TYPE_GROUP: 
            {

                // Obtain the full path name
                string full_path_name =
                    string(gname) + string(oname.begin(),oname.end()-1) + "/";

                BESDEBUG("h5", "=depth_first():H5G_GROUP " << full_path_name
                    << endl);

                vector <char>t_fpn;
                t_fpn.resize(full_path_name.size()+1);
                copy(full_path_name.begin(),full_path_name.end(),t_fpn.begin());

                t_fpn[full_path_name.size()] = '\0';

                hid_t cgroup = H5Gopen(pid, t_fpn.data(),H5P_DEFAULT);
                if (cgroup < 0){
                    string msg = "H5Gopen failed for the group name " + full_path_name + ".";
                    throw BESInternalError(msg,__FILE__,__LINE__);
                }

                // Check the hard link loop and break the loop if it exists.
                // Note the function get_hardlink is defined in h5das.cc
                string oid = get_hardlink(pid, oname.data());
                if (oid == "") {
                    try {
                        depth_first(cgroup, t_fpn.data(), dds, fname);
                    }
                    catch(...) {
                        H5Gclose(cgroup);
                        throw;
                    }
                }

                if (H5Gclose(cgroup) < 0){
                    string msg = "Could not close the group.";
                    throw BESInternalError(msg,__FILE__,__LINE__);
                }
                break;
            }

            case H5O_TYPE_DATASET:
            {

                // Obtain the absolute path of the HDF5 dataset
                string full_path_name = string(gname) + string(oname.begin(),oname.end()-1);

                // Obtain the hdf5 dataset handle stored in the structure dt_inst. 
                // All the metadata information in the handler is stored in dt_inst.
                // Don't consider the dim. scale support for DAP2 now.
#if 0
                //get_dataset(pid, full_path_name, &dt_inst,false);
#endif
                get_dataset(pid, full_path_name, &dt_inst);

                // Put the hdf5 dataset structure into DODS dds.
                read_objects(dds, full_path_name, fname);
                break;
            }

            case H5O_TYPE_NAMED_DATATYPE:
                // ignore the named datatype
                break;
            default:
                break;
            }
            
    } // for i is 0 ... nelems

    BESDEBUG("h5", "<depth_first() " << endl);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \fn read_objects_base_type(DDS & dds_table,
///                            const string & varname,
///                            const string & filename)
/// fills in information of a dataset (name, data type, data space) into one
/// DDS table.
/// 
/// Given a reference to an instance of class DDS, a filename that refers
/// to an hdf5 file and an HDF5 dataset name, read hdf5 file and extract all 
/// the dimensions of the HDF5 dataset.
/// Add the HDF5 dataset that maps to the DAP variable  and its dimensions 
/// to the instance of DDS.
///
/// It will use dynamic cast to put necessary information into subclass of dods
/// datatype. 
///
///    \param dds_table Destination for the HDF5 objects. 
///    \param varname Absolute name of an HDF5 dataset. 
///    \param filename The HDF5 dataset name that maps to the DDS dataset name.
///    \throw error a string of error message to the dods interface.
///////////////////////////////////////////////////////////////////////////////
void
read_objects_base_type(DDS & dds_table, const string & varname,
                       const string & filename)
{
    // Obtain the DDS dataset name.
    dds_table.set_dataset_name(name_path(filename)); 

    // Get a base type. It should be atomic datatype
    // DDS: varname is the absolute path
    BaseType *bt = Get_bt(varname, varname,filename, dt_inst.type,false);
    
    if (!bt) {
        // NB: We're throwing InternalErr even though it's possible that
        // someone might ask for an HDF5 varaible which this server cannot
        // handle.
        string msg = "Unable to convert hdf5 datatype to dods basetype ";
        msg += "for variable "+varname + ".";
        throw
            BESInternalError(msg,__FILE__,__LINE__);
    }

    // First deal with scalar data. 
    if (dt_inst.ndims == 0) {
        dds_table.add_var(bt);
        delete bt;
    }
    else {

        // Next, deal with Array data. This 'else clause' runs to
        // the end of the method. jhrg
        auto ar_unique = make_unique<HDF5Array>(varname, filename, bt);
        auto ar = ar_unique.get();
        delete bt;
        ar->set_memneed(dt_inst.need);
        ar->set_numdim(dt_inst.ndims);
        ar->set_numelm((int) (dt_inst.nelmts));
	    for (int dim_index = 0; dim_index < dt_inst.ndims; dim_index++)
            ar->append_dim_ll((int64_t)(dt_inst.size[dim_index])); 
        dds_table.add_var(ar);
    }
    BESDEBUG("h5", "<read_objects_base_type(dds)" << endl);
}

///////////////////////////////////////////////////////////////////////////////
/// \fn read_objects_structure(DDS & dds_table,const string & varname,
///                  const string & filename)
/// fills in information of a structure dataset (name, data type, data space)
/// into a DDS table. HDF5 compound datatype will map to DAP structure.
/// 
///    \param dds_table Destination for the HDF5 objects. 
///    \param varname Absolute name of structure
///    \param filename The HDF5 file  name that maps to the DDS dataset name.
///    \throw error a string of error message to the dods interface.
///////////////////////////////////////////////////////////////////////////////
void
read_objects_structure(DDS & dds_table, const string & varname,
                       const string & filename)
{
    dds_table.set_dataset_name(name_path(filename));

    Structure *structure = Get_structure(varname, varname,filename, dt_inst.type,false);

    try {
        // Assume Get_structure() uses exceptions to signal an error. jhrg
        BESDEBUG("h5", "=read_objects_structure(): Dimension is " 
            << dt_inst.ndims << endl);

        if (dt_inst.ndims != 0) {   // Array of Structure
            BESDEBUG("h5", "=read_objects_structure(): array of size " <<
                dt_inst.nelmts << endl);
            BESDEBUG("h5", "=read_objects_structure(): memory needed = " <<
                dt_inst.need << endl);
            auto ar_unique = make_unique<HDF5Array>(varname, filename, structure);
            auto ar = ar_unique.get();
            delete structure; structure = nullptr;

            ar->set_memneed(dt_inst.need);
            ar->set_numdim(dt_inst.ndims);
            ar->set_numelm((int) (dt_inst.nelmts));
            ar->set_length((int) (dt_inst.nelmts));

            for (int dim_index = 0; dim_index < dt_inst.ndims; dim_index++) {
                ar->append_dim_ll((int64_t)(dt_inst.size[dim_index]));
                BESDEBUG("h5", "=read_objects_structure(): append_dim = " <<
                    dt_inst.size[dim_index] << endl);
            }

            dds_table.add_var(ar);

        } 
        else {// A scalar structure

            dds_table.add_var(structure);
            delete structure; structure = nullptr;
        }

    } // try     Structure *structure is  Get_structure(...)
    catch (...) {
        delete structure;
        throw;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \fn read_objects(DDS & dds_table,const string & varname,
///                  const string & filename)
/// fills in information of a dataset (name, data type, data space) into one
/// DDS table.
/// 
///    \param dds_table Destination for the HDF5 objects. 
///    \param varname Absolute name of an HDF5 dataset. 
///    \param filename The HDF5 file  name that maps to the DDS dataset name.
///    \throw error a string of error message to the dods interface.
///////////////////////////////////////////////////////////////////////////////
void
read_objects(DDS & dds_table, const string &varname, const string &filename)
{

    switch (H5Tget_class(dt_inst.type)) {

    // HDF5 compound maps to DAP structure.
    case H5T_COMPOUND:
        read_objects_structure(dds_table, varname, filename);
        break;

    case H5T_ARRAY:
    {
        H5Tclose(dt_inst.type);
        string msg = "Currently don't support accessing data of Array datatype when array datatype is not inside the compound.";
        throw BESInternalError(msg,__FILE__,__LINE__);       
    }
    default:
        read_objects_base_type(dds_table, varname, filename);
        break;
    }
    // We must close the datatype obtained in the get_dataset routine since this is the end of reading DDS.
    if(H5Tclose(dt_inst.type)<0) {
        string msg = "Cannot close the HDF5 datatype.";
        throw BESInternalError(msg,__FILE__,__LINE__);       
    }
}

