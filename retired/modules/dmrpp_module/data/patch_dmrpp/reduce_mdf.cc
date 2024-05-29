#include<iostream>
#include<sstream>
#include<fstream>
#include<string.h>
#include<stdio.h>
#include<vector>
#include <openssl/sha.h>
#include <unistd.h>

using  namespace std;

bool obtain_offset_nbytes(const vector<string>& str_vec, vector<size_t>& offsets, vector<size_t>& nbytes);

void string_tokenize(const string &in_str,const char delim,vector<string>&out_vec);

size_t string_to_size_t(const string& str);

bool retrieve_chunk_info(FILE*,vector<size_t> &offsets,vector<size_t> &nbytes);

string retrieve_data_sha256(FILE*,const vector<size_t> &offsets,const vector<size_t> &nbytes);

short write_sha256_file(char* m_dmrpp_fname,char* m_h5_fname,char* m_sha256_fname,const string & sha256_buf);

short update_sha256_file(char* m_dmrpp_fname,char* m_h5_fname,char* m_sha256_fname,char*stored_fname,const string & sha256_buf);

string to_hex(unsigned char s) {
    stringstream ss;
    ss << hex << (int) s;
    return ss.str();
}   

// If the return value is 0, the sha256 exists, no need to use the generated HDF5 file.
// If the return value is 1, the sha256 doesn't exist, need to use the generated HDF5 file.
int main(int argc,char **argv ) {

    if(argc !=5) {
        cout<<"Please provide four arguments: "<< endl;
        cout<<"  The first is the dmrpp file that contains the missing variable value information. "<<endl;
        cout<<"  The second is the hdf5 file path that stores the missing variable values. "<<endl;
        cout<<"  The third is the text file that stores the file path and the sha256 value." <<endl;
        cout<<"  The fourth is the text file that stores the final HDF5 file path for this dmrpp file. "<<endl;
    }

    // Retrieve the chunk info from the dmrpp file.
    FILE* fp_dmrpp = fopen(argv[1],"r");
    if(fp_dmrpp == NULL) {
        cout<<"The dmrpp file doesn't exist"<<endl;
        return -1;
    }

    vector<size_t>offsets;
    vector<size_t>nbytes;
    bool ret_chunk = retrieve_chunk_info(fp_dmrpp,offsets,nbytes);
    if(false == ret_chunk) {
        cout<<"Cannot retrieve the chunk info from the dmrpp file successfully. "<<endl;
        return -1;
    }
    fclose(fp_dmrpp);

    // Obtain the sha256.
    FILE* fp_h5 = fopen(argv[2],"r");
    if(fp_h5 == NULL) {
        cout<<"The HDF5 file doesn't exist"<<endl;
        return -1;
    }

    string sha256_buf = retrieve_data_sha256(fp_h5,offsets,nbytes);
    if(sha256_buf=="") {
        cout<<"The sha256 of this file doesn't exist"<<endl;
        return -1;
    }
    fclose(fp_h5);

    // Store the sha256 if necessary to a file.
    short ret_value = update_sha256_file(argv[1],argv[2],argv[3],argv[4],sha256_buf);
    return ret_value;

}

// Append the sha256 to a file.
short write_sha256_file(char* m_dmrpp_fname,char* m_h5_fname,char* m_sha256_fname,const string & sha256_buf) {

    short sha_fname_ret = 1;
    FILE*fp = fopen(m_sha256_fname,"a");
    string fname_str(m_h5_fname);
    string dname_str(m_dmrpp_fname);
    string file_content = fname_str +' '+dname_str+' '+sha256_buf+'\n';
    vector<char>buf(file_content.begin(),file_content.end());
    size_t fsize = fwrite(buf.data(),1,file_content.size(),fp);
    if(fsize != file_content.size())
        sha_fname_ret = -1;
    fclose(fp);

    return sha_fname_ret;

}

