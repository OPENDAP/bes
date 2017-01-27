// fits_read_descriptors.cc

// This file is part of fits_handler, a data handler for the OPeNDAP data
// server. 

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.edu> and Jose Garcia <jgarcia@ucar.edu>
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
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301

// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include "config.h"

#include <sstream>
#include <memory>

#include <DDS.h>
#include <Structure.h>
#include <Str.h>
#include <Array.h>
#include <Byte.h>
#include <Int16.h>
#include <Int32.h>
#include <Float32.h>
#include <Float64.h>
#include <Error.h>
#include <mime_util.h>

#include <BESDebug.h>

#include <fitsio.h>

#include "fits_read_descriptors.h"
#include "BESAutoPtr.h"

using namespace std;
using namespace libdap;

void fits_handler::process_status(int status, string &error)
{
    if (status) {
        fits_report_error(stderr, status);
    }
    char error_description[30] = "";
    fits_get_errstatus(status, error_description);
    error = error_description;
    return;
}

#if 0
char *
fits_handler::ltoa(long val, /* value to be converted */
char *buf, /* output string         */
int base) /* conversion base       */
{
    ldiv_t r; /* result of val / base  */

    if (base > 36 || base < 2) /* no conversion if wrong base */
    {
        *buf = '\0';
        return buf;
    }
    if (val < 0)
        *buf++ = '-';
    r = ldiv(labs(val), base);

    /* output digits of val/base first */

    if (r.quot > 0)
        buf = ltoa(r.quot, buf, base);
    /* output last digit */

    *buf++ = "0123456789abcdefghijklmnopqrstuvwxyz"[(int) r.rem];
    *buf = '\0';
    return buf;
}
#endif

bool fits_handler::fits_read_descriptors(DDS &dds, const string &filename, string &error)
{
    fitsfile *fptr;
    int status = 0;
    if (fits_open_file(&fptr, filename.c_str(), READONLY, &status)) {
        error = "Can not open fits file ";
        error += filename;
        return false;
    }

    dds.set_dataset_name(name_path(filename));

    int hdutype;
    for (int ii = 1; !(fits_movabs_hdu(fptr, ii, &hdutype, &status)); ii++) {
        ostringstream hdu;
        hdu << "HDU_" << ii;
        switch (hdutype) {
        case IMAGE_HDU:
        	// 'hdu' holds the name of the Structure; hdu + _IMAGE is the name of
        	// the variable within that structure that holds the image data.
            status = process_hdu_image(fptr, dds, hdu.str(), hdu.str() + "_IMAGE");
            process_status(status, error);
            break;
        case ASCII_TBL:
            status = process_hdu_ascii_table(fptr, dds, hdu.str(), hdu.str() + "_ASCII");
            process_status(status, error);
            break;
        case BINARY_TBL:
        	// FIXME This should return an error!
        	// process_hdu_binary_table does/did nothing but return a non-error status code.
        	// NB: I rewrote the call, commented out, using the two new args that eliminate
        	// the global string variables. jhrg 6/14/13
            status = 0; // process_hdu_binary_table(fptr, dds, hdu.str(), hdu.str() + "_BINARY");
            process_status(status, error);
            break;
        default:
            // cerr << "Invalid HDU type in file " << filename << endl;
            process_status(1, error);
            break;
        }
    }

    if (status == END_OF_FILE) // status values are defined in fitsioc.h
        status = 0; // got the expected EOF error; reset = 0
    else {
        process_status(status, error);
        fits_close_file(fptr, &status);
        return false;
    }
    if (fits_close_file(fptr, &status)) {
        process_status(status, error);
        return false;
    }
    return true;
}

