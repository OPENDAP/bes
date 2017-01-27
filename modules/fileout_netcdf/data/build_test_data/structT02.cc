// structT.cc

#include <cstdlib>
#include <fstream>
#include <iostream>

using std::ofstream;
using std::ios;
using std::cerr;
using std::endl;

#include <DataDDS.h>
#include <Structure.h>
#include <Array.h>
#include <Byte.h>
#include <Int16.h>
#include <Int32.h>
#include <UInt16.h>
#include <UInt32.h>
#include <Float32.h>
#include <Float64.h>
#include <Str.h>

using namespace libdap;

#include <BESDataHandlerInterface.h>
#include <BESDataNames.h>
#include <BESDebug.h>

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

        // nested with constraint
        DDS *dds = new DDS(NULL, "virtual");

        Structure s1("s1");
        Structure s2("s2");
        Structure s3("s3");

        Byte b("byte");
        b.set_value(28);
        s1.add_var(&b);

        Int16 i16("i16");
        i16.set_value(-2048);
        s2.add_var(&i16);

        Int32 i32("i32");
        i32.set_value(-105467);
        s3.add_var(&i32);

        UInt16 ui16("ui16");
        ui16.set_value(2048);
        s1.add_var(&ui16);

        UInt32 ui32("ui32");
        ui32.set_value(105467);
        s2.add_var(&ui32);

        Float32 f32("f32");
        f32.set_value(5.7866);
        s3.add_var(&f32);

        Float64 f64("f64");
        f64.set_value(10245.1234);
        s1.add_var(&f64);

        Str str("str");
        str.set_value("This is a String Value");
        s2.add_var(&str);

        s2.add_var(&s3);
        s1.add_var(&s2);

        dds->add_var(&s1);

        build_dods_response(&dds, "./structT02.dods");
        // apply CE "s1.ui16,s1.s2.str,s1.s2.s3.i32"

        delete dds;
    }
    catch (BESError &e) {
        cerr << e.get_message() << endl;
        return 1;
    }

    return 0;
}