// Update the sha256 in the recording file if necessary.
short update_sha256_file(char* m_dmrpp_fname,char* m_h5_fname,char* m_sha256_fname,char* store_h5_fname,const string & sha256_buf) {

    // If the recording file that stores thesha256 doesn't exist, 
    // just create this file and write the sha256 etc information to the file.
    if(access(m_sha256_fname,F_OK)==-1) 
        return write_sha256_file(m_dmrpp_fname,m_h5_fname,m_sha256_fname,sha256_buf);

    // 
    // If the recording file exists, open this file and see if the sha256 of 
    // this missing data can be found from the recording file.
    // If the sha256 can be found,then the missing data file exists, we don't
    // need to create a new one, otherwise, a new one needs to be created.
    // If the sha256 can be found, we need to create a temp. text file to store
    // the missing data file name so that this information can be passed to
    // the patched dmrpp program afterwards.
    short ret_value = 1;
    ifstream sha_fstream;
    sha_fstream.open(m_sha256_fname,ifstream::in);
    string sha_line;
    char space_char=' ';
    char end_line='\n';
    bool space_fname_ret = true;
    bool need_add_sha256 = true;

    while(getline(sha_fstream,sha_line)) {

        size_t fname_epos = sha_line.find(space_char);
        if(fname_epos==string::npos) {
            space_fname_ret = false;
            break;
        }

        size_t dname_epos = sha_line.find(space_char,fname_epos+1);
        if(dname_epos==string::npos) {
            space_fname_ret = false;
            break;
        }

        string f_sha256_buf = sha_line.substr(dname_epos+1);
        if(f_sha256_buf == sha256_buf) {

            need_add_sha256 = false;

            string exist_m_h5_name = sha_line.substr(0,fname_epos);
            string exist_m_dmrpp_name = sha_line.substr(fname_epos+1,dname_epos-fname_epos-1);

            // Open the file to store the HDF5 and dmrpp file
            FILE*fp = fopen(store_h5_fname,"a");
            string file_content = exist_m_h5_name +' '+exist_m_dmrpp_name;
            vector<char>buf(file_content.begin(),file_content.end());
            size_t fsize = fwrite(buf.data(),1,file_content.size(),fp);
            if(fsize != file_content.size())
                ret_value = -1;
            fclose(fp);
            break;
        }
    }
    sha_fstream.close();


    if(false == space_fname_ret)
        ret_value = -1;
    if(false == need_add_sha256)
        ret_value = 0;

    // sha256 is not found, append this sha256 and the missing data file name to the recording file.
    if(true == space_fname_ret) {
        if(true == need_add_sha256) {
            ret_value = write_sha256_file(m_dmrpp_fname,m_h5_fname,m_sha256_fname,sha256_buf);
        }
    }

    return ret_value;
}

// Obtain the sha256 from the data values.
string retrieve_data_sha256(FILE*fp,const vector<size_t> &offsets,const vector<size_t> &nbytes){

    string ret_str;
    size_t fSize = 0;
    unsigned char hash[SHA256_DIGEST_LENGTH];

    // This is the buffer size
    for(int i = 0; i <nbytes.size();i++) 
        fSize+=nbytes[i];

    // Read in the offset and byte information.
    vector<char>buf;
    buf.resize(fSize);
    
    size_t cur_size = 0;
    for(int i = 0; i<offsets.size();i++) {
        // Seek according to offset
        if(fseek(fp,offsets[i],SEEK_SET)!=0)
            return ret_str;
        size_t result = fread(&buf[cur_size],1,nbytes[i],fp);
        cur_size +=nbytes[i];
    }

    // Calculate the hash
    SHA256((const unsigned char*)buf.data(),fSize,hash);

    string output="";

    // Change 256 to hex and to a string
    for(int i =0; i<SHA256_DIGEST_LENGTH;i++) 
        output+=to_hex(hash[i]);

    return output;
}

