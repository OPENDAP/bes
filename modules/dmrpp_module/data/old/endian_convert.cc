// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES

// Copyright (c) 2016 OPeNDAP, Inc.
// Author: Nathan David Potter <ndp@opendap.org>
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

#include "config.h"

// Added copyright, config.h and removed unused headers. jhrg

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#include <cstdlib>

#include <GetOpt.h>

static bool debug = false;
using namespace std;

// Code used and didn't use 'std' (likely a header has using... in it).
// I switched to using... because it was too much work to put 'std' everywhere.
// jhrg

string usage(string prog_name) {
	ostringstream ss;
	ss << "usage: " << prog_name << "[-d] -f <file> -w <word_width>]" << endl;
	return ss.str();
}

long long get_file_size(string filename) {
	long long size;
	ifstream in(filename.c_str(), ifstream::ate | ifstream::binary);
	size = in.tellg();
	in.close();
	return size;
}

/**
 * Invert the 'width' bytes pointed to by 'vals'
 *
 * Invert means transform from big-endian to little-endian
 * or vice-versa.
 *
 * @param vals Start of bytes to invert
 * @param width Number of bytes to invert
 */
void invert_byte_order(char *vals, int width) {

	if (debug) {
		cerr << "BEFORE vals: " << static_cast<void*>(vals) << endl;
		for (int i = 0; i < width; i++) {
			cerr << "vals[" << i << "]: " << +vals[i] << endl;
		}
	}
	// Make a copy
	char tmp[width];
	for (int i = 0; i < width; i++) {
		tmp[i] = vals[i];
	}
	// Invert the order
	for (int i = 0, j = width - 1; i < width; i++, j--) {
			vals[i] = tmp[j];
	}
	if (debug) {
		cerr << "AFTER  vals: " << static_cast<void*>(vals) << endl;
		for (int i = 0; i < width; i++) {
			cerr << "vals[" << i << "]: " << +vals[i] << endl;
		}
	}
}

int main(int argc, char **argv) {
	string file;
	unsigned int width = 0;

	GetOpt getopt(argc, argv, "df:w:");
	int option_char;
	while ((option_char = getopt()) != EOF) {
		switch (option_char) {
		case 'd':
			debug = 1;
			break;
		case 'f':
			file = getopt.optarg;
			break;
		case 'w':
		    width = strtol(getopt.optarg, 0, 10);
			break;
		case '?':
			cerr << usage(argv[0]);
		}
	}
	cerr << "debug:     " << (debug ? "ON" : "OFF") << endl;
	if (debug) {
		cerr << "InputFile: '" << file << "'" << endl;
		cerr << "TypeWidth: " << width << endl;
	}

	bool qc_flag = false;
	if (file.empty()) {
		cerr << "File name must be set using the -f option" << endl;
		qc_flag = true;
	}
	if (width == 0) {
		cerr << "Type width must be set using the -w option"
				<< endl;
		qc_flag = true;
	}
	if (qc_flag) {
		return 1;
	}

	// How big is it?
	long long file_size = get_file_size(file);
	if (file_size < 0) {
		cerr << "ERROR: Unable to open file '" << file << "'" << endl;
		return 1;
	}
	if (debug)
		cerr << "File Size: " << file_size << " (bytes)" << endl;

	// Read the whole thing into memory
	char values[file_size];
	ifstream source_file_is(file.c_str(), ifstream::in | ifstream::binary);
	source_file_is.read(values, file_size);
	if (!source_file_is) {
		cerr << "ERROR: only " << source_file_is.gcount()
				<< " of "<< file_size << " bytes could be read" << endl;
		return 1;
	}
	if (debug)
		cerr << "File has been read successfully." << endl;

	// Don't need to close this object. jhrg
	source_file_is.close();

	// Invert byte order for each "word" of size width
	for (long long i = 0; i < file_size; i += width) {
		if (debug) cerr << "i: " << i << endl;
		invert_byte_order(&values[i], width);
	}

	// Write the result to stdout
	cout.write(values, file_size);

	return 0;
}
