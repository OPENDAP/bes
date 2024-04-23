/////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf4 data handler for the OPeNDAP data server.
// It retrieves the Vdata fields from NASA HDF4 data products.
// Each Vdata will be decomposed into individual Vdata fields.
// Each field will be mapped to A DAP variable.

//  Authors:   Kent Yang <myang6@hdfgroup.org>
// Copyright (c) The HDF Group
/////////////////////////////////////////////////////////////////////////////

#include "HDFDMRArray_VD.h"
#include "HDFStructure.h"
#include <iostream>
#include <sstream>
#include <libdap/debug.h>
#include <libdap/InternalErr.h>
#include <BESDebug.h>

using namespace std;
using namespace libdap;


bool
HDFDMRArray_VD::read ()
{

    BESDEBUG("h4","Coming to HDFDMRArray_VD read "<<endl);
    if (length() == 0)
        return true; 


    // Declaration of offset,count and step
    vector<int>offset;
    offset.resize(rank);
    vector<int>count;
    count.resize(rank);
    vector<int>step;
    step.resize(rank);

    // Obtain offset,step and count from the client expression constraint
    int nelms = format_constraint(offset.data(),step.data(),count.data());

    // Open the file
    int32 file_id = Hopen (filename.c_str (), DFACC_READ, 0);
    if (file_id < 0) {
        ostringstream eherr;
        eherr << "File " << filename.c_str () << " cannot be open.";
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }

    // Start the Vdata interface
    int32 vdata_id = 0;
    if (Vstart (file_id) < 0) {
        Hclose(file_id);
        ostringstream eherr;
        eherr << "This file cannot be open.";
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }

    // Attach the vdata
    vdata_id = VSattach (file_id, vdref, "r");
    if (vdata_id == -1) {
        Vend (file_id);
        Hclose(file_id);
        ostringstream eherr;
        eherr << "Vdata cannot be attached.";
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }

    int32 vs_nflds = VFnfields(vdata_id);
    if (vs_nflds == FAIL) {
        VSdetach(vdata_id);
        Vend (file_id);
        Hclose(file_id);
        throw InternalErr(__FILE__, __LINE__, "Cannot get the number of fields of a vdata.");
    }

    try {
        if (vs_nflds == 1)
            read_one_field_vdata(vdata_id, offset,count,step,nelms);
        else
            read_multi_fields_vdata(vdata_id,offset, count,step,nelms);
        VSdetach(vdata_id);
        Vend(file_id);
        Hclose(file_id);
  
    }
    catch(...) {
        VSdetach(vdata_id);
        Vend(file_id);
        Hclose(file_id);
        throw;
    }

}

void
HDFDMRArray_VD::read_one_field_vdata(int32 vdata_id,const vector<int>&offset, const vector<int>&count, const vector<int>&step, int nelms) {

    int32 fdorder = VFfieldorder(vdata_id,0);
    if (fdorder == FAIL) {
        throw InternalErr(__FILE__, __LINE__, "VFfieldorder failed");
    }

    const char *fieldname = VFfieldname(vdata_id,0);
    if (fieldname == nullptr) {
        throw InternalErr(__FILE__, __LINE__, "Cannot get vdata field name.");
    }

    int32 fdtype = VFfieldtype(vdata_id,0);
    if (fdtype == FAIL) {
        throw InternalErr(__FILE__, __LINE__, "VFfieldtype failed");
    }

    // Seek the position of the starting point
    if (VSseek (vdata_id, (int32) offset[0]) == -1) {
        ostringstream eherr;
        eherr << "VSseek failed at " << offset[0];
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }


    // Prepare the vdata field
    if (VSsetfields (vdata_id, fieldname) == -1) {
        ostringstream eherr;
        eherr << "VSsetfields failed with the name " << fieldname;
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }

    int32 vdfelms = fdorder * count[0] * step[0];
    // SSTTOOPP Add more information.
}
void
HDFDMRArray_VD::read_multi_fields_vdata(int32 vdata_id,const vector<int>&offset, const vector<int>&count, const vector<int>&step, int nelms) {

    // Read the subset's vdata values
    int32 n_records = 0;
    int32 vdata_size = 0;
    if (VSQueryvsize(vdata_id,&vdata_size)==FAIL) 
        throw InternalErr(__FILE__,__LINE__,"unable to query vdata size");

    if (VSQuerycount(vdata_id,&n_records) == FAIL) 
        throw InternalErr(__FILE__,__LINE__,"unable to query number of records of vdata");

    vector<uint8_t> data_buf;
    data_buf.resize(n_records * vdata_size);
    if (VSread (vdata_id, data_buf.data(),n_records,FULL_INTERLACE)<0)  
        throw InternalErr(__FILE__,__LINE__,"unable to read vdata");

    vector<uint8_t> subset_buf;
    subset_buf.resize(nelms*vdata_size);
    if (rank !=1) 
        throw InternalErr(__FILE__,__LINE__,"vdata must be 1-D array of structure");

    uint8_t *tmp_buf = data_buf.data() + offset[0]*vdata_size;
    uint8_t *tmp_subset_buf = subset_buf.data();
    if (step[0] == 1)
        memcpy((void*)tmp_subset_buf,(const void*)tmp_buf,count[0]*vdata_size);
    else {
        for (int i = 0; i<count[0]; i++) {
            memcpy((void*)tmp_subset_buf,(const void*)tmp_buf,vdata_size);
            tmp_subset_buf +=vdata_size;
            tmp_buf +=step[0]*vdata_size;
        }
    }
      
    size_t values_offset = 0;
    // Write the values to the DAP4
    for (int64_t element = 0; element < nelms; ++element) {
    
        auto vdata_s = dynamic_cast<HDFStructure*>(var()->ptr_duplicate());
        if(!vdata_s)
            throw InternalErr(__FILE__, __LINE__, "Cannot obtain the structure pointer.");
        try {
            vdata_s->read_from_value(subset_buf,values_offset);
        }
        catch(...) {
            delete vdata_s;
            string err_msg = "Cannot read the data of a vdata  " + var()->name();
            throw InternalErr(__FILE__, __LINE__, err_msg);
        }
        vdata_s->set_read_p(true);
        set_vec_ll((uint64_t)element,vdata_s);
        delete vdata_s;
    }
    
    set_read_p(true);

}

// Standard way of DAP handlers to pass the coordinates of the subsetted region to the handlers
// Return the number of elements to read. 
int
HDFDMRArray_VD::format_constraint (int *offset, int *step, int *count)
{
    int nels = 1;
    int id = 0;

    Dim_iter p = dim_begin ();
    while (p != dim_end ()) {

        int start = dimension_start (p, true);
        int stride = dimension_stride (p, true);
        int stop = dimension_stop (p, true);

        // Check for illegal  constraint
        if (start > stop) {
            ostringstream oss;
            oss << "Array/Grid hyperslab start point "<< start <<
                   " is greater than stop point " <<  stop <<".";
            throw Error(malformed_expr, oss.str());
        }

        offset[id] = start;
        step[id] = stride;
        count[id] = ((stop - start) / stride) + 1;      // count of elements
        nels *= count[id];              // total number of values for variable

        BESDEBUG ("h4",
                         "=format_constraint():"
                         << "id=" << id << " offset=" << offset[id]
                         << " step=" << step[id]
                         << " count=" << count[id]
                         << endl);

        id++;
        p++;
    }

    return nels;
}


