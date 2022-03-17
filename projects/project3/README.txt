PROJECT: InstaQuack
AUTHOR: Lukas Hanson
DATE: 2020/12/05
CLASS: CIS 415

USAGE:
Using the makefile generates the file "server.exe" which requires an initial
input file of instructions. It can be run as follows:

./server.exe [input file]

Said input file needs to include valid instruction files for the publisher and
subscriber threads.

The number of proxy threads, the maximum number of queues, and the maximum
number of times a publisher or subscriber thread will attempt to put/get
if the attempt is unsuccessful are all initialized with global variables at the
top of the file. The default settings are as follows:

NUMPROXIES = 8
MAXQUEUES = 5
NUMATTEMPTS = 5

Do NOT change the global flag "START," as that is used to indicate whether a
thread needs to wait for a broadcast from the main thread.

If there are any questions or general confusion about my code, please email me
at lhanson7@uoregon.edu and I will reply as soon as I can.
