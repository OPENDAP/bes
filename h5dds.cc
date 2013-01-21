// This file is part of hdf5_handler a HDF5 file handler for the OPeNDAP
// data server.

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
/// \author Muqun Yang    <myang6@hdfgroup.org>
///

#include "config_hdf5.h"

#include <InternalErr.h>
#include <debug.h>
#include <mime_util.h>

#include "h5dds.h"
#include "HDF5Int32.h"
#include "HDF5UInt32.h"
#include "HDF5UInt16.h"
#include "HDF5Int16.h"
#include "HDF5Byte.h"
#include "HDF5Array.h"
#include "HDF5Str.h"
#include "HDF5Float32.h"
#include "HDF5Float64.h"
#include "HDF5Url.h"
#include "HDF5Structure.h"
#include "h5get.h"


/// An instance of DS_t structure defined in hdf5_handler.h.
static DS_t dt_inst; 

extern string get_hardlink(hid_t, const string &);
///////////////////////////////////////////////////////////////////////////////
/// \fn depth_first(hid_t pid, char *gname, DDS & dds, const char *fname)
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
bool depth_first(hid_t pid, char *gname, DDS & dds, const char *fname)
{
    DBG(cerr
        << ">depth_first()" 
        << " pid: " << pid
        << " gname: " << gname
        << " fname: " << fname
        << endl);
// cerr<<"coming to the depth_first "<<endl;
    
    // Iterate through the file to see the members of the group from the root.
    H5G_info_t g_info; 
    hsize_t nelems = 0;
    if(H5Gget_info(pid,&g_info) <0) {
      string msg =
            "h5_dds handler: counting hdf5 group elements error for ";
        msg += gname;
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    nelems = g_info.nlinks;
// cerr <<"nelems = " << nelems <<endl;
        
    ssize_t oname_size;
    for (hsize_t i = 0; i < nelems; i++) {

        char *oname = NULL;
//        vector <char>oname;

        try {

            // Query the length of object name.
            oname_size =
 		H5Lget_name_by_idx(pid,".",H5_INDEX_NAME,H5_ITER_NATIVE,i,NULL,
                (size_t)DODS_NAMELEN, H5P_DEFAULT);
            if (oname_size <= 0) {
                string msg = "h5_dds handler: Error getting the size of the hdf5 object from the group: ";
                msg += gname;
                throw InternalErr(__FILE__, __LINE__, msg);
            }

            // Obtain the name of the object
            // TODO vector<char>
            oname = new char[(size_t) oname_size + 1];

            if (H5Lget_name_by_idx(pid,".",H5_INDEX_NAME,H5_ITER_NATIVE,i,oname,
                (size_t)(oname_size+1), H5P_DEFAULT) < 0){
                string msg =
                    "h5_dds handler: Error getting the hdf5 object name from the group: ";
                msg += gname;
                throw InternalErr(__FILE__, __LINE__, msg);
            }

            // Check if it is the hard link or the soft link
            H5L_info_t linfo;
            if (H5Lget_info(pid,oname,&linfo,H5P_DEFAULT)<0) {
                string msg = "hdf5 link name error from: ";
                msg += gname;
                throw InternalErr(__FILE__, __LINE__, msg);
            }
            
            // We ignore soft link and hard link in this release 
            if(linfo.type == H5L_TYPE_SOFT || linfo.type == H5L_TYPE_EXTERNAL) 
              continue;

            // Obtain the object type, such as group or dataset. 
            H5O_info_t oinfo;

            if (H5Oget_info_by_idx(pid, ".", H5_INDEX_NAME, H5_ITER_NATIVE,
                              i, &oinfo, H5P_DEFAULT)<0) {
                string msg = "h5_dds handler: Error obtaining the info for the object";
                msg += oname;
                throw InternalErr(__FILE__, __LINE__, msg);
            }

            H5O_type_t obj_type = oinfo.type;

            switch (obj_type) {  

            case H5O_TYPE_GROUP: {

                // Obtain the full path name
                string full_path_name =
                    string(gname) + string(oname) + "/";

                DBG(cerr << "=depth_first():H5G_GROUP " << full_path_name
                    << endl);

                // Get the C char* of the object name
                // FIXME t_fpn leaked

                vector <char>t_fpn;
                t_fpn.resize(full_path_name.length()+1);
                copy(full_path_name.begin(),full_path_name.end(),t_fpn.begin());

//                char *t_fpn = new char[full_path_name.length() + 1];
 //               (void)full_path_name.copy(t_fpn, full_path_name.length());
                t_fpn[full_path_name.length()] = '\0';

                hid_t cgroup = H5Gopen(pid, &t_fpn[0],H5P_DEFAULT);
                if (cgroup < 0){
		   throw InternalErr(__FILE__, __LINE__, "h5_dds handler: H5Gopen() failed.");
		}

                // Check the hard link loop and break the loop if it exists.
                // Note the function get_hardlink is defined in h5das.cc
		string oid = get_hardlink(pid, oname);
                if (oid == "") {
                    depth_first(cgroup, &t_fpn[0], dds, fname);
                }

                if (H5Gclose(cgroup) < 0){
		   throw InternalErr(__FILE__, __LINE__, "Could not close the group.");
		}
  //              if (t_fpn) {delete[]t_fpn; t_fpn = NULL;}                
                break;
            }

            case H5O_TYPE_DATASET:{

                // Obtain the absolute path of the HDF5 dataset
                string full_path_name = string(gname) + string(oname);
//cerr<<"dataset full_path "<<full_path_name <<endl;

                // Obtain the hdf5 dataset handle stored in the structure dt_inst. 
                // All the metadata information in the handler is stored in dt_inst.
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
        } // try
        catch(...) {

            if (oname) {delete[]oname; oname = NULL;}
            throw;
            
        } // catch
        if (oname) {delete[]oname; oname = NULL;}
    } // for i is 0 ... nelems

    DBG(cerr << "<depth_first() " << endl);
    return true;
}


///////////////////////////////////////////////////////////////////////////////
/// \fn Get_bt(const string &varname,  const string  &dataset, hid_t datatype)
/// returns the pointer to the base type
///
/// This function will create a new DODS object that corresponds to HDF5
/// dataset and return the pointer of a new object of DODS datatype. If an
/// error is found, an exception of type InternalErr is thrown. 
///
/// \param varname object name
/// \param dataset name of dataset where this object comes from
/// \param datatype datatype id
/// \return pointer to BaseType
///////////////////////////////////////////////////////////////////////////////
static BaseType *Get_bt(const string &vname,
                        const string &dataset,
                        hid_t datatype)
{
    BaseType *btp = NULL;

    try {

        DBG(cerr << ">Get_bt varname=" << vname << " datatype=" << datatype
            << endl);

        size_t size = 0;
        int sign = -2;
        switch (H5Tget_class(datatype)) {

        case H5T_INTEGER:
            size = H5Tget_size(datatype);
            sign = H5Tget_sign(datatype);
            DBG(cerr << "=Get_bt() H5T_INTEGER size = " << size << " sign = "
                << sign << endl);

	    if (sign == H5T_SGN_ERROR) {
                throw InternalErr(__FILE__, __LINE__, "cannot retrieve the sign type of the integer");
            }
            if (size == 0) {
		throw InternalErr(__FILE__, __LINE__, "cannot return the size of the datatype");
	    }
	    else if (size == 1) { // Either signed char or unsigned char
                // DAP2 doesn't support signed char, it maps to DAP int16.
                if (sign == H5T_SGN_2) // signed char to DAP int16
                    btp = new HDF5Int16(vname, dataset);
                else
                    btp = new HDF5Byte(vname, dataset);
            }
            else if (size == 2) {
                if (sign == H5T_SGN_2)
                    btp = new HDF5Int16(vname, dataset);
                else
                    btp = new HDF5UInt16(vname, dataset);
            }
            else if (size == 4) {
                if (sign == H5T_SGN_2)
                    btp = new HDF5Int32(vname, dataset);
                else
                    btp = new HDF5UInt32(vname, dataset);
            }
            else if (size == 8) {
                throw
                    InternalErr(__FILE__, __LINE__,
                                string("Unsupported HDF5 64-bit Integer type:")
                                + vname);
            }
            break;

        case H5T_FLOAT:
            size = H5Tget_size(datatype);
            DBG(cerr << "=Get_bt() H5T_FLOAT size = " << size << endl);

	    if (size == 0) {
                throw InternalErr(__FILE__, __LINE__, "cannot return the size of the datatype");
            }
            else if (size == 4) {
                btp = new HDF5Float32(vname, dataset);
            }
            else if (size == 8) {
                btp = new HDF5Float64(vname, dataset);
            }
            break;

        case H5T_STRING:
            btp = new HDF5Str(vname, dataset);
            break;

        // The array datatype is rarely,rarely used. So this
        // part of code is not reviewed.
        case H5T_ARRAY: {
            BaseType *ar_bt = 0;
            try {
                DBG(cerr <<
                    "=Get_bt() H5T_ARRAY datatype = " << datatype
                    << endl);

                // Get the base datatype of the array
                hid_t dtype_base = H5Tget_super(datatype);
                ar_bt = Get_bt(vname, dataset, dtype_base);
                btp = new HDF5Array(vname, dataset, ar_bt);
                delete ar_bt; ar_bt = 0;

                // Set the size of the array.
                int ndim = H5Tget_array_ndims(datatype);
                size = H5Tget_size(datatype);
                int nelement = 1;

		if (dtype_base < 0) {
                throw InternalErr(__FILE__, __LINE__, "cannot return the base datatype");
 	        }
		if (ndim < 0) {
                throw InternalErr(__FILE__, __LINE__, "cannot return the rank of the array datatype");
                }
		if (size == 0) {
                throw InternalErr(__FILE__, __LINE__, "cannot return the size of the datatype");
                }
                DBG(cerr
                    << "=Get_bt()" << " Dim = " << ndim
                    << " Size = " << size
                    << endl);

                hsize_t size2[DODS_MAX_RANK];
                if(H5Tget_array_dims(datatype, size2) < 0){
                    throw
                        InternalErr(__FILE__, __LINE__,
                                    string("Could not get array dims for: ")
                                      + vname);
                }


                HDF5Array &h5_ar = dynamic_cast < HDF5Array & >(*btp);
                for (int dim_index = 0; dim_index < ndim; dim_index++) {
                    h5_ar.append_dim(size2[dim_index]);
                    DBG(cerr << "=Get_bt() " << size2[dim_index] << endl);
                    nelement = nelement * size2[dim_index];
                }

                h5_ar.set_did(dt_inst.dset);
                // Assign the array datatype id.
                h5_ar.set_tid(datatype);
                h5_ar.set_memneed(size);
                h5_ar.set_numdim(ndim);
                h5_ar.set_numelm(nelement);
                h5_ar.set_length(nelement);
                h5_ar.d_type = H5Tget_class(dtype_base); 
		if (h5_ar.d_type == H5T_NO_CLASS){
		    throw InternalErr(__FILE__, __LINE__, "cannot return the datatype class identifier");
		}
            }
            catch (...) {
                if( ar_bt ) delete ar_bt;
                if( btp ) delete btp;
                throw;
            }
            break;
        }

        // Reference map to DAP URL, check the technical note.
        case H5T_REFERENCE:
            btp = new HDF5Url(vname, dataset);
            break;
        
        default:
            throw InternalErr(__FILE__, __LINE__,
                              string("Unsupported HDF5 type:  ") + vname);
        }
    }
    catch (...) {
        if( btp ) delete btp;
        throw;
    }

    if (!btp)
        throw InternalErr(__FILE__, __LINE__,
                          string("Could not make a DAP variable for: ")
                          + vname);
                                                  
    // Dynamic cast the HDF5 datatype to DAP2 type, also save the HDF5 datatype
    switch (btp->type()) {

    case dods_byte_c: {
    	// TODO In this/these case(s) you know the type is a dods_byte so you can
    	// safely use static_cast<>() instead of the more expensive dynamic_cast
    	// operator. Not a huge deal, but static_cast is faster.
        HDF5Byte &v = dynamic_cast < HDF5Byte & >(*btp);
        v.set_did(dt_inst.dset);
        v.set_tid(dt_inst.type);
        break;
    }
        
    case dods_int16_c: {
        HDF5Int16 &v = dynamic_cast < HDF5Int16 & >(*btp);
        v.set_did(dt_inst.dset);
        v.set_tid(dt_inst.type);
        break;
    }
    case dods_uint16_c: {
        HDF5UInt16 &v = dynamic_cast < HDF5UInt16 & >(*btp);
        v.set_did(dt_inst.dset);
        v.set_tid(dt_inst.type);
        break;
    }
    case dods_int32_c: {
        HDF5Int32 &v = dynamic_cast < HDF5Int32 & >(*btp);
        v.set_did(dt_inst.dset);
        v.set_tid(dt_inst.type);
        break;
    }
    case dods_uint32_c: {
        HDF5UInt32 &v = dynamic_cast < HDF5UInt32 & >(*btp);
        v.set_did(dt_inst.dset);
        v.set_tid(dt_inst.type);
        break;
    }
    case dods_float32_c: {
        HDF5Float32 &v = dynamic_cast < HDF5Float32 & >(*btp);
        v.set_did(dt_inst.dset);
        v.set_tid(dt_inst.type);
        break;
    }
    case dods_float64_c: {
        HDF5Float64 &v = dynamic_cast < HDF5Float64 & >(*btp);
        v.set_did(dt_inst.dset);
        v.set_tid(dt_inst.type);
        break;
    }
    case dods_str_c: {
        HDF5Str &v = dynamic_cast < HDF5Str & >(*btp);
        v.set_did(dt_inst.dset);
        v.set_tid(dt_inst.type);
        break;
    }
    case dods_array_c:
        break;
        
    case dods_url_c: {
        HDF5Url &v = dynamic_cast < HDF5Url & >(*btp);
        v.set_did(dt_inst.dset);
        v.set_tid(dt_inst.type);
        break;
    }
    default:
        delete btp;
        throw InternalErr(__FILE__, __LINE__,
                          string("error counting hdf5 group elements for ") 
                          + vname);
    }
    DBG(cerr << "<Get_bt()" << endl);
    return btp;
}


///////////////////////////////////////////////////////////////////////////////
/// \fn Get_structure(const string& varname, const string &dataset,
///     hid_t datatype)
/// returns a pointer of structure type. An exception is thrown if an error
/// is encountered.
/// 
/// This function will create a new DODS object that corresponds to HDF5
/// compound dataset and return a pointer of a new structure object of DODS.
///
/// \param varname object name
/// \param dataset name of the dataset from which this structure created
/// \param datatype datatype id
/// \return pointer to Structure type
///
///////////////////////////////////////////////////////////////////////////////
static Structure *Get_structure(const string &varname,
                                const string &dataset,
                                hid_t datatype)
{
    HDF5Structure *structure_ptr = NULL;

    DBG(cerr << ">Get_structure()" << datatype << endl);

    if (H5Tget_class(datatype) != H5T_COMPOUND)
        throw InternalErr(__FILE__, __LINE__,
                          string("Compound-to-structure mapping error for ")
                          + varname);

    try {
        structure_ptr = new HDF5Structure(varname, dataset);
        structure_ptr->set_did(dt_inst.dset);
        structure_ptr->set_tid(dt_inst.type);

        // Retrieve member types
        int nmembs = H5Tget_nmembers(datatype);
        DBG(cerr << "=Get_structure() has " << nmembs << endl);
	if (nmembs < 0){
	   throw InternalErr(__FILE__, __LINE__, "cannot retrieve the number of elements");
	}
        for (int i = 0; i < nmembs; i++) {
            char *memb_name = H5Tget_member_name(datatype, i);
            H5T_class_t memb_cls = H5Tget_member_class(datatype, i);
            hid_t memb_type = H5Tget_member_type(datatype, i);
	    if (memb_name == NULL){
		throw InternalErr(__FILE__, __LINE__, "cannot retrieve the name of the member");
	    }
            if (memb_cls < 0 | memb_type < 0) {
                // structure_ptr is deleted in the catch ... block
                // below. So if this exception is thrown, it will
                // get caught below and the ptr deleted.
                // pwest Mar 18, 2009
                //delete structure_ptr;
                throw InternalErr(__FILE__, __LINE__,
                                  string("Type mapping error for ")
                                  + string(memb_name) );
            }
            
            // ~Structure() will delete these if they are added.
            if (memb_cls == H5T_COMPOUND) {
                Structure *s = Get_structure(memb_name, dataset, memb_type);
                structure_ptr->add_var(s);
                delete s; s = 0;
            } 
            else {
                BaseType *bt = Get_bt(memb_name, dataset, memb_type);
                structure_ptr->add_var(bt);
                delete bt; bt = 0;
            }
        }
    }
    catch (...) {
        // if memory allocation exception thrown it will be caught
        // here, so should check if structure ptr exists before
        // deleting it. pwest Mar 18, 2009
        if( structure_ptr ) delete structure_ptr;
        throw;
    }

    DBG(cerr << "<Get_structure()" << endl);

    return structure_ptr;
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

    //declare an array to store HDF5 dimensions
    hid_t *dimids = NULL;
    try {
        dimids = new hid_t[dt_inst.ndims];
    }
    catch (...) {
        if(dimids) {
            delete [] dimids;
            dimids=0;
        }
        throw;
    }

    // Get a base type. It should be int, float, double, etc. -- atomic
    // datatype. 
    BaseType *bt = Get_bt(varname, filename, dt_inst.type);
    
    if (!bt) {
        // NB: We're throwing InternalErr even though it's possible that
        // someone might ask for an HDF5 varaible which this server cannot
        // handle.
        throw
            InternalErr(__FILE__, __LINE__,
                        "Unable to convert hdf5 datatype to dods basetype");
    }

    // First deal with scalar data. 
    if (dt_inst.ndims == 0) {
        dds_table.add_var(bt);
        delete bt; bt = 0;
    }
    else {
        // Next, deal with Array and Grid data. This 'else clause' runs to
        // the end of the method. jhrg

        HDF5Array *ar = new HDF5Array(varname, filename, bt);
        delete bt; bt = 0;
        ar->set_did(dt_inst.dset);
        ar->set_tid(dt_inst.type);
        ar->set_memneed(dt_inst.need);
        ar->set_numdim(dt_inst.ndims);
        ar->set_numelm((int) (dt_inst.nelmts));
	for (int dim_index = 0; dim_index < dt_inst.ndims; dim_index++)
            ar->append_dim(dt_inst.size[dim_index]); 
        dds_table.add_var(ar);
        delete ar; ar = 0;
    }

    DBG(cerr << "<read_objects_base_type(dds)" << endl);
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

    Structure *structure = Get_structure(varname, filename, dt_inst.type);
    try {
        // Assume Get_structure() uses exceptions to signal an error. jhrg
        DBG(cerr << "=read_objects_structure(): Dimension is " 
            << dt_inst.ndims << endl);

        if (dt_inst.ndims != 0) {   // Array of Structure
            int dim_index;
            DBG(cerr << "=read_objects_structure(): array of size " <<
                dt_inst.nelmts << endl);
            DBG(cerr << "=read_objects_structure(): memory needed = " <<
                dt_inst.need << endl);
            HDF5Array *ar = new HDF5Array(varname, filename, structure);
            delete structure; structure = 0;
            try {
                ar->set_did(dt_inst.dset);
                ar->set_tid(dt_inst.type);
                ar->set_memneed(dt_inst.need);
                ar->set_numdim(dt_inst.ndims);
                ar->set_numelm((int) (dt_inst.nelmts));
                ar->set_length((int) (dt_inst.nelmts));

                for (dim_index = 0; dim_index < dt_inst.ndims; dim_index++) {
                    ar->append_dim(dt_inst.size[dim_index]);
                    DBG(cerr << "=read_objects_structure(): append_dim = " <<
                        dt_inst.size[dim_index] << endl);
                }

                dds_table.add_var(ar);
                delete ar; ar = 0;
            } // try Array *ar
            catch (...) {
                delete ar;
                throw;
            }
        } 
        else {// A scalar structure

            dds_table.add_var(structure);
            delete structure; structure = 0;
        }

    } // try     Structure *structure = Get_structure(...)
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

    default:
        read_objects_base_type(dds_table, varname, filename);
        break;
    }
}

