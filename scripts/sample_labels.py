
import argparse

import h5py
import numpy as np


def main():
    parser = argparse.ArgumentParser(
        description=("Samples a labels HDF5 file, creating a new HDF5 file "
                     "containing one in every N frames."))
    parser.add_argument(
        "--in-file", type=str, help="The labels hdf5 file to sample.",
        required=True)
    parser.add_argument(
        "--every-n", type=int, help="Select one in N frames.", required=True)
    parser.add_argument(
        "--out-file", type=str, help="The output file to create.",
        required=True)
    args = parser.parse_args()
    in_filepath = args.in_file
    n = args.every_n
    out_filepath = args.out_file

    in_file = h5py.File(in_filepath, "r")
    out_file = h5py.File(out_filepath, "w")

    # All of the idxs from in_file that will be copied to out_file.
    idxs = range(0, len(in_file["labels"]), n)

    out_file.create_dataset("labels", [len(idxs)], dtype=np.int32)
    for out_idx, in_idx in enumerate(idxs):
        assert in_idx % n == 0
        out_file["labels"][out_idx] = in_file["labels"][in_idx]

    out_file.close()


if __name__ == "__main__":
    main()
