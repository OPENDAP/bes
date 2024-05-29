// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES
// Copyright (c) 2020 OPeNDAP, Inc.
// Author: Nathan Potter<ndp@opendap.org>
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
//
// Created by ndp on 12/2/20.
//

#include "config.h"

#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>

#include <libdap/BaseType.h>
#include <libdap/D4Group.h>

#include "BESInternalError.h"
#include "BESInternalFatalError.h"
#include "BESDebug.h"
#include "BESUtil.h"
#include "TheBESKeys.h"
#include "BESLog.h"
#include "BESStopWatch.h"
#include "BESIndent.h"

#include "DmrppNames.h"
#include "DMRpp.h"
#include "DmrppD4Group.h"
#include "DmrppArray.h"
#include "DmrppParserSax2.h"
#include "DmrppTypeFactory.h"

#include "SuperChunk.h"

#define prolog std::string("superchunky::").append(__func__).append("() - ")

namespace dmrpp {

bool debug = true;

void compute_super_chunks(dmrpp::DmrppArray *array, bool /*only_constrained*/, vector<SuperChunk *> &super_chunks){

        // Now we get the chunkyness
        auto chunk_dim_sizes = array->get_chunk_dimension_sizes();
        //unsigned int chunk_size_in_elements = array->get_chunk_size_in_elements();
        auto const &chunks = array->get_immutable_chunks();

        unsigned long long sc_count=0;
        stringstream sc_id;
        sc_id << array->name() << "-" << sc_count++;

        //unsigned long long super_chunk_index = 0;
        auto currentSuperChunk = new SuperChunk(sc_id.str(), array);
        super_chunks.push_back(currentSuperChunk); // first super chunk...
        if(debug) cout << "SuperChunking array: "<< array->name() << endl;

        for(const auto &chunk:chunks){
            bool was_added = currentSuperChunk->add_chunk(chunk);
            if(!was_added){
                if(debug) {
                    unsigned long long next_contiguous_chunk_offset = currentSuperChunk->get_offset() + currentSuperChunk->get_size();
                    unsigned long long gap_size;
                    bool is_behind = false;
                    if(chunk->get_offset() > next_contiguous_chunk_offset){
                        gap_size = chunk->get_offset() - next_contiguous_chunk_offset;
                    }
                    else {
                        is_behind = true;
                        gap_size = next_contiguous_chunk_offset - chunk->get_offset();
                    }
                    stringstream msg;
                    msg << "FOUND GAP chunk(offset: " << chunk->get_offset();
                    msg << " size: " << chunk->get_size() << ")";
                    msg << " SuperChunk(ptr: " << (void *) currentSuperChunk;
                    msg << " offset: " << currentSuperChunk->get_offset();
                    msg << " size: " << currentSuperChunk->get_size();
                    msg << " next_contiguous_chunk_offset: " << next_contiguous_chunk_offset << ") ";
                    msg << " gap_size: " << gap_size;
                    msg << " bytes" << (is_behind?" behind":" beyond") << " target offset";
                    msg << endl;
                    cerr << msg.str();
                }
                // If we were working on a SuperChunk (i.e. the current SuperChunk contains chunks)
                // then we need to start a new one.
                if(!currentSuperChunk->empty()){
                    sc_id.str(std::string());
                    sc_id << array->name() << "-" << sc_count++;
                    currentSuperChunk = new SuperChunk(sc_id.str(), array);
                    super_chunks.push_back(currentSuperChunk); // next super chunk...
                }
                bool add_first_successful = currentSuperChunk->add_chunk(chunk);
                if(!add_first_successful)
                    throw BESInternalError("ERROR: Failed to add first Chunk to a new SuperChunk."+
                            chunk->to_string() ,__FILE__,__LINE__);

            }
        }
        // Dump the currentSuperChunk if it doesn't have anything in it.
        if(currentSuperChunk->empty()) {
            super_chunks.pop_back();
            delete currentSuperChunk;
        }
        if(false){
            cout << "SuperChunk Inventory For Array: " << array->name() << endl;
            for(auto super_chunk: super_chunks) {
                cout << super_chunk->to_string(true) << endl;
            }
        }
}

void compute_super_chunks(libdap::BaseType *var, bool only_constrained, vector<SuperChunk *> &super_chunks) {
    if (var->is_simple_type())
        return;
    if (var->is_constructor_type())
        return;
    if (var->is_vector_type()) {
        auto array = dynamic_cast<dmrpp::DmrppArray *>(var);
        if (array) {
            if(debug) cout << "Found DmrppArray: "<< array->name() << endl;
            compute_super_chunks(array, only_constrained, super_chunks);
        }
        else {
            BESDEBUG(MODULE, prolog << "The variable: "<< var->name()
                                    << " is not an instance of DmrppArray. SKIPPING"<< endl);
        }
    }
}

