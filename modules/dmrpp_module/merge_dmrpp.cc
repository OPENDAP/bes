#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <algorithm>

using namespace std;

// I. The following block of functions retrieves the "missing" variable type, variable name and data value information.
// 1. General
bool obtain_var_info(const string &miss_dmrpp_info, const vector<string> &var_type_check_list,
                     vector<string> &var_types, vector<string> &var_names, vector<string> &chunk_info_list,
                     bool &is_chunk_mark1);
bool find_var_name(const string &str, size_t &str_pos, size_t &var_name_pos_start, size_t &var_name_pos_end);
bool find_end_var_block(const string &str, const string &var_type, const size_t &str_pos, size_t &var_end_pos);
bool find_chunk_info(const string &str, const size_t &str_pos, size_t &chunk_info_pos_start, size_t &chunk_info_pos_end,
                     const size_t &var_end_pos, bool &is_mark1);

// 2. group handling
bool obtain_var_path_info(const string &fname, const vector<string> &var_type_list, vector<string> &var_path,
                          vector<string>& var_type, vector<string> &var_name, vector<unsigned int> &var_lines);

bool obtain_var_grp_info(const string &fname, const vector<string> &var_type_list, vector<string> &grp_names,
                         vector<unsigned int> &grp_lines, vector<unsigned int> &end_grp_lines,
                         vector<string> &var_type, vector<string> &var_name, vector<unsigned int> &var_lines);

string obtain_var_grp_paths(const vector<unsigned int> &gs_line_nums,const vector<unsigned int> &ge_line_nums,
                            const vector<string> &grp_names, unsigned int var_line);

int obtain_gse_line_index(const vector<unsigned int> &gse_line_nums, unsigned int var_line);

bool find_grp(const string &str, unsigned int line_num, vector<string> &grp_names,
              vector<unsigned int> &grp_lines, vector<unsigned int> &end_grp_lines);

bool find_end_grp(const string &dmrpp_line,unsigned int line_num, vector<unsigned int> &end_grp_lines);

bool find_var_helper(const string &dmrpp_line, const vector<string> &var_type_list, vector<string>& var_type,
                     vector<string>&var_name);

bool find_var(const string &str, const vector<string> &var_type_list, vector<string> &var_type, vector<string> &var_name,
              vector<unsigned int> &var_lines, unsigned int line_num);

bool merge_chunk_info_g(const string &str, const vector<string> &var_type, const vector<string> &var_name,
                        const vector<string> &var_fqn, const vector<string> &miss_var_fqn,
                        const vector<string> &miss_var_type, const vector<string> &chunk_info);

bool obtain_miss_var_candidate_pos(const string &dmrpp_str, const vector<string> &var_type,
                                   const vector<string> &var_name, vector<size_t> &var_pos);

void obtain_final_miss_var_info(const vector <string> &var_fqn, const vector <string> &miss_var_fqn,
                                const vector <string> &var_type, vector <string> &final_var_type,
                               const vector <size_t> &var_candidate_pos, vector<size_t> &var_pos,
                               const vector <string> &chunk_info, vector<string> &ordered_chunk_info);

bool merge_chunk_info_vec(vector <string> &dmrpp_vec, const vector<string> &miss_var_type,
                          const vector<string> &ordered_chunk_info);

bool insert_chunk_info_to_vec(string &dmrpp_block_str, const string &var_type, const string &chunk_info);

// The following block of functions add the file address
// (mostly the absolute path of the HDF5 file that stores the data value) to the chunk block.
bool add_faddr_chunk_info_simple(vector<string> &chunk_info_list, bool is_dmrpp_mark1, const string& faddr_source = "");

// The original function considers more factors that seem not necessary.
#if 0
//bool add_faddr_chunk_info(const string& miss_dmrpp_info,vector<string>&chunk_info_list,bool is_dmrpp_mark1, const string faddr_source = "");
#endif

bool add_faddr_contig_line(string &chunk_info, const string &file_addr);
bool add_faddr_chunk_comp_lines(string &chunk_info,const string &file_addr);

// The following block of functions merge the "missing" variable data value information to the original dmrpp file.
// These routines are used for the case when there are no groups in the dmrpp file. The algorithm is faster.
bool add_missing_info_to_file(const string &fname2,const vector<string> &var_types,const vector<string> &var_names,
                              const vector<string> &chunk_info_list);
void gen_block(const vector<string> &var_type_list,const vector<string> &var_name_list,vector<string> &block_begin,
               vector<string> &block_end);
bool check_overlap_intervals(const vector<size_t> &sort_block_pos, const vector<size_t> &block_pos_start);
void obtain_bindex_in_modified_string(const vector<size_t> &block_pos_start, vector<int> &block_index);
bool split_string(const string &str, vector<string> &str_vec, const vector<string> &block_begin,
                  const vector<string> &block_end, vector<int> &block_index);
bool convert_dmrppstr_to_vec(const string &dmrpp_str, vector<string> &dmrpp_str_vec, const vector<string> &var_types,
                             const vector<string> &var_names,vector<int> &block_index);
void add_missing_info_to_vec(vector<string> &dmrpp_str_vec,const vector<string> &chunk_info_list,
                             const vector<int> &block_index);
void write_vec_to_file(const string &fname,const vector<string> &dmrpp_str_vec);

// The following two functions are helper functions
void file_to_string(const string &filename, string &out);
bool string_tokenize(const string &in_str, const char delim, vector<string>&out_vec);
bool string_tokenize_by_pos(const string &in_str, const vector<size_t> &var_pos, vector<string> &out_vec);


