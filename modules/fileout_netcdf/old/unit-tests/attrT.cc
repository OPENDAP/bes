// attrT.cc

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
#include <Structure.h>
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

        // build a DataDDS of simple types and set values for each of the
        // simple types.
        DataDDS *dds = new DataDDS(NULL, "virtual");

        Byte b("byte");
        b.set_value(28);
        AttrTable &attrs = b.get_attr_table();
        for (unsigned char c = 65; c < 75; c++) {
            ostringstream strm;
            strm << (int) c;
            attrs.append_attr("byteattr", "Byte", strm.str());
        }
        for (short s = -50; s <= 50;) {
            ostringstream strm;
            strm << (int) s;
            attrs.append_attr("int16attr", "Int16", strm.str());
            s += 10;
        }
        for (unsigned short us = 1000; us <= 10000;) {
            ostringstream strm;
            strm << (unsigned int) us;
            attrs.append_attr("ui16attr", "UInt16", strm.str());
            us += 1000;
        }
        for (int i = -100000; i <= 100000;) {
            ostringstream strm;
            strm << (int) i;
            attrs.append_attr("int32attr", "Int32", strm.str());
            i += 20000;
        }
        for (unsigned int i = 0; i <= 1000000;) {
            ostringstream strm;
            strm << (unsigned int) i;
            attrs.append_attr("uint32attr", "UInt32", strm.str());
            i += 100000;
        }
        for (float f = 0.15; f <= 50.00;) {
            ostringstream strm;
            strm << (float) f;
            attrs.append_attr("floatattr", "Float32", strm.str());
            f += 5.66;
        }
        for (double d = 100.155; d <= 10000;) {
            ostringstream strm;
            strm << (double) d;
            attrs.append_attr("doubleattr", "Float64", strm.str());
            d += 876.268;
        }
        {
            ostringstream strm;
            strm << (int) 77;
            attrs.append_attr("singlebyteattr", "Byte", strm.str());
        }
        {
            attrs.append_attr("singlestringattr", "String", "String Value");
        }
        {
            AttrTable *table = attrs.append_container("container");
            table->append_attr("containerattr", "String", "Container Attribute Value");
            AttrTable *subtable = table->append_container("subcontainer");
            subtable->append_attr("subcontainerattr", "String", "Sub Container Attribute Value");
        }
        dds->add_var(&b);

        Structure s("mystruct");
        AttrTable &sattrs = s.get_attr_table();
        sattrs.append_attr("mystructattr1", "String", "mystructattr1val");
        sattrs.append_attr("mystructattr2", "String", "mystructattr2val");
        sattrs.append_attr("mystructattr3", "String", "mystructattr3val");

        Float32 f32("f32");
        f32.set_value(5.7866);
        s.add_var(&f32);

        Float64 f64("f64");
        f64.set_value(10245.1234);
        AttrTable &f64attrs = f64.get_attr_table();
        f64attrs.append_attr("f64attr1", "String", "f64attr1val");
        s.add_var(&f64);

        Str str("str");
        str.set_value("This is a String Value");
        AttrTable &strattrs = str.get_attr_table();
        strattrs.append_attr("strattr1", "String", "strattr1val");
        strattrs.append_attr("strattr2", "String", "strattr2val");
        s.add_var(&str);

        Structure ss("mysubstructure");
        AttrTable &ssattrs = ss.get_attr_table();
        ssattrs.append_attr("ssattr1", "String", "ssattr1val");

        Int16 i16("i16");
        i16.set_value(-2048);
        AttrTable &i16attrs = i16.get_attr_table();
        i16attrs.append_attr("i16attr1", "String", "i16attr1val");
        i16attrs.append_attr("i16attr2", "String", "i16attr2val");
        i16attrs.append_attr("i16attr3", "String", "i16attr3val");
        i16attrs.append_attr("i16attr4", "String", "i16attr4val");
        i16attrs.append_attr("i16attr5", "String", "i16attr5val");
        ss.add_var(&i16);

        Int32 i32("i32");
        i32.set_value(-105467);
        ss.add_var(&i32);

        s.add_var(&ss);

        dds->add_var(&s);
        AttrTable &gattrs = dds->get_attr_table();
        gattrs.append_attr("title", "String", "Attribute Test DDS");
        gattrs.append_attr("contact", "String", "Patrick West");
        gattrs.append_attr("contact_email", "String", "opendap-tech@opendap.org");

        // transform the DataDDS into a netcdf file. The dhi only needs the
        // output stream and the post constraint. Test no constraints and
        // then some different constraints (1 var, 2 var)

        // The resulting netcdf file is streamed back. Write this file to a
        // test file locally
        BESDataHandlerInterface dhi;
        ofstream fstrm("./attrT.nc", ios::out | ios::trunc);
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

