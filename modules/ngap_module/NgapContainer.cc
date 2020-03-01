// NgapContainer.cc

// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of ngap_module, A C++ module that can be loaded in to
// the OPeNDAP Back-End Server (BES) and is able to handle remote requests.

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

// Authors:
//      pcw       Patrick West <pwest@ucar.edu>

#include <map>

#include <BESSyntaxUserError.h>
#include "BESNotFoundError.h"
#include <BESInternalError.h>
#include <BESDebug.h>
#include <BESUtil.h>
#include <TheBESKeys.h>
#include <WhiteList.h>

#include "NgapContainer.h"
#include "NgapApi.h"
#include "NgapUtils.h"
#include "NgapNames.h"
#include "NgapResponseNames.h"
#include "RemoteHttpResource.h"
#include "curl_utils.h"

#define prolog std::string("NgapContainer::").append(__func__).append("() - ")

using namespace std;
using namespace bes;

namespace ngap {

    string NGAP_PROVIDER_KEY("provider");
    string NGAP_DATASETS_KEY("datasets");
    string NGAP_GRANULES_KEY("granules");
    string CMR_REQUEST_BASE("https://cmr.earthdata.nasa.gov/search/granules.umm_json_v1_4");

    string CMR_PROVIDER("provider");
    string CMR_ENTRY_TITLE("entry_title");
    string CMR_NATIVE_ID("native_id");


    /**
     * We know that the for the NGAP container the path will follow the template:
     * provider/daac_name/datasets/collection_name/granules/granule_name(s?)
     * Where "provider", "datasets", and "granules" are NGAP keys and
     * "ddac_name", "collection_name", and granule_name the their respective values.
     * provider/GHRC_CLOUD/datasets/ACES_CONTINUOUS_DATA_V1/granules/aces1cont.nc

https://cmr.earthdata.nasa.gov/search/granules.umm_json_v1_4?
https://cmr.earthdata.nasa.gov/search/granules.umm_json_v1_4?
     provider=GHRC_CLOUD&entry_title=ACES_CONTINUOUS_DATA_V1&native_id=aces1cont.nc

    provider=GHRC_CLOUD&entry_title=ACES CONTINUOUS DATA V1&native_id=aces1cont_2002.191_v2.50.tar



     * @param real_name The name to decompose.
     * @param kvp The resulting key value pairs.
     */
    string convert_ngap_resty_path_to_cmr_request_url(string real_name){
        vector<string> tokens;
        BESUtil::tokenize(real_name,tokens);
        if( tokens[0]!= NGAP_PROVIDER_KEY || tokens[2]!=NGAP_DATASETS_KEY || tokens[4]!=NGAP_GRANULES_KEY){
            string err = (string) "The specified path " + real_name
                         + " does not conform to the NGAP request interface API.";
            throw BESSyntaxUserError(err, __FILE__, __LINE__);
        }

        string cmr_url = CMR_REQUEST_BASE + "?";
        cmr_url += CMR_PROVIDER + "=" + tokens[1] + "&";
        cmr_url += CMR_ENTRY_TITLE + "=" + tokens[3] + "&";
        cmr_url += CMR_NATIVE_ID + "=" + tokens[5] ;
        BESDEBUG( MODULE, prolog << "CMR Request URL: "<< cmr_url << endl );
        rapidjson::Document cmr_response = ngap_curl::http_get_as_json(cmr_url);

        rapidjson::Value& val = cmr_response["hits"];
        int hits = val.GetInt();
        if(hits < 1){
            string err = (string) "The specified path " + real_name
                         + " does not identify a thing we know about....";
            throw BESNotFoundError(err, __FILE__, __LINE__);

        }

        return "";
    }


