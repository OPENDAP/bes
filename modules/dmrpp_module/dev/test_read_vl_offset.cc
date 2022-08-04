#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <iostream>

using namespace std;

int main(int argc, char *argv[]) {

    if (argc != 4) {
        printf("Please provide the HDF5 file name and the HDF5 real data offset and size-in-bytes as the following:\n");
        printf(" ./h5_vl_read h5_file_name offset size \n");
        return 0;
    }


    // Not checking errors, the wrong offset and size, errors will occur afterwards.
    char *t_ptr;
    unsigned long long offset = strtoul(argv[2],&t_ptr,10);
    unsigned long long t_nelems = strtoul(argv[3],&t_ptr,10);
 
    int numofread;
    FILE *fp;
    
    if((fp= fopen(argv[1],"rb"))==NULL) {
    printf("Error, Cann't open file.");
    exit(1);
    }
    vector<char>databuf(t_nelems);
    fseek(fp,offset,SEEK_SET);
    
    numofread = fread(&databuf[0],t_nelems,1,fp);
    for (unsigned i = 0; i <t_nelems;i++)
        cout <<databuf[i]<<" ";
    cout <<endl;
      
    
    fclose(fp);
    return 0;
}

