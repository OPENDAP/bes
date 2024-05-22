/////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf4 data handler for the OPeNDAP data server.
// It retrieves HDF4 Vdata values for a direct DMR-mapping DAP4 response.
// Each Vdata will be mapped to a DAP variable.

//  Authors:   Kent Yang <myang6@hdfgroup.org>
// Copyright (c) The HDF Group
/////////////////////////////////////////////////////////////////////////////


#include "HDFDMRArray_VD.h"
#include "HDFStructure.h"
#include "HDFCFUtil.h"
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
    return true;

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
    int32 fieldtype = VFfieldtype(vdata_id,0);
    if (fieldtype == -1) 
        throw InternalErr (__FILE__, __LINE__, "VFfieldtype failed");

    int32 r = -1;
    // TODO: reduce the following code by not checking each datatype.

    // Loop through each data type
    switch (fieldtype) {
        case DFNT_INT8:
        case DFNT_NINT8:
        case DFNT_CHAR8:
        {
            vector<int8> val;
            val.resize(nelms);

            vector<int8>orival;
            orival.resize(vdfelms);

            // Read the data
            r = VSread (vdata_id, (uint8 *) orival.data(), 1+(count[0] -1)* step[0],FULL_INTERLACE);
            if (r == -1) {
                ostringstream eherr;
                eherr << "VSread failed.";
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }

            // Obtain the subset portion of the data
            if (fdorder > 1) {
               for (int i = 0; i < count[0]; i++)
                    for (int j = 0; j < count[1]; j++)
                        val[i * count[1] + j] =
                        orival[i * fdorder * step[0] + offset[1] + j * step[1]];
            }
            else {
                for (int i = 0; i < count[0]; i++)
                    val[i] = orival[i * step[0]];
            }

            set_value ((dods_int8 *) val.data(), nelms);
        }

            break;
        case DFNT_UINT8:
        case DFNT_NUINT8:
        case DFNT_UCHAR8:
        {

            vector<uint8>val;
            val.resize(nelms);
      
            vector<uint8>orival;
            orival.resize(vdfelms);

            r = VSread (vdata_id, orival.data(), 1+(count[0] -1)* step[0], FULL_INTERLACE);
            if (r == -1) {
                ostringstream eherr;
                eherr << "VSread failed.";
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }

            if (fdorder > 1) {
                for (int i = 0; i < count[0]; i++)
                    for (int j = 0; j < count[1]; j++)
                        val[i * count[1] + j] =	orival[i * fdorder * step[0] + offset[1] + j * step[1]];
            }
            else {
                for (int i = 0; i < count[0]; i++)
                    val[i] = orival[i * step[0]];
            }

            set_value ((dods_byte *) val.data(), nelms);
        }

            break;

        case DFNT_INT16:
        {
            vector<int16>val;
            val.resize(nelms);
            vector<int16>orival;
            orival.resize(vdfelms);

            r = VSread (vdata_id, (uint8 *) orival.data(), 1+(count[0] -1)* step[0],
                    FULL_INTERLACE);
            if (r == -1) {
                ostringstream eherr;
                eherr << "VSread failed.";
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }

            if (fdorder > 1) {
                for (int i = 0; i < count[0]; i++)
                    for (int j = 0; j < count[1]; j++)
                        val[i * count[1] + j] =	orival[i * fdorder * step[0] + offset[1] + j * step[1]];
            }
            else {
                for (int i = 0; i < count[0]; i++)
                    val[i] = orival[i * step[0]];
            }

            set_value ((dods_int16 *) val.data(), nelms);
        }
            break;

        case DFNT_UINT16:

        {
            vector<uint16>val;
            val.resize(nelms);

            vector<uint16>orival;
            orival.resize(vdfelms);

            r = VSread (vdata_id, (uint8 *) orival.data(), 1+(count[0] -1)* step[0],
                    FULL_INTERLACE);
            if (r == -1) {
                ostringstream eherr;
                eherr << "VSread failed.";
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }

            if (fdorder > 1) {
                for (int i = 0; i < count[0]; i++)
                    for (int j = 0; j < count[1]; j++)
                        val[i * count[1] + j] =	orival[i * fdorder * step[0] + offset[1] + j * step[1]];
            }
            else {
                for (int i = 0; i < count[0]; i++)
                    val[i] = orival[i * step[0]];
            }

            set_value ((dods_uint16 *) val.data(), nelms);
        }
            break;
        case DFNT_INT32:
        {
            vector<int32>val;
            val.resize(nelms);
            vector<int32>orival;
            orival.resize(vdfelms);

            r = VSread (vdata_id, (uint8 *) orival.data(), 1+(count[0] -1)* step[0],
                    FULL_INTERLACE);
            if (r == -1) {
                ostringstream eherr;
                eherr << "VSread failed.";
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }

            if (fdorder > 1) {
                for (int i = 0; i < count[0]; i++)
                    for (int j = 0; j < count[1]; j++)
                        val[i * count[1] + j] =	orival[i * fdorder * step[0] + offset[1] + j * step[1]];
            }
            else {
                for (int i = 0; i < count[0]; i++)
                    val[i] = orival[i * step[0]];
            }

            set_value ((dods_int32 *) val.data(), nelms);
        }
            break;

        case DFNT_UINT32:
        {

            vector<uint32>val;
            val.resize(nelms);

            vector<uint32>orival;
            orival.resize(vdfelms);

            r = VSread (vdata_id, (uint8 *) orival.data(), 1+(count[0] -1)* step[0],
                    FULL_INTERLACE);
            if (r == -1) {
                ostringstream eherr;
                eherr << "VSread failed.";
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }

            if (fdorder > 1) {
                for (int i = 0; i < count[0]; i++)
                    for (int j = 0; j < count[1]; j++)
                        val[i * count[1] + j] =	orival[i * fdorder * step[0] + offset[1] + j * step[1]];
            }
            else {
                for (int i = 0; i < count[0]; i++)
                    val[i] = orival[i * step[0]];
            }

            set_value ((dods_uint32 *) val.data(), nelms);
        }
            break;
        case DFNT_FLOAT32:
        {
            vector<float32>val;
            val.resize(nelms);
            vector<float32>orival;
            orival.resize(vdfelms);

            r = VSread (vdata_id, (uint8 *) orival.data(), 1+(count[0] -1)* step[0],
                    FULL_INTERLACE);
            if (r == -1) {
                ostringstream eherr;
                eherr << "VSread failed.";
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }

            if (fdorder > 1) {
                for (int i = 0; i < count[0]; i++)
                    for (int j = 0; j < count[1]; j++)
                        val[i * count[1] + j] =	orival[i * fdorder * step[0] + offset[1] + j * step[1]];
            }
            else {
                for (int i = 0; i < count[0]; i++)
                    val[i] = orival[i * step[0]];
            }

            set_value ((dods_float32 *) val.data(), nelms);
        }
            break;
        case DFNT_FLOAT64:
        {

            vector<float64>val;
            val.resize(nelms);

            vector<float64>orival;
            orival.resize(vdfelms);

            r = VSread (vdata_id, (uint8 *) orival.data(), 1+(count[0] -1)* step[0],
                    FULL_INTERLACE);
            if (r == -1) {
                ostringstream eherr;
                eherr << "VSread failed.";
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }

            if (fdorder > 1) {
                for (int i = 0; i < count[0]; i++)
                    for (int j = 0; j < count[1]; j++)
                        val[i * count[1] + j] =	orival[i * fdorder * step[0] + offset[1] + j * step[1]];
            }
            else {
                for (int i = 0; i < count[0]; i++)
                    val[i] = orival[i * step[0]];
            }

            set_value ((dods_float64 *) val.data(), nelms);
        }
            break;
        default:
            throw InternalErr (__FILE__, __LINE__, "unsupported data type.");
    }

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

    char            field_name_list[VSFIELDMAX*FIELDNAMELENMAX];
    if(VSinquire(vdata_id,nullptr,nullptr,
                     field_name_list,nullptr,nullptr) == FAIL) {
        throw InternalErr(__FILE__,__LINE__,"unable to query number of records of vdata");
    }
    if(VSsetfields(vdata_id,field_name_list) == FAIL) 
        throw InternalErr(__FILE__,__LINE__,"unable to set vdata fields");

    vector<uint8_t> data_buf;
    data_buf.resize(n_records * vdata_size);
    if (VSread (vdata_id, (uint8 *)data_buf.data(),n_records,FULL_INTERLACE)<0)  
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
      
    vector<string> field_names;
    char sep =',';
    HDFCFUtil::Split(field_name_list,sep,field_names);
#if 0
for (const auto &fn:field_names)
	cerr<<fn<<endl;
#endif
    unsigned num_fields = field_names.size();
    size_t values_offset = 0;

    HDFStructure *vdata_s = nullptr;
    // Write the values to the DAP4
    for (int64_t element = 0; element < nelms; ++element) {
    
        //auto vdata_s = dynamic_cast<HDFStructure*>(var(element));
        vdata_s = dynamic_cast<HDFStructure*>(var()->ptr_duplicate());
	size_t struct_elem_offset = values_offset + vdata_size*element;

        if(!vdata_s)
            throw InternalErr(__FILE__, __LINE__, "Cannot obtain the structure pointer.");
	
        try {
//#if 0
            int field_offset = 0;
	    for (unsigned u =0;u<num_fields;u++) {

	       BaseType * field = vdata_s->var(field_names[u]);        
	       int field_size = VFfieldisize(vdata_id,u);
	       field->val2buf(subset_buf.data() + struct_elem_offset + field_offset);
	       field_offset +=field_size;
	       field->set_read_p(true);

	    }
//#endif
#if 0
            vdata_s->read_from_value(subset_buf,values_offset);
#endif

            
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


