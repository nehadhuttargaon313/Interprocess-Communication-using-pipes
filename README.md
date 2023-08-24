# Interprocess-Communication-using-pipes
In this question, you will use ordinary pipes to implement an inter-process
communication scheme for message passing between processes. Assume that there are
two directories, d1 and d2, and there are different files in each one of them. Also, each
file contains a short-length string of characters. You have to use a parent process that
forks two children processes and have each child process check one of the directories.
* Assume that child 1 (child 2) is responsible to check the directory d1 (directory
d2, respectively). This child process has to create a list of names of the files in the
directory and their contents.
* After creating the list, each child process will send its list to the other child
process using a pipe.
* If one of the child processes encounters an error while reading the files in their
directory, it should immediately convey the message to other child with its
current updates using message passing scheme.
* Upon receiving the list, child 2 (child 1) will create the files listed by child 1
(child 2) in directory d2 (directory d1, respectively) and fill the files with their
initial contents.
After these steps, the directories d1 and d2 should be identical.
