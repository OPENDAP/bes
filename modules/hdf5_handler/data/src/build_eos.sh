#!/bin/sh
rm *.h5 *.o

/hdfdap/local/bin/h5cc -I/hdfdap/local/include grid_1_2d.c -L/hdfdap/local/lib -o grid_1_2d -lhe5_hdfeos -lGctp
./grid_1_2d

/hdfdap/local/bin/h5cc -I/hdfdap/local/include grid_1_2d_xy.c -L/hdfdap/local/lib -o grid_1_2d_xy -lhe5_hdfeos -lGctp
./grid_1_2d_xy

/hdfdap/local/bin/h5cc -I/hdfdap/local/include grid_1_3d.c -L/hdfdap/local/lib -o grid_1_3d -lhe5_hdfeos -lGctp
./grid_1_3d

/hdfdap/local/bin/h5cc -I/hdfdap/local/include grid_1_3d_3x3y.c -L/hdfdap/local/lib -o grid_1_3d_3x3y -lhe5_hdfeos -lGctp
./grid_1_3d_3x3y

/hdfdap/local/bin/h5cc -I/hdfdap/local/include grid_1_3d_xy.c -L/hdfdap/local/lib -o grid_1_3d_xy -lhe5_hdfeos -lGctp
./grid_1_3d_xy

/hdfdap/local/bin/h5cc -I/hdfdap/local/include grid_1_3d_xyz.c -L/hdfdap/local/lib -o grid_1_3d_xyz -lhe5_hdfeos -lGctp
./grid_1_3d_xyz

/hdfdap/local/bin/h5cc -I/hdfdap/local/include grid_1_3d_z.c -L/hdfdap/local/lib -o grid_1_3d_z -lhe5_hdfeos -lGctp
./grid_1_3d_z

/hdfdap/local/bin/h5cc -I/hdfdap/local/include grid_1_3d_zz.c -L/hdfdap/local/lib -o grid_1_3d_zz -lhe5_hdfeos -lGctp
./grid_1_3d_zz


/hdfdap/local/bin/h5cc -I/hdfdap/local/include grid_2_2d.c -L/hdfdap/local/lib -o grid_2_2d -lhe5_hdfeos -lGctp
./grid_2_2d

/hdfdap/local/bin/h5cc -I/hdfdap/local/include grid_2_2d_pixel.c -L/hdfdap/local/lib -o grid_2_2d_pixel -lhe5_hdfeos -lGctp
./grid_2_2d_pixel

 /hdfdap/local/bin/h5cc -I/hdfdap/local/include grid_2_2d_size.c -L/hdfdap/local/lib -o grid_2_2d_size -lhe5_hdfeos -lGctp
./grid_2_2d_size

/hdfdap/local/bin/h5cc -I/hdfdap/local/include grid_2_2d_xy.c -L/hdfdap/local/lib -o grid_2_2d_xy -lhe5_hdfeos -lGctp
./grid_2_2d_xy

/hdfdap/local/bin/h5cc -I/hdfdap/local/include grid_2_3d_size.c -L/hdfdap/local/lib -o grid_2_3d_size -lhe5_hdfeos -lGctp
./grid_2_3d_size

/hdfdap/local/bin/h5cc -I/hdfdap/local/include grid_2_3d_xyz.c -L/hdfdap/local/lib -o grid_2_3d_xyz -lhe5_hdfeos -lGctp
./grid_2_3d_xyz

/hdfdap/local/bin/h5cc -I/hdfdap/local/include grid_4_2d_origin.c -L/hdfdap/local/lib -o grid_4_2d_origin -lhe5_hdfeos -lGctp
./grid_4_2d_origin

/hdfdap/local/bin/h5cc -I/hdfdap/local/include grid_swath_za_1_2d.c -L/hdfdap/local/lib -o grid_swath_za_1_2d -lhe5_hdfeos -lGctp
./grid_swath_za_1_2d

/hdfdap/local/bin/h5cc -I/hdfdap/local/include swath_1_2d_xyz.c -L/hdfdap/local/lib -o swath_1_2d_xyz -lhe5_hdfeos -lGctp
./swath_1_2d_xyz

/hdfdap/local/bin/h5cc -I/hdfdap/local/include swath_1_3d_2x2yz.c -L/hdfdap/local/lib -o swath_1_3d_2x2yz -lhe5_hdfeos -lGctp
./swath_1_3d_2x2yz

/hdfdap/local/bin/h5cc -I/hdfdap/local/include swath_1_3d_2x2yztd.c -L/hdfdap/local/lib -o swath_1_3d_2x2yztd -lhe5_hdfeos -lGctp
./swath_1_3d_2x2yztd

/hdfdap/local/bin/h5cc -I/hdfdap/local/include swath_1_2d_xyzz.c -L/hdfdap/local/lib -o swath_1_2d_xyzz -lhe5_hdfeos -lGctp
./swath_1_2d_xyzz

/hdfdap/local/bin/h5cc -I/hdfdap/local/include swath_2_2d_xyz.c -L/hdfdap/local/lib -o swath_2_2d_xyz -lhe5_hdfeos -lGctp
./swath_2_2d_xyz

/hdfdap/local/bin/h5cc -I/hdfdap/local/include swath_2_3d_2x2yz.c -L/hdfdap/local/lib -o swath_2_3d_2x2yz -lhe5_hdfeos -lGctp
./swath_2_3d_2x2yz

/hdfdap/local/bin/h5cc -I/hdfdap/local/include za_1_2d_yz.c -L/hdfdap/local/lib -o za_1_2d_yz -lhe5_hdfeos -lGctp
./za_1_2d_yz

/hdfdap/local/bin/h5cc -I/hdfdap/local/include za_1_3d_yzt.c -L/hdfdap/local/lib -o za_1_3d_yzt -lhe5_hdfeos -lGctp
./za_1_3d_yzt

/hdfdap/local/bin/h5cc -I/hdfdap/local/include za_1_3d_yztd.c -L/hdfdap/local/lib -o za_1_3d_yztd -lhe5_hdfeos -lGctp
./za_1_3d_yztd

/hdfdap/local/bin/h5cc -I/hdfdap/local/include za_2_2d_yz.c -L/hdfdap/local/lib -o za_2_2d_yz -lhe5_hdfeos -lGctp
./za_2_2d_yz
#



