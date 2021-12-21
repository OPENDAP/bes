
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2002,2003 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

// (c) COPYRIGHT URI/MIT 1999
// Please read the full copyright statement in the file COPYRIGHT_URI.
//
// Authors:
//      jhrg,jimg       James Gallagher <jgallagher@gso.uri.edu>

// Test the gse scanner and parser.
//
// 1/17/99 jhrg

#include "config.h"

static char rcsid[] not_used =
    {"$Id: gse-test.cc 18315 2008-03-03 20:14:44Z jimg $"
    };

#include <cstdlib>
#include <cstdio>
#include <string>

#include <GetOpt.h>

#include <libdap/dods-datatypes.h>
#include <libdap/BaseType.h>
#include <libdap/Grid.h>
#include <libdap/DDS.h>
#include "GSEClause.h"
#include <libdap/parser.h>
#include "gse.tab.hh"
#include <libdap/debug.h>

#define YY_BUFFER_STATE (void *)

void test_gse_scanner(const char *str);
void test_gse_scanner(bool show_prompt);
void test_parser(const string &dds_file);

int gse_lex();   // gse_lex() uses the global gse_exprlval
int gse_parse(void *arg);
int gse_restart(FILE *in);

// Glue routines declared in expr.lex
void gse_switch_to_buffer(void *new_buffer);
void gse_delete_buffer(void * buffer);
void *gse_string(const char *yy_str);

extern int gse_debug;


const string version = "$Revision: 18315 $";
const string prompt = "gse-test: ";
const string options = "sS:p:dv";
const string usage = "gse-test [-s [-S string] -d -v [-p dds file]\n\
                     Test the grid selections expression evaluation software.\n\
                     Options:\n\
                     -s: Feed the input stream directly into the expression scanner, does\n\
                     not parse.\n\
                     -S: <string> Scan the string as if it was standard input.\n\
                     -p: <dds file> parse stdin using the grid defined in the DDS file.\n\
                     -d: Turn on expression parser debugging.\n\
                     -v: Print the version of expr-test";

int
main(int argc, char *argv[])
{
    GetOpt getopt(argc, argv, options.c_str());
    int option_char;
    bool scan_gse = false, scan_gse_string = false;
    bool test_parse = false;
    string constraint = "";
    string dds_file;
    // process options

    while ((option_char = getopt()) != EOF)
        switch (option_char) {
        case 'd':
            gse_debug = true;
            break;
        case 's':
            scan_gse = true;
            break;
        case 'S':
            scan_gse_string = true;
            scan_gse = true;
            constraint = getopt.optarg;
            break;
        case 'p':
            test_parse = true;
            dds_file = getopt.optarg;
            break;
        case 'v':
            fprintf(stderr, "%s: %s\n", argv[0], version.c_str()) ;
            exit(0);
        case '?':
        default:
            fprintf(stderr, "%s\n", usage.c_str()) ;
            exit(1);
            break;
        }

    if (!scan_gse && !test_parse) {
        fprintf(stderr, "%s\n", usage.c_str()) ;
        exit(1);
    }

    // run selected tests

    if (scan_gse) {
        if (scan_gse_string)
            test_gse_scanner(constraint.c_str());
        else
            test_gse_scanner(true);
        exit(0);
    }

    if (test_parse) {
        test_parser(dds_file);
        exit(0);
    }
}

// Instead of reading the tokens from stdin, read them from a string.

void
test_gse_scanner(const char *str)
{
    gse_restart(0);
    void *buffer = gse_string(str);
    gse_switch_to_buffer(buffer);

    test_gse_scanner(false);

    gse_delete_buffer(buffer);
}

