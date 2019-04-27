# Opsys3
Casey Kwatkosky
Emilio Sillart
Cami Rios

All team members met up at the same time and coded together. Mostly it involved one person coding at a time with the other two  of us peer programming with the "typer" and then rotating periodically. This went on for dozens of sessions over a three and a half week period. However, Casey was mostly responsible  for making test cases to find bugs in the program and coding during the initial phases of the project in some solo sessions. Cami also coded the open and close sections in some solo sessions, and Emilio  was responsible for coding rmdir and rm in some solo sessions. However, not any of these sessions fully implemented the feature, and was then completed during joint peer programming sessions.

contents of tar archive:
Included in the tarball is the proj3.c file which includes the entirety of code for our project. This includes all of the function definitions and main code loop.
Also included is this README.txt file, and also an image of the "progress log" for this project.
*DO BE AWARE that we only used one GitHub account for this project. So while all commits are under Emilio, we all partook in coding the project, but relied on committing with his account only.*
Lastly, included is the makefile for this project, which compiles the code. To run the code, use "./a.out fat32.img" in the command line. The fat32 image must be passed in as a parameter during execution.

how to compile executables using the makefile:
type "make" to compile the file, then use "./a.out fat32.img" to start the program with the fat32 file image as an argument. Make sure the fat32 file image resides in the current directory, and that it is a clean copy.


known bugs:  Sometimes extra characters will print out when making directories, such as having an entry that says "..E2", but you can still ls/cd into ".." as it doesn't count the miscellaneous extra characters that randomly show up. However, this is pretty rare and the issue is fixed when you restart the program with a clean fat32.img file.

When using mkdir to create a directory with the same name as a file, the directory will be created, but an error message will print, and an extra ".." will print under it. Then, trying to ls or cd into this directory will cause problems.

Then, same issue, when using creat to make a file with the same name as a directory, the file will make, but trying to use size on the file will yield an error.

Basically, creating file names and directory names that are equal to their opposite type is possible, but causes errors when trying to do operations on them. 

For the purpose of this program, it would be best to avoid making duplicate file names of any type, even if they are different types, to ensure this program runs smoothly.

unfinished portions:
The read and write portions of the project were unimplemented due to a lack of time remaining to finish the project. In the final hours of the project, rm and rmdir were fully implemented to completion and then further edited to fix any bugs. After this, there was little time remaining to implement the read and write portions of the project.