int main(int argc,char**argv)
{
    string dmrpp_line;
    vector<string> var_types;
    vector<string> var_names;
    vector<string> chunk_info_list;
    
    bool add_dmrpp_info = false;
    bool is_chunk_mark1 = true;

    string missing_dmrpp_str;

    if(argc != 5) {
        cout<<"Please provide four arguments: "<< endl;
        cout<<"  The first is the dmrpp file that contains the missing variable value information. "<<endl;
        cout<<"  The second is the original dmrpp file. "<<endl;
        cout<<"  The third one is the href to the missing variables HDF5 file. "<<endl;
        cout<<"  The fourth one is the text file that includes the missing variable information. "<<endl;
        cout <<endl;
        cout <<" Warning: before running this program, one must run the check_dmrpp program first to see "
             <<"if the original dmrpp file contains any missing variable, the variable that cannot find the"
             <<" data in the original data file. "<<endl; 
        return 0;
    }

    // We only consider the atomic datatype for the missing variables.
    vector<string> var_type_check_list;

    var_type_check_list.emplace_back("Float32");
    var_type_check_list.emplace_back("Int32");
    var_type_check_list.emplace_back("Float64");
    var_type_check_list.emplace_back("Byte");
    var_type_check_list.emplace_back("Int16");
    var_type_check_list.emplace_back("UInt16");
    var_type_check_list.emplace_back("String");
    var_type_check_list.emplace_back("UInt32");
    var_type_check_list.emplace_back("Int8");
    var_type_check_list.emplace_back("Int64");
    var_type_check_list.emplace_back("UInt64");
    var_type_check_list.emplace_back("UInt8");
    var_type_check_list.emplace_back("Char");

    // Obtain the dmrpp file name that contains the missing variable value.
    string fname(argv[1]);

    // Read the "missing dmrpp file" to a string
    file_to_string(fname,missing_dmrpp_str);

    // Obtain the missing chunk information from the dmrpp file.
    add_dmrpp_info = obtain_var_info(missing_dmrpp_str,var_type_check_list,var_types,var_names,
                                     chunk_info_list,is_chunk_mark1);

    // Just output a warning if there is no chunk info, in the supplemental dmrpp file.
    if (false == add_dmrpp_info) {
        cout<<"Cannot find the corresponding chunk info. from the supplemental dmrpp file."<<endl;
        cout<<"You may need to check if there is any variable in the dmrpp file. "<<endl;
        cout<<"The dmrpp file is "<<fname <<endl;
    }

    // Sanity check
    if (var_types.size() != var_names.size() || var_names.size() != chunk_info_list.size()) {
        cout <<"Var type, var name and chunk_info must have the same number of elements. "<<endl;
        cout <<"The dmrpp file is "<<fname <<endl;
        return 0;
    }

    // For debugging
#if 0
    for (size_t i =0; i<var_names.size();i++) {
cout<<"var type["<<i<<"] "<< var_types[i]<<endl;
cout<<"var name["<<i<<"] "<< var_names[i]<<endl;
cout<<"chunk_info_list["<<i<<"] "<< chunk_info_list[i] << endl;

    }
#endif

    // We need to erase those variables that are not really missing but are added by the generation program
    string mvar_fname(argv[4]);
    string missing_vname_str;

    // Read the missing variable names to a string and tokenize the string to a vector of string.
    file_to_string(mvar_fname,missing_vname_str);

    vector<string> missing_vname_list;

    // Here we come to the different syntax of DAP2 and DAP4 constraints.
    // DAP2 uses comma(,) whereas DAP4 uses semicolon(;). We need to support both.
    char delim=';';
    bool has_delim = string_tokenize(missing_vname_str,delim,missing_vname_list);
    if (!has_delim) {
        delim=',';
        missing_vname_list.clear();
        string_tokenize(missing_vname_str,delim,missing_vname_list);
    }

    // Check if the dmrpp file that contains just the missing variables has groups.
    // Note: we don't need to consider if there are groups in the original dmrpp file since
    //       we only care about the insertion of the chunk info for the missing variables.
    bool handle_grp = false;
    for (const auto &mv_name:missing_vname_list) {
        // Find the last path position.
        size_t path_pos = mv_name.find_last_of('/');

        // The missing variables under the root group are treated as no-group.
        if (path_pos !=string::npos && path_pos!=0) {
            handle_grp = true;
            break;
        }
    }

    // For debugging
#if 0
    for(size_t i = 0;i<missing_vname_list.size();i++)
        cout <<"missing_vname_list["<<i<<"]= "<<missing_vname_list[i]<<endl;
#endif

    // We need to handle differently if finding group(s) in the missing dmrpp file.
    if (handle_grp == true) {

        // Obtain the variable path of all the variables in the missing dmrpp string.
        vector<string> mdp_var_fqn;
        vector<string> mdp_var_names_g;
        vector<string> mdp_var_types_g;
        vector<unsigned int> mdp_var_lines;

        if (false == obtain_var_path_info(fname, var_type_check_list, mdp_var_fqn, mdp_var_types_g,
                                          mdp_var_names_g, mdp_var_lines))
            return -1;

        // Now we can use the var_path to match the missing vars,
        // Remove the additional variables added by the filenetCDF-4 module.
        vector<string> new_var_types;
        vector<string> new_var_names;
        vector<string> new_var_fqns;
        vector<string> new_chunk_info_list;

        if (mdp_var_names_g != var_names) {
            cout <<" Internal error: variable names should be the same even retrieved with different methods."<<endl;
            return -1;
        }

        for (size_t i =0; i<mdp_var_fqn.size();i++) {
            for (const auto & mvl:missing_vname_list) {
                if (mdp_var_fqn[i] == mvl) {
                    new_var_names.push_back(mdp_var_names_g[i]);
                    new_var_fqns.push_back(mdp_var_fqn[i]);
                    new_var_types.push_back(mdp_var_types_g[i]);
                    new_chunk_info_list.push_back(chunk_info_list[i]);
                    break;
                }
            }
        }

        // Add the file location to each chunk. Mostly the file location is the absolute path of the HDF5 file.
        string fadd_source(argv[3]);
        add_faddr_chunk_info_simple(new_chunk_info_list,is_chunk_mark1,fadd_source);
#if 0
        //add_faddr_chunk_info(missing_dmrpp_str,new_chunk_info_list,is_chunk_mark1,fadd_source);
#endif

        // For debugging
#if 0
for (const auto &nc_info:new_chunk_info_list) 
cout <<"chunk_info "<<nc_info <<endl;
#endif

        // Now go to the original dmrpp. Find the missing var blocks based on the var_path.
        // Obtain the variable path of all the variables in the missing dmrpp string.
        string fname2(argv[2]);
        vector<string> odp_var_fqn;
        vector<string> odp_var_names_g;
        vector<string> odp_var_types_g;
        vector<unsigned int> odp_var_lines;
        
        // Note: if the original dmrpp file contains many variables and groups, this may take some time. We will see if 
        //          the performance is an issue. 
        if (false == obtain_var_path_info(fname2,var_type_check_list,odp_var_fqn,
                                          odp_var_types_g,odp_var_names_g,odp_var_lines))
            return -1;

        // To reduce the comparison of all the variable path in the original dmrpp file with the variables in the
        // missing dmrpp file,we further select only the relevant variables: the variables that hold the same names as
        // those of missing variables.
        // For example, the missing variable path is /foo/missing1, /foo2/missing2.
        // In the original dmrpp we may have 100 variables,
        // the relevant variables may just include missing1, /foo/missing1, /foo2/missing2, /foo/foo1/missing2. 
        // We only need to compare these four variable path with the missing variable path to identify the location of
        // the variables that miss the chunk information in the original dmrpp file.

        vector<unsigned int> final_odp_var_lines;
        vector<string> final_odp_var_fqns;
        vector<string> final_odp_var_names;
        vector<string> final_odp_var_types;

        for (unsigned int i = 0; i < odp_var_names_g.size();i++) {
            for (unsigned int j = 0; j < new_var_names.size(); j++) {
                if ((odp_var_names_g[i] == new_var_names[j]) && (odp_var_types_g[i] == new_var_types[j])) {
                    final_odp_var_lines.push_back(odp_var_lines[i]);
                    final_odp_var_fqns.push_back(odp_var_fqn[i]);
                    final_odp_var_names.push_back(odp_var_names_g[i]);
                    final_odp_var_types.push_back(odp_var_types_g[i]);
                    break;
                }
            }
        }

        // debugging info
#if 0
cout <<" Before the final step "<<endl;
for (unsigned int i = 0; i <final_odp_var_types.size(); i++) {
cout <<"vtype: "<<final_odp_var_types[i] <<endl;
cout <<"vname: "<<final_odp_var_names[i] <<endl;
cout <<"vfqn: "<<final_odp_var_fqns[i] <<endl;
cout <<"new vfqn: "<<new_var_fqns[i] <<endl;
}
#endif

        // Merge the missing chunk info to the original dmrpp file.
        merge_chunk_info_g(fname2,final_odp_var_types,final_odp_var_names,
                           final_odp_var_fqns,new_var_fqns,new_var_types,
                           new_chunk_info_list);

        // This is not necessary now. But leave it for now.
#if 0
        // Now go to the original dmrpp. Find the missing var blocks based on the var_path. 
        // We need to compare the var full path in the missing dmrpp(new_var_fqns) with the var full path
        // in the original dmrpp(odp_var_fqn).
        // We want to find the corresponding line number in the original dmrpp to insert the missing chunk info.
        vector <unsigned int> missing_chunk_info_lines;
        for (unsigned int i = 0; i < new_var_fqns.size(); i++) {
            for (unsigned int j = 0; j <final_odp_var_fqns.size(); j++) {
                if (new_var_fqns[i] == final_odp_var_fqns[j]) {
                    missing_chunk_info_lines.push_back(final_odp_var_lines[j]);
                    break;
                }
            }
        }
#endif

// for debugging info
#if 0
for (const auto &mcil:missing_chunk_info_lines) 
    cout <<"missing chunk info line is: "<<mcil <<endl;
#endif

    }
    else {

#if 0
cout <<"coming to the nogroup case"<<endl;
#endif

        // Remove the additional variables added by the filenetCDF-4 module.
        vector<string> new_var_types;
        vector<string> new_var_names;
        vector<string> new_chunk_info_list;

        // Trim missing_vname_list if the missing_vname_list includes the root path /.
        vector<string> missing_vname_list_trim;
        for (size_t j = 0; j <missing_vname_list.size();j++) {
            string temp_str = missing_vname_list[j];
            if (temp_str[0] == '/') 
                temp_str = temp_str.substr(1);
            missing_vname_list_trim.push_back(temp_str);
        }

        for (size_t i =0; i<var_names.size();i++) {
            for (size_t j = 0; j<missing_vname_list_trim.size();j++) {
                if (var_names[i] == missing_vname_list_trim[j]) {
                    new_var_names.push_back(var_names[i]);
                    new_var_types.push_back(var_types[i]);
                    new_chunk_info_list.push_back(chunk_info_list[i]);
                    break;
                }
            }
        }

        // Add the file location to each chunk. Mostly the file location is the absolute path of the HDF5 file.
        string fadd_source(argv[3]);

#if 0
        //add_faddr_chunk_info(missing_dmrpp_str,new_chunk_info_list,is_chunk_mark1,fadd_source);
#endif
        add_faddr_chunk_info_simple(new_chunk_info_list,is_chunk_mark1,fadd_source);

#if 0
for (size_t i =0; i<new_var_types.size();i++)  {
    cout<<"new chunk_info_list["<<i<<"]"<< endl;
    cout<<new_chunk_info_list[i]<<endl;
}
#endif

        string fname2(argv[2]);

        // Add the missing chunk info to the original dmrpp file.
        bool well_formed = add_missing_info_to_file(fname2,new_var_types,new_var_names,
                                                    new_chunk_info_list);

        if (false == well_formed) {
            cout <<"The dmrpp file to be modified is either not well-formed or contains nested variable blocks ";
            cout <<"that cannot be supported by this routine. " <<endl;
            cout <<"The dmrpp file is "<<fname2<<endl;
        }
    }

    return 0;

}

