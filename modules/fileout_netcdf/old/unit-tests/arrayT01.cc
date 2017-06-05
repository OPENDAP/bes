// arrayT01.cc

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

using std::ofstream;
using std::ios;
using std::cerr;
using std::endl;
using std::ostringstream;

#include <DataDDS.h>
#include <Array.h>
#include <Byte.h>
#include <Int16.h>
#include <Int32.h>
#include <UInt16.h>
#include <UInt32.h>
#include <Float32.h>
#include <Float64.h>
#include <Str.h>

using namespace ::libdap;

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

        // build a DataDDS of simple type arrays with shared dimensions
        DataDDS *dds = new DataDDS(NULL, "virtual");
        {
            Byte *b = new Byte("byte_array");
            Array *a = new Array("array", b);
            delete b;
            b = 0;
            a->append_dim(2, "dim1");
            a->append_dim(5, "dim2");
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
            a->append_dim(2, "dim1");
            a->append_dim(5, "dim2");
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
            for (dods_int16 i = 0; i < 10; i++) {
                i16a.push_back(i * (-16));
            }
            a->set_value(i16a, i16a.size());
        }
        {
            Int32 *i32 = new Int32("i32_array");
            Array *a = new Array("array", i32);
            delete i32;
            i32 = 0;
            a->append_dim(2, "dim1");
            a->append_dim(5, "dim2");
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
            for (dods_int32 i = 0; i < 10; i++) {
                i32a.push_back(i * (-512));
            }
            a->set_value(i32a, i32a.size());
        }

        // transform the DataDDS into a netcdf file. The dhi only needs the
        // output stream and the post constraint. Test no constraints and
        // then some different constraints (1 var, 2 var)

        // The resulting netcdf file is streamed back. Write this file to a
        // test file locally
        BESDataHandlerInterface dhi;
        ofstream fstrm("./arrayT01.nc", ios::out | ios::trunc);
        dhi.set_output_stream(&fstrm);
        dhi.data[POST_CONSTRAINT] = "";

        ConstraintEvaluator eval;
        send_data(dds, eval, dhi);

        fstrm.close();

        // deleting the response object deletes the DataDDS
        delete dds;
    }
    catch (BESError &e) {
        cerr << e.get_message() << endl;
        return 1;
    }

    return 0;
}

