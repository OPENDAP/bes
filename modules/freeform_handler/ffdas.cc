// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of ff_handler a FreeForm API handler for the OPeNDAP
// DAP2 data server.

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
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

// (c) COPYRIGHT URI/MIT 1997-98
// Please read the full copyright statement in the file COPYRIGHT.
//
// Authors: reza (Reza Nekovei)

// This file contains functions which read the variables and their attributes
// from a netcdf file and build the in-memeory DAS. These functions form the
// core of the server-side software necessary to extract the DAS from a
// netcdf data file.
//
// ReZa 6/23/97

#include "config_ff.h"

#include <cstdio>
#include <cstring>

#include <iostream>
#include <string>

#include <util.h>
#include <DAS.h>
#include <Error.h>
#include <InternalErr.h>

#include "FreeFormCPP.h"
#include "FFRequestHandler.h"
#include "util_ff.h"
#include "freeform.h"

// Added 7/30/08 as part of the fix for ticket #1163. jhrg
#define ATTR_STRING_QUOTE_FIX

// Read header information and populate an AttrTable with the information.

static void header_to_attributes(AttrTable *at, DATA_BIN_PTR dbin)
{
    PROCESS_INFO_LIST pinfo_list = NULL;
    PROCESS_INFO_PTR hd_pinfo = NULL;

    char text[256];

    int error = db_ask(dbin, DBASK_PROCESS_INFO, FFF_INPUT | FFF_FILE | FFF_HEADER, &pinfo_list);
    // A general error indicates that the dataset does not have a header.
    // Anything else is bad news. 1/29/2001 jhrg
    if (error) {
        if (error == ERR_GENERAL) {
            return;
        }
        else {
            string msg = "Cannot get attribute values. FreeForm error code: ";
            append_long_to_string((long) error, 10, msg);
            throw Error(msg);
        }
    }

    pinfo_list = dll_first(pinfo_list);
    hd_pinfo = ((PROCESS_INFO_PTR) (pinfo_list)->data.u.pi);
    if (hd_pinfo) {
        VARIABLE_LIST vlist = NULL;
        VARIABLE_PTR var = NULL;

        vlist = FFV_FIRST_VARIABLE(PINFO_FORMAT(hd_pinfo));
        var = ((VARIABLE_PTR) (vlist)->data.u.var);

        while (var) {
            if (IS_EOL(var)) {
                vlist = (vlist)->next;
                var = ((VARIABLE_PTR) (vlist)->data.u.var);

                continue;
            }

            switch (FFV_DATA_TYPE(var)) {
            case FFV_TEXT:
                nt_ask(dbin, FFF_FILE | FFF_HEADER, var->name, FFV_TEXT, text);
#ifndef ATTR_STRING_QUOTE_FIX
                // Multiple word strings must be quoted.
                if (strpbrk(text, " \r\t")) { // Any whitespace?
                    string quoted_text;
                    quoted_text = (string)"\"" + text + "\"";
                    at->append_attr(var->name, "STRING",
                            quoted_text.c_str());
                }
                else {
                    at->append_attr(var->name, "STRING", text);
                }
#else
                // The new version of libdap provides the quotes when
                // printing the DAS. The DDX does not need quotes. jhrg
                // 7/30/08
                at->append_attr(var->name, "STRING", text);
#endif
                break;

            case FFV_INT8:
                unsigned char int8;
                nt_ask(dbin, FFF_FILE | FFF_HEADER, var->name, FFV_INT8, &int8);
                snprintf(text, sizeof(text), "%d", int8);
                at->append_attr(var->name, "BYTE", text);
                break;

            case FFV_INT16:
                short int16;
                nt_ask(dbin, FFF_FILE | FFF_HEADER, var->name, FFV_INT16, &int16);
                snprintf(text, sizeof(text), "%d", int16);
                at->append_attr(var->name, "INT16", text);
                break;

            case FFV_INT32:
                int int32;
                nt_ask(dbin, FFF_FILE | FFF_HEADER, var->name, FFV_INT32, &int32);
                snprintf(text, sizeof(text), "%d", int32);
                at->append_attr(var->name, "INT32", text);
                break;

            case FFV_INT64:
                long int64;
                nt_ask(dbin, FFF_FILE | FFF_HEADER, var->name, FFV_INT64, &int64);
                snprintf(text, sizeof(text), "%ld", int64);
                at->append_attr(var->name, "INT32", text);
                break;

            case FFV_UINT8:
                unsigned char uint8;
                nt_ask(dbin, FFF_FILE | FFF_HEADER, var->name, FFV_UINT8, &uint8);
                snprintf(text, sizeof(text), "%d", uint8);
                at->append_attr(var->name, "BYTE", text);
                break;

            case FFV_UINT16:
                unsigned short uint16;
                nt_ask(dbin, FFF_FILE | FFF_HEADER, var->name, FFV_UINT16, &uint16);
                snprintf(text, sizeof(text), "%d", uint16);
                at->append_attr(var->name, "UINT16", text);
                break;

            case FFV_UINT32:
                unsigned int uint32;
                nt_ask(dbin, FFF_FILE | FFF_HEADER, var->name, FFV_UINT32, &uint32);
                snprintf(text, sizeof(text), "%u", uint32);
                at->append_attr(var->name, "UINT32", text);
                break;

            case FFV_UINT64:
                unsigned long uint64;
                nt_ask(dbin, FFF_FILE | FFF_HEADER, var->name, FFV_UINT64, &uint64);
                snprintf(text, sizeof(text), "%lu", uint64);
                at->append_attr(var->name, "UINT32", text);
                break;

            case FFV_FLOAT32:
                float float32;
                nt_ask(dbin, FFF_FILE | FFF_HEADER, var->name, FFV_FLOAT32, &float32);
                snprintf(text, sizeof(text), "%f", float32);
                at->append_attr(var->name, "FLOAT32", text);
                break;

            case FFV_FLOAT64:
                double float64;
                nt_ask(dbin, FFF_FILE | FFF_HEADER, var->name, FFV_FLOAT64, &float64);
                snprintf(text, sizeof(text), "%f", float64);
                at->append_attr(var->name, "FLOAT64", text);
                break;

            case FFV_ENOTE:
                double enote;
                nt_ask(dbin, FFF_FILE | FFF_HEADER, var->name, FFV_ENOTE, &enote);
                snprintf(text, sizeof(text), "%e", enote);
                at->append_attr(var->name, "FLOAT64", text);
                break;

            default:
                throw InternalErr(__FILE__, __LINE__, "Unknown FreeForm type!");
            }
            vlist = (vlist)->next;
            var = ((VARIABLE_PTR) (vlist)->data.u.var);
        }
    }
    if (pinfo_list)
        ff_destroy_process_info_list(pinfo_list);
 
}

