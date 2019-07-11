
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

// (c) COPYRIGHT URI/MIT 1997-99
// Please read the full copyright statement in the file COPYRIGHT.
//
// Authors: reza (Reza Nekovei)

// Utility functions for the FreeForm data server.
//
// jhrg 3/29/96

#include "config_ff.h"

static char rcsid[] not_used =
    { "$Id$" };

#ifndef WIN32
#include <unistd.h> // for access
#else
#define F_OK 0
#endif

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdlib>

#if 0
#include <regex>
#else
#include <BESRegex.h>
#endif

#include <BESDebug.h>

#include <BaseType.h>
#include <Byte.h>
#include <Int16.h>
#include <Int32.h>
#include <UInt16.h>
#include <UInt32.h>
#include <Float32.h>
#include <Float64.h>
#include <InternalErr.h>
#include <dods-limits.h>
#include <util.h>
#include <debug.h>

#include "FFRequestHandler.h"
#include "util_ff.h"

using namespace std;

/**
 * Given a reference to a string object, look for occurrences of '/
 * and remove the text they bracket. This has the affect of removing
 * pathnames from the strings. This function modifies its argument.
 * If zero or one '/' is found, the string is not modified.
 *
 * @note this is used to sanitize error messages from the FreeForm
 * API so pathnames are not leaked back to clients in those messages.
 *
 * @param src The reference to a string
 * @return A reference to the original argument which has likely been
 * modified.
 */
static string &remove_paths(string &src)
{
    size_t p1 = src.find_first_of('/');
    if (p1 == string::npos)
        return src;
    size_t p2 = src.find_last_of('/');
    // The string has one '/', not a big deal
    if (p2 == p1)
        return src;

    src.erase(p1, p2-p1+1);
    return src;
}

// These two functions are defined in FFND/error.c. They were originally
// static functions. I used them to read error strings since FreeForm
// did not have a good way to get the error text. jhrg 9/11/12
extern "C" FF_ERROR_PTR pull_error(void);
extern "C" BOOLEAN is_a_warning(FF_ERROR_PTR error);

/**
 * Build a string that contains message text read from the FreeForm API.
 * This function uses internal methods of the FreeForm API to build up an
 * error message. Because the API only provided a way to write these
 * messages to stderr or a file, I modified the library, making two functions
 * public that were originally static.
 *
 * @note Test for errors from the most recent FrreForm operations using
 * the err_count() function. Calling this function when there is no
 * error is itself an error that results in an exception.
 *
 * @return The error string read from the FreeForm Library.
 */
static string freeform_error_message()
{
    FF_ERROR_PTR error = pull_error();
    if (!error)
        throw BESInternalError("Called the FreeForm error message code, but there was no error.", __FILE__, __LINE__);

    ostringstream oss;
    do {
        if (is_a_warning(error))
            oss << "Warning: ";
        else
            oss << "Error: ";

        // if either of these contain a pathname, remove it
        string problem = error->problem;
        string message = error->message;
        oss << remove_paths(problem) << ": " << remove_paths(message) << endl;

        ff_destroy_error (error);
        error = pull_error();
    } while (error);

    return oss.str();
}

/** Read from a file/database using the FreeForm API. Data values are read
 using an input file descriptor and written using an output format
 description.

 @note I moved this function from ff_read.c (C code) here so I could use
 exceptions to report errors found while using the FreeForm API. jhrg 9/11/12

 @param dataset The name of the file/database to read from
 @param if_file The input format descriptor
 @param o_format The output format description
 @param o_buffer Value-result parameter for the data
 @param bsize Size of the buffer in bytes */
