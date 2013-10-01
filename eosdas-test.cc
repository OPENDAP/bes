
// This file is part of the hdf4 data handler for the OPeNDAP data server.

// Copyright (c) 2005 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
//
// This is free software; you can redistribute it and/or modify it under the
// terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 2.1 of the License, or (at your
// option) any later version.
// 
// This software is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
// License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this software; if not, write to the Free Software Foundation,
// Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

// (c) COPYRIGHT URI/MIT 2000
// Please read the full copyright statement in the file COPYRIGHT_URI
//
// Authors:
//      jhrg,jimg       James Gallagher (jgallagher@gso.uri.edu)

// Test the HDF-EOS attribute parser. 3/30/2000 jhrg

#include "config_hdf.h"

#include <cstdlib>
#include <sstream>
#include <iostream>
#include <string>

#include <GetOpt.h>

#define YYSTYPE char *

#include <DAS.h>
#include <parser.h>

#include "hdfeos.tab.hh"

#ifdef TRACE_NEW
#include "trace_new.h"
#endif

using namespace std;
using namespace libdap;
#include <BESLog.h>

extern int hdfeosparse(parser_arg *arg);      // defined in hdfeos.tab.c

void parser_driver(DAS & das);
void test_scanner();

int hdfeoslex();

extern int hdfeosdebug;
const char *prompt = "hdfeos-test: ";
const char *version = "$Revision$";

void usage(string name)
{
      cerr << "usage: " << name
        << " [-v] [-s] [-d] [-p] {< in-file > out-file}" << endl
        << " s: Test the DAS scanner." << endl
        << " p: Scan and parse from <in-file>; print to <out-file>." <<
        endl << " v: Print the version of das-test and exit." << endl <<
        " d: Print parser debugging information." <<endl;
}

int main(int argc, char *argv[])
{

    GetOpt getopt(argc, argv, "spvd");
    int option_char;
    bool parser_test = false;
    bool scanner_test = false;

    while ((option_char = getopt()) != EOF)
        switch (option_char) {
        case 'p':
            parser_test = true;
            break;
        case 's':
            scanner_test = true;
            break;
        case 'v':
            cerr << argv[0] << ": " << version << endl;
            exit(0);
        case 'd':
            hdfeosdebug = 1;
            break;
        case '?':
        default:
            usage(argv[0]);
            exit(1);
        }

    DAS das;

    if (!parser_test && !scanner_test) {
        usage(argv[0]);
        exit(1);
    }

    if (parser_test)
        parser_driver(das);

    if (scanner_test)
        test_scanner();

    return (0);
}

void test_scanner()
{
    int tok;

    cout << prompt << flush;    // first prompt
    while ((tok = hdfeoslex())) {
        switch (tok) {
        case GROUP:
            cout << "GROUP" << endl;
            break;
        case END_GROUP:
            cout << "END_GROUP" << endl;
            break;
        case OBJECT:
            cout << "OBJECT" << endl;
            break;
        case END_OBJECT:
            cout << "END_OBJECT" << endl;
            break;

        case STR:
            cout << "STR=" << hdfeoslval << endl;
            break;
        case INT:
            cout << "INT=" << hdfeoslval << endl;
            break;
        case FLOAT:
            cout << "FLOAT=" << hdfeoslval << endl;
            break;

        case '=':
            cout << "Equality" << endl;
            break;
        case '(':
            cout << "LParen" << endl;
            break;
        case ')':
            cout << "RParen" << endl;
            break;
        case ',':
            cout << "Comma" << endl;
            break;
        case ';':
            cout << "Semicolon" << endl;
            break;

        default:
            cout << "Error: Unrecognized input" << endl;
            break;
        }

        cout << prompt << flush;        // print prompt after output
    }
}

void parser_driver(DAS & das)
{
    AttrTable *at = das.get_table("test");
    if (!at)
        at = das.add_table("test", new AttrTable);

    parser_arg arg(at);
    if (hdfeosparse(&arg) != 0)
        (*BESLog::TheLog()) << "HDF-EOS parse error !" <<endl;

    das.print(stdout);
}
