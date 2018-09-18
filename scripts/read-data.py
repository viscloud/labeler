#!/usr/bin/python

import os
import sys

partial_start_str = "Partial Start";
partial_end_str = "Partial Finish";
full_start_str = "Full Start";
full_end_str = "Full Finish";

state_no_person = "No Person";
state_unknown = "Unknown";
state_person = "Person";

def read_file(filename, data_points):
    print(filename);
    f = open(filename);
    for line in f:
        split_line = line.rstrip().split(":");
        data_points[int(split_line[1])] = split_line[0];

def main(data_dir):
    data_points = dict()
    # After running main, full_labels contains a mapping from every frame to a label
    full_labels = dict()

    prev_state = state_no_person;
    state = state_no_person;
    label_files = os.listdir(data_dir);
    for file in label_files:
        if(file != "readme"):
            read_file(data_dir + '/' + file, data_points);
    for idx in range(0, sorted(data_points.keys())[-1]):
        if idx in data_points:
            if(data_points[idx] == partial_start_str):
                prev_state = state_unknown;
                state = state_unknown;
            if(data_points[idx] == partial_end_str):
                prev_state = state_no_person;
                state = state_no_person;
            if(data_points[idx] == full_start_str):
                state = state_person;
            if(data_points[idx] == full_end_str):
                state = prev_state;
        full_labels[idx] = state;
    for key in sorted(full_labels.keys()):
        val = full_labels[key];

    return data_points, full_labels

if __name__ == '__main__':
    data_dir = sys.argv[1]
    main(data_dir)
