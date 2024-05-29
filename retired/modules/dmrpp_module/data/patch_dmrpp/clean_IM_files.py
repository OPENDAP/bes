import argparse as ap
import os
import subprocess

parser = ap.ArgumentParser(description='Patch dmrpp files to ensure the values of all variables can be located.')
parser.add_argument("-v","--verbosity",type=int, choices=[1,2],
                    help='when value is 1, print more output messages. value is 2, intermediate files are kept.')
args = parser.parse_args()

#remove intermediate files.
if(args.verbosity!=2):
    files=[f for f in os.listdir('.') if os.path.isfile(f)]
    for f in files:
        if f.endswith(".missvar"):
            subprocess.run(["rm", "-rf", f])
        if f.endswith("_missing.bescmd"):
            subprocess.run(["rm", "-rf", f])
        if f.endswith("_missing.h5.dmr"):
            subprocess.run(["rm", "-rf", f])
        if f.endswith("_missing.h5.dmrpp"):
            subprocess.run(["rm", "-rf", f])
        if f.endswith("_missing.h5.dmr.bescmd"):
            subprocess.run(["rm", "-rf", f])
    subprocess.run(["rm", "-rf", "bes.conf"])
    subprocess.run(["rm", "-rf", "bes.log"])
    subprocess.run(["rm", "-rf", "mds_ledger.txt"])



