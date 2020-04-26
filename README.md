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