/** Read the attributes and store their names and values in the attribute
 table.
 @return false if an error was detected reading from the freeform format
 file, true otherwise. */
void read_attributes(string filename, AttrTable *at)
{
    int error = 0;
    FF_BUFSIZE_PTR bufsize = NULL;
    DATA_BIN_PTR dbin = NULL;
    FF_STD_ARGS_PTR SetUps = NULL;

    if (!file_exist(filename.c_str()))
        throw Error((string) "Could not open file " + path_to_filename(filename) + ".");

    // ff_create_std_args uses calloc so the struct's pointers are all initialized
    // to null.
    SetUps = ff_create_std_args();
    if (!SetUps)
        throw Error("ff_das: Insufficient memory");

    /** set the structure values to create the FreeForm DB**/
    SetUps->user.is_stdin_redirected = 0;

    // Use const_cast because FF only copies the referenced data; the older
    // version of this handler allocated memory, copied values, leaked the
    // memory...
    SetUps->input_file = const_cast<char*>(filename.c_str());

    string iff = "";
    if (FFRequestHandler::get_RSS_format_support()) {
        iff = find_ancillary_rss_formats(filename);
        SetUps->input_format_file = const_cast<char*>(iff.c_str());
    }
    // Regex support
    if (FFRequestHandler::get_Regex_format_support()) {
        iff = get_Regex_format_file(filename);
        if (!iff.empty())
            SetUps->input_format_file = const_cast<char*>(iff.c_str());
    }

    SetUps->output_file = NULL;

    char Msgt[255];
    error = SetDodsDB(SetUps, &dbin, Msgt);
    if (error && error < ERR_WARNING_ONLY) {
        if (dbin)
            db_destroy(dbin);
        ff_destroy_std_args(SetUps);
        throw Error(Msgt);
    }

    ff_destroy_std_args(SetUps);

    try {
        error = db_ask(dbin, DBASK_FORMAT_SUMMARY, FFF_INPUT, &bufsize);
        if (error) {
            string msg = "Cannot get Format Summary. FreeForm error code: ";
            append_long_to_string((long) error, 10, msg);
            throw Error(msg);
        }

#ifndef ATTR_STRING_QUOTE_FIX
        at->append_attr("Server", "STRING",
                "\"DODS FreeFrom based on FFND release " + FFND_LIB_VER + "\"");
#else
        at->append_attr("Server", "STRING", string("DODS FreeFrom based on FFND release ") + FFND_LIB_VER);
#endif

        header_to_attributes(at, dbin); // throws Error
    }
    catch (...) {
        if (bufsize)
            ff_destroy_bufsize(bufsize);
        if (dbin)
            db_destroy(dbin);

        throw;
    }

    ff_destroy_bufsize(bufsize);
    db_destroy(dbin);
}

