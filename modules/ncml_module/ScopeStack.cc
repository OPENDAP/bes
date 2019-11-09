//////////////////////////////////////////////////////////////////////////////
// This file is part of the "NcML Module" project, a BES module designed
// to allow NcML files to be used to be used as a wrapper to add
// AIS to existing datasets of any format.
//
// Copyright (c) 2009 OPeNDAP, Inc.
// Author: Michael Johnson  <m.johnson@opendap.org>
//
// For more information, please also see the main website: http://opendap.org/
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
// Please see the files COPYING and COPYRIGHT for more information on the GLPL.
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
/////////////////////////////////////////////////////////////////////////////

#include "ScopeStack.h"
#include "BESDebug.h"
#include "BESInternalError.h"

using std::string;
using std::endl;
using std::vector;

namespace ncml_module {
/* static */
/*  enum ScopeType { GLOBAL=0, VARIABLE_ATOMIC, VARIABLE_CONSTRUCTOR, ATTRIBUTE_ATOMIC, ATTRIBUTE_CONTAINER, NUM_SCOPE_TYPES}; */
// Make sure these match!
const string ScopeStack::Entry::sTypeStrings[NUM_SCOPE_TYPES] = { "<GLOBAL>", "<Variable_Atomic>",
    "<Variable_Constructor>", "<Attribute_Atomic>", "<Attribute_Container>", };

ScopeStack::Entry::Entry(ScopeType theType, const string& theName) :
    type(theType), name(theName)
{
    if (theType < 0 || theType >= NUM_SCOPE_TYPES) {
        BESDEBUG("ncml",
            "ScopeStack::Entry(): Invalid scope type = " << theType << " for scope name=" << theName << endl);
        throw BESInternalError("Invalid Scope Type!", __FILE__, __LINE__);
    }
}

/////////// ScopeStack impl

ScopeStack::ScopeStack() :
    _scope(0)
{
}

ScopeStack::~ScopeStack()
{
    _scope.clear();
    _scope.resize(0);
}

void ScopeStack::clear()
{
    _scope.clear();
}

void ScopeStack::pop()
{
    _scope.pop_back();
}

const ScopeStack::Entry&
ScopeStack::top() const
{
    return _scope.back();
}

bool ScopeStack::empty() const
{
    return _scope.empty();
}

int ScopeStack::size() const
{
    return _scope.size();
}

string ScopeStack::getScopeString() const
{
    string scope("");
    vector<Entry>::const_iterator iter;
    for (iter = _scope.begin(); iter != _scope.end(); iter++) {
        if (iter != _scope.begin()) {
            scope.append(".");  // append scoping operator if not first entry
        }
        scope.append((*iter).name);
    }
    return scope;
}

string ScopeStack::getTypedScopeString() const
{
    string scope("");
    vector<Entry>::const_iterator iter;
    for (iter = _scope.begin(); iter != _scope.end(); iter++) {
        if (iter != _scope.begin()) {
            scope.append(".");  // append scoping operator if not first entry
        }
        scope.append((*iter).getTypedName());
    }
    return scope;
}

bool ScopeStack::isCurrentScope(ScopeType type) const
{
    if (_scope.empty() && type == GLOBAL) {
        return true;
    }
    else if (_scope.empty()) {
        return false;
    }
    else {
        return (_scope.back().type == type);
    }
}

/////////////////////////// Non public

void ScopeStack::push(const Entry& entry)
{
    if (entry.type == GLOBAL) {
        BESDEBUG("ncml", "Logic error: can't push a GLOBAL scope type, ignoring." << endl);
    }
    else {
        _scope.push_back(entry);
    }
}
}
