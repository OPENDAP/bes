#include <iostream>
#include<fstream>
#include <string>
#include <vector>
using namespace std;

bool find_var_helper(const string &str, const vector<string> &var_type_list,
                     vector<string> &var_type, vector<string> &var_name);

bool find_var(const string &str, const vector<string> &var_type_list,
              vector<string> &var_type, vector<string> &var_name,
              vector<unsigned int> &var_lines, unsigned int line_num);
bool find_endvar(const string &str,const string &vtype);

bool find_raw_data_location_info(const string &str);
bool find_fillValue_in_chunks(const string &str);
bool find_data_offset(const string &str);
bool find_embedded_data_info(const string &str);

string obtain_var_grp_paths(const vector<unsigned int> &gs_line_nums,
                            const vector<unsigned int> &ge_line_nums,
                            const vector<string> &grp_names,
                            unsigned int var_line);

int obtain_gse_line_index(const vector<unsigned int> &gse_line_nums, unsigned int var_line);

bool find_grp(const string &str, unsigned int line_num, vector<string> &grp_names,
              vector<unsigned int> &grp_lines, vector<unsigned int> &end_grp_lines);

bool find_end_grp(const string &dmrpp_line, unsigned int line_num, vector<unsigned int> &end_grp_lines);

bool obtain_grp_info(const string &fname, vector<string> &grp_names, vector<unsigned int> &grp_lines,
                     vector<unsigned int> &end_grp_lines);

int main (int argc, char** argv)
{
    // Provide the dmrpp file name and the file name to store the variables that miss values
    if(argc !=3) {
        cout<<"Please provide the dmrpp file name to be checked and the output name."<<endl;
        return -1;
    }

    string fname(argv[1]);
    ifstream dmrpp_fstream;
    dmrpp_fstream.open(fname.c_str(),ifstream::in);
    string dmrpp_line;

    // DAP4 supported atomic datatype 
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
    var_type_list.push_back("Structure");
    
    // var_type and var_name should be var data type and var name in the dmrpp file
    vector<string> var_type;
    vector<string> var_name;
    vector<unsigned int> var_lines;

    //The vector to check if there is raw data info inside this var block(<var ..> </var>) 
    vector<bool> data_exist;

    // The following flags are used to check the variables that miss the values.
    // In a dmrpp file, an example of variable block may start from
    // <Float32 name="temperature"> and end with </Float32>
    // fin_vb_start: flag to find the start of the var block
    // fin_vb_end: flag  to find the end of the var block
    // data_found: flag to find if raw data information is inside the var block
    bool fin_vb_start = false;
    bool fin_vb_end = false;
    bool data_found = false;

    unsigned int line_num = 0;

    // Check every line of the dmrpp file.
    // We also remember the line number for every variable.
    while (getline(dmrpp_fstream,dmrpp_line)) {

        // If we find the start of the var block(<var..>)
        if (true == fin_vb_start) {

            // var data type must exist.
            if (var_type.empty()) {
                cout<<"Doesn't have the variable datatype, abort for dmrpp file "<<fname << endl;
                return -1;
            }
            // Not find the end of var block. try to find it.
            if (false == fin_vb_end)
                fin_vb_end = find_endvar(dmrpp_line, var_type[var_type.size()-1]);

            // If find the end of var block, check if the raw data info is already found in the var block.
            if (true == fin_vb_end) {

                if (false == data_found)
                    data_exist.push_back(false);

                // If we find the end of this var block, 
                // reset all bools for the next variable.
                fin_vb_start = false;
                fin_vb_end = false;
                data_found = false;
            }
            else {// Check if having raw data info. within this var block.

                if (false == data_found) {

                    data_found = find_raw_data_location_info(dmrpp_line);

                    // When finding the raw data location info in this dmrpp file, update the data_exist vector.
                    if (true == data_found)
                        data_exist.push_back(true);
                        
                }
            }
        }
        else // Continue finding the var block
            fin_vb_start = find_var(dmrpp_line,var_type_list,var_type,var_name, var_lines,line_num);
          
        line_num++;
    }

    //Sanity check to make sure the data_exist vector is the same as var_type vector.
    //If not, something is wrong with this dmrpp file.
    if (data_exist.size() != var_type.size()) {
        cout<<"Number of chunk check is not consistent with the number of var check."<<endl;
        cout<< "The dmrpp file is "<<fname<<endl;
        return -1;
    }

    bool has_missing_info = false;
    size_t last_missing_chunk_index = 0;

    // Check if there is any missing variable information.
    // Here we need to remember the last missing chunking index for the final output.
    if (!var_type.empty()) {
        auto ritr = var_type.rbegin();
        size_t i = var_type.size() - 1;
        while (ritr != var_type.rend()) {
            if (!data_exist[i]) {
                has_missing_info = true;
                last_missing_chunk_index = i;
                break;
            }
            ritr++;
            i--;
        }
    }

    // Report the final output.
    if (true == has_missing_info) {

        // Check group and var_grp names for group hierarchy enhancement.
        vector<string> grp_names;
        vector<unsigned int> grp_lines;
        vector<unsigned int> end_grp_lines;
 
        bool has_grps = obtain_grp_info(fname,grp_names,grp_lines,end_grp_lines);
        if (grp_lines.size() != end_grp_lines.size()) {
            cout<<"The number of group bracket is NOT the same as the number of end group bracket."<<endl;
            return -1;
        }

        // fname2 is the output file that contains the missing variable information.
        ofstream dmrpp_ofstream;
        string fname2(argv[2]);
        dmrpp_ofstream.open(fname2.c_str(),ofstream::out | ofstream::trunc);

        // We need to loop through every variable. Note: we just another index to obtain the corresponding
        // variable lines and variable names.
        size_t i = 0;
        for (auto vt:var_type) {
            if(!data_exist[i]) {
                string var_str;
                if (has_grps) {
                    // We need to obtain the missing variable FQN.
                    string var_path = obtain_var_grp_paths(grp_lines,end_grp_lines,grp_names,var_lines[i]);
                    var_str = var_path + "/" + var_name[i];
                }
                else 
                    var_str = var_name[i];

                // Note: DAP4 constraint syntax needs semicolon(';") whereas DAP2 constraint needs comma(',');
                // Essentially the DAP4 constraint is general enough to cover everything, however, the current
                // get_dmrpp still uses the DAP2 constraint. To keep it compatible with get_dmrpp for the non-group case,
                // I still keep comma.
                if (i != last_missing_chunk_index) {
                    if (has_grps) 
                        dmrpp_ofstream<<var_str <<";";
                    else
                        dmrpp_ofstream<<var_str <<",";
                }
                else  
                    dmrpp_ofstream<<var_str;
            }
            i++;
        }
    }
    return 0;

}

