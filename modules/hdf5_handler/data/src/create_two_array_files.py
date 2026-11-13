#!/usr/bin/env python3
"""
create_two_array_files.py

Creates two HDF5 files, each holding two 2-D datasets stored as a single
chunk and compressed with deflate (gzip) level 1:

  file 1: two_arrays_16x16.h5
      uint16_data  : uint16,  64 x 16, random integers in [0, 3]
      float_data   : float32, 16 x 16, 0.5, 1.0, 1.5, ... 128.0

  file 2: two_arrays_32x16.h5
      uint16_data  : uint16,  64 x 16, random integers in [0, 3]
      float_data   : float32, 32 x 16, 0.5, 1.0, 1.5, ... 256.0

Usage:
    python3 create_two_array_files.py [--outdir DIR] [--seed N]
"""

import argparse
import os

import h5py
import numpy as np


def make_file(path, float_shape, rng):
    uint_shape = (64, 16)
    uint_data = rng.integers(0, 4, size=uint_shape, dtype=np.uint16)  # 0..3 inclusive

    n = float_shape[0] * float_shape[1]
    float_data = (0.5 * np.arange(1, n + 1, dtype=np.float32)).reshape(float_shape)

    with h5py.File(path, "w") as f:
        f.create_dataset(
            "uint16_data", data=uint_data, dtype="<u2",
            chunks=uint_shape,            # one chunk = whole array
            compression="gzip", compression_opts=1,
        )
        f.create_dataset(
            "float_data", data=float_data, dtype="<f4",
            chunks=float_shape,           # one chunk = whole array
            compression="gzip", compression_opts=1,
        )
    print(f"Wrote {path}")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--outdir", default=".")
    parser.add_argument("--seed", type=int, default=None, help="random seed (default: none)")
    args = parser.parse_args()

    rng = np.random.default_rng(args.seed)
    os.makedirs(args.outdir, exist_ok=True)
    make_file(os.path.join(args.outdir, "two_arrays_16x16.h5"), (16, 16), rng)
    make_file(os.path.join(args.outdir, "two_arrays_32x16.h5"), (32, 16), rng)


if __name__ == "__main__":
    main()
