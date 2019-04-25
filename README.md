# Opsys3
repository for OpSys project 3

known bugs:  Sometimes extra characters will print out when making directories, such as having an entry that says "..E2", but you can still ls/cd into ".." as it doesn't count the miscellaneous extra characters that randomly show up.

When using mkdir or creat with filenames that already exist, will print an error if appropriate. But if there is a file of the opposite type, will make but will have issues using ls/cd on those files.
