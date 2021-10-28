//
// Created by James Gallagher on 10/12/21.
//

#include <vector>
#include <string>
#include <iostream>
#include <fstream>

#include "rapidxml/rapidxml.hpp"
#if 0
#include "rapidxml/rapidxml_iterators.hpp"
#include "rapidxml/rapidxml_utils.hpp"
#endif
#include "rapidxml/rapidxml_print.hpp"


using namespace rapidxml;
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

int main(int argc, char *argv[])
{
#if 1
    // Read the XML document into memory as a c string.
    vector<char> text;
    read_xml(argv[1], text);
#else
    ifstream the_file (argv[1]);
    vector<char> text((istreambuf_iterator<char>(the_file)), istreambuf_iterator<char>());
    text.push_back('\0');
#endif
    xml_document<> doc;    // character type defaults to char
    doc.parse<0>(text.data());    // 0 means default parse flags

#if 0
    // Print to stream using operator <<
    std::cout << doc;
#endif
#if 0
    // Print to stream using print function, specifying printing flags
    print(std::cout, doc, 0);   // 0 means default printing flags

    // Print to string using output iterator
    std::string s;
    print(std::back_inserter(s), doc, 0);

    // Print to memory buffer using output iterator
    char buffer[4096];                      // You are responsible for making the buffer large enough!
    char *end = print(buffer, doc, 0);      // end contains pointer to character after last printed character
    *end = 0;                               // Add string terminator after XML
#endif
}
