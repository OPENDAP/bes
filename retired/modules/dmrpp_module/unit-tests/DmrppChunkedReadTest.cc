// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2016 OPeNDAP, Inc.
// Author: Nathan David Potter <ndp@opendap.org>, James Gallagher
// <jgallagher@opendap.org>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

#include <memory>
#include <stdlib.h>
#include <unistd.h>

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include <libdap/DMR.h>

#include <BESDebug.h>
#include <BESUtil.h>

#include "Chunk.h"
#include "DmrppArray.h"
#include "DmrppByte.h"
#include "DmrppCommon.h"
#include "DmrppD4Enum.h"
#include "DmrppD4Group.h"
#include "DmrppD4Opaque.h"
#include "DmrppD4Sequence.h"
#include "DmrppFloat32.h"
#include "DmrppFloat64.h"
#include "DmrppInt16.h"
#include "DmrppInt32.h"
#include "DmrppInt64.h"
#include "DmrppInt8.h"
#include "DmrppModule.h"
#include "DmrppStr.h"
#include "DmrppStructure.h"
#include "DmrppUInt16.h"
#include "DmrppUInt32.h"
#include "DmrppUInt64.h"
#include "DmrppUrl.h"

#include "DmrppParserSax2.h"
#include "DmrppRequestHandler.h"
#include "DmrppTypeFactory.h"

//#include "GetOpt.h"
#include "test_config.h"
#include <libdap/util.h>

#include "H5Ppublic.h"
//#include "HDF5RequestHandler.h"
//#include "h5get.h"
//#include "HDF5CF.h"
#include "H5Dpublic.h"

using namespace libdap;
//using namespace HDF5CF;

static bool debug = false;

namespace dmrpp {

class DmrppChunkedReadTest: public CppUnit::TestFixture {
private:
    DmrppParserSax2 parser;

public:
    // Called once before everything gets tested
    DmrppChunkedReadTest() :
            parser()
    {
    }

    // Called at the end of the test
    ~DmrppChunkedReadTest()
    {
    }

    // Called before each test
    void setUp()
    {
        if (debug) BESDebug::SetUp("cerr,dmrpp");
    }

    // Called after each test
    void tearDown()
    {
    }

