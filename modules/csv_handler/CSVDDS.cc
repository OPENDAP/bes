// CSVDDS.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004-2009 University Corporation for Atmospheric Research
// Author: Stephan Zednik <zednik@ucar.edu> and Patrick West <pwest@ucar.edu>
// and Jose Garcia <jgarcia@ucar.edu>
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
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301

// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//	zednik      Stephan Zednik <zednik@ucar.edu>
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include <vector>
#include <string>

#include "CSVDDS.h"
#include "CSV_Obj.h"

#include <BESInternalError.h>
#include <BESNotFoundError.h>
#include <BaseTypeFactory.h>
#include <DDS.h>
#include <Error.h>

#include <Str.h>
#include <Int16.h>
#include <Int32.h>
#include <Float32.h>
#include <Float64.h>
#include <mime_util.h>

#include <Array.h>

#include <BESDebug.h>

void csv_read_descriptors(DDS &dds, const string &filename)
{
    string type;
    int index = 0;

    Array* ar = 0;
    void* data = 0;
    BaseType *bt = 0;

    CSV_Obj* csvObj = new CSV_Obj();
    if (!csvObj->open(filename)) {
        delete csvObj;
        string err = (string) "Unable to open file " + filename;
        throw BESNotFoundError(err, __FILE__, __LINE__);
    }
    csvObj->load();

    BESDEBUG( "csv", "File loaded:" << endl << *csvObj << endl );

    dds.set_dataset_name(name_path(filename));

    vector<string> fieldList;
    csvObj->getFieldList(fieldList);
    int recordCount = csvObj->getRecordCount();
    if (recordCount < 0)
        throw BESError("Could not read record count from the CSV dataset.", BES_NOT_FOUND_ERROR, __FILE__, __LINE__);

    vector<string>::iterator it = fieldList.begin();
    vector<string>::iterator et = fieldList.end();
    for (; it != et; it++) {
        string fieldName = (*it);
        type = csvObj->getFieldType(fieldName);
        ar = dds.get_factory()->NewArray(fieldName);
        data = csvObj->getFieldData(fieldName);

        if (type.compare(string(STRING)) == 0) {
            string* strings = new string[recordCount];

            bt = dds.get_factory()->NewStr(fieldName);
            ar->add_var(bt);
            ar->append_dim(recordCount, "record");

            index = 0;
            vector<string>::iterator iv = ((vector<string>*) data)->begin();
            vector<string>::iterator ev = ((vector<string>*) data)->end();
            for (; iv != ev; iv++) {
                strings[index] = *iv;
                index++;
            }

            ar->set_value(strings, recordCount);
            delete[] strings;
        }
        else if (type.compare(string(INT16)) == 0) {
            short* int16 = new short[recordCount];
            bt = dds.get_factory()->NewInt16(fieldName);
            ar->add_var(bt);
            ar->append_dim(recordCount, "record");

            index = 0;
            vector<short>::iterator iv = ((vector<short>*) data)->begin();
            vector<short>::iterator ev = ((vector<short>*) data)->end();
            for (; iv != ev; iv++) {
                int16[index] = *iv;
                index++;
            }

            ar->set_value(int16, recordCount);
            delete[] int16;
        }
        else if (type.compare(string(INT32)) == 0) {
            int *int32 = new int[recordCount];
            bt = dds.get_factory()->NewInt32(fieldName);
            ar->add_var(bt);
            ar->append_dim(recordCount, "record");

            index = 0;
            vector<int>::iterator iv = ((vector<int>*) data)->begin();
            vector<int>::iterator ev = ((vector<int>*) data)->end();
            for (; iv != ev; iv++) {
                int32[index] = *iv;
                index++;
            }

            ar->set_value((dods_int32*) int32, recordCount);
            delete[] int32;
        }
        else if (type.compare(string(FLOAT32)) == 0) {
            float *floats = new float[recordCount];
            bt = dds.get_factory()->NewFloat32(fieldName);
            ar->add_var(bt);
            ar->append_dim(recordCount, "record");

            index = 0;
            vector<float>::iterator iv = ((vector<float>*) data)->begin();
            vector<float>::iterator ev = ((vector<float>*) data)->end();
            for (; iv != ev; iv++) {
                floats[index] = *iv;
                index++;
            }

            ar->set_value(floats, recordCount);
            delete[] floats;
        }
        else if (type.compare(string(FLOAT64)) == 0) {
            double *doubles = new double[recordCount];
            bt = dds.get_factory()->NewFloat64(fieldName);
            ar->add_var(bt);
            ar->append_dim(recordCount, "record");

            index = 0;
            vector<double>::iterator iv = ((vector<double>*) data)->begin();
            vector<double>::iterator ev = ((vector<double>*) data)->end();
            for (; iv != ev; iv++) {
                doubles[index] = *iv;
                index++;
            }

            ar->set_value(doubles, recordCount);
            delete[] doubles;
        }
        else {
            delete csvObj;
            string err = (string) "Unknown type for field " + fieldName;
            throw BESInternalError(err, __FILE__, __LINE__);
        }

        dds.add_var(ar);

        //if( ar ) {
        delete ar;
        ar = 0; // }
        //if( bt ) {
        delete bt;
        bt = 0; //}
    }

    delete csvObj;
    csvObj = 0;
}

