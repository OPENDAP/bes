/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is considered to be an HDF5 demo utility. The full HDF5         *
 * copyright notice, including                                               *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found                                      *
 * in https://support.hdfgroup.org/ftp/HDF5/releases.                        *
 * If you do not have access to the file, you may request a copy from        *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
 *  This code shows how to retrieve the variable length string data offset and length
 *  Assumption: contiguous storage, the file name, dataset offset and length should be provided.
 *  This can be extended to include chunking storage and other variable length data.
 *  The code is unpolished and error check and handling are limited.
 *  Some printf and cout are left deliberately for debugging.
 */

/* Author: Kent Yang <myang6@hdfgroup.org> */ 

#include <cstdio>
#include <cstdlib>
#include <stdint.h>
#include <iostream>
#include <vector>
#include <string>

using namespace std;


// The following macros are adapted from the HDF5 library
#  define UINT32DECODE(p, i) {                              \
   (i)    =  (uint32_t)(*(p) & 0xff);       (p)++;                  \
   (i) |= ((uint32_t)(*(p) & 0xff) <<  8); (p)++;                  \
   (i) |= ((uint32_t)(*(p) & 0xff) << 16); (p)++;                  \
   (i) |= ((uint32_t)(*(p) & 0xff) << 24); (p)++;                  \
}

#  define UINT64DECODE(p, n) {                              \
   /* WE DON'T CHECK FOR OVERFLOW! */                         \
   size_t _i;                                      \
                                        \
   n = 0;                                      \
   (p) += 8;                                      \
   for (_i = 0; _i < sizeof(uint64_t); _i++)                      \
      n = (n << 8) | *(--p);                              \
   (p) += 8;                                      \
}

#  define UINT16DECODE(p, i) {                              \
   (i)    = (uint16_t) (*(p) & 0xff);      (p)++;                  \
   (i) |= (uint16_t)((*(p) & 0xff) << 8); (p)++;                  \
}

#define VLS_TSIZE 16

typedef struct {
    char buf[VLS_TSIZE];
} vlstr_t;

typedef struct {
    uint64_t heap_addr;
    vector<uint16_t> obj_index;
    vector<uint32_t> obj_length;
    vector<uint32_t> obj_pos;
} heap_obj_info_t;

typedef struct {
    uint64_t obj_addr;
    uint32_t obj_length;
} obj_info_t;


typedef struct {
    uint64_t heap_addr;
    vector<uint64_t> obj_addr;
} heap_obj_data_info_t;

bool obtain_heap_obj_data_info(const vector<heap_obj_info_t>&, FILE *fp, const vector<uint16_t>&, vector<heap_obj_data_info_t>&);
bool decode_heap_info(FILE*fp, const heap_obj_info_t hobj_info, uint16_t s_max_obj_index, vector<uint64_t>& s_obj_addr);

