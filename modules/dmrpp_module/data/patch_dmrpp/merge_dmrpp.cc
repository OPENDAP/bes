#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <algorithm>

using namespace std;

// The following block of functions retrieve the "missing" variable type, variable name and data value information.
bool obtain_var_info(const string & miss_dmrpp_info,vector<string> & var_types, vector<string>&var_names,vector<string>&chunk_info_list,bool & is_chunk_mark1); 
bool find_var_name(const string &str,size_t &str_pos,size_t &var_name_pos_start,size_t &var_name_pos_end);
bool find_end_var_block(const string&str, const string&var_type, const size_t &str_pos, size_t &var_end_pos);
bool find_chunk_info(const string &str,const size_t&str_pos,size_t &chunk_info_pos_start, size_t &chunk_info_pos_end,const size_t&var_end_pos,bool & is_mark1);

// The following block of functions add the file address(mostly the absolute path of the HDF5 file that stores the data value) to the chunk block.
bool add_faddr_chunk_info(const string& miss_dmrpp_info,vector<string>&chunk_info_list,bool is_dmrpp_mark1,const string faddr_source = "");
bool add_faddr_contig_line(string &chunk_info,const string &file_addr);
bool add_faddr_chunk_comp_lines(string & chunk_info,const string &file_addr);

// The following block of functions merge the "missing" variable data value information to the original dmrpp file.
bool add_missing_info_to_file(const string &fname2,const vector<string> &var_types,const vector<string> &var_names,const vector<string> &chunk_info_list);
void gen_block(const vector<string>&var_type_list,const vector<string>&var_name_list,vector<string>&block_begin,vector<string>&block_end);
bool check_overlap_intervals(const vector<size_t> &sort_block_pos, const vector<size_t> & block_pos_start);
void obtain_bindex_in_modified_string(const vector<size_t>& block_pos_start, vector<int>& block_index); 
bool split_string(const string & str, vector<string> &str_vec,const vector<string> &block_begin, const vector<string> &block_end,vector<int> &block_index);
bool convert_dmrppstr_to_vec(const string &dmrpp_str,vector<string> &dmrpp_str_vec,const vector<string> &var_types,const vector<string> &var_names,vector<int> & block_index);
void add_missing_info_to_vec(vector<string> &dmrpp_str_vec,const vector<string> &chunk_info_list,const vector<int>&block_index);
void write_vec_to_file(const string &fname,const vector<string> &dmrpp_str_vec);

// The following two functions are helper functions
void file_to_string(const string &filename, string & out);
void string_tokenize(const string &in_str,const char delim,vector<string>&out_vec);