long read_ff(const char *dataset, const char *if_file, const char *o_format, char *o_buffer, unsigned long bsize)
{
    FF_BUFSIZE_PTR newform_log = NULL;
    FF_STD_ARGS_PTR std_args = NULL;

    try {
        std_args = ff_create_std_args();
        if (!std_args)
            throw BESInternalError("FreeForm could not allocate a 'stdargs' object.", __FILE__, __LINE__);

        // set the std_arg structure values - cast away const for dataset, if_file,
        // and o_format.
        std_args->error_prompt = FALSE;
        std_args->user.is_stdin_redirected = 0;
        std_args->input_file = (char*) (dataset);
        std_args->input_format_file = (char*) (if_file);
        std_args->output_file = NULL;
        std_args->output_format_buffer = (char*) (o_format);
        std_args->log_file = (char *) "/dev/null";
#if 0
        // This just doesn't seem to work within the BES framework. jhrg 9/11/12
        std_args->log_file = (char *)"/tmp/ffdods.log";
#endif

        // memory freed automatically on exit
        vector<char> l_bufsz(sizeof(FF_BUFSIZE));
        //bufsz = (FF_BUFSIZE *)&l_bufsz[0];
        FF_BUFSIZE_PTR bufsz = reinterpret_cast<FF_BUFSIZE_PTR>(&l_bufsz[0]);

        bufsz->usage = 1;
        bufsz->buffer = o_buffer;
        bufsz->total_bytes = (FF_BSS_t) bsize;
        bufsz->bytes_used = (FF_BSS_t) 0;

        std_args->output_bufsize = bufsz;

        newform_log = ff_create_bufsize(SCRATCH_QUANTA);
        if (!newform_log)
            throw BESInternalError("FreeForm could not allocate a 'newform_log' object.", __FILE__, __LINE__);

        // passing 0 for the FILE* param is a wash since a non-null
        // value for newform_log selects that as the 'logging' sink.
        // jhrg 9/11/12
        int status = newform(std_args, newform_log, 0 /*stderr*/);

        BESDEBUG("ff", "FreeForm: newform returns " << status << endl);

        if (err_count()) {
            string message = freeform_error_message();
            BESDEBUG("ff", "FreeForm: error message " << message << endl);
            throw BESError(message, BES_SYNTAX_USER_ERROR, __FILE__, __LINE__);
        }

        ff_destroy_bufsize(newform_log);
        ff_destroy_std_args(std_args);

        return bufsz->bytes_used;
    }
    catch (...) {
        if (newform_log)
            ff_destroy_bufsize(newform_log);
        if (std_args)
            ff_destroy_std_args(std_args);

        throw;
    }

    return 0;
}

/**
 * Free a char ** vector that db_ask() allocates. This code uses free()
 * to do the delete.
 *
 * @note There maybe code in FreeForm to do this, but I can't find it.
 */
void free_ff_char_vector(char **v, int len)
{
    for (int i = 0; i < len; ++i)
        if (v && v[i])
            free (v[i]);
    if (v && len > 0)
        free (v);
}

// Given the name of a DODS data type, return the name of the corresponding
// FreeForm data type.
//
// Returns: a const char * if the DODS type maps into a FreeForm type,
// otherwise NULL.

const string ff_types(Type dods_type)
{
    switch (dods_type) {
    case dods_byte_c:
        return "uint8";
    case dods_int16_c:
        return "int16";
    case dods_uint16_c:
        return "uint16";
    case dods_int32_c:
        return "int32";
    case dods_uint32_c:
        return "uint32";
    case dods_float32_c:
        return "float32";
    case dods_float64_c:
        return "float64";
    case dods_str_c:
        return "text";
    case dods_url_c:
        return "text";
    default:
        throw Error("ff_types: DODS type " + D2type_name(dods_type) + " does not map to a FreeForm type.");
    }
}

// Given the name of a DODS data type, return the precision of the
// corresponding FreeForm data type.
//
// Returns: a positive integer if the DODS type maps into a FreeForm type,
// otherwise -1.

int ff_prec(Type dods_type)
{
    switch (dods_type) {
    case dods_byte_c:
    case dods_int16_c:
    case dods_uint16_c:
    case dods_int32_c:
    case dods_uint32_c:
        return 0;
    case dods_float32_c:
        return DODS_FLT_DIG;
    case dods_float64_c:
        return DODS_DBL_DIG;
    case dods_str_c:
    case dods_url_c:
        return 0;
    default:
    	throw Error("ff_prec: DODS type " + D2type_name(dods_type) + " does not map to a FreeForm type.");
    }
}

/** Make a FreeForm output format specification.
    For the current instance of FFArray, build a FreeForm output format
    specification.

    @return The format string. */
const string
make_output_format(const string & name, Type type, const int width)
{
    ostringstream str;

    str << "binary_output_data \"DODS binary output data\"" << endl;
    str << name << " 1 " << width << " " << ff_types(type)
        << " " << ff_prec(type) << endl;

    return str.str();
}

// format for multi-dimension array
const string
makeND_output_format(const string & name, Type type, const int width,
                     int ndim, const long *start, const long *edge, const
                     long *stride, string * dname)
{
    ostringstream str;
    str << "binary_output_data \"DODS binary output data\"" << endl;
    str << name << " 1 " << width << " ARRAY";

    for (int i = 0; i < ndim; i++)
        str << "[" << "\"" << dname[i] << "\" " << start[i] + 1 << " to "
            << start[i] + (edge[i] - 1) * stride[i] +
            1 << " by " << stride[i] << " ]";

    str << " of " << ff_types(type) << " " << ff_prec(type) << endl;

    DBG(cerr << "ND output format: " << str.str() << endl);

    return str.str();
}

