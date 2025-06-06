Read Me

Kanav Desai - khd52 Sunay Hegde - srh185

Test Plan:


To make sure our shell mysh worked properly, we went through a bunch of different types of tests that hit every part of the spec. We tested 
both interactive mode and batch mode. We started with basic commands first like echo hello and it worked right away. Then we tested /bin/ls 
to make sure we could execute full paths, and that worked aswell. Next, we focused on the built-in commands. For cd, we tried going into a 
valid subdirectory and then checked with pwd—it worked fine. But when we tried to cd into a directory that didn’t exist, we realized we 
weren’t printing an error message. We updated the code to print “cd: No such file or directory” when chdir() fails. For which, we tested 
with commands like which ls, which correctly printed /bin/ls. But when we tried which cd, it failed, and at first it gave the wrong output. 
We added a check to ignore built-ins in the search and fixed that. exit and die were straightforward, and we tested that die prints its arguments and exits with failure. One small issue we had was when we were 
piping into an exit statement, it didn't run the piped command first, so we had to make a few changes for that.

For redirection, we tested echo hello > file.txt and then opened file.txt to see if “hello” was there—and it was, so that was good. We also 
did cat < file.txt, and it worked. But when we tested commands like echo hello > (missing filename), the shell 
just crashed. So we added a check to make sure there’s a token after the > or < and now it prints an error instead of breaking. Pipes were harder for us. We started with echo hello | cat, and that worked—“hello” 
came through the pipe correctly. Then we did ls | grep txt to test filtering, and that also worked. For wildcards, we tested patterns like *.c and made sure they expanded to the actual .c files in the directory.  
At first, it wasn’t matching anything unless the pattern was at the start of the file name, but we fixed that by adjusting 
how we compare prefix and suffix around the *. We also tried *.xyz in a directory with no matches and confirmed that the literal string got passed through, which is what we wanted.

Next up we looked at conditionals. We ran false or echo ok, and it worked—the echo command ran only after false failed. Then we tested true and echo ok, which also worked. We tried chaining them like true and 
false or echo recovered, and that gave us the expected result. We also made sure things like or echo something at the start of a script didn’t run at all. One bug we ran into early was letting and/or appear after 
a pipe, which isn’t allowed. We added a check to catch that and now print a syntax error when it happens. We tested a bunch of invalid syntax cases to see what would happen too. Things like echo hello >, cat < < 
file.txt, and | echo hello gave us trouble at first—some of them crashed or gave weird behavior. So we added better parsing checks to catch bad syntax and now the shell prints an error and skips to the next 
command instead of breaking.

For interactive mode, we checked for the welcome and goodbye message. All of that worked, though we had to make sure we only showed those messages if input was from a terminal.
Finally, we tested batch mode by creating several script files and piping them in. We confirmed that no prompt or welcome message showed up, and the commands ran correctly. We tested both ./mysh myscript.sh and 
cat myscript.sh | ./mysh, and both worked fine. At one point, the input was still open in child processes, which caused weird behavior, but we fixed that by closing stdin for child processes in batch mode as 
specified.To keep things organized, we wrote 2 main test scripts: test.txt, and test2.txt. Each has comments that describe what each test is testing, and is divided into sections to test specific things we were 
doing. These helped us rerun all tests quickly every time we made changes.




