// Obtain the var info from the supplemental(missing) dmrpp file.
// The variable types we checked are limited to DAP2 data types plus 64-bit integers.
bool obtain_var_info(const string &miss_dmrpp_info,const vector<string> &var_type_check_list,
                     vector<string> &var_types, vector<string> &var_names,vector<string> &chunk_info_list,
                     bool &is_chunk_mark1) {

    bool ret = false;

    size_t var_type_pos_start = 0;
    size_t var_name_pos_start = 0;
    size_t var_name_pos_end = 0;
    size_t chunk_pos_start = 0;
    size_t chunk_pos_end = 0;
    size_t var_end_pos = 0;
    size_t str_pos = 0;

    if (miss_dmrpp_info.empty())
        return ret;

    size_t str_last_char_pos = miss_dmrpp_info.size() - 1;
    bool well_formed = true;

    // Go through the whole missing dmrpp string
    while (str_pos <= str_last_char_pos && well_formed) {

        size_t i = 0;
        string var_sign;
        string temp_var_sign;
        size_t temp_var_type_pos_start = string::npos;
        int var_type_index = -1;

        // Go through the var_type_check_list to obtain the var data type,
        // We need to find the index in the var_type_check_list to
        // obtain the correct var datatype.
        while (i < var_type_check_list.size()) {

            var_sign = "<" + var_type_check_list[i] + " name=\"";
            var_type_pos_start = miss_dmrpp_info.find(var_sign, str_pos);

            if (var_type_pos_start == string::npos) {
                i++;
                continue;
            } else {

                // We want to make sure we don't skip any vars.
                if (temp_var_type_pos_start > var_type_pos_start) {
                    temp_var_type_pos_start = var_type_pos_start;
                    var_type_index = i;
                    temp_var_sign = var_sign;
                }
                i++;
            }
        }

        // Ensure all variables are scanned.
        if (temp_var_type_pos_start != string::npos) {
            var_type_pos_start = temp_var_type_pos_start;
            var_sign = temp_var_sign;
        }

        // This line will ignore the datatypes that are not in the var_type_check_list
        if (var_type_pos_start == string::npos) {
            str_pos = string::npos;
            continue;
        } else
            str_pos = var_type_pos_start + var_sign.size();

        // Now we can retrieve var name, var type and the corresponding chunk info.
        // Sanity check is also applied.
        if (false == find_var_name(miss_dmrpp_info, str_pos, var_name_pos_start, var_name_pos_end))
            well_formed = false;
        else if (false == find_end_var_block(miss_dmrpp_info, var_type_check_list[var_type_index],
                                             str_pos, var_end_pos))
            well_formed = false;
        else if (false == find_chunk_info(miss_dmrpp_info, str_pos, chunk_pos_start, chunk_pos_end,
                                          var_end_pos, is_chunk_mark1))
            well_formed = false;
        else {

            // Move the string search pos to the next block
            str_pos = var_end_pos + 1;

            // Obtain var type, var name and chunk info. and save them to vectors.
            var_types.push_back(var_type_check_list[var_type_index]);
            var_names.push_back(miss_dmrpp_info.substr(var_name_pos_start, var_name_pos_end - var_name_pos_start));
            string temp_chunk_info = miss_dmrpp_info.substr(chunk_pos_start, chunk_pos_end - chunk_pos_start);
            if (true == is_chunk_mark1)
                temp_chunk_info += "</dmrpp:chunks>";
            else
                temp_chunk_info += "/>";
            chunk_info_list.push_back(temp_chunk_info);
        }
    }

    return well_formed;
}

