#include <iostream>
#include<fstream>
#include <string>
#include <vector>
using namespace std;

bool find_var(const string &str, const vector<string>var_type_list,
              vector<string>&var_type,vector<string>&var_name);
bool find_endvar(const string &str,const string vtype);
bool find_chunk(const string &str);

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
    
    // var_type and var_name should be var data type and var name in the dmrpp file
    vector<string>var_type;
    vector<string>var_name;

    //The vector to check if chunk block inside this var block(<var ..> </var>) 
    vector<bool>chunk_exist;

    // The following flags are used to check the variables that miss the values.
    // In a dmrpp file, an example of variable block may start from
    // <Float32 name="temperature"> and end with </Float32>
    // fin_vb_start: flag to find the start of the var block
    // fin_vb_end: flag  to find the end of the var block
    // chunk_found: flag to find is chunking information is inside the var block
    bool fin_vb_start = false;
    bool fin_vb_end = false;
    bool chunk_found = false;

    // Check every line of the dmrpp file. This will use less memory.
    while(getline(dmrpp_fstream,dmrpp_line)) {

        // If we find the start of the var block(<var..>)
        if(true == fin_vb_start) {

            // var data type must exist.
            if(var_type.empty()) {
                cout<<"Doesn't have the variable datatype, abort for dmrpp file "<<fname << endl;
                return -1;
            }
            // Not find the end of var block. try to find it.
            if(false == fin_vb_end) 
                fin_vb_end = find_endvar(dmrpp_line, var_type[var_type.size()-1]);

            // If find the end of var block, check if the chunk is already found in the var block.
            if(true == fin_vb_end) {
                if(false == chunk_found)
                    chunk_exist.push_back(false);

                // If we find the end of this var block, 
                // reset all bools for the next variable.
                fin_vb_start = false;
                fin_vb_end = false;
                chunk_found = false;
            }
            else {// Check if having chunks within this var block.
                if(false == chunk_found) {
                    chunk_found = find_chunk(dmrpp_line);
                    // When finding the chunk info, update the chunk_exist vector.
                    if(true == chunk_found)
                        chunk_exist.push_back(true);
                }
            }
        }
        else // Continue finding the var block
            fin_vb_start = find_var(dmrpp_line,var_type_list,var_type,var_name);
          
    }

    //Sanity check to make sure the chunk_exist vector is the same as var_type vector.
    //If not, something is wrong with this dmrpp file.
    if(chunk_exist.size()!=var_type.size()) { 
        cout<<"Number of chunk check is not consistent with the number of var check."<<endl;
        cout<< "The dmrpp file is "<<fname<<endl;
        return -1;
    }

#if 0
for(int i = 0; i<var_type.size(); i++)
cout<<"var_type["<<i<<"]= "<<var_type[i]<<endl;
for(int i = 0; i<var_name.size(); i++) {
cout<<"var_name["<<i<<"]= "<<var_name[i]<<endl;
cout<<"chunk_exist["<<i<<"]= "<<chunk_exist[i]<<endl;
}
#endif

    bool has_missing_info = false;
    int last_missing_chunk_index = -1;

    // Check if there are any missing variable information.
    for (int i =var_type.size()-1;i>=0;i--) {
        if(false == chunk_exist[i]){
            has_missing_info = true;
            last_missing_chunk_index = i;
            break;
        }
    }

#if 0
    int j = 0;
    for (int i =0;i<var_type.size();i++) {
        if(false == chunk_exist[i]){
            j++;
            if(j == 1) 
                cout<<"The following variables don't have data value information(datatype + data name): "<<endl;
            cout<< var_type[i] <<" "<<var_name[i] <<endl;
        }
    }
#endif

    // Report the final output.
    if(true == has_missing_info) {

        ofstream dmrpp_ofstream;
        string fname2(argv[2]);
        dmrpp_ofstream.open(fname2.c_str(),ofstream::out);

        for (int i =0;i<var_type.size();i++) {
            if(false == chunk_exist[i]) {
                if (i!=last_missing_chunk_index)
                    dmrpp_ofstream<<var_name[i] <<",";
                else 
                    dmrpp_ofstream<<var_name[i];
            }
        }

        dmrpp_ofstream.close();
    }
    

    return 0;

}

// Find the the var type and var name like <Int16 name="foo">
bool find_var(const string &str, const vector<string>var_type_list,
              vector<string>&var_type,vector<string>&var_name) {

    bool ret = false;
    //if(str[0]=='\n' || str[0]!=' '){ 
    // Every var block will have spaces before <
    if(str[0]!=' '){ 
        return ret;
    }

    // Ignore the line with all spaces
    size_t non_space_char_pos = str.find_first_not_of(' ');
    if(non_space_char_pos == string::npos){
        return ret;
    }

    // The first non-space character should be '<'
    if(str[non_space_char_pos]!='<') {
        return ret;
    }
    
    // After space, must at least contain '<','>' 
    if(str.size() <= (non_space_char_pos+1)){
        return ret;
    }
    
    // The last character must be '>', maybe this is too strict.
    // We will see.
    if(str[str.size()-1]!='>' ) {
        return ret;
    }
    
    // char_2 is a character right after<
    char char_2 = str[non_space_char_pos+1];

    // The first var character must be one of the list.
    // The following list includes the first character
    // of all possible variable types.
    string v_1char_list = "FIUBSC";

    // If the first character is not one of DAP type,ignore.
    if(v_1char_list.find_first_of(char_2)==string::npos) {
        return ret;
    }
    
    // Find ' name="' and the position after non_space_char_pos+1, like <Int16 name="d16_1">
    string sep=" name=\"";
    size_t sep_pos = str.find(sep,non_space_char_pos+2);

    // Cannot find "name=..", ignore this line.
    if(sep_pos == string::npos){
        return ret;
    }

    // Try to figure out the variable type.
    int var_index = -1;
    for (int i = 0; i<var_type_list.size();i++) {
        if(str.compare(non_space_char_pos+1,sep_pos-non_space_char_pos-1,var_type_list[i]) == 0) {
            var_index = i;
            break;
        }
    }

    // If cannot find the supported type, ignore this line.
    if(var_index == -1) {
        return ret;
    }
    
    // Find the end quote position of the variable name.
    char end_quote='"';
    size_t end_name_pos = str.find(end_quote,sep_pos+sep.size()+1);
    if(end_name_pos == string::npos)
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

// Find whether there are chunks inside the var block.
// Any chunk info(chunk or contiguous) should include
// "<dmrpp:chunk " and "offset".
bool find_chunk(const string &str) {
    bool ret = false;
    string chunk_mark = "<dmrpp:chunk ";
    string offset_mark = "offset";
    size_t chunk_mark_pos = str.find(chunk_mark);
    if(chunk_mark_pos !=string::npos) {
        if(string::npos != str.find(offset_mark, chunk_mark_pos+chunk_mark.size())) 
            ret = true;
    }
    return ret;
}

// Find the end of var block such as </Int32>
// There may be space before </Int32>
bool find_endvar(const string &str, const string vtype) {
    bool ret = false;
    string end_var = "</" + vtype + '>';
    size_t vb_end_pos = str.find(end_var);
    if(vb_end_pos !=string::npos) {
        if((vb_end_pos + end_var.size())==str.size())
            ret = true;
    } 
    return ret;
}

        
        




       




