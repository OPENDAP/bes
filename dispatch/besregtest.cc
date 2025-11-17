// besregtest.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004-2009 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.edu>
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

#include <iostream>
#include <map>
#include <string>

using std::cout;
using std::endl;
using std::multimap;
using std::pair;
using std::string;

#include "BESError.h"
#include "BESRegex.h"

multimap<string, string> expressions;

bool break_includes(const string &s, string &err);
bool break_types(const string &s, string &err);

void usage(const string &prog) {
    cout << "Usage: " << prog << " include|exclude|type <regular_expression> <string_to_match>" << endl;
    cout << "  samples:" << endl;
    cout << "    besregtest include \"123456;\" 01234567 matches 6 of 8 characters" << endl;
    cout << "    besregtest include \"^123456$;\" 01234567 does not match" << endl;
    cout << "    besregtest include \"^123456$;\" 123456 matches all 6 of 6 characters" << endl;
    cout << "    besregtest include \".*\\.nc$;\" fnoc1.nc matches" << endl;
    cout << "    besregtest include \".*\\.nc$;\" fnoc1.ncd does not matche" << endl;
    cout << "    besregtest type \"nc:.*\\.nc$;nc:.*\\.nc\\.gz$;ff:.*\\.dat$;ff:.*\\.dat\\.gz$;\" fnoc1.nc matches "
            "type nc"
         << endl;
}

int main(int argc, char **argv) {
    if (argc != 4) {
        usage(argv[0]);
        return 1;
    }

    string what = argv[1];
    if (what != "include" && what != "exclude" && what != "type") {
        cout << "please specify either an Include or TypeMatch expression "
             << "by using include or type respectively as first parameter" << endl;
        usage(argv[0]);
        return 1;
    }

    string err;
    bool status = true;
    if (what == "include" || what == "exclude")
        status = break_includes(argv[2], err);
    else
        status = break_types(argv[2], err);

    if (!status) {
        cout << err << endl;
        usage(argv[0]);
        return 1;
    }

    string inQuestion(argv[3]);

    multimap<string, string>::const_iterator i = expressions.begin();
    multimap<string, string>::const_iterator ie = expressions.end();

    for (; i != ie; i++) {
        string reg = (*i).second;
        try {
            BESRegex reg_expr(reg.c_str());
            int result = reg_expr.match(inQuestion.c_str(), inQuestion.size());
            if (result != -1) {
                if ((unsigned int)result == inQuestion.size()) {
                    cout << "expression \"" << reg << "\" matches exactly";
                } else {
                    cout << "expression \"" << reg << "\" matches " << result << " characters out of "
                         << inQuestion.size() << " characters";
                }
                if (what == "type")
                    cout << ", type = " << (*i).first;
                cout << endl;
            } else {
                cout << "expression \"" << reg << "\" does not match";
                if (what == "type")
                    cout << " for type " << (*i).first;
                cout << endl;
            }
        } catch (BESError &e) {
            string serr = (string) "malformed regular expression \"" + reg + "\": " + e.get_message();
            cout << serr << endl;
        }
    }

    return 0;
}

bool break_includes(const string &listStr, string &err) {
    string::size_type str_begin = 0;
    string::size_type str_end = listStr.size();
    string::size_type semi = 0;
    bool done = false;
    while (done == false) {
        semi = listStr.find(";", str_begin);
        if (semi == string::npos) {
            err = (string) "regular expression malformed, no semicolon";
            return false;
        } else {
            string a_member = listStr.substr(str_begin, semi - str_begin);
            str_begin = semi + 1;
            if (semi == str_end - 1) {
                done = true;
            }
            if (a_member != "") {
                expressions.insert(pair<string, string>("", a_member));
            }
        }
    }

    return true;
}

bool break_types(const string &listStr, string &err) {
    string::size_type str_begin = 0;
    string::size_type str_end = listStr.size();
    string::size_type semi = 0;
    bool done = false;
    while (done == false) {
        semi = listStr.find(";", str_begin);
        if (semi == string::npos) {
            err = (string) "type match malformed, no semicolon, " + "looking for type:regexp;[type:regexp;]";
            return false;
        } else {
            string a_pair = listStr.substr(str_begin, semi - str_begin);
            str_begin = semi + 1;
            if (semi == str_end - 1) {
                done = true;
            }

            string::size_type col = a_pair.find(":");
            if (col == string::npos) {
                err = (string) "Catalog type match malformed, no colon, " + "looking for type:regexp;[type:regexp;]";
                return false;
            } else {
                string a_type = a_pair.substr(0, col);
                string a_reg = a_pair.substr(col + 1, a_pair.size() - col);
                expressions.insert(pair<string, string>(a_type, a_reg));
            }
        }
    }

    return true;
}
