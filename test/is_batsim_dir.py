#!/usr/bin/env python

"""Checks whether a directory looks like Batsim root directory."""
import argparse
import os
import sys


def looks_like_batsim_dir(dir):
    """Return whether the directory looks like Batsim root directory."""
    if os.path.isdir(dir):
        subdirs = [dir + '/' + x for x in ['cmake', 'platforms',
                                           'schedulers', 'src', 'tools']]
        for subdir in subdirs:
            if not os.path.isdir(subdir):
                print("Directory '{dir}' does not look like the "
                      "Batsim root one : the '{subdir}' subdirectory does not "
                      "exist.".format(dir=dir, subdir=subdir))
                return False
        print("Directory '{}' looks like the Batsim root one".format(dir))
        return True
    else:
        print("Directory '{dir}' does not exist!", dir)
        return False


def main():
    """Entry point. Parses input arguments then calls looks_like_batsim_dir."""
    parser = argparse.ArgumentParser(description='Checks whether a directory '
                                     'seems to be the Batsim root one')

    parser.add_argument('dir',
                        type=str,
                        help='The directory to check')

    args = parser.parse_args()

    if looks_like_batsim_dir(args.dir):
        sys.exit(0)
    else:
        sys.exit(1)


if __name__ == "__main__":
    main()
