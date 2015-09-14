#!/bin/sh
rm *.h5 *.o

/hdfdap/local/bin/h5cc t_space_null.c -o t_space_null
./t_space_null

/hdfdap/local/bin/h5cc t_space_zero.c -o t_space_zero
./t_space_zero

/hdfdap/local/bin/h5cc t_int_scalar.c -o t_int_scalar
./t_int_scalar

/hdfdap/local/bin/h5cc t_float.c  -o t_float 
./t_float

/hdfdap/local/bin/h5cc t_group_scalar_attrs.c  -o t_group_scalar_attrs 
./t_group_scalar_attrs

/hdfdap/local/bin/h5cc t_int.c -o t_int
./t_int

/hdfdap/local/bin/h5cc  t_link_comment.c -o t_link_comment
./t_link_comment

/hdfdap/local/bin/h5cc t_size8.c -o t_size8
./t_size8

/hdfdap/local/bin/h5cc t_string.c -o t_string
./t_string

/hdfdap/local/bin/h5cc t_string_cstr.c -o t_string_cstr
./t_string_cstr

/hdfdap/local/bin/h5cc t_unsigned_int.c -o t_unsigned_int
./t_unsigned_int

/hdfdap/local/bin/h5cc t_unsupported.c -o t_unsupported
./t_unsupported

/hdfdap/local/bin/h5cc t_vl_string.c -o t_vl_string
./t_vl_string

/hdfdap/local/bin/h5cc t_vl_string_cstr.c -o t_vl_string_cstr
./t_vl_string_cstr

/hdfdap/local/bin/h5cc t_special_char_attr.c -o t_special_char_attr  
./t_special_char_attr