    /** @brief Creates an instances of NgapContainer with symbolic name and real
     * name, which is the remote request.
     *
     * The real_name is the remote request URL.
     *
     * @param sym_name symbolic name representing this remote container
     * @param real_name the remote request URL
     * @throws BESSyntaxUserError if the url does not validate
     * @see NgapUtils
     */
    NgapContainer::NgapContainer(const string &sym_name,
                                 const string &real_name, const string &type) :
            BESContainer(sym_name, real_name, type), d_remoteResource(0) {

        if (type.empty())
            set_container_type("ngap");

        string cmr_request_url = convert_ngap_resty_path_to_cmr_request_url(real_name);

        // Because we know the name is really a URL, then we know the "relative_name" is meaningless
        // So we set it to be the same as "name"
        set_relative_name(real_name);
    }

/**
 * TODO: I think this implementation of the copy constructor is incomplete/inadequate. Review and fix as needed.
 */
    NgapContainer::NgapContainer(const NgapContainer &copy_from) :
            BESContainer(copy_from), d_remoteResource(copy_from.d_remoteResource) {
        // we can not make a copy of this container once the request has
        // been made
        if (d_remoteResource) {
            string err = (string) "The Container has already been accessed, "
                         + "can not create a copy of this container.";
            throw BESInternalError(err, __FILE__, __LINE__);
        }
    }

    void NgapContainer::_duplicate(NgapContainer &copy_to) {
        if (copy_to.d_remoteResource) {
            string err = (string) "The Container has already been accessed, "
                         + "can not duplicate this resource.";
            throw BESInternalError(err, __FILE__, __LINE__);
        }
        copy_to.d_remoteResource = d_remoteResource;
        BESContainer::_duplicate(copy_to);
    }

    BESContainer *
    NgapContainer::ptr_duplicate() {
        NgapContainer *container = new NgapContainer;
        _duplicate(*container);
        return container;
    }

    NgapContainer::~NgapContainer() {
        if (d_remoteResource) {
            release();
        }
    }

/** @brief access the remote target response by making the remote request
 *
 * @return full path to the remote request response data file
 * @throws BESError if there is a problem making the remote request
 */
    string NgapContainer::access() {

        BESDEBUG( MODULE, prolog << "BEGIN" << endl);

        // Since this the ngap we know that the real_name is a URL.
        string url  = get_real_name();

        BESDEBUG( MODULE, prolog << "Accessing " << url << endl);

        get_granule_path(url);

        string type = get_container_type();
        if (type == "ngap")
            type = "";

        if(!d_remoteResource) {
            BESDEBUG( MODULE, prolog << "Building new RemoteResource." << endl );
            d_remoteResource = new ngap::RemoteHttpResource(url);
            d_remoteResource->retrieveResource();
        }
        BESDEBUG( MODULE, prolog << "Located remote resource." << endl );


        string cachedResource = d_remoteResource->getCacheFileName();
        BESDEBUG( MODULE, prolog << "Using local cache file: " << cachedResource << endl );

        type = d_remoteResource->getType();
        set_container_type(type);
        BESDEBUG( MODULE, prolog << "Type: " << type << endl );


        BESDEBUG( MODULE, prolog << "Done accessing " << get_real_name() << " returning cached file " << cachedResource << endl);
        BESDEBUG( MODULE, prolog << "Done accessing " << *this << endl);
        BESDEBUG( MODULE, prolog << "END" << endl);

        return cachedResource;    // this should return the file name from the NgapCache
    }



/** @brief release the resources
 *
 * Release the resource
 *
 * @return true if the resource is released successfully and false otherwise
 */
    bool NgapContainer::release() {
        if (d_remoteResource) {
            BESDEBUG( MODULE, prolog << "Releasing RemoteResource" << endl);
            delete d_remoteResource;
            d_remoteResource = 0;
        }

        BESDEBUG( MODULE, prolog << "Done releasing Ngap response" << endl);
        return true;
    }

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with information about
 * this container.
 *
 * @param strm C++ i/o stream to dump the information to
 */
    void NgapContainer::dump(ostream &strm) const {
        strm << BESIndent::LMarg << "NgapContainer::dump - (" << (void *) this
             << ")" << endl;
        BESIndent::Indent();
        BESContainer::dump(strm);
        if (d_remoteResource) {
            strm << BESIndent::LMarg << "RemoteResource.getCacheFileName(): " << d_remoteResource->getCacheFileName()
                 << endl;
            strm << BESIndent::LMarg << "response headers: ";
            vector<string> *hdrs = d_remoteResource->getResponseHeaders();
            if (hdrs) {
                strm << endl;
                BESIndent::Indent();
                vector<string>::const_iterator i = hdrs->begin();
                vector<string>::const_iterator e = hdrs->end();
                for (; i != e; i++) {
                    string hdr_line = (*i);
                    strm << BESIndent::LMarg << hdr_line << endl;
                }
                BESIndent::UnIndent();
            } else {
                strm << "none" << endl;
            }
        } else {
            strm << BESIndent::LMarg << "response not yet obtained" << endl;
        }
        BESIndent::UnIndent();
    }