// T = Byte --> U = dods_byte, fits_types = TBYTE
template<class T, class U>
static int read_hdu_image_data(int fits_type, fitsfile *fptr, const string &varname, const string &datasetname, int number_axes, const vector<long> &naxes, Structure *container)
{
	auto_ptr<T> in(new T(varname, datasetname));
	auto_ptr<Array> arr(new Array(varname, datasetname, in.get()));
	long npixels = 1;
	for (register int w = 0; w < number_axes; w++) {
		ostringstream oss;
		oss << "NAXIS" << w;
		arr->append_dim(naxes[w], oss.str());

		npixels = npixels * naxes[w];
	}
	dods_byte nullval = 0;
	vector<U> buffer(npixels);
	int anynull, status = 0;
	// the original code always read the whole array; I dropped passing in fpixel and replaced it with a '1'.
	// jhrg 6/14/13
	fits_read_img(fptr, fits_type, 1 /*first pixel*/, npixels, &nullval, &buffer[0], &anynull, &status);
	arr->set_value(&buffer[0], npixels);
	container->add_var(arr.get());

	return status;
}

int fits_handler::process_hdu_image(fitsfile *fptr, DDS &dds, const string &hdu, const string &str)
{
    string datasetname = dds.get_dataset_name();

    auto_ptr<Structure> container(new Structure(hdu, datasetname));

    int status = 0;
    int nkeys, keypos;
    if (fits_get_hdrpos(fptr, &nkeys, &keypos, &status))
        return status;

	for (int jj = 1; jj <= nkeys; jj++) {
		char name[FLEN_KEYWORD];
		char value[FLEN_VALUE];
		char comment[FLEN_COMMENT];
		if (fits_read_keyn(fptr, jj, name, value, comment, &status)) return status;

		ostringstream keya;
		keya << "key_" << jj;
		auto_ptr<Structure> st(new Structure(keya.str(), datasetname));

		auto_ptr<Str> s1(new Str("name", datasetname));
		string ppp = name;
		s1->set_value(ppp);

		auto_ptr<Str> s2(new Str("value", datasetname));
		ppp = value;
		s2->set_value(ppp);

		auto_ptr<Str> s3(new Str("comment", datasetname));
		ppp = comment;
		s3->set_value(ppp);

		st->add_var(s1.get());
		st->add_var(s2.get());
		st->add_var(s3.get());
		container->add_var(st.get());
	}

    char value[FLEN_VALUE];
    if (fits_read_keyword(fptr, "BITPIX", value, NULL, &status))
        return status; // status is not 0, so something is wrong and we get until here...
    int bitpix = atoi(value);

    if (fits_read_keyword(fptr, "NAXIS", value, NULL, &status))
        return status;
    int number_axes = atoi(value);

    vector<long> naxes(number_axes);
    int nfound;
    if (fits_read_keys_lng(fptr, "NAXIS", 1, number_axes, &naxes[0], &nfound, &status))
        return status; // status is not 0, so something is wrong

    switch (bitpix) {
    case BYTE_IMG:
    	status = read_hdu_image_data<Byte,dods_byte>(TBYTE, fptr, str, datasetname, number_axes, naxes, container.get());
    	break;

    case SHORT_IMG:
    	status = read_hdu_image_data<Int16,dods_int16>(TSHORT, fptr, str, datasetname, number_axes, naxes, container.get());
    	break;

    case LONG_IMG:
    	// Here is/was the bug that started me down the whole rabbit hole of reworking this code. Passing TLONG
    	// instead of TINT seems to make cfitsio 3270 to 3340 (and others?) treat the data as if it is an
    	// array of 64-bit integers. This is odd since TLONG is a 32-bit int type #define'd in cfitsio.h.
    	// In valgrind, using TLONG returns the following stack trace from valgrind:
    	//
    	// ==22851== Invalid write of size 8
    	// ==22851==    at 0x3186835: fffi4i4 (getcolj.c:1158)
    	// ==22851==    by 0x31899A2: ffgclj (getcolj.c:779)
    	// ==22851==    by 0x3186372: ffgpvj (getcolj.c:56)
    	// ==22851==    by 0x3178719: ffgpv (getcol.c:637)
    	// ==22851==    by 0x3122127: fits_handler::process_hdu_image(fitsfile*, libdap::DDS&, std::string const&, std::string const&) (fits_read_descriptors.cc:169)
    	//
       	// valgrind --dsymutil=yes besstandalone -c bes.conf -i fits/dpm.dds.bescmd
    	//
    	// If TLONG is used and the array of dods_int32 is allocated as 2 * npixels, the code does not trigger
    	// the invalid write message.
    	//
    	// jhrg 6/14/13
    	status = read_hdu_image_data<Int32,dods_int32>(TINT/*TLONG*/, fptr, str, datasetname, number_axes, naxes, container.get());
    	break;

    case FLOAT_IMG:
    	status = read_hdu_image_data<Float32,dods_float32>(TFLOAT, fptr, str, datasetname, number_axes, naxes, container.get());
    	break;

    case DOUBLE_IMG:
    	status = read_hdu_image_data<Float64,dods_float64>(TDOUBLE, fptr, str, datasetname, number_axes, naxes, container.get());
    	break;

    default:
        status = 1;
        break;
    }

    dds.add_var(container.get());
    return status;
}

