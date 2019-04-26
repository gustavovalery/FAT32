# Opsys3
repository for OpSys project 3

known bugs:  Sometimes extra characters will print out when making directories, such as having an entry that says "..E2", but you can still ls/cd into ".." as it doesn't count the miscellaneous extra characters that randomly show up. However, this is pretty rare and the issue is fixed when you restart the program with a clean fat32.img file.

When using mkdir to create a directory with the same name as a file, the directory will be created, but an error message will print, and an extra ".." will print under it. Then, trying to ls or cd into this directory will cause problems.

Then, same issue, when using creat to make a file with the same name as a directory, the file will make, but trying to use size on the file will yield an error.

Basically, creating file names and directory names that are equal to their opposite type is possible, but causes errors when trying to do operations on them. 

For the purpose of this program, it would be best to avoid making duplicate file names of any type, even if they are different types, to ensure this program runs smoothly.
