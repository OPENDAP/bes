# s3-proxy

## How to use
A server to emulate S3 errant behavior

To start the server 'python3 main.py'

To emulate the S3 '500 message' behavior, send it http://localhost:8000/500/N/name.
For example, to emulate a 500 error on the 3rd request, send it http://localhost:8000/500/3/name
and the service will return 3 HTTP 500 errors before returning a 200 response. The 200 response
will contain 183 characters.

To stop the server, send it http://localhost:8080/exit.

## Where to find
github.com/OPENDAP/s3-proxy
