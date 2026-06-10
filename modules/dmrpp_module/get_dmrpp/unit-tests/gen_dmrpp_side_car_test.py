#!/usr/bin/env python3
"""
Test gen_dmrpp_side_car script.
python3 -m unittest gen_dmrpp_side_car_test

To persist generated assets (e.g. for debugging) set environment
variable `PRESERVE_TEST_ASSETS` to any value:
PRESERVE_TEST_ASSETS=yes python3 -m unittest gen_dmrpp_side_car_test
"""
import unittest
import subprocess
import filecmp
import os
import gen_bescmd_conf

class TestSample(unittest.TestCase):

    @classmethod
    def setUpClass(self):
        gen_bescmd_conf.generate_bes_conf()
        
    def test_gen_dmrpp_side_car(self):
        
        print("Testing grid_2_2d_ps.hdf")
        subprocess.run(["./gen_dmrpp_side_car", "-i", "grid_2_2d_ps.hdf", "-u", "grid_2_2d_ps.hdf", "-H"])
        bescmd_name = gen_bescmd_conf.generate_bes_cmd("grid_2_2d_ps.hdf.dmrpp")
        fonc_name=bescmd_name+"_fonc.nc4"
        subprocess.run(["besstandalone", "-c", "bes.test.conf", "-i",bescmd_name , "-f", fonc_name])
        with open('grid_2_2d_ps.hdf.dmrpp_fonc.nc4.header','w') as nc_header_file:
            subprocess.run(["ncdump", "-h", fonc_name],stdout=nc_header_file)
        result = filecmp.cmp("grid_2_2d_ps.hdf.dmrpp_fonc.nc4.header","grid_2_2d_ps.hdf.dmrpp_fonc.nc4.header.baseline")
        self.assertEqual(result ,True )
        if not os.environ.get('PRESERVE_TEST_ASSETS'):
            self.addCleanup(os.remove, "grid_2_2d_ps.hdf.dmrpp")
            self.addCleanup(os.remove, "grid_2_2d_ps.hdf_mvs.h5")
            self.addCleanup(os.remove, fonc_name)
            self.addCleanup(os.remove, bescmd_name)
            self.addCleanup(os.remove, "grid_2_2d_ps.hdf.dmrpp_fonc.nc4.header")

    def test_gen_dmrpp_side_car2(self):
        
        print("Testing grid_2_2d_sin.h5")
        subprocess.run(["./gen_dmrpp_side_car", "-i", "grid_2_2d_sin.h5", "-u", "grid_2_2d_sin.h5"])
        bescmd_name = gen_bescmd_conf.generate_bes_cmd("grid_2_2d_sin.h5.dmrpp")
        fonc_name=bescmd_name+"_fonc.nc4"
        subprocess.run(["besstandalone", "-c", "bes.test.conf", "-i",bescmd_name , "-f", fonc_name])
        with open('grid_2_2d_sin.h5.dmrpp_fonc.nc4.header','w') as nc_header_file:
            subprocess.run(["ncdump", "-h", fonc_name],stdout=nc_header_file)
        result = filecmp.cmp("grid_2_2d_sin.h5.dmrpp_fonc.nc4.header","grid_2_2d_sin.h5.dmrpp_fonc.nc4.header.baseline")
        self.assertEqual(result ,True )
        if not os.environ.get('PRESERVE_TEST_ASSETS'):
            self.addCleanup(os.remove, "grid_2_2d_sin.h5.dmrpp")
            self.addCleanup(os.remove, "grid_2_2d_sin.h5_mvs.h5")
            self.addCleanup(os.remove, fonc_name)
            self.addCleanup(os.remove, bescmd_name)
            self.addCleanup(os.remove, "grid_2_2d_sin.h5.dmrpp_fonc.nc4.header")

    def test_gen_dmrpp_side_car_h4_nsc(self):
        
        print("Testing grid_1_2d.hdf :no side car file")
        subprocess.run(["./gen_dmrpp_side_car", "-i", "grid_1_2d.hdf","-H","-D","-u", "grid_1_2d.hdf"])
        bescmd_name = gen_bescmd_conf.generate_bes_cmd("grid_1_2d.hdf.dmrpp")
        fonc_name=bescmd_name+"_fonc.nc4"
        subprocess.run(["besstandalone", "-c", "bes.test.conf", "-i",bescmd_name , "-f", fonc_name])
        with open('grid_1_2d.hdf.dmrpp_fonc.nc4.data','w') as nc_data_file:
            subprocess.run(["ncdump", fonc_name],stdout=nc_data_file)
        result = filecmp.cmp("grid_1_2d.hdf.dmrpp_fonc.nc4.data","grid_1_2d.hdf.dmrpp_fonc.nc4.data.baseline")
        self.assertEqual(result ,True )
        if not os.environ.get('PRESERVE_TEST_ASSETS'):
            self.addCleanup(os.remove, "grid_1_2d.hdf.dmrpp")
            self.addCleanup(os.remove, fonc_name)
            self.addCleanup(os.remove, bescmd_name)
            self.addCleanup(os.remove, "grid_1_2d.hdf.dmrpp_fonc.nc4.data")



    def test_gen_dmrpp_side_car_h5_cf(self):
        
        print("Testing grid_1_2d.h5 :CF option")
        subprocess.run(["./gen_dmrpp_side_car", "-i", "grid_1_2d.h5","-c","-u", "grid_1_2d.h5"])
        bescmd_name = gen_bescmd_conf.generate_bes_cmd("grid_1_2d.h5.dmrpp")
        fonc_name=bescmd_name+"_fonc.nc4"
        subprocess.run(["besstandalone", "-c", "bes.test.conf", "-i",bescmd_name , "-f", fonc_name])
        with open('grid_1_2d.h5.dmrpp_fonc.nc4.data','w') as nc_data_file:
            subprocess.run(["ncdump", fonc_name],stdout=nc_data_file)
        result = filecmp.cmp("grid_1_2d.h5.dmrpp_fonc.nc4.data","grid_1_2d.h5.dmrpp_fonc.nc4.data.baseline")
        self.assertEqual(result ,True )
 
        if not os.environ.get('PRESERVE_TEST_ASSETS'):
            self.addCleanup(os.remove, "grid_1_2d.h5.dmrpp")
            self.addCleanup(os.remove, "grid_1_2d.h5_mvs.h5")
            self.addCleanup(os.remove, fonc_name)
            self.addCleanup(os.remove, bescmd_name)
            self.addCleanup(os.remove, "grid_1_2d.h5.dmrpp_fonc.nc4.data")


    @classmethod
    def tearDownClass(cls):
        if os.path.exists("bes.test.conf"):
            os.remove("bes.test.conf")


if __name__ == '__main__':
    unittest.main()