    void test_chunked_hdf5()
    {
        BESDEBUG("dmrpp", "Use: ./h5dstoreinfo h5_file_name h5_dset_path." << endl);

        string h5_file_name = string(TEST_DATA_DIR).append("/chunked_fourD.h5");
        //TODO: How to get h5_dset_path?
        string h5_dset_path = "d_16_chunks";
        hid_t file = -1; /* handles */
        hid_t dataset;

        /* Will be used to store the chunking info. */
        H5D_chunk_storage_info_t* chunk_st_ptr;
        herr_t status;

        unsigned int num_chunk_dims = 0;


        /*
         * Open the file and the dataset.
         */
        file = H5_DLL::H5Fopen(h5_file_name.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
        if (file < 0) {
            BESDEBUG("dmrpp", "HDF5 file %s cannot be opened successfully,check the file name and try again." << h5_file_name << endl);
        }

        dataset = H5_DLL::H5Dopen2(file, h5_dset_path.c_str(), H5P_DEFAULT);
        if (dataset < 0) {
            H5Fclose(file);
            BESDEBUG("dmrpp", "HDF5 dataset %s cannot be opened successfully,check the dataset path and try again." << h5_dset_path);
        }
        uint8_t layout_type = 0;
        uint8_t storage_status = 0;
        int num_chunk = 0;
        int i = 0;
        int j = 0;

        status = H5Dget_dataset_storage_info(dataset, &layout_type, (hsize_t*) &num_chunk, &storage_status);
        if (status < 0) {
            H5Dclose(dataset);
            H5Fclose(file);
            BESDEBUG("dmrpp", "Cannot get HDF5 dataset storage info. successfully." << endl);
        }

        if (storage_status == 0) {

            hid_t dtype_id = H5Dget_type(dataset);
            if (dtype_id < 0) {
                H5Dclose(dataset);
                H5Fclose(file);
                BESDEBUG("dmrpp", "Cannot obtain the correct HDF5 datatype." << endl);
            }
            if (H5Tget_class(dtype_id) == H5T_INTEGER || H5Tget_class(dtype_id) == H5T_FLOAT) {
                hid_t dcpl_id = H5Dget_create_plist(dataset);
                if (dcpl_id < 0) {
                    H5Dclose(dataset);
                    H5Fclose(file);
                    BESDEBUG("dmrpp", "Cannot obtain the HDF5 dataset creation property list." << endl);
                }

                H5D_fill_value_t fvalue_status;
                if (H5Pfill_value_defined(dcpl_id, &fvalue_status) < 0) {
                    H5Pclose(dcpl_id);
                    H5Dclose(dataset);
                    H5Fclose(file);
                    BESDEBUG("dmrpp", "Cannot obtain the fill value status." << endl);
                }

                if (fvalue_status == H5D_FILL_VALUE_UNDEFINED) {
                    if (layout_type == 1)
                        BESDEBUG("dmrpp", " The storage size is 0 and the storge type is contiguous." << endl);
                    else if (layout_type == 2)
                        BESDEBUG("dmrpp", " The storage size is 0 and the storage type is chunking." << endl);
                    else if (layout_type == 3) BESDEBUG("dmrpp", " The storage size is 0 and the storage type is compact." << endl);
                    BESDEBUG("dmrpp", " The Fillvalue is undefined ." << endl);

                }
                else {

                    if (layout_type == 1)
                        BESDEBUG("dmrpp", " The storage size is 0 and the storage type is contiguous." << endl);
                    else if (layout_type == 2)
                        BESDEBUG("dmrpp", " The storage size is 0 and the storage type is chunking." << endl);
                    else if (layout_type == 3) BESDEBUG("dmrpp", " The storage size is 0 and the storage type is compact." << endl);

                    char *fvalue = NULL;
                    size_t fv_size = H5Tget_size(dtype_id);
                    if (fv_size == 1)
                        fvalue = (char *) malloc(1);
                    else if (fv_size == 2)
                        fvalue = (char *) malloc(2);
                    else if (fv_size == 4)
                        fvalue = (char *) malloc(4);
                    else if (fv_size == 8)
                        fvalue = (char *) malloc(8);

                    if (fv_size <= 8) {
                        if (H5Pget_fill_value(dcpl_id, dtype_id, (void*) fvalue) < 0) {
                            H5Pclose(dcpl_id);
                            H5Dclose(dataset);
                            H5Fclose(file);
                            BESDEBUG("dmrpp", "Cannot obtain the fill value status." << endl);
                        }

                        if (H5Tget_class(dtype_id) == H5T_INTEGER) {

                            H5T_sign_t fv_sign = H5Tget_sign(dtype_id);
                            if (fv_size == 1) {
                                if (fv_sign == H5T_SGN_NONE) {

                                    BESDEBUG("dmrpp", "This dataset's datatype is unsigned char " << endl);
                                    BESDEBUG("dmrpp", "and the fillvalue is " << *fvalue << endl);
                                }
                                else {
                                    BESDEBUG("dmrpp", "This dataset's datatype is char ");
                                    BESDEBUG("dmrpp", "and the fillvalue is " << *fvalue << endl);
                                }
                            }
                            else if (fv_size == 2) {
                                if (fv_sign == H5T_SGN_NONE) {
                                    BESDEBUG("dmrpp", "This dataset's datatype is unsigned short ");
                                    BESDEBUG("dmrpp", "and the fillvalue is " << *fvalue << endl);
                                }
                                else {
                                    BESDEBUG("dmrpp", "This dataset's datatype is short ");
                                    BESDEBUG("dmrpp", "and the fillvalue is " << *fvalue << endl);
                                }
                            }
                            else if (fv_size == 4) {
                                if (fv_sign == H5T_SGN_NONE) {
                                    BESDEBUG("dmrpp", "This dataset's datatype is unsigned int ");
                                    BESDEBUG("dmrpp", "and the fillvalue is " << *fvalue << endl);
                                }
                                else {
                                    BESDEBUG("dmrpp", "This dataset's datatype is int ");
                                    BESDEBUG("dmrpp", "and the fillvalue is " << *fvalue << endl);
                                }
                            }
                            else if (fv_size == 8) {
                                if (fv_sign == H5T_SGN_NONE) {
                                    BESDEBUG("dmrpp", "This dataset's datatype is unsigned long long ");
                                    BESDEBUG("dmrpp", "and the fillvalue is " << *fvalue << endl);
                                }
                                else {
                                    BESDEBUG("dmrpp", "This dataset's datatype is long long ");
                                    BESDEBUG("dmrpp", "and the fillvalue is " << *fvalue << endl);
                                }
                            }
                        }
                        if (H5Tget_class(dtype_id) == H5T_FLOAT) {
                            if (fv_size == 4) {
                                BESDEBUG("dmrpp", "This dataset's datatype is float ");
                                BESDEBUG("dmrpp", "and the fillvalue is " << *fvalue << endl);
                            }
                            else if (fv_size == 8) {
                                BESDEBUG("dmrpp", "This dataset's datatype is double ");
                                BESDEBUG("dmrpp", "and the fillvalue is " << *fvalue << endl);
                            }
                        }

                        if (fvalue != NULL) free(fvalue);

                    }
                    else
                        BESDEBUG("dmrpp",
                            "The size of the datatype is greater than 8 bytes, Use HDF5 API H5Pget_fill_value() to retrieve the fill value of this dataset." << endl);

                }
                H5Pclose(dcpl_id);

            }
            else {
                if (layout_type == 1)
                    BESDEBUG("dmrpp", " The storage size is 0 and the storage type is contiguous." << endl);
                else if (layout_type == 2)
                    BESDEBUG("dmrpp", " The storage size is 0 and the storage type is chunking." << endl);
                else if (layout_type == 3) BESDEBUG("dmrpp", " The storage size is 0 and the storage type is compact." << endl);

                BESDEBUG("dmrpp",
                    "The datatype is neither float nor integer,use HDF5 API H5Pget_fill_value() to retrieve the fill value of this dataset." << endl);

            }

        }

        else {
            /* layout_type:  1 contiguous 2 chunk 3 compact */
            if (layout_type == 1) {/* Contiguous storage */
                haddr_t cont_addr = 0;
                hsize_t cont_size = 0;
                BESDEBUG("dmrpp", "Storage: contiguous" << endl);
                if (H5Dget_dataset_contiguous_storage_info(dataset, &cont_addr, &cont_size) < 0) {
                    H5Dclose(dataset);
                    H5Fclose(file);
                    BESDEBUG("dmrpp", "Cannot obtain the contiguous storage info." << endl);
                }
                BESDEBUG("dmrpp", "    Addr: " << cont_addr << endl);
                BESDEBUG("dmrpp", "    Size: " << cont_size << endl);

            }
            else if (layout_type == 2) {/*chunking storage */
                BESDEBUG("dmrpp", "storage: chunked." << endl);
                BESDEBUG("dmrpp", "   Number of chunks is " << num_chunk << endl);

                /* Allocate the memory for the struct to obtain the chunk storage information */
                chunk_st_ptr = (H5D_chunk_storage_info_t*) calloc(num_chunk, sizeof(H5D_chunk_storage_info_t));
                if (!chunk_st_ptr) {
                    H5Dclose(dataset);
                    H5Fclose(file);
                    BESDEBUG("dmrpp", "Cannot allocate the memory to store the chunked storage info." << endl);
                }

                if (H5Dget_dataset_chunk_storage_info(dataset, chunk_st_ptr, &num_chunk_dims) < 0) {
                    H5Dclose(dataset);
                    H5Fclose(file);
                    BESDEBUG("dmrpp", "Cannot get HDF5 chunk storage info. successfully." << endl);
                }

                BESDEBUG("dmrpp", "   Number of dimensions in a chunk is " << num_chunk_dims - 1 << endl);
                for (int i = 0; i < num_chunk; i++) {
                    BESDEBUG("dmrpp", "    Chunk index:  " << i << endl);
                    BESDEBUG("dmrpp", "      Number of bytes: " << chunk_st_ptr[i].nbytes << endl);
                    BESDEBUG("dmrpp", "      Logical offset: offset");
                    for (int j = 0; j < num_chunk_dims - 1; j++)
                        BESDEBUG("dmrpp", "[" << chunk_st_ptr[i].chunk_offset[j] << "]" << endl);
                    BESDEBUG("dmrpp", "" << endl);
                    BESDEBUG("dmrpp", "      Physical offset: " << chunk_st_ptr[i].chunk_addr << endl);
                }
                free(chunk_st_ptr);
            }
            else if (layout_type == 3) {/* Compact storage */
                BESDEBUG("dmrpp", "Storage: compact" << endl);
                size_t comp_size = 0;
                if (H5Dget_dataset_compact_storage_info(dataset, &comp_size) < 0) {
                    H5Dclose(dataset);
                    H5Fclose(file);
                    BESDEBUG("dmrpp", "Cannot obtain the compact storage info." << endl);
                }
                BESDEBUG("dmrpp", "   Size: " << comp_size << endl);

            }
        }
        H5Dclose(dataset);
        H5Fclose(file);
    }

#if 0
    /**
     * Evaluates a D4Group against exected values for the name, number of child
     * groups and the number of child variables.
     */
    void checkGroupsAndVars(D4Group *grp, string name, int expectedNumGrps, int expectedNumVars)
    {
        CPPUNIT_ASSERT(grp);
        BESDEBUG("dmrpp", "Checking D4Group '" << grp->name() << "'" << endl);

        CPPUNIT_ASSERT(grp->name() == name);

        int numGroups = grp->grp_end() - grp->grp_begin();
        BESDEBUG("dmrpp", "The D4Group '" << grp->name() << "' has " << numGroups << " child groups." << endl);
        CPPUNIT_ASSERT(numGroups == expectedNumGrps);

        int numVars = grp->var_end() - grp->var_begin();
        BESDEBUG("dmrpp", "The D4Group '" << grp->name() << "' has " << numVars << " child variables." << endl);
        CPPUNIT_ASSERT(numVars == expectedNumVars);
    }

    /**
     * This is a test hack in which we capitalize on the fact that we know
     * that the test Dmrpp files all have "local" names stored for their
     * xml:base URLs. Here we make these full paths on the file system
     * so that the source data files can be located at test runtime.
     */
    void set_data_url_in_chunks(DmrppCommon *dc)
    {
        // Get the chunks and make sure there's at least one
        vector<Chunk> &chunks = dc->get_chunk_vec();
        CPPUNIT_ASSERT(chunks.size() > 0);
        // Tweak the data URLs for the test
        for (unsigned int i = 0; i < chunks.size(); i++) {
            BESDEBUG("dmrpp", "chunk_refs[" << i << "]: " << chunks[i].to_string() << endl);
        }
        for (unsigned int i = 0; i < chunks.size(); i++) {
            string data_url = BESUtil::assemblePath(TEST_DMRPP_CATALOG, chunks[i].get_data_url(), true);
            data_url = "file://" + data_url;
            chunks[i].set_data_url(data_url);
        }
        for (unsigned int i = 0; i < chunks.size(); i++) {
            BESDEBUG("dmrpp", "altered chunk_refs[" << i << "]: " << chunks[i].to_string() << endl);
        }
    }

    /**
     * Checks name, reads data, checks # of read bytes.
     */
    void read_var_check_name_and_size(DmrppArray *array, string name, int length)
    {
        BESDEBUG("dmrpp", __func__ << "() - array->name(): " << array->name() << " expected_name: " << name << endl);
        CPPUNIT_ASSERT(array->name() == name);

        set_data_url_in_chunks(array);

        // Read the data
        array->read();
        BESDEBUG("dmrpp", __func__ << "() - array->size(): " << array->size() << " expected_length: " << length << endl);
        CPPUNIT_ASSERT(array->size() == length);
    }

    /**
     * Since we have a lot of test data files that contain a single array here
     * is a complete test of reading the array and verifying its content.
     */
    void check_f32_test_array(string filename, string variable_name, unsigned long long array_length)
    {
        auto_ptr<DMR> dmr(new DMR);
        DmrppTypeFactory dtf;
        dmr->set_factory(&dtf);
        dods_float32 test_float32;

        BESDEBUG("dmrpp", __func__ << "() - Opening: " << filename << endl);

        ifstream in(filename.c_str());
        parser.intern(in, dmr.get(), debug);
        BESDEBUG("dmrpp", __func__ << "() - Parsing complete"<< endl);

        // Check to make sure we have something that smells like our test array
        D4Group *root = dmr->root();
        checkGroupsAndVars(root, "/", 0, 1);
        // Walk the vars and testy testy
        D4Group::Vars_iter vIter = root->var_begin();
        try {
            // Read the variable and transfer the data
            DmrppArray *var = dynamic_cast<DmrppArray*>(*vIter);
            read_var_check_name_and_size(var, variable_name, array_length);
            vector<dods_float32> values(var->size());
            var->value(values.data());

            // Test data set is incrementally valued: Check Them All!
            for (unsigned long long a_index = 0; a_index < array_length; a_index++) {
                test_float32 = a_index;
                if (!double_eq(values[a_index], test_float32))
                BESDEBUG("dmrpp",
                    "values[" << a_index << "]: " << values[a_index] << "  test_float32: " << test_float32 << endl);
                CPPUNIT_ASSERT(double_eq(values[a_index], test_float32));
            }
        }
        catch (BESError &e) {
            CPPUNIT_FAIL(e.get_message());
        }
        catch (Error &e) {
            CPPUNIT_FAIL(e.get_error_message());
        }
        catch (std::exception &e) {
            CPPUNIT_FAIL(e.what());
        }

        CPPUNIT_ASSERT("Passed");
    }

    /**
     * Tests oneD array against a CE with start stride and stop that
     * retrieves data from all chunks. The stride >1 means that each value
     * must be individually copied.
     */
    void test_chunked_oneD_CE_00()
    {
        string chnkd_oneD = string(TEST_DATA_DIR).append("/").append("chunked_oneD.h5.dmrpp");

        unsigned long long array_length = 40000;
        string variable_name = "d_4_chunks";
        string filename = chnkd_oneD;
        auto_ptr<DMR> dmr(new DMR);
        DmrppTypeFactory dtf;
        dmr->set_factory(&dtf);
        dods_float32 test_float32;

        BESDEBUG("dmrpp", __func__ << "() - Opening: " << filename << endl);

        ifstream in(filename.c_str());
        parser.intern(in, dmr.get(), debug);
        BESDEBUG("dmrpp", __func__ << "() - Parsing complete"<< endl);

        // Check to make sure we have something that smells like our test array
        D4Group *root = dmr->root();
        checkGroupsAndVars(root, "/", 0, 1);
        // Walk the vars and testy testy
        D4Group::Vars_iter vIter = root->var_begin();
        try {
            DmrppArray *var = dynamic_cast<DmrppArray*>(*vIter);

            unsigned int start = 10;
            unsigned int stride = 1; //100
            unsigned int stop = 35010;
            // Constrain the array
            array_length = 1 + (stop - start) / stride;
            BESDEBUG("dmrpp", __func__ << "() - array_length:  " << array_length << endl);
            var->add_constraint(var->dim_begin(), start, stride, stop);

            BESDEBUG("dmrpp", __func__ << "() - dim_start:  " << var->dimension_start(var->dim_begin()) << endl);
            BESDEBUG("dmrpp", __func__ << "() - dim_stride: " << var->dimension_stride(var->dim_begin()) << endl);
            BESDEBUG("dmrpp", __func__ << "() - dim_stop:   " << var->dimension_stop(var->dim_begin()) << endl);
            BESDEBUG("dmrpp", __func__ << "() - dim_size:   " << var->dimension_size(var->dim_begin()) << endl);
            BESDEBUG("dmrpp", __func__ << "() - length: " << var->size() << endl);

            // Read the variable and transfer the data
            read_var_check_name_and_size(var, variable_name, array_length);
            vector<dods_float32> values(var->size());
            var->value(values.data());

            // Test data set is incrementally valued: Check Them All!
            for (unsigned long long a_index = 0; a_index < array_length; a_index++) {
                test_float32 = a_index * stride + start;
                if (!double_eq(values[a_index], test_float32))
                BESDEBUG("dmrpp",
                    "values[" << a_index << "]: " << values[a_index] << "  test_float32: " << test_float32 << endl);
                CPPUNIT_ASSERT(double_eq(values[a_index], test_float32));
            }
        }
        catch (BESError &e) {
            CPPUNIT_FAIL(e.get_message());
        }
        catch (Error &e) {
            CPPUNIT_FAIL(e.get_error_message());
        }
        catch (std::exception &e) {
            CPPUNIT_FAIL(e.what());
        }

        CPPUNIT_ASSERT("Passed");
    }

    /**
     * Here we use a CE which:
     * a) Does not retrieve from all chunks
     * b) Uses a stride of 1
     * Against the chunked oneD test array. The stride==1 means that
     * contiguous blocks may be copied.
     */
    void test_chunked_oneD_CE_01()
    {
        string chnkd_oneD = string(TEST_DATA_DIR).append("/").append("chunked_oneD.h5.dmrpp");

        unsigned long long array_length = 40000;
        string variable_name = "d_4_chunks";
        string filename = chnkd_oneD;
        auto_ptr<DMR> dmr(new DMR);
        DmrppTypeFactory dtf;
        dmr->set_factory(&dtf);
        dods_float32 test_float32;

        BESDEBUG("dmrpp", __func__ << "() - Opening: " << filename << endl);

        ifstream in(filename.c_str());
        parser.intern(in, dmr.get(), debug);
        BESDEBUG("dmrpp", __func__ << "() - Parsing complete"<< endl);

        // Check to make sure we have something that smells like our test array
        D4Group *root = dmr->root();
        checkGroupsAndVars(root, "/", 0, 1);
        // Walk the vars and testy testy
        D4Group::Vars_iter vIter = root->var_begin();
        try {
            DmrppArray *var = dynamic_cast<DmrppArray*>(*vIter);

            unsigned int start = 10;
            unsigned int stride = 1;
            unsigned int stop = 15009;
            // Constrain the array
            array_length = 1 + (stop - start) / stride;
            BESDEBUG("dmrpp", __func__ << "() - array_length:  " << array_length << endl);
            var->add_constraint(var->dim_begin(), start, stride, stop);

            BESDEBUG("dmrpp", __func__ << "() - dim_start:  " << var->dimension_start(var->dim_begin()) << endl);
            BESDEBUG("dmrpp", __func__ << "() - dim_stride: " << var->dimension_stride(var->dim_begin()) << endl);
            BESDEBUG("dmrpp", __func__ << "() - dim_stop:   " << var->dimension_stop(var->dim_begin()) << endl);
            BESDEBUG("dmrpp", __func__ << "() - dim_size:   " << var->dimension_size(var->dim_begin()) << endl);
            BESDEBUG("dmrpp", __func__ << "() - length: " << var->size() << endl);

            // Read the variable and transfer the data
            read_var_check_name_and_size(var, variable_name, array_length);
            vector<dods_float32> values(var->size());
            var->value(values.data());

            // Test data set is incrementally valued: Check Them All!
            for (unsigned long long a_index = 0; a_index < array_length; a_index++) {
                test_float32 = a_index + 10;
                if (!double_eq(values[a_index], test_float32))
                BESDEBUG("dmrpp",
                    "values[" << a_index << "]: " << values[a_index] << "  test_float32: " << test_float32 << endl);
                CPPUNIT_ASSERT(double_eq(values[a_index], test_float32));
            }
        }
        catch (BESError &e) {
            CPPUNIT_FAIL(e.get_message());
        }
        catch (Error &e) {
            CPPUNIT_FAIL(e.get_error_message());
        }
        catch (std::exception &e) {
            CPPUNIT_FAIL(e.what());
        }

        CPPUNIT_ASSERT("Passed");
    }

    void test_read_oneD_chunked_array()
    {
        string chnkd_oneD = string(TEST_DATA_DIR).append("/").append("chunked_oneD.h5.dmrpp");
        check_f32_test_array(chnkd_oneD, "d_4_chunks", 40000);
    }

    void test_read_oneD_uneven_chunked_array()
    {
        string chnkd_oneD = string(TEST_DATA_DIR).append("/").append("chunked_oneD_uneven.h5.dmrpp");
        check_f32_test_array(chnkd_oneD, "d_5_odd_chunks", 40000);
    }

    void test_read_twoD_chunked_array()
    {
        string chnkd_twoD = string(TEST_DATA_DIR).append("/").append("chunked_twoD.h5.dmrpp");
        check_f32_test_array(chnkd_twoD, "d_4_chunks", 10000);
    }

    void test_read_twoD_uneven_chunked_array()
    {
        string chnkd_twoD = string(TEST_DATA_DIR).append("/").append("chunked_twoD_uneven.h5.dmrpp");
        check_f32_test_array(chnkd_twoD, "d_10_odd_chunks", 10000);
    }

    void test_read_twoD_chunked_asymmetric_array()
    {
        string chnkd_twoD_asym = string(TEST_DATA_DIR).append("/").append("chunked_twoD_asymmetric.h5.dmrpp");
        check_f32_test_array(chnkd_twoD_asym, "d_8_chunks", 20000);
    }

    void test_read_threeD_chunked_array()
    {
        string chnkd_threeD = string(TEST_DATA_DIR).append("/").append("chunked_threeD.h5.dmrpp");
        check_f32_test_array(chnkd_threeD, "d_8_chunks", 1000000);
    }

    void test_read_threeD_chunked_asymmetric_array()
    {
        string chnkd_threeD_asym = string(TEST_DATA_DIR).append("/").append("chunked_threeD_asymmetric.h5.dmrpp");
        check_f32_test_array(chnkd_threeD_asym, "d_8_chunks", 1000000);
    }

    void test_read_fourD_chunked_array()
    {
        string chnkd_fourD = string(TEST_DATA_DIR).append("/").append("chunked_fourD.h5.dmrpp");
        check_f32_test_array(chnkd_fourD, "d_16_chunks", 2560000);
    }

    void test_chunked_gzipped_oneD()
    {
        string chnkd_oneD = string(TEST_DATA_DIR).append("/").append("chunked_gzipped_oneD.h5.dmrpp");
        check_f32_test_array(chnkd_oneD, "d_4_gzipped_chunks", 40000);
    }

    void test_chunked_gzipped_twoD()
    {
        string chnkd_twoD = string(TEST_DATA_DIR).append("/").append("chunked_gzipped_twoD.h5.dmrpp");
        check_f32_test_array(chnkd_twoD, "d_4_gzipped_chunks", 100*100);
    }

    void test_chunked_gzipped_threeD()
    {
        string chnkd_threeD = string(TEST_DATA_DIR).append("/").append("chunked_gzipped_threeD.h5.dmrpp");
        check_f32_test_array(chnkd_threeD, "d_8_gzipped_chunks", 100*100*100);
    }
    void test_chunked_gzipped_fourD()
    {
        string chnkd_fourD = string(TEST_DATA_DIR).append("/").append("chunked_gzipped_fourD.h5.dmrpp");
        check_f32_test_array(chnkd_fourD, "d_16_gzipped_chunks", 40*40*40*40);
    }

    void test_chunked_gzipped_oneD_CE_00()
    {
        string chnkd_oneD = test_data_dir + "/chunked_gzipped_oneD.h5.dmrpp";

        unsigned long long array_length = 40000;
        string variable_name = "d_4_gzipped_chunks";
        string filename = chnkd_oneD;
        auto_ptr<DMR> dmr(new DMR);
        DmrppTypeFactory dtf;
        dmr->set_factory(&dtf);
        dods_float32 test_float32;

        BESDEBUG("dmrpp", __func__ << "() - Opening: " << filename << endl);

        ifstream in(filename.c_str());
        parser.intern(in, dmr.get(), debug);
        BESDEBUG("dmrpp", __func__ << "() - Parsing complete"<< endl);

        // Check to make sure we have something that smells like our test array
        D4Group *root = dmr->root();
        checkGroupsAndVars(root, "/", 0, 1);
        // Walk the vars and testy testy
        D4Group::Vars_iter vIter = root->var_begin();
        try {
            DmrppArray *var = dynamic_cast<DmrppArray*>(*vIter);

            unsigned int start = 10;
            unsigned int stride = 1; //100
            unsigned int stop = 35010;
            // Constrain the array
            array_length = 1 + (stop - start) / stride;
            BESDEBUG("dmrpp", __func__ << "() - array_length:  " << array_length << endl);
            var->add_constraint(var->dim_begin(), start, stride, stop);

            BESDEBUG("dmrpp", __func__ << "() - dim_start:  " << var->dimension_start(var->dim_begin()) << endl);
            BESDEBUG("dmrpp", __func__ << "() - dim_stride: " << var->dimension_stride(var->dim_begin()) << endl);
            BESDEBUG("dmrpp", __func__ << "() - dim_stop:   " << var->dimension_stop(var->dim_begin()) << endl);
            BESDEBUG("dmrpp", __func__ << "() - dim_size:   " << var->dimension_size(var->dim_begin()) << endl);
            BESDEBUG("dmrpp", __func__ << "() - length: " << var->size() << endl);

            // Read the variable and transfer the data
            read_var_check_name_and_size(var, variable_name, array_length);
            vector<dods_float32> values(var->size());
            var->value(values.data());

            // Test data set is incrementally valued: Check Them All!
            for (unsigned long long a_index = 0; a_index < array_length; a_index++) {
                test_float32 = a_index * stride + start;
                if (!double_eq(values[a_index], test_float32))
                BESDEBUG("dmrpp",
                    "values[" << a_index << "]: " << values[a_index] << "  test_float32: " << test_float32 << endl);
                CPPUNIT_ASSERT(double_eq(values[a_index], test_float32));
            }
        }
        catch (BESError &e) {
            CPPUNIT_FAIL(e.get_message());
        }
        catch (Error &e) {
            CPPUNIT_FAIL(e.get_error_message());
        }
        catch (std::exception &e) {
            CPPUNIT_FAIL(e.what());
        }

        CPPUNIT_ASSERT("Passed");
    }

    void test_chunked_shuffled_oneD()
    {
        string chnkd_fourD = string(TEST_DATA_DIR).append("/").append("chunked_shuffled_oneD.h5.dmrpp");
        check_f32_test_array(chnkd_fourD, "d_4_shuffled_chunks", 40000);
    }

    void test_chunked_shuffled_twoD()
    {
        string chnkd_fourD = string(TEST_DATA_DIR).append("/").append("chunked_shuffled_twoD.h5.dmrpp");
        check_f32_test_array(chnkd_fourD, "d_4_shuffled_chunks", 10000);
    }
    void test_chunked_shuffled_threeD()
    {
        string chnkd_fourD = string(TEST_DATA_DIR).append("/").append("chunked_shuffled_threeD.h5.dmrpp");
        check_f32_test_array(chnkd_fourD, "d_8_shuffled_chunks", 1000000);
    }
    void test_chunked_shuffled_fourD()
    {
        string chnkd_fourD = string(TEST_DATA_DIR).append("/").append("chunked_shuffled_fourD.h5.dmrpp");
        check_f32_test_array(chnkd_fourD, "d_16_shuffled_chunks", 2560000);
    }

    void test_chunked_shuffled_zipped_oneD()
    {
        string chnkd_fourD = string(TEST_DATA_DIR).append("/").append("chunked_shufzip_oneD.h5.dmrpp");
        check_f32_test_array(chnkd_fourD, "d_4_shufzip_chunks", 40000);
    }

    void test_chunked_shuffled_zipped_twoD()
    {
        string chnkd_fourD = string(TEST_DATA_DIR).append("/").append("chunked_shufzip_twoD.h5.dmrpp");
        check_f32_test_array(chnkd_fourD, "d_4_shufzip_chunks", 10000);
    }
    void test_chunked_shuffled_zipped_threeD()
    {
        string chnkd_fourD = string(TEST_DATA_DIR).append("/").append("chunked_shufzip_threeD.h5.dmrpp");
        check_f32_test_array(chnkd_fourD, "d_8_shufzip_chunks", 1000000);
    }
    void test_chunked_shuffled_zipped_fourD()
    {
        string chnkd_fourD = string(TEST_DATA_DIR).append("/").append("chunked_shufzip_fourD.h5.dmrpp");
        check_f32_test_array(chnkd_fourD, "d_16_shufzip_chunks", 2560000);
    }

    void test_a2_local_twoD_chunked_array()
    {
        string chnkd_twoD = string(TEST_DATA_DIR).append("/").append("a2_local_twoD.h5.dmrpp");
        check_f32_test_array(chnkd_twoD, "d_4_chunks", 10000);
    }

    void test_a3_local_twoD_chunked_array()
    {
        string chnkd_twoD = string(TEST_DATA_DIR).append("/").append("a3_local_twoD.h5.dmrpp");
        check_f32_test_array(chnkd_twoD, "d_4_shufzip_chunks", 10000);
    }
#endif

    CPPUNIT_TEST_SUITE( DmrppChunkedReadTest );

#if 0
    CPPUNIT_TEST(test_read_oneD_chunked_array);
    CPPUNIT_TEST(test_read_twoD_chunked_array);
    CPPUNIT_TEST(test_read_twoD_chunked_asymmetric_array);
    CPPUNIT_TEST(test_read_threeD_chunked_array);
    CPPUNIT_TEST(test_read_threeD_chunked_asymmetric_array);
    CPPUNIT_TEST(test_read_fourD_chunked_array);
    CPPUNIT_TEST(test_chunked_oneD_CE_00);
    CPPUNIT_TEST(test_chunked_oneD_CE_01);
    CPPUNIT_TEST(test_read_oneD_uneven_chunked_array);
    CPPUNIT_TEST(test_read_twoD_uneven_chunked_array);
#if 1

    CPPUNIT_TEST(test_chunked_gzipped_oneD);
    CPPUNIT_TEST(test_chunked_gzipped_twoD);
    CPPUNIT_TEST(test_chunked_gzipped_threeD);
    CPPUNIT_TEST(test_chunked_gzipped_fourD);
    CPPUNIT_TEST(test_chunked_gzipped_oneD_CE_00);

    CPPUNIT_TEST(test_chunked_shuffled_oneD);
    CPPUNIT_TEST(test_chunked_shuffled_twoD);
    CPPUNIT_TEST(test_chunked_shuffled_threeD);
    CPPUNIT_TEST(test_chunked_shuffled_fourD);

    CPPUNIT_TEST(test_chunked_shuffled_zipped_oneD);
    CPPUNIT_TEST(test_chunked_shuffled_zipped_twoD);
    CPPUNIT_TEST(test_chunked_shuffled_zipped_threeD);
    CPPUNIT_TEST(test_chunked_shuffled_zipped_fourD);

    CPPUNIT_TEST(test_a2_local_twoD_chunked_array);
    CPPUNIT_TEST(test_a3_local_twoD_chunked_array);
    CPPUNIT_TEST(test_chunked_hdf5);
#endif
#endif

    CPPUNIT_TEST(test_chunked_hdf5);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(DmrppChunkedReadTest);

} // namespace dmrpp

int main(int argc, char*argv[])
{
#if 0
    GetOpt getopt(argc, argv, "dh");
    int option_char;
    while ((option_char = getopt()) != -1){
#endif

        int option_char;
        while ((option_char = getopt(argc, argv, "dh")) != -1) {
        switch (option_char) {
        case 'd':
            debug = true;  // debug is a static global
            break;
        case 'h': {     // help - show test names
            std::cerr << "Usage: DmrppChunkedReadTest has the following tests:" << std::endl;
            const std::vector<CppUnit::Test*> &tests = dmrpp::DmrppChunkedReadTest::suite()->getTests();
            unsigned int prefix_len = dmrpp::DmrppChunkedReadTest::suite()->getName().append("::").size();
            for (std::vector<CppUnit::Test*>::const_iterator i = tests.begin(), e = tests.end(); i != e; ++i) {
                std::cerr << (*i)->getName().replace(0, prefix_len, "") << std::endl;
            }
            break;
        }
        default:
            break;
        }
    }

    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

    bool wasSuccessful = true;
    string test = "";
    int i = optind;
    if (i == argc) {
        // run them all
        wasSuccessful = runner.run("");
    }
    else {
        while (i < argc) {
            if (debug) cerr << "Running " << argv[i] << endl;
            test = dmrpp::DmrppChunkedReadTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
            ++i;
        }
    }

    return wasSuccessful ? 0 : 1;
}

