// simpleT.cc

#include <cstdlib>
#include <fstream>
#include <iostream>

using std::ofstream;
using std::ios;
using std::cerr;
using std::endl;

#include <DataDDS.h>
#include <Byte.h>
#include <Int16.h>
#include <Int32.h>
#include <UInt16.h>
#include <UInt32.h>
#include <Float32.h>
#include <Float64.h>
#include <Str.h>
#include <Error.h>

using namespace libdap;

#include <BESDataHandlerInterface.h>
#include <BESDapResponseBuilder.h>
#include <BESDataNames.h>
#include <BESDebug.h>

//#include "test_config.h"
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
        DDS *dds = new DDS(NULL, "virtual");

        Byte b("byte");
        b.set_value(28);
        dds->add_var(&b);

        Int16 i16("i16");
        i16.set_value(-2048);
        dds->add_var(&i16);

        Int32 i32("i32");
        i32.set_value(-105467);
        dds->add_var(&i32);

        UInt16 ui16("ui16");
        ui16.set_value(2048);
        dds->add_var(&ui16);

        UInt32 ui32("ui32");
        ui32.set_value(105467);
        dds->add_var(&ui32);

        Float32 f32("f32");
        f32.set_value(5.7866);
        dds->add_var(&f32);

        Float64 f64("f64");
        f64.set_value(10245.1234);
        dds->add_var(&f64);

        Str s("str");
        s.set_value("This is a String Value");
        dds->add_var(&s);

        build_dods_response(&dds, "./simpleT00.dods");

        delete dds;
    }
    catch (BESError &e) {
        cerr << e.get_message() << endl;
        return 1;
    }
    catch (Error &e) {
        cerr << e.get_error_message() << endl;
        return 1;
    }
    catch (std::exception &e) {
        cerr << e.what() << endl;
        return 1;
    }

    return 0;
}

