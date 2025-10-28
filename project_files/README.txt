Short description:

I've completed the project, the only thing that doesn't match the assignment pdf is the library leader
book array which i didn't implement (couldn't find a reason to anyways and the TAs was ok with it). I've included some of my random notes
below that could help provide insight about my project! I'm providing a shell script and a Makefile. 
I've only tested 2 test files. I'm running with 3 hosts, you can see them in the host_file.


HOW To run my project:
make
./run.sh 3 loaners_13_libs_9_np_23.txt



My Notes:

banana runs mpiexec version 3.4.1

There will be defines for the MPI TAGS in my "my_funcs.h" since everybody is going to be using it. 
"my_funcs.h" includes useful functions such as a string splitter.

You can turn off debug prints by commencting out the "DEBUG_ENABLED" in my my_funcs.h file

modified 'ACK_TB <b_id>' to include the cost of the book.

DONATE_BOOKS is from coordinator, DONATE_BOOK is from client to client leader

i've taken care of the edge cases in takeBook and donateBook (e.g. what happens if the coordinator send a message straight to the leader?)

i've take care of the edge cases in library-side check_num

i've included a "SHUTDOWN" scenario to terminate the processes

IMPORTANT: client get_most_pop_book gets stuck for the testfiles 2 & 3 and idk why. i just the for leader loop and instead of waiting
for specific messages to come (i was waiting in order e.g. rank 18, rank 19, ...) to MPI_ANY_SOURCE but it still took too long
and the messages timed out. Still works for testfile 1 after the change though.
