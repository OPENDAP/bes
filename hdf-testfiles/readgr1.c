/* Test to demonstrate another HDF bug:  subsetting in GRI's with
 * multiple components is completely broken.
 */

#include <stdio.h>
#include "hdf.h"
#include "mfhdf.h"

#define FILENAME "testgr1.hdf"
#define NAME "GR_DFNT_UINT32"

uint32 array_data[3][3][3];

main() {
	int32 file_id, gr_id, gr_index, ri_id, status, i, j, k;
	int32 start[2], stride[2], edges[2];
	// char gr_name[64]; Unused jhrg 3/16/11
	int32 ncomp, data_type, interlace_mode, dimsizes[2], num_attrs;

	file_id = Hopen(FILENAME, DFACC_RDONLY, 0);
	gr_id = GRstart(file_id);
	gr_index = GRnametoindex(gr_id, NAME);
	ri_id = GRselect(gr_id, gr_index);
	GRgetiminfo(ri_id, gr_name, &ncomp, &data_type, &interlace_mode,
			dimsizes, &num_attrs);
	printf("name: %s ncomp=%d data_type=%d int_mode=%d dim[0]=%d dim[1]=%d\n",
		gr_name, ncomp, data_type, interlace_mode, dimsizes[0], dimsizes[1]);

	for(k=0; k<3; k++)
	  for(i=0; i<3; i++)
	    for(j=0; j<3; j++)
	      array_data[k][i][j] = 999;

	start[0] = 0;
	start[1] = 1;
	stride[0] = 1;
	stride[1] = 1;
	edges[0] = 3;
	edges[1] = 3;

	status = GRreadimage(ri_id, start, stride, edges, (VOIDP)array_data);
	if(status) {
		printf("error in GRreadimage: %d\n", status);
		return 1;
	}
	status = GRendaccess(ri_id);
	status = GRend(gr_id);
	status = Hclose(file_id);

	for(k=0; k<3; k++)
	  for(i=0; i<3; i++)
	    for(j=0; j<3; j++)
	      printf("data[%d][%d][%d] = %d\n", k, i, j, array_data[k][i][j]);
}
