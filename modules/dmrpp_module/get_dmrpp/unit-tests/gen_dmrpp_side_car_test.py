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

class TestSample(unittest.TestCase):

    def test_gen_dmrpp_side_car(self):
        
        print("Testing grid_2_2d_ps.hdf")
        subprocess.run(["./gen_dmrpp_side_car", "-i", "grid_2_2d_ps.hdf","-H"])
        if not os.environ.get('PRESERVE_TEST_ASSETS'):
            self.addCleanup(os.remove, "grid_2_2d_ps.hdf.dmrpp")
            self.addCleanup(os.remove, "grid_2_2d_ps.hdf_mvs.h5")
        
        #result = filecmp.cmp("grid_2_2d_ps.hdf.dmrpp","grid_2_2d_ps.hdf.dmrpp.baseline")
        #self.assertEqual(result ,True )
        
        # Since we also add the dmrpp metadata generation information for the HDF4 files,
        # we need to ignore that information when doing comparison.
        with open('grid_2_2d_ps.hdf.dmrpp') as f:
            dmrpp_lines_after_79 = f.readlines()[79:]
        with open('grid_2_2d_ps.hdf.dmrpp.baseline') as f1:
            baseline_lines_after_79 = f1.readlines()[79:]
        self.assertEqual(dmrpp_lines_after_79 ,baseline_lines_after_79)
   

    def test_gen_dmrpp_side_car2(self):
        
        print("Testing grid_2_2d_sin.h5")
        subprocess.run(["./gen_dmrpp_side_car", "-i", "grid_2_2d_sin.h5"])
        if not os.environ.get('PRESERVE_TEST_ASSETS'):
            self.addCleanup(os.remove, "grid_2_2d_sin.h5.dmrpp")
            self.addCleanup(os.remove, "grid_2_2d_sin.h5_mvs.h5")
        with open('grid_2_2d_sin.h5.dmrpp') as f:
            dmrpp_lines_after_19 = f.readlines()[19:]
        with open('grid_2_2d_sin.h5.dmrpp.baseline') as f1:
            baseline_lines_after_19 = f1.readlines()[19:]
        self.assertEqual(dmrpp_lines_after_19 ,baseline_lines_after_19)

    def test_gen_dmrpp_side_car_h4_nsc(self):
        
        print("Testing grid_1_2d.hdf :no side car file")
        subprocess.run(["./gen_dmrpp_side_car", "-i", "grid_1_2d.hdf","-H","-D"])
        if not os.environ.get('PRESERVE_TEST_ASSETS'):
            self.addCleanup(os.remove, "grid_1_2d.hdf.dmrpp")
        
        # Since we also add the dmrpp metadata generation informatio for the HDF4 files,
        # we need to ignore those information when doing comparision.
        with open('grid_1_2d.hdf.dmrpp') as f:
            dmrpp_lines_after_54 = f.readlines()[54:]
        with open('grid_1_2d.hdf.dmrpp.baseline') as f1:
            baseline_lines_after_54 = f1.readlines()[54:]
        self.assertEqual(dmrpp_lines_after_54 ,baseline_lines_after_54)

    def test_gen_dmrpp_side_car_h5_cf(self):
        
        print("Testing grid_1_2d.h5 :CF option")
        subprocess.run(["./gen_dmrpp_side_car", "-i", "grid_1_2d.h5","-c"])
        if not os.environ.get('PRESERVE_TEST_ASSETS'):
            self.addCleanup(os.remove, "grid_1_2d.h5.dmrpp")
        
        
        # Since we also add the dmrpp metadata generation informatio for the HDF4 files,
        # we need to ignore those information when doing comparision.
        # We also ignore the first two lines so as to avoid false failure due to different
        # dmrpp versions
        with open('grid_1_2d.h5.dmrpp') as f:
            dmrpp_minus_18_lines = f.readlines()[2:-18]
        with open('grid_1_2d.h5.dmrpp.baseline') as f1:
            baseline_minus_18_lines = f1.readlines()[2:-18]
        self.assertEqual(dmrpp_minus_18_lines ,baseline_minus_18_lines)
 
if __name__ == '__main__':
    unittest.main()

