# 2020-1 Operating System assignments

## PA1 : LKM Rootkit
### jerry.c
A command-line interface to get input from user what actions to do. the interface will process users input by converting username into uid and create single string to write into proc file system of mousehole LKM.

usage:
~~~
./jerry < -BlockOpen username filename | -BlockKill username | -ReleaseAll >
   -BlockOpen: Block user with given user name from opening file has given filename as substring
   -BlockKill: Make processes made by given user name never be killed by other process
   -ReleaseAll: Release the protection provided by Mousehole.
~~~

### mousehole.c
A Loadable Kernel Module supports three functions.

1. Block specific user from opening files with specific substring in their filename.
2. Make all processes made by specific user never be killed. 
3. Release currently supporting action by mousehole LKM.

## PA2 : Parallel TSP Solver 
A TSP(Travel Salesman Problem) solver by creating multiple child processes. The child processes solve given subtasks assined by main process and the length of the subtask is always be 12. When user stops the process, the solver returns the best value up to the point.

usage:
~~~
./ptsp <data file> <number of maximum child process>
~~~


## PA3 : Muti-Threaded TSP Solver 
A TSP(Travel Saleman Problem) solver by creating mutiple process. This program uses concept of producer-consumer structure with bounded buffer to assign new subtask to working threads. Since this program uses multiple threads, the main thread of the program makes intraction with user to give information of current status while the child threads are working for finding solution.

usage:
~~~
./mtsp <datafile> <initial number of solver threads>
~~~

During program execution user can give below commands to interact with the program.

~~~
stat: print the best solution up to the moment, and the number of checked routes
threads: print the information of all consumer threads including thread ID, the number of subtasks processed so far, the number of checked routes in the current subtask.
num N: change the number of consumber threads into N
~~~

## PA4 : Runtime Deadlock Detector
A dynamic linked library of deadlock detector. This detecting system consists of two different program. The first one is called "ddmon", a dynamic linked library to hook whenever the target program calls pthread_mutex_lock and pthread_mutex_unlock. This library sends information of the called mutex and the thread who calls the function to checker program called "ddchck". ddchck gets the information from ddmon and construct lock graph to determine the existence of deadlock situation. The basic algorithm concept is described in below.

Algorithm:
1. Create a node nx when a thread acquires lock X.
2. Create an edge (nx, ny) when a thread acquires lock Y while holding lock X.
3. Remove all edges connected to nx when a thread releases X. Remove nx when a thread releases X and no other threads had acquired X
4. Report deadlcok when the graph has any cycle

Usage:
~~~
Build
> make clean
> make

Execute ddchck:
> ./ddchck <Executable of target program>

Execute Target Program with ddmon:
> LD_PRELOAD="./ddmon.so" <Executable of target program>
~~~