// Find the var name in the supplemental dmrpp file.
// var name block must end with " such as name="temperature"
bool find_var_name(const string &str,size_t &str_pos,size_t &var_name_pos_start,size_t &var_name_pos_end) {

    bool ret = true;
    var_name_pos_start = str_pos;
    var_name_pos_end = str.find("\"",str_pos);
    if (var_name_pos_end == string::npos)
        ret = false;
    else
        str_pos = var_name_pos_end;

    // debugging info
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

    // debugging info
#if 0
if(var_end_pos==string::npos)
cout<<"cannot find end var block"<<endl;
#endif

    return !(var_end_pos==string::npos);

}

// The chunk info must be confined by either <dmrpp::chunks> and </dmrpp::chunks> or <dmrpp:chunk> and />.
bool find_chunk_info(const string &str,const size_t&str_pos,size_t &chunk_info_pos_start, size_t &chunk_info_pos_end,
                     const size_t&var_end_pos,bool & is_mark1){

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
    
    if (string::npos == chunk_info_pos_start) {
        chunk_info_pos_start = str.find(chunk_start_mark2,str_pos);
        if(string::npos != chunk_info_pos_start) 
            chunk_info_pos_end = str.find(chunk_end_mark2,str_pos);

        // This line is used to find the starting point of <dmrpp:chunk,
        // The character ahead of "<dmrpp::chunk" is always a ' ' (space)
        chunk_info_pos_start = str.find_last_not_of(wspace,chunk_info_pos_start-1) + 1;
        is_mark1 = false;
    }
    else {
        chunk_info_pos_start = str.find_last_not_of(wspace,chunk_info_pos_start-1) + 1;
        chunk_info_pos_end = str.find(chunk_end_mark1,str_pos);
        is_mark1 = true;
#if 0
        //chunk_info_pos_end = str.find(chunk_end_mark1.c_str(),str_pos,var_end_pos-str_pos);
#endif
    }
    if (string::npos == chunk_info_pos_start || string::npos == chunk_info_pos_end)
        ret = false;
    else if (var_end_pos <= chunk_info_pos_end)
        ret = false;
#if 0
if (ret == false)
    cout<<"cannot find_chunk_info "<<endl;
#endif
    return ret;
}

// We need to add the supplemental file path to the chunk info.
// It seems that we don't need the sanity check of the missing data dmrpp based on the current get_dmrpp implementation.
// Make this routine simpler.
bool add_faddr_chunk_info_simple(vector<string>& chunk_info, bool is_dmrpp_mark1, const string &faddr_source) {

    if (chunk_info.size() == 0)
        return true;
    string addr_mark = "dmrpp:href=\"";

    // The missing DMRPP file can have file address specified along with chunk info. 
    // But we assume if they do this for one chunk, they should do this for all chunks.
    // If this is the case, no need to find address.
    if (chunk_info[0].find(addr_mark)!=string::npos)
        return true;

    // Retrieve name and reference
    string hdf5_faddr;
    string end_delim1 ="\"";

    // The string for use in each missing_variable <chunk href:"value" >
    hdf5_faddr = " href=\"" + faddr_source + end_delim1;

#if 0
//cout<<"hdf5_faddr is "<<hdf5_faddr <<endl;        
#endif

    for (size_t i = 0; i<chunk_info.size(); i++) {
    
        //If is_dmrpp_mark1 is true, 
        //add hdf5_faddr to each chunk line(The chunk line should have offset==)
        //However, the variable may also use the contiguous storage. 
        //That chunk line marks with (nbyte==). Essentially it is not a chunk but
        //the dmrpp still starts with the dmrpp:chunk.
        if (true == is_dmrpp_mark1)
            add_faddr_chunk_comp_lines(chunk_info[i],hdf5_faddr);
        else 
            add_faddr_contig_line(chunk_info[i],hdf5_faddr);
        
    }
    return true;

}

// The following code is not used. Leave it here now.
#if 0
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

    // The string for use in each missing_variable <chunk href:"value" >
    hdf5_faddr = " href=\"" + faddr_source + end_delim1;

    /*if (hdf5_faddr.rfind(hdf5_fname) == string::npos) {
        //trim hdf5 file address.
        hdf5_faddr = " href=\"" +hdf5_faddr+'/'+hdf5_fname+end_delim1;
    }
    else {
        hdf5_faddr = " href=\"" +hdf5_faddr+end_delim1;
    }*/