    void inventory_super_chunks(libdap::D4Group *group, bool only_constrained, vector<SuperChunk *> &super_chunks){

        // Process Groups - RECURSION HAPPENS HERE.
        auto gtr = group->grp_begin();
        while(gtr!=group->grp_end()){
            if(debug) cout << "Found Group: "<< (*gtr)->name() << endl;
            inventory_super_chunks(*gtr++, only_constrained, super_chunks);
        }

        // Process Vars
        auto vtr = group->var_begin();
        while(vtr!=group->var_end()){
            if(debug) cout << "Found Variable: "<< (*vtr)->type_name() << " " << (*vtr)->name() << endl;
            compute_super_chunks(*vtr++, only_constrained, super_chunks);
            //inventory_super_chunks(*vtr++, only_constrained);
        }
    }

    void inventory_super_chunks(DMRpp &dmr, bool only_constrained, vector<SuperChunk *> &super_chunks){
        inventory_super_chunks(dmr.root(), only_constrained, super_chunks);
    }

    dmrpp::DMRpp *get_dmrpp(const string dmrpp_filename){
        ifstream dmrpp_ifs (dmrpp_filename);
        if (dmrpp_ifs.is_open())
        {
            dmrpp::DmrppParserSax2  parser;
            dmrpp::DmrppTypeFactory factory;
            auto dmr = new DMRpp(&factory,dmrpp_filename);
            parser.intern(dmrpp_ifs, dmr);
            return dmr;
        }
        else {
            throw BESInternalFatalError("The provided file could not be opened. filename: '"+dmrpp_filename+"'",__FILE__,__LINE__);
        }
    }

    void inventory_super_chunks(const string dmrpp_filename){
        cout << "DMR++ file:  " << dmrpp_filename << endl;
        dmrpp::DMRpp *dmr = get_dmrpp(dmrpp_filename);

        vector<SuperChunk *> super_chunks;

        {
            BESStopWatch sw;
            sw.start(prolog);
            dmrpp::inventory_super_chunks(*dmr, false, super_chunks);
        }

        cout << "DMR++ file:  " << dmrpp_filename << endl;
        cout << "Produced " << super_chunks.size() << " SuperChunks." << endl;
        for(auto super_chunk: super_chunks) {
            cout << super_chunk->to_string(true) << endl;
        }

        delete dmr;
    }

    void dump_vars(libdap::D4Group *group){
        // Process Groups - RECURSION HAPPENS HERE.
        auto gtr = group->grp_begin();
        while(gtr!=group->grp_end()){
            if(debug) cout << "Found Group: "<< (*gtr)->name() << endl;
            dump_vars(*gtr++);
        }

        // Process Vars
        auto vtr = group->var_begin();
        while(vtr!=group->var_end()){
            libdap::BaseType *bt = *vtr++;
            bt->dump(cout);
            cout << endl;
        }
    }

    void dump_vars(DMRpp &dmr){
        dump_vars(dmr.root());
    }
} // namespace dmrpp

int main(int argc, char *argv[]) {
    string bes_log_file("superchunky_bes.log");
    string prefix;
    string cache_effective_urls("false");
    char *prefixCstr = getenv("prefix");
    if (prefixCstr) {
        prefix = prefixCstr;
    } else {
        prefix = "/";
    }

    cout << "bes_log_file: " << bes_log_file << endl;

    auto bes_config_file = BESUtil::assemblePath(prefix, "/etc/bes/bes.conf", true);
    TheBESKeys::ConfigFile = bes_config_file; // Set the config file for TheBESKeys
    TheBESKeys::TheKeys()->set_key("BES.LogName", bes_log_file); // Set the log file so it goes where we say.
    TheBESKeys::TheKeys()->set_key("AllowedHosts", "^https?:\\/\\/.*$", false); // Set AllowedHosts to allow any URL
    TheBESKeys::TheKeys()->set_key("AllowedHosts", "^file:\\/\\/\\/.*$", true); // Set AllowedHosts to allow any file
    TheBESKeys::TheKeys()->set_key("Http.cache.effective.urls", cache_effective_urls, false); // Set AllowedHosts to allow any file

    BESIndent::SetIndent("");

    for(auto i=1; i<argc; i++){
        string dmrpp_filename(argv[i]);

        dmrpp::DMRpp *dmrpp = dmrpp::get_dmrpp( dmrpp_filename);
        dump_vars(*dmrpp);
    }
    return 0;
}



