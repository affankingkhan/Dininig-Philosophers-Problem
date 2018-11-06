# Dininig-Philosophers-Problem
My implementation of the dining philosophers problem in C


gcc -o dinphil dinphil.c -lpthread to compile 

./dinphil to run program

T0 is a request to think from Philosopher 0.
E1 is a request to eat from Philosopher.

In general, Ti and Ei are a think request and an eat request from Philosopher i,respectively.

If the input is P, the output is the representation of states of philosophers in
turn and separated by one space, immediately followed by a '\n' at the end. 0
represents the state of thinking and 1 represents the state of eating.
For instance, entering P in the beginning of program execution would output 0 0
0 0 0\n. 

Entering ! immediately terminates the program
