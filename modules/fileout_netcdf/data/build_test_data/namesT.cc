// namesT.cc

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
#include <UInt32.h>
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
        Grid order("/order name/#with^chars&");

        {
            Int32 bt("i dim");
            Array a("i dim", &bt);
            a.append_dim(2, "i dim");
            vector<dods_int32> btv;
            btv.push_back(2);
            btv.push_back(4);
            a.set_value(btv, 2);
            order.add_var(&a, maps);
            dds->add_var(&a);
        }
        {
            Float32 bt("j!dim");
            Array a("j!dim", &bt);
            a.append_dim(3, "j!dim");
            vector<dods_float32> btv;
            btv.push_back(3.3);
            btv.push_back(6.6);
            btv.push_back(9.9);
            a.set_value(btv, 3);
            order.add_var(&a, maps);
        }
        {
            Int16 bt("/order name/#with^chars&");
            Array a("/order name/#with^chars&", &bt);
            a.append_dim(2, "i dim");
            a.append_dim(3, "j!dim");
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
        order.set_read_p(true);
        dds->add_var(&order);

        Structure s("my&struct");

        UInt32 ui32("ui(32)");
        ui32.set_value(105467);
        s.add_var(&ui32);

        Float32 f32("f[32]");
        f32.set_value(5.7866);
        s.add_var(&f32);

        dds->add_var(&s);

        build_dods_response(&dds, "./namesT.dods");

        delete dds;
    }
    catch (BESError &e) {
        cerr << e << endl;
        return 1;
    }

    return 0;
}

