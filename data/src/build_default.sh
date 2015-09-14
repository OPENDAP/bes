#!/bin/sh
rm *.h5 *.o

h5cc -Wall d_compound.c -o d_compound
./d_compound

h5cc -Wall h5_comp_scalar.c -o h5_comp_scalar
./h5_comp_scalar

h5cc -Wall h5_comp_complex.c -o h5_comp_complex
./h5_comp_complex


h5cc -Wall d_group.c -o d_group
./d_group

h5cc -Wall d_array.c -o d_array
./d_array

h5cc -Wall d_objref.c -o d_objref
./d_objref

h5cc -Wall d_regref.c -o d_regref
./d_regref

h5cc -Wall d_links.c -o d_links
./d_links

h5cc -Wall d_int.c -o d_int
./d_int

h5cc -Wall d_link_soft.c -o d_link_soft
./d_link_soft

h5cc -Wall d_link_hard.c -o d_link_hard
./d_link_hard