//cout<<"hdf5_faddr is "<<hdf5_faddr <<endl;        

    for (size_t i = 0;i<chunk_info.size();i++) {
    
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
#endif

// Add the chunk address when the HDF5 chunking address is used.
bool add_faddr_chunk_comp_lines(string & chunk_info, const string &file_addr) {

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
    while (true == loop_continue) {
        temp_pos = chunk_info.find(chunk_line_mark,str_pos);
        if (temp_pos != string::npos) {

            chunk_line_end_pos = chunk_info.find(chunk_line_end_mark,temp_pos);
            if (chunk_line_end_pos != string::npos) {
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
        else { // We will go to the last line </dmrpp:chunks>

            temp_pos = chunk_info.find(chunk_stop_mark,str_pos);
            loop_continue = false;

            //Add the last part of the chunk info. Note: a space between
            //.h5" and "/>"
            if (temp_pos != string::npos)
                temp_str += ' '+ chunk_info.substr(str_pos);
            else 
                well_formed = false;
        }
    }
    if (true == find_chunk_line)
        chunk_info = temp_str;
    else 
        well_formed = false;
    return well_formed;

}

// Add the file address with the contiguous storage.
bool add_faddr_contig_line(string &chunk_info, const string &file_addr) {
    
    bool well_formed = true;
    string chunk_line_start_mark = "<dmrpp::chunk nBytes=";
    string chunk_line_end_mark = "/>";
    string temp_str;

    // Just find the line and change it,this should always be the first line.
    //May add a check to see if the start position is always 0.
    size_t chunk_line_end_pos = chunk_info.find(chunk_line_end_mark);
    if (string::npos == chunk_line_end_pos)
        well_formed = false;
    else {
        temp_str = chunk_info.substr(0,chunk_line_end_pos);
        temp_str += file_addr;
        temp_str += ' ' +chunk_info.substr(chunk_line_end_pos);
        chunk_info = temp_str;
    }
    return well_formed;
}
    
// Add the missing info to the original dmrpp file.
bool add_missing_info_to_file(const string &fname,const vector<string> &var_types,const vector<string> &var_names,
                              const vector<string> &chunk_info_list) {

    bool well_formed = true;
    string dmrpp_str;

    // The original dmrpp file to string
    file_to_string(fname,dmrpp_str);

    vector<string> dmrpp_str_vec;
    vector<int> block_index;

    // Convert the original DMRPP string to vector according to var_types and var_names.
    // We need to remember the block index of the missing variables 
    // since the missing variable order in the supplemental dmrpp 
    // may be different from the original one.
    well_formed = convert_dmrppstr_to_vec(dmrpp_str,dmrpp_str_vec,var_types,var_names,block_index);

    // Release the memory of dmpstr. For a >10MB dmrpp file, this is not a small value.
    string().swap(dmrpp_str);

    // adding the missing chunk info to the dmrpp vector and then write back to the file.
    if (true == well_formed) {
        add_missing_info_to_vec(dmrpp_str_vec,chunk_info_list,block_index);
        write_vec_to_file(fname,dmrpp_str_vec);
    }
    return well_formed;
}

// Convert the original dmrpp to vectors according to the *missing* variables. 
// Here we should NOT tokenize the orginal dmrpp according to every variable in it.
// We only care about feeding those variables that miss the value information.
bool convert_dmrppstr_to_vec(const string &dmrpp_str, vector<string> &dmrpp_str_vec,
                             const vector<string> &var_types, const vector<string> &var_names,
                             vector<int> &block_index) {

    vector<string>block_begin;
    block_begin.resize(var_types.size());
    vector<string>block_end;
    block_end.resize(var_types.size());
    gen_block(var_types,var_names,block_begin,block_end);

#if 0
for(size_t i =0; i<block_begin.size();i++)
{
cout<<"block_begin["<<i<<"]= "<<block_begin[i]<<endl;
cout<<"block_end["<<i<<"]= "<<block_end[i]<<endl;

}
#endif

    bool well_formed = split_string(dmrpp_str,dmrpp_str_vec,block_begin,block_end,block_index);
    return well_formed;
 
}

// Add missing information to vector according to the right block_index
void add_missing_info_to_vec(vector<string> &dmrpp_str_vec,const vector<string> &chunk_info_list,
                             const vector<int> &block_index) {

    string temp_str;
    char insert_mark = '>';
    for (size_t i = 0; i < block_index.size(); i++) {

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
        string temp_str2 = '\n' + chunk_info_list[block_index[i]];
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
void write_vec_to_file(const string &fname, const vector<string> &dmrpp_str_vec) {

    string str_to_file;
    for (size_t i =0;i<dmrpp_str_vec.size();i++)
        str_to_file +=dmrpp_str_vec[i];

    ofstream outFile;
    outFile.open(fname.c_str());
    outFile<<str_to_file;
    outFile.close();

}

// Obtain the beginning and the ending information of the block information.
void gen_block(const vector<string> &var_type_list,const vector<string> &var_name_list,
               vector<string> &block_begin, vector<string> &block_end) {
    
    for (size_t i = 0; i < var_type_list.size(); i++) {
        block_begin[i] = '<' +var_type_list[i] +' '+"name=\""+var_name_list[i]+"\">";
        block_end[i] = "</" + var_type_list[i] + '>';
    }
}

// Split the string into different blocks.
bool split_string(const string &str, vector<string> &str_vec, const vector<string> &block_begin,
                  const vector<string> &block_end,vector<int> &block_index) {

    bool well_formed = true;
    vector<size_t> block_begin_pos;
    vector<size_t> block_end_pos;
    block_begin_pos.resize(block_begin.size());
    block_end_pos.resize(block_end.size());

    // Note: 
    // 1) We just want to split the string according to the variables that miss values.
    // 2) block_begin_pos in the original dmrpp file may NOT be sorted.
    // However, when we read back the string vector, we want to read from beginning to the end.
    // So we need to remember the index of each <var block> of the supplemental dmrpp file
    // in the original dmrpp file so that the correct chunk info can be given to the var block that misses the values.
    for(size_t i = 0; i<block_begin.size(); i++) {
        block_begin_pos[i] = str.find(block_begin[i]);
        block_end_pos[i] = str.find(block_end[i],block_begin_pos[i])+(block_end[i].size());
    }
    
    obtain_bindex_in_modified_string(block_begin_pos,block_index);

#if 0
for(size_t i = 0; i<block_index.size();i++)
cout<<"block_index["<<i<<"] is: "<<block_index[i] <<endl;
#endif
    vector<size_t> block_pos;
    block_pos.resize(2*block_begin_pos.size());
    for (size_t i = 0; i < block_begin.size(); i++) {
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
    if (true == well_formed) {

        size_t str_block_pos = 0;
        str_vec.resize(block_pos.size()+1);
        for (size_t i =0; i < block_pos.size(); i++) {
            str_vec[i] = str.substr(str_block_pos,block_pos[i]-str_block_pos);
            str_block_pos = block_pos[i];
        }
        str_vec[block_pos.size()] = str.substr(str_block_pos);

#if 0
for(size_t i = 0; i <str_vec.size();i++)
    cout<<"str_vec["<<i<<"] is: "<<str_vec[i] <<endl;
#endif
    }

    return well_formed;

}

// Check if there are overlaps between any two var blocks.
// Note: If there are no overlaps between var blocks, the sorted block-start's position set should be
// the same as the unsorted one. This will take O(nlogn) rather than O(n*n) time.
bool check_overlap_intervals(const vector<size_t> &sort_block_pos, const vector<size_t> &block_pos_start){

    // No overlapping, return true.
    set<size_t>sort_start_pos;
    set<size_t>start_pos;
    for (size_t i = 0; i<block_pos_start.size();i++) {
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
void obtain_bindex_in_modified_string(const vector<size_t> &block_pos_start, vector<int> &block_index) {

    vector<pair<size_t,int> > pos_index;
    for (size_t i = 0; i <block_pos_start.size(); i++)
        pos_index.push_back(make_pair(block_pos_start[i],i));

    // The pos_index will be sorted according to the first element,block_pos_start
    sort(pos_index.begin(),pos_index.end());

    for (size_t i = 0; i < block_pos_start.size(); i++)
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

// Tokenize the string to a vector of string according to the delim.
bool string_tokenize(const string &in_str, const char delim, vector<string> &out_vec) {
    stringstream ss_str(in_str);
    string temp_str;
    while (getline(ss_str,temp_str,delim))
        out_vec.push_back(temp_str);

    return (in_str.find(delim)!= string::npos);
}

// Tokenize the string to a vector of string according to positions.
// We assume the positions are pre-sorted from small to large.
bool string_tokenize_by_pos(const string &in_str,const vector<size_t>& pos, vector<string> &out_vec) {

    if (pos.empty() || pos.front() ==0 || (pos.back()+1) >in_str.size())
        return false;

    out_vec.push_back(in_str.substr(0,pos[0]));
    for (unsigned int i = 0; i < (pos.size()-1); i++)
        out_vec.push_back(in_str.substr(pos[i],pos[i+1]-pos[i]));
    out_vec.push_back(in_str.substr(pos.back()));

#if 0
for (unsigned int i = 0; i < out_vec.size(); i ++)
cout <<"string vec is: " <<  out_vec[i] <<endl;
#endif

     return true;
}

// Find the var type and var name like <Int16 name="foo">
bool find_var_helper(const string &str, const vector<string> &var_type_list,
                     vector<string> &var_type, vector<string> &var_name) {

    bool ret = false;

#if 0
    //if(str[0]=='\n' || str[0]!=' '){
#endif

    // Every var block will have spaces before <
    if (str[0]!=' ')
        return ret;

    // Ignore the line with all spaces
    size_t non_space_char_pos = str.find_first_not_of(' ');
    if (non_space_char_pos == string::npos)
        return ret;

    // The first non-space character should be '<'
    if (str[non_space_char_pos] != '<')
        return ret;

    // After space, must at least contain '<','>' 
    if (str.size() <= (non_space_char_pos+1))
        return ret;

    // The last character must be '>', maybe this is too strict.
    // We will see.
    if (str[str.size()-1] != '>' )
        return ret;

    // char_2 is a character right after<
    char char_2 = str[non_space_char_pos+1];

    // The first var character must be one of the list.
    // The following list includes the first character
    // of all possible variable types.
    string v_1char_list = "FIUBSC";

    // If the first character is not one of DAP type,ignore.
    if (v_1char_list.find_first_of(char_2) == string::npos)
        return ret;
    
    // Find ' name="' and the position after non_space_char_pos+1, like <Int16 name="d16_1">
    string sep=" name=\"";
    size_t sep_pos = str.find(sep,non_space_char_pos+2);

    // Cannot find "name=..", ignore this line.
    if (sep_pos == string::npos)
        return ret;

    // Try to figure out the variable type.
    bool found_var_index = false;
    size_t var_index = 0;
    for (size_t i = 0; i<var_type_list.size(); i++) {
        if (str.compare(non_space_char_pos+1,sep_pos-non_space_char_pos-1,var_type_list[i]) == 0) {
            var_index = i;
            found_var_index = true;
        }
    }

    // If cannot find the supported type, ignore this line.
    if (!found_var_index)
        return ret;
    
    // Find the end quote position of the variable name.
    char end_quote='"';
    size_t end_name_pos = str.find(end_quote,sep_pos+sep.size()+1);
    if (end_name_pos == string::npos)
        ret = false;
    else {
        // Find both var type and var name. Store them in the vector
        string var_name_line = str.substr(sep_pos+sep.size(),end_name_pos-sep_pos-sep.size());
        var_type.push_back(var_type_list[var_index]);
        var_name.push_back(var_name_line);
        ret = true;
    }

    return ret;    
}

bool find_var(const string &str, const vector<string> &var_type_list, vector<string>&var_type,
              vector<string>&var_name, vector<unsigned int> &var_lines,unsigned int line_num) {

    bool ret_value = find_var_helper(str,var_type_list,var_type,var_name);
    if (ret_value == true) 
        var_lines.push_back(line_num);
    return ret_value;
}

// Find  group
bool find_grp(const string &str, unsigned int line_num, vector<string> &grp_names,
              vector<unsigned int> &grp_lines, vector<unsigned int> &end_grp_lines) {

    bool ret = false;

    // Every group block will have spaces before <
    if (str[0]!=' ')
        return ret;

    // Ignore the line with all spaces
    size_t non_space_char_pos = str.find_first_not_of(' ');
    if (non_space_char_pos == string::npos)
        return ret;

    // The first non-space character should be '<'
    if (str[non_space_char_pos]!='<')
        return ret;
    
    // After space, must at least contain '<','>' 
    if (str.size() <= (non_space_char_pos+1))
        return ret;
    
    // The last character must be '>', maybe this is too strict.
    // We will see.
    if (str[str.size()-1]!='>' )
        return ret;

    // char_2 is a character right after<
    char char_2 = str[non_space_char_pos+1];
    if (char_2 != 'G')
        return ret;
    
    // Find ' name="' and the position after non_space_char_pos+1, like <Int16 name="d16_1">
    string sep="Group name=\"";
    size_t sep_pos = str.find(sep,non_space_char_pos+1);

    // Cannot find "Group name=", ignore this line.
    if (sep_pos == string::npos){
        return ret;
    }

    // Find the end quote position of the group name.
    char end_quote='"';
    size_t end_name_pos = str.find(end_quote,sep_pos+sep.size()+1);
    if (end_name_pos == string::npos)
        ret = false;
    else {
        // Store the group name in the vector
        string grp_name = str.substr(sep_pos+sep.size(),end_name_pos-sep_pos-sep.size());
        grp_names.push_back(grp_name);
        grp_lines.push_back(line_num);

        // We also need to check the empty group case. That is when Group name="foo"/>
        // For this case, we need to remember this line also as the end group line. 
        // Like <Group name="FILE_ATTRIBUTES"/>
        if ((str.size() >(end_name_pos+1)) && str[end_name_pos+1]=='/')  
            end_grp_lines.push_back(line_num);
        
        ret = true;
    }
    
    return ret;    
}

// Find the end of var block such as </Int32>
// There may be space before </Int32>
bool find_end_grp(const string &dmrpp_line,unsigned int line_num, vector<unsigned int> &end_grp_lines) {
    bool ret = false;
    string end_grp = "</Group>" ;
    size_t end_grp_pos = dmrpp_line.find(end_grp);
    if (end_grp_pos != string::npos) {
        if ((end_grp_pos + end_grp.size()) == dmrpp_line.size()) {
            end_grp_lines.push_back(line_num);
            ret = true;
        }
    } 
    return ret;
}



// Obtain the variable path.
string obtain_var_grp_paths(const vector<unsigned int> &gs_line_nums,
                            const vector<unsigned int> &ge_line_nums,
                            const vector<string> &grp_names,
                            unsigned int var_line) {
    string ret_value;

    vector<unsigned int> gse_line_nums;
    vector<bool> is_group_start;

    unsigned int end_grp_index = 0;
    unsigned int start_grp_index = 0;

    // The maximum index of the group is the number of groups minus 1 since index is from 0.
    unsigned int max_grp_index = gs_line_nums.size() -1;

    // We combine both group lines and end_group lines to one vector.
    // Another vector of bool with the same size is created to mark if
    // this line is a start_of_a_group or an end_of_a_group.
    // During this process, we also eliminate the trivial groups.
    
    while (end_grp_index <= max_grp_index) {

        while (start_grp_index <= max_grp_index) {

            if (gs_line_nums[start_grp_index] < ge_line_nums[end_grp_index]) {
                gse_line_nums.push_back(gs_line_nums[start_grp_index]); 
                is_group_start.push_back(true);
                start_grp_index++;
            }
            else if (gs_line_nums[start_grp_index] == ge_line_nums[end_grp_index]) {
                // Exclude the case when the starting group line is equal to the ending group line.
                // This is the empty group case.
                start_grp_index++;
                end_grp_index++;
            }
            else {
                gse_line_nums.push_back(ge_line_nums[end_grp_index]);
                is_group_start.push_back(false);
                end_grp_index++;
            }
        }
        if (end_grp_index < (max_grp_index+1)) {
                gse_line_nums.push_back(ge_line_nums[end_grp_index]);
                is_group_start.push_back(false);
                end_grp_index++;
        }
    }

    // No need to check this. It should always be true.
#if 0
    if (is_group_start.size() != gse_line_nums.size()) {
        cerr<<"The group "<<endl;
        return ret_value;
    }
#endif 

    // Debugging info, leave the block now.
#if 0
for (unsigned int i =0; i<gse_line_nums.size();i++) {
    cerr<<"gse_line["<<i<<"] = "<<gse_line_nums[i] <<endl;
    cerr<<"is_group_start["<<i<<"] = "<<is_group_start[i] <<endl;
}
#endif

    // Obtain the start_end_group line index just before the the variable line.
    int gse_line_index= obtain_gse_line_index(gse_line_nums,var_line);   

#if 0
cerr<<"gse_line_index: "<<gse_line_index <<endl;
#endif

    // obtain group lines that this variable belongs to.
    vector<unsigned int> grp_path_lines;

    if (gse_line_index >= 0) {

        int temp_index = gse_line_index;

        // temp_rem_grp_index indicates the groups we need to remove for this var.
        unsigned int temp_rem_grp_index = 0;

        // We have to search backward.
        while (temp_index >= 0) {

           // Encounter an end-group, we need to increase the index.
           if (is_group_start[temp_index] == false)
               temp_rem_grp_index++;
           else {
               // Only when the number of end-group counter is 0 for this block,
               // does this group path belong to this variable.
               if (temp_rem_grp_index == 0)
                    grp_path_lines.push_back(gse_line_nums[temp_index]);
               else
                    temp_rem_grp_index--; //Cancel one start-group and end-group
           }
           temp_index--;
        }
    }

    // For debugging
#if 0
for (const auto &gpl:grp_path_lines)
cerr<<"grp_path_lines "<<gpl <<endl;
for (const auto &gsn:gs_line_nums)
cerr<<"gs_lines "<<gsn <<endl;
for (const auto &gn:grp_names)
cerr<<"group name  is "<<gn <<endl;
#endif

    // Both the group path for this var and the group lines are sorted.
    // group path is from backward. So we match the group line backward.
    int gl_index = gs_line_nums.size() - 1; // An gl_index starts at size-1

    for (const auto &gpl:grp_path_lines) {

        // Note: gl_index is modified. This is intentionally since
        // we don't need to search the lines already visited.
        // We just need to prepend the group path as we search backward.
        for (; gl_index >= 0; gl_index--) {

            if (gpl == gs_line_nums[gl_index]) {

                ret_value = "/" + grp_names[gl_index] + ret_value;
                gl_index--;
                break;
            }
        }
    }  

#if 0
cerr<<"ret_value is "<<ret_value <<endl;
#endif

    return ret_value;

}

// Obtain the start_end_group line index just before the variable line.
// The returned value is -1 if there is no group before this var.
int obtain_gse_line_index(const vector<unsigned int> &gse_line_nums, unsigned int var_line) {

    int ret_value = -1;
    unsigned int total_gse_lines = gse_line_nums.size();
    
    if (total_gse_lines > 0) { 

        for (int i = total_gse_lines-1;  i>=0 ; i--) {
            if (gse_line_nums[i] >var_line) 
                continue;
            else {
                ret_value = i;
                break;
            }
        }

    }
    return ret_value;
}

bool obtain_var_path_info(const string &fname, const vector<string> &var_type_list, vector<string> &var_fqn,
                          vector<string> &var_type, vector<string> &var_name, vector<unsigned int> &var_lines) {

    vector<string> grp_names;
    vector<unsigned int> grp_lines;
    vector<unsigned int> end_grp_lines;
    
    bool has_group = obtain_var_grp_info(fname,var_type_list,grp_names,grp_lines,end_grp_lines,var_type, var_name,var_lines);
    if (!has_group)  {
        cout <<" the missing variable info shows this dmrpp has groups, however, no group is found. "<<endl;
        return false;
    }
    for (unsigned int i =0; i <var_lines.size(); i++) {
        string var_path = obtain_var_grp_paths(grp_lines,end_grp_lines,grp_names,var_lines[i]);
        string vfqn = var_path + "/" + var_name[i];
        var_fqn.push_back(vfqn);
    }

// For debugging
#if 0
for (unsigned int i = 0; i <var_lines.size(); i++) {
cerr<<" var fqn: "<<var_fqn[i] <<endl;
cerr<<" var line: "<<var_lines[i] <<endl;

}
#endif

    return true;
}

bool obtain_var_grp_info(const string &fname,const vector<string> &var_type_list, vector<string> &grp_names,
                         vector<unsigned int> &grp_lines, vector<unsigned int> &end_grp_lines,
                         vector<string> &var_type, vector<string> &var_name, vector<unsigned int> &var_lines) {

    string dmrpp_line;

    // find  <Group> and </Group>
    bool find_grp_start = false;
    bool find_grp_end = false;

    unsigned int line_num = 0;

    ifstream dmrpp_fstream;
    dmrpp_fstream.open(fname.c_str(),ifstream::in);
 
    while(getline(dmrpp_fstream,dmrpp_line)) {

        find_grp_start = find_grp(dmrpp_line,line_num,grp_names,grp_lines,end_grp_lines);

        if (find_grp_start == false) 
            find_grp_end = find_end_grp(dmrpp_line,line_num,end_grp_lines);
        if (!find_grp_start && !find_grp_end) 
            find_var(dmrpp_line,var_type_list,var_type, var_name,var_lines,line_num);
        line_num++;
    }

    return !(grp_names.empty());

}

bool merge_chunk_info_g(const string &fname, const vector<string> &var_type,const vector<string> &var_name,
                        const vector<string> &var_candidate_fqn, const vector<string> &miss_var_fqn,
                        const vector<string> &miss_var_type, const vector<string> &chunk_info) {

    string dmrpp_str;
    bool ret_value = true;

    // Read the "original dmrpp file" to a string
    file_to_string(fname,dmrpp_str);

#if 0
cout <<"dmrpp_str is "<<dmrpp_str<<endl;
#endif

    // Now find the *possible* missing variable positions in the dmrpp_str based on the variable names
    // and types in the original dmrpp file.
    vector<size_t> var_candidate_pos;
    ret_value = obtain_miss_var_candidate_pos(dmrpp_str, var_type, var_name,var_candidate_pos);

    if (ret_value == false)
        return ret_value;
#if 0
for (const auto &vcp:var_candidate_pos) 
    cout <<"pos is: "<<vcp <<endl;
#endif

    
    // Convert string according to the string positions.
    vector<string> dmrpp_vec;
    vector<string> ordered_chunk_info;

    // Find the positions and the datatypes of the final missing variables in the original dmrpp file.
    vector<string> final_var_type;
    vector<size_t> var_pos;
    obtain_final_miss_var_info(var_candidate_fqn,miss_var_fqn,miss_var_type,final_var_type,var_candidate_pos, var_pos,chunk_info,ordered_chunk_info);

#if 0
for (const auto &oci:ordered_chunk_info)
    cout << "chunk info: "<<oci <<endl;
for (const auto &fvt:final_var_type)
    cout << "fvt: "<<fvt <<endl;

#endif

    string_tokenize_by_pos(dmrpp_str, var_pos, dmrpp_vec); 
    
    ret_value = merge_chunk_info_vec(dmrpp_vec, final_var_type, ordered_chunk_info);
    if (ret_value == true)
        write_vec_to_file(fname,dmrpp_vec);

    return ret_value;
}

bool obtain_miss_var_candidate_pos(const string &dmrpp_str, const vector<string> &var_type,
                                   const vector<string> &var_name, vector<size_t> &var_pos) {

    bool ret_value = true;
    size_t str_start_pos = 0;
    for (unsigned int i = 0; i < var_name.size(); i++) {

        string var_sign = "<"+var_type[i] +" name=\"" + var_name[i] +"\">";
        size_t v_pos = dmrpp_str.find(var_sign,str_start_pos);
        if (v_pos == string::npos) {
            cout <<"Cannot find the var name " << var_name[i] << "in the original dmrpp file "<<endl;
            ret_value = false;
            break;
        }
        var_pos.push_back(v_pos);
        str_start_pos = v_pos + var_sign.size();
    }

    return ret_value;
}

void obtain_final_miss_var_info(const vector<string> &var_fqn, const vector<string> &miss_var_fqn,
                               const vector<string> &miss_var_type, vector<string> &final_var_type,
                               const vector<size_t> &var_candidate_pos, vector<size_t> &var_pos,
                               const vector<string> &chunk_info, vector<string> &ordered_chunk_info) {

    for (unsigned int i = 0; i<var_fqn.size(); i++) {
        for (unsigned int j = 0; j<miss_var_fqn.size(); j++) {

            // This block assures that the chunk info is put in the right place of a missing variable
            // in the original dmrpp file.
            if (var_fqn[i] == miss_var_fqn[j]) {
                var_pos.push_back(var_candidate_pos[i]);
                final_var_type.push_back(miss_var_type[j]);
                ordered_chunk_info.push_back(chunk_info[j]);
                break;
            }
        }
    }
}


bool merge_chunk_info_vec(vector<string> &dmrpp_vec, const vector<string> &miss_var_type,
                          const vector<string> &ordered_chunk_info) {

    bool ret_value = true;
    // Note: the first element of the dmrpp_vec doesn't have chunk info.
    for (unsigned int i = 1; i < dmrpp_vec.size(); i++) {
        string temp_dmrpp_seg = dmrpp_vec[i];
        ret_value  = insert_chunk_info_to_vec(temp_dmrpp_seg, miss_var_type[i-1], ordered_chunk_info[i-1]);
        if (ret_value == false) 
            break;
        else 
            dmrpp_vec[i] = temp_dmrpp_seg;
    }

    return ret_value;
}

bool insert_chunk_info_to_vec(string &dmrpp_block_str, const string &var_type, const string &chunk_info) {

    bool ret_value = true;
    string end_var = "</" + var_type + '>';   
    size_t end_var_pos = dmrpp_block_str.find(end_var);

    if (end_var_pos == string::npos) { 
        cout << "Cannot find:\n "<<end_var << " \n in the string \n"<<dmrpp_block_str <<endl;
        ret_value = false;
    }
    else {

        char add_chunk_mark = '>';
        size_t chunk_mark_pos = dmrpp_block_str.rfind(add_chunk_mark,end_var_pos);
        if (chunk_mark_pos == string::npos) { 
            cout << "Cannot find:\n "<<add_chunk_mark << " \n in the string \n"<<dmrpp_block_str <<endl;
            ret_value = false;
        }
        else {
            string before_chunk_info_str = dmrpp_block_str.substr(0,chunk_mark_pos+1);
            string after_chunk_info_str = dmrpp_block_str.substr(chunk_mark_pos+1);
            dmrpp_block_str = before_chunk_info_str + '\n' + chunk_info + after_chunk_info_str;
        } 
    }

    return ret_value;
}

