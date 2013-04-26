Learning diary
==============

Antti Nilakari 66648T <antti.nilakari@aalto.fi>

S-38.3610 Network Programming, assignment 1

NOTE: this document has been written in Markdown syntax. If you prefer HTML output to ASCII, type this:

        markdown diary_assignment1.md > diary.html && your_favorite_web_browser diary.html

Introduction
------------

To be overly honest, I did not have too much time to follow the course lectures nor spend time studying the materials.
However, I'm somewhat familiar with sockets programming in C
(have read Beej's guide to network programming when doing the course assignment for the AUT department's C++ course).
Thus the following diary is a reflection of me reading through the course lecture slide material and thus isn't in a completely chronological order.
There's also some reflection on the discussions on the course's IRC channel, #nwprog.


The diary
=========

Lecture 1
---------

- Looks like I've already got the book by Stevens. Got it from old books sale from the University of Helsinki library at Kumpula. The book is the
second edition, though. It also seems to cover the XTI interfaces alternative to raw UNIX sockets.

- The sample code in approximately half way through the first lecture slides uses the str* functions which in my not so humble opinion result in unreadable code. As for the question "what does the code do" it translates a string like this:

        +---+---+---+---+---+---+---+---+
        | f | o | o | . | b | a | r |\0 |
        +---+---+---+---+---+---+---+---+

  to this:

        +---+---+---+---+---+---+---+---+
        | 3 | f | o | o | 3 | b | a | r |
        +---+---+---+---+---+---+---+---+

  In the latter string, the first character contains the number of bytes that follow the first character, like a length + data format.

- OSI Model and TCP models are well-understood at this point of my studies.

- The socket types TCP / SOCK_STREAM and UDP / SOCK_STREAM are also well-known at this point of my studies. It's funny how STCP, or SOCK_SEQPACKET like it's called in the UNIX socket syscall world, hasn't caught on even though everyone speaks of it.


Lecture 2
---------

- The "everything can and will fail" philosopy of UNIX interfaces seems to
be present here, too. It makes developing applications tedious without using exceptions found in high-level programming languages. I've yet to develop a natural way of handling errors. Perhaps `do { ... } while (0);` and breaking will do the trick. Previously I've tried `goto`s in the Linux kernel style, but it makes the code look even worse.

- Errno and strerror will be of use when debugging the sockets code. Perhaps it's best to pepper the code with them and see the resulting wall of text when something goes awry. At least the error printing code can be commented away when deploying the programs for real world.

- Delayed calls, however, is the Real Challenge (TM). It means that we must use multithreading or `select` to handle them.

- Endianness isn't much of a problem when handling text protocols. At least
that's the case with IRC. `htons` and `ntohs` won't be of use when dealing with HTTP, since `getaddrinfo` does most of the hard work.

- Using fixed width types in C seems to be a good idea.

- Buffer management is actually the hard part in HTTP. TCP does not preserve boundaries and HTTP/1.1 does not send multiple queries in lockstep, so manual buffer management will be tedious. IRC does not have this problem, since all lines are shorter than 512 characters and the protocol is completely asynchronous.

- Writing your own wrappers for write and read / send and recv is probably a good idea. I've already done this in the UNIX App Programming course.

- From previous experience I can tell that using shutdown on sockets is generally a good idea. Just closing them can keep the connections hanging forever.


Lecture 3
=========

- The point of the lecture seems to show how to get rid of IPv4 for once and for all. At least on application level it seems to be easy, just switch to AF_INET6 and use AI_V4MAPPED and everything just magically works.

- Getaddrinfo (and its reverse counterpart, getnameinfo) are the swiss army knife for name resolution. The linked list operation makes it easy to use and iterate over. Some discussions in the #nwprog channel indicated that its IPv6 implementation in glibc leaks memory.

- HTTP message boundary management is tricky, like I pointed out in the Lecture 2 section.


Lecture 4
=========

- Agenda: server sockets. This is something I never had to do in the C++ IRC bot assignment, so I might have to read through the material with some thought put into it.

- The basic idea seems to be that you create a single listening socket and then use it to spawn new client sockets whenever there is an incoming connection.

- The server socket has to be bound to one or more interfaces and ports before it can listen for incoming connections.

- `Select` syscall must be used to multiplex many connections. Again, writing your own wrappers for it will be beneficial and require some insight. There seems to be a library called libevent that does this, but using it is (at least I think) forbidden since it does something that we're trying to learn to bo manually in this course.

- Daemon processes are familiar already if one has taken the UNIX App Programming course. Nothing new here.


Lecture 5
=========

- Closing TCP connections will result in SIGPIPE flying around and programs
crashing unexpectedly to the command line without any textual warning. At least that's my experience of them in the IRC bot course project. Ignoring SIGPIPE seems to be a good idea.

- SIGCHLD is probably of use only in a multiprocess application.

- The usefulness of nonblocking sockets when using select is not clear to me. In the Applications and Services in Internet P course, a friend of mine just iterated over nonblocking sockets manually even though I told him it's probably not a good idea. For some reason, manual busylooping didn't end up wasting any CPU cycles, so I don't know what really happened. If I have time, I'll do some benchmarks myself.

- Stevens seems to have example code called `str_cli()` to facilitate buffer management.

- Connect should be used just with select if it must not block. Same with Accept.

- I was already familiar with different threading / multiprocess models that were presented on the slides.

- Lecture slides seemed to have references to "NetProgBox" even though our homework assignment should be "HttpDns". Is this intentional or is there some other homework assignment I haven't been told about?


Lecture 6
=========

- UDP sockets are also something new, since IRC works over TCP, not UDP.

- Connect is not needed, but in can be used to create sockets that are bound
to a single UDP remote endpoint and thus allow communicating like one would
with TCP (disregarding the fact that packets might disappear).

- Hard errors result in ICMP packets and thus normal error codes, but "soft" errors won't, which might cause problems.

- Gather write and scatter read are useful if a protocol has distinct header and payload. Gotta keep this in mind.

- Ancillary data might be useful with IPV6 features.

- Reusing addresses is useful if previous instances of program prevent a new instance from binding to a socket.

- Disabling Nagle's algorithm might be a good idea if a protocol is not too chatty.
