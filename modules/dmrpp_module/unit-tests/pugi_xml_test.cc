//
// Created by James Gallagher on 10/12/21.
//

#include <vector>
#include <string>
#include <iostream>
#include <fstream>

#define PUGIXML_HEADER_ONLY
#include <pugixml.hpp>

using namespace pugi;
using namespace std;

/**
 * @brief read a text file into memory and null terminate
 *
 * @note Don't make a string for the data - this function is for use with
 * the RapidXML parser which needs mutable data and we're trying to avoid
 * making copies of what maybe a large XML document.
 *
 * @param file_name Name of the file
 * @param bytes Reference to a vector that will hold the data
 */
void read_xml(const string &file_name, vector<char> &bytes)
{
    ifstream ifs(file_name.c_str(), ios::in | ios::binary | ios::ate);

    ifstream::pos_type file_size = ifs.tellg();
    ifs.seekg(0, ios::beg);

    bytes.resize(file_size + ifstream::pos_type(1LL));   // Add space for null termination
    ifs.read(bytes.data(), file_size);

    bytes[file_size] = '\0';

    // The above version is about 3x faster than the below three line version on a 5MB XML doc.
    //
    //    ifstream the_file (argv[1]);
    //    vector<char> text((istreambuf_iterator<char>(the_file)), istreambuf_iterator<char>());
    //    text.push_back('\0');
}

int main(int, char *argv[])
{
    pugi::xml_document doc;

    pugi::xml_parse_result result = doc.load_file(argv[1]);

    std::cout << "Load result: " << result.description() << ", mesh name: " << doc.child("Dataset").attribute("name").value() << std::endl;
}
