# My-shell
Designed and implemented a custom shell in C that supports both interactive and batch modes, with command parsing and input-processing loop. Helped me learn a lot about C and systems programming. 

#Usage:

mysht.c is the main program that implements the shell. All other files are for testing purposes. To run, first call make then ./mysht to run. If you put no other command line arguments the shell will run in interactive mode, where you can input different command line arguments into the shell. If you want batch mode, run the shell with a file or scripts (i put some in the myshell directory), and the shell will run that file and any command line argumnents in it.

#Testing:

Included a bunch of different test cases (directory traversal, wildcards, conditionals, etc..), on top of some test scripts that just have a bunch of command line argumnents in order for integration testing. 