int main (int argc,char**argv)
{
    string dmrpp_line;
    vector<string>var_types;
    vector<string>var_names;
    vector<string>chunk_info_list;
    
    bool add_dmrpp_info = false;
    bool is_chunk_mark1 = true;

    string missing_dmrpp_str;

    if(argc != 5) {
        cout<<"Please provide four arguments: "<< endl;
        cout<<"  The first is the dmrpp file that contains the missing variable value information. "<<endl;
        cout<<"  The second is the original dmrpp file. "<<endl;
        cout<<"  An third one is the href to the missing variables HDF5 file. "<<endl;
        cout<<"  The fourth one is the text file that includes the missing variable information. "<<endl;
        return 0;
    }

    // Obtain the dmrpp file name that contains the missing variable value.
    string fname(argv[1]);

    // Read the "missing dmrpp file" to a string
    file_to_string(fname,missing_dmrpp_str);

    // Obtain the missing chunk information from the dmrpp file.
    add_dmrpp_info = obtain_var_info(missing_dmrpp_str,var_types,var_names,chunk_info_list,is_chunk_mark1);

    // Just output a warning that there is no chunk info, in the supplemental dmrpp file.
    if(false == add_dmrpp_info) {
        cout<<"Cannot find corresponding chunk info. from the supplemental dmrpp file."<<endl;
        cout<<"You may need to check if there is any variable in the dmrpp file. "<<endl;
        cout<<"The dmrpp file is "<<fname <<endl;
    }

    if(var_types.size() !=var_names.size() || var_names.size() != chunk_info_list.size()) {
        cout <<"Var type, var name and chunk_info must have the same number of sizes. "<<endl;
        cout <<"The dmrpp file is "<<fname <<endl;
        return 0;
    }
       
#if 0

    for (int i =0; i<var_names.size();i++) {
//cout<<"var type["<<i<<"]"<< var_types[i]<<endl;
//cout<<"var name["<<i<<"]"<< var_names[i]<<endl;
//cout<<"chunk_info_list["<<i<<"]"<< endl;

    }
#endif

    // We need to erase those variables that are not really missing but are added by the generation program
    string mvar_fname(argv[4]);
    string missing_vname_str;

    // Read the missing variable names to a string and tokenize the string to a vector of string.
    file_to_string(mvar_fname,missing_vname_str);

    vector<string> missing_vname_list;
    char delim=',';
    string_tokenize(missing_vname_str,delim,missing_vname_list);

#if 0
    for(int i = 0;i<missing_vname_list.size();i++)
        cout <<"missing_vname_list["<<i<<"]"<<missing_vname_list[i]<<endl;
#endif

    // Remove the additional variables added by the filenetCDF-4 module.
    vector<string>new_var_types;
    vector<string>new_var_names;
    vector<string>new_chunk_info_list;

    for (int i =0; i<var_names.size();i++) {
        for(int j = 0; j<missing_vname_list.size();j++) {
            if(var_names[i] == missing_vname_list[j]) {
                new_var_names.push_back(var_names[i]);
                new_var_types.push_back(var_types[i]);
                new_chunk_info_list.push_back(chunk_info_list[i]);
                break;
            }
        }
    }

    // Add file address to each chunk. Mostly the file address is the absolute path of the HDF5 files.
    string fadd_source(argv[3]);
    add_faddr_chunk_info(missing_dmrpp_str,new_chunk_info_list,is_chunk_mark1,fadd_source);

#if 0
for (int i =0; i<new_var_types.size();i++)  {
cout<<"new chunk_info_list["<<i<<"]"<< endl;
cout<<new_chunk_info_list[i]<<endl;
}
#endif

    //string dmrpp_str;
    string fname2(argv[2]);

    // Add the missing chunk info to the original dmrpp file.
    bool well_formed = add_missing_info_to_file(fname2,new_var_types,new_var_names,new_chunk_info_list);    

    if(false == well_formed) {
        cout <<"The dmrpp file to be modified is either not well-formed or contains nested variable blocks  that cannot be supported by this routine" <<endl;
        cout <<"The dmrpp file is "<<fname2<<endl;

    }

    return 0;

}

