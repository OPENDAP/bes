#include <stdio.h>
#include <stdlib.h>

int main() {

int* temptr;
int numofread;
FILE *fp;
int temp;
temp = -1;
temptr = &temp;
printf("temp %d\n",temp);
if((fp= fopen("datafile","r"))==NULL) {
printf("Error, Cann't open file.");
exit(1);
}
numofread = fread(temptr,sizeof(int),1,fp);
printf("numofread %d\n",numofread);
{

printf("temp %d\n",*temptr);
}
fclose(fp);
return 0;
}