// I made minimal changes to this code below; mostly using auto_ptr where applicable and fixing
// the bug with cfitsio where TLONG seems to be treated as a 64-but integer.

int fits_handler::process_hdu_ascii_table(fitsfile *fptr, DDS &dds, const string &hdu, const string &str)
{
    string datasetname = dds.get_dataset_name();
    auto_ptr<Structure> container(new Structure(hdu, datasetname));
    int status = 0;
    int nfound, anynull;
    int ncols;
    long nrows;
    int nkeys, keypos;
    char name[FLEN_KEYWORD];
    char value[FLEN_VALUE];
    char comment[FLEN_COMMENT];
    anynull = 0;

    if (fits_get_hdrpos(fptr, &nkeys, &keypos, &status))
        return status;

    for (int jj = 1; jj <= nkeys; jj++) {
        if (fits_read_keyn(fptr, jj, name, value, comment, &status))
            return status;
    	ostringstream oss;
    	oss << "key_" << jj;
        auto_ptr<Structure> st(new Structure(oss.str(), datasetname));
        auto_ptr<Str> s1(new Str("name", datasetname));
        string ppp = name;
        s1->set_value(ppp);
        auto_ptr<Str> s2(new Str("value", datasetname));
        ppp = value;
        s2->set_value(ppp);
        auto_ptr<Str> s3(new Str("comment", datasetname));
        ppp = comment;
        s3->set_value(ppp);
        st->add_var(s1.get());
        st->add_var(s2.get());
        st->add_var(s3.get());
        container->add_var(st.get());
    }

    fits_get_num_rows(fptr, &nrows, &status);
    fits_get_num_cols(fptr, &ncols, &status);

    // I am sure this is one of the most obscure piece of code you have ever seen
    // First I get an auto pointer object to hold securely an array of auto pointer
    // objects holding securely pointers to char...
    BESAutoPtr<BESAutoPtr<char> > ttype_autoptr(new BESAutoPtr<char> [ncols], true);

    // Now I set each one of my auto pointer objects holding pointers to char
    // to hold the address of a dynamically allocated piece of memory (array of chars)...
    for (int j = 0; j < ncols; j++) {
        (ttype_autoptr.get())[j].reset();
        (ttype_autoptr.get())[j].set(new char[FLEN_VALUE], true);
    }

    // Now I align my pointers to char inside each BESAutoPtr <char> object
    // inside my BESAutoPtr< BESAutoPtr <char> > object to a char** pointer.

    // Step 1:
    // I get a insecure pointer an get some memory on it...
    char **ttype = new char*[ncols];

    // Step 2;
    // Lets secure the memory area pointed by ttype inside
    // a BESAutoPtr<char*> object, Lets not forget ttype is a vector
    BESAutoPtr<char*> secure_ttype(ttype, true);

    // Step 3:
    // OK here we are, now we have ncols beautifully aligned pointers to char
    // let the pointer inside ttype point to the same address as the secure ones...
    for (int j = 0; j < ncols; j++)
        ttype[j] = (ttype_autoptr.get())[j].get();

    // Step 4:
    // Now we read the data!
    if (fits_read_keys_str(fptr, "TTYPE", 1, ncols, ttype, &nfound, &status))
        return status;

    // wasn't that fun ? :)


    auto_ptr<Structure> table(new Structure(str, datasetname));

    for (int h = 0; h < ncols; h++) {
        int typecode;
        long repeat, width;
        fits_get_coltype(fptr, h + 1, &typecode, &repeat, &width, &status);

        switch (typecode) {
        case TSTRING: {
            int p;
            auto_ptr<Str> in(new Str(ttype[h], datasetname));
            auto_ptr<Array> arr(new Array(ttype[h], datasetname, in.get()));
            arr->append_dim(nrows);
            char strnull[10] = "";
            char **name = new char*[nrows];
            // secure the pointer for exceptions, remember is an array so second parameter is true
            BESAutoPtr<char *> sa1(name, true);
            // get an auto pointer (sa3) object to hold securely an array of auto pointers to char
            BESAutoPtr<BESAutoPtr<char> > sa3(new BESAutoPtr<char> [nrows], true);
            for (p = 0; p < nrows; p++) {
                // get memory
                name[p] = new char[width + 1];
                // reset auto pointer
                (sa3.get())[p].reset();
                // storage safely the area in the heap pointed by name[p]  in an auto pointer
                (sa3.get())[p].set(name[p], true);
            }
            fits_read_col(fptr, typecode, h + 1, 1, 1, nrows, strnull, name, &anynull, &status);

            string *strings = new string[nrows];
            // secure the pointer for exceptions, remember is an array
            BESAutoPtr<string> sa2(strings, true);

            for (int p = 0; p < nrows; p++)
                strings[p] = name[p];
            arr->set_value(strings, nrows);

            table->add_var(arr.get());
        }
            break;
        case TSHORT: {
            BESAutoPtr<Int16> in(new Int16(ttype[h], datasetname));
            BESAutoPtr<Array> arr(new Array(ttype[h], datasetname, in.get()));
            arr->append_dim(nrows);
            dods_int16 nullval = 0;
            BESAutoPtr<dods_int16> buffer(new dods_int16[nrows], true);
            fits_read_col(fptr, typecode, h + 1, 1, 1, nrows, &nullval, buffer.get(), &anynull, &status);
            arr->set_value(buffer.get(), nrows);
            table->add_var(arr.get());
        }
            break;
        case TLONG: {
            BESAutoPtr<Int32> in(new Int32(ttype[h], datasetname));
            BESAutoPtr<Array> arr(new Array(ttype[h], datasetname, in.get()));
            arr->append_dim(nrows);
            dods_int32 nullval = 0;
            BESAutoPtr<dods_int32> buffer(new dods_int32[nrows], true);
            fits_read_col(fptr, TINT/*typecode*/, h + 1, 1, 1, nrows, &nullval, buffer.get(), &anynull, &status);
            arr->set_value(buffer.get(), nrows);
            table->add_var(arr.get());
        }
            break;
        case TFLOAT: {
            BESAutoPtr<Float32> in(new Float32(ttype[h], datasetname));
            BESAutoPtr<Array> arr(new Array(ttype[h], datasetname, in.get()));
            arr->append_dim(nrows);
            dods_float32 nullval = 0;
            BESAutoPtr<dods_float32> buffer(new dods_float32[nrows], true);
            fits_read_col(fptr, typecode, h + 1, 1, 1, nrows, &nullval, buffer.get(), &anynull, &status);
            arr->set_value(buffer.get(), nrows);
            table->add_var(arr.get());
        }
            break;
        case TDOUBLE: {
            BESAutoPtr<Float64> in(new Float64(ttype[h], datasetname));
            BESAutoPtr<Array> arr(new Array(ttype[h], datasetname, in.get()));
            arr->append_dim(nrows);
            dods_float64 nullval = 0;
            BESAutoPtr<dods_float64> buffer(new dods_float64[nrows], true);
            fits_read_col(fptr, typecode, h + 1, 1, 1, nrows, &nullval, buffer.get(), &anynull, &status);
            arr->set_value(buffer.get(), nrows);
            table->add_var(arr.get());
        }
            break;
        }
    }
    container->add_var(table.get());
    dds.add_var(container.get());
    return status;
}

#if 0
int fits_handler::process_hdu_binary_table(fitsfile *, DDS &)
{
    return 0;
}
#endif