void
test_gse_scanner(bool show_prompt)
{
    if (show_prompt)
        fprintf(stdout, "%s", prompt.c_str()) ;  // first prompt

    int tok;
    while ((tok = gse_lex())) {
        switch (tok) {
        case SCAN_WORD:
            fprintf(stdout, "WORD: %s\n", gse_lval.id) ;
            break;
        case SCAN_INT:
            fprintf(stdout, "INT: %d\n", (int)gse_lval.val) ;
            break;
        case SCAN_FLOAT:
            fprintf(stdout, "FLOAT: %f\n", gse_lval.val) ;
            break;
        case SCAN_EQUAL:
            fprintf(stdout, "EQUAL: %d\n", gse_lval.op) ;
            break;
        case SCAN_NOT_EQUAL:
            fprintf(stdout, "NOT_EQUAL: %d\n", gse_lval.op) ;
            break;
        case SCAN_GREATER:
            fprintf(stdout, "GREATER: %d\n", gse_lval.op) ;
            break;
        case SCAN_GREATER_EQL:
            fprintf(stdout, "GREATER_EQL: %d\n", gse_lval.op) ;
            break;
        case SCAN_LESS:
            fprintf(stdout, "LESS: %d\n", gse_lval.op) ;
            break;
        case SCAN_LESS_EQL:
            fprintf(stdout, "LESS_EQL: %d\n", gse_lval.op) ;
            break;
        default:
            fprintf(stdout, "Error: Unrecognized input\n") ;
        }
        fprintf(stdout, "%s", prompt.c_str()) ;   // print prompt after output
    }
}

// Create and return a map vector for use in testing the GSE parser. The map
// is monotonically increasing in T.

template<class T>
static T*
new_map(int size)
{
    T *t = new T[size];
    srand(time(0));
    double offset = rand() / (double)((1 << 15) - 1);
    double space = rand() / (double)((1 << 15) - 1);

    offset *= 10.0;
    space *= 10.0;

    for (int i = 0; i < size; ++i)
        t[i] = (int)(i * space  + offset);

    return t;
}

void
test_parser(const string &dds_file)
{
    // Read the grid

    Grid *grid = 0;
    DDS dds;
    dds.parse(dds_file);
    // Add 'grid' to loop test? jhrg
    for (DDS::Vars_iter p = dds.var_begin(); p != dds.var_end(); p++) {
        if ((*p)->type() == dods_grid_c)
            grid = dynamic_cast<Grid *>((*p));
    }
    if (!grid) {
        fprintf(stderr,
                "Could not find a grid variable in the DDS, exiting.\n") ;
        exit(1);
    }

    // Load the grid map indices. The GSEClause object does not process the
    // Array values, but does need valid map data. Assume that the maps are
    // all either Float64 or Int32 maps.

    for (Grid::Map_iter p = grid->map_begin(); p != grid->map_end(); p++) {
        Array *map = dynamic_cast<Array *>((*p));
        // Can safely assume that maps are one-dimensional Arrays.
        Array::Dim_iter diter = map->dim_begin() ;
        int size = map->dimension_size(diter);
        switch (map->var()->type()) {
        case dods_int16_c: {
                dods_int16 *vec = new_map<dods_int16>(size);
                map->val2buf(vec);
                break;
            }
        case dods_int32_c: {
                dods_int32 *vec = new_map<dods_int32>(size);
                map->val2buf(vec);
                break;
            }
        case dods_float32_c: {
                dods_float32 *vec = new_map<dods_float32>(size);
                map->val2buf(vec);
                break;
            }
        case dods_float64_c: {
                dods_float64 *vec = new_map<dods_float64>(size);
                map->val2buf(vec);
                break;
            }
        default:
            fprintf(stderr, "Invalid map vector type in grid, exiting.\n") ;
            exit(1);
        }
    }

    dds.print(stdout);
    for (Grid::Map_iter p = grid->map_begin(); p != grid->map_end(); p++)
        (*p)->print_val(stdout);

    // Parse the GSE and mark the selection in the Grid.

    gse_restart(stdin);

    fprintf(stdout, "%s", prompt.c_str()) ;
    fflush(stdout) ;

    gse_arg *arg = new gse_arg(grid);
    bool status = false ;
    try {
        status = gse_parse((void *)arg) == 0;
    }
    catch (Error &e) {
        e.display_message();
        exit(1);
    }

    if (status) {
        fprintf(stdout, "Input parsed\n") ;
        GSEClause *gsec = arg->get_gsec();
        fprintf(stdout, "Start: %d Stop: %d\n", gsec->get_start(),
                gsec->get_stop()) ;
    }
    else
        fprintf(stdout, "Input did not parse\n") ;
}

