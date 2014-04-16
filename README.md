concurrent<...object...> wrapper
=================================

Makes access to *any* object asynchronous. All asynchronous calls will be executed in the background and in FIFO order.


The code is modified from Herb Sutters example of Concurrent object wrapper.   
http://channel9.msdn.com/Shows/Going+Deep/C-and-Beyond-2012-Herb-Sutter-Concurrency-and-Parallelism


It is slightly modified to allow in-place object creation instead of copy of object. 
See blog post at: http://kjellkod.wordpress.com/2014/04/07/concurrency-concurrent-wrapper/