// Obtain the var info from the supplemental(missing) dmrpp file. The variable types we checked are limited to DAP2 data types plus 64-bit integers.
bool obtain_var_info(const string & miss_dmrpp_info,vector<string> & var_types, vector<string>&var_names,vector<string>&chunk_info_list,bool & is_chunk_mark1) {

    bool ret = false;
    vector<string> var_type_list;
    var_type_list.push_back("Float32");
    var_type_list.push_back("Int32");
    var_type_list.push_back("Float64");
    var_type_list.push_back("Byte");
    var_type_list.push_back("Int16");
    var_type_list.push_back("UInt16");
    var_type_list.push_back("String");
    var_type_list.push_back("UInt32");
    var_type_list.push_back("Int8");
    var_type_list.push_back("Int64");
    var_type_list.push_back("UInt64");
    var_type_list.push_back("UInt8");
    var_type_list.push_back("Char");
    
    size_t var_type_pos_start =0;
    size_t var_name_pos_start = 0;
    size_t var_name_pos_end = 0;
    size_t chunk_pos_start = 0;
    size_t chunk_pos_end = 0;
    size_t var_end_pos= 0;
    size_t str_pos = 0;


    if(miss_dmrpp_info.empty()) 
        return ret;

    size_t str_last_char_pos = miss_dmrpp_info.size()-1;
    bool well_formed = true;
    
    // Go through the whole missing dmrpp string
    while (str_pos <=str_last_char_pos && well_formed) {

        int i = 0;
        string var_sign;
        string temp_var_sign;
        size_t temp_var_type_pos_start=string::npos;
        int var_type_index = -1;

        // Go through the var_type_list to obtain the var data type
        // We need to find the index in the var_type_list to
        // obtain the correct var datatype.
        while(i <var_type_list.size()) {
            var_sign = "<"+var_type_list[i]+" name=\"";
            var_type_pos_start  = miss_dmrpp_info.find(var_sign,str_pos);
            if(var_type_pos_start ==string::npos) {
                i++;
                continue;
            }
            else {
                // We want to make sure we don't skip any vars.
                if(temp_var_type_pos_start>var_type_pos_start){
                    temp_var_type_pos_start = var_type_pos_start;
                    var_type_index = i;
                    temp_var_sign = var_sign;
                }
                i++;
            }

        }

        // Ensure all variables are scanned.
        if(temp_var_type_pos_start !=string::npos) {
            var_type_pos_start = temp_var_type_pos_start;
            var_sign = temp_var_sign;

        }

        // This line will ignore datatypes that are not in the var_type_list
        if(var_type_pos_start == string::npos) {
            str_pos = string::npos;
            continue;
        }
        else 
            str_pos = var_type_pos_start+var_sign.size();
        
        // Now we can retrieve var name, var type and the corresponding chunk info
        // Sanity check is also applied.
        if(false == find_var_name(miss_dmrpp_info,str_pos,var_name_pos_start,var_name_pos_end)) 
            well_formed = false;
        else if(false == find_end_var_block(miss_dmrpp_info,var_type_list[var_type_index],str_pos,var_end_pos))
            well_formed = false;
        else if(false == find_chunk_info(miss_dmrpp_info,str_pos,chunk_pos_start,chunk_pos_end,var_end_pos,is_chunk_mark1))
            well_formed = false;
        else {
            // Move the string search pos to the next block
            str_pos = var_end_pos+1;
            // Obtain var type, var name and chunk info. and save them to vectors.
            var_types.push_back(var_type_list[var_type_index]);
            var_names.push_back(miss_dmrpp_info.substr(var_name_pos_start,var_name_pos_end-var_name_pos_start));
            string temp_chunk_info = miss_dmrpp_info.substr(chunk_pos_start,chunk_pos_end-chunk_pos_start);
            if(true == is_chunk_mark1) 
                temp_chunk_info +="</dmrpp:chunks>";
            else 
                temp_chunk_info +="/>";
            chunk_info_list.push_back(temp_chunk_info);
        }
        
    }
    return well_formed;

}

// Find var name in the supplemental dmrpp file.
// var name block must end with " such as name="temperature"
bool find_var_name(const string &str,size_t &str_pos,size_t &var_name_pos_start,size_t &var_name_pos_end) {

    bool ret = true;
    var_name_pos_start = str_pos;
    var_name_pos_end = str.find("\"",str_pos);
    if(var_name_pos_end == string::npos) 
        ret = false;
    else
        str_pos = var_name_pos_end;
#if 0
if(ret==false)
cout<<"cannot find var name"<<endl;
#endif

    return ret;
}

// The end var block must be something like </Float32>
bool find_end_var_block(const string&str, const string&var_type, const size_t &str_pos, size_t &var_end_pos) {

    string end_var = "</" + var_type + '>';
    var_end_pos = str.find(end_var,str_pos);
#if 0
if(var_end_pos==string::npos)
cout<<"cannot find end var block"<<endl;
#endif
    return !(var_end_pos==string::npos);

}

