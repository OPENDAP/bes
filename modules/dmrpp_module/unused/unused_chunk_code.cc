#if 0
    switch (dimensions()) {

    //########################### OneD Arrays ###############################
    case 1: {
        for(unsigned long i=0; i<chunk_refs->size(); i++) {
            Chunk h4bs = (*chunk_refs)[i];
            BESDEBUG("dmrpp", "----------------------------------------------------------------------------" << endl);
            BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - Processing chunk[" << i << "]: BEGIN" << endl);
            BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - " << h4bs.to_string() << endl);

            if (!is_projected()) {  // is there a projection constraint?
                BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - ################## There is no constraint. Getting entire chunk." << endl);
                // Apparently not, so we send it all!
                // read the data
                h4bs.read();
                // Get the source (read) and write (target) buffers.
                char * source_buffer = h4bs.get_rbuf();
                char * target_buffer = get_buf();
                // Compute insertion point for this chunk's inner (only) row data.
                vector<unsigned int> start_position = h4bs.get_position_in_array();
                unsigned long long start_char_index = start_position[0] * prototype()->width();
                BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - start_char_index: " << start_char_index << " start_position: " << start_position[0] << endl);
                // if there is no projection constraint then just copy those bytes.
                memcpy(target_buffer+start_char_index, source_buffer, h4bs.get_rbuf_size());
            }
            else {

                BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - Found constraint expression." << endl);

                // There is a projection constraint.
                // Recover constraint information for this (the first and only) dim.
                Dim_iter this_dim = dim_begin();
                unsigned int ce_start = dimension_start(this_dim,true);
                unsigned int ce_stride = dimension_stride(this_dim,true);
                unsigned int ce_stop = dimension_stop(this_dim,true);
                BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - ce_start:  " << ce_start << endl);
                BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - ce_stride: " << ce_stride << endl);
                BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - ce_stop:   " << ce_stop << endl);

                // Gather chunk information
                unsigned int chunk_inner_row_start = h4bs.get_position_in_array()[0];
                unsigned int chunk_inner_row_size = get_chunk_dimension_sizes()[0];
                unsigned int chunk_inner_row_end = chunk_inner_row_start + chunk_inner_row_size;
                BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - chunk_inner_row_start: " << chunk_inner_row_start << endl);
                BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - chunk_inner_row_size:  " << chunk_inner_row_size << endl);
                BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - chunk_inner_row_end:   " << chunk_inner_row_end << endl);

                // Do we even want this chunk?
                if( ce_start > chunk_inner_row_end || ce_stop < chunk_inner_row_start) {
                    // No, no we don't. Skip this.
                    BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - Chunk not accessed by CE. SKIPPING." << endl);
                }
                else {
                    // We want this chunk.
                    // Read the chunk.
                    h4bs.read();

                    unsigned long long chunk_start_element, chunk_end_element;

                    // In this next bit we determine the first element in this
                    // chunk that matches the subset expression.
                    int first_element_offset = 0;
                    if(ce_start < chunk_inner_row_start) {
                        // This case means that we must find the first element matching
                        // a stride that started at ce_start, somewhere up the line from
                        // this chunk. So, we subtract the ce_start from the chunk start to align
                        // the values for modulo arithmetic on the stride value.
                        BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - chunk_inner_row_start: " << chunk_inner_row_start << endl);

                        if(ce_stride!=1)
                            first_element_offset = ce_stride - (chunk_inner_row_start - ce_start) % ce_stride;
                        BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - first_element_offset: " << first_element_offset << endl);
                    }
                    else {
                        first_element_offset = ce_start - chunk_inner_row_start;
                        BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - ce_start is in this chunk. first_element_offset: " << first_element_offset << endl);
                    }

                    chunk_start_element = (chunk_inner_row_start + first_element_offset);
                    BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - chunk_start_element: " << chunk_start_element << endl);

                    // Now we figure out the correct last element, based on the subset expression
                    chunk_end_element = chunk_inner_row_end;
                    if(ce_stop<chunk_inner_row_end) {
                        chunk_end_element = ce_stop;
                        BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - ce_stop is in this chunk. " << endl);
                    }
                    BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - chunk_end_element: " << chunk_end_element << endl);

                    // Compute the read() address and length for this chunk's inner (only) row data.
                    unsigned long long element_count = chunk_end_element - chunk_start_element + 1;
                    BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - element_count: " << element_count << endl);
                    unsigned long long length = element_count * prototype()->width();
                    BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - length: " << length << endl);

                    if(ce_stride==1) {
                        BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - ################## Copy contiguous block mode." << endl);
                        // Since the stride is 1 we are getting everything between start
                        // and stop, so memcopy!
                        // Get the source (read) and write (target) buffers.
                        char * source_buffer = h4bs.get_rbuf();
                        char * target_buffer = get_buf();
                        unsigned int target_char_start_index = ((chunk_start_element - ce_start) / ce_stride ) * prototype()->width();
                        BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - target_char_start_index: " << target_char_start_index << endl);
                        unsigned long long source_char_start_index = first_element_offset * prototype()->width();
                        BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - source_char_start_index: " << source_char_start_index << endl);
                        // if there is no projection constraint then just copy those bytes.
                        memcpy(target_buffer+target_char_start_index, source_buffer + source_char_start_index,length);
                    }
                    else {

                        BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - ##################  Copy individual elements mode." << endl);

                        // Get the source (read) and write (target) buffers.
                        char * source_buffer = h4bs.get_rbuf();
                        char * target_buffer = get_buf();
                        // Since stride is not equal to 1 we have to copy individual values
                        for(unsigned int element_index=chunk_start_element; element_index<=chunk_end_element;element_index+=ce_stride) {
                            BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - element_index: " << element_index << endl);
                            unsigned int target_char_start_index = ((element_index - ce_start) / ce_stride ) * prototype()->width();
                            BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - target_char_start_index: " << target_char_start_index << endl);
                            unsigned int chunk_char_start_index = (element_index - chunk_inner_row_start) * prototype()->width();
                            BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - chunk_char_start_index: " << chunk_char_start_index << endl);
                            memcpy(target_buffer+target_char_start_index, source_buffer + chunk_char_start_index,prototype()->width());
                        }
                    }
                }
            }
            BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - Processing chunk[" << i << "]:  END" << endl);
        }
        BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - END ##################  END" << endl);

    }break;
    //########################### TwoD Arrays ###############################
    case 2: {
        if(!is_projected()) {
            BESDEBUG("dmrpp", "DmrppArray::"<< __func__ << "() - 2D Array. No CE Found. Reading " << chunk_refs->size() << " chunks" << endl);
            char * target_buffer = get_buf();
            vector<unsigned int> chunk_shape = get_chunk_dimension_sizes();
            for(unsigned long i=0; i<chunk_refs->size(); i++) {
                BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - READING chunk[" << i << "]: " << (*chunk_refs)[i].to_string() << endl);
                Chunk h4bs = (*chunk_refs)[i];
                h4bs.read();
                char * source_buffer = h4bs.get_rbuf();
                vector<unsigned int> chunk_origin = h4bs.get_position_in_array();
                vector<unsigned int> chunk_row_address = chunk_origin;
                unsigned long long target_element_index = get_index(chunk_origin,array_shape);
                unsigned long long target_char_index = target_element_index * prototype()->width();
                unsigned long long source_element_index = 0;
                unsigned long long source_char_index = source_element_index * prototype()->width();
                unsigned long long chunk_inner_dim_bytes = chunk_shape[1] * prototype()->width();
                BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - Packing Array From Chunks: "
                    << " chunk_inner_dim_bytes: " << chunk_inner_dim_bytes << endl);

                for(unsigned int i=0; i<chunk_shape[0];i++) {
                    BESDEBUG("dmrpp", "DmrppArray::" << __func__ << "() - "
                        "target_char_index: " << target_char_index <<
                        " source_char_index: " << source_char_index << endl);
                    memcpy(target_buffer+target_char_index, source_buffer+source_char_index, chunk_inner_dim_bytes);
                    chunk_row_address[0] += 1;
                    target_element_index = get_index(chunk_row_address,array_shape);
                    target_char_index = target_element_index * prototype()->width();
                    source_element_index += chunk_shape[1];
                    source_char_index = source_element_index * prototype()->width();
                }
            }
        }
        else {
            char * target_buffer = get_buf();

            vector<unsigned int> array_shape = get_shape(false);
            BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - array_shape: " << vec2str(array_shape) << endl);

            vector<unsigned int> constrained_array_shape = get_shape(true);
            BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - constrained_array_shape: " << vec2str(constrained_array_shape) << endl);

            vector<unsigned int> chunk_shape = get_chunk_dimension_sizes();
            BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - chunk_shape: " << vec2str(chunk_shape) << endl);

            // There is a projection constraint.
            // Recover constraint information for the first/outer dim.
            Dim_iter outer_dim_itr = dim_begin();
            Dim_iter inner_dim_itr = dim_begin();
            inner_dim_itr++;
            unsigned int outer_dim = 0;
            unsigned int inner_dim = 1;
            unsigned int outer_start = dimension_start(outer_dim_itr,true);
            unsigned int outer_stride = dimension_stride(outer_dim_itr,true);
            unsigned int outer_stop = dimension_stop(outer_dim_itr,true);
            BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - outer_start:  " << outer_start << endl);
            BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - outer_stride: " << outer_stride << endl);
            BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - outer_stop:   " << outer_stop << endl);

            // There is a projection constraint.
            // Recover constraint information for the last/inner dim.
            unsigned int inner_start = dimension_start(inner_dim_itr,true);
            unsigned int inner_stride = dimension_stride(inner_dim_itr,true);
            unsigned int inner_stop = dimension_stop(inner_dim_itr,true);
            BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - inner_start:  " << inner_start << endl);
            BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - inner_stride: " << inner_stride << endl);
            BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - inner_stop:   " << inner_stop << endl);

            for(unsigned long i=0; i<chunk_refs->size(); i++) {
                BESDEBUG("dmrpp", "- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - " << endl);
                BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - Processing chunk[" << i << "]: " << endl);
                BESDEBUG("dmrpp", "DmrppArray::"<< __func__ << (*chunk_refs)[i].to_string() << endl);
                Chunk h4bs = (*chunk_refs)[i];
                vector<unsigned int> chunk_origin = h4bs.get_position_in_array();
                BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - chunk_origin:   " << vec2str(chunk_origin) << endl);

                // What's the first row/element that we are going to access for the outer dimension of the chunk?
                int outer_first_element_offset = 0;
                if(outer_start < chunk_origin[outer_dim]) {
                    BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - outer_start: " << outer_start << endl);
                    if(outer_stride!=1) {
                        outer_first_element_offset = (chunk_origin[outer_dim] - outer_start) % outer_stride;
                        if(outer_first_element_offset!=0)
                            outer_first_element_offset = outer_stride - outer_first_element_offset;
                    }
                    BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - outer_first_element_offset: " << outer_first_element_offset << endl);
                }
                else {
                    outer_first_element_offset = outer_start - chunk_origin[outer_dim];
                    BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - outer_start is in this chunk. outer_first_element_offset: " << outer_first_element_offset << endl);
                }
                unsigned long long outer_start_element = chunk_origin[outer_dim] + outer_first_element_offset;
                BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - outer_start_element: " << outer_start_element << endl);

                // Now we figure out the correct last element, based on the subset expression
                unsigned long long outer_end_element = chunk_origin[outer_dim] + chunk_shape[outer_dim] - 1;
                if(outer_stop<outer_end_element) {
                    outer_end_element = outer_stop;
                    BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - outer_stop is in this chunk. " << endl);
                }
                BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - outer_end_element: " << outer_end_element << endl);

                // What's the first row/element that we are going to access for the inner dimension of the chunk?
                int inner_first_element_offset = 0;
                if(inner_start < chunk_origin[inner_dim]) {
                    BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - inner_start: " << inner_start << endl);
                    if(inner_stride!=1) {
                        inner_first_element_offset = (chunk_origin[inner_dim] - inner_start) % inner_stride;
                        if(inner_first_element_offset!=0)
                            inner_first_element_offset = inner_stride - inner_first_element_offset;
                    }
                    BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - inner_first_element_offset: " << inner_first_element_offset << endl);
                }
                else {
                    inner_first_element_offset = inner_start - chunk_origin[inner_dim];
                    BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - inner_start is in this chunk. inner_first_element_offset: " << inner_first_element_offset << endl);
                }
                unsigned long long inner_start_element = chunk_origin[inner_dim] + inner_first_element_offset;
                BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - inner_start_element: " << inner_start_element << endl);

                // Now we figure out the correct last element, based on the subset expression
                unsigned long long inner_end_element = chunk_origin[inner_dim] + chunk_shape[inner_dim] - 1;
                if(inner_stop<inner_end_element) {
                    inner_end_element = inner_stop;
                    BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - inner_stop is in this chunk. " << endl);
                }
                BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - inner_end_element: " << inner_end_element << endl);

                // Do we even want this chunk?
                if( outer_start > (chunk_origin[outer_dim]+chunk_shape[outer_dim]) || outer_stop < chunk_origin[outer_dim]) {
                    // No. No, we do not. Skip this.
                    BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - Chunk not accessed by CE (outer_dim). SKIPPING." << endl);
                }
                else if( inner_start > (chunk_origin[inner_dim]+chunk_shape[inner_dim])
                    || inner_stop < chunk_origin[inner_dim]) {
                    // No. No, we do not. Skip this.
                    BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - Chunk not accessed by CE (inner_dim). SKIPPING." << endl);
                }
                else {
                    // Read and Process chunk

                    // Now. Now we are going to read this thing.
                    h4bs.read();
                    char * source_buffer = h4bs.get_rbuf();

                    vector<unsigned int> chunk_row_address = chunk_origin;
                    unsigned long long outer_chunk_end = outer_end_element - chunk_origin[outer_dim];
                    unsigned long long outer_chunk_start = outer_start_element - chunk_origin[outer_dim];
                    // unsigned int outer_result_position = 0;
                    // unsigned int inner_result_position = 0;
                    vector<unsigned int> target_address;
                    target_address.push_back(0);
                    target_address.push_back(0);
                    // unsigned long long chunk_inner_dim_bytes =  constrained_array_shape[inner_dim] * prototype()->width();
                    for(unsigned int odim_index=outer_chunk_start; odim_index<=outer_chunk_end;odim_index+=outer_stride) {
                        BESDEBUG("dmrpp", "DmrppArray::" << __func__ << "() ----------------------------------" << endl);
                        BESDEBUG("dmrpp", "DmrppArray::" << __func__ << "() --- "
                            "odim_index: " << odim_index << endl);
                        chunk_row_address[outer_dim] = chunk_origin[outer_dim] + odim_index;
                        BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - chunk_row_address: " << vec2str(chunk_row_address) << endl);

                        target_address[outer_dim] = (chunk_row_address[outer_dim] - outer_start)/outer_stride;

                        if(inner_stride==1) {
                            //#############################################################################
                            // 2D - inner_stride == 1

                            BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - The InnerMostStride is 1." << endl);

                            // Compute how much we are going to copy
                            unsigned long long chunk_constrained_inner_dim_elements = inner_end_element - inner_start_element + 1;
                            BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - chunk_constrained_inner_dim_elements: " << chunk_constrained_inner_dim_elements << endl);

                            unsigned long long chunk_constrained_inner_dim_bytes = chunk_constrained_inner_dim_elements * prototype()->width();
                            BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - chunk_constrained_inner_dim_bytes: " << chunk_constrained_inner_dim_bytes << endl);

                            // Compute where we need to put it.
                            target_address[inner_dim] = (inner_start_element - inner_start ) / inner_stride;
                            BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - target_address: " << vec2str(target_address) << endl);

                            unsigned int target_start_element_index = get_index(target_address,constrained_array_shape);
                            BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - target_start_element_index: " << target_start_element_index << endl);

                            unsigned int target_char_start_index = target_start_element_index* prototype()->width();
                            BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - target_char_start_index: " << target_char_start_index << endl);

                            // Compute where we are going to read it from
                            vector<unsigned int> chunk_source_address;
                            chunk_source_address.push_back(odim_index);
                            chunk_source_address.push_back(inner_first_element_offset);
                            BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - chunk_source_address: " << vec2str(chunk_source_address) << endl);

                            unsigned int chunk_start_element_index = get_index(chunk_source_address,chunk_shape);
                            BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - chunk_start_element_index: " << chunk_start_element_index << endl);

                            unsigned int chunk_char_start_index = chunk_start_element_index * prototype()->width();
                            BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - chunk_char_start_index: " << chunk_char_start_index << endl);

                            // Copy the bytes
                            BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - Using memcpy to transfer " << chunk_constrained_inner_dim_bytes << " bytes." << endl);
                            memcpy(target_buffer+target_char_start_index, source_buffer+chunk_char_start_index, chunk_constrained_inner_dim_bytes);
                        }
                        else {
                            //#############################################################################
                            // 2D -  inner_stride != 1
                            unsigned long long vals_in_chunk = 1 + (inner_end_element-inner_start_element)/inner_stride;
                            BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - InnerMostStride is equal to " << inner_stride
                                << ". Copying " << vals_in_chunk << " individual values." << endl);

                            unsigned long long inner_chunk_start = inner_start_element - chunk_origin[inner_dim];
                            BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - inner_chunk_start: " << inner_chunk_start << endl);

                            unsigned long long inner_chunk_end = inner_end_element - chunk_origin[inner_dim];
                            BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - inner_chunk_end: " << inner_chunk_end << endl);

                            vector<unsigned int> chunk_source_address = chunk_origin;
                            chunk_source_address[outer_dim] = odim_index;

                            for(unsigned int idim_index=inner_chunk_start; idim_index<=inner_chunk_end; idim_index+=inner_stride) {
                                BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() --------- idim_index: " << idim_index << endl);

                                // Compute where we need to put it.
                                target_address[inner_dim] = ( idim_index + chunk_origin[inner_dim] - inner_start ) / inner_stride;
                                BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - target_address: " << vec2str(target_address) << endl);

                                unsigned int target_start_element_index = get_index(target_address,constrained_array_shape);
                                BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - target_start_element_index: " << target_start_element_index << endl);

                                unsigned int target_char_start_index = target_start_element_index* prototype()->width();
                                BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - target_char_start_index: " << target_char_start_index << endl);

                                // Compute where we are going to read it from

                                chunk_row_address[inner_dim] = chunk_origin[inner_dim] + idim_index;
                                BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - chunk_row_address: " << vec2str(chunk_row_address) << endl);

                                chunk_source_address[inner_dim] = idim_index;
                                BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - chunk_source_address: " << vec2str(chunk_source_address) << endl);

                                unsigned int chunk_start_element_index = get_index(chunk_source_address,chunk_shape);
                                BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - chunk_start_element_index: " << chunk_start_element_index << endl);

                                unsigned int chunk_char_start_index = chunk_start_element_index * prototype()->width();
                                BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - chunk_char_start_index: " << chunk_char_start_index << endl);

                                // Copy the bytes
                                BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - Using memcpy to transfer " << prototype()->width() << " bytes." << endl);
                                memcpy(target_buffer+target_char_start_index, source_buffer+chunk_char_start_index, prototype()->width());
                            }
                        }
                    }
                }
            }
        }
    }break;