// Find the var type and var name like <Int16 name="foo">
bool find_var_helper(const string &str, const vector<string> &var_type_list,
                    vector<string> &var_type,vector<string> &var_name) {

    bool ret = false;

    // Every var block will have spaces before <
    if (str[0] != ' ')
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
    size_t var_index = -1;
    bool found = false;
    for (size_t i = 0; i < var_type_list.size() && !found ; i++) {
        if(str.compare(non_space_char_pos+1,sep_pos-non_space_char_pos-1,var_type_list[i]) == 0) {
            var_index = i;
            found = true;
        }
    }

    // If cannot find the supported type, ignore this line.
    if (!found)
        return ret;

    // Find the end quote position of the variable name.
    char end_quote='"';
    size_t end_name_pos = str.find(end_quote,sep_pos + sep.size() + 1);
    if (end_name_pos != string::npos) {

        // Find both var type and var name. Store them in the vector
        string var_name_line = str.substr(sep_pos + sep.size(),end_name_pos - sep_pos - sep.size());
        var_type.push_back(var_type_list[var_index]);
        var_name.push_back(var_name_line);
        ret = true;
    }
    return ret;    
}

// Find if this var block contains the raw data info.
bool find_raw_data_location_info(const string &dmrpp_line) {

    bool ret = false;

    // Check if this var contains data storage key word fillValue.
    ret  = find_fillValue_in_chunks(dmrpp_line);

    // Check if this var contains the key word chunk or block and offset.
    if (ret == false)
        ret  = find_data_offset(dmrpp_line);

    // Also need to find if having a key word such as dmrpp:missingdata.
    // These key words indicate the data is stored inside the dmrpp file.
    if (false == ret) 
        ret = find_embedded_data_info(dmrpp_line);

    return ret;

}

// Find if this var block contains dmrpp:chunks and fillValue 
bool find_fillValue_in_chunks(const string &str) {

    bool ret = false;
    string fvalue_mark = "fillValue";
    string dmrpp_chunks_mark = "<dmrpp:chunks ";

    size_t dmrpp_chunks_mark_pos = str.find(dmrpp_chunks_mark);
    if (dmrpp_chunks_mark_pos != string::npos) {
        if (string::npos != str.find(fvalue_mark, dmrpp_chunks_mark_pos+dmrpp_chunks_mark.size())) 
            ret = true;
    }
    return ret;

}

