#!/usr/bin/env python3
"""
Test gen_dmrpp_side_car script.
python3 -m unittest gen_dmrpp_side_car_test

"""
import unittest
import subprocess
import filecmp
import os

class TestSample(unittest.TestCase):

    def test_gen_dmrpp_side_car(self):
        
        print("Testing grid_2_2d_ps.hdf")
        subprocess.run(["./gen_dmrpp_side_car", "-i", "grid_2_2d_ps.hdf","-H"])
        self.addCleanup(os.remove, "grid_2_2d_ps.hdf.dmrpp")
        self.addCleanup(os.remove, "grid_2_2d_ps.hdf_mvs.h5")
        result = filecmp.cmp("grid_2_2d_ps.hdf.dmrpp","grid_2_2d_ps.hdf.dmrpp.baseline")
        self.assertEqual(result ,True )
   

    def test_gen_dmrpp_side_car2(self):
        
        print("Testing grid_2_2d_sin.h5")
        subprocess.run(["./gen_dmrpp_side_car", "-i", "grid_2_2d_sin.h5"])
        self.addCleanup(os.remove, "grid_2_2d_sin.h5.dmrpp")
        self.addCleanup(os.remove, "grid_2_2d_sin.h5_mvs.h5")
        with open('grid_2_2d_sin.h5.dmrpp') as f:
            dmrpp_lines_after_19 = f.readlines()[19:]
        with open('grid_2_2d_sin.h5.dmrpp.baseline') as f1:
            baseline_lines_after_19 = f1.readlines()[19:]
        self.assertEqual(dmrpp_lines_after_19 ,baseline_lines_after_19)


if __name__ == '__main__':
    unittest.main()