// Input should be the offset and the buf size(in bytes) from H5Dread.
int main(int argc, char *argv[]) {

    if (argc != 4) {
        cerr << "Please provide the HDF5 file name and the HDF5 dataset offset and size-in-bytes as the following:" << endl;
        cerr << argv[0] << "h5_file_name offset size" << endl;
        return 0;
    }

    // Not checking errors, the wrong offset and size, errors will occur afterwards.
    char *t_ptr;
    unsigned long long d_offset = strtoul(argv[2],&t_ptr,10);
    unsigned long long d_size = strtoul(argv[3],&t_ptr,10);
    
    FILE *fp;
    int elems_size = VLS_TSIZE;

    // VL string size is 16 bytes, obtain this from HDF5 APIs
    // Add somewhere: use HDF5 API to obtain vlstr type size 
    // and check if the type size is 16.
    int t_nelems = d_size/elems_size;
    int numofread = 0;
    
    vector<char> vls_dinfo_buf;
    vls_dinfo_buf.resize(t_nelems*elems_size);

    vector<vlstr_t> vls_dinfo;
    vls_dinfo.resize(t_nelems);

    // dataset offset comes from the HDF5 APIs.
    if((fp= fopen(argv[1],"rb"))==NULL) {
        printf("Error, Can't open file.");
        return -1;
    }

    if (fseek(fp,d_offset,SEEK_SET) !=0) {
       cout <<" fseek fails for the dataset offset "<<endl;
       fclose(fp);
       return -1;
    }

    numofread = fread(&vls_dinfo[0],elems_size,t_nelems,fp);
    printf("numofread %d\n",numofread);

    if(numofread != t_nelems) {
       cout <<" fread fails to read the data  "<<endl;
       fclose(fp);
       return -1;
    }

    // This may not be necessary. 
    vector<char>vls_sdinfo_buf(t_nelems*elems_size);
    
    memcpy((void*)&vls_sdinfo_buf[0],(void*)&vls_dinfo[0],t_nelems*elems_size);

cout<<"vls_sdinfo_buf size is "<<vls_sdinfo_buf.size()<<endl;

    uint64_t seq_addr = 0;
    uint32_t seq_len = 0,seq_index = 0;
    vector<heap_obj_info_t> obj_info;
    uint8_t* tp = (uint8_t *)&vls_sdinfo_buf[0];
    uint64_t tmp = 0;

    // Loop through all elements and decode 
    for(unsigned int idx =0;idx < t_nelems; idx++) {

        // Decode sequence length
        UINT32DECODE(tp,seq_len);
        printf("seq_len is %u\n",seq_len);

        // Must set seq_addr to 0 first before retrieving it. 
        seq_addr = 0;
    
        // Decode sequence collection heap address
        for (unsigned u = 0; u < 8;u++) {
            uint8_t c = *tp;
            tmp  = c;
            tmp <<= (u*8);
            seq_addr |= tmp;
            tp++;
        }
        printf("seq_heap_addr is %llu\n",seq_addr);

        // Decode sequence object index
        UINT32DECODE(tp,seq_index)
        printf("seq_index is %u\n",seq_index);

        heap_obj_info_t tmp_obj_info;

        bool find_heap_addr = false;

        // If heap address is found already, just insert the index and length.
        for (unsigned i =0; i<obj_info.size();i++) {
            if (obj_info[i].heap_addr == seq_addr) {
                find_heap_addr = true;
                obj_info[i].obj_index.push_back((uint16_t)seq_index);
                obj_info[i].obj_length.push_back(seq_len);
                obj_info[i].obj_pos.push_back(idx);
                break;
            }
        }

        // Insert heap address,index,length and pos.
        // The pos is needed because the index order is not guaranteed.
        if (find_heap_addr == false) {
            heap_obj_info_t temp_obj_info;
            temp_obj_info.heap_addr = seq_addr;
            temp_obj_info.obj_index.push_back(seq_index);
            temp_obj_info.obj_length.push_back(seq_len);
            temp_obj_info.obj_pos.push_back(idx);
            obj_info.push_back(temp_obj_info);
        }
    }

    // Before diving into the file again, we need to know the maximum object index for each heap address,
    // The object index stored in the obj_info may be the subset of the objects up to the maximum object index.
    // We need to decode and find each object length even if this object is not in our selection.  
    // Why needing the maximum object index? Because we don't need
    // to obtain any object of which the index is greater than the maximum object index. 
    // Remember the heap not only stores the variable length data, it also stores other objects. We only need to search what we want.
   
    vector<uint16_t> max_obj_index(obj_info.size(),0);

    for(unsigned i = 0; i < obj_info.size(); i++) {
        for (unsigned j = 0; j < obj_info[i].obj_index.size(); j++) {
            if (max_obj_index[i] < obj_info[i].obj_index[j])
                max_obj_index[i] = obj_info[i].obj_index[j];
        }
    }

    // Now we need to go to every heap, obtain the heap size, then obtain each object address and length.
    vector<heap_obj_data_info_t> heap_obj_data_info;
    //heap_obj_info.resize(obj_info.size());
    //heap_obj_data_info.resize(obj_info.size());
    bool ret_value = obtain_heap_obj_data_info(obj_info,fp,max_obj_index,heap_obj_data_info);

    vector<obj_info_t> final_obj_info(t_nelems);
    if(ret_value == false) { 
        cout << "Cannot obtain object offset and length \n" <<endl;
        fclose(fp);
        return -1;
    }
    else  {
        // Later: We can also have a sanity check to ensure the same vector size of object length,offset, pos.
        for (unsigned idx = 0; idx <t_nelems; idx++) {
            for (unsigned i = 0; i < obj_info.size(); i++) {
                //cout << "heap address = "<< obj_info[i].heap_addr << " :" <<endl;
                // Make sure the final object offset and length are according to the element order.
                for (unsigned j = 0; j < obj_info[i].obj_length.size(); j++) {
                    if (idx == obj_info[i].obj_pos[j]) {
                        final_obj_info[idx].obj_addr = heap_obj_data_info[i].obj_addr[j];
                        final_obj_info[idx].obj_length = obj_info[i].obj_length[j];

                        //cout <<"obj_index["<<j<<"]= "<<obj_info[i].obj_index[j] << endl;
                        cout << "element index = "<<idx <<endl;
                        cout <<"obj_addr["<<j<<"]= "<<heap_obj_data_info[i].obj_addr[j] << endl;
                        cout <<"obj_length["<<j<<"]= "<<obj_info[i].obj_length[j] << endl;
                    }
                }
            }
        }
    }

    fclose(fp);
    return 0;
}



bool obtain_heap_obj_data_info(const vector<heap_obj_info_t>& heap_obj_info, 
                               FILE *fp, 
                               const vector<uint16_t>& max_obj_index, 
                               vector<heap_obj_data_info_t>& all_obj_data_info) {

    bool can_decode = true;

    for (unsigned i = 0; i < heap_obj_info.size(); i++) {

        vector<uint64_t> obj_addr;
        obj_addr.resize((heap_obj_info[i].obj_index).size());

        // Obtain the object address of each selected object in a heap.
        // Note: the object addresses are ensured to be consistent with the corresponding object index
        bool can_decode = decode_heap_info(fp, heap_obj_info[i],max_obj_index[i],obj_addr);

        // If something is wrong, just get out of the loop.
        if (false == can_decode) 
            break;

        // FIll in the object address info. for all_obj_data_info.
        // This is necessary since object index is only the key within a heap. The heap address
        // is also needed to ensure the retrieval of an object.
        // The order in the vector of object address is consistent with the corresponding object index
        heap_obj_data_info_t t_heap_obj_data_info;
        t_heap_obj_data_info.heap_addr = heap_obj_info[i].heap_addr;
        for (unsigned j = 0; j < obj_addr.size();j++)
            t_heap_obj_data_info.obj_addr.push_back(obj_addr[j]);
        all_obj_data_info.push_back(t_heap_obj_data_info);
    }

    return can_decode;
}

