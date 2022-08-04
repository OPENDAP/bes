#include <stdio.h>
#include <stdlib.h>
#  define UINT32DECODE(p, i) {                              \
   (i)    =  (uint32_t)(*(p) & 0xff);       (p)++;                  \
   (i) |= ((uint32_t)(*(p) & 0xff) <<  8); (p)++;                  \
   (i) |= ((uint32_t)(*(p) & 0xff) << 16); (p)++;                  \
   (i) |= ((uint32_t)(*(p) & 0xff) << 24); (p)++;                  \
}

void heap_addr_decode(size_t addr_len, const uint8_t **pp/*in,out*/, uint64_t *addr_p/*out*/);

int main() {

char* temptr;
int numofread;
FILE *fp;
temptr = (char*)malloc(16);
size_t offset = 6392;
if((fp= fopen("datafile","rb"))==NULL) {
printf("Error, Cann't open file.");
exit(1);
}
fseek(fp,offset,SEEK_SET);
numofread = fread(temptr,16,1,fp);
printf("numofread %d\n",numofread);
{
    uint8_t* tp = (uint8_t *)temptr;
    uint32_t seq_len,seq_index;
    uint64_t tmp;
    // Must set seq_addr to 0. 
    uint64_t seq_addr = 0;

    // Decode sequence length
    UINT32DECODE(tp,seq_len);
    printf("seq_len is %u\n",seq_len);

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
fclose(fp);

fp = fopen("vlstr-addr","wb");
if(fwrite((void*)temptr,16,1,fp) <0) {
printf("Error writing\n");
exit(1);

}
fclose(fp);
free(temptr);
return 0;
}

void heap_addr_decode(size_t addr_len, const uint8_t **pp/*in,out*/, uint64_t *addr_p/*out*/) {

    bool        all_zero = true;    /* True if address was all zeroes */

    /* Reset value in destination */
    *addr_p = 0;

    /* Decode bytes from address */
    for(unsigned u = 0; u < addr_len; u++) {
        uint8_t        c;          /* Local decoded byte */

        /* Get decoded byte (and advance pointer) */
        c = *(*pp)++;

        /* Check for non-undefined address byte value */
        if(c != 0xff)
            all_zero = false;

        if(u < sizeof(*addr_p)) {

            uint64_t        tmp = c;    /* Local copy of address, for casting */
printf("tmp is %llx \n",tmp);
            /* Shift decoded byte to correct position */
            tmp <<= (u * 8);    /*use tmp to get casting right */

            /* Merge into already decoded bytes */
            *addr_p |= tmp;
        } /* end if */
    } /* end for */
printf("addr_p =%llx\n",addr_p);
    if(all_zero)
        *addr_p = 0;

}