    void NgapContainer::get_granule_path(const string &ppath) const {
        /*enum RestifiedPathValues { cmrProvider, cmrDatasets, cmrGranuleUR };
        static std::map<std::string, RestifiedPathValues> mapRestifiedPathValues;*/

        string path = BESUtil::normalize_path(ppath, true, false);
        vector<string> path_elements = BESUtil::split(path);
        BESDEBUG(MODULE, prolog << "path: '" << path << "'   path_elements.size(): " << path_elements.size() << endl);

        string epoch_time = BESUtil::get_time(0, false);

        NgapApi ngapApi;
        //bes::CatalogNode *node;

        if (path_elements.empty()) {
            /*node = new CatalogNode("/");
            node->set_lmt(epoch_time);
            node->set_catalog_name(CMR_CATALOG_NAME);
            for(size_t i=0; i<d_collections.size() ; i++){
                CatalogItem *collection = new CatalogItem();
                collection->set_name(d_collections[i]);
                collection->set_type(CatalogItem::node);
                node->add_node(collection);
            }*/
        } else {
            for (size_t i = 0; i < path_elements.size(); i++) {
                if (path_elements[i] == "-")
                    path_elements[i] = "";
            }

            bool valid_provider = false;
            bool valid_dataset = false;
            bool valid_granule = false;
            string provider = "";
            string dataset = "";
            string granuleUrl = "";
            string facet;

            for (size_t i = 0; i < path_elements.size(); i++) {
                BESDEBUG(MODULE, prolog << "Checking facet: " << path_elements[i] << endl);

                facet = BESUtil::lowercase(path_elements[i]);
                if (facet == "provider") {
                    valid_provider = true;
                    provider = path_elements[++i];
                } else if (facet == "datasets") {
                    valid_dataset = true;
                    dataset = path_elements[++i];
                } else if (facet == "granule_ur") {
                    valid_granule = true;
                    granuleUrl = path_elements[++i];
                } else {
                    throw BESNotFoundError("No such resource: " + path, __FILE__, __LINE__);
                }
            }

            /*for (size_t i = 0; i < path_elements.size(); i + 2) {

                BESDEBUG(MODULE, prolog << "Checking facet: " << path_elements[i] << endl);
                facet = BESUtil::lowercase(path_elements[i]);

                switch (mapRestifiedPathValues(facet)) {
                    case cmrProvider:
                        valid_provider = true;
                        provider = path_elements[i + 1];
                        break;
                    case cmrDatasets:
                        valid_dataset = true;
                        dataset = path_elements[i + 1];
                        break;
                    case cmrGranuleUR:
                        valid_granule = true;
                        granuleUrl = path_elements[i + 1];
                        break;
                    default:
                        throw BESNotFoundError("No such CMR facet: " + facet, __FILE__, __LINE__);
                }
            }*/

            if (valid_provider && valid_dataset && valid_granule) {

                BESDEBUG(MODULE, prolog << "Request resolved to leaf provider:" << provider << " datasets:" << dataset
                                        << " granule:" << granuleUrl << endl);
                /*Granule *granule = NgapApi.get_granule(collection,year,month,day,granule_id);
                if(granule){
                    *//*CatalogItem *granuleItem = new CatalogItem();
                granuleItem->set_type(CatalogItem::leaf);
                granuleItem->set_name(granule->getName());
                granuleItem->set_is_data(true);
                granuleItem->set_lmt(granule->getLastModifiedStr());
                granuleItem->set_size(granule->getSize());
                node->set_leaf(granuleItem);*//*
            }
            else {
                throw BESNotFoundError("No such resource: "+path,__FILE__,__LINE__);
            }*/

            }
        }
    }
}
