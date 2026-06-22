#!/usr/bin/env python3
#
# -*- mode: python; c-basic-offset:4 -*-
#
# This file is part of the Hyrax Docker Project
#
# Copyright (c) 2026 OPeNDAP, Inc.
# Author: Nathan Potter <ndp@opendap.org>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
#
# You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
#
import http.server
import socketserver
import sys
import getopt
from datetime import datetime, timezone

PORT_NUMBER = 9001

REDIRECT_URL = "http://localhost:9002"  # Replace with your destination

RETURN_FILENAME = ""
def show_config():
    print("#")
    print("###################################################")
    print(f"#    REDIRECT_URL: '{REDIRECT_URL}'")
    print(f"# RETURN_FILENAME: '{RETURN_FILENAME}'")
    print(f"#     PORT_NUMBER: {PORT_NUMBER}")
    print("#")
    print(f"# -- TCP Server will listen on port {PORT_NUMBER}")
    if not REDIRECT_URL:
        print(f"# -- No traffic will be redirected. ")
        print(f"# -- Every response will be '{RETURN_FILENAME}'")
    else:
        print(f"# -- All traffic will be redirected to \"{REDIRECT_URL}\"")
    print("###################################################")
    print("#")


class RedirectHandler(http.server.SimpleHTTPRequestHandler):
    def do_GET(self):
        msg = ""
        # 1. Log headers to the console / file
        print("# --------------------------------------------------------------")
        print(f"# Request @ {datetime.now(timezone.utc).timestamp()}")
        print("# Request Headers:")
        for key in self.headers:
            value = self.headers[key]
            print(f"#    {key}: {value}")
        print("#")
        if not REDIRECT_URL:
            # No Redirect
            self.send_response(200)
            self.send_header('Location', "NO-REDIRECT")
            self.end_headers()
            if RETURN_FILENAME:
                # Return File contents with status 200
                in_file = open(RETURN_FILENAME, "rb") # opening for [r]eading as [b]inary
                data = in_file.read() # if you only wanted to read 512 bytes, do .read(512)
                in_file.close()
                self.wfile.write(data)
                msg = f"# Returned \"{RETURN_FILENAME}\" as response body with status 200 (SUCCESS)"
            else:
                # Return empty body with status 200
                msg = f"# Returned empty response body and status 200 (SUCCESS))"
        else:
            # Send 302 Redirect
            self.send_response(302)
            self.send_header('Location', REDIRECT_URL)
            self.end_headers()
            msg = f"# Redirecting to {REDIRECT_URL}"

        print("#")
        print(msg)


def usage():
    # opts, args = getopt.getopt(argv, "vdp:r:", ["port", "log_file", "redirect_url"])
    print("")
    print("###################################################")
    print(f"# Usage: {sys.argv[0]} [OPTIONS]")
    print(f"# Options:")
    print(f"#    -r, --redirect Redirect traffix to this URL")
    print(f"#    -f, --file_out File to return to requesting client.")
    print(f"#    -p, --port  Port Number On Which To Listen.")
    print("###################################################")
    print("")


###############################################################################
# main
def main(argv):
    """main(): In which we parse arguments and call functions toread the BES log
    lines, and write the json. woot."""

    global REDIRECT_URL
    global PORT_NUMBER
    global RETURN_FILENAME

    try:
        opts, args = getopt.getopt(argv, "p:r:f:", ["port", "redirect_url", "file_out"])
    except getopt.GetoptError:
        usage()
        sys.exit(2)

    if len(args) > 0:
        print("# ERROR: Found unexpected/unrecognized command line content.", file=sys.stderr)
        usage()
        sys.exit(2)

    arg: str
    for opt, arg in opts:
        if opt in ("-h", "--help"):
            usage()
            sys.exit()

        elif opt in ("-p", "--port"):
            PORT_NUMBER = int(arg)

        elif opt in ("-r", "--redirect_url"):
            REDIRECT_URL = arg

        elif opt in ("-f", "--file_out"):
            RETURN_FILENAME = arg


    show_config()

    with socketserver.TCPServer(("", PORT_NUMBER), RedirectHandler) as httpd:
        httpd.serve_forever()


if __name__ == "__main__":
    main(sys.argv[1:])