/** Set or get the format file delimiter.
    If given no argument, return the format file basename delimiter. If given
    a string argument, set the format file basename delimiter to that string.

    @return A reference to the delimiter string. */
const string & format_delimiter(const string & new_delimiter)
{
    static string delimiter = ".";

    if (new_delimiter != "")
        delimiter = new_delimiter;

    return delimiter;
}

/** Set or get the format file extension.
    If given no argument, return the format file extension. If given a string
    argument, set the format file extension to that string.

    @return A reference to the format file extension. */

const string & format_extension(const string & new_extension)
{
    static string extension = ".fmt";

    if (new_extension != "")
        extension = new_extension;

    return extension;
}

/** FreeForm data and format initialization calls (input format only) */

static bool
cmp_array_conduit(FF_ARRAY_CONDUIT_PTR src_conduit,
                  FF_ARRAY_CONDUIT_PTR trg_conduit)
{
    if (src_conduit->input && trg_conduit->input)
        return (bool) ff_format_comp(src_conduit->input->fd->format,
                                     trg_conduit->input->fd->format);
    else if (src_conduit->output && trg_conduit->output)
        return (bool) ff_format_comp(src_conduit->output->fd->format,
                                     trg_conduit->output->fd->format);
    else
        return false;
}

static int merge_redundant_conduits(FF_ARRAY_CONDUIT_LIST conduit_list)
{
    int error = 0;
    error = list_replace_items((pgenobj_cmp_t) cmp_array_conduit,
                               conduit_list);
    return (error);
}

/**
 * Given a set of standard arguments (input filenames), allocate a DATA-BIN_HANDLE
 * and return an error code. Once called, the caller should free the std_args
 * parameter.
 *
 * @param std_args A pointer to a structure that holds a number of input
 * and out file names.
 */
int SetDodsDB(FF_STD_ARGS_PTR std_args, DATA_BIN_HANDLE dbin_h, char *Msgt)
{
    FORMAT_DATA_LIST format_data_list = NULL;
    int error = 0;

    assert(dbin_h);

    if (!dbin_h) {
        snprintf(Msgt, Msgt_size, "Error: NULL DATA_BIN_HANDLE in %s", ROUTINE_NAME);
        return (ERR_API);
    }

    if (!*dbin_h) {
        *dbin_h = db_make(std_args->input_file);

        if (!*dbin_h) {
            snprintf(Msgt, Msgt_size, "Error in Standard Data Bin");
            return (ERR_MEM_LACK);
        }
    }

    /* Now set the formats and the auxiliary files */

    if (db_set(*dbin_h, DBSET_READ_EQV, std_args->input_file)) {
        snprintf(Msgt, Msgt_size, "Error making name table for %s",
                std_args->input_file);
        return (DBSET_READ_EQV);
    }

    if (db_set(*dbin_h,
               DBSET_INPUT_FORMATS,
               std_args->input_file,
               std_args->output_file,
               std_args->input_format_file,
               std_args->input_format_buffer,
               std_args->input_format_title, &format_data_list)) {
        if (format_data_list)
            dll_free_holdings(format_data_list);

        snprintf(Msgt, Msgt_size, "Error setting an input format for %s",
                std_args->input_file);
        return (DBSET_INPUT_FORMATS);
    }

    error =
        db_set(*dbin_h, DBSET_CREATE_CONDUITS, std_args, format_data_list);
    dll_free_holdings(format_data_list);
    if (error) {
        snprintf(Msgt, Msgt_size, "Error creating array information for %s",
                std_args->input_file);
        return (DBSET_CREATE_CONDUITS);
    }

    if (db_set(*dbin_h, DBSET_HEADER_FILE_NAMES, FFF_INPUT,
               std_args->input_file)) {
        snprintf(Msgt, Msgt_size, "Error determining input header file names for %s",
                std_args->input_file);
        return (DBSET_HEADER_FILE_NAMES);
    }

    if (db_set(*dbin_h, DBSET_HEADERS)) {
        snprintf(Msgt, Msgt_size, "getting header file for %s", std_args->input_file);
        return (DBSET_HEADERS);
    }


    if (db_set(*dbin_h, DBSET_INIT_CONDUITS, FFF_DATA,
               std_args->records_to_read)) {
        snprintf(Msgt, Msgt_size, "Error creating array information for %s",
                std_args->input_file);
        return (DBSET_INIT_CONDUITS);
    }

    error = merge_redundant_conduits((*dbin_h)->array_conduit_list);
    if (error)
        snprintf(Msgt, Msgt_size, "Error merging redundent conduits");

    return (error);
}

