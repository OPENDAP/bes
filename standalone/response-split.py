#!/usr/bin/env python3
#
# This script is used to split the response file into multiple files

def process_input(f, args):
    count = 1
    output = open(f'{args.output}_{count}', 'wb')

    line = f.readline()
    while line != b'':
        if line == b'Next-Response:\n':
            # close the current file and open a new one
            output.close()
            count += 1
            output = open(f'{args.output}_{count}', 'wb')
        else:
            output.write(line)
        line = f.readline()

    output.close()


def main():
    import argparse
    parser = argparse.ArgumentParser(description="Read from a besstandalone 'response file' and separate different "
                                                 "responses using the 'Next-Response:' delimiter. Each response is "
                                                 "stored in a separate file so that getdap/4 can be used to parse "
                                                 "the response for testing.")
    parser.add_argument("-v", "--verbose", help="increase output verbosity", action="store_true")

    parser.add_argument("-i", "--input", help="The name of the input file. Read from stdin if not specified.")
    parser.add_argument("-o", "--output", help="The base name of the output file, defaults to the input file name.")

    args = parser.parse_args()

    if args.output is None:
        if args.input is None:
            args.output = 'response'
        else:
            args.output = args.input

    try:
        if args.input is None:
            import sys
            # Trick: Use sys.stdin.buffer to read binary data from stdin
            # This is needed because the responses might be dods or dap responses.
            process_input(sys.stdin.buffer, args)
        else:
            with open(args.input, 'rb') as f:
                process_input(f, args)

    except Exception as e:
        print(e)


if __name__ == "__main__":
    main()
