HttpDns Client ver 1.
=====================

Antti Nilakari 66648T `<antti.nilakari@aalto.fi>`


Overview
========

The HttpDnsClient has the following module & header structure:

        httpdnsc.cc                // Main executable
        +- httpgetput.hh           // HTTP GET and PUT functions
           +- addrinfo.hh          // Wrapper for addrinfo
           +- file.hh              // Wrapper for int file sockets
           |  +- filedescriptor.cc // Wrapper for readadable / writeable FDs.
           +- tcpclientsocket.hh   // Functionality for TCP client sockets
              +- tcpsocket.hh      // Funcionality for TCP sockets in general
                 +- socket.hh      // Functionality for sockets in general

The class hierarchy for Sockets is as follows and allows generic functionality to be implemented on int-like file descriptor based sockets:

        File
        +- FileDescriptor
        +- Socket
           +- TCPSocket
           +- TCPClientSocket


Key Functionality
=================

The network logic is very straightforward: there is no need for parallel activity. Reading and writing is very straightforward looping as long as the sockets give data when reading or the to-be-sent buffer is not empty when writing. In case of error, an exception in C++ is thrown and the operation is aborted.


Build and Installation Instructions
===================================

        $ cd src
        $ make clean all


User Instructions
=================

The action must be given as the first parameter when calling the client. The URL must be canonical and have the http: scheme part.

        $ ./httpdnsc GET http://nwprog1.netlab.hut.fi:3000/nwhttplog log.txt
        $ ./httpdnsc PUT log.txt http://nwprog1.netlab.hut.fi:3000/new-file.txt


Testing and known limitations
=============================

The client performs well only if the server accepts the result, i.e. a 200 OK is returned. In other cases, it will hang and wait for timeout (which takes a few seconds). The program was tested against the nwprog1.netlab.hut.fi:3000 server implementation.


Answers to specific questions
=============================

1. The implementation is IPv6 compatible. The Socket implementation is protocol agnostic and accepts everything it is given, but Addrinfo uses IPv6 and V4MAPPED and thus Socket uses it too:

        // Snippet from the code
        int flags = AI_V4MAPPED | AI_ALL,
        int family = AF_INET6

   It has not been specifically tested, but this will be done in the final version.

2. If connecting fails, an exception is thrown and the operation aborted. The Addrinfo list is looped through to ensure that all addresses are tried.