// The chunk info must be confined by either <dmrpp::chunks> and </dmrpp::chunks> or <dmrpp:chunk> and />.
bool find_chunk_info(const string &str,const size_t&str_pos,size_t &chunk_info_pos_start, size_t &chunk_info_pos_end,const size_t&var_end_pos,bool & is_mark1){

    bool ret = true;
    string chunk_start_mark1 = "<dmrpp:chunks";
    string chunk_end_mark1  = "</dmrpp:chunks>";
    string chunk_start_mark2 = "<dmrpp:chunk ";
    string chunk_end_mark2 = "/>";
    char wspace=' ';

#if 0
cout<<"str_pos is "<<str_pos <<endl;
cout<<"var_end_pos is "<<var_end_pos <<endl;
cout<<"substr is "<<str.substr(str_pos,var_end_pos-str_pos)<<endl;
#endif
    chunk_info_pos_start = str.find(chunk_start_mark1,str_pos);
    
    if(string::npos == chunk_info_pos_start) {

        chunk_info_pos_start = str.find(chunk_start_mark2,str_pos);
        if(string::npos != chunk_info_pos_start) 
            chunk_info_pos_end =str.find(chunk_end_mark2,str_pos);

        //This line is used to find the starting point of <dmrpp:chunk, 
        //The character ahead of "<dmrpp::chunk" is always a ' ' (space)
        chunk_info_pos_start = str.find_last_not_of(wspace,chunk_info_pos_start-1)+1;
        is_mark1 = false;
    }
    else { 

        chunk_info_pos_start = str.find_last_not_of(wspace,chunk_info_pos_start-1)+1;  
        chunk_info_pos_end = str.find(chunk_end_mark1,str_pos);
        is_mark1 = true;
       //chunk_info_pos_end = str.find(chunk_end_mark1.c_str(),str_pos,var_end_pos-str_pos);
    }
    if(string::npos == chunk_info_pos_start || string::npos== chunk_info_pos_end)
        ret = false;
    else if(var_end_pos <=chunk_info_pos_end) 
        ret = false;
#if 0
if(ret==false)
cout<<"cannot find_chunk_info "<<endl;
#endif
    return ret;
}

// We need to add the supplemental file path to the chunk info.
// The file name usually starts with "name= ..." and the path usually starts with dmrpp:href=" 
bool add_faddr_chunk_info(const string &str,vector<string>& chunk_info,bool is_dmrpp_mark1, const string faddr_source) {

    bool well_formed= true;
    if(chunk_info.size()==0)
        return true;
    string addr_mark = "dmrpp:href=\"";

    // The missing DMRPP file can have file address specified along with chunk info. 
    // But we assume if they do this for one chunk, they should do this for all chunks.
    // If this is the case, no need to find address.
    if(chunk_info[0].find(addr_mark)!=string::npos) 
        return true;

    // retrieve name and reference
    string hdf5_fname;
    string hdf5_faddr;
    string name_mark = " name=\"";
    string end_delim1 ="\"";
    
    // We must find a valid hdf5 file name.
    size_t hdf5_fname_start_pos = str.find(name_mark);
    if(hdf5_fname_start_pos == string::npos) 
        well_formed = false;
    size_t hdf5_fname_end_pos = str.find(end_delim1,hdf5_fname_start_pos+name_mark.size());
    if(hdf5_fname_end_pos == string::npos)
        well_formed = false;
    hdf5_fname = str.substr(hdf5_fname_start_pos+name_mark.size(),hdf5_fname_end_pos-hdf5_fname_start_pos-name_mark.size());
    if(hdf5_fname=="")
        well_formed = false;
    
    // We also must find a valid file location .
    size_t hdf5_faddr_start_pos = str.find(addr_mark);
    if(hdf5_faddr_start_pos != string::npos) {
        size_t hdf5_faddr_end_pos = str.find(end_delim1,hdf5_faddr_start_pos+addr_mark.size());
        if(hdf5_faddr_end_pos == string::npos)
            well_formed = false;
        hdf5_faddr = str.substr(hdf5_faddr_start_pos+addr_mark.size(),hdf5_faddr_end_pos-hdf5_faddr_start_pos-addr_mark.size());
    }

    if(hdf5_faddr.rfind(hdf5_fname)==string::npos) {
        //trim hdf5 file address.
        hdf5_faddr = " href=\"" +faddr_source+hdf5_faddr+'/'+hdf5_fname+end_delim1;
    }
    else {
        hdf5_faddr = " href=\"" + faddr_source + hdf5_faddr + end_delim1;
    }
//cout<<"hdf5_faddr is "<<hdf5_faddr <<endl;        

    for (int i = 0;i<chunk_info.size();i++) {   
    
        //If is_dmrpp_mark1 is true, 
        //add hdf5_faddr to each chunk line(The chunk line should have offset==)
        //However, the variable may also use the contiguous storage. 
        //That chunk line marks with (nbyte==). Essentially it is not a chunk but
        //the dmrpp still starts with the dmrpp:chunk.
        if(true == is_dmrpp_mark1) 
            add_faddr_chunk_comp_lines(chunk_info[i],hdf5_faddr);
        else 
            add_faddr_contig_line(chunk_info[i],hdf5_faddr);
        
    }
    return well_formed;

}

