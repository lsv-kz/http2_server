# Server uses only HTTP2(h2) protocol

Written on C++ (version C++11) with using C functions.

Tested on OS: Debian, OpenBSD, FreeBSD

### Features:
* ALPN
* Methods: GET, POST, HEAD
* Partial content
* Stream flow control
* Directory indexing
* CGI
* FastCGI
* PHP-FPM
* SCGI

### Not supported:
* Dynamic Table of Header Fields
* Server push
* Stream prioritization
* Frames CONTINUATION
* Cookies
* Compress data
* Caching
* And everything that was not included in the Features

### Compiling and run:
Required libraries: OpenSSL or LibreSSL.  
cd http2_server/  
mkdir objs/  
mkdir cert/  
make clean  
make  

Edit configuration file: http2_server.conf  
Create TLS certificate (cert.pem), private key (key.pem) and place them in folder cert/  
./http2_server  
