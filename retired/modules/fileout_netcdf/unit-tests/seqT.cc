// arrayT.cc

#include <fstream>
#include <iostream>
#include <sstream>

using std::ofstream;
using std::ios;
using std::cerr;
using std::endl;
using std::ostringstream;

#include <libdap/DataDDS.h>
#include <libdap/Sequence.h>
#include <libdap/Int32.h>
#include <libdap/Str.h>
#include <libdap/ConstraintEvaluator.h>

using namespace libdap;

#include <BESDataHandlerInterface.h>
#include <BESDataNames.h>
#include <BESDebug.h>

#include "test_config.h"
#include "test_send_data.h"

class MySequence: public Sequence {
public:
    MySequence(const string &n, const string &d) :
            Sequence(n, d)
    {
    }
    MySequence(const MySequence &rhs) :
            Sequence(rhs)
    {
    }
    virtual ~MySequence()
    {
    }

    MySequence &operator=(const MySequence &rhs)
    {
        if (this == &rhs)
            return *this;

        dynamic_cast<Sequence &>(*this) = rhs; // run Sequence assignment

        return *this;
    }
    virtual BaseType *ptr_duplicate()
    {
        return new MySequence(*this);
    }
    virtual bool read()
    {
        set_read_p(true);
        return true;
    }
};

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
        MySequence s("people", "");
        Str name("name");
        s.add_var(&name);
        Int32 age("age");
        s.add_var(&age);
        dds->add_var(&s);

        MySequence *sp = dynamic_cast<MySequence *>(dds->var("people"));
        BaseTypeRow *row1 = new BaseTypeRow;
        Str *pcw = new Str("name");
        pcw->set_value("Patrick West");
        row1->push_back(pcw);
        Int32 *pcw_age = new Int32("age");
        pcw_age->set_value(41);
        row1->push_back(pcw_age);

        BaseTypeRow *row2 = new BaseTypeRow;
        Str *crw = new Str("name");
        crw->set_value("Christopher West");
        row2->push_back(crw);
        Int32 *crw_age = new Int32("age");
        crw_age->set_value(10);
        row2->push_back(crw_age);

        SequenceValues values;
        values.push_back(row1);
        values.push_back(row2);

        sp->set_value(values);
        sp->set_read_p(true);

        // transform the DataDDS into a netcdf file. The dhi only needs the
        // output stream and the post constraint. Test no constraints and
        // then some different constraints (1 var, 2 var)

        // The resulting netcdf file is streamed back. Write this file to a
        // test file locally

        BESDataHandlerInterface dhi;
        ofstream fstrm("./seqT.nc", ios::out | ios::trunc);
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

