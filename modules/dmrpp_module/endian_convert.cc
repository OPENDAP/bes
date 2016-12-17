#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <GetOpt.h>

#include <XDRStreamUnMarshaller.h>
#include <Type.h>

bool debug = false;

string usage(string progName) {
	std::ostringstream ss;
	ss << "usage: " << progName << "[-d] -f <file> -w <word_width>]" << std::endl;
	return ss.str();
}

long long get_file_size(std::string filename) {
	long long size;
	std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
	size = in.tellg();
	in.close();
	return size;
}

void invert_byte_order(char *vals, int width) {

	if (debug) {
		std::cerr << "BEFORE vals: " << static_cast<void*>(vals) << endl;
		for (int i = 0; i < width; i++) {
			std::cerr << "vals[" << i << "]: " << +vals[i] << endl;
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
		std::cerr << "AFTER  vals: " << static_cast<void*>(vals) << endl;
		for (int i = 0; i < width; i++) {
			std::cerr << "vals[" << i << "]: " << +vals[i] << endl;
		}
	}
}

int main(int argc, char **argv) {
	std::string file;
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
			width = stol(getopt.optarg);
			break;
		case '?':
			std::cerr << usage(argv[0]);
		}
	}
	std::cerr << "debug:     " << (debug ? "ON" : "OFF") << endl;
	if (debug) {
		std::cerr << "InputFile: '" << file << "'" << std::endl;
		std::cerr << "TypeWidth: " << width << std::endl;
	}

	bool qc_flag = false;
	if (file.empty()) {
		std::cerr << "File name must be set using the -f option" << std::endl;
		qc_flag = true;
	}
	if (width == 0) {
		std::cerr << "Type width must be set using the -w option"
				<< std::endl;
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
		std::cerr << "File Size: " << file_size << " (bytes)" << std::endl;

	// Read the whole thing into memory
	char values[file_size];
	std::ifstream source_file_is(file,
			std::ifstream::in | std::ifstream::binary);
	source_file_is.read(values, file_size);
	if (source_file_is) {
		if(debug)
			std::cerr << "File has been read successfully." << endl;
	} else {
		std::cerr << "ERROR: only " << source_file_is.gcount()
				<< " of "<< file_size << " bytes could be read" << endl;
		return 1;
	}
	source_file_is.close();

	// Invert byte order for each "word" of size width
	for (long long i = 0; i < file_size; i += width) {
		if (debug) std::cerr << "i: " << i << endl;
		invert_byte_order(&values[i], width);
	}

	// Write the result to stdout
	cout.write(values, file_size);

	return 0;
}