// Add chunk address when HDF5 chunking is used.
bool add_faddr_chunk_comp_lines(string & chunk_info,const string &file_addr) {

    string chunk_line_mark = "<dmrpp:chunk offset=";
    string chunk_line_end_mark = "/>";
    string chunk_stop_mark = "</dmrpp:chunks>";
    size_t str_pos = 0;
    size_t temp_pos = 0;
    size_t chunk_line_end_pos = 0;
    bool loop_continue = true;
    string temp_str;
    bool well_formed = true;
    bool find_chunk_line = false;

    // While loop from <dmrpp::chunks, until /dmrpp:chunks>
    while(true == loop_continue) {
        temp_pos = chunk_info.find(chunk_line_mark,str_pos);
        if(temp_pos != string::npos) {
            chunk_line_end_pos = chunk_info.find(chunk_line_end_mark,temp_pos);
            if(chunk_line_end_pos != string::npos) {
                find_chunk_line = true;
                temp_str += chunk_info.substr(str_pos,chunk_line_end_pos-str_pos);
                temp_str += file_addr; 
                str_pos = chunk_line_end_pos;
            }
            else {// Each chunk offset line must end with "/>"
                loop_continue = false;
                well_formed = false;
            }
        }
        else {// We will go to the last line </dmrpp:chunks>
            temp_pos = chunk_info.find(chunk_stop_mark,str_pos);
            loop_continue = false;
            //Add the last part of the chunk info. Note: a space between
            //.h5" and "/>"
            if(temp_pos!=string::npos)
                temp_str += ' '+chunk_info.substr(str_pos);
            else 
                well_formed = false;
        }
    }
    if(true == find_chunk_line)
        chunk_info = temp_str;
    else 
        well_formed = false;
    return well_formed;

}

// Add the file address with the contiguous storage.
bool add_faddr_contig_line(string &chunk_info,const string &file_addr) {
    
    bool well_formed = true;
    string chunk_line_start_mark ="<dmrpp::chunk nBytes=";
    string chunk_line_end_mark = "/>";
    string temp_str;

    // Just find the line and change it,this should always be the first line.
    //May add a check to see if the start position is always 0.
    size_t chunk_line_end_pos = chunk_info.find(chunk_line_end_mark);
    if(string::npos == chunk_line_end_pos)
        well_formed = false;
    else {
        temp_str = chunk_info.substr(0,chunk_line_end_pos);
        temp_str +=file_addr;
        temp_str +=' ' +chunk_info.substr(chunk_line_end_pos);
        chunk_info = temp_str;
    }
    return well_formed;
}
    
// Add the missing info to the original dmrpp file.
bool add_missing_info_to_file(const string &fname,const vector<string> &var_types,const vector<string> &var_names,const vector<string> &chunk_info_list) {

    bool well_formed = true;
    string dmrpp_str;

    // The original dmrpp file to string
    file_to_string(fname,dmrpp_str);  
    vector<string>dmrpp_str_vec;
    vector <int> block_index;

    // Convert the original DMRPP string to vector according to var_types and var_names.
    // We need to remember the block index of the missing variables 
    // since the missing variable order in the supplemental dmrpp 
    // may be different than the original one..
    well_formed = convert_dmrppstr_to_vec(dmrpp_str,dmrpp_str_vec,var_types,var_names,block_index);

    // Release the memory of dmpstr. For a >10MB dmrpp file, this is not a small value.
    string().swap(dmrpp_str);

    // adding the missing chunk info to the dmrpp vector and then write back to the file.
    if(true == well_formed) {
        add_missing_info_to_vec(dmrpp_str_vec,chunk_info_list,block_index);
        write_vec_to_file(fname,dmrpp_str_vec);
    }
    return well_formed;

}

