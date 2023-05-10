
"""
This server/service will return various kinds of responses based on the
input path. The intent is to mimic the S3 behavior of returning 500 responses
in a pseudo-random way. This server, however, is completely predictable.
It can return a series of 500 responses, for example, and then a 200.

jhrg 5/8/23
"""

from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from threading import Thread

hostName = "localhost"
serverPort = 8000


class ErrorServer(BaseHTTPRequestHandler):
    """
    Process requests where path is of the form /<code>/<N>/<text-path>.
    For that path, the service will return <code> for <N> consecutive
    requests and then return a 200 response. The body of the 200 will
    be <text-path>.
    """

    """ 
    Track how many times a request has been made by indexing them it <text-path>. 
    Structure: {<text_path>: [code, times so far]}
    """
    requests = {}

    def process_request(self, path):
        """
        Process a request. Send a response or an error (422) if the request was malformed.
        Note that the caller does the actual send operation. This method returns a list
        of the return code and a bit of text for the response.
        :param path: The URL path & query string
        :return: Return a list with the response code and some text. The codes will be either
        the requested code (500, 404, etc.) or 200.
        """
        if path[0] == '/':
            path = path[1:]
        parts = path.split('/')
        if len(parts) < 3:
            return [422, f'The service expected 3 parts to the <path>, but found {len(parts)} - path: {path}, parts: {parts}']
        code = int(parts[0])
        times = int(parts[1])
        text_path = parts[2]

        if type(code) is not int:
            return [422, f'The service expected the "code" to be an integer but got <{code}> - {path}']
        if type(times) is not int:
            return [422, f'The service expected the "times" to be an integer but got <{times}> - {path}']

        if text_path not in self.requests:
            # add info for text_path, count from 0 to N-1
            self.requests[text_path] = [code, 1]
            return [code, f'Response {1} for code {code}.']
        else:
            # update info for text_path
            info = self.requests[text_path]
            # n is stored in info[1]
            if info[1] < times:
                info[1] += 1
                self.requests[text_path] = info
                return [info[0], f'Response {info[1]} for code {info[0]}.']
            else:
                # remove the entry so the same <path> can be used again
                self.requests.pop(text_path)
                return [200, text_path]

    def do_GET(self):
        """
        Process the request. If the request starts with the word 'exit' then this method will
        throw an exception causing the server to stop.
        :return: Does not return a value.
        """
        response_items = self.process_request(self.path)
        if self.path.startswith("/exit"):
            self.send_response(200)
            self.send_header("Content-type", "text/html")
            self.end_headers()
            self.wfile.write(bytes("<html><head><title>HTTP Test Service exiting</title></head>", "utf-8"))
            self.wfile.write(bytes(f"<p>Request: {self.path}</p>", "utf-8"))
            self.wfile.write(bytes("<body>", "utf-8"))
            self.wfile.write(bytes(f"<p>The HTTP response service <i>has left the building</i>; invoked with {self.path}.</p>",
                                   "utf-8"))
            self.wfile.write(bytes("</body></html>\n", "utf-8"))
            t = Thread(target=lambda server: server.shutdown(), args=(webServer,))
            t.run()
        elif response_items[0] == 200:
            self.send_response(200)
            self.send_header("Content-type", "text/html")
            self.end_headers()
            self.wfile.write(bytes("<html><head><title>HTTP Test Service</title></head>", "utf-8"))
            self.wfile.write(bytes(f"<p>Request: {self.path}</p>", "utf-8"))
            self.wfile.write(bytes("<body>", "utf-8"))
            self.wfile.write(bytes(f"<p>This 200 response is from the test HTTP service invoked with {self.path}.</p>",
                                   "utf-8"))
            self.wfile.write(bytes("</body></html>\n", "utf-8"))
        else:
            self.send_error(response_items[0], None, response_items[1])


if __name__ == "__main__":
    webServer = ThreadingHTTPServer((hostName, serverPort), ErrorServer)
    print(f"Server started http://{hostName}:{serverPort}")

    try:
        webServer.serve_forever()
    except KeyboardInterrupt:
        pass

    webServer.server_close()
    print("Server stopped.")
