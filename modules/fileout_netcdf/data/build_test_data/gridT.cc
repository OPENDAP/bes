// gridT.cc

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
#include <Grid.h>
#include <Structure.h>
#include <Int16.h>
#include <Int32.h>
#include <Float32.h>
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

        // build a DataDDS of simple types and set values for each of the
        // simple types.
        DDS *dds = new DDS(NULL, "virtual");
        Grid order("order");
        Grid shot("shot");
        Grid bears("bears");

        {
            Int32 bt("i");
            Array a("i", &bt);
            a.append_dim(2, "i");
            vector<dods_int32> btv;
            btv.push_back(2);
            btv.push_back(4);
            a.set_value(btv, 2);
            order.add_var(&a, maps);
            shot.add_var(&a, maps);
            bears.add_var(&a, maps);
            dds->add_var(&a);
        }
        {
            Float32 bt("j");
            Array a("j", &bt);
            a.append_dim(3, "j");
            vector<dods_float32> btv;
            btv.push_back(3.3);
            btv.push_back(6.6);
            btv.push_back(9.9);
            a.set_value(btv, 3);
            order.add_var(&a, maps);
            shot.add_var(&a, maps);
        }
        {
            Str bt("k");
            Array a("k", &bt);
            a.append_dim(4, "k");
            vector<string> btv;
            btv.push_back("str1");
            btv.push_back("str2");
            btv.push_back("str3");
            btv.push_back("str4");
            a.set_value(btv, 4);
            bears.add_var(&a, maps);
        }
        {
            Int16 bt("order");
            Array a("order", &bt);
            a.append_dim(2, "i");
            a.append_dim(3, "j");
            vector<dods_int16> btv;
            btv.push_back(4);
            btv.push_back(8);
            btv.push_back(12);
            btv.push_back(5);
            btv.push_back(10);
            btv.push_back(15);
            a.set_value(btv, 6);
            order.add_var(&a, libdap::array);
        }
        {
            Int32 bt("shot");
            Array a("shot", &bt);
            a.append_dim(2, "i");
            a.append_dim(3, "j");
            vector<dods_int32> btv;
            btv.push_back(6);
            btv.push_back(12);
            btv.push_back(18);
            btv.push_back(7);
            btv.push_back(14);
            btv.push_back(21);
            a.set_value(btv, 6);
            shot.add_var(&a, libdap::array);
        }
        {
            Int32 bt("bears");
            Array a("bears", &bt);
            a.append_dim(2, "i");
            a.append_dim(4, "k");
            vector<dods_int32> btv;
            btv.push_back(8);
            btv.push_back(16);
            btv.push_back(24);
            btv.push_back(32);
            btv.push_back(9);
            btv.push_back(18);
            btv.push_back(27);
            btv.push_back(36);
            a.set_value(btv, 8);
            bears.add_var(&a, libdap::array);
        }
        order.set_read_p(true);
        dds->add_var(&order);

        shot.set_read_p(true);
        Structure s("gstruct");
        s.add_var(&shot);
        dds->add_var(&s);

        bears.set_read_p(true);
        Structure ss("bstruct");
        ss.add_var(&bears);
        dds->add_var(&ss);

        build_dods_response(&dds, "./gridT.dods");

        delete dds;
    }
    catch (BESError &e) {
        cerr << e.get_message() << endl;
        return 1;
    }

    return 0;
}