// Find whether there are chunks or blocks inside the var block.
// Any chunk info(chunk or contiguous) should include
// "<dmrpp:chunk " / "<dmrpp:block " and "offset".
bool find_data_offset(const string &str) {

    bool ret = false;
    string offset_mark = "offset";
    vector<string> data_storage_mark_list = {"<dmrpp:chunk ","<dmrpp:block "};

    for (const auto & data_storage_mark:data_storage_mark_list) {

        size_t data_storage_mark_pos = str.find(data_storage_mark);
        if (data_storage_mark_pos != string::npos) {
            if (string::npos != str.find(offset_mark, data_storage_mark_pos+data_storage_mark.size())) {
                ret = true;
                break;
            }
        }
    }
    return ret;
}

// Find whether there are embedded_data_info in this var block.
// Currently the embedded_data_info includes <dmrpp:compact>, <dmrpp:missingdata>, <dmrpp:vlsa>
// and <dmrpp:specialstructuredata>.
bool find_embedded_data_info(const string &str) {

    bool ret = false;
    vector<string> embedded_data_block_list = {"<dmrpp:compact>",
                                               "<dmrpp:missingdata>",
                                               "<dmrpp:vlsa>",
                                               "<dmrpp:specialstructuredata>"};

    for (const auto & embedded_data_block:embedded_data_block_list) {
        size_t embedded_data_block_pos = str.find(embedded_data_block);
        if (embedded_data_block_pos != string::npos) {
            ret = true;
            break;
        }
    }
    return ret;
}


// Find the end of var block such as </Int32>
// There may be space before </Int32>
bool find_endvar(const string &str, const string &vtype) {

    bool ret = false;
    string end_var = "</" + vtype + '>';
    size_t vb_end_pos = str.find(end_var);
    if (vb_end_pos != string::npos) {
        if ((vb_end_pos + end_var.size()) == str.size())
            ret = true;
    } 
    return ret;
}

bool find_var(const string &str, const vector<string> &var_type_list,
              vector<string> &var_type,vector<string> &var_name,
              vector<unsigned int> &var_lines, unsigned int line_num) {

    bool ret_value = find_var_helper(str,var_type_list,var_type,var_name);
    if (ret_value == true) 
        var_lines.push_back(line_num);
    return ret_value;
}
    
// obtain the group names, group line numbers and end group line numbers.
// The return value is true if there are any groups.
bool obtain_grp_info(const string &fname, vector<string> &grp_names,
                     vector<unsigned int> &grp_lines,vector<unsigned int> &end_grp_lines)
{    

    string dmrpp_line;

    // find  <Group>
    bool find_grp_start = false;
    unsigned int line_num = 0;

    ifstream dmrpp_fstream;
    dmrpp_fstream.open(fname.c_str(),ifstream::in);
 
    while(getline(dmrpp_fstream,dmrpp_line)) {

        find_grp_start = find_grp(dmrpp_line,line_num,grp_names,grp_lines,end_grp_lines);
        if (find_grp_start == false) 
            find_end_grp(dmrpp_line,line_num,end_grp_lines);
        line_num++;
    }

    return !(grp_names.empty());

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
    if (char_2 != 'G')
        return ret;
    
    // Find ' name="' and the position after non_space_char_pos+1, like <Int16 name="d16_1">
    string sep="Group name=\"";
    size_t sep_pos = str.find(sep,non_space_char_pos+1);

    // Cannot find "Group name=", ignore this line.
    if (sep_pos == string::npos)
        return ret;

    // Find the end quote position of the group name.
    char end_quote='"';
    size_t end_name_pos = str.find(end_quote,sep_pos+sep.size()+1);
    if (end_name_pos != string::npos) {

        // Store the group name in the vector
        string grp_name = str.substr(sep_pos+sep.size(),end_name_pos-sep_pos-sep.size());
        grp_names.push_back(grp_name);
        grp_lines.push_back(line_num);

        // We also need to check the empty group case. That is when Group name="foo"/>
        // For this case, we need to remember this line also as the end group line. 
        // Like <Group name="FILE_ATTRIBUTES"/>
        if ((str.size() > (end_name_pos+1)) && str[end_name_pos+1]=='/')
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
                            const vector<string> & grp_names,
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
    // During this process, we elimiate the trivial groups. 
    
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
        // end group </Group> will always be at last.
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
        // The removed groups are groups that don't contain this variable.
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
    int gl_index = gs_line_nums.size() - 1; // gl_index should start with size-1 since we count backwards to zero

    for (const auto & gpl:grp_path_lines) {

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
    auto total_gse_lines = (unsigned int)(gse_line_nums.size());
    
    if (total_gse_lines > 0) { 

        for (int i = total_gse_lines-1;  i >= 0 ; i--) {
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

 

       




