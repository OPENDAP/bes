// NCMLContainer.cc

// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of ncml_module, A C++ module that can be loaded in to
// the OPeNDAP Back-End Server (BES) and is able to handle ncml requests.

// Copyright (c) 2002,2003 OPeNDAP, Inc.
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
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

// (c) COPYRIGHT URI/MIT 1994-1999
// Please read the full copyright statement in the file COPYRIGHT_URI.
//
// Authors:
//      pcw       Patrick West <pwest@ucar.edu>

#include "config.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <fstream>
#include <cerrno>
#include <cstdlib>
#include <cstring>

#if 0
using std::ofstream;
using std::endl;
using std::ios_base;
#endif

#include "NCMLContainer.h"
#include "NCMLContainerStorage.h"

#include <BESSyntaxUserError.h>
#include <BESInternalError.h>
#include <BESDebug.h>

#include <libdap/util.h>       // libdap; for open_temp_fstream()

using namespace std;

/** @brief Creates an instances of NCMLContainer with the symbolic name
 * and the xml document string.
 *
 * The xml document is contained within the DHI, so must be passed in
 * here.
 *
 * The type of the container is ncml. This way we know to use the ncml
 * handler for this container.
 *
 * @param sym_name symbolic name representing this NCML container
 * @param ncml_doc string of the xml document
 * @see NCMLGatewayUtils
 */
NCMLContainer::NCMLContainer(const string &sym_name, const string &xml_doc) :
    BESContainer(sym_name, "", "ncml"), _xml_doc(xml_doc), _accessed(false)
{
}

NCMLContainer::NCMLContainer(const NCMLContainer &copy_from) :
    BESContainer(copy_from), _xml_doc(copy_from._xml_doc), _accessed(copy_from._accessed)
{
    // we can not make a copy of this container once the NCML document has
    // been written to the temporary file
    if (_accessed) {
        string err = (string) "The Container has already been accessed, " + "can not create a copy of this container.";
        throw BESInternalError(err, __FILE__, __LINE__);
    }
}

void NCMLContainer::_duplicate(NCMLContainer &copy_to)
{
    if (copy_to._accessed) {
        string err = (string) "The Container has already been accessed, " + "can not duplicate this resource.";
        throw BESInternalError(err, __FILE__, __LINE__);
    }
    copy_to._xml_doc = _xml_doc;
    copy_to._accessed = false;
    BESContainer::_duplicate(copy_to);
}

BESContainer *
NCMLContainer::ptr_duplicate()
{
    NCMLContainer *container = new NCMLContainer;
    _duplicate(*container);
    return container;
}

NCMLContainer::~NCMLContainer()
{
    if (_accessed) {
        release();
    }
}

/** @brief access the NCML target response by making the NCML request
 *
 * @return full path to the NCML request response data file
 * @throws BESError if there is a problem making the NCML request
 */
string NCMLContainer::access()
{
    BESDEBUG("ncml", "accessing " << _xml_doc << endl);
    if (!_accessed) {
        // save the xml document to a temporary file, open it, unlink
        // it. In release, close the file. This will remove the file if
        // it is no longer open.
#if 0
        // Replaced with code that uses mkstemp() which lacks the security
        // problems of mktemp(). jhrg 5/13/15
        string tempfile_template = NCMLContainerStorage::NCML_TempDir + "/ncml_module_XXXXXX";
        char *tempfile_c = mktemp((char *) tempfile_template.c_str());

        string tempfile;
        if (tempfile_c) {
            tempfile = tempfile_c;
        }
        else {
            string err = (string) "Unable to create temporary ncml document " + _tmp_file_name;
            throw BESInternalError(err, __FILE__, __LINE__);
        }

        _tmp_file_name = NCMLContainerStorage::NCML_TempDir + "/" + tempfile + ".ncml";
#endif
        // FIXME Don't close the stream 'ostrm' and instead store it in a field of the
        // object. see the class comment. jhrg 8/12/15

        // FIXME! If we use this code, switch to the BESHandlerUtil.h/cc code that
        // supports temp files. Or use the code in libdap that supports mkstemps() and
        // does so in a way the avoids portability problems. THe libdap code (mkstemps)
        // code supports temp files with extensions (not that big a deal, but it does).
        // jhrg 8/25/17

        ofstream ostrm;
        string _tmp_file_name = libdap::open_temp_fstream(ostrm,
            NCMLContainerStorage::NCML_TempDir + "/ncml_module_XXXXXX", ".ncml");

        if (!ostrm) {
            string err = (string) "Unable to write out the ncml document " + _tmp_file_name;
            if (errno) {
                char *str = strerror(errno);
                if (str) err += (string) " " + str;
            }
            throw BESInternalError(err, __FILE__, __LINE__);
        }

        // write out <?xml version="1.0" encoding="UTF-8"?>
        ostrm << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl;

        // then write out the r_name as the ncml document (no validation)
        ostrm << _xml_doc << endl;

        ostrm.close();

        _accessed = true;
    }

    return _tmp_file_name;
}

/** @brief release the NCML cached resources
 *
 * Release the resource
 *
 * @return true if the resource is released successfully and false otherwise
 */
bool NCMLContainer::release()
{
    if (_accessed && !_tmp_file_name.empty()) {
        unlink(_tmp_file_name.c_str());
        _tmp_file_name = "";
    }
    _accessed = false;
    return true;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with information about
 * this container.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void NCMLContainer::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "NCMLContainer::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    if (_accessed) {
        strm << BESIndent::LMarg << "temporary file: " << _tmp_file_name << endl;
    }
    else {
        strm << BESIndent::LMarg << "temporary file: not open" << endl;
    }
    BESContainer::dump(strm);
    BESIndent::UnIndent();
}

