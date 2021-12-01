// -*- mode: c++; c-basic-offset:4 -*-
//
// StringStream.h
//
// This file is part of BES CovJSON File Out Module
//
// Copyright (c) 2018 OPeNDAP, Inc.
// Author: Corey Hemphill <hemphilc@oregonstate.edu>
// Author: River Hendriksen <hendriri@oregonstate.edu>
// Author: Riley Rimer <rrimer@oregonstate.edu>
//
// Adapted from the File Out JSON module implemented by Nathan Potter
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

#ifndef STREAMSTRING_H_
#define STREAMSTRING_H_

#include <libdap/Str.h>

namespace libdap {

class StreamString: public Str{

public:
    StreamString(const string &n): Str(n) {};
    StreamString(const string &n, const string &d): Str(n,d) {};
    virtual ~StreamString() {};
    StreamString(const Str &copy_from): Str(copy_from) {};

    friend ostream& operator<<(ostream& out, const Str& s) // output
    {
        out <<  s.value();

	return out;
    }

    friend istream& operator>>(istream& in, Str& s) // input
    {
	string tmp;
	in >> tmp;
	s.set_value(tmp);

	return in;
    }
};

} /* namespace libdap */

#endif /* STREAMSTRING_H_ */
