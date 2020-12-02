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

#include "BaseType.h"
#include "D4Group.h"

#include "BESInternalError.h"
#include "BESDebug.h"

#include "DmrppNames.h"
#include "DMRpp.h"
#include "DmrppD4Group.h"
#include "DmrppArray.h"

#include "SuperChunky.h"

#define prolog std::string("SuperChunky::").append(__func__).append("() - ")

namespace dmrpp {




void inventory_super_chunks(libdap::BaseType *var, bool only_constrained){
        if(var->is_simple_type())
            return;
        if(var->is_constructor_type())
            return;
        if(var->is_vector_type()){
            auto *array = dynamic_cast<DmrppArray*>(var);
            if(array){
                // Now we get the chunkyness
                auto &chunk_dim_sizes = array->get_chunk_dimension_sizes();
                unsigned int chunk_size_in_elements = array->get_chunk_size_in_elements();
                auto &chunks = array->get_immutable_chunks();
                unsigned long long last_offset = 0;
                bool is_contiguous = true;
                bool conty;

                unsigned long long super_chunk_index = 0;
                vector<vector<const Chunk &> *> super_chunks;
                super_chunks.push_back(new vector<const Chunk &>()); // first super chunk...
                for(auto &chunk:chunks){
                    unsigned long long offset = chunk.get_offset();
                    unsigned long long size = chunk.get_size();
                    auto &c_pia = chunk.get_position_in_array();
                    conty = offset==last_offset;
                    if(!conty){
                        unsigned long long gap_size = offset - last_offset;
                        BESDEBUG(MODULE, prolog << "FOUND GAP  offset: " << offset <<
                        " nbytes: " << size << " next_offset: " << last_offset << " gap_size: " << gap_size<< endl);
                    }
                    else if(is_contiguous){
                        super_chunks[super_chunk_index].push_back(chunk);
                    }
                    is_contiguous = is_contiguous && conty;
                    last_offset = offset + size;
                }
            }
            else {
                BESDEBUG(MODULE, prolog << " ERROR! The variable: "<< var->name()
                << " is not an instance of DmrppArray. SKIPPING"<< endl);
            }

        }
    }
    void inventory_super_chunks(libdap::D4Group *group, bool only_constrained){

        // Process Groups - RECURSION HAPPENS HERE.
        auto gtr = group->grp_begin();
        while(gtr!=group->grp_end()){
            inventory_super_chunks(*gtr++, only_constrained);
        }

        // Process Vars
        auto vtr = group->var_begin();
        while(vtr!=group->var_end()){
            libdap::BaseType *bt = *vtr;
        }

    }

    void inventory_super_chunks(DMRpp &dmr, bool only_constrained){
        inventory_super_chunks(dmr.root(), only_constrained);
    }


} // namespace dmrpp