// Retrieve the offsets and number of bytes of variable values.
bool retrieve_chunk_info(FILE*fp,vector<size_t> &offsets,vector<size_t> &nbytes) {

    long fSize = 0;

    // Read in the offset and byte information.
    if(fseek(fp,0,SEEK_END)!=0) 
        return false;
    fSize = ftell(fp);
    if(fSize <0) 
        return false;
    
    if(fseek(fp,0,SEEK_SET)!=0)
        return false;

    vector<char>buf;
    buf.resize((size_t)fSize);
    size_t result = fread(buf.data(),1,fSize,fp);
    if(result != fSize) 
        return false;

    string str(buf.begin(),buf.end());
    char delim='\n';
    vector<string> str_vec;
    string_tokenize(str,delim,str_vec);

    bool get_offset_nbytes = obtain_offset_nbytes(str_vec,offsets,nbytes);
    if(false == get_offset_nbytes) {
        cout<<"cannot successfully obtain the offset and nbytes. \n";
        return false;
    }   

#if 0
    for (int i = 0; i <offsets.size();i++) {
        cout<<"offset["<<i<<"]= " <<offsets[i] <<endl;
        cout<<"nbyte["<<i<<"]= " <<nbytes[i] <<endl;
    }
#endif

    return get_offset_nbytes;

}

// Obtain the offset and number of bytes from the dmrpp file.
// Here we don't need to worry about the filters. We just want to
// make sure the data values(either in compressed form or uncompressed form)
// can be retrieved.
bool obtain_offset_nbytes(const vector<string>& str_vec, vector<size_t>& offsets, vector<size_t>& nbytes){

    bool ret=true;
    vector<string>chunk_info_str;
    string delim1 ="chunk offset=\"";
    string delim2 ="nBytes=\"";
    string delim3="\"";

    vector<size_t> unfiltered_offsets;
    vector<size_t> unfiltered_nbytes;

    // Pick up the line that includes chunk offset and save them to a vector.
    for(int i = 0; i <str_vec.size(); i++)
        if(str_vec[i].find(delim1)!=string::npos)
            chunk_info_str.push_back(str_vec[i]);

    // Obtain the offsets and number of bytes and save them to vectors.
    for(int i = 0; i<chunk_info_str.size();i++) {
        size_t co_spos = chunk_info_str[i].find(delim1);
        size_t co_epos = chunk_info_str[i].find(delim3,co_spos+delim1.size());
        if(co_epos==string::npos) {
            ret = false;
            break;       
        }
        string temp_offset=chunk_info_str[i].substr(co_spos+delim1.size(),co_epos-co_spos-delim1.size());
        unfiltered_offsets.push_back(string_to_size_t(temp_offset));

        size_t nb_spos = chunk_info_str[i].find(delim2,co_epos);
        size_t nb_epos = chunk_info_str[i].find(delim3,nb_spos+delim2.size());
        if(nb_epos==string::npos) {
            ret = false;
            break;       
        }
        string temp_nbyte=chunk_info_str[i].substr(nb_spos+delim2.size(),nb_epos-nb_spos-delim2.size());
        unfiltered_nbytes.push_back(string_to_size_t(temp_nbyte));

    }

    // Remove nbyte = 0 case. This is a bug caused by build_dmrpp. Before that is fixed, we
    // remove this case since this fortuately doesn't affect our purpose and the patch_dmrpp program.
    if(true == ret) {
        for(int i = 0; i<unfiltered_nbytes.size();i++) {
            if(unfiltered_nbytes[i] != 0) {
                offsets.push_back(unfiltered_offsets[i]);
                nbytes.push_back(unfiltered_nbytes[i]);
            }
        }
    }

    return ret;
}

// Tokenize the string to a vector of string according to the delimiter
void string_tokenize(const string &in_str,const char delim,vector<string>&out_vec) {
    stringstream ss_str(in_str);
    string temp_str;
    while (getline(ss_str,temp_str,delim)) {
        out_vec.push_back(temp_str);
    }
}

// Convert string to size_t.
size_t string_to_size_t(const string& str) {
    stringstream sstream(str);
    size_t str_num;
    sstream >>str_num;
    return str_num;
}