// Convert the original dmrpp to vectors according to the *missing* variables. 
// Here we should NOT tokenize the orginal dmrpp according to every variable in it.
// We only care about feeding those variables that miss the value information.
bool convert_dmrppstr_to_vec(const string &dmrpp_str,vector<string> &dmrpp_str_vec,const vector<string> &var_types,const vector<string> &var_names,vector<int>&block_index){

    vector<string>block_begin;
    block_begin.resize(var_types.size());
    vector<string>block_end;
    block_end.resize(var_types.size());
    gen_block(var_types,var_names,block_begin,block_end);

#if 0
for(int i =0; i<block_begin.size();i++)
{
cout<<"block_begin["<<i<<"]= "<<block_begin[i]<<endl;
cout<<"block_end["<<i<<"]= "<<block_end[i]<<endl;

}
#endif

    bool well_formed = split_string(dmrpp_str,dmrpp_str_vec,block_begin,block_end,block_index);
    return well_formed;
 
}

// Add missing information to vector according to the right block_index
void add_missing_info_to_vec(vector<string> &dmrpp_str_vec,const vector<string> &chunk_info_list,const vector<int> &block_index) {
    string temp_str;
    char insert_mark = '>';
    for (int i = 0; i<block_index.size();i++) {
        //cout<<"["<<2*i+1 <<"]= "<<dmrpp_str_vec[2*i+1]<<endl;
        // The vector has to include the beginning and ending block.
        // An example: 
        // The original string:  Moses gre up i Egypt. 
        // The missing information is w in 'gre' and n in 'i'.
        // So we have 2 missing blocks: grew and in.
        // The original string should be divided into 5 to patch the
        // missing characters. "Moses ","gre"," up ","i"," Egypt.".
        // The final string then can be "Moses grew up in Egypt."
        temp_str = dmrpp_str_vec[2*i+1];
        size_t insert_pos = temp_str.find_last_of(insert_mark);
        insert_pos = temp_str.find_last_of(insert_mark,insert_pos-1);

        // The block_index[i] will ensure the right chunk info.
        string temp_str2 = '\n'+chunk_info_list[block_index[i]];
        temp_str.insert(insert_pos+1,temp_str2);
#if 0
        //cout<<"chunk_list["<<block_index[i]<<"]= "<<chunk_info_list[block_index[i]]<<endl;
        //cout<<"temp_str is "<<temp_str <<endl;
#endif
        dmrpp_str_vec[2*i+1] = temp_str;
    }
    
    return;
    
}

// Used in the final step: to generate the final DMRPP file since
// the dmrpp is relatively small, rewriting is still the fast way.
void write_vec_to_file(const string &fname,const vector<string> &dmrpp_str_vec) {

    string str_to_file;
    for (int i =0;i<dmrpp_str_vec.size();i++) 
        str_to_file +=dmrpp_str_vec[i];
        //str_to_file +=dmrpp_str_vec[i]+'\n';
    ofstream outFile;
    outFile.open(fname.c_str());
    outFile<<str_to_file;
    outFile.close();

}

// Obtain the beginning and the ending information of the block information.
void gen_block(const vector<string>&var_type_list,const vector<string>&var_name_list,vector<string>&block_begin,vector<string>&block_end) {
    
    for (int i =0; i<var_type_list.size();i++) {
        block_begin[i] = '<' +var_type_list[i] +' '+"name=\""+var_name_list[i]+"\">";
        block_end[i] = "</" + var_type_list[i] + '>';
    }
}

