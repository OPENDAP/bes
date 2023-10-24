#!/usr/bin/env python3
#
# These files from besstandalone cannot be treated as text files
# The different responses are written to 'fn'_response_n where n = 1, ..., N
#
# This is complete, but I have not written any autotests that use it.
# Do that next.

import sys

if len(sys.argv) != 2:
    print("Expected an argument.")
    exit(1)

fn = sys.argv[1]    # 'stuff.dap'
print("Reading {}".format(fn))

# open the file and read it all into 'd'
with open(fn, "rb") as f:
    d = f.read()
    f.close()

# build a list of places where the Dataset element is found.
# end the list with a -1
found = [d.find(b"<Dataset")]
while found[-1] != -1:
    found.append(d.find(b"<Dataset", found[-1] + 1))

# found = found[0:-2]     # shave off the last element which is -1
# before '<Dataset' there are 'preamble' chars
preamble = 48

# extract responses as substrings
responses = []
while len(found) > 1:
    start = found[0] - preamble
    if found[1] == -1:
        end = -1
    else:
        end = found[1] - preamble
    responses.append(d[start:end])
    found = found[1:]

print("Found {} Responses".format(len(responses)))

n = 1
fn += ".response_"
for r in responses:
    rn = fn + str(n)
    with open(rn, "wb") as f:
        f.write(r)
        f.close()
    n += 1

exit(0)
