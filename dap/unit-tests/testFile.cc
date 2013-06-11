#include "config.h"

#include <fstream>
#include <string>
#include <vector>

#include "testFile.h"

using namespace std;

string
readTestBaseline(const string &fn)
{
    int length;

    ifstream is;
    is.open (fn.c_str(), ios::binary );

    // get length of file:
    is.seekg (0, ios::end);
    length = is.tellg();

    // back to start
    is.seekg (0, ios::beg);

    // allocate memory:
    vector<char> buffer(length+1);

    // read data as a block:
    is.read (&buffer[0], length);
    is.close();
    buffer[length] = '\0';

    return string(&buffer[0]);
}