// Split the string into different blocks.
bool split_string(const string & str, vector<string> &str_vec,const vector<string> &block_begin, const vector<string>&block_end,vector<int>&block_index) {

    bool well_formed = true;
    vector<size_t> block_begin_pos;
    vector<size_t> block_end_pos;
    block_begin_pos.resize(block_begin.size());
    block_end_pos.resize(block_end.size());

    // Note: 
    // 1) We just want to split the string according to the variables that miss values.
    // 2) block_begin_pos in the orginal dmrpp file may NOT be sorted.
    // However, when we read back the string vector, we want to read from beginnng to the end.
    // So we need to remember the index of each <var block> of the supplemental dmrpp file
    // in the original dmrpp file so that the correct chunk info can be given to the var block that misses the values.
    for(int i = 0; i<block_begin.size();i++) {
        block_begin_pos[i] = str.find(block_begin[i]);
        block_end_pos[i] = str.find(block_end[i],block_begin_pos[i])+(block_end[i].size());
    }
    
    obtain_bindex_in_modified_string(block_begin_pos,block_index);

#if 0
for(int i = 0; i<block_index.size();i++)
cout<<"block_index["<<i<<"] is: "<<block_index[i] <<endl;
#endif
    vector<size_t>block_pos;
    block_pos.resize(2*block_begin_pos.size());
    for (int i = 0; i<block_begin.size();i++) {
        block_pos[2*i] = block_begin_pos[i];
        block_pos[2*i+1] = block_end_pos[i];
    }
    
    // This will ensure the string vector is kept from beginning to the end.
    sort(block_pos.begin(),block_pos.end());

    // Use a set: resume a different set, compare with the previous one. set_difference
    // This will ensure that each <var block> doesn't overlap with others.
    // It is a sanity check.
    well_formed = check_overlap_intervals(block_pos,block_begin_pos);

    // We need to consider the starting and the ending of the string
    // So the string vector size is block_size + 1.
    // Examples: 
    // string: Moses grew up in Egypt. It has four space intervals but five substrings.
    if(true == well_formed) {
        size_t str_block_pos = 0;
        str_vec.resize(block_pos.size()+1);
        for (int i =0; i<block_pos.size(); i++) {
            str_vec[i] = str.substr(str_block_pos,block_pos[i]-str_block_pos);
            str_block_pos = block_pos[i];
        }
        str_vec[block_pos.size()] = str.substr(str_block_pos);

#if 0
for(int i = 0; i <str_vec.size();i++) 
    cout<<"str_vec["<<i<<"] is: "<<str_vec[i] <<endl;
#endif
    }
    return well_formed;

}

// Check if there are overlaps between any two var blocks.
// Note: If there are no overlaps between var blocks, the sorted block-start's position set should be
// the same as the unsorted one. This will take O(nlogn) rather than O(n*n) time.
bool check_overlap_intervals(const vector<size_t> &sort_block_pos, const vector<size_t>&block_pos_start){

    // No overlapping, return true.
    set<size_t>sort_start_pos;
    set<size_t>start_pos;
    for (int i = 0; i<block_pos_start.size();i++) {
        sort_start_pos.insert(sort_block_pos[2*i]);
        start_pos.insert(block_pos_start[i]);
    }
    return (sort_start_pos == start_pos);

}

// Obtain the block index of the var block in the supplemental dmrpp file.
// We need to remember the index of a var block in the supplemental dmrpp file to correctly match
// the same var block in the original dmrpp file.
// An example: 
// ex.h5.dmrpp has the variables as the order: ex1,ex2,lon,ex3,fakedim,lat.
// It misses the values of lon,fakedime,lat. 
// In the supplemental dmrpp that has the value information, the variable order is lat,lon,fakedim.
// In order to correctly provide the value info of lon,fakedim and lat without explicitly searching
// the string. I decide to remember the vector index of variables in the supplemental dmrpp file.
// In this case, the index of lat is 0, lon is 1 and fakedim is 2. While adding value info of the 
// missing variables in the ex.h5.dmrpp, I can just use the index to identify which chunk info I 
// should use to fill in.
//
void obtain_bindex_in_modified_string(const vector<size_t>& block_pos_start, vector<int>& block_index) {

    vector<pair<size_t,int> > pos_index;
    for (int i = 0; i <block_pos_start.size();i++)
        pos_index.push_back(make_pair(block_pos_start[i],i));

    // The pos_index will be sorted according to the first element,block_pos_start
    sort(pos_index.begin(),pos_index.end());

    for (int i = 0; i <block_pos_start.size();i++)
        block_index.push_back(pos_index[i].second);
    return;
}

// Help function: read the file content to a string.
void file_to_string(const string &filename, string &out_str) {

    ifstream inFile;
    inFile.open(filename.c_str()); 
 
    stringstream strStream;
    strStream << inFile.rdbuf(); 

    // Save the content to the string
    out_str = strStream.str(); 
    inFile.close();

}

//tokenize the string to a vector of string according the delim.
void string_tokenize(const string &in_str,const char delim,vector<string>&out_vec) {
    stringstream ss_str(in_str);
    string temp_str;
    while (getline(ss_str,temp_str,delim)) { 
        out_vec.push_back(temp_str);
    }
}