static void add_variable_containers(DAS &das, const string &filename) throw (Error)
{
    if (!file_exist(filename.c_str()))
        throw Error(string("ff_dds: Could not open file ") + path_to_filename(filename) + string("."));

    // Setup the DB access.
    FF_STD_ARGS_PTR SetUps = ff_create_std_args();
    if (!SetUps)
        throw Error("Insufficient memory");

    SetUps->user.is_stdin_redirected = 0;

    SetUps->input_file = const_cast<char*>(filename.c_str());

    string iff = "";
    if (FFRequestHandler::get_RSS_format_support()) {
        iff = find_ancillary_rss_formats(filename);
        SetUps->input_format_file = const_cast<char*>(iff.c_str());
    }

    // Regex support
    if (FFRequestHandler::get_Regex_format_support()) {
        iff = get_Regex_format_file(filename);
        if (!iff.empty())
            SetUps->input_format_file = const_cast<char*>(iff.c_str());
    }

    SetUps->output_file = NULL;

    // Set the structure values to create the FreeForm DB
    char Msgt[255];
    DATA_BIN_PTR dbin = NULL;
    int error = SetDodsDB(SetUps, &dbin, Msgt);
    if (error && error < ERR_WARNING_ONLY) {
        if (dbin)
            db_destroy(dbin);
        ff_destroy_std_args(SetUps);
        string msg = string(Msgt) + " FreeForm error code: ";
        append_long_to_string((long) error, 10, msg);
        throw Error(msg);
    }

    ff_destroy_std_args(SetUps);

    // These are defined here so that they can be freed in the catch(...)
    // block below
    char **var_names_vector = NULL;
    PROCESS_INFO_LIST pinfo_list = NULL;
    char **dim_names_vector = NULL;

    try {
        // Get the names of all the variables.
        int num_names = 0;
        error = db_ask(dbin, DBASK_VAR_NAMES, FFF_INPUT | FFF_DATA, &num_names, &var_names_vector);
        if (error) {
            string msg = "Could not get varible list from the input file. FreeForm error code: ";
            append_long_to_string((long) error, 10, msg);
            throw Error(msg);
        }

        // I don't understand why this has to happen here, but maybe it's moved
        // outside the loop because FreeForm is designed so that the pinfo list
        // only needs to be accessed once and can be used when working with
        // any/all of the variables. 4/4/2002 jhrg
        error = db_ask(dbin, DBASK_PROCESS_INFO, FFF_INPUT | FFF_DATA, &pinfo_list);
        if (error) {
            string msg = "Could not get process info for the input file. FreeForm error code: ";
            append_long_to_string((long) error, 10, msg);
            throw Error(msg);
        }

        // For each variable, figure out what its name really is (arrays have
        // funny names).
        for (int i = 0; i < num_names; i++) {
            int num_dim_names = 0;
            error = db_ask(dbin, DBASK_ARRAY_DIM_NAMES, var_names_vector[i], &num_dim_names, &dim_names_vector);
            if (error) {
                string msg = "Could not get array dimension names for variable: " + string(var_names_vector[i])
                        + string(", FreeForm error code: ");
                append_long_to_string((long) error, 10, msg);
                throw Error(msg);
            }

            // Note: FreeForm array names are returned appended to their format
            // name with '::'.
            char *cp = NULL;
            if (num_dim_names == 0) // sequence names
                cp = var_names_vector[i];
            else {
                cp = strstr(var_names_vector[i], "::");
                // If cp is not null, advance past the "::"
                if (cp)
                    cp += 2;
            }

            // We need this to figure out if this variable is the/a EOL
            // variable. We read the pinfo_list just before starting this
            // for-loop.
            pinfo_list = dll_first(pinfo_list);
            PROCESS_INFO_PTR pinfo = ((PROCESS_INFO_PTR) (pinfo_list)->data.u.pi);
            FORMAT_PTR iformat = PINFO_FORMAT(pinfo);
            VARIABLE_PTR var = ff_find_variable(cp, iformat);

            // For some formats: Freefrom sends an extra EOL variable at the end of
            // the list. Add an attribute container for all the other variables.
            if (!IS_EOL(var))
                das.add_table(cp, new AttrTable);

            memFree(dim_names_vector, "**dim_names_vector");
            dim_names_vector = NULL;
        }
    }
    catch (...) {
        if (var_names_vector)
            memFree(var_names_vector, "**var_names_vector");
        if (pinfo_list)
            ff_destroy_process_info_list(pinfo_list);
        if (dim_names_vector)
            memFree(dim_names_vector, "**dim_names_vector");
        if (dbin)
            db_destroy(dbin);

        throw;
    }

    memFree(var_names_vector, "**var_names_vector");
    var_names_vector = NULL;

    ff_destroy_process_info_list(pinfo_list);
    db_destroy(dbin);
}

// Given a reference to an instance of class DAS and a filename that refers
// to a freefrom file, read the format file and extract the existing
// attributes and add them to the instance of DAS.
//
// Returns: false if an error accessing the file was detected, true
// otherwise.

void ff_get_attributes(DAS &das, string filename) throw (Error)
{
    AttrTable *attr_table_p = new AttrTable;

    das.add_table("FF_GLOBAL", attr_table_p);
    read_attributes(filename, attr_table_p);
    // Add a table/container for each variable. See bug 284. The DAP spec
    // calls for each variable to have an attribute container, even if it is
    // empty. Previously the server did not have containers for all the
    // variables. 4/4/2002 jhrg
    add_variable_containers(das, filename);
}
