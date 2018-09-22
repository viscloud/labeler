
import argparse

import h5py
import numpy as np


def main():
    parser = argparse.ArgumentParser(
        description=("Converts labels HDF5 file into a text file where each "
                     "line is frame number of a positive example."))
    parser.add_argument(
        "--in-file", type=str, help="The labels HDF5 file to parse.",
        required=True)
    parser.add_argument(
        "--out-file", type=str, help="The text file to create.", required=True)
    args = parser.parse_args()

    in_filepath = args.in_file
    out_filepath = args.out_file

    in_file = h5py.File(in_filepath, "r")["labels"][:]

    print("Processing {} frames".format(len(in_file)))
    with open(out_filepath, "w") as out_file:
        for frame_id in np.where(in_file == 1)[0]:
            out_file.write("{}\n".format(frame_id))
    return 0


if __name__ == "__main__":
    main()