/**
 * Figure out how many records there are in the dataset.
 *
 * @param filename
 */
long Records(const string &filename)
{
    int error = 0;
    DATA_BIN_PTR dbin = NULL;
    FF_STD_ARGS_PTR SetUps = NULL;
    PROCESS_INFO_LIST pinfo_list = NULL;
    PROCESS_INFO_PTR pinfo = NULL;
    static char Msgt[255];

    SetUps = ff_create_std_args();
    if (!SetUps) {
        return -1;
    }

    /** set the structure values to create the FreeForm DB**/
    SetUps->user.is_stdin_redirected = 0;
    SetUps->input_file = const_cast<char*>(filename.c_str());

    SetUps->output_file = NULL;

    error = SetDodsDB(SetUps, &dbin, Msgt);
    if (error && error < ERR_WARNING_ONLY) {
        ff_destroy_std_args(SetUps);
        db_destroy(dbin);
        return -1;
    }

    ff_destroy_std_args(SetUps);

    error = db_ask(dbin, DBASK_PROCESS_INFO, FFF_INPUT | FFF_DATA, &pinfo_list);
    if (error)
        return (-1);

    pinfo_list = dll_first(pinfo_list);

    pinfo = ((PROCESS_INFO_PTR) (pinfo_list)->data.u.pi);

    long num_records = PINFO_SUPER_ARRAY_ELS(pinfo);

    ff_destroy_process_info_list(pinfo_list);
    db_destroy(dbin);

    return num_records;
}


bool file_exist(const char *filename)
{
    return access(filename, F_OK) == 0;
}

/** Find the RSS (Remote Sensing Systems) format file using their naming
    convention.

    File naming convention: <data source> + '_' + <date_string> + <version> +
    (optional)< _d3d > When <date_string> includes YYYYMMDDVV ('DD') the file
    contains 'daily' data. When <date_string> only includes YYYYMMVV ( no
    'DD'), or includes ('DD') and optional '_d3d' then the file contains
    averaged data.

    Different format files are required for 'daily' and 'averaged' data.

    @return A const string object which contains the format file name. */
const string
find_ancillary_rss_formats(const string & dataset, const string & /* delimiter */,
			   const string & /* extension */)
{
    string FormatFile;
    string FormatPath = FFRequestHandler::get_RSS_format_files();
    string BaseName;
    string FileName;

    // Separate the filename from the pathname, for both plain files
    // and cached decompressed files (ones with '#' in their names).
    size_t delim = dataset.rfind("#");
    if (delim != string::npos)
        FileName = dataset.substr(delim + 1, dataset.length() - delim + 1);
    else {
        delim = dataset.rfind("/");
        if (delim != string::npos)
            FileName = dataset.substr(delim + 1, dataset.length() - delim + 1);
        else
            FileName = dataset;
    }

    // The file/dataset name has to have an underscore...
    delim = FileName.find("_");
    if ( delim != string::npos ) {
      BaseName = FileName.substr(0,delim+1);
    }
    else {
      throw Error("Could not find input format for: " + dataset);
    }

    // Now determine if this is files holds averaged or daily data.
    string DatePart = FileName.substr(delim+1, FileName.length()-delim+1);

    if (FormatPath[FormatPath.length()-1] != '/')
        FormatPath.append("/");

    if ( (DatePart.find("_") != string::npos) || (DatePart.length() < 10) )
        FormatFile = FormatPath + BaseName + "averaged.fmt";
    else
        FormatFile = FormatPath + BaseName + "daily.fmt";

    return string(FormatFile);
}

/** Find the RSS (Remote Sensing Systems) format file using their naming
    convention.

    File naming convention: <data source> + '_' + <date_string> + <version> +
    (optional)< _d3d > When <date_string> includes YYYYMMDDVV ('DD') the file
    contains 'daily' data. When <date_string> only includes YYYYMMVV ( no
    'DD'), or includes ('DD') and optional '_d3d' then the file contains
    averaged data.

    Different format files are required for 'daily' and 'averaged' data.

    @return A const string object which contains the format file name. */
