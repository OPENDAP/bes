#include <cstdio>
#include <cstdlib>
#include <vector>
#include <iostream>

using namespace std;

int main(int argc, char *argv[]) {

    if (argc != 4) {
        printf("Please provide the HDF5 file name and the HDF5 real data offset and size-in-bytes as the following:\n");
        printf(" ./test_read_vl_offset h5_file_name offset size \n");
        return 0;
    }
    char *t_ptr;

    string filename(argv[1]);
    cout << "# filename: "  << filename << endl;

    string offset_str(argv[2]);
    cout << "# offset_str: "  << offset_str << endl;
    unsigned long long offset = strtoul(offset_str.c_str(),&t_ptr,10);
    cout << "# offset: "  << offset << endl;

    string num_elems_str(argv[3]);
    cout << "# num_elems_str: "  << num_elems_str << endl;
    unsigned long long t_nelems = strtoul(num_elems_str.c_str(),&t_ptr,10);
    cout << "# t_nelems: "  << t_nelems << endl;

    // Not checking errors, the wrong offset and size, errors will occur afterwards.

    FILE *fp;
    
    if((fp= fopen(filename.c_str(),"rb")) == nullptr) {
        cerr << "ERROR: Can't open file: '"  << argv[1];
        exit(1);
    }
    vector<char>databuf(t_nelems);
    fseek(fp,offset,SEEK_SET);
    
    auto numofread = fread(databuf.data(),t_nelems,1,fp);
    cout << "# numofread: "  << numofread << endl;


    cout << "# DATA: " << endl;
    for (unsigned long long i = 0; i < t_nelems; i++) {
        cout << "'" << std::hex << databuf[i] << std::dec << "', ";
    }
    cout <<endl;
      
    
    fclose(fp);
    return 0;
}

