// structT.cc

#include <cstdlib>
#include <fstream>
#include <iostream>

using std::ofstream;
using std::ios;
using std::cerr;
using std::endl;

#include <libdap/DataDDS.h>
#include <libdap/Structure.h>
#include <libdap/Array.h>
#include <libdap/Byte.h>
#include <libdap/Int16.h>
#include <libdap/Int32.h>
#include <libdap/UInt16.h>
#include <libdap/UInt32.h>
#include <libdap/Float32.h>
#include <libdap/Float64.h>
#include <libdap/Str.h>
#include <libdap/Error.h>

using namespace libdap;

#include <BESDataHandlerInterface.h>
#include <BESDataNames.h>
#include <BESDebug.h>

//#include "test_config.h"
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
        if (debug)
            BESDebug::SetUp("cerr,fonc");

        // build a DataDDS of simple types and set values for each of the
        // simple types.
        DDS *dds = new DDS(NULL, "virtual");

        Structure s("mystruct");

        Byte b("byte");
        b.set_value(28);
        s.add_var(&b);

        Int16 i16("i16");
        i16.set_value(-2048);
        s.add_var(&i16);

        Int32 i32("i32");
        i32.set_value(-105467);
        s.add_var(&i32);

        UInt16 ui16("ui16");
        ui16.set_value(2048);
        s.add_var(&ui16);

        UInt32 ui32("ui32");
        ui32.set_value(105467);
        s.add_var(&ui32);

        Float32 f32("f32");
        f32.set_value(5.7866);
        s.add_var(&f32);

        Float64 f64("f64");
        f64.set_value(10245.1234);
        s.add_var(&f64);

        Str str("str");
        str.set_value("This is a String Value");
        s.add_var(&str);

        dds->add_var(&s);

        build_dods_response(&dds, "./structT00.dods");

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

