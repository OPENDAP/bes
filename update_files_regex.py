
# Ripped from the Interwebs. jhrg 11/30/21
# https://www.kite.com/python/answers/how-to-update-and-replace-text-in-a-file-in-python

import re
import argparse
from libdap_headers import headers


def update_includes(filenames, patterns, verbose):
    for filename in filenames:
        with open(filename, 'r+') as f:
            text = f.read()
            for p in patterns:
                text = re.sub(r'#include [<"](' + p + r')\.h[>"]', r'#include <libdap/\1.h>', text)
            f.seek(0)
            if verbose > 0:
                print(f"File {filename}: {text}")
            if verbose < 2:
                f.write(text)
                f.truncate()


def main():
    parser = argparse.ArgumentParser(description="Update libdap headers in #include statements so they are prefixed by libdal/")
    parser.add_argument('files',  metavar='N', nargs='+', help='a file to edit')
    parser.add_argument('--verbose', '-v', action='count', default=0, help='verbose output while hacking your files.')
    args = parser.parse_args()

    if args.verbose > 1:
        for header in headers:
            print(f"header: {header}")

        for file in args.files:
            print(f"File: {file}")

    update_includes(args.files, headers, args.verbose)


if __name__ == "__main__":
    main()