bool decode_heap_info(FILE*fp, const heap_obj_info_t hobj_info, uint16_t s_max_obj_idx, vector<uint64_t>& s_obj_addr) {
   
    bool can_decode_theap = true;

    // From HDF5 file specification
    // The following is according to HDF5 file specification
    // https://docs.hdfgroup.org/hdf5/develop/_f_m_t3.html section III E.
 
    // I. First check if this is a valid global heap and also obtain the global heap size
cout <<"heap address is " <<hobj_info.heap_addr <<endl;
    if (fseek(fp,hobj_info.heap_addr,SEEK_SET) != 0) {
        cout <<" fseek fails for the heap address "<< hobj_info.heap_addr << endl;
        return false;
    }
    vector<char> heap_hdr_buf;
    heap_hdr_buf.resize(16);
    if (fread(&heap_hdr_buf[0],16,1,fp) !=1) {
        cout <<" fread fails to read the heap header "<< hobj_info.heap_addr << endl;
        return false;
    }
    if (heap_hdr_buf[0]!='G' ||
        heap_hdr_buf[1]!='C' ||
        heap_hdr_buf[2]!='O' ||
        heap_hdr_buf[3]!='L') {
        cout<<"not valid global heap" <<endl;
        return false;
    }

    uint8_t *tp=(uint8_t*)&heap_hdr_buf[8];
    uint64_t coll_size;
    UINT64DECODE(tp,coll_size)

cout<<"collection size is "<<coll_size <<endl;
     
    // II. Now start from the beginning of the heap again
    //     Obtain the object index and real object length for each object until the max_obj_index
    size_t size_so_far = 0;
    uint64_t addr_so_far = hobj_info.heap_addr;

    //II.1 read the whole heap to a buffer
    if (fseek(fp,hobj_info.heap_addr,SEEK_SET) != 0) {
       cout <<" fseek fails to seek the heap at address "<< hobj_info.heap_addr << "." << endl;
       return false;
    }
    vector<char> heap_buf(coll_size);
    if (fread(&heap_buf[0],coll_size,1,fp) != 1) {
        cout <<" fread fails to read the heap at address  "<< hobj_info.heap_addr << "." << endl;
        return false;
    }

    // 2. skip the heap header
    size_so_far = 16;
    tp = (uint8_t *)&heap_buf[size_so_far];
    uint16_t curr_obj_idx = 0;
     
    //3. Scan the whole heap, basically it may contain multiple objects and each object contains the obj index, length and data.
    while ((size_so_far <coll_size) && (curr_obj_idx < s_max_obj_idx) && can_decode_theap) {
        UINT16DECODE(tp,curr_obj_idx)
cout<<"current object index is "<<curr_obj_idx <<endl;
        // object index 0 is for free space management at it is at the end of all the objects. For our purpose, we should never 
        // reach this object. 
        if (curr_obj_idx >s_max_obj_idx || curr_obj_idx == 0) {
            cout << " curr_obj_idx " << curr_obj_idx << " is beyond the range. "<<endl;
            can_decode_theap = false;
            break;
        }
        uint64_t curr_obj_size;
        // There are some bytes to store other irrelevant info, we need to skip them. 
        size_so_far = size_so_far+8;
        tp = (uint8_t *)&heap_buf[size_so_far];
        UINT64DECODE(tp,curr_obj_size)
cout<<"current object size is "<< curr_obj_size <<endl;
        size_so_far = size_so_far+8;
        addr_so_far = hobj_info.heap_addr + size_so_far;
cout<<"current object addr is "<< addr_so_far <<endl;

        // Here we want to check if this object is our selected object, if yes, stor the addr.
        // We have to loop this way since the object index stored in the heap is NOT sorted.
        for(unsigned i = 0; i < hobj_info.obj_index.size(); i++) {
            if (curr_obj_idx == hobj_info.obj_index[i]) {
                s_obj_addr[i] = addr_so_far;
                break;
            }
        }

        // We also need to obtain the occupied object size which is rounded up to a multiple of 8.
        size_t occupied_obj_size = (curr_obj_size%8==0)?curr_obj_size:(curr_obj_size/8*8+8);
cout<<"occupied_obj_size is "<<occupied_obj_size <<endl;
        size_so_far +=occupied_obj_size;
        // Update the buffer pointer
        tp = (uint8_t *)&heap_buf[size_so_far];
    }
   
    return can_decode_theap;
}
