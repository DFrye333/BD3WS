BD3WS
=====

A simple multithreaded web server written in C.

TODO:

* Ensure that calls to fopen() don't fail due to nonexistent file paths.
* Rewrite build_response_header_content() to handle content negotiation more 
intelligently.
* Allow server to serve system/web/default.html web page when public/ is empty.
* Redo threading in a more intelligent fashion. Rather than 
one-thread-per-connection, maybe extract work from connections, queue it, and 
feed the queue to a thread pool? Something to think about.
* Cross-platform compatibility! It would be nice to successfully compile this
to Windows as well.
* ...And much more!
