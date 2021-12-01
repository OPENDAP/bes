
# Ripped from the Interwebs. jhrg 11/30/21
# https://www.kite.com/python/answers/how-to-update-and-replace-text-in-a-file-in-python

import re

filename = "AsciiArray.cc"
pattern = "debug"
with open(filename, 'r+') as f:
    text = f.read()
    text = re.sub(r'#include [<"](' + pattern + r')\.h[>"]', r'#include <libdap/\1.h>', text)
    f.seek(0)
    f.write(text)
    f.truncate()
