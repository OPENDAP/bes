
# Ripped from the Interwebs. jhrg 11/30/21
# https://www.kite.com/python/answers/how-to-update-and-replace-text-in-a-file-in-python

import re
#import argparse


# def main():
#     parser = argparse.ArgumentParser()
#     parser.add_argument("-f", "--file", help="data file to process", default='EM40647_Data.csv')
#     parser.add_argument("-i", "--interval", help="sample interval (0-60 seconds)", type=int, default=60)
#     parser.add_argument("-o", "--offset", help="time offset in seconds", type=int, default=354049200)
#     args = parser.parse_args()


filename = "AsciiArray.cc"
pattern = ["debug", "util"]
with open(filename, 'r+') as f:
    text = f.read()
    for p in pattern:
        text = re.sub(r'#include [<"](' + p + r')\.h[>"]', r'#include <libdap/\1.h>', text)
    f.seek(0)
    f.write(text)
    f.truncate()
