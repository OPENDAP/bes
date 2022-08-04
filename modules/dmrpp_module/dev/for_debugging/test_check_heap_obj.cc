#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <string>

using namespace std;
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

int main() {

char* temptr;
int numofread;
FILE *fp;
int t_nelems = 4;
int elems_size = 16;

temptr = (char*)malloc(t_nelems*elems_size);
size_t offset = 6392;
if((fp= fopen("datafile","rb"))==NULL) {
printf("Error, Cann't open file.");
exit(1);
}
fseek(fp,offset,SEEK_SET);
numofread = fread(temptr,t_nelems*elems_size,1,fp);
printf("numofread %d\n",numofread);
uint64_t seq_addr = 0;
uint32_t seq_len,seq_index;

{
    uint8_t* tp = (uint8_t *)temptr;
    uint64_t tmp;

   for(int i =0;i < t_nelems; i++) {

    // Decode sequence length
    UINT32DECODE(tp,seq_len);
    printf("seq_len is %u\n",seq_len);

    // Must set seq_addr to 0. 
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
    UINT32DECODE(tp,seq_index);
    printf("seq_index is %u\n",seq_index);
  }

  fseek(fp,seq_addr,SEEK_SET);
  
  vector<char> heap_hdr_buf;
  heap_hdr_buf.resize(16);
  fread(&heap_hdr_buf[0],16,1,fp);
  if(heap_hdr_buf[0]!='G' ||
     heap_hdr_buf[1]!='C' ||
     heap_hdr_buf[2]!='O' ||
     heap_hdr_buf[3]!='L')
     cerr<<"not valid global heap" <<endl;

  tp=(uint8_t*)&heap_hdr_buf[8];
  uint64_t coll_size;
  UINT64DECODE(tp,coll_size)
cout<<"collection size is "<<coll_size <<endl;
     
  fseek(fp,seq_addr,SEEK_SET);
  vector<char> heap_buf(coll_size);
  fread(&heap_buf[0],coll_size,1,fp);
  tp = (uint8_t *)&heap_buf[16];
     
  uint16_t obj_idx;    
  UINT16DECODE(tp,obj_idx)
cout<<"first object index is "<<obj_idx <<endl;
  uint64_t obj_size;
  tp = (uint8_t *)&heap_buf[24];
  UINT64DECODE(tp,obj_size)
cout<<"first object size is "<<obj_size <<endl;
cout <<heap_buf[32] <<endl;  
cout <<heap_buf[33] <<endl;  
cout <<heap_buf[34] <<endl;  
cout <<heap_buf[35] <<endl;  
    
  tp = (uint8_t *)&heap_buf[40];
  UINT16DECODE(tp,obj_idx)
cout<<"second object index is "<<obj_idx <<endl;
  tp = (uint8_t *)&heap_buf[48];
  UINT64DECODE(tp,obj_size)
cout<<"second object size is "<<obj_size <<endl;
cout <<heap_buf[56] <<endl;  

  tp = (uint8_t *)&heap_buf[56];
  UINT16DECODE(tp,obj_idx)
cout<<"third object index is "<<obj_idx <<endl;
  tp = (uint8_t *)&heap_buf[64];
  UINT64DECODE(tp,obj_size)
cout<<"third object size is "<<obj_size <<endl;
cout <<heap_buf[72] <<endl;  
cout <<heap_buf[73] <<endl;  
cout <<heap_buf[74] <<endl;  


}
fclose(fp);

fp = fopen("vlstr-addr","wb");
if(fwrite((void*)temptr,t_nelems*elems_size,1,fp) <0) {
printf("Error writing\n");
exit(1);

}
fclose(fp);
free(temptr);
return 0;
}


