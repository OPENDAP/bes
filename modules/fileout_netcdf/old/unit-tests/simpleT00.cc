// simpleT.cc

#include <cstdlib>
#include <fstream>
#include <iostream>

using std::ofstream;
using std::ios;
using std::cerr;
using std::endl;

#include <libdap/DataDDS.h>
#include <libdap/Byte.h>
#include <libdap/Int16.h>
#include <libdap/Int32.h>
#include <libdap/UInt16.h>
#include <libdap/UInt32.h>
#include <libdap/Float32.h>
#include <libdap/Float64.h>
#include <libdap/Str.h>

using namespace ::libdap;

#include <BESDataHandlerInterface.h>
#include <BESDapResponseBuilder.h>
#include <BESDataNames.h>
#include <BESDebug.h>

#include "test_config.h"
#include "test_send_data.h"

int main(int argc, char **argv)
{
    bool debug = false;
    bool dods_response = false; // write either a .dods or a netcdf file
    if (argc > 1) {
        for (int i = 0; i < argc; i++) {
            string arg = argv[i];
            if (arg == "debug") {
                debug = true;
            }
            else if (arg == "dods") {
                dods_response = true;
            }
        }
    }

    try {
        if (debug) {
            BESDebug::SetUp("cerr,fonc");
        }

        // build a DataDDS of simple types and set values for each of the
        // simple types.
        DDS **dds, *pdds;
        pdds = new DataDDS(NULL, "virtual");
        *dds = pdds;

        Byte b("byte");
        b.set_value(28);
        pdds->add_var(&b);

        Int16 i16("i16");
        i16.set_value(-2048);
        pdds->add_var(&i16);

        Int32 i32("i32");
        i32.set_value(-105467);
        pdds->add_var(&i32);

        UInt16 ui16("ui16");
        ui16.set_value(2048);
        pdds->add_var(&ui16);

        UInt32 ui32("ui32");
        ui32.set_value(105467);
        pdds->add_var(&ui32);

        Float32 f32("f32");
        f32.set_value(5.7866);
        pdds->add_var(&f32);

        Float64 f64("f64");
        f64.set_value(10245.1234);
        pdds->add_var(&f64);

        Str s("str");
        s.set_value("This is a String Value");
        pdds->add_var(&s);

        // Hack this to write out a .dods file as well. The code can ol=nly produce one
        // of the two responses since calling serialize() now erases the 'local data.'
        // jhrg 11/28/15
        if (dods_response) {
            build_dods_response(dds, "./simpleT00.dods");
        }
        else {
            build_netcdf_file(dds, "./simpleT00.nc");
        }

        delete dds;
    }
    catch (BESError &e) {
        cerr << e.get_message() << endl;
        return 1;
    }

    return 0;
}

