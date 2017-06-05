#include <iostream>
#include <fstream>

using std::cout;
using std::cerr;
using std::endl;
using std::ofstream;

#include <Connect.h>
#include <DataDDS.h>
#include <Response.h>
#include <Sequence.h>
#include <Error.h>
#include <DataDDS.h>
#include <DDS.h>
#include <Structure.h>
#include <Sequence.h>
#include <ConstraintEvaluator.h>

using namespace libdap;

#include <BESDataHandlerInterface.h>
#include <BESDataNames.h>
#include <BESDebug.h>
#include <BESInternalError.h>

#include "test_config.h"
#include "test_send_data.h"

#include "ReadTypeFactory.h"

void set_sequence_read(Sequence *s);

void set_structure_read(Structure *s)
{
    s->set_read_p(true);
    s->set_send_p(true);
    Constructor::Vars_iter i = s->var_begin();
    Constructor::Vars_iter e = s->var_end();
    for (; i != e; i++) {
        BaseType *v = *i;
        if (v->type() == dods_sequence_c) {
            Sequence *new_s = dynamic_cast<Sequence *>(v);
            if (new_s)
                set_sequence_read(new_s);
            else
                throw BESInternalError("Expected a Sequence", __FILE__, __LINE__);
        }
        else if (v->type() == dods_structure_c) {
            Structure *new_s = dynamic_cast<Structure *>(v);
            if (new_s)
                set_structure_read(new_s);
            else
                throw BESInternalError("Expected a Structure", __FILE__, __LINE__);
        }
        else {
            v->set_read_p(true);
            v->set_send_p(true);
        }
    }
}

void set_sequence_read(Sequence *s)
{
    s->set_read_p(true);
    s->set_send_p(true);
    s->reset_row_number();
    Constructor::Vars_iter i = s->var_begin();
    Constructor::Vars_iter e = s->var_end();
    for (; i != e; i++) {
        BaseType *v = *i;
        if (v->type() == dods_sequence_c) {
            Sequence *new_s = dynamic_cast<Sequence *>(v);
            if (new_s)
                set_sequence_read(new_s);
            else
                throw BESInternalError("Expected a Sequence", __FILE__, __LINE__);
        }
        else if (v->type() == dods_structure_c) {
            Structure *new_s = dynamic_cast<Structure *>(v);
            if (new_s)
                set_structure_read(new_s);
            else
                throw BESInternalError("Expected a Structure", __FILE__, __LINE__);
        }
        else {
            v->set_read_p(true);
            v->set_send_p(true);
        }
    }
}

static void set_read(DDS *dds)
{
    for (DDS::Vars_iter i = dds->var_begin(); i != dds->var_end(); i++) {
        BaseType *v = *i;
        if (v->type() == dods_sequence_c) {
            Sequence *s = dynamic_cast<Sequence *>(v);
            if (s)
                set_sequence_read(s);
            else
                throw BESInternalError("Expected a Sequence", __FILE__, __LINE__);
        }
        else if (v->type() == dods_structure_c) {
            Structure *s = dynamic_cast<Structure *>(v);
            if (s)
                set_structure_read(s);
            else
                throw BESInternalError("Expected a Structure", __FILE__, __LINE__);
        }
        else {
            v->set_read_p(true);
            v->set_send_p(true);
        }
    }
}

static void print_data(DDS *dds, bool print_rows)
{
    cout << "The data:" << endl;

    for (DDS::Vars_iter i = dds->var_begin(); i != dds->var_end(); i++) {
        BaseType *v = *i;
        if (print_rows && (*i)->type() == dods_sequence_c)
            dynamic_cast<Sequence &>(**i).print_val_by_rows(cout);
        else
            v->print_val(cout);
    }

    cout << endl << flush;
}

int main(int argc, char **argv)
{
    bool debug = false;
    string file;
    if (argc > 1) {
        for (int i = 0; i < argc; i++) {
            string arg = argv[i];
            if (arg == "debug") {
                debug = true;
            }
            else {
                file = arg;
            }
        }
    }
    if (file.empty()) {
        cout << "Must specify a file to load" << endl;
        return 1;
    }

#if 0
    string bes_conf = (string) "BES_CONF=" + TEST_BUILD_DIR + "/bes.conf";
    putenv((char *) bes_conf.c_str());
#endif
    string fpath = (string) TEST_SRC_DIR + "/data/" + file;
    string opath = file + ".nc";

    Connect *url = 0;
    Response *r = 0;
    ReadTypeFactory factory;
    DataDDS *dds = new DataDDS(&factory);
    try {
        if (debug) BESDebug::SetUp("cerr,fonc");

        url = new Connect(fpath);
        r = new Response(fopen(fpath.c_str(), "r"), 0);

        if (!r->get_stream()) {
            cout << "Failed to create stream for " << fpath << endl;
            return 1;
        }

        url->read_data_no_mime(*dds, r);

        if (debug) print_data(dds, false);
    }
    catch (Error & e) {
        cout << e.get_error_message() << endl;
        if (r) delete r;
        if (url) delete url;
        return 1;
    }
    catch (BESError &e) {
        cout << e.get_message() << endl;
        return 1;
    }

    delete r; r = 0;
    delete url; url = 0;

    try {
	dds->tag_nested_sequences();
	if (debug) dds->print(cerr);
	set_read(dds);
	if (debug) cerr << *dds << endl;

        // transform the DataDDS into a netcdf file. The dhi only needs the
        // output stream and the post constraint. Test no constraints and
        // then some different constraints (1 var, 2 var)

        // The resulting netcdf file is streamed back. Write this file to a
        // test file locally
        BESDataHandlerInterface dhi;
        ofstream fstrm(opath.c_str(), ios::out | ios::trunc);
        dhi.set_output_stream(&fstrm);
        dhi.data[POST_CONSTRAINT] = "";

        ConstraintEvaluator eval;
        send_data(dds, eval, dhi);

        fstrm.close();

        delete dds;
    }
    catch (BESError &e) {
        cout << e.get_message() << endl;
        return 1;
    }

    return 0;
}