const string
find_ancillary_rss_das(const string & dataset, const string & /* delimiter */,
		       const string & /* extension */)
{
    string FormatFile;
    string FormatPath = FFRequestHandler::get_RSS_format_files();
    string BaseName;
    string FileName;

    size_t delim = dataset.rfind("#");
    if (delim != string::npos)
        FileName = dataset.substr(delim + 1, dataset.length() - delim + 1);
    else {
        delim = dataset.rfind("/");
        if (delim != string::npos)
            FileName = dataset.substr(delim + 1, dataset.length() - delim + 1);
        else
            FileName = dataset;
    }

    delim = FileName.find("_");
    if ( delim != string::npos ) {
        BaseName = FileName.substr(0,delim+1);
    }
    else {
        string msg = "Could not find input format for: ";
        msg += dataset;
        throw InternalErr(msg);
    }

    string DatePart = FileName.substr(delim+1, FileName.length()-delim+1);

    if (FormatPath[FormatPath.length()-1] != '/')
        FormatPath.append("/");

    if ( (DatePart.find("_") != string::npos) || (DatePart.length() < 10) )
        FormatFile = FormatPath + BaseName + "averaged.das";
    else
        FormatFile = FormatPath + BaseName + "daily.das";

    return string(FormatFile);
}

// These functions are used by the Date/Time Factory classes but they might
// be generally useful in writing server-side functions. 1/21/2002 jhrg

bool is_integer_type(BaseType * btp)
{
    switch (btp->type()) {
    case dods_null_c:
        return false;

    case dods_byte_c:
    case dods_int16_c:
    case dods_uint16_c:
    case dods_int32_c:
    case dods_uint32_c:
        return true;

    case dods_float32_c:
    case dods_float64_c:
    case dods_str_c:
    case dods_url_c:
    case dods_array_c:
    case dods_structure_c:
    case dods_sequence_c:
    case dods_grid_c:
    default:
        return false;
    }
}

bool is_float_type(BaseType * btp)
{
    switch (btp->type()) {
    case dods_null_c:
    case dods_byte_c:
    case dods_int16_c:
    case dods_uint16_c:
    case dods_int32_c:
    case dods_uint32_c:
        return false;

    case dods_float32_c:
    case dods_float64_c:
        return true;

    case dods_str_c:
    case dods_url_c:
    case dods_array_c:
    case dods_structure_c:
    case dods_sequence_c:
    case dods_grid_c:
    default:
        return false;
    }
}

/** Get the value of the BaseType Variable. If it's not something that we can
    convert to an integer, throw InternalErr.

    @param var The variable */
dods_uint32 get_integer_value(BaseType * var) throw(InternalErr)
{
    if (!var)
        return 0;

    switch (var->type()) {
    case dods_byte_c:
    	return static_cast<Byte*>(var)->value();

    case dods_int16_c:
    	return static_cast<Int16*>(var)->value();

    case dods_int32_c:
    	return static_cast<Int32*>(var)->value();

    case dods_uint16_c:
    	return static_cast<UInt16*>(var)->value();

    case dods_uint32_c:
    	return static_cast<UInt32*>(var)->value();

    default:
        throw InternalErr(__FILE__, __LINE__,
                          "Tried to get an integer value for a non-integer datatype!");
    }
}

dods_float64 get_float_value(BaseType * var) throw(InternalErr)
{
    if (!var)
        return 0.0;

    switch (var->type()) {
    case dods_int16_c:
    case dods_uint16_c:
    case dods_int32_c:
    case dods_uint32_c:
        return get_integer_value(var);

    case dods_float32_c:
    	return static_cast<Float32*>(var)->value();

    case dods_float64_c:
    	return static_cast<Float64*>(var)->value();

    default:
        throw InternalErr(__FILE__, __LINE__,
                          "Tried to get an float value for a non-numeric datatype!");
    }
}

string get_Regex_format_file(const string & filename)
{
    string::size_type found = filename.find_last_of("/\\");
    string base_name = filename.substr(found+1);
    string retVal = "";
    std::map<string,string> mapFF = FFRequestHandler::get_fmt_regex_map();
    for (auto rgx = mapFF.begin(); rgx != mapFF.end(); ++ rgx) {
        BESDEBUG("ff", "get_Regex_format_file() - filename: '" << filename << "'  regex: '" << (*rgx).first << "'  format: '" << (*rgx).second << "'" << endl);
#if 0
        if (regex_match (base_name, regex((*rgx).first) )){
            retVal = string((*rgx).second);
            break;
        }
#else
        BESRegex regex(((*rgx).first).c_str());
         if ( regex.match(base_name.c_str(), base_name.length()) > 0 ){
             retVal = string((*rgx).second);
             break;
         }
#endif
    }
    BESDEBUG("ff", "get_Regex_format_file() - returning format filename: '"<< retVal << "'" << endl);
    return retVal;
}
