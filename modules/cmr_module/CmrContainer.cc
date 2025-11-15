// CmrContainer.cc

// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of cnr_module, A C++ module that can be loaded in to
// the OPeNDAP Back-End Server (BES) and is able to handle remote requests.

// Copyright (c) 2018 OPeNDAP, Inc.
// Author: Nathan Potter <ndp@opendap.org>
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

// Authors:
//      ndp       Nathan Potter <ndp@opendap.org>
# include "config.h"

#include <sstream>

#include <BESSyntaxUserError.h>
#include <BESInternalError.h>
#include <BESNotFoundError.h>
#include <BESDebug.h>
#include <BESUtil.h>
#include <TheBESKeys.h>
#include <AllowedHosts.h>
#include "RemoteResource.h"

#include "CmrContainer.h"
#include "CmrNames.h"
#include "CmrApi.h"

using namespace std;
using namespace bes;

#define prolog std::string("CmrContainer::").append(__func__).append("() - ")

namespace cmr {

/** @brief Creates an instances of CmrContainer with symbolic name and real
 * name, which is the remote request.
 *
 * The real_name is the remote request URL.
 *
 * @param sym_name symbolic name representing this remote container
 * @param real_name the virtual CMR path to a dataset or file.
 * @throws BESSyntaxUserError if the path does not validate
 * @see CmrUtils
 */
CmrContainer::CmrContainer(const string &sym_name,
                           const string &real_name, const string &type) :
        BESContainer(sym_name, real_name, type), d_remoteResource(0) {

    BESDEBUG(MODULE, prolog << "BEGIN sym_name: " << sym_name
                            << " real_name: " << real_name << " type: " << type << endl);


    string path = BESUtil::normalize_path(real_name, true, false);
    vector<string> path_elements = BESUtil::split(path);
    BESDEBUG(MODULE, prolog << "path: '" << path << "'  path_elements.size(): " << path_elements.size() << endl);


    set_relative_name(path);

    if (type == "") {
        // @TODO FIX Dynamically determine the type from the Granule information (type-match to name, mime-type, etc)
        this->set_container_type("nc");
    }


    BESDEBUG( MODULE, prolog << "END" << endl);

}

/**
 * TODO: I think this implementation of the copy constructor is incomplete/inadequate. Review and fix as needed.
 */
CmrContainer::CmrContainer(const CmrContainer &copy_from) :
        BESContainer(copy_from), d_remoteResource(copy_from.d_remoteResource) {
    // we can not make a copy of this container once the request has
    // been made
    if (d_remoteResource) {
        string err = (string) "The Container has already been accessed, "
                + "can not create a copy of this container.";
        throw BESInternalError(err, __FILE__, __LINE__);
    }
}

void CmrContainer::_duplicate(CmrContainer &copy_to) {
    if (copy_to.d_remoteResource) {
        string err = (string) "The Container has already been accessed, "
                + "can not duplicate this resource.";
        throw BESInternalError(err, __FILE__, __LINE__);
    }
    copy_to.d_remoteResource = d_remoteResource;
    BESContainer::_duplicate(copy_to);
}

BESContainer *
CmrContainer::ptr_duplicate() {
    auto *container = new CmrContainer;
    _duplicate(*container);
    return container;
}

CmrContainer::~CmrContainer() {
    if (d_remoteResource) {
        release();
    }
}

/** @brief access the remote target response by making the remote request
 *
 * @return full path to the remote request response data file
 * @throws BESError if there is a problem making the remote request
 */
string CmrContainer::access() {

    BESDEBUG( MODULE, prolog << "BEGIN" << endl);

    // Since this the CMR thang we know that the real_name is a path of facets and such.
    string path  = get_real_name();
    BESDEBUG( MODULE, prolog << "path: " << path << endl);

    auto granule = getTemporalFacetGranule(path);
    if (!granule) {
        throw BESNotFoundError("Failed to locate a granule associated with the path " + path, __FILE__, __LINE__);
    }
    string granule_url = granule->getDataGranuleUrl();

    if(!d_remoteResource) {
        BESDEBUG( MODULE, prolog << "Building new RemoteResource." << endl );
        shared_ptr<http::url> target_url(new http::url(granule_url,true));
        d_remoteResource = new http::RemoteResource(target_url);
        d_remoteResource->retrieve_resource();
    }
    BESDEBUG( MODULE, prolog << "Retrieved RemoteResource." << endl );

    string cachedResource = d_remoteResource->get_filename();
    BESDEBUG( MODULE, prolog << "Using local cache file: " << cachedResource << endl );

    string type = d_remoteResource->get_type();
    set_container_type(type);
    BESDEBUG( MODULE, prolog << "Type: " << type << endl );

    BESDEBUG( MODULE, prolog << "Done accessing " << get_real_name() << " returning cached file " << cachedResource << endl);
    BESDEBUG( MODULE, prolog << "Done accessing " << *this << endl);
    BESDEBUG( MODULE, prolog << "END" << endl);

    return cachedResource;    // this should return the file name from the CmrCache
}



/** @brief release the resources
 *
 * Release the resource
 *
 * @return true if the resource is released successfully and false otherwise
 */
bool CmrContainer::release() {
    BESDEBUG( MODULE, prolog << "BEGIN" << endl);
    if (d_remoteResource) {
        BESDEBUG( MODULE, prolog << "Releasing RemoteResource" << endl);
        delete d_remoteResource;
        d_remoteResource = nullptr;
    }
    BESDEBUG( MODULE, prolog << "END" << endl);
    return true;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with information about
 * this container.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void CmrContainer::dump(ostream &strm) const {
    strm << BESIndent::LMarg << prolog << "(" << (void *) this
            << ")" << endl;
    BESIndent::Indent();
    BESContainer::dump(strm);
    if (d_remoteResource) {
        strm << BESIndent::LMarg << "RemoteResource.get_filename(): " << d_remoteResource->get_filename()
                << endl;
     } else {
        strm << BESIndent::LMarg << "response not yet obtained" << endl;
    }
    BESIndent::UnIndent();
}

/**
 *
 * @param granule_path
 * @return
 */
unique_ptr<Granule> CmrContainer::getTemporalFacetGranule(const std::string &granule_path)
{
    const unsigned int PATH_SIZE = 7;

    BESDEBUG(MODULE, prolog << "BEGIN  (granule_path: '" << granule_path  << ")" << endl);

    string path = BESUtil::normalize_path(granule_path,false, false);
    vector<string> path_elements = BESUtil::split(path);
    BESDEBUG(MODULE, prolog << "path: '" << path << "'   path_elements.size(): " << path_elements.size() << endl);

    if(path_elements.size() != PATH_SIZE){
        stringstream msg;
        msg << "The path component: '" << granule_path << "' of your request has ";
        msg << (path_elements.size()<PATH_SIZE?"too few components. ":"too many components. ");
        msg << "I was expecting " << PATH_SIZE << " elements but I found " << path_elements.size() << ". ";
        msg << "I was unable to locate a granule from what you provided.";
        throw BESNotFoundError(msg.str(),__FILE__,__LINE__);
    }

    // We don't need the provider_id for this operation, so commented out.
    // string provider = path_elements[0];
    // BESDEBUG(MODULE, prolog << "  provider: '" << provider << endl);

    string collection = path_elements[1];
    BESDEBUG(MODULE, prolog << "collection: '" << collection << endl);
    string facet = path_elements[2];
    BESDEBUG(MODULE, prolog << "     facet: '" << facet << endl);
    string year = path_elements[3];
    BESDEBUG(MODULE, prolog << "      year: '" << year << endl);
    string month = path_elements[4];
    BESDEBUG(MODULE, prolog << "     month: '" << month << endl);
    string day = path_elements[5];
    BESDEBUG(MODULE, prolog << "       day: '" << day << endl);
    string granule_id = path_elements[6];
    BESDEBUG(MODULE, prolog << "granule_id: '" << granule_id << endl);

    CmrApi cmrApi;
    return cmrApi.get_granule( collection, year, month, day, granule_id);
}



} // namespace cmr
