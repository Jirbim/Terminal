# Description

This code executes almost all Linux terminal console commands such as ls, wc, etc., including using a command pipeline (ls -l | wc). 

This is implemented through system libraries such as unistd.h, wait.h, fcntl.h. 

In this program, I practiced using functions such as fork() and, in general, with the concept of a channel and processes in Unix.

***Note:*** The program does not use the system() function (passes the command to the operating system command processor in the string pointed to by the str parameter and returns the command exit status)

# Dependencies

Any environment capable of executing Linux terminal commands is required. 

This can be either a Linux virtual machine (in my case, Ubuntu), or, for example, any online compiler (onlinegdb).

The presence of libraries unistd.h, wait.h, fcntl.h, stdlib.h, string.h, ctype.h, types.h, signal.h and the C language itself.

# How to use it

You can copy the contents of the Terminal.c file and paste it on the opengdb website and compile this program there.

Or, through the terminal of your OS, compile the program and run it in the same terminal.

Then, after compiling and running the program, you can enter commands for the Linux terminal.

The program should execute them as in a regular terminal.

To exit the program, type exit. The program will print "Shell is closing, bye" and terminate the program.

### Example: ###

Here I show the output of ls -l | wc command in a normal terminal and my terminal.

![](https://github.com/Jirbim/Terminal/blob/master/Ex1.gif)

The program can open directories (command cd), if the directory is entered incorrectly, the program will display "Directory not found"

When entering a non-existent command, the program will display "Command not found"

When trying to redirect program input via > (or <) and not finding a file to redirect, the program will display "File not found"

In other errors "Syntax error"
