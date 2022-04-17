// arrayT.cc

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

using std::ofstream;
using std::ios;
using std::cerr;
using std::endl;
using std::ostringstream;

#include <libdap/DataDDS.h>
#include <libdap/Array.h>
#include <libdap/Byte.h>
#include <libdap/Int16.h>
#include <libdap/Int32.h>
#include <libdap/UInt16.h>
#include <libdap/UInt32.h>
#include <libdap/Float32.h>
#include <libdap/Float64.h>
#include <libdap/Str.h>

using namespace ::libdap;

//#include <BESPlugin.h>

//#include <BESDapModule.h>

#include <BESDataHandlerInterface.h>
#include <BESDataNames.h>
#include <BESDebug.h>

#include "test_config.h"
#include "test_send_data.h"

int main(int argc, char **argv)
{
    bool debug = false;
    if (argc > 1) {
        for (int i = 0; i < argc; i++) {
            string arg = argv[i];
            if (arg == "debug") {
                debug = true;
            }
        }
    }

    try {
#if 0
        string bes_conf = (string) "BES_CONF=" + TEST_BUILD_DIR + "/bes.conf";
        putenv((char *) bes_conf.c_str());
#endif
        if (debug)
            BESDebug::SetUp("cerr,fonc");

        // build a DataDDS of simple types and set values for each of the
        // simple types.
        DataDDS *dds = new DataDDS(NULL, "virtual");
        {
            Byte *b = new Byte("byte_array");
            Array *a = new Array("array", b);
            delete b;
            b = 0;
            a->append_dim(2, "byte_dim1");
            a->append_dim(5, "byte_dim2");
            dds->add_var(a);
            delete a;
            a = 0;

            a = dynamic_cast<Array *>(dds->var("byte_array"));
            if (!a) {
                delete dds;
                string err = "cast error for byte_array";
                throw BESError(err, 0, __FILE__, __LINE__);
            }

            vector<dods_byte> ba;
            for (dods_byte i = 0; i < 10; i++) {
                ba.push_back(i);
            }
            a->set_value(ba, ba.size());
        }
        {
            Int16 *i16 = new Int16("i16_array");
            Array *a = new Array("array", i16);
            delete i16;
            i16 = 0;
            a->append_dim(3, "i16_dim1");
            a->append_dim(6, "i16_dim2");
            dds->add_var(a);
            delete a;
            a = 0;

            a = dynamic_cast<Array *>(dds->var("i16_array"));
            if (!a) {
                delete dds;
                string err = "cast error for i16_array";
                throw BESError(err, 0, __FILE__, __LINE__);
            }

            vector<dods_int16> i16a;
            for (dods_int16 i = 0; i < 18; i++) {
                i16a.push_back(i * (-16));
            }
            a->set_value(i16a, i16a.size());
        }
        {
            Int32 *i32 = new Int32("i32_array");
            Array *a = new Array("array", i32);
            delete i32;
            i32 = 0;
            a->append_dim(2, "i32_dim1");
            a->append_dim(20, "i32_dim2");
            dds->add_var(a);
            delete a;
            a = 0;

            a = dynamic_cast<Array *>(dds->var("i32_array"));
            if (!a) {
                delete dds;
                string err = "cast error for i32_array";
                throw BESError(err, 0, __FILE__, __LINE__);
            }

            vector<dods_int32> i32a;
            for (dods_int32 i = 0; i < 40; i++) {
                i32a.push_back(i * (-512));
            }
            a->set_value(i32a, i32a.size());
        }
        {
            UInt16 *ui16 = new UInt16("ui16_array");
            Array *a = new Array("array", ui16);
            delete ui16;
            ui16 = 0;
            a->append_dim(3, "ui16_dim1");
            a->append_dim(7, "ui16_dim2");
            dds->add_var(a);
            delete a;
            a = 0;

            a = dynamic_cast<Array *>(dds->var("ui16_array"));
            if (!a) {
                delete dds;
                string err = "cast error for ui16_array";
                throw BESError(err, 0, __FILE__, __LINE__);
            }

            vector<dods_uint16> ui16a;
            for (dods_uint16 i = 0; i < 21; i++) {
                ui16a.push_back(i * 16);
            }
            a->set_value(ui16a, ui16a.size());
        }
        {
            UInt32 *ui32 = new UInt32("ui32_array");
            Array *a = new Array("array", ui32);
            delete ui32;
            ui32 = 0;
            a->append_dim(3, "ui32_dim1");
            a->append_dim(40, "ui32_dim2");
            dds->add_var(a);
            delete a;
            a = 0;

            a = dynamic_cast<Array *>(dds->var("ui32_array"));
            if (!a) {
                delete dds;
                string err = "cast error for ui32_array";
                throw BESError(err, 0, __FILE__, __LINE__);
            }

            vector<dods_uint32> ui32a;
            for (dods_uint32 i = 0; i < 120; i++) {
                ui32a.push_back(i * 512);
            }
            a->set_value(ui32a, ui32a.size());
        }
        {
            Float32 *f32 = new Float32("f32_array");
            Array *a = new Array("array", f32);
            delete f32;
            f32 = 0;
            a->append_dim(100, "f32_dim1");
            dds->add_var(a);
            delete a;
            a = 0;

            a = dynamic_cast<Array *>(dds->var("f32_array"));
            if (!a) {
                delete dds;
                string err = "cast error for f32_array";
                throw BESError(err, 0, __FILE__, __LINE__);
            }

            vector<dods_float32> f32a;
            for (int i = 0; i < 100; i++) {
                f32a.push_back(i * 5.7862);
            }
            a->set_value(f32a, f32a.size());
        }
        {
            Float64 *f64 = new Float64("f64_array");
            Array *a = new Array("array", f64);
            delete f64;
            f64 = 0;
            a->append_dim(2, "f64_dim1");
            a->append_dim(2, "f64_dim2");
            a->append_dim(100, "f64_dim3");
            dds->add_var(a);
            delete a;
            a = 0;

            a = dynamic_cast<Array *>(dds->var("f64_array"));
            if (!a) {
                delete dds;
                string err = "cast error for f64_array";
                throw BESError(err, 0, __FILE__, __LINE__);
            }

            vector<dods_float64> f64a;
            for (int i = 0; i < 400; i++) {
                f64a.push_back(i * 589.288);
            }
            a->set_value(f64a, f64a.size());
        }
        {
            Str *str = new Str("str_array");
            Array *a = new Array("array", str);
            delete str;
            str = 0;
            a->append_dim(2, "str_dim1");
            a->append_dim(3, "str_dim2");
            a->append_dim(6, "str_dim3");
            dds->add_var(a);
            delete a;
            a = 0;

            a = dynamic_cast<Array *>(dds->var("str_array"));
            if (!a) {
                delete dds;
                string err = "cast error for str_array";
                throw BESError(err, 0, __FILE__, __LINE__);
            }

            vector<string> stra;
            for (int i = 0; i < 2; i++) {
                for (int j = 0; j < 3; j++) {
                    for (int k = 0; k < 6; k++) {
                        ostringstream strm;
                        strm << "str_" << i << "." << j << "." << k;
                        stra.push_back(strm.str());
                    }
                }
            }
            a->set_value(stra, stra.size());
        }

        // transform the DataDDS into a netcdf file. The dhi only needs the
        // output stream and the post constraint. Test no constraints and
        // then some different constraints (1 var, 2 var)

        // The resulting netcdf file is streamed back. Write this file to a
        // test file locally
        BESDataHandlerInterface dhi;
        ofstream fstrm("./arrayT.nc", ios::out | ios::trunc);
        dhi.set_output_stream(&fstrm);
        dhi.data[POST_CONSTRAINT] = "";

        ConstraintEvaluator eval;
        send_data(dds, eval, dhi);

        fstrm.close();

        delete dds;
    }
    catch (BESError &e) {
        cerr << e.get_message() << endl;
        return 1;
    }

    return 0;
}