#endif
#if 0
    //########################### ThreeD Arrays ###############################
    case 3: {
        BESDEBUG("dmrpp", "DmrppArray::"<< __func__ << "() - 3D Array. Reading " << chunk_refs->size() << " chunks" << endl);

        char * target_buffer = get_buf();
        vector<unsigned int> chunk_shape = get_chunk_dimension_sizes();
        unsigned long long chunk_inner_dim_bytes = chunk_shape[2] * prototype()->width();

        for(unsigned long i=0; i<chunk_refs->size(); i++) {
            BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - READING chunk[" << i << "]: " << (*chunk_refs)[i].to_string() << endl);
            Chunk h4bs = (*chunk_refs)[i];
            h4bs.read();

            vector<unsigned int> chunk_origin = h4bs.get_position_in_array();

            char * source_buffer = h4bs.get_rbuf();
            unsigned long long source_element_index = 0;
            unsigned long long source_char_index = 0;

            unsigned long long target_element_index = get_index(chunk_origin,array_shape);
            unsigned long long target_char_index = target_element_index * prototype()->width();

            BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - Packing Array From Chunks: "
                << " chunk_inner_dim_bytes: " << chunk_inner_dim_bytes << endl);

            unsigned int K_DIMENSION = 0; // Outermost dim
            unsigned int J_DIMENSION = 1;
            unsigned int I_DIMENSION = 2;// inner most dim (fastest varying)

            vector<unsigned int> chunk_row_insertion_point_address = chunk_origin;
            for(unsigned int k=0; k<chunk_shape[K_DIMENSION]; k++) {
                chunk_row_insertion_point_address[K_DIMENSION] = chunk_origin[K_DIMENSION] + k;
                BESDEBUG("dmrpp", "DmrppArray::" << __func__ << "() - "
                    << "k: " << k << "  chunk_row_insertion_point_address: "
                    << vec2str(chunk_row_insertion_point_address) << endl);
                for(unsigned int j=0; j<chunk_shape[J_DIMENSION]; j++) {
                    chunk_row_insertion_point_address[J_DIMENSION] = chunk_origin[J_DIMENSION] + j;
                    target_element_index = get_index(chunk_row_insertion_point_address,array_shape);
                    target_char_index = target_element_index * prototype()->width();

                    BESDEBUG("dmrpp", "DmrppArray::" << __func__ << "() - "
                        "k: " << k << " j: " << j <<
                        " target_char_index: " << target_char_index <<
                        " source_char_index: " << source_char_index <<
                        " chunk_row_insertion_point_address: " << vec2str(chunk_row_insertion_point_address) << endl);

                    memcpy(target_buffer+target_char_index, source_buffer+source_char_index, chunk_inner_dim_bytes);
                    source_element_index += chunk_shape[I_DIMENSION];
                    source_char_index = source_element_index * prototype()->width();
                }
            }
        }

    }break;
    //########################### FourD Arrays ###############################
    case 4: {
        char * target_buffer = get_buf();
        vector<unsigned int> chunk_shape = get_chunk_dimension_sizes();
        unsigned long long chunk_inner_dim_bytes = chunk_shape[2] * prototype()->width();

        for(unsigned long i=0; i<chunk_refs->size(); i++) {
            BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - READING chunk[" << i << "]: " << (*chunk_refs)[i].to_string() << endl);
            Chunk h4bs = (*chunk_refs)[i];
            h4bs.read();

            vector<unsigned int> chunk_origin = h4bs.get_position_in_array();

            char * source_buffer = h4bs.get_rbuf();
            unsigned long long source_element_index = 0;
            unsigned long long source_char_index = 0;

            unsigned long long target_element_index = get_index(chunk_origin,array_shape);
            unsigned long long target_char_index = target_element_index * prototype()->width();

            BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - Packing Array From Chunk[" << i << "]"
                << " chunk_origin: " << vec2str(chunk_origin) << endl);

            unsigned int L_DIMENSION = 0; // Outermost dim
            unsigned int K_DIMENSION = 1;
            unsigned int J_DIMENSION = 2;
            unsigned int I_DIMENSION = 3;// inner most dim (fastest varying)

            vector<unsigned int> chunk_row_insertion_point_address = chunk_origin;
            for(unsigned int l=0; l<chunk_shape[L_DIMENSION]; l++) {
                chunk_row_insertion_point_address[L_DIMENSION] = chunk_origin[L_DIMENSION] + l;
                BESDEBUG("dmrpp", "DmrppArray::" << __func__ << "() - "
                    << "l: " << l << "  chunk_row_insertion_point_address: "
                    << vec2str(chunk_row_insertion_point_address) << endl);
                for(unsigned int k=0; k<chunk_shape[K_DIMENSION]; k++) {
                    chunk_row_insertion_point_address[K_DIMENSION] = chunk_origin[K_DIMENSION] + k;
                    BESDEBUG("dmrpp", "DmrppArray::" << __func__ << "() - "
                        << "l: " << l
                        << " k: " << k
                        << " chunk_row_insertion_point_address: "
                        << vec2str(chunk_row_insertion_point_address) << endl);
                    for(unsigned int j=0; j<chunk_shape[J_DIMENSION]; j++) {
                        chunk_row_insertion_point_address[J_DIMENSION] = chunk_origin[J_DIMENSION] + j;
                        target_element_index = get_index(chunk_row_insertion_point_address,array_shape);
                        target_char_index = target_element_index * prototype()->width();

                        BESDEBUG("dmrpp", "DmrppArray::" << __func__ << "() - "
                            << "l: " << l << " k: " << k << " j: " << j <<
                            " target_char_index: " << target_char_index <<
                            " source_char_index: " << source_char_index <<
                            " chunk_row_insertion_point_address: " << vec2str(chunk_row_insertion_point_address) << endl);

                        memcpy(target_buffer+target_char_index, source_buffer+source_char_index, chunk_inner_dim_bytes);
                        source_element_index += chunk_shape[I_DIMENSION];
                        source_char_index = source_element_index * prototype()->width();
                    }
                }
            }
        }
    }break;
    //########################### N-Dimensional Arrays ###############################
    default:

        break;
    }
#endif
